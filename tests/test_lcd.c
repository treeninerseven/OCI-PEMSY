#include "pemsy/lcd.h"
#include "pemsy/board.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <assert.h>
#include <stdio.h>

static bool levels[30];
static uint8_t nibbles[256];
static bool data_mode[256];
static unsigned int count;
static pwm_config active_config;
static uint16_t pwm_level;
static uint64_t elapsed_us;

void gpio_init(unsigned int pin) { (void)pin; }
void gpio_set_dir(unsigned int pin, bool output) { (void)pin; assert(output); }
void gpio_set_function(unsigned int pin, unsigned int function) {
    assert(pin == PEMSY_LCD_CONTRAST_PIN && function == GPIO_FUNC_PWM);
}
void gpio_put(unsigned int pin, bool value) {
    if (pin == PEMSY_LCD_ENABLE_PIN && levels[pin] && !value) {
        assert(!levels[PEMSY_LCD_RW_PIN]);
        assert(count < sizeof(nibbles));
        nibbles[count] = (uint8_t)(levels[PEMSY_LCD_D4_PIN] |
            (levels[PEMSY_LCD_D5_PIN] << 1) | (levels[PEMSY_LCD_D6_PIN] << 2) |
            (levels[PEMSY_LCD_D7_PIN] << 3));
        data_mode[count] = levels[PEMSY_LCD_RS_PIN];
        ++count;
    }
    levels[pin] = value;
}
void sleep_us(uint64_t duration) { elapsed_us += duration; }
void sleep_ms(uint32_t duration) { elapsed_us += (uint64_t)duration * 1000; }
uint32_t clock_get_hz(enum clock_index clock) { assert(clock == clk_sys); return 125000000; }
unsigned int pwm_gpio_to_slice_num(unsigned int pin) { assert(pin == 0); return 0; }
pwm_config pwm_get_default_config(void) { return (pwm_config){65535, 1}; }
void pwm_config_set_wrap(pwm_config *config, uint16_t wrap) { config->wrap = wrap; }
void pwm_config_set_clkdiv(pwm_config *config, float divider) { config->divider = divider; }
void pwm_config_set_clkdiv_mode(pwm_config *config, enum pwm_clkdiv_mode mode) {
    (void)config; assert(mode == PWM_DIV_FREE_RUNNING);
}
void pwm_config_set_phase_correct(pwm_config *config, bool enabled) { (void)config; assert(!enabled); }
void pwm_config_set_output_polarity(pwm_config *config, bool a, bool b) {
    (void)config; assert(!a && !b);
}
void pwm_init(unsigned int slice, const pwm_config *config, bool enabled) {
    assert(slice == 0 && !enabled); active_config = *config;
}
void pwm_set_gpio_level(unsigned int pin, uint16_t level) { assert(pin == 0); pwm_level = level; }
void pwm_set_enabled(unsigned int slice, bool enabled) { assert(slice == 0 && enabled); }

static uint8_t byte_at(unsigned int index, bool data) {
    assert(2 * index + 1 < count);
    assert(data_mode[2 * index] == data && data_mode[2 * index + 1] == data);
    return (uint8_t)((nibbles[2 * index] << 4) | nibbles[2 * index + 1]);
}

int main(void) {
    lcd_init();
    assert(count >= 4 && nibbles[0] == 3 && nibbles[1] == 3 &&
        nibbles[2] == 3 && nibbles[3] == 2);
    assert(elapsed_us >= 57000);
    assert(active_config.wrap == 31249 && active_config.divider == 1.0f);
    assert(pwm_level == 469); /* Closest integer to 1.5% of 31250. */
    lcd_set_contrast(1001); assert(pwm_level == 31250);
    lcd_set_contrast(0); assert(pwm_level == 0);

    count = 0;
    lcd_render("Hi", "12345678901234567");
    assert(count == 68);
    assert(byte_at(0, false) == 0x80);
    assert(byte_at(1, true) == 'H' && byte_at(2, true) == 'i');
    for (unsigned int i = 3; i <= 16; ++i) assert(byte_at(i, true) == ' ');
    assert(byte_at(17, false) == 0xc0);
    for (unsigned int i = 0; i < 16; ++i) assert(byte_at(18 + i, true) == (uint8_t)"1234567890123456"[i]);

    lcd_set_cursor(5, 1);
    count = 0;
    const uint8_t pixels[8] = {0xff, 1, 2, 3, 4, 5, 6, 7};
    lcd_create_char(3, pixels);
    assert(count == 20);
    assert(byte_at(0, false) == 0x58);
    for (unsigned int i = 0; i < 8; ++i) assert(byte_at(i + 1, true) == (pixels[i] & 31u));
    assert(byte_at(9, false) == 0xc5); /* DDRAM cursor restored after CGRAM write. */
    count = 0;
    lcd_create_char(8, pixels);
    lcd_set_cursor(16, 0);
    assert(count == 0);
    lcd_write_char(0); /* Slot zero must work through the binary character API. */
    assert(byte_at(0, true) == 0);

    puts("lcd tests passed");
    return 0;
}
