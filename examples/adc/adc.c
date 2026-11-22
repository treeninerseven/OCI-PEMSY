#include "adc.h"
#include "hardware/adc.h"

void adc_initialise(void)
{
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
}

uint32_t adc_getTemp(void)
{
    adc_select_input(4);
    uint16_t raw = adc_read();
    float voltage = raw * 3.3f / 4096.0f;
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;

    // The course interface uses an unsigned integer, so it cannot return negatives.
    if (temperature < 0.0f) {
        return 0;
    }
    return (uint32_t)(temperature + 0.5f);
}
