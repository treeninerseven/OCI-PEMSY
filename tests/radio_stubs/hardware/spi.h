#ifndef RADIO_TEST_HARDWARE_SPI_H
#define RADIO_TEST_HARDWARE_SPI_H
#include <stddef.h>
#include <stdint.h>
typedef struct { unsigned unused; } spi_inst_t;
extern spi_inst_t *spi0;
int spi_write_blocking(spi_inst_t *spi, const uint8_t *data, size_t length);
int spi_write_read_blocking(spi_inst_t *spi, const uint8_t *data, uint8_t *response, size_t length);
#endif
