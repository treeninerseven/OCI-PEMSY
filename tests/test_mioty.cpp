#include "pemsy/mioty.h"
#include "pemsy/SPI.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static uint64_t now_us;
static bool present = true, late;
static unsigned tx_count, fs_count, writes;
static uint8_t last_mode;
static spi_inst_t spi_instance;
spi_inst_t *spi0 = &spi_instance;

void SPI_Initialise(void) {}
void gpio_put(uint pin, bool value) { (void)pin; (void)value; }
uint64_t time_us_64(void) { return now_us; }
absolute_time_t from_us_since_boot(uint64_t us) { return us; }
void busy_wait_until(absolute_time_t deadline) { now_us = deadline + (late ? 3000 : 0); }
int spi_write_blocking(spi_inst_t *, const uint8_t *data, size_t length) {
    ++writes;
    if (length == 2 && data[0] == 0x81) {
        last_mode = data[1];
        if (last_mode == 0x0c) ++tx_count;
        if (last_mode == 0x08) ++fs_count;
        assert(last_mode != 0x01 && last_mode != 0x02);
    }
    return (int)length;
}
int spi_write_read_blocking(spi_inst_t *, const uint8_t *data, uint8_t *response, size_t length) {
    std::memset(response, 0, length);
    if (length == 2 && data[0] == 0x0c && present) response[1] = 0x02;
    return (int)length;
}

int main() {
    assert(mioty_is_enabled());
    present = false;
    assert(!mioty_setup());
    present = true;
    assert(mioty_setup());
    assert(!mioty_ep_to_base("no counter"));
    mioty_set_counter(0);
    assert(!mioty_ep_to_base("zero counter"));
    mioty_set_counter(0x01000000u);
    assert(!mioty_ep_to_base("overflow"));
    mioty_set_counter(42);
    assert(!mioty_ep_to_base("0123456789abcdefg"));
    assert(mioty_ep_to_base("T: 23.5 C"));
    assert(tx_count >= 24 && tx_count == fs_count && last_mode == 0);
    assert(now_us > 1000000 && now_us < 60000000);
    assert(mioty_next_counter() == 43);
    assert(!mioty_ep_to_base("reuse"));
    assert(mioty_setup());
    mioty_set_counter(43);
    unsigned before = writes;
    late = true;
    assert(!mioty_ep_to_base("late"));
    assert(last_mode == 0 && writes > before);
    assert(mioty_next_counter() == 44);
    std::puts("mioty adapter tests passed");
}
