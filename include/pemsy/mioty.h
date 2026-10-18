#ifndef PEMSY_MIOTY_H
#define PEMSY_MIOTY_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool mioty_is_enabled(void);
/* Setup and send exclusively own the radio/SPI; run on the worker core. */
bool mioty_setup(void);
bool mioty_ep_to_base(const char *message);
/* Application must reserve a persistent counter before handing it to the worker. */
void mioty_set_counter(uint32_t counter);
uint32_t mioty_next_counter(void);
#ifdef __cplusplus
}
#endif
#endif
