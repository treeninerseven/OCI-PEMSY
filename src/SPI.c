#include "pemsy/SPI.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

enum { SPI_RX_PIN = 16, SPI_CS_PIN = 17, SPI_CLOCK_PIN = 18, SPI_TX_PIN = 19 };
static bool enabled;

void SPI_Initialise(void) {
    gpio_init(SPI_CS_PIN);
    gpio_put(SPI_CS_PIN, true);
    gpio_set_dir(SPI_CS_PIN, GPIO_OUT);
    gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_CLOCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
    spi_init(spi0, 4000000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    enabled = true;
}

bool SPI_isEnabled(void) { return enabled; }

bool SPI_write(uint8_t address, const uint8_t *data, uint8_t length) {
    if (!enabled || !data || !length) return false;
    address |= 0x80;
    gpio_put(SPI_CS_PIN, false);
    int command = spi_write_blocking(spi0, &address, 1);
    int result = spi_write_blocking(spi0, data, length);
    gpio_put(SPI_CS_PIN, true);
    return command == 1 && result == length;
}

bool SPI_read(uint8_t address, uint8_t *data, uint8_t length) {
    if (!enabled || !data || !length) return false;
    address &= 0x7f;
    gpio_put(SPI_CS_PIN, false);
    int command = spi_write_blocking(spi0, &address, 1);
    int result = spi_read_blocking(spi0, 0, data, length);
    gpio_put(SPI_CS_PIN, true);
    return command == 1 && result == length;
}
