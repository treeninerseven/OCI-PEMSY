#pragma once

#include <stdint.h>

#define LCD_CONTR 0
#define LCD_RS 3
#define LCD_RW 2
#define LCD_E 1
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7

void lcd_writeNibble(char wNibble);

void lcd_writeByte(char wByte);

void lcd_writeCommand(char wCommand);

void lcd_writedate(char wByte);

void lcd_init(void);

void lcd_print(char *str);

void lcd_locate(uint8_t row, uint8_t col);

/* slot: 0..7; pixels: 8 rows, with the lowest 5 bits used in each row.
 * Returns the write position to the top-left corner after loading CGRAM. */
void lcd_createChar(uint8_t slot, const uint8_t pixels[8]);
