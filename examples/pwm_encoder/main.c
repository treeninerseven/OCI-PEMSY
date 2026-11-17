#include <stdio.h>
#include "pico/stdlib.h"
#include "lcd.h"
#include "encoder.h"
#include "led.h"

#define MESSAGE_LENGTH 16

int main(void)
{
    stdio_init_all();
    led_init();
    lcd_init();
    encoder_init();

    char message[MESSAGE_LENGTH + 1] = {0};
    int length = 0;
    char letter = 'A';
    int previous_rotation = 0;
    int previous_short_press = 0;
    int previous_long_press = 0;
    const uint8_t led_pins[] = {LED1, LED2, LED3};
    int current_led = -1;
    lcd_writeCommand(0x01);
    lcd_locate(0, 0);
    lcd_writedate(letter);

    while (true) {
        // The ISR owns the counters; the main loop never clears them.
        int current_rotation = rotation;
        int current_short_press = short_press_count;
        int current_long_press = long_press_count;
        int steps = current_rotation - previous_rotation;
        previous_rotation = current_rotation;

        while (steps > 0) {
            printf("Right (clockwise)\n");
            current_led++;
            if (current_led == 3) {
                current_led = 0;
            }
            led_all_off();
            led_green(led_pins[current_led]);
            if (letter == 'Z') {
                letter = 'A';
            } else {
                letter++;
            }
            steps--;
        }
        while (steps < 0) {
            printf("Left (counterclockwise)\n");
            current_led--;
            if (current_led < 0) {
                current_led = 2;
            }
            led_all_off();
            led_red(led_pins[current_led]);
            if (letter == 'A') {
                letter = 'Z';
            } else {
                letter--;
            }
            steps++;
        }
        lcd_locate(0, length);
        lcd_writedate(letter);

        if (current_long_press != previous_long_press) {
            previous_long_press = current_long_press;
            led_toggle(LED2);
            message[length] = letter;
            message[length + 1] = '\0';
            printf("Saved: %s\n", message);
            lcd_locate(1, 0);
            lcd_print("Saved");
        }
        while (previous_short_press < current_short_press) {
            previous_short_press++;
            led_toggle(LED1);
            if (length < MESSAGE_LENGTH - 1) {
                message[length] = letter;
                length++;
                letter = 'A';
            }
        }
        sleep_ms(10);
    }
}
