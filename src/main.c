#include "pemsy/app.h"
#include "pemsy/board.h"
#include "pemsy/counter.h"
#include "pemsy/encoder.h"
#include "pemsy/lcd.h"
#include "pemsy/mioty.h"
#include "pemsy/radio.h"
#include "pemsy/temperature.h"
#include "hardware/watchdog.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct { uint32_t counter; char text[32]; } mioty_request_t;
static queue_t requests, results;
static uint32_t worker_stack[4096] __attribute__((aligned(8)));

static void gpio_irq(uint gpio, uint32_t events) {
    encoder_handle_irq(gpio, events);
    radio_handle_irq(gpio, events);
}

static void mioty_worker(void) {
    bool ready = flash_safe_execute_core_init();
    queue_add_blocking(&results, &ready);
    for (;;) {
        mioty_request_t request;
        queue_remove_blocking(&requests, &request);
        bool success = ready && mioty_setup();
        if (success) {
            mioty_set_counter(request.counter);
            success = mioty_ep_to_base(request.text);
        }
        /* Returning the result transfers exclusive ownership of SPI to core0. */
        queue_add_blocking(&results, &success);
    }
}

int main(void) {
    stdio_init_all();
    gpio_init(PEMSY_ONBOARD_LED_PIN);
    gpio_set_dir(PEMSY_ONBOARD_LED_PIN, GPIO_OUT);
    gpio_put(PEMSY_ONBOARD_LED_PIN, false);
    lcd_init();
    lcd_render("PEMSY pager", "Starting...");
    gpio_set_irq_callback(gpio_irq);
    irq_set_enabled(IO_IRQ_BANK0, true);
    encoder_init();
    temperature_init();
    bool radio_online = radio_init();

    queue_init(&requests, sizeof(mioty_request_t), 1);
    queue_init(&results, sizeof(bool), 1);
    multicore_launch_core1_with_stack(mioty_worker, worker_stack, sizeof(worker_stack));
    bool worker_ready;
    queue_remove_blocking(&results, &worker_ready);
    counter_t counter;
    bool counter_ready = mioty_is_enabled() && worker_ready &&
                         counter_init(&counter, counter_flash_storage());

    pemsy_app_t app;
    pemsy_app_init(&app, PEMSY_NODE_ADDRESS, PEMSY_DEFAULT_RECIPIENT);
    printf("PEMSY node=%u peer=%u frequency=%u radio=%s mioty=%s\n",
           PEMSY_NODE_ADDRESS, PEMSY_DEFAULT_RECIPIENT,
           PEMSY_RADIO_FREQUENCY_HZ, radio_online ? "ready" : "offline",
           mioty_is_enabled() ? (counter_ready ? "ready" : "counter error") : "disabled");
    watchdog_enable(8000, true);
    bool mioty_busy = false, mioty_sent_once = false;
    uint32_t mioty_started = 0, last_mioty = 0;
    uint32_t last_temperature = (uint32_t)-1000, last_radio_retry = 0;
    uint32_t last_render = (uint32_t)-100;
    uint32_t led_started = 0;
    bool led_pulse = false;
    char previous[2][17] = {{0}};

    for (;;) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        watchdog_update();
        pemsy_app_tick(&app, now);
        encoder_events_t event = encoder_poll(now);
        if (event.rotation) pemsy_app_rotate(&app, event.rotation);
        if (event.long_press) pemsy_app_long_press(&app);
        else if (event.click) pemsy_app_click(&app);

        if ((uint32_t)(now - last_temperature) >= 1000) {
            float value = temperature_read_c();
            if (isfinite(value)) pemsy_app_set_temperature(&app, (int32_t)(value * 100.0f));
            last_temperature = now;
        }

        if (mioty_busy) {
            bool success;
            if (queue_try_remove(&results, &success)) {
                mioty_busy = false;
                radio_online = radio_init();
                last_radio_retry = now;
                pemsy_app_mioty_result(&app, success);
                printf("mioty TX %s; peer RX %s\n", success ? "complete" : "failed",
                       radio_online ? "restored" : "offline");
                led_pulse = success;
                led_started = now;
            } else if ((uint32_t)(now - mioty_started) >= 60000) {
                /* A stalled worker must not keep exclusive ownership forever.
                 * Reboot both cores; the persistent counter range is skipped. */
                lcd_render("mioty timeout", "Restarting...");
                printf("mioty worker timeout; watchdog reset\n");
                watchdog_reboot(0, 0, 10);
                for (;;) tight_loop_contents();
            }
        } else {
            radio_poll();
            bool success;
            if (radio_take_tx_result(&success)) {
                pemsy_app_send_result(&app, success);
                printf("peer TX %s (no delivery ACK)\n", success ? "complete" : "failed");
                led_pulse = success;
                led_started = now;
            }
            radio_packet_t packet;
            while (radio_receive(&packet)) {
                pemsy_app_receive(&app, packet.sender, (const uint8_t *)packet.text,
                                  strlen(packet.text));
                printf("RX from %u%s RSSI=%d: %s\n", packet.sender,
                       packet.is_broadcast ? " broadcast" : "", packet.rssi_dbm, packet.text);
                led_pulse = true;
                led_started = now;
            }
            radio_online = radio_is_online();
            if (!radio_online && (uint32_t)(now - last_radio_retry) >= 5000) {
                radio_online = radio_init();
                last_radio_retry = now;
            }
        }

        pemsy_app_command_t command;
        if (pemsy_app_take_command(&app, &command)) {
            if (command.kind == PEMSY_APP_COMMAND_CONTRAST) {
                lcd_set_contrast(command.contrast_permille);
            } else if (command.kind == PEMSY_APP_COMMAND_SEND) {
                if (mioty_busy || !radio_send(command.destination, command.text))
                    pemsy_app_send_result(&app, false);
            } else if (command.kind == PEMSY_APP_COMMAND_MIOTY) {
                uint32_t sequence;
                bool allowed = !mioty_busy && !radio_busy() && counter_ready &&
                    (!mioty_sent_once || (uint32_t)(now - last_mioty) >= 60000);
                if (allowed && counter_next(&counter, &sequence) && radio_suspend()) {
                    mioty_request_t request = {.counter = sequence};
                    int32_t temperature = command.temperature_centi_c;
                    int32_t magnitude = temperature < 0 ? -temperature : temperature;
                    snprintf(request.text, sizeof(request.text), "%s%ld.%02ld C",
                             temperature < 0 ? "-" : "", (long)(magnitude / 100),
                             (long)(magnitude % 100));
                    if (queue_try_add(&requests, &request)) {
                        mioty_busy = mioty_sent_once = true;
                        mioty_started = last_mioty = now;
                        printf("mioty TX started counter=%lu; peer RX paused\n", (unsigned long)sequence);
                    } else {
                        radio_online = radio_init();
                        pemsy_app_mioty_result(&app, false);
                    }
                } else {
                    pemsy_app_mioty_result(&app, false);
                    printf("mioty unavailable: enable/configure it, check radio/counter, or wait 60s\n");
                }
            }
        }

        if (led_pulse && (uint32_t)(now - led_started) >= 150) led_pulse = false;
        gpio_put(PEMSY_ONBOARD_LED_PIN, led_pulse || mioty_busy);
        if ((uint32_t)(now - last_render) >= 50) {
            char lines[2][17];
            pemsy_app_render(&app, lines);
            if (!mioty_busy && !radio_online && app.screen == PEMSY_APP_INBOX)
                memcpy(lines[0], "Radio offline   ", 17);
            if (memcmp(previous, lines, sizeof(lines)) != 0) {
                lcd_render(lines[0], lines[1]);
                memcpy(previous, lines, sizeof(lines));
            }
            last_render = now;
        }
        sleep_ms(5);
    }
}
