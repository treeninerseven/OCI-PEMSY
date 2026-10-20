#include "pico/stdlib.h"
#include "led.h"

int main(void) {
    led_init();

    /* Milestone 4.4: all green, all red, then all off, two seconds each. */
    led_green(LED1);
    led_green(LED2);
    led_green(LED3);
    sleep_ms(2000);

    led_red(LED1);
    led_red(LED2);
    led_red(LED3);
    sleep_ms(2000);

    led_all_off();
    sleep_ms(2000);

    /* Milestone 4.5: change all three groups' colors every second. */
    while (true) {
        led_toggle(LED1);
        led_toggle(LED2);
        led_toggle(LED3);
        sleep_ms(1000);
    }
}
