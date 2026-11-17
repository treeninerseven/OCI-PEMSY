#include "pemsy/app.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void test_compose_and_send(void)
{
    pemsy_app_t app;
    pemsy_app_command_t command;
    char lcd[2][17];
    pemsy_app_init(&app, 1u, 2u);
    pemsy_app_long_press(&app);
    assert(app.screen == PEMSY_APP_DESTINATION);
    pemsy_app_click(&app);
    pemsy_app_rotate(&app, 7); /* A -> H */
    pemsy_app_click(&app);
    pemsy_app_rotate(&app, 8); /* A -> I */
    pemsy_app_long_press(&app);
    assert(app.screen == PEMSY_APP_CONFIRM);
    assert(strcmp(app.draft, "HI") == 0);
    pemsy_app_click(&app);
    assert(app.screen == PEMSY_APP_SENDING);
    pemsy_app_send_result(&app, true); /* Can't complete an undispatched command. */
    assert(app.screen == PEMSY_APP_SENDING);
    assert(pemsy_app_take_command(&app, &command));
    assert(command.kind == PEMSY_APP_COMMAND_SEND);
    assert(command.destination == 2u && command.length == 2u);
    assert(strcmp(command.text, "HI") == 0);
    assert(!pemsy_app_take_command(&app, &command));
    pemsy_app_click(&app);
    pemsy_app_long_press(&app);
    assert(app.screen == PEMSY_APP_SENDING);
    pemsy_app_send_result(&app, false);
    assert(app.screen == PEMSY_APP_CONFIRM);
    assert(strcmp(app.draft, "HI") == 0);
    pemsy_app_click(&app);
    assert(pemsy_app_take_command(&app, &command));
    pemsy_app_send_result(&app, true);
    assert(app.screen == PEMSY_APP_INBOX);
    pemsy_app_render(&app, lcd);
    assert(strncmp(lcd[0], "TX sent", 7u) == 0);
    assert(strlen(lcd[0]) == 16u && strlen(lcd[1]) == 16u);
}

static void test_bounds_and_selection(void)
{
    pemsy_app_t app;
    pemsy_app_init(&app, 1u, 2u);
    pemsy_app_click(&app);
    pemsy_app_rotate(&app, -1);
    assert(app.destination == 255u);
    pemsy_app_rotate(&app, 1);
    assert(app.destination == 2u);
    pemsy_app_rotate(&app, INT_MIN);
    assert(app.destination >= 2u && app.destination <= 255u);
    pemsy_app_rotate(&app, INT_MAX);
    assert(app.destination >= 2u && app.destination <= 255u);
    pemsy_app_click(&app);
    pemsy_app_rotate(&app, -34); /* A -> ~, wrapping printable ASCII */
    assert(app.selected == '~');
    pemsy_app_rotate(&app, 1);
    assert(app.selected == ' ');
    pemsy_app_rotate(&app, 33);
    for (unsigned i = 0; i < 16u; ++i) pemsy_app_click(&app);
    assert(app.screen == PEMSY_APP_CONFIRM);
    assert(app.draft_length == 16u && strlen(app.draft) == 16u);
    assert(strcmp(app.draft, "AAAAAAAAAAAAAAAA") == 0);
    pemsy_app_long_press(&app); /* Cancel without transmitting. */
    assert(app.screen == PEMSY_APP_INBOX && !app.command_pending);
    pemsy_app_click(&app);
    pemsy_app_click(&app);
    pemsy_app_click(&app); /* Commit A, then immediately finish. */
    pemsy_app_long_press(&app);
    assert(strcmp(app.draft, "A") == 0); /* No accidental trailing A. */
    pemsy_app_init(&app, 255u, 255u);
    assert(app.node_id == 1u && app.destination == 255u);
    pemsy_app_init(&app, 254u, 254u);
    assert(app.destination == 1u);
    pemsy_app_click(&app);
    pemsy_app_rotate(&app, -1);
    assert(app.destination == 255u);
    pemsy_app_rotate(&app, -1);
    assert(app.destination == 253u);
}

static void test_broadcast(void)
{
    pemsy_app_t app;
    pemsy_app_command_t command;
    char lcd[2][17];
    pemsy_app_init(&app, 7u, 255u);
    pemsy_app_click(&app);
    pemsy_app_render(&app, lcd);
    assert(strncmp(lcd[0], "To ALL (255)", 12u) == 0);
    pemsy_app_click(&app);
    pemsy_app_long_press(&app);
    pemsy_app_render(&app, lcd);
    assert(strcmp(lcd[0], "Broadcast? Click") == 0);
    pemsy_app_click(&app);
    assert(pemsy_app_take_command(&app, &command));
    assert(command.kind == PEMSY_APP_COMMAND_SEND && command.destination == 255u);
    assert(command.length == 1u && command.text[0] == 'A');
}

static void test_inbox_validation_and_editing(void)
{
    pemsy_app_t app;
    char lcd[2][17];
    const uint8_t good[] = "HELLO";
    const uint8_t binary[] = {'A', 0u, 'B'};
    const uint8_t extended[] = {128u};
    pemsy_app_init(&app, 1u, 2u);
    assert(pemsy_app_receive(&app, 2u, good, 5u));
    pemsy_app_render(&app, lcd);
    assert(strncmp(lcd[0], "From 002", 8u) == 0);
    assert(strncmp(lcd[1], "HELLO", 5u) == 0);
    assert(!pemsy_app_receive(&app, 2u, good, 17u));
    assert(!pemsy_app_receive(&app, 2u, good, 0u));
    assert(!pemsy_app_receive(&app, 2u, NULL, 1u));
    assert(!pemsy_app_receive(&app, 0u, good, 5u));
    assert(!pemsy_app_receive(&app, 255u, good, 5u));
    assert(!pemsy_app_receive(&app, 2u, binary, sizeof binary));
    assert(!pemsy_app_receive(&app, 2u, extended, sizeof extended));
    assert(strcmp(app.inbox, "HELLO") == 0);
    pemsy_app_click(&app);
    pemsy_app_click(&app);
    pemsy_app_rotate(&app, 1);
    assert(pemsy_app_receive(&app, 3u, (const uint8_t *)"new", 3u));
    assert(app.screen == PEMSY_APP_COMPOSE && app.selected == 'B');
    pemsy_app_long_press(&app);
    pemsy_app_long_press(&app);
    pemsy_app_render(&app, lcd);
    assert(strncmp(lcd[1], "new", 3u) == 0);
}

static void test_temperature_contrast_and_time(void)
{
    pemsy_app_t app;
    pemsy_app_command_t command;
    char lcd[2][17];
    pemsy_app_init(&app, 1u, 2u);
    pemsy_app_rotate(&app, 1);
    pemsy_app_click(&app);
    assert(!pemsy_app_take_command(&app, &command));
    pemsy_app_tick(&app, UINT32_MAX - 1000u);
    pemsy_app_set_temperature(&app, -25);
    pemsy_app_rotate(&app, 3); /* Clear notice, remain at temperature. */
    pemsy_app_render(&app, lcd);
    assert(strncmp(lcd[0], "Temp -0.25 C", 12u) == 0);
    pemsy_app_click(&app);
    pemsy_app_click(&app);
    assert(pemsy_app_take_command(&app, &command));
    assert(command.kind == PEMSY_APP_COMMAND_MIOTY && command.temperature_centi_c == -25);
    pemsy_app_click(&app);
    assert(!pemsy_app_take_command(&app, &command)); /* No second in-flight mioty TX. */
    pemsy_app_mioty_result(&app, false);
    assert(!app.mioty_busy);
    pemsy_app_tick(&app, 1498u);
    assert(app.notice[0] != '\0');
    pemsy_app_tick(&app, 1499u);
    assert(app.notice[0] == '\0'); /* 2500 ms elapsed across uint32 wrap. */
    pemsy_app_set_temperature(&app, INT32_MIN);
    pemsy_app_click(&app);
    assert(!pemsy_app_take_command(&app, &command));
    pemsy_app_rotate(&app, 1);
    assert(app.screen == PEMSY_APP_CONTRAST && app.contrast_permille == 15u);
    pemsy_app_click(&app);
    pemsy_app_rotate(&app, INT_MAX);
    assert(pemsy_app_take_command(&app, &command));
    assert(command.kind == PEMSY_APP_COMMAND_CONTRAST && command.contrast_permille == 100u);
    pemsy_app_rotate(&app, INT_MIN);
    assert(pemsy_app_take_command(&app, &command));
    assert(command.contrast_permille == 0u);
    pemsy_app_long_press(&app);
    assert(app.screen == PEMSY_APP_INBOX && !app.contrast_editing);
}

int main(void)
{
    test_compose_and_send();
    test_bounds_and_selection();
    test_broadcast();
    test_inbox_validation_and_editing();
    test_temperature_contrast_and_time();
    puts("app tests passed");
    return 0;
}
