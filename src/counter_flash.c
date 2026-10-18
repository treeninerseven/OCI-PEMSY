#include "pemsy/counter.h"
#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"
#include "pico/flash.h"
#include <string.h>

/* CMake reserves the final 8 KiB from the linker's FLASH region. */
#define STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - 2u * FLASH_SECTOR_SIZE)
typedef struct { unsigned offset; const uint8_t *data; } flash_operation_t;
static void program_safe(void *arg) {
    flash_operation_t *op = arg;
    flash_range_program(op->offset, op->data, FLASH_PAGE_SIZE);
}
static void erase_safe(void *arg) {
    flash_operation_t *op = arg;
    flash_range_erase(op->offset, FLASH_SECTOR_SIZE);
}
static bool read_page(void *ctx, unsigned page, uint8_t *data) {
    (void)ctx;
    if (page >= 32) return false;
    memcpy(data, (const void *)(XIP_BASE + STORAGE_OFFSET + page * 256u), 256);
    return true;
}
static bool program_page(void *ctx, unsigned page, const uint8_t *data) {
    (void)ctx;
    if (page >= 32) return false;
    flash_operation_t op = {STORAGE_OFFSET + page * 256u, data};
    return flash_safe_execute(program_safe, &op, 1000) == PICO_OK;
}
static bool erase_sector(void *ctx, unsigned sector) {
    (void)ctx;
    if (sector >= 2) return false;
    flash_operation_t op = {STORAGE_OFFSET + sector * 4096u, NULL};
    return flash_safe_execute(erase_safe, &op, 1000) == PICO_OK;
}
counter_storage_t counter_flash_storage(void) {
    return (counter_storage_t){NULL, read_page, program_page, erase_sector};
}
