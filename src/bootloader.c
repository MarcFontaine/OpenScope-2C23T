#include "bootloader.h"
#include "hw.h"

_Static_assert(BOOTLOADER_SIZE == (HW_TARGET_HW40 ? 0x7000u : 0x8000u),
               "Bootloader export must match the hardware application's entry point");

uint8_t bootloader_readable(void) {
    // Artery's SLIB_ENF and sector range are read-only; never unlock protection.
    if (REG32(FLASH_R_BASE + 0xCCu) & (1u << 3)) {
        uint32_t range = REG32(FLASH_R_BASE + 0xD0u);
        uint32_t start = range & 0x7FFu;
        uint32_t end = range >> 22;
        if (start > end || start < BOOTLOADER_SIZE / 2048u) {
            return 0;
        }
    }
    return 1;
}

static uint32_t bootloader_word(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

uint8_t bootloader_vectors_valid(const uint8_t *data) {
    uint32_t sp = bootloader_word(data);
    uint32_t pc = bootloader_word(data + 4);
    return sp > 0x20000000u && sp <= 0x20038000u && !(sp & 3u) &&
        (pc & 1u) && (pc & ~1u) >= BOOTLOADER_BASE && (pc & ~1u) < APP_BASE_ADDR;
}

const uint8_t *bootloader_source(void) {
    return (const uint8_t *)(uintptr_t)BOOTLOADER_BASE;
}

uint32_t bootloader_crc32(const uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    while (len--) {
        crc ^= *data++;
        for (uint8_t bit = 0; bit < 8u; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
        }
    }
    return ~crc;
}

static uint8_t bootloader_append(char *out, uint16_t capacity, uint16_t *pos,
                                 const char *text) {
    while (*text) {
        if (*pos + 1u >= capacity) {
            return 0;
        }
        out[(*pos)++] = *text++;
    }
    out[*pos] = 0;
    return 1;
}

static void bootloader_hex(char out[9], uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < 8u; ++i) {
        out[i] = digits[(value >> ((7u - i) * 4u)) & 15u];
    }
    out[8] = 0;
}

uint16_t bootloader_describe(char *out, uint16_t capacity, uint32_t crc, const char *version) {
    uint16_t pos = 0;
    char hex[9];
    if (!out || !capacity || !version) {
        return 0;
    }
    if (!bootloader_append(out, capacity, &pos, "FNIRSI bootloader backup\r\nBuild target: " HW_TARGET_NAME "\r\nFirmware: ") ||
        !bootloader_append(out, capacity, &pos, version) ||
        !bootloader_append(out, capacity, &pos, "\r\nStart: 0x08000000\r\nSize: 0x")) {
        return 0;
    }
    bootloader_hex(hex, BOOTLOADER_SIZE);
    if (!bootloader_append(out, capacity, &pos, hex) ||
        !bootloader_append(out, capacity, &pos, "\r\nCRC32: ")) {
        return 0;
    }
    bootloader_hex(hex, crc);
    if (!bootloader_append(out, capacity, &pos, hex) ||
        !bootloader_append(out, capacity, &pos, "\r\n")) {
        return 0;
    }
    return pos;
}
