#include "encoder.h"
#include "pico/stdlib.h"

#define ENC_A 9
#define ENC_B 8
#define ENC_SW 10
#define DEBOUNCE_MS 30
#define LONG_PRESS_MS 1000

volatile int rotation;
volatile int short_press_count;
volatile int long_press_count;
static int previous_ab;
static int quarter_steps;
static int last_button_ms;
static int pressed_ms;
static bool button_pressed;

static void irq_handler_rotary(uint gpio, uint32_t events)
{
    // This exercise runs for less than one hour; times are in milliseconds.
    int now = to_ms_since_boot(get_absolute_time());

    if ((gpio == ENC_A || gpio == ENC_B) &&
        (events & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))) {
        // Encode A/B as 0=00, 1=01, 2=10, 3=11.
        int ab = (gpio_get(ENC_A) << 1) | gpio_get(ENC_B);
        if (ab == previous_ab) {
            return;
        }

        // Clockwise: 11 -> 01 -> 00 -> 10 -> 11.
        if ((previous_ab == 3 && ab == 1) ||
            (previous_ab == 1 && ab == 0) ||
            (previous_ab == 0 && ab == 2) ||
            (previous_ab == 2 && ab == 3)) {
            quarter_steps++;
        } else if ((previous_ab == 3 && ab == 2) ||
                   (previous_ab == 2 && ab == 0) ||
                   (previous_ab == 0 && ab == 1) ||
                   (previous_ab == 1 && ab == 3)) {
            quarter_steps--;
        } else {
            // Both signals changed: discard an incomplete step.
            quarter_steps = 0;
        }
        previous_ab = ab;

        // Count only a complete detent. Back-and-forth bounce cancels out.
        if (ab == 3) {
            if (quarter_steps == 4) {
                rotation++;
            } else if (quarter_steps == -4) {
                rotation--;
            }
            quarter_steps = 0;
        }
    } else if (gpio == ENC_SW) {
        if (now - last_button_ms < DEBOUNCE_MS) {
            return;
        }
        bool pressed = !gpio_get(ENC_SW);
        if (pressed == button_pressed) {
            return;
        }
        last_button_ms = now;
        button_pressed = pressed;
        if (pressed) {
            pressed_ms = now;
        } else if (now - pressed_ms >= LONG_PRESS_MS) {
            long_press_count++;
        } else {
            short_press_count++;
        }
    }
}

void encoder_init(void)
{
    const uint pins[] = {ENC_A, ENC_B, ENC_SW};
    for (uint i = 0; i < 3; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
    previous_ab = (gpio_get(ENC_A) << 1) | gpio_get(ENC_B);
    gpio_set_irq_enabled_with_callback(ENC_SW,GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &irq_handler_rotary);
    gpio_set_irq_enabled(ENC_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(ENC_B, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
}
