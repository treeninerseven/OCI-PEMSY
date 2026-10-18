#ifndef PEMSY_TEMPERATURE_H
#define PEMSY_TEMPERATURE_H

/* Measures approximate chip temperature, not calibrated ambient temperature.
 * The main loop owns the ADC; do not call concurrently with another ADC user. */
void temperature_init(void);
float temperature_read_c(void);
/* Supports fractional ADC codes when averaging multiple 12-bit samples. */
float temperature_adc_to_c(float adc_code);

#endif
