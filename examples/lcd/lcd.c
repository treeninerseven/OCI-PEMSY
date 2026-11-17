#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "lcd.h"

void lcd_init_pwm(void)
{
    // FAST PWM
    gpio_set_function(LCD_CONTR, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LCD_CONTR);
    int top=125000000/4000-1;
    int level=(top+1)*0.015;
    pwm_set_wrap(0, top);
    pwm_set_gpio_level(LCD_CONTR, level);
    pwm_set_enabled(slice, true);
}

void lcd_cycle_pwm(void)
{
    int top = 125000000 / 4000 - 1;

    for (int percent = 0; percent <= 100; percent++)
    {
        int level = (top + 1) * percent / 100;
        pwm_set_gpio_level(LCD_CONTR, level);
        printf("Duty cycle: %d%%, level: %d\n", percent, level);
        sleep_ms(500);
    }

    // Restore approximately 1.5% after the demonstration.
    pwm_set_gpio_level(LCD_CONTR, 469);
}

void lcd_writeNibble(char wNibble)
{
    gpio_put(LCD_D4, (wNibble >> 0) & 1);
    gpio_put(LCD_D5, (wNibble >> 1) & 1);
    gpio_put(LCD_D6, (wNibble >> 2) & 1);
    gpio_put(LCD_D7, (wNibble >> 3) & 1);

    sleep_us(1);
    gpio_put(LCD_E, 1);
    sleep_us(1);
    gpio_put(LCD_E, 0);
    sleep_us(1);
}

void lcd_writeByte(char wByte)
{
    char Nibble_low = wByte & 0x0F;
    char Nibble_high = wByte >> 4 & 0x0F;

    lcd_writeNibble(Nibble_high);
    lcd_writeNibble(Nibble_low);
}

void lcd_writeCommand(char wCommand)
{
    gpio_put(LCD_RS, 0);
    gpio_put(LCD_RW, 0);
    lcd_writeByte(wCommand);
    sleep_ms(5);
}

void lcd_writedate(char wDate)
{
    gpio_put(LCD_RS, 1);
    gpio_put(LCD_RW, 0);
    lcd_writeByte(wDate);
    sleep_ms(5);
}

void lcd_init(void)
{
    lcd_init_pwm();

    const uint8_t pins[] = {
        LCD_E, LCD_RW, LCD_RS, LCD_D4, LCD_D5, LCD_D6, LCD_D7};

    for (uint8_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i)
    {
        gpio_init(pins[i]);
        gpio_put(pins[i], 0);
        gpio_set_dir(pins[i], GPIO_OUT);
    }

    sleep_ms(50);

    gpio_put(LCD_RS, 0);
    gpio_put(LCD_RW, 0);

    for (int i = 0; i < 3; ++i)
    {
        lcd_writeNibble(0x03);
        sleep_ms(5);
    }

    lcd_writeNibble(0x02);
    sleep_ms(5);

    lcd_writeCommand(0x28);
    lcd_writeCommand(0x06);
    lcd_writeCommand(0x01);
    lcd_writeCommand(0x0F);
}

void lcd_print(char *str){
    while (*str != '\0') {
        lcd_writedate(*str);
        str++;
    }
}


// task 6.7-1
void lcd_locate(uint8_t row, uint8_t col){
    if(row >= 2 || col >= 16){
        return;
    }
    uint8_t address;
    if(row==0){
        address=col;
    }
    else{
        address=0x40 + col;
    }
    lcd_writeCommand(0x80 | address );
}

// Task 6.8: define one 5 x 8 character in CGRAM.
void lcd_createChar(uint8_t slot, const uint8_t pixels[8])
{
    if (slot >= 8) {
        return;
    }

    lcd_writeCommand(0x40 + slot * 8);
    for (int row = 0; row < 8; row++) {
        lcd_writedate(pixels[row] & 0x1F);
    }

    // Select display memory again before printing characters.
    lcd_locate(0, 0);
}
