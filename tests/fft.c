#include "../src/fft.c"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static void test_sqrt(void) {
    const float values[] = {0.0f, 1.0e-44f, 1.0e-30f, 0.001f, 1.0f,
                            1000000.0f, 1.0e20f, FLT_MAX};
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        float expected = sqrtf(values[i]);
        assert(fabsf(local_sqrt(values[i]) - expected) <= expected * 0.000001f);
    }
    assert(local_sqrt(-1.0f) == 0.0f);
    assert(isinf(local_sqrt(INFINITY)));
    assert(isnan(local_sqrt(NAN)));
}

static void test_fft_amplitude(void) {
    const float amplitudes[] = {0.001f, 1.0f, 500.0f, 1000.0f, 1000000.0f};
    float input[FFT_SIZE];
    float magnitude[FFT_SIZE / 2];
    for (uint8_t window = 0; window < 4u; ++window) {
        float reference = 0.0f;
        for (unsigned a = 0; a < sizeof(amplitudes) / sizeof(amplitudes[0]); ++a) {
            for (unsigned i = 0; i < FFT_SIZE; ++i) {
                input[i] = amplitudes[a] * sinf(6.28318530718f * 8.0f * i / FFT_SIZE + 0.3f);
            }
            compute_fft(input, magnitude, window);
            float normalized = magnitude[8] / amplitudes[a];
            if (!a) reference = normalized;
            assert(fabsf(normalized - reference) < reference * 0.00001f);
            float bin_magnitude;
            float phase;
            compute_fft_bin(input, window, 8, &bin_magnitude, &phase);
            assert(fabsf(bin_magnitude - magnitude[8]) < magnitude[8] * 0.00001f);
            assert(isfinite(phase));
        }
    }
    for (unsigned i = 0; i < FFT_SIZE; ++i) {
        float phase = 6.28318530718f * 8.0f * i / FFT_SIZE;
        input[i] = 1000.0f * sinf(phase) + 500.0f * sinf(2.0f * phase);
    }
    compute_fft(input, magnitude, 3);
    assert(fabsf(magnitude[16] / magnitude[8] - 0.5f) < 0.02f);
}

int main(void) {
    test_sqrt();
    test_fft_amplitude();
    puts("FFT amplitude regression tests passed");
    return 0;
}
