#ifndef PEMSY_SPI_H
#define PEMSY_SPI_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void SPI_Initialise(void);
bool SPI_isEnabled(void);
bool SPI_write(uint8_t address, const uint8_t *data, uint8_t length);
bool SPI_read(uint8_t address, uint8_t *data, uint8_t length);
#ifdef __cplusplus
}
#endif
#endif
