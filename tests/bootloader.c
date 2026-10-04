#include "../src/hw.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t slib_status;
static uint32_t slib_range;
static uint32_t *test_reg(uint32_t addr) {
    assert(addr == FLASH_R_BASE + 0xCCu || addr == FLASH_R_BASE + 0xD0u);
    return addr == FLASH_R_BASE + 0xCCu ? &slib_status : &slib_range;
}
#undef REG32
#define REG32(addr) (*test_reg(addr))
#include "../src/bootloader.c"

static void put_word(uint8_t *dst, uint32_t word) {
    for (uint8_t i = 0; i < 4; ++i) dst[i] = (uint8_t)(word >> (i * 8u));
}

int main(void) {
    uint8_t vectors[8];
    char info[192];
    assert(BOOTLOADER_SIZE == (HW_TARGET_HW40 ? 28672u : 32768u));
    assert((uintptr_t)bootloader_source() == 0x08000000u);
    assert(bootloader_crc32((const uint8_t *)"123456789", 9) == 0xCBF43926u);
    assert(bootloader_crc32((const uint8_t *)"", 0) == 0);
    slib_status = 0;
    assert(bootloader_readable());
    slib_status = 1u << 3;
    slib_range = 63u | (2u << 22);
    assert(!bootloader_readable());
    slib_range = 2u | (15u << 22);
    assert(!bootloader_readable());
    slib_range = (BOOTLOADER_SIZE / 2048u) | (63u << 22);
    assert(bootloader_readable());
    put_word(vectors, 0x20036078u);
    put_word(vectors + 4, 0x08000101u);
    assert(bootloader_vectors_valid(vectors));
    put_word(vectors + 4, APP_BASE_ADDR | 1u);
    assert(!bootloader_vectors_valid(vectors));
    uint16_t len = bootloader_describe(info, sizeof(info), 0xCBF43926u, "v2026.10.1");
    assert(len == strlen(info) && len < sizeof(info));
    assert(strstr(info, "Build target: " HW_TARGET_NAME "\r\n"));
    assert(strstr(info, "Firmware: v2026.10.1\r\n"));
    assert(strstr(info, "CRC32: CBF43926\r\n"));
    assert(strstr(info, HW_TARGET_HW40 ? "Size: 0x00007000" : "Size: 0x00008000"));
    assert(!bootloader_describe(info, 0, 0, "version"));
    assert(!bootloader_describe(info, 8, 0, "version"));
    put_word(vectors + 4, 0x08000100u);
    assert(!bootloader_vectors_valid(vectors));
    put_word(vectors + 4, 0x08000101u);
    put_word(vectors, 0xFFFFFFFFu);
    assert(!bootloader_vectors_valid(vectors));
    put_word(vectors, 0x20000002u);
    assert(!bootloader_vectors_valid(vectors));
    puts("bootloader read-only checks passed (" HW_TARGET_NAME ")");
}
