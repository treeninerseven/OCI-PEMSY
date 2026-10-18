/* Portable foreground/UI simulator. It does not emulate the RF hardware. */
#include "pemsy/app.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void display(const pemsy_app_t *app)
{
    char lines[2][17];
    pemsy_app_render(app, lines);
    printf("+----------------+\n|%s|\n|%s|\n+----------------+\n", lines[0], lines[1]);
}

static bool integer(const char *text, long minimum, long maximum, long *value,
                    const char **end)
{
    char *tail;
    errno = 0;
    long parsed = strtol(text, &tail, 10);
    if (text == tail || errno != 0 || parsed < minimum || parsed > maximum) return false;
    *value = parsed;
    if (end != NULL) *end = tail;
    else {
        while (isspace((unsigned char)*tail)) ++tail;
        if (*tail != '\0') return false;
    }
    return true;
}

int main(void)
{
    pemsy_app_t app;
    pemsy_app_command_t command;
    bool next_send_success = true;
    uint32_t now_ms = 0u;
    char input[256];
    pemsy_app_init(&app, 1u, 2u);
    pemsy_app_set_temperature(&app, 2350);
    puts("PEMSY UI simulator (simulated TX completion; no radio hardware)");
    puts("Destination 255 is broadcast; select it by turning left from peer 2.");
    puts("+/-: rotate; c: click; h: hold 3 seconds; n NUMBER: rotate steps");
    puts("t CENTI_C: temperature; r NODE TEXT: receive; f: fail next TX; q: quit");
    display(&app);
    while (fgets(input, sizeof input, stdin) != NULL) {
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            puts("Input line too long");
            continue;
        }
        input[strcspn(input, "\r\n")] = '\0';
        now_ms += 100u;
        pemsy_app_tick(&app, now_ms);
        long value;
        if (strcmp(input, "q") == 0) break;
        if (strcmp(input, "+") == 0) pemsy_app_rotate(&app, 1);
        else if (strcmp(input, "-") == 0) pemsy_app_rotate(&app, -1);
        else if (strcmp(input, "c") == 0) pemsy_app_click(&app);
        else if (strcmp(input, "h") == 0) {
            now_ms += PEMSY_APP_LONG_PRESS_MS;
            pemsy_app_tick(&app, now_ms);
            pemsy_app_long_press(&app);
        } else if (strncmp(input, "n ", 2u) == 0 &&
                   integer(input + 2, INT_MIN, INT_MAX, &value, NULL)) {
            pemsy_app_rotate(&app, (int)value);
        } else if (strncmp(input, "t ", 2u) == 0 &&
                   integer(input + 2, INT32_MIN, INT32_MAX, &value, NULL)) {
            pemsy_app_set_temperature(&app, (int32_t)value);
        } else if (strncmp(input, "r ", 2u) == 0) {
            const char *message;
            if (integer(input + 2, 1, 254, &value, &message) && *message == ' ') {
                ++message;
                if (!pemsy_app_receive(&app, (uint8_t)value, (const uint8_t *)message,
                                       strlen(message))) puts("Invalid message (1..16 printable ASCII)");
            } else puts("Use r NODE TEXT, where NODE is 1..254");
        } else if (strcmp(input, "f") == 0) {
            next_send_success = false;
            puts("The next simulated TX will fail.");
        } else if (input[0] != '\0') puts("Unknown command");
        while (pemsy_app_take_command(&app, &command)) {
            switch (command.kind) {
            case PEMSY_APP_COMMAND_SEND:
                printf("SIMULATED RF TX to %u: %s\n", command.destination, command.text);
                pemsy_app_send_result(&app, next_send_success);
                next_send_success = true;
                break;
            case PEMSY_APP_COMMAND_MIOTY:
                printf("SIMULATED mioty temperature: %ld centi-C\n", (long)command.temperature_centi_c);
                pemsy_app_mioty_result(&app, next_send_success);
                next_send_success = true;
                break;
            case PEMSY_APP_COMMAND_CONTRAST:
                printf("SIMULATED contrast PWM: %u/1000\n", command.contrast_permille);
                break;
            }
        }
        display(&app);
    }
    return ferror(stdin) ? 1 : 0;
}
