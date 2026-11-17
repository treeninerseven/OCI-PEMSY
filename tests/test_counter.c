#include "pemsy/counter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { uint8_t pages[32][256]; int fail_bytes; bool fail_erase; } flash_t;
static bool read_page(void *ctx, unsigned page, uint8_t *out) {
    assert(page < 32);
    memcpy(out, ((flash_t *)ctx)->pages[page], 256);
    return true;
}
static bool program(void *ctx, unsigned page, const uint8_t *data) {
    flash_t *f = ctx;
    unsigned count = f->fail_bytes < 0 ? 256u : (unsigned)f->fail_bytes;
    for (unsigned i = 0; i < count; ++i) f->pages[page][i] &= data[i];
    return count == 256;
}
static bool erase(void *ctx, unsigned sector) {
    flash_t *f = ctx;
    if (f->fail_erase) return false;
    memset(f->pages + sector * 16, 0xff, 4096);
    return true;
}
static counter_storage_t storage(flash_t *f) {
    return (counter_storage_t){f, read_page, program, erase};
}
static void reset(flash_t *f) { memset(f, 0xff, sizeof(*f)); f->fail_erase = false; }
int main(void) {
    flash_t f;
    counter_t c;
    uint32_t n;
    reset(&f);
    assert(counter_init(&c, storage(&f)));
    for (uint32_t i = 1; i <= 257; ++i) {
        assert(counter_next(&c, &n) && n == i);
    }
    assert(counter_init(&c, storage(&f)));
    assert(counter_next(&c, &n) && n == 513);
    uint32_t previous = n;
    /* Repeated boots force both sector rollovers while retaining monotonicity. */
    for (unsigned i = 0; i < 70; ++i) {
        assert(counter_init(&c, storage(&f)));
        assert(counter_next(&c, &n) && n > previous);
        previous = n;
    }
    /* Tear the next reservation at every byte of its record, then reboot. */
    for (int cut = 0; cut <= 16; ++cut) {
        reset(&f);
        assert(counter_init(&c, storage(&f)));
        assert(counter_next(&c, &n) && n == 1);
        f.fail_bytes = cut;
        assert(!counter_init(&c, storage(&f)));
        assert(!counter_next(&c, &n));
        f.fail_bytes = -1;
        assert(counter_init(&c, storage(&f)));
        assert(counter_next(&c, &n) && n > 256);
    }
    reset(&f);
    for (unsigned i = 0; i < 16; ++i) assert(counter_init(&c, storage(&f)));
    f.fail_erase = true;
    assert(!counter_init(&c, storage(&f)));
    f.fail_erase = false;
    assert(counter_init(&c, storage(&f)));
    assert(counter_next(&c, &n) && n == 4097);
    assert(!counter_init(NULL, storage(&f)));
    puts("persistent counter tests passed");
}
