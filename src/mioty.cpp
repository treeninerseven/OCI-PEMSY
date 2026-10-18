#include "pemsy/mioty.h"

#if defined(PEMSY_ENABLE_MIOTY) && PEMSY_ENABLE_MIOTY
#include "pemsy/radio.h"
#include "pemsy/SPI.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "TsUnb/RadioBurst.h"
#include "TsUnb/FixedMac.h"
#include "TsUnb/Phy.h"
#include "TsUnb/SimpleNode.h"
#include "Trx/Rfm69hw.h"

#ifndef PEMSY_MIOTY_CONFIG_HEADER
#define PEMSY_MIOTY_CONFIG_HEADER "mioty_credentials.h"
#endif
#include PEMSY_MIOTY_CONFIG_HEADER
#ifndef PEMSY_MIOTY_TX_DBM
#define PEMSY_MIOTY_TX_DBM 10
#endif
static_assert(PEMSY_MIOTY_TX_DBM >= -2 && PEMSY_MIOTY_TX_DBM <= 13,
              "mioty transmit power must be in the shared W/HW range -2..13 dBm");

namespace {
/* Platform implementation for the supplied generic TS-UNB templates. It uses
 * the hardware microsecond clock, so an unavailable alarm slot cannot hang TX.
 * Symbol deadlines use integer rational arithmetic to avoid drift. */
class PemsyCpu {
public:
    bool failed = false;
    void spiInit() { SPI_Initialise(); }
    void spiDeinit() { gpio_put(17, true); }
    void spiSend(const uint8_t *data, uint8_t count) {
        if (!data || !count) { failed = true; return; }
        const bool sleep_command = count == 2 && data[0] == 0x81 && data[1] == 0;
        if (failed && !sleep_command) return;
        // Upstream's standby/FS constants are mode numbers; OpMode uses bits4:2.
        uint8_t corrected[2];
        if (count == 2 && data[0] == 0x81 && (data[1] == 1 || data[1] == 2)) {
            corrected[0] = data[0];
            corrected[1] = (uint8_t)(data[1] << 2);
            data = corrected;
        }
        gpio_put(17, false);
        int result = spi_write_blocking(spi0, data, count);
        gpio_put(17, true);
        if (result != count) failed = true;
    }
    void spiSendReceive(uint8_t *data, uint8_t count) {
        uint8_t response[255];
        if (!data || !count || failed) return;
        gpio_put(17, false);
        int result = spi_write_read_blocking(spi0, data, response, count);
        gpio_put(17, true);
        if (result != count) { failed = true; return; }
        for (unsigned i = 0; i < count; ++i) data[i] = response[i];
    }
    void initTimer() { symbols = 1; failed = false; invalid_delay = false; }
    void startTimer() { started_us = time_us_64(); }
    void stopTimer() {}
    void addTimerDelay(int32_t count) {
        // Upstream schedules one unused negative delay after its final burst.
        // Reject it only if another wait actually attempts to consume it.
        if (count < 1) { invalid_delay = true; return; }
        symbols += (uint32_t)(count - 1);
    }
    void waitTimer() {
        if (failed) return;
        if (invalid_delay) { failed = true; return; }
        // Rate = 49.591064453125 * 48 = 2380.37109375 symbols/s.
        uint64_t target = started_us + (symbols * 2048000000ull + 2437500ull) / 4875000ull;
        uint64_t now = time_us_64();
        if (target - started_us > 60000000ull || now > target + 2000ull) {
            failed = true;
            return;
        }
        busy_wait_until(from_us_since_boot(target));
        ++symbols;
    }
    void resetWatchdog() {}
private:
    uint64_t started_us = 0;
    uint64_t symbols = 1;
    bool invalid_delay = false;
};

using namespace TsUnbLib;
using Burst = TsUnb::RadioBurst<2, 2>;
using Phy = TsUnb::Phy<14224261, 14222623, 39, 39, TsUnb::TsUnb_UPG1, 0, 3, Burst>;
using Tx = Trx::Rfm69hw<PemsyCpu, PEMSY_RFM69_HIGH_POWER != 0, 10, Burst>;
TsUnb::SimpleNode<TsUnb::FixedUplinkMac, Phy, Tx, false> node;
bool ready;
bool counter_reserved;
}

extern "C" bool mioty_is_enabled(void) { return true; }
extern "C" bool mioty_setup(void) {
    ready = counter_reserved = false;
    node.Tx.Cpu.failed = false;
    if (node.init() < 0 || node.Tx.Cpu.failed) return false;
    node.Tx.setTxPower(PEMSY_MIOTY_TX_DBM);
    node.Mac.setNetworkKey(PEMSY_MIOTY_NETWORK_KEY);
    node.Mac.setEui64(PEMSY_MIOTY_EUI64);
    node.Mac.setShortAddress(PEMSY_MIOTY_SHORT_ADDRESS);
    ready = true;
    return true;
}
extern "C" void mioty_set_counter(uint32_t counter) {
    // Reject rollover of the over-the-air 24-bit counter; re-provision first.
    counter_reserved = ready && counter > 0 && counter <= 0x00ffffffu;
    if (counter_reserved) node.Mac.extPkgCnt = counter;
}
extern "C" uint32_t mioty_next_counter(void) { return node.Mac.extPkgCnt; }
extern "C" bool mioty_ep_to_base(const char *message) {
    if (!ready || !counter_reserved || !message) return false;
    size_t length = 0;
    while (length <= RADIO_MAX_TEXT && message[length]) ++length;
    if (!length || length > RADIO_MAX_TEXT) return false;
    counter_reserved = false; // Every attempt consumes its reserved counter.
    int16_t result = node.send(reinterpret_cast<const uint8_t *>(message), length);
    ready = false; // Peer operation will change hardware settings afterwards.
    return result >= 0 && !node.Tx.Cpu.failed;
}
#else
extern "C" bool mioty_is_enabled(void) { return false; }
extern "C" bool mioty_setup(void) { return false; }
extern "C" bool mioty_ep_to_base(const char *) { return false; }
extern "C" void mioty_set_counter(uint32_t) {}
extern "C" uint32_t mioty_next_counter(void) { return 0; }
#endif
