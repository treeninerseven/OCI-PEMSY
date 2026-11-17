#include "pico/stdlib.h"
#include "lcd.h"

int main(void)
{
    stdio_init_all();
    lcd_init();
    lcd_locate(0, 0);
    lcd_print("PWM contrast");
    sleep_ms(3000);
    lcd_cycle_pwm();
    while (1)
    {
        tight_loop_contents();
    }
}
