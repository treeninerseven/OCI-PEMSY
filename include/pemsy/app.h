#ifndef PEMSY_APP_H
#define PEMSY_APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PEMSY_APP_TEXT_MAX 16u
#define PEMSY_APP_LONG_PRESS_MS 3000u

typedef enum {
    PEMSY_APP_INBOX,
    PEMSY_APP_TEMPERATURE,
    PEMSY_APP_CONTRAST,
    PEMSY_APP_DESTINATION,
    PEMSY_APP_COMPOSE,
    PEMSY_APP_CONFIRM,
    PEMSY_APP_SENDING
} pemsy_app_screen_t;

typedef enum {
    PEMSY_APP_COMMAND_SEND,
    PEMSY_APP_COMMAND_MIOTY,
    PEMSY_APP_COMMAND_CONTRAST
} pemsy_app_command_kind_t;

typedef struct {
    pemsy_app_command_kind_t kind;
    uint8_t destination;
    char text[PEMSY_APP_TEXT_MAX + 1u];
    size_t length;
    int32_t temperature_centi_c;
    uint16_t contrast_permille; /* PWM duty in thousandths; UI range 0..100. */
} pemsy_app_command_t;

typedef struct {
    pemsy_app_screen_t screen;
    uint8_t node_id, destination, inbox_source;
    uint16_t contrast_permille;
    char draft[PEMSY_APP_TEXT_MAX + 1u];
    char inbox[PEMSY_APP_TEXT_MAX + 1u];
    size_t draft_length, inbox_length;
    char selected;
    bool selected_active, contrast_editing, temperature_valid;
    int32_t temperature_centi_c;
    pemsy_app_command_t command;
    bool command_pending, mioty_busy;
    uint32_t now_ms, notice_started_ms;
    char notice[17];
} pemsy_app_t;

/* Foreground-only API: debounce the encoder externally. Deliver long_press once
 * at 3000 ms and suppress click on release after a long press. No heap or I/O. */
/* Local/source addresses are 1..254; destination 255 broadcasts to all peers. */
void pemsy_app_init(pemsy_app_t *app, uint8_t node_id, uint8_t destination);
void pemsy_app_rotate(pemsy_app_t *app, int delta);
void pemsy_app_click(pemsy_app_t *app);
void pemsy_app_long_press(pemsy_app_t *app);
void pemsy_app_tick(pemsy_app_t *app, uint32_t now_ms);
void pemsy_app_set_temperature(pemsy_app_t *app, int32_t centi_c);
/* Rejects malformed, non-printable or overlength input without modifying inbox. */
bool pemsy_app_receive(pemsy_app_t *app, uint8_t source,
                       const uint8_t *text, size_t length);
bool pemsy_app_take_command(pemsy_app_t *app, pemsy_app_command_t *command);
/* Called for the outstanding SEND only; the radio driver owns its timeout. */
void pemsy_app_send_result(pemsy_app_t *app, bool success);
void pemsy_app_mioty_result(pemsy_app_t *app, bool success);
/* Two space-padded, NUL-terminated LCD rows, each exactly sixteen columns. */
void pemsy_app_render(const pemsy_app_t *app, char lines[2][17]);

#endif
