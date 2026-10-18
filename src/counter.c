#include "pemsy/counter.h"
#include <string.h>

#define MAGIC UINT32_C(0x504d4331)
#define LIMIT UINT32_C(0x00ffffff)
#define RESERVATION 256u

static uint32_t load32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void store32(uint8_t *p, uint32_t n) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(n >> (8u * i));
}
static bool blank(const uint8_t *p) {
    for (unsigned i = 0; i < 256; ++i) if (p[i] != 0xff) return false;
    return true;
}
static uint32_t valid_high(const uint8_t *p) {
    uint32_t n = load32(p + 4);
    return load32(p) == MAGIC && load32(p + 8) == ~n &&
           load32(p + 12) == (MAGIC ^ n ^ UINT32_C(0x93a67d02)) &&
           n > 0 && n <= LIMIT ? n : 0;
}
static bool reserve(counter_t *c) {
    uint8_t page[256], verify[256];
    uint32_t high = 0;
    int newest = -1;
    bool all_blank = true;
    bool erased[32];
    for (unsigned i = 0; i < 32; ++i) {
        if (!c->storage.read(c->storage.context, i, page)) return false;
        erased[i] = blank(page);
        all_blank = all_blank && erased[i];
        uint32_t n = valid_high(page);
        if (n > high) { high = n; newest = (int)i; }
    }
    /* An unrecognizable existing journal must never reset the sequence to 1. */
    if (newest < 0 && !all_blank) return false;
    /* Counter exhaustion requires new endpoint credentials, never wrap. */
    if (high == LIMIT) return false;
    unsigned sector = newest < 0 ? 0u : (unsigned)newest / 16u;
    int target = -1;
    for (unsigned i = sector * 16; i < (sector + 1) * 16; ++i)
        if (erased[i]) { target = (int)i; break; }
    if (target < 0) {
        sector ^= 1u;
        /* The sector containing the latest reservation is never erased here. */
        if (!c->storage.erase(c->storage.context, sector)) return false;
        target = (int)(sector * 16);
    }
    uint32_t end = LIMIT - high < RESERVATION ? LIMIT : high + RESERVATION;
    memset(page, 0xff, sizeof(page));
    store32(page, MAGIC);
    store32(page + 4, end);
    store32(page + 8, ~end);
    store32(page + 12, MAGIC ^ end ^ UINT32_C(0x93a67d02));
    if (!c->storage.program(c->storage.context, (unsigned)target, page) ||
        !c->storage.read(c->storage.context, (unsigned)target, verify) ||
        memcmp(page, verify, sizeof(page)) != 0) return false;
    c->next = high + 1;
    c->reserved = end;
    return true;
}
bool counter_init(counter_t *c, counter_storage_t storage) {
    if (!c) return false;
    memset(c, 0, sizeof(*c));
    if (!storage.read || !storage.program || !storage.erase) return false;
    c->storage = storage;
    c->ready = reserve(c);
    return c->ready;
}
bool counter_next(counter_t *c, uint32_t *value) {
    if (!c || !value || !c->ready) return false;
    if (c->next > c->reserved && !reserve(c)) {
        c->ready = false;
        return false;
    }
    *value = c->next++;
    return true;
}
