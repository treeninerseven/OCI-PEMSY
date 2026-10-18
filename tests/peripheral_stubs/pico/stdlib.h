#ifndef TEST_STDLIB_H
#define TEST_STDLIB_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
typedef uint64_t absolute_time_t;
absolute_time_t get_absolute_time(void);
uint32_t to_ms_since_boot(absolute_time_t time);
void sleep_ms(uint32_t duration);
void sleep_us(uint64_t duration);
#endif
