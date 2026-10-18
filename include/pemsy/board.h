#ifndef PEMSY_BOARD_H
#define PEMSY_BOARD_H

/* GPIO numbers, not physical header pin numbers. Course PCB tables 6.5/8.5/10.3. */
#define PEMSY_LCD_CONTRAST_PIN 0u
#define PEMSY_LCD_ENABLE_PIN 1u
#define PEMSY_LCD_RW_PIN 2u
#define PEMSY_LCD_RS_PIN 3u
#define PEMSY_LCD_D4_PIN 4u
#define PEMSY_LCD_D5_PIN 5u
#define PEMSY_LCD_D6_PIN 6u
#define PEMSY_LCD_D7_PIN 7u
#define PEMSY_ENCODER_A_PIN 9u
#define PEMSY_ENCODER_B_PIN 8u
#define PEMSY_ENCODER_BUTTON_PIN 10u
#define PEMSY_LED1_PIN 11u
#define PEMSY_LED2_PIN 14u
#define PEMSY_LED3_PIN 15u
#define PEMSY_RADIO_MISO_PIN 16u
#define PEMSY_RADIO_CS_PIN 17u
#define PEMSY_RADIO_SCK_PIN 18u
#define PEMSY_RADIO_MOSI_PIN 19u
#define PEMSY_RADIO_DIO0_PIN 20u
#define PEMSY_ONBOARD_LED_PIN 25u

/* Four Gray-code edges per full cycle. Override for a two-edge detent encoder. */
#ifndef PEMSY_ENCODER_EDGES_PER_DETENT
#define PEMSY_ENCODER_EDGES_PER_DETENT 4
#endif

#endif
