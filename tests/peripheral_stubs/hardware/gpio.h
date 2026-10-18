#ifndef TEST_GPIO_H
#define TEST_GPIO_H
#include <stdbool.h>
#include <stdint.h>
#define GPIO_IN 0
#define GPIO_OUT 1
#define GPIO_IRQ_EDGE_FALL 4u
#define GPIO_IRQ_EDGE_RISE 8u
#define GPIO_FUNC_PWM 4u
void gpio_init(unsigned int pin);
void gpio_set_dir(unsigned int pin, bool output);
void gpio_pull_up(unsigned int pin);
bool gpio_get(unsigned int pin);
void gpio_put(unsigned int pin, bool value);
void gpio_set_function(unsigned int pin, unsigned int function);
void gpio_set_irq_enabled(unsigned int pin, uint32_t events, bool enabled);
#endif
