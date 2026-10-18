#include "pemsy/lcd.h"
#include "pemsy/board.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

static uint8_t cursor_address;
#define LCD_PWM_PERIOD 31250u

static void write_nibble(uint8_t nibble) {
    gpio_put(PEMSY_LCD_D4_PIN, (nibble >> 0u) & 1u);
    gpio_put(PEMSY_LCD_D5_PIN, (nibble >> 1u) & 1u);
    gpio_put(PEMSY_LCD_D6_PIN, (nibble >> 2u) & 1u);
    gpio_put(PEMSY_LCD_D7_PIN, (nibble >> 3u) & 1u);
    sleep_us(1);
    gpio_put(PEMSY_LCD_ENABLE_PIN, true);
    sleep_us(1);
    gpio_put(PEMSY_LCD_ENABLE_PIN, false);
    sleep_us(1);
}

static void write_byte(uint8_t value, bool data) {
    gpio_put(PEMSY_LCD_RS_PIN, data);
    gpio_put(PEMSY_LCD_RW_PIN, false);
    write_nibble(value >> 4u);
    write_nibble(value & 0x0fu);
    /* Busy-flag reads would expose a 3.3V GPIO to the 5V LCD data bus. */
    sleep_us(!data && (value == 0x01u || value == 0x02u) ? 2000 : 50);
}

static void contrast_init(void) {
    /* 125MHz / (1 * 31250) = 4kHz; deriving the divider also supports 133MHz. */
    const uint16_t period = LCD_PWM_PERIOD;
    const unsigned int slice = pwm_gpio_to_slice_num(PEMSY_LCD_CONTRAST_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, period - 1u);
    pwm_config_set_clkdiv(&config,
        (float)clock_get_hz(clk_sys) / (4000.0f * (float)period));
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
    pwm_config_set_phase_correct(&config, false);
    pwm_config_set_output_polarity(&config, false, false);
    gpio_set_function(PEMSY_LCD_CONTRAST_PIN, GPIO_FUNC_PWM);
    pwm_init(slice, &config, false);
    lcd_set_contrast(15);
    pwm_set_enabled(slice, true);
}

void lcd_set_contrast(uint16_t permille) {
    if (permille > 1000u) {
        permille = 1000u;
    }
    pwm_set_gpio_level(PEMSY_LCD_CONTRAST_PIN,
                       (LCD_PWM_PERIOD * permille + 500u) / 1000u);
}

void lcd_init(void) {
    const unsigned int pins[] = {
        PEMSY_LCD_ENABLE_PIN, PEMSY_LCD_RW_PIN, PEMSY_LCD_RS_PIN,
        PEMSY_LCD_D4_PIN, PEMSY_LCD_D5_PIN, PEMSY_LCD_D6_PIN, PEMSY_LCD_D7_PIN
    };
    for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        gpio_init(pins[i]);
        gpio_put(pins[i], false);
        gpio_set_dir(pins[i], GPIO_OUT);
    }
    contrast_init();
    sleep_ms(50);
    /* Recover a known 8-bit state even after an MCU-only reset, then select 4-bit. */
    write_nibble(0x03);
    sleep_ms(5);
    write_nibble(0x03);
    sleep_us(150);
    write_nibble(0x03);
    sleep_us(150);
    write_nibble(0x02);
    sleep_us(150);
    write_byte(0x28, false); /* 4-bit bus, two lines, 5x8 font. */
    write_byte(0x08, false); /* Display off during initialization. */
    lcd_clear();
    write_byte(0x06, false); /* Increment address, no display shift. */
    lcd_set_cursor_visible(false, false);
}

void lcd_clear(void) {
    write_byte(0x01, false);
    cursor_address = 0;
}

void lcd_set_cursor(uint8_t column, uint8_t row) {
    if (column >= PEMSY_LCD_COLUMNS || row >= PEMSY_LCD_ROWS) {
        return;
    }
    cursor_address = (uint8_t)((row == 0 ? 0x00u : 0x40u) + column);
    write_byte((uint8_t)(0x80u | cursor_address), false);
}

void lcd_set_cursor_visible(bool visible, bool blink) {
    write_byte((uint8_t)(0x0cu | (visible ? 0x02u : 0u) | (blink ? 0x01u : 0u)), false);
}

void lcd_write_char(uint8_t character) {
    write_byte(character, true);
    cursor_address = (uint8_t)((cursor_address + 1u) & 0x7fu);
}

void lcd_write(const char *text) {
    if (text == NULL) {
        return;
    }
    while (*text != '\0') {
        lcd_write_char((uint8_t)*text++);
    }
}

void lcd_render(const char *row0, const char *row1) {
    const char *rows[] = {row0, row1};
    for (uint8_t row = 0; row < PEMSY_LCD_ROWS; ++row) {
        const char *text = rows[row];
        lcd_set_cursor(0, row);
        for (uint8_t column = 0; column < PEMSY_LCD_COLUMNS; ++column) {
            lcd_write_char(text != NULL && *text != '\0' ? (uint8_t)*text++ : ' ');
        }
    }
}

void lcd_create_char(uint8_t slot, const uint8_t pixels[8]) {
    if (slot >= 8 || pixels == NULL) {
        return;
    }
    write_byte((uint8_t)(0x40u | (slot << 3u)), false);
    for (unsigned int row = 0; row < 8; ++row) {
        write_byte(pixels[row] & 0x1fu, true);
    }
    write_byte((uint8_t)(0x80u | cursor_address), false);
}
