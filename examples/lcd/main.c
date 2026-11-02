#include "pico/stdlib.h"
#include "lcd.h"
#include "mioty_logo.h"

int main(void)
{
    lcd_init();
    lcd_show_mioty_logo();
    while (1)
    {
        tight_loop_contents();
    }
}
