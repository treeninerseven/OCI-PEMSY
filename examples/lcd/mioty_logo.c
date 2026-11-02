#include "lcd.h"
#include "mioty_logo.h"

// Eight tiles arranged as:
//     0 1 2 3
//     4 5 6 7
// Each row uses 5 bits: bit 4 is the leftmost pixel.
const uint8_t mioty_logo_pixels[8][8] = {
    { /* Tile 0 */
        0x00, /* ..... */
        0x00, /* ..... */
        0x00, /* ..... */
        0x00, /* ..... */
        0x01, /* ....# */
        0x03, /* ...## */
        0x07, /* ..### */
        0x0F, /* .#### */
    },
    { /* Tile 1 */
        0x01, /* ....# */
        0x03, /* ...## */
        0x07, /* ..### */
        0x0F, /* .#### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
    },
    { /* Tile 2 */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
    },
    { /* Tile 3 */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
    },
    { /* Tile 4 */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
    },
    { /* Tile 5 */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
    },
    { /* Tile 6 */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1F, /* ##### */
        0x1E, /* ####. */
        0x1C, /* ###.. */
        0x18, /* ##... */
        0x10, /* #.... */
    },
    { /* Tile 7 */
        0x1E, /* ####. */
        0x1C, /* ###.. */
        0x18, /* ##... */
        0x10, /* #.... */
        0x00, /* ..... */
        0x00, /* ..... */
        0x00, /* ..... */
        0x00, /* ..... */
    },
};

void lcd_show_mioty_logo(void)
{
    lcd_writeCommand(0x0C); // Display on; cursor and blinking off.
    lcd_writeCommand(0x01); // Clear old text.

    for (uint8_t slot = 0; slot < 8; slot++) {
        lcd_createChar(slot, mioty_logo_pixels[slot]);
    }

    // Center four columns on the 16-column display.
    for (uint8_t row = 0; row < 2; row++) {
        lcd_locate(row, 6);
        for (uint8_t col = 0; col < 4; col++) {
            // Write the slot number directly. Slot 0 is valid!
            // A string would stop at slot 0 because it is also NUL.
            lcd_writedate(row * 4 + col);
        }
    }
}
