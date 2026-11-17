#include "pemsy/counter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Seeds the documented on-flash record format to reach exhaustion directly. */
#define RECORD_MAGIC UINT32_C(0x504d4331)
#define COUNTER_LIMIT UINT32_C(0x00ffffff)
typedef struct {
    uint8_t pages[32][256];
    bool fail_read;
    bool acknowledge_program;
    unsigned int programs;
} flash_t;

static bool read_page(void *context, unsigned int page, uint8_t *output) {
    flash_t *flash = context;
    assert(page < 32);
    if (flash->fail_read) return false;
    memcpy(output, flash->pages[page], 256);
    return true;
}
static bool program_page(void *context, unsigned int page, const uint8_t *input) {
    flash_t *flash = context;
    assert(page < 32);
    for (unsigned int i = 0; i < 256; ++i) flash->pages[page][i] &= input[i];
    ++flash->programs;
    return flash->acknowledge_program;
}
static bool erase_sector(void *context, unsigned int sector) {
    flash_t *flash = context;
    assert(sector < 2);
    memset(flash->pages[sector * 16], 0xff, 4096);
    return true;
}
static counter_storage_t storage(flash_t *flash) {
    return (counter_storage_t){flash, read_page, program_page, erase_sector};
}
static void reset(flash_t *flash) {
    memset(flash, 0, sizeof(*flash));
    memset(flash->pages, 0xff, sizeof(flash->pages));
    flash->acknowledge_program = true;
}
static void store32(uint8_t *target, uint32_t value) {
    for (unsigned int i = 0; i < 4; ++i) target[i] = (uint8_t)(value >> (8 * i));
}
static void seed(flash_t *flash, uint32_t high) {
    store32(flash->pages[0], RECORD_MAGIC);
    store32(flash->pages[0] + 4, high);
    store32(flash->pages[0] + 8, ~high);
    store32(flash->pages[0] + 12, RECORD_MAGIC ^ high ^ UINT32_C(0x93a67d02));
}

int main(void) {
    flash_t flash;
    counter_t counter;
    uint32_t value = 0;

    reset(&flash);
    seed(&flash, COUNTER_LIMIT - 2);
    assert(counter_init(&counter, storage(&flash)));
    assert(counter_next(&counter, &value) && value == COUNTER_LIMIT - 1);
    assert(counter_next(&counter, &value) && value == COUNTER_LIMIT);
    assert(!counter_next(&counter, &value));
    assert(value == COUNTER_LIMIT);
    assert(!counter_next(&counter, &value));
    assert(!counter_init(&counter, storage(&flash))); /* No wrap after reboot. */

    reset(&flash);
    seed(&flash, COUNTER_LIMIT - 256);
    assert(counter_init(&counter, storage(&flash)));
    for (uint32_t expected = COUNTER_LIMIT - 255; expected <= COUNTER_LIMIT; ++expected)
        assert(counter_next(&counter, &value) && value == expected);
    assert(!counter_next(&counter, &value));

    /* Unknown nonblank data with no trustworthy high-water mark must not start
     * an existing endpoint again at one. Recovery requires explicit provisioning. */
    for (unsigned int malformed = 0; malformed < 5; ++malformed) {
        reset(&flash);
        if (malformed == 0) flash.pages[0][0] = 0; /* Torn first reservation. */
        if (malformed == 1) seed(&flash, 0);       /* Zero high-water mark. */
        if (malformed == 2) seed(&flash, COUNTER_LIMIT + 1);
        if (malformed == 3) { seed(&flash, 256); flash.pages[0][8] ^= 1; }
        if (malformed == 4) { seed(&flash, 256); flash.pages[0][12] ^= 1; }
        assert(!counter_init(&counter, storage(&flash)));
        assert(!counter_next(&counter, &value));
        assert(flash.programs == 0);
    }

    /* A valid older reservation remains usable beside a torn newer page. */
    reset(&flash);
    seed(&flash, 256);
    flash.pages[1][0] = 0;
    assert(counter_init(&counter, storage(&flash)));
    assert(counter_next(&counter, &value) && value == 257);

    /* The write reached flash but its acknowledgement was lost. A reboot must
     * honor that reservation and skip it rather than replaying its counters. */
    reset(&flash);
    flash.acknowledge_program = false;
    assert(!counter_init(&counter, storage(&flash)));
    assert(!counter_next(&counter, &value));
    flash.acknowledge_program = true;
    assert(counter_init(&counter, storage(&flash)));
    assert(counter_next(&counter, &value) && value == 257);

    reset(&flash);
    flash.fail_read = true;
    assert(!counter_init(&counter, storage(&flash)));
    assert(!counter_next(&counter, &value));
    assert(flash.programs == 0);
    reset(&flash);
    assert(!counter_init(&counter, (counter_storage_t){0}));
    assert(counter_init(&counter, storage(&flash)));
    assert(!counter_next(&counter, NULL));
    assert(counter_next(&counter, &value) && value == 1);
    assert(!counter_init(&counter, (counter_storage_t){0}));
    assert(!counter_next(&counter, &value));
    assert(!counter_next(NULL, &value));

    puts("counter boundary and corruption tests passed");
    return 0;
}
