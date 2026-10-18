#include "pemsy/temperature.h"

#include "hardware/adc.h"
#include "pico/stdlib.h"

void temperature_init(void) {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    sleep_us(20);
}

float temperature_adc_to_c(float adc_code) {
    const float voltage = adc_code * (3.3f / 4096.0f);
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

float temperature_read_c(void) {
    uint32_t sum = 0;
    adc_select_input(4);
    (void)adc_read(); /* Discard the first conversion after selecting the channel. */
    for (unsigned int sample = 0; sample < 16; ++sample) {
        sum += adc_read();
    }
    return temperature_adc_to_c((float)sum / 16.0f);
}
