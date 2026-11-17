#include "led.h"
#include "pico/stdlib.h"

static bool led_valid(uint8_t pin) {
    return pin == LED1 || pin == LED2 || pin == LED3;
}

static void led_set(uint8_t pin, bool level) {
    if (!led_valid(pin)) {
        return;
    }
    /* Set the output latch before enabling the output driver. */
    gpio_put(pin, level);
    gpio_set_dir(pin, GPIO_OUT);
}

void led_all_off(void) {
    /* Neither HIGH nor LOW turns both LEDs off: release the pin instead. */
    gpio_set_dir(LED1, GPIO_IN);
    gpio_set_dir(LED2, GPIO_IN);
    gpio_set_dir(LED3, GPIO_IN);
    gpio_disable_pulls(LED1);
    gpio_disable_pulls(LED2);
    gpio_disable_pulls(LED3);
}

void led_init(void) {
    gpio_init(LED1);
    gpio_init(LED2);
    gpio_init(LED3);
    led_all_off();
}

void led_green(uint8_t led_pin) {
    led_set(led_pin, LED_GREEN_LEVEL);
}

void led_red(uint8_t led_pin) {
    led_set(led_pin, !LED_GREEN_LEVEL);
}

void led_toggle(uint8_t led_pin) {
    if (!led_valid(led_pin)) {
        return;
    }
    if (!gpio_is_dir_out(led_pin)) {
        led_green(led_pin);
        return;
    }
    led_set(led_pin, !gpio_get_out_level(led_pin));
}
