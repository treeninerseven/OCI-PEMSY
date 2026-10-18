#include "pemsy/temperature.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static unsigned int reads;
static bool initialized;
static bool sensor_enabled;
static unsigned int selected;

void adc_init(void) { initialized = true; }
void adc_set_temp_sensor_enabled(bool enabled) { sensor_enabled = enabled; }
void adc_select_input(unsigned int input) { selected = input; }
uint16_t adc_read(void) {
    assert(initialized && sensor_enabled && selected == 4);
    return reads++ == 0 ? 0 : 876;
}
void sleep_us(uint64_t duration) { assert(duration >= 20); }

int main(void) {
    const float code_27 = 0.706f * 4096.0f / 3.3f;
    assert(fabsf(temperature_adc_to_c(code_27) - 27.0f) < 0.001f);
    assert(temperature_adc_to_c(code_27 - 10.0f) > 27.0f);
    assert(temperature_adc_to_c(code_27 + 10.0f) < 27.0f);
    temperature_init();
    const float temperature = temperature_read_c();
    assert(reads == 17); /* One discarded sample plus sixteen averaged samples. */
    assert(fabsf(temperature - temperature_adc_to_c(876.0f)) < 0.001f);
    assert(temperature > 26.0f && temperature < 28.0f);
    puts("temperature tests passed");
    return 0;
}
