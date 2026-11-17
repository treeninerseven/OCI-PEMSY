#include "pemsy/radio.h"
#include "pemsy/SPI.h"
#include "pico/stdlib.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t registers[128], fifo[66];
static unsigned fifo_read, fifo_write;
static uint64_t now_us;
static bool enabled, mode_ready = true, irq_enabled;
static unsigned spi_calls;

void SPI_Initialise(void) { enabled = true; }
bool SPI_isEnabled(void) { return enabled; }
bool SPI_write(uint8_t address, const uint8_t *data, uint8_t length) {
    ++spi_calls;
    for (unsigned i = 0; i < length; ++i) {
        if (address == 0) {
            assert(fifo_write < sizeof(fifo));
            fifo[fifo_write++] = data[i];
        } else {
            registers[address + i] = data[i];
            if (address + i == 0x28 && data[i] == 0x10) {
                fifo_read = fifo_write = 0;
                registers[0x28] = 0;
            }
            if (address + i == 1) {
                registers[0x27] = mode_ready ? 0x80 : 0;
                registers[0x28] &= (uint8_t)~0x08u;
            }
        }
    }
    return true;
}
bool SPI_read(uint8_t address, uint8_t *data, uint8_t length) {
    ++spi_calls;
    for (unsigned i = 0; i < length; ++i) {
        if (address == 0) {
            assert(fifo_read < fifo_write);
            data[i] = fifo[fifo_read++];
        } else data[i] = registers[address + i];
    }
    return true;
}
absolute_time_t make_timeout_time_ms(uint32_t ms) { return now_us + ms * 1000u; }
bool time_reached(absolute_time_t deadline) { return now_us >= deadline; }
void sleep_ms(uint32_t ms) { now_us += ms * 1000u; }
void sleep_us(uint64_t us) { now_us += us; }
void gpio_set_irq_enabled(uint pin, uint32_t events, bool value) {
    assert(pin == RADIO_IRQ_PIN && events == GPIO_IRQ_EDGE_RISE);
    irq_enabled = value;
}
void gpio_init(uint pin) { (void)pin; }
void gpio_set_dir(uint pin, bool out) { (void)pin; (void)out; }
void gpio_pull_down(uint pin) { (void)pin; }
void gpio_acknowledge_irq(uint pin, uint32_t events) { (void)pin; (void)events; }

static void inject(uint8_t destination, uint8_t source, const char *text, unsigned length, bool crc) {
    fifo_read = fifo_write = 0;
    fifo[fifo_write++] = (uint8_t)(length + 2);
    fifo[fifo_write++] = destination;
    fifo[fifo_write++] = source;
    memcpy(fifo + fifo_write, text, length);
    fifo_write += length;
    registers[0x28] = crc ? 6 : 4;
    registers[0x24] = 120;
    radio_handle_irq(RADIO_IRQ_PIN, GPIO_IRQ_EDGE_RISE);
    radio_poll();
}

int main(void) {
    assert(!radio_init()); /* absent chip must not block */
    assert(!radio_is_online());
    registers[0x10] = 0x24;
    assert(radio_init());
    assert(radio_is_online() && irq_enabled && registers[1] == 0x10);
    assert(registers[0x25] == 0x40 && registers[0x37] == 0x94);
    unsigned calls = spi_calls;
    radio_handle_irq(RADIO_IRQ_PIN, GPIO_IRQ_EDGE_RISE);
    assert(spi_calls == calls); /* IRQ does no bus work */

    radio_packet_t packet;
    inject(PEMSY_NODE_ADDRESS, 2, "0123456789abcdef", 16, true);
    assert(radio_receive(&packet));
    assert(packet.sender == 2 && !packet.is_broadcast && packet.rssi_dbm == -60);
    assert(strcmp(packet.text, "0123456789abcdef") == 0);
    assert(!radio_receive(&packet));
    inject(RADIO_BROADCAST, 2, "hi", 2, true);
    assert(radio_receive(&packet) && packet.is_broadcast);
    inject(42, 2, "wrong address", 13, true);
    assert(!radio_receive(&packet));
    inject(PEMSY_NODE_ADDRESS, 2, "crc", 3, false);
    assert(!radio_receive(&packet));
    inject(PEMSY_NODE_ADDRESS, 2, "bad\0text", 8, true);
    assert(!radio_receive(&packet));
    inject(PEMSY_NODE_ADDRESS, 255, "bad sender", 10, true);
    assert(!radio_receive(&packet));
    inject(PEMSY_NODE_ADDRESS, 2, "oversize message!", 17, true);
    assert(!radio_receive(&packet));
    inject(PEMSY_NODE_ADDRESS, 2, "first", 5, true);
    inject(PEMSY_NODE_ADDRESS, 3, "second", 6, true);
    assert(radio_receive(&packet) && strcmp(packet.text, "first") == 0);

    assert(!radio_send(2, ""));
    assert(!radio_send(2, "0123456789abcdefg"));
    assert(!radio_send(2, "bad\n"));
    assert(radio_send(RADIO_BROADCAST, "hello"));
    assert(radio_busy() && registers[1] == 0x0c && registers[0x25] == 0);
    assert(fifo[0] == 7 && fifo[1] == RADIO_BROADCAST && fifo[2] == PEMSY_NODE_ADDRESS);
    assert(!radio_suspend() && !radio_send(2, "busy"));
    registers[0x28] |= 8;
    radio_poll(); /* status fallback works even without the IRQ */
    bool success;
    assert(!radio_busy() && radio_take_tx_result(&success) && success);
    assert(!radio_take_tx_result(&success));
    assert(registers[1] == 0x10 && registers[0x25] == 0x40);

    assert(radio_send(2, "timeout"));
    now_us += 300000;
    radio_poll();
    assert(radio_take_tx_result(&success) && !success);
    assert(registers[1] == 0x10 && !radio_busy());
    assert(radio_suspend() && !irq_enabled && !radio_is_online());
    assert(!radio_send(2, "suspended"));
    assert(radio_init());
    mode_ready = false;
    uint64_t before = now_us;
    assert(!radio_send(2, "mode timeout"));
    assert(now_us - before <= 30000 && !radio_is_online());
    puts("radio tests passed");
    return 0;
}
