#ifndef PEMSY_ENCODER_H
#define PEMSY_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#define PEMSY_ENCODER_DEBOUNCE_MS 30u
#define PEMSY_ENCODER_LONG_PRESS_MS 3000u

typedef struct {
    int32_t rotation; /* Clockwise positive, counterclockwise negative. */
    bool click;      /* Debounced release, suppressed after a long press. */
    bool long_press; /* Emitted once per hold, including a delayed release poll. */
} encoder_events_t;

/* Register the application's shared GPIO callback before enabling these IRQs.
 * All calls and the GPIO callback must run on the same RP2040 core. */
void encoder_init(void);
void encoder_handle_irq(unsigned int gpio, uint32_t events);
/* Call regularly (e.g. every 5-10ms). now_ms must be the Pico boot time in ms. */
encoder_events_t encoder_poll(uint32_t now_ms);

#endif
