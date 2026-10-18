#include "pemsy/app.h"

#include <stdio.h>
#include <string.h>

static void notice(pemsy_app_t *app, const char *text)
{
    (void)snprintf(app->notice, sizeof app->notice, "%.16s", text);
    app->notice_started_ms = app->now_ms;
}

static int wrap(int value, int delta, int count)
{
    /* Reduce first so even INT_MIN/INT_MAX rotary deltas cannot overflow. */
    int result = (value + delta % count) % count;
    return result < 0 ? result + count : result;
}

static bool valid_address(uint8_t address)
{
    return address > 0u && address < 255u;
}

void pemsy_app_init(pemsy_app_t *app, uint8_t node_id, uint8_t destination)
{
    if (app == NULL) return;
    memset(app, 0, sizeof *app);
    app->node_id = valid_address(node_id) ? node_id : 1u;
    app->destination = destination > 0u && destination != app->node_id
                           ? destination : (app->node_id == 1u ? 2u : 1u);
    app->contrast_permille = 15u;
    app->selected = 'A';
}

void pemsy_app_rotate(pemsy_app_t *app, int delta)
{
    if (app == NULL || delta == 0) return;
    app->notice[0] = '\0';
    switch (app->screen) {
    case PEMSY_APP_INBOX:
    case PEMSY_APP_TEMPERATURE:
        app->screen = (pemsy_app_screen_t)wrap((int)app->screen, delta, 3);
        break;
    case PEMSY_APP_CONTRAST:
        if (!app->contrast_editing) {
            app->screen = (pemsy_app_screen_t)wrap((int)app->screen, delta, 3);
        } else if (!app->command_pending) {
            int next = (int)app->contrast_permille;
            if (delta > 100 - next) next = 100;
            else if (delta < -next) next = 0;
            else next += delta;
            app->contrast_permille = (uint16_t)next;
            app->command = (pemsy_app_command_t){.kind = PEMSY_APP_COMMAND_CONTRAST,
                                               .contrast_permille = app->contrast_permille};
            app->command_pending = true;
        }
        break;
    case PEMSY_APP_DESTINATION: {
        /* Rank 254 destinations: peer IDs and broadcast 255, excluding self. */
        int rank = (int)app->destination - 1;
        if (app->destination > app->node_id) --rank;
        rank = wrap(rank, delta, 254);
        int destination = rank + 1;
        if (destination >= app->node_id) ++destination;
        app->destination = (uint8_t)destination;
        break;
    }
    case PEMSY_APP_COMPOSE:
        app->selected = (char)(' ' + wrap(app->selected - ' ', delta, 95));
        app->selected_active = true;
        break;
    default:
        break;
    }
}

static void commit_character(pemsy_app_t *app)
{
    if (app->draft_length < PEMSY_APP_TEXT_MAX) {
        app->draft[app->draft_length++] = app->selected;
        app->draft[app->draft_length] = '\0';
    }
    app->selected = 'A';
    app->selected_active = false;
    if (app->draft_length == PEMSY_APP_TEXT_MAX) app->screen = PEMSY_APP_CONFIRM;
}

void pemsy_app_click(pemsy_app_t *app)
{
    if (app == NULL) return;
    app->notice[0] = '\0';
    switch (app->screen) {
    case PEMSY_APP_INBOX:
        app->screen = PEMSY_APP_DESTINATION;
        break;
    case PEMSY_APP_DESTINATION:
        app->draft_length = 0u;
        app->draft[0] = '\0';
        app->selected = 'A';
        app->selected_active = true;
        app->screen = PEMSY_APP_COMPOSE;
        break;
    case PEMSY_APP_COMPOSE:
        commit_character(app);
        break;
    case PEMSY_APP_CONFIRM:
        if (app->command_pending) { notice(app, "Device busy"); break; }
        if (app->draft_length == 0u) { notice(app, "Message empty"); break; }
        app->command = (pemsy_app_command_t){.kind = PEMSY_APP_COMMAND_SEND,
                                           .destination = app->destination,
                                           .length = app->draft_length};
        memcpy(app->command.text, app->draft, app->draft_length + 1u);
        app->command_pending = true;
        app->screen = PEMSY_APP_SENDING;
        break;
    case PEMSY_APP_TEMPERATURE:
        if (!app->temperature_valid) { notice(app, "Temp unavailable"); break; }
        if (app->mioty_busy) { notice(app, "mioty busy"); break; }
        if (app->command_pending) { notice(app, "Device busy"); break; }
        app->command = (pemsy_app_command_t){.kind = PEMSY_APP_COMMAND_MIOTY,
                              .temperature_centi_c = app->temperature_centi_c};
        app->command_pending = true;
        app->mioty_busy = true;
        notice(app, "mioty requested");
        break;
    case PEMSY_APP_CONTRAST:
        app->contrast_editing = !app->contrast_editing;
        break;
    case PEMSY_APP_SENDING:
        break;
    }
}

void pemsy_app_long_press(pemsy_app_t *app)
{
    if (app == NULL) return;
    app->notice[0] = '\0';
    switch (app->screen) {
    case PEMSY_APP_COMPOSE:
        /* The first preview is active. After a click, the next preview is only
         * committed on a further click or rotation, avoiding a trailing A. */
        if (app->selected_active) commit_character(app);
        app->screen = PEMSY_APP_CONFIRM;
        break;
    case PEMSY_APP_DESTINATION:
    case PEMSY_APP_CONFIRM:
        app->screen = PEMSY_APP_INBOX;
        break;
    case PEMSY_APP_SENDING:
        break;
    case PEMSY_APP_CONTRAST:
        if (app->contrast_editing) {
            app->contrast_editing = false;
            app->screen = PEMSY_APP_INBOX;
            break;
        }
        /* fall through */
    default:
        app->screen = PEMSY_APP_DESTINATION;
        break;
    }
}

void pemsy_app_tick(pemsy_app_t *app, uint32_t now_ms)
{
    if (app == NULL) return;
    app->now_ms = now_ms;
    if (app->notice[0] != '\0' && (uint32_t)(now_ms - app->notice_started_ms) >= 2500u)
        app->notice[0] = '\0';
}

void pemsy_app_set_temperature(pemsy_app_t *app, int32_t centi_c)
{
    if (app == NULL) return;
    app->temperature_centi_c = centi_c;
    /* Physical sensor plausibility and safe fixed-width formatting. */
    app->temperature_valid = centi_c >= -5000 && centi_c <= 15000;
}

bool pemsy_app_receive(pemsy_app_t *app, uint8_t source,
                       const uint8_t *text, size_t length)
{
    if (app == NULL || text == NULL || !valid_address(source) ||
        length == 0u || length > PEMSY_APP_TEXT_MAX) return false;
    for (size_t i = 0; i < length; ++i)
        if (text[i] < 32u || text[i] > 126u) return false;
    memcpy(app->inbox, text, length);
    app->inbox[length] = '\0';
    app->inbox_length = length;
    app->inbox_source = source;
    return true;
}

bool pemsy_app_take_command(pemsy_app_t *app, pemsy_app_command_t *command)
{
    if (app == NULL || command == NULL || !app->command_pending) return false;
    *command = app->command;
    app->command_pending = false;
    return true;
}

void pemsy_app_send_result(pemsy_app_t *app, bool success)
{
    if (app == NULL || app->screen != PEMSY_APP_SENDING) return;
    /* A synchronous driver can complete immediately after take_command. */
    if (app->command_pending) return;
    app->screen = success ? PEMSY_APP_INBOX : PEMSY_APP_CONFIRM;
    notice(app, success ? "TX sent" : "TX failed: retry");
}

void pemsy_app_mioty_result(pemsy_app_t *app, bool success)
{
    if (app == NULL || !app->mioty_busy) return;
    if (app->command_pending && app->command.kind == PEMSY_APP_COMMAND_MIOTY) return;
    app->mioty_busy = false;
    notice(app, success ? "mioty TX sent" : "mioty TX failed");
}

static void row(char target[17], const char *text)
{
    memset(target, ' ', 16u);
    size_t i = 0u;
    while (i < 16u && text[i] != '\0') { target[i] = text[i]; ++i; }
    target[16] = '\0';
}

void pemsy_app_render(const pemsy_app_t *app, char lines[2][17])
{
    if (app == NULL || lines == NULL) return;
    char heading[48] = "", detail[48] = "";
    switch (app->screen) {
    case PEMSY_APP_INBOX:
        if (app->inbox_length != 0u) {
            (void)snprintf(heading, sizeof heading, "From %03u", app->inbox_source);
            (void)snprintf(detail, sizeof detail, "%s", app->inbox);
        } else {
            (void)snprintf(heading, sizeof heading, "Pager %03u ready", app->node_id);
            (void)snprintf(detail, sizeof detail, "Click: new text");
        }
        break;
    case PEMSY_APP_TEMPERATURE:
        if (app->temperature_valid) {
            int32_t value = app->temperature_centi_c;
            int32_t magnitude = value < 0 ? -value : value;
            (void)snprintf(heading, sizeof heading, "Temp %s%ld.%02ld C",
                          value < 0 ? "-" : "", (long)(magnitude / 100),
                          (long)(magnitude % 100));
        } else (void)snprintf(heading, sizeof heading, "Temp unavailable");
        (void)snprintf(detail, sizeof detail, "Click: mioty TX");
        break;
    case PEMSY_APP_CONTRAST:
        (void)snprintf(heading, sizeof heading, "Contrast %u.%u%%",
                      app->contrast_permille / 10u, app->contrast_permille % 10u);
        (void)snprintf(detail, sizeof detail, "%s",
                      app->contrast_editing ? "Turn; click done" : "Click: adjust");
        break;
    case PEMSY_APP_DESTINATION:
        if (app->destination == 255u)
            (void)snprintf(heading, sizeof heading, "To ALL (255)");
        else (void)snprintf(heading, sizeof heading, "To node %03u", app->destination);
        (void)snprintf(detail, sizeof detail, "Turn; click edit");
        break;
    case PEMSY_APP_COMPOSE:
        (void)snprintf(heading, sizeof heading, "Text %02u/16 3s:OK",
                      (unsigned)app->draft_length);
        memcpy(detail, app->draft, app->draft_length);
        if (app->draft_length < PEMSY_APP_TEXT_MAX)
            detail[app->draft_length] = app->selected;
        break;
    case PEMSY_APP_CONFIRM:
        if (app->destination == 255u)
            (void)snprintf(heading, sizeof heading, "Broadcast? Click");
        else (void)snprintf(heading, sizeof heading, "Send %03u? Click", app->destination);
        (void)snprintf(detail, sizeof detail, "%s", app->draft);
        break;
    case PEMSY_APP_SENDING:
        (void)snprintf(heading, sizeof heading, "Sending to %03u", app->destination);
        (void)snprintf(detail, sizeof detail, "%s", app->draft);
        break;
    }
    row(lines[0], app->notice[0] != '\0' ? app->notice : heading);
    row(lines[1], detail);
}
