#include "pemsy/radio.h"
#include "pemsy/SPI.h"
#include "pico/stdlib.h"
#include <string.h>

_Static_assert(PEMSY_NODE_ADDRESS >= 1 && PEMSY_NODE_ADDRESS < RADIO_BROADCAST,
               "Node address must be 1..254");
_Static_assert(PEMSY_RADIO_FREQUENCY_HZ >= 869800000u &&
               PEMSY_RADIO_FREQUENCY_HZ <= 869900000u &&
               (PEMSY_RADIO_FREQUENCY_HZ - 869800000u) % 20000u == 0,
               "Choose a group frequency from script table 10.1");

enum { REG_FIFO = 0x00, REG_MODE = 0x01, REG_VERSION = 0x10,
       REG_RSSI = 0x24, REG_IRQ1 = 0x27, REG_IRQ2 = 0x28,
       MODE_STANDBY = 0x04, MODE_TX = 0x0c, MODE_RX = 0x10 };
static bool online, transmitting, result_pending, tx_success, packet_pending;
static volatile bool irq_pending;
static absolute_time_t tx_deadline;
static radio_packet_t received;

static uint8_t read_reg(uint8_t address) {
    uint8_t value = 0;
    SPI_read(address, &value, 1);
    return value;
}
static void write_reg(uint8_t address, uint8_t value) {
    SPI_write(address, &value, 1);
}
static bool set_mode(uint8_t mode) {
    write_reg(REG_MODE, mode);
    absolute_time_t deadline = make_timeout_time_ms(25);
    do {
        if (read_reg(REG_IRQ1) & 0x80) return true;
        sleep_us(100);
    } while (!time_reached(deadline));
    online = false;
    return false;
}
static bool standby(void) {
    if (!set_mode(MODE_STANDBY)) return false;
    sleep_ms(2);
    return true;
}
static bool resume_rx(void) {
    if (!standby()) return false;
    write_reg(REG_IRQ2, 0x10); /* W1C FIFO overrun also clears FIFO. */
    write_reg(0x25, 0x40); /* DIO0: PayloadReady in receive mode. */
    irq_pending = false;
    return set_mode(MODE_RX);
}

bool radio_test_spi(void) {
    static const uint8_t pattern[16] = {
        0x50, 0x45, 0x4d, 0x53, 0x59, 0x55, 0xaa, 0x01,
        0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xff
    };
    uint8_t result[sizeof(pattern)];
    if (!standby()) return false;
    write_reg(REG_IRQ2, 0x10);
    bool ok = SPI_write(REG_FIFO, pattern, sizeof(pattern)) &&
              SPI_read(REG_FIFO, result, sizeof(result)) &&
              memcmp(pattern, result, sizeof(pattern)) == 0;
    write_reg(REG_IRQ2, 0x10);
    return ok;
}

bool radio_init(void) {
    gpio_set_irq_enabled(RADIO_IRQ_PIN, GPIO_IRQ_EDGE_RISE, false);
    online = transmitting = packet_pending = result_pending = irq_pending = false;
    SPI_Initialise();
    if (read_reg(REG_VERSION) != 0x24) return false;
    if (!standby()) return false;
    /* Explicitly reset settings changed by mioty, which uses continuous mode. */
    static const uint8_t config[][2] = {
        {0x02, 0x00}, /* packet FSK, no shaping */
        {0x03, 0x1a}, {0x04, 0x0b}, /* 4800 bit/s */
        {0x05, 0x00}, {0x06, 0x52}, /* +/- 5005 Hz deviation */
        {0x12, 0x09}, {0x13, 0x1a}, /* PA ramp 40 us, OCP enabled */
        {0x18, 0x88}, /* LNA 50 ohm, automatic gain */
        {0x19, 0x55}, {0x1a, 0x55}, /* RX/AFC BW 10.4 kHz */
        {0x1e, 0x00}, /* AFC disabled; no stale correction */
        {0x25, 0x40}, {0x26, 0x07}, /* DIO0 PayloadReady, clock off */
        {0x29, 0xe4}, {0x2a, 0x00}, {0x2b, 0x00},
        {0x2c, 0x00}, {0x2d, 0x03}, /* three preamble bytes */
        {0x2e, 0x88}, {0x2f, 0x2d}, {0x30, 0xd4}, /* two sync bytes */
        {0x37, 0x94}, /* variable, CRC on/auto clear, node+broadcast */
        {0x38, RADIO_MAX_TEXT + 2}, {0x39, PEMSY_NODE_ADDRESS},
        {0x3a, RADIO_BROADCAST}, {0x3b, 0x00}, {0x3c, 0x8f},
        {0x3d, 0x02}, /* auto RX restart, AES disabled */
        {0x58, 0x1b}, {0x5a, 0x55}, {0x5c, 0x70}, /* normal PA tests */
        {0x6f, 0x30} /* improved digital DAGC */
    };
    for (unsigned i = 0; i < sizeof(config) / sizeof(config[0]); ++i)
        write_reg(config[i][0], config[i][1]);
    uint32_t frf = (uint32_t)(((uint64_t)PEMSY_RADIO_FREQUENCY_HZ << 19) / 32000000u);
    uint8_t frequency[] = {(uint8_t)(frf >> 16), (uint8_t)(frf >> 8), (uint8_t)frf};
    SPI_write(0x07, frequency, sizeof(frequency));
    /* W: PA0 at -8 dBm. HW: PA1's minimum supported power is -2 dBm. */
    write_reg(0x11, PEMSY_RFM69_HIGH_POWER ? (0x40 | 16) : (0x80 | 10));
    if (!radio_test_spi()) return false;
    gpio_init(RADIO_IRQ_PIN);
    gpio_set_dir(RADIO_IRQ_PIN, GPIO_IN);
    gpio_pull_down(RADIO_IRQ_PIN);
    online = true;
    if (!resume_rx()) return false;
    gpio_acknowledge_irq(RADIO_IRQ_PIN, GPIO_IRQ_EDGE_RISE);
    gpio_set_irq_enabled(RADIO_IRQ_PIN, GPIO_IRQ_EDGE_RISE, true);
    return true;
}

void radio_handle_irq(uint gpio, uint32_t events) {
    if (gpio == RADIO_IRQ_PIN && (events & GPIO_IRQ_EDGE_RISE)) irq_pending = true;
}

bool radio_send(uint8_t recipient, const char *message) {
    if (!online || transmitting || result_pending || !message) return false;
    size_t length = 0;
    while (length <= RADIO_MAX_TEXT && message[length]) ++length;
    if (!length || length > RADIO_MAX_TEXT) return false;
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)message[i] < 32 || (unsigned char)message[i] > 126) return false;
    radio_poll(); /* preserve any already received packet before clearing FIFO */
    if (!online || !standby()) return false;
    write_reg(REG_IRQ2, 0x10);
    uint8_t frame[RADIO_MAX_TEXT + 3] = {
        (uint8_t)(length + 2), recipient, PEMSY_NODE_ADDRESS
    };
    memcpy(frame + 3, message, length);
    SPI_write(REG_FIFO, frame, (uint8_t)(length + 3));
    write_reg(0x25, 0x00); /* DIO0: PacketSent in transmit mode. */
    irq_pending = false;
    transmitting = true;
    tx_deadline = make_timeout_time_ms(250);
    if (!set_mode(MODE_TX)) {
        transmitting = false;
        return false;
    }
    return true;
}

void radio_poll(void) {
    if (!online) return;
    /* Poll status as well as the edge flag, recovering a missed GPIO edge. */
    irq_pending = false;
    uint8_t flags = read_reg(REG_IRQ2);
    if (transmitting) {
        if ((flags & 0x08) || time_reached(tx_deadline)) {
            tx_success = (flags & 0x08) != 0;
            transmitting = false;
            if (!resume_rx()) tx_success = false;
            result_pending = true;
        }
        return;
    }
    if (flags & 0x10) { resume_rx(); return; }
    if (!(flags & 0x04)) return;
    if (!(flags & 0x02) || !standby()) { resume_rx(); return; }
    uint8_t length = read_reg(REG_FIFO);
    if (length >= 3 && length <= RADIO_MAX_TEXT + 2) {
        uint8_t frame[RADIO_MAX_TEXT + 2];
        SPI_read(REG_FIFO, frame, length);
        bool printable = true;
        for (unsigned i = 2; i < length; ++i)
            if (frame[i] < 32 || frame[i] > 126) printable = false;
        if ((frame[0] == PEMSY_NODE_ADDRESS || frame[0] == RADIO_BROADCAST) &&
            frame[1] != 0 && frame[1] != RADIO_BROADCAST && printable && !packet_pending) {
            received.sender = frame[1];
            received.is_broadcast = frame[0] == RADIO_BROADCAST;
            memcpy(received.text, frame + 2, length - 2);
            received.text[length - 2] = '\0';
            received.rssi_dbm = -(int16_t)read_reg(REG_RSSI) / 2;
            packet_pending = true;
        }
    }
    resume_rx();
}

bool radio_receive(radio_packet_t *packet) {
    if (!packet || !packet_pending) return false;
    *packet = received;
    packet_pending = false;
    return true;
}
bool radio_busy(void) { return transmitting; }
bool radio_is_online(void) { return online; }
bool radio_take_tx_result(bool *success) {
    if (!success || !result_pending) return false;
    *success = tx_success;
    result_pending = false;
    return true;
}
bool radio_suspend(void) {
    if (!online || transmitting || result_pending) return false;
    gpio_set_irq_enabled(RADIO_IRQ_PIN, GPIO_IRQ_EDGE_RISE, false);
    bool ok = standby();
    online = false;
    return ok;
}
