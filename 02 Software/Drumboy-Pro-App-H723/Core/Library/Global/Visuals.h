#ifndef __VISUALS_H
#define __VISUALS_H

// VISUALS mode (v1.14): the maths behind the full-screen music visuals - FFT, spectrum bands,
// colours, random numbers. Pure C++ with no hardware access, so it is host-tested
// (02 Software/Tests/test_visuals.cpp). The drawing itself is in ControllerVisual.cpp.

#include <stdint.h>
#include <math.h>

namespace vis {

// ---- Capture (written by the audio ISR while VISUALS is on) --------------------------------

constexpr uint16_t kScopeSize = 1024;   // power of two: ring index wraps with a mask
constexpr uint16_t kFftSize = 1024;     // 43 Hz per bin at 44.1 kHz
constexpr uint8_t kLayers = 10;
constexpr uint8_t kBands = 32;

struct Tap {
    float scopeL[kScopeSize];
    float scopeR[kScopeSize];
    volatile uint32_t pos;              // next write index (free-running, masked on use)
    volatile float peakL;               // max |sample| since the main loop last read it
    volatile float peakR;
    volatile float layerPeak[kLayers];  // per layer, same
    volatile float hit[kLayers];        // velocity of the last trigger, 0 = none since last read
};

// ---- Colours (RGB565) ----------------------------------------------------------------------

inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Hue 0..255 around the colour wheel, value 0..255 (full saturation).
inline uint16_t hue(uint8_t h, uint8_t v = 255) {
    const uint8_t region = h / 43;
    const uint8_t rem = static_cast<uint8_t>((h - (region * 43)) * 6);
    const uint8_t q = static_cast<uint8_t>((v * (255 - rem)) >> 8);
    const uint8_t t = static_cast<uint8_t>((v * rem) >> 8);
    switch (region) {
        case 0: return rgb(v, t, 0);
        case 1: return rgb(q, v, 0);
        case 2: return rgb(0, v, t);
        case 3: return rgb(0, q, v);
        case 4: return rgb(t, 0, v);
        default: return rgb(v, 0, q);
    }
}

// Scales a colour's brightness by level/255.
inline uint16_t dim(uint16_t c, uint8_t level) {
    const uint32_t r = (c >> 11) & 0x1F;
    const uint32_t g = (c >> 5) & 0x3F;
    const uint32_t b = c & 0x1F;
    return static_cast<uint16_t>((((r * level) >> 8) << 11) | (((g * level) >> 8) << 5) | ((b * level) >> 8));
}

// Moffe Edition palette swap for the SD card's UI images (v2.00 "Gold Rush"): factory cyan and
// green become `gold`, factory magenta becomes `second`, each at the original brightness so
// anti-aliased edges stay smooth. Greys, black, white and the red record icon are left alone.
inline uint16_t recolor(uint16_t c, uint16_t gold, uint16_t second) {
    const uint32_t r = ((c >> 11) & 0x1F) << 3;
    const uint32_t g = ((c >> 5) & 0x3F) << 2;
    const uint32_t b = (c & 0x1F) << 3;
    uint32_t mx = r;
    if (g > mx) mx = g;
    if (b > mx) mx = b;
    if (mx < 24) return c;  // (near) black
    const uint32_t hi = (mx * 6) / 10;
    const uint32_t lo = (mx * 4) / 10;
    const uint8_t level = static_cast<uint8_t>((mx > 255) ? 255 : mx);
    const bool cyan = (g >= hi) && (b >= hi) && (r <= lo);
    const bool green = (g >= hi) && (r <= lo) && (b <= lo);
    const bool magenta = (r >= hi) && (b >= hi) && (g <= lo);
    if (!(cyan || green || magenta)) return c;
    const uint16_t target = magenta ? second : gold;
    return (mx >= 248) ? target : dim(target, level);  // full brightness: exactly the target
}

// Heat-map colour for 0..255: black -> blue -> magenta -> orange -> yellow -> white.
inline uint16_t heat(uint8_t x) {
    if (x < 64) return rgb(0, 0, static_cast<uint8_t>(x * 4));
    if (x < 128) return rgb(static_cast<uint8_t>((x - 64) * 4), 0, 255);
    if (x < 192) return rgb(255, static_cast<uint8_t>((x - 128) * 3), static_cast<uint8_t>(255 - ((x - 128) * 4)));
    return rgb(255, static_cast<uint8_t>(192 + ((x - 192))), static_cast<uint8_t>((x - 192) * 4));
}

// ---- Random numbers (xorshift32, never 0) --------------------------------------------------

struct Rng {
    uint32_t s = 0x9E3779B9u;
    uint32_t next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    int32_t range(int32_t lo, int32_t hi) {  // lo..hi inclusive
        return lo + static_cast<int32_t>(next() % static_cast<uint32_t>(hi - lo + 1));
    }
};

// ---- FFT -----------------------------------------------------------------------------------

// In-place iterative radix-2 complex FFT; n must be a power of two.
inline void fft(float* re, float* im, uint16_t n) {
    for (uint16_t i = 1, j = 0; i < n; i++) {  // bit reversal
        uint16_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (uint16_t len = 2; len <= n; len <<= 1) {
        const float ang = -6.28318530718f / static_cast<float>(len);
        const float wr = cosf(ang);
        const float wi = sinf(ang);
        for (uint16_t i = 0; i < n; i += len) {
            float cr = 1.0f;
            float ci = 0.0f;
            for (uint16_t k = 0; k < len / 2; k++) {
                const uint16_t a = i + k;
                const uint16_t b = a + (len / 2);
                const float xr = (re[b] * cr) - (im[b] * ci);
                const float xi = (re[b] * ci) + (im[b] * cr);
                re[b] = re[a] - xr;
                im[b] = im[a] - xi;
                re[a] += xr;
                im[a] += xi;
                const float ncr = (cr * wr) - (ci * wi);
                ci = (cr * wi) + (ci * wr);
                cr = ncr;
            }
        }
    }
}

// Log-spaced bands: first[b]..last[b] (inclusive FFT bins) for `bands` bands from fLo to fHi.
// Every band has at least one bin and bands never go backwards.
inline void bandRanges(uint8_t bands, uint16_t fftSize, float fs, float fLo, float fHi, uint16_t* first, uint16_t* last) {
    const float binHz = fs / static_cast<float>(fftSize);
    const uint16_t maxBin = static_cast<uint16_t>((fftSize / 2) - 1);
    uint16_t prev = 0;
    for (uint8_t b = 0; b < bands; b++) {
        const float f0 = fLo * powf(fHi / fLo, static_cast<float>(b) / bands);
        const float f1 = fLo * powf(fHi / fLo, static_cast<float>(b + 1) / bands);
        uint16_t lo = static_cast<uint16_t>(f0 / binHz);
        uint16_t hi = static_cast<uint16_t>(f1 / binHz);
        if (lo < 1) lo = 1;
        if (lo <= prev && b > 0) lo = static_cast<uint16_t>(prev + 1);
        if (lo > maxBin) lo = maxBin;
        if (hi < lo) hi = lo;
        if (hi > maxBin) hi = maxBin;
        first[b] = lo;
        last[b] = hi;
        prev = hi;
    }
}

// Magnitude (linear, full scale ~1) to 0..1 over a 60 dB window.
inline float level01(float mag) {
    if (mag <= 1e-6f) return 0.0f;
    const float db = 20.0f * log10f(mag);
    const float x = (db + 60.0f) / 60.0f;
    return (x < 0.0f) ? 0.0f : ((x > 1.0f) ? 1.0f : x);
}

// Peak follower: jumps up at once, falls by `fall` per frame.
inline float follow(float current, float input, float fall) {
    if (input >= current) return input;
    const float v = current - fall;
    return (v > input) ? v : input;
}

// ---- Perspective floor ---------------------------------------------------------------------

// Screen y of a floor line at depth z (1 = horizon far away ... 0 = near the viewer):
// lines bunch up towards the horizon like a road. phase 0..1 moves them towards the viewer.
inline int16_t floorY(int16_t horizon, int16_t bottom, float depth) {
    const float d = (depth < 0.02f) ? 0.02f : depth;
    const float y = static_cast<float>(horizon) + (static_cast<float>(bottom - horizon) * (0.12f / d));
    return (y > static_cast<float>(bottom)) ? bottom : static_cast<int16_t>(y);
}

}  // namespace vis

#endif
