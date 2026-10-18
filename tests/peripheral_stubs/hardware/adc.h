#ifndef TEST_ADC_H
#define TEST_ADC_H
#include <stdbool.h>
#include <stdint.h>
void adc_init(void);
void adc_set_temp_sensor_enabled(bool enabled);
void adc_select_input(unsigned int input);
uint16_t adc_read(void);
#endif
