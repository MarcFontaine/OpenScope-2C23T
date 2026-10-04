#pragma once

#include "app_config.h"

#include <stdint.h>

enum {
    BOOTLOADER_BASE = 0x08000000u,
    BOOTLOADER_SIZE = APP_BASE_ADDR - BOOTLOADER_BASE,
};

#if HW_TARGET_HW40
#define BOOTLOADER_FILE_NAME "boot_hw4.bin"
#define BOOTLOADER_FAT_NAME "BOOT_HW4BIN"
#define BOOTLOADER_FAT_INFO_NAME "BOOT_HW4TXT"
#else
#define BOOTLOADER_FILE_NAME "boot_old.bin"
#define BOOTLOADER_FAT_NAME "BOOT_OLDBIN"
#define BOOTLOADER_FAT_INFO_NAME "BOOT_OLDTXT"
#endif

uint8_t bootloader_readable(void);
uint8_t bootloader_vectors_valid(const uint8_t *data);
const uint8_t *bootloader_source(void);
uint32_t bootloader_crc32(const uint8_t *data, uint32_t len);
uint16_t bootloader_describe(char *out, uint16_t capacity, uint32_t crc, const char *version);
