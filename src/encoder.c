#include "pemsy/encoder.h"
#include "pemsy/board.h"

#include <limits.h>
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

_Static_assert(PEMSY_ENCODER_EDGES_PER_DETENT == 2 || PEMSY_ENCODER_EDGES_PER_DETENT == 4,
               "Encoder must use two or four edges per detent");

static volatile int32_t pending_rotation;
static volatile bool raw_pressed;
static volatile uint32_t raw_changed_ms;
static uint8_t previous_ab;
static int8_t quarter_steps;
static bool debounced_pressed;
static bool long_emitted;
static uint32_t pressed_since_ms;

static uint8_t read_ab(void) {
    return (uint8_t)((gpio_get(PEMSY_ENCODER_A_PIN) ? 2u : 0u) |
                     (gpio_get(PEMSY_ENCODER_B_PIN) ? 1u : 0u));
}

void encoder_init(void) {
    const unsigned int pins[] = {
        PEMSY_ENCODER_A_PIN, PEMSY_ENCODER_B_PIN, PEMSY_ENCODER_BUTTON_PIN
    };
    for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
    pending_rotation = 0;
    quarter_steps = 0;
    previous_ab = read_ab();
    raw_pressed = !gpio_get(PEMSY_ENCODER_BUTTON_PIN);
    raw_changed_ms = to_ms_since_boot(get_absolute_time());
    debounced_pressed = false;
    long_emitted = false;
    pressed_since_ms = 0;
    for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        gpio_set_irq_enabled(pins[i], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    }
}

void encoder_handle_irq(unsigned int gpio, uint32_t events) {
    if ((events & (GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL)) == 0) {
        return;
    }
    if (gpio == PEMSY_ENCODER_BUTTON_PIN) {
        const bool pressed = !gpio_get(PEMSY_ENCODER_BUTTON_PIN);
        if (pressed != raw_pressed) {
            raw_pressed = pressed;
            raw_changed_ms = to_ms_since_boot(get_absolute_time());
        }
    } else if (gpio == PEMSY_ENCODER_A_PIN || gpio == PEMSY_ENCODER_B_PIN) {
        /* Bounces traverse the same edge in reverse and cancel. Impossible
         * two-bit transitions discard a partial step instead of inventing motion. */
        static const int8_t transitions[16] = {
             0, -1,  1,  0,  1,  0,  0, -1,
            -1,  0,  0,  1,  0,  1, -1,  0
        };
        const uint8_t current_ab = read_ab();
        if ((previous_ab ^ current_ab) == 3u) {
            quarter_steps = 0;
        } else {
            quarter_steps += transitions[(previous_ab << 2u) | current_ab];
            if (quarter_steps >= PEMSY_ENCODER_EDGES_PER_DETENT) {
                if (pending_rotation < INT32_MAX) {
                    ++pending_rotation;
                }
                quarter_steps = 0;
            } else if (quarter_steps <= -PEMSY_ENCODER_EDGES_PER_DETENT) {
                if (pending_rotation > INT32_MIN) {
                    --pending_rotation;
                }
                quarter_steps = 0;
            }
        }
        previous_ab = current_ab;
    }
}

encoder_events_t encoder_poll(uint32_t now_ms) {
    encoder_events_t events = {0};
    /* Volatile alone cannot protect the read-and-clear transfer from an ISR. */
    const uint32_t saved_irq = save_and_disable_interrupts();
    events.rotation = pending_rotation;
    pending_rotation = 0;
    const bool pressed = raw_pressed;
    const uint32_t changed_ms = raw_changed_ms;
    restore_interrupts(saved_irq);

    if (pressed != debounced_pressed &&
        (int32_t)(now_ms - changed_ms) >= (int32_t)PEMSY_ENCODER_DEBOUNCE_MS) {
        debounced_pressed = pressed;
        if (pressed) {
            pressed_since_ms = changed_ms;
            long_emitted = false;
        } else if (!long_emitted) {
            if ((uint32_t)(changed_ms - pressed_since_ms) >= PEMSY_ENCODER_LONG_PRESS_MS) {
                events.long_press = true;
            } else {
                events.click = true;
            }
        }
    }
    if (debounced_pressed && pressed && !long_emitted &&
        (uint32_t)(now_ms - pressed_since_ms) >= PEMSY_ENCODER_LONG_PRESS_MS) {
        events.long_press = true;
        long_emitted = true;
    }
    return events;
}
