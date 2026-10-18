#ifndef RADIO_TEST_PICO_STDLIB_H
#define RADIO_TEST_PICO_STDLIB_H
#include <stdbool.h>
#include <stdint.h>
#include "pico/types.h"
typedef uint64_t absolute_time_t;
#define GPIO_IN 0
#define GPIO_IRQ_EDGE_RISE 8u
absolute_time_t make_timeout_time_ms(uint32_t ms);
bool time_reached(absolute_time_t deadline);
void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us);
void gpio_set_irq_enabled(uint pin, uint32_t events, bool enabled);
void gpio_init(uint pin);
void gpio_set_dir(uint pin, bool out);
void gpio_pull_down(uint pin);
void gpio_acknowledge_irq(uint pin, uint32_t events);
void gpio_put(uint pin, bool value);
uint64_t time_us_64(void);
absolute_time_t from_us_since_boot(uint64_t us);
void busy_wait_until(absolute_time_t deadline);
#endif
