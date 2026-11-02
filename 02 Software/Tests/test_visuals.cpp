// Host tests for Global/Visuals.h (VISUALS mode maths).
#include <cstdio>
#include <cmath>
#include "../Drumboy-Pro-App-H723/Core/Library/Global/Visuals.h"

static int checks = 0;
static int failures = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        checks++;                                                        \
        if (!(c)) {                                                      \
            failures++;                                                  \
            std::printf("FAIL line %d: %s\n", __LINE__, #c);             \
        }                                                                \
    } while (0)

static float re[vis::kFftSize];
static float im[vis::kFftSize];

int main() {
    // FFT: a sine at bin k peaks at bin k (and its mirror), for several bins.
    for (uint16_t k = 1; k < 500; k += 7) {
        for (uint16_t i = 0; i < vis::kFftSize; i++) {
            re[i] = sinf(6.28318530718f * k * i / vis::kFftSize);
            im[i] = 0.0f;
        }
        vis::fft(re, im, vis::kFftSize);
        uint16_t best = 0;
        float bestMag = 0.0f;
        for (uint16_t i = 1; i < vis::kFftSize / 2; i++) {
            const float m = sqrtf(re[i] * re[i] + im[i] * im[i]);
            if (m > bestMag) {
                bestMag = m;
                best = i;
            }
        }
        CHECK(best == k);
        CHECK(fabsf(bestMag - vis::kFftSize / 2.0f) < 1.0f);  // amplitude 1 -> N/2
    }
    // FFT of silence is silence; of DC is all in bin 0.
    for (uint16_t i = 0; i < vis::kFftSize; i++) { re[i] = 0.5f; im[i] = 0.0f; }
    vis::fft(re, im, vis::kFftSize);
    CHECK(fabsf(re[0] - 512.0f) < 0.01f);
    for (uint16_t i = 1; i < vis::kFftSize; i++) CHECK(fabsf(re[i]) < 0.01f && fabsf(im[i]) < 0.01f);

    // Bands: inside the spectrum, at least one bin each, never backwards, low to high.
    uint16_t first[vis::kBands];
    uint16_t last[vis::kBands];
    for (int variant = 0; variant < 4; variant++) {
        const uint8_t bands = (variant == 0) ? 32 : ((variant == 1) ? 64 : ((variant == 2) ? 16 : 8));
        uint16_t f[64];
        uint16_t l[64];
        vis::bandRanges(bands, vis::kFftSize, 44100.0f, 40.0f, 16000.0f, f, l);
        for (uint8_t b = 0; b < bands; b++) {
            CHECK(f[b] >= 1);
            CHECK(l[b] >= f[b]);
            CHECK(l[b] <= vis::kFftSize / 2 - 1);
            if (b > 0) CHECK(f[b] > l[b - 1] || f[b] == vis::kFftSize / 2 - 1);
        }
    }
    vis::bandRanges(vis::kBands, vis::kFftSize, 44100.0f, 40.0f, 16000.0f, first, last);
    CHECK(last[vis::kBands - 1] * 44100.0f / vis::kFftSize > 12000.0f);  // the top band reaches the treble

    // Colours.
    CHECK(vis::rgb(255, 255, 255) == 0xFFFF);
    CHECK(vis::rgb(0, 0, 0) == 0x0000);
    CHECK(vis::rgb(255, 0, 0) == 0xF800);
    CHECK(vis::hue(0) == 0xF800);                 // red
    CHECK(vis::dim(0xFFFF, 0) == 0x0000);
    CHECK(vis::dim(0xF800, 128) == (15 << 11));   // half red
    CHECK(vis::heat(0) == 0x0000);
    for (int h = 0; h < 256; h++) {
        const uint16_t c = vis::hue(static_cast<uint8_t>(h));
        CHECK(c != 0);                            // full value is never black
        (void)vis::heat(static_cast<uint8_t>(h));
    }

    // Palette swap for the SD card's UI images.
    const uint16_t gold = vis::rgb(255, 194, 26);
    const uint16_t orange = vis::rgb(255, 138, 31);
    CHECK(vis::recolor(0x07FF, gold, orange) == gold);                      // factory cyan -> gold
    CHECK(vis::recolor(0x07E0, gold, orange) == gold);                      // play icon green -> gold
    CHECK(vis::recolor(0xF81F, gold, orange) == orange);                    // factory magenta -> orange
    CHECK(vis::recolor(vis::rgb(0, 128, 128), gold, orange) == vis::dim(gold, 128));  // dimmer stays dimmer
    CHECK(vis::recolor(0x0000, gold, orange) == 0x0000);                    // black stays
    CHECK(vis::recolor(0xFFFF, gold, orange) == 0xFFFF);                    // white stays
    CHECK(vis::recolor(0x8410, gold, orange) == 0x8410);                    // grey stays
    CHECK(vis::recolor(vis::rgb(255, 0, 127), gold, orange) == vis::rgb(255, 0, 127));  // record red stays
    CHECK(vis::recolor(vis::rgb(220, 0, 110), gold, orange) == vis::rgb(220, 0, 110));  // ...and its shades

    // Level mapping and follower.
    CHECK(vis::level01(0.0f) == 0.0f);
    CHECK(vis::level01(1.0f) == 1.0f);
    CHECK(vis::level01(0.001f) == 0.0f);          // -60 dB
    CHECK(fabsf(vis::level01(0.0316f) - 0.5f) < 0.01f);  // -30 dB
    CHECK(vis::follow(0.2f, 0.8f, 0.1f) == 0.8f);
    CHECK(fabsf(vis::follow(0.8f, 0.0f, 0.1f) - 0.7f) < 1e-6f);
    CHECK(vis::follow(0.05f, 0.0f, 0.1f) == 0.0f);

    // Random numbers stay in range and are not stuck.
    vis::Rng rng;
    int seen[5] = {};
    for (int i = 0; i < 1000; i++) {
        const int32_t v = rng.range(-2, 2);
        CHECK(v >= -2 && v <= 2);
        seen[v + 2]++;
    }
    for (int i = 0; i < 5; i++) CHECK(seen[i] > 100);

    // Floor lines: below the horizon, never past the bottom, nearer = lower.
    int16_t prev = 0;
    for (int i = 0; i <= 100; i++) {
        const float depth = 1.0f - i / 100.0f;
        const int16_t y = vis::floorY(300, 540, depth);
        CHECK(y >= 300 && y <= 540);
        CHECK(y >= prev);
        prev = y;
    }

    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
