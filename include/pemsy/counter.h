#ifndef PEMSY_COUNTER_H
#define PEMSY_COUNTER_H
#include <stdbool.h>
#include <stdint.h>

/* Two 4096-byte sectors, 256-byte pages. Operations must be durable on return. */
typedef struct {
    void *context;
    bool (*read)(void *, unsigned page, uint8_t data[256]);
    bool (*program)(void *, unsigned page, const uint8_t data[256]);
    bool (*erase)(void *, unsigned sector);
} counter_storage_t;
typedef struct {
    counter_storage_t storage;
    uint32_t next, reserved;
    bool ready;
} counter_t;

/* A fresh boot skips the previously reserved range, preventing replay after
 * power loss. A failed write never releases a counter to the transmitter. */
bool counter_init(counter_t *, counter_storage_t);
bool counter_next(counter_t *, uint32_t *value);
counter_storage_t counter_flash_storage(void);
#endif
