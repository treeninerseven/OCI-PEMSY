#ifndef TEST_PWM_H
#define TEST_PWM_H
#include <stdbool.h>
#include <stdint.h>
enum pwm_clkdiv_mode { PWM_DIV_FREE_RUNNING };
typedef struct { uint16_t wrap; float divider; } pwm_config;
unsigned int pwm_gpio_to_slice_num(unsigned int pin);
pwm_config pwm_get_default_config(void);
void pwm_config_set_wrap(pwm_config *config, uint16_t wrap);
void pwm_config_set_clkdiv(pwm_config *config, float divider);
void pwm_config_set_clkdiv_mode(pwm_config *config, enum pwm_clkdiv_mode mode);
void pwm_config_set_phase_correct(pwm_config *config, bool enabled);
void pwm_config_set_output_polarity(pwm_config *config, bool a, bool b);
void pwm_init(unsigned int slice, const pwm_config *config, bool enabled);
void pwm_set_gpio_level(unsigned int pin, uint16_t level);
void pwm_set_enabled(unsigned int slice, bool enabled);
#endif
