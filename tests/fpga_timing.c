#include "../src/hw.h"

#include <assert.h>
#include <stdio.h>

static struct { uint32_t address; uint32_t value; } registers[32];
static unsigned register_count;
static uint8_t transmitted[32];
static unsigned transmitted_count;

static volatile uint32_t *mock_register(uint32_t address) {
    for (unsigned i = 0; i < register_count; ++i) {
        if (registers[i].address == address) return &registers[i].value;
    }
    assert(register_count < 32);
    registers[register_count].address = address;
    return &registers[register_count++].value;
}

#undef REG32
#define REG32(address) (*mock_register(address))

static void mock_gpio_set(uint32_t base, uint32_t mask) {
    GPIO_ODR(base) |= mask;
#if HW_TARGET_HW40
    if (base == GPIOB_BASE && mask == (1u << 3)) {
        assert(transmitted_count < sizeof(transmitted));
        transmitted[transmitted_count++] = (uint8_t)GPIO_ODR(GPIOC_BASE);
    }
#else
    if (base == GPIOA_BASE && mask == (1u << 15)) {
        assert(transmitted_count < sizeof(transmitted));
        transmitted[transmitted_count++] = (uint8_t)SPI_DT(SPI3_BASE);
    }
#endif
}
static void mock_gpio_clear(uint32_t base, uint32_t mask) { GPIO_ODR(base) &= ~mask; }
static uint32_t mock_gpio_read(uint32_t base, uint32_t mask) { return GPIO_IDR(base) & mask; }
#define gpio_set mock_gpio_set
#define gpio_clear mock_gpio_clear
#define gpio_read mock_gpio_read
#include "../src/fpga.c"

void gpio_config_mask(uint32_t base, uint16_t mask, uint8_t cfg) {
    (void)base; (void)mask; (void)cfg;
}

static uint32_t read_be32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | bytes[3];
}

static void assert_frame(uint32_t word, uint32_t span) {
    unsigned offset = HW_TARGET_HW40 ? 0u : 1u;
    assert(transmitted_count == (HW_TARGET_HW40 ? 16u : 9u));
    assert(read_be32(&transmitted[offset]) == word);
    assert(read_be32(&transmitted[offset + 4]) == span);
    assert(fpga_last_tuning_word == word && fpga_last_span == span);
#if HW_TARGET_HW40
    for (unsigned i = 0; i < 8; ++i) assert(transmitted[8 + i] == i);
#else
    assert(transmitted[0] == 0);
#endif
    transmitted_count = 0;
}

int main(void) {
    SPI_STS(SPI3_BASE) = 2;
    fpga_write_generator_timing(42950);
    assert_frame(42950, FPGA_SAMPLE_COUNT);
    static const uint32_t spans[] = {0, 52, 52428, 0x80000, 0x100000};
    for (unsigned i = 0; i < sizeof(spans) / sizeof(spans[0]); ++i) {
        fpga_write_scope_timing(spans[i]);
        assert_frame(42950, spans[i]);
        fpga_write_generator_timing(85900);
        assert_frame(85900, spans[i]);
        fpga_write_generator_timing(0);
        assert_frame(0, spans[i]);
        fpga_write_generator_timing(42950);
        assert_frame(42950, spans[i]);
    }
    printf("FPGA timing: independent DDS/capture fields and hardware packet framing passed (HW4=%d)\n", HW_TARGET_HW40);
    return 0;
}
