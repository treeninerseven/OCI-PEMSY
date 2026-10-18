#ifndef TEST_CLOCKS_H
#define TEST_CLOCKS_H
#include <stdint.h>
enum clock_index { clk_sys };
uint32_t clock_get_hz(enum clock_index clock);
#endif
