#ifndef PEMSY_LED_H
#define PEMSY_LED_H

#include <stdint.h>

#define LED1 11
#define LED2 14
#define LED3 15

/* Assumption: HIGH lights green. Set to 0 if your board is the reverse. */
#define LED_GREEN_LEVEL 1

void led_init(void);
void led_green(uint8_t led_pin);
void led_red(uint8_t led_pin);
void led_all_off(void);
/* An off group starts with green on its first toggle. */
void led_toggle(uint8_t led_pin);

#endif
