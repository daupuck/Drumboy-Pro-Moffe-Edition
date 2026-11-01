// Host unit tests for the WAV PCM -> float conversion shared by the sample loader and the
// sample preview: Core/Library/Global/SampleFormat.h.
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_sample_format.cpp -o test_sample_format && ./test_sample_format
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cmath>
#include <cstdio>
#include <cstring>

#include "SampleFormat.h"

static int failures = 0;
static int checks = 0;

#define CHECK_EQ(actual, expected)                                                                   \
    do {                                                                                             \
        checks++;                                                                                    \
        const long long a_ = (long long)(actual), e_ = (long long)(expected);                        \
        if (a_ != e_) {                                                                              \
            failures++;                                                                              \
            std::printf("FAIL %s:%d  %s == %lld, expected %lld\n", __FILE__, __LINE__, #actual, a_, e_); \
        }                                                                                            \
    } while (0)

#define CHECK_NEAR(actual, expected)                                                                 \
    do {                                                                                             \
        checks++;                                                                                    \
        const double a_ = (double)(actual), e_ = (double)(expected);                                 \
        if (std::fabs(a_ - e_) > 1e-6) {                                                             \
            failures++;                                                                              \
            std::printf("FAIL %s:%d  %s == %.9f, expected %.9f\n", __FILE__, __LINE__, #actual, a_, e_); \
        }                                                                                            \
    } while (0)

static void testPlan() {
    using sampleformat::plan;
    // 44.1 kHz: one frame per value, no upsampling.
    CHECK_EQ(plan(44100, 1, 16).offsetSize, 2);
    CHECK_EQ(plan(44100, 2, 16).offsetSize, 4);
    CHECK_EQ(plan(44100, 2, 24).offsetSize, 6);
    CHECK_EQ(plan(44100, 1, 8).offsetSize, 1);
    CHECK_EQ(plan(48000, 2, 32).offsetSize, 8);
    CHECK_EQ(plan(44100, 2, 16).writeCount, 1);
    CHECK_EQ(plan(48000, 2, 16).writeCount, 1);
    // High rates: decimate by stepping over 2 / 4 frames.
    CHECK_EQ(plan(88200, 2, 16).offsetSize, 8);
    CHECK_EQ(plan(96000, 1, 24).offsetSize, 6);
    CHECK_EQ(plan(176400, 2, 16).offsetSize, 16);
    CHECK_EQ(plan(192000, 2, 32).offsetSize, 32);
    // Low rates: sample-and-hold upsampling.
    CHECK_EQ(plan(8000, 1, 16).writeCount, 6);
    CHECK_EQ(plan(11025, 1, 16).writeCount, 4);
    CHECK_EQ(plan(16000, 1, 16).writeCount, 3);
    CHECK_EQ(plan(22050, 1, 16).writeCount, 2);
    CHECK_EQ(plan(24000, 1, 16).writeCount, 2);
    CHECK_EQ(plan(32000, 1, 16).writeCount, 1);  // 44100 / 32000 = 1 (same as the original loader)
    CHECK_EQ(plan(0, 1, 16).writeCount, 1);
    CHECK_EQ(plan(44100, 2, 24).bitsPerSample, 24);

    // Every supported format divides the preview chunk (6144) and the loader buffer (24576)
    // evenly, so a chunk never ends mid-frame.
    const unsigned rates[] = {8000, 11025, 16000, 22050, 24000, 44100, 48000, 88200, 96000, 176400, 192000};
    for (unsigned r : rates) {
        for (int ch = 1; ch <= 2; ch++) {
            for (int bits = 8; bits <= 32; bits += 8) {
                const auto p = plan(r, (uint16_t)ch, (uint16_t)bits);
                CHECK_EQ(6144 % p.offsetSize, 0);
                CHECK_EQ(24576 % p.offsetSize, 0);
            }
        }
    }
}

static void testToFloat() {
    using sampleformat::toFloat;
    // 8-bit unsigned, centre 128.
    const uint8_t u8[] = {0, 128, 255, 192};
    CHECK_NEAR(toFloat(&u8[0], 8), -1.0);
    CHECK_NEAR(toFloat(&u8[1], 8), 0.0);
    CHECK_NEAR(toFloat(&u8[2], 8), 127.0 / 128.0);
    CHECK_NEAR(toFloat(&u8[3], 8), 0.5);
    // 16-bit signed little-endian.
    const uint8_t s16[] = {0x00, 0x80, 0xFF, 0x7F, 0x00, 0x40, 0xFF, 0xFF};
    CHECK_NEAR(toFloat(&s16[0], 16), -1.0);
    CHECK_NEAR(toFloat(&s16[2], 16), 32767.0 / 32768.0);
    CHECK_NEAR(toFloat(&s16[4], 16), 0.5);
    CHECK_NEAR(toFloat(&s16[6], 16), -1.0 / 32768.0);
    // 24-bit signed little-endian with sign extension.
    const uint8_t s24[] = {0xFF, 0xFF, 0x7F, 0x01, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x40};
    CHECK_NEAR(toFloat(&s24[0], 24), 1.0);
    CHECK_NEAR(toFloat(&s24[3], 24), -8388607.0 / 8388607.0);
    CHECK_NEAR(toFloat(&s24[6], 24), -1.0 / 8388607.0);
    CHECK_NEAR(toFloat(&s24[9], 24), 4194304.0 / 8388607.0);
    // 32-bit IEEE float, copied bit for bit.
    const float f = -0.25f;
    uint8_t f32[4];
    std::memcpy(f32, &f, 4);
    CHECK_NEAR(toFloat(f32, 32), -0.25);
    // Unknown width -> silence.
    CHECK_NEAR(toFloat(u8, 12), 0.0);
}

// Mirrors the conversion loop used by both the loader and the preview, on a tiny stereo
// 16-bit 22.05 kHz file: left channel only, every value written twice.
static void testConversionLoop() {
    const uint8_t pcm[] = {0x00, 0x40, 0x11, 0x11,   // L = +0.5, R ignored
                           0x00, 0xC0, 0x22, 0x22};  // L = -0.5
    const auto p = sampleformat::plan(22050, 2, 16);
    float out[8] = {};
    unsigned n = 0;
    for (unsigned i = 0; i < sizeof(pcm) / p.offsetSize; i++) {
        const float v = sampleformat::toFloat(&pcm[i * p.offsetSize], p.bitsPerSample);
        for (unsigned k = 0; k < p.writeCount; k++) out[n++] = v;
    }
    CHECK_EQ(n, 4);
    CHECK_NEAR(out[0], 0.5);
    CHECK_NEAR(out[1], 0.5);
    CHECK_NEAR(out[2], -0.5);
    CHECK_NEAR(out[3], -0.5);
}

int main() {
    testPlan();
    testToFloat();
    testConversionLoop();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
