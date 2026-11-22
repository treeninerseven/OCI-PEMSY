#include <inttypes.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "adc.h"
#include "lcd.h"

int main(void)
{
    stdio_init_all();
    lcd_init();
    adc_initialise();

    lcd_writeCommand(0x0C); // Display on, cursor off.
    lcd_locate(0, 0);
    lcd_print("Pico temperature");

    while (true) {
        uint32_t temperature = adc_getTemp();
        char line[17];
        snprintf(line, sizeof(line), "Temp: %3" PRIu32 " C     ", temperature);
        lcd_locate(1, 0);
        lcd_print(line);
        printf("Pico temperature: %" PRIu32 " C\n", temperature);
        sleep_ms(1000);
    }
}
