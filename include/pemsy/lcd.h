#ifndef PEMSY_LCD_H
#define PEMSY_LCD_H

#include <stdbool.h>
#include <stdint.h>

#define PEMSY_LCD_COLUMNS 16u
#define PEMSY_LCD_ROWS 2u

/* Main-loop only: commands include the HD44780's required settling delays. */
void lcd_init(void);
/* Contrast duty cycle in thousandths, clamped to 0..1000; default is 15 (1.5%). */
void lcd_set_contrast(uint16_t permille);
void lcd_clear(void);
void lcd_set_cursor(uint8_t column, uint8_t row);
void lcd_set_cursor_visible(bool visible, bool blink);
void lcd_write_char(uint8_t character);
void lcd_write(const char *text);
/* Clips and space-pads each row to 16 characters. NULL denotes an empty row. */
void lcd_render(const char *row0, const char *row1);
/* Slots 0..7, eight rows of five-bit pixel patterns. Restores DDRAM addressing. */
void lcd_create_char(uint8_t slot, const uint8_t pixels[8]);

#endif
