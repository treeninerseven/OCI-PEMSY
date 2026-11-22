#pragma once

#include <stdint.h>

void adc_initialise(void);

/* Whole degrees Celsius, rounded to nearest integer; negative values return 0.
 * Call adc_initialise() before the first read. */
uint32_t adc_getTemp(void);
