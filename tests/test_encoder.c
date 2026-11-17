#include "pemsy/encoder.h"
#include "pemsy/board.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <assert.h>
#include <stdio.h>

static bool levels[30];
static uint32_t fake_ms;
static bool interrupts_disabled;
static unsigned int enabled_count;

void gpio_init(unsigned int pin) { (void)pin; }
void gpio_set_dir(unsigned int pin, bool output) { (void)pin; assert(!output); }
void gpio_pull_up(unsigned int pin) { (void)pin; }
bool gpio_get(unsigned int pin) { return levels[pin]; }
void gpio_set_irq_enabled(unsigned int pin, uint32_t events, bool enabled) {
    (void)pin;
    assert(events == (GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL));
    assert(enabled);
    ++enabled_count;
}
absolute_time_t get_absolute_time(void) { return fake_ms; }
uint32_t to_ms_since_boot(absolute_time_t time) { return (uint32_t)time; }
uint32_t save_and_disable_interrupts(void) {
    assert(!interrupts_disabled);
    interrupts_disabled = true;
    return 42;
}
void restore_interrupts(uint32_t flags) {
    assert(interrupts_disabled && flags == 42);
    interrupts_disabled = false;
}

static void reset(uint32_t now) {
    for (unsigned int i = 0; i < 30; ++i) levels[i] = true;
    fake_ms = now;
    enabled_count = 0;
    encoder_init();
    assert(enabled_count == 3);
}

static void edge(unsigned int pin, bool value, uint32_t now) {
    fake_ms = now;
    levels[pin] = value;
    encoder_handle_irq(pin, value ? GPIO_IRQ_EDGE_RISE : GPIO_IRQ_EDGE_FALL);
}

static encoder_events_t poll(uint32_t now) {
    fake_ms = now;
    return encoder_poll(now);
}

static void assert_none(encoder_events_t event) {
    assert(event.rotation == 0 && !event.click && !event.long_press);
}

static void clockwise(void) {
    edge(PEMSY_ENCODER_A_PIN, false, fake_ms + 1);
    edge(PEMSY_ENCODER_B_PIN, false, fake_ms + 1);
    edge(PEMSY_ENCODER_A_PIN, true, fake_ms + 1);
    edge(PEMSY_ENCODER_B_PIN, true, fake_ms + 1);
}

int main(void) {
    reset(0);
    clockwise(); clockwise(); clockwise();
    assert(poll(20).rotation == 12 / PEMSY_ENCODER_EDGES_PER_DETENT);
    assert_none(poll(21)); /* Pending motion is consumed exactly once. */
    edge(PEMSY_ENCODER_B_PIN, false, 22);
    edge(PEMSY_ENCODER_A_PIN, false, 23);
    edge(PEMSY_ENCODER_B_PIN, true, 24);
    edge(PEMSY_ENCODER_A_PIN, true, 25);
    assert(poll(26).rotation == -4 / PEMSY_ENCODER_EDGES_PER_DETENT);

    reset(0);
    edge(PEMSY_ENCODER_A_PIN, false, 1);
    edge(PEMSY_ENCODER_A_PIN, true, 2);
    edge(PEMSY_ENCODER_A_PIN, false, 3);
    edge(PEMSY_ENCODER_A_PIN, true, 4);
    assert_none(poll(5)); /* Contact bounce cancels without a full step. */
    levels[PEMSY_ENCODER_A_PIN] = false;
    levels[PEMSY_ENCODER_B_PIN] = false;
    encoder_handle_irq(PEMSY_ENCODER_A_PIN, GPIO_IRQ_EDGE_FALL);
    assert_none(poll(6)); /* A missed/illegal two-bit jump invents no step. */

    reset(0);
    edge(PEMSY_ENCODER_BUTTON_PIN, false, 100);
    assert_none(poll(110));
    edge(PEMSY_ENCODER_BUTTON_PIN, true, 112);
    edge(PEMSY_ENCODER_BUTTON_PIN, false, 120);
    assert_none(poll(149));
    assert_none(poll(150));
    edge(PEMSY_ENCODER_BUTTON_PIN, true, 180);
    assert_none(poll(200));
    edge(PEMSY_ENCODER_BUTTON_PIN, false, 201);
    edge(PEMSY_ENCODER_BUTTON_PIN, true, 205);
    assert_none(poll(234));
    encoder_events_t event = poll(235);
    assert(event.click && !event.long_press);
    assert_none(poll(236));

    reset(0);
    edge(PEMSY_ENCODER_BUTTON_PIN, false, 100);
    assert_none(poll(130));
    assert_none(poll(3099));
    event = poll(3100);
    assert(event.long_press && !event.click);
    assert_none(poll(4000));
    edge(PEMSY_ENCODER_BUTTON_PIN, true, 4100);
    assert_none(poll(4130)); /* No short press after a long hold. */

    reset(0);
    edge(PEMSY_ENCODER_BUTTON_PIN, false, 10);
    assert_none(poll(40));
    edge(PEMSY_ENCODER_BUTTON_PIN, true, 4000);
    event = poll(4030); /* Release itself reports long when the loop was delayed. */
    assert(event.long_press && !event.click);
    assert_none(poll(4040));

    reset(UINT32_MAX - 2000);
    const uint32_t pressed = UINT32_MAX - 1000;
    edge(PEMSY_ENCODER_BUTTON_PIN, false, pressed);
    assert_none(poll(pressed + 30u));
    assert_none(poll(pressed + 2999u));
    event = poll(pressed + 3000u);
    assert(event.long_press && !event.click); /* Millisecond clock wraps safely. */

    reset(0);
    edge(PEMSY_ENCODER_BUTTON_PIN, false, 101);
    assert_none(poll(100)); /* IRQ newer than caller's timestamp is not elapsed time. */
    edge(PEMSY_ENCODER_BUTTON_PIN, true, 110);
    assert_none(poll(150)); /* That short glitch was never a debounced press. */

    puts("encoder tests passed");
    return 0;
}
