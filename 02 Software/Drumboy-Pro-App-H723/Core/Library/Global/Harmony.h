#ifndef __HARMONY_H
#define __HARMONY_H

// Harmony layers (v2.04): a TONE or SYNTH layer can FOLLOW another layer and play every note
// it plays, an interval higher or lower in the followed layer's scale (so a 3rd is major or minor
// wherever the key says). Pure C++ with no hardware access, so it is host-tested
// (02 Software/Tests/test_harmony.cpp). The playing is in ControllerLayer.cpp (layer_playBeat).
//
// Stored per layer in the project's .rwn note file as the "HRM1" section:
//   10 x [follow (0 = off, 1..10 = layer), interval] + "EOF"
// Older firmware stops reading before it and plays the layers as before.

#include <stdint.h>

#include "ToneScale.h"

namespace harmony {

constexpr int8_t kNone = -1;

// Intervals in scale steps; an octave is the scale's own size (7 for major/minor, 5 for
// pentatonic). In 5- and 6-note scales the "3rd" etc. are the scale's next notes, which is what
// keeps them in key.
enum Interval : uint8_t { THIRD_UP = 0, FOURTH_UP, FIFTH_UP, SIXTH_UP, OCTAVE_UP, THIRD_DOWN, FOURTH_DOWN, FIFTH_DOWN, SIXTH_DOWN, OCTAVE_DOWN, kIntervalCount };

constexpr const char* kIntervalNames[kIntervalCount] = {"3RD UP", "4TH UP", "5TH UP", "6TH UP", "OCT UP",
                                                        "3RD DN", "4TH DN", "5TH DN", "6TH DN", "OCT DN"};

inline uint8_t safeInterval(uint8_t interval) { return (interval < kIntervalCount) ? interval : static_cast<uint8_t>(THIRD_UP); }

// Scale steps for an interval in a scale.
inline int steps(uint8_t interval, uint8_t scale) {
    const int octave = tone::kScales[tone::safeScale(scale)].size;
    switch (safeInterval(interval)) {
        case THIRD_UP: return 2;
        case FOURTH_UP: return 3;
        case FIFTH_UP: return 4;
        case SIXTH_UP: return 5;
        case OCTAVE_UP: return octave;
        case THIRD_DOWN: return -2;
        case FOURTH_DOWN: return -3;
        case FIFTH_DOWN: return -4;
        case SIXTH_DOWN: return -5;
        default: return -octave;  // OCTAVE_DOWN
    }
}

// The harmony note (a scale degree in the followed layer's scale) for a followed note.
inline int8_t note(int8_t followed, uint8_t interval, uint8_t scale) {
    return tone::clampDegree(followed + steps(interval, scale), scale);
}

// Next / previous FOLLOW choice for a layer: OFF (-1), then the other layers 0..layers-1
// (never itself).
inline int8_t stepFollow(int8_t current, bool up, uint8_t self, uint8_t layers) {
    int8_t f = current;
    for (uint8_t guard = 0; guard <= layers; guard++) {
        if (up) {
            if (f >= static_cast<int8_t>(layers - 1)) return current;  // stays at the last choice
            f++;
        } else {
            if (f <= kNone) return current;  // stays at OFF
            f--;
        }
        if (f != static_cast<int8_t>(self)) return f;
    }
    return current;
}

// Card bytes. Anything invalid (or a layer following itself) reads as "not following".
inline uint8_t encodeFollow(int8_t follow) { return (follow >= 0) ? static_cast<uint8_t>(follow + 1) : 0; }
inline int8_t decodeFollow(uint8_t b, uint8_t self, uint8_t layers) {
    if ((b == 0) || (b > layers)) return kNone;
    const int8_t f = static_cast<int8_t>(b - 1);
    return (f == static_cast<int8_t>(self)) ? kNone : f;
}

// FOLLOW knob text, 10 characters: "       OFF" or "L3  3RD UP".
inline void label(int8_t follow, uint8_t interval, char* out) {
    for (uint8_t i = 0; i < 10; i++) out[i] = ' ';
    out[10] = 0;
    if (follow < 0) {
        out[7] = 'O';
        out[8] = 'F';
        out[9] = 'F';
        return;
    }
    uint8_t p = 0;
    out[p++] = 'L';
    const int n = follow + 1;
    if (n >= 10) out[p++] = static_cast<char>('0' + (n / 10));
    out[p++] = static_cast<char>('0' + (n % 10));
    const char* name = kIntervalNames[safeInterval(interval)];
    uint8_t len = 0;
    while (name[len] != 0) len++;
    for (uint8_t i = 0; i < len; i++) out[10 - len + i] = name[i];  // right-aligned
}

}  // namespace harmony

#endif
