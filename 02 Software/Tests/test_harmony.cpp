// Host tests for Global/Harmony.h (harmony layers: intervals, FOLLOW choice, storage, labels).
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_harmony.cpp -o test_harmony && ./test_harmony
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cstdio>
#include <cstring>

#include "Harmony.h"

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

static int scaleIndex(const char* name) {
    for (uint8_t i = 0; i < tone::kScaleCount; i++) {
        if (std::strstr(tone::kScales[i].name, name) != nullptr) return i;
    }
    return -1;
}

static void testIntervalsInKey() {
    // C major (root C): a 3rd above C is E (+4), above D is F (+3, minor): diatonic thirds.
    const int major = scaleIndex("MAJOR");
    CHECK(major >= 0);
    if (major >= 0) {
        const uint8_t s = static_cast<uint8_t>(major);
        CHECK(tone::semitones(harmony::note(0, harmony::THIRD_UP, s), s, 0) - tone::semitones(0, s, 0) == 4);
        CHECK(tone::semitones(harmony::note(1, harmony::THIRD_UP, s), s, 0) - tone::semitones(1, s, 0) == 3);
        CHECK(tone::semitones(harmony::note(0, harmony::FIFTH_UP, s), s, 0) - tone::semitones(0, s, 0) == 7);
        CHECK(tone::semitones(harmony::note(0, harmony::OCTAVE_UP, s), s, 0) - tone::semitones(0, s, 0) == 12);
        CHECK(tone::semitones(harmony::note(0, harmony::OCTAVE_DOWN, s), s, 0) - tone::semitones(0, s, 0) == -12);
        CHECK(tone::semitones(harmony::note(0, harmony::THIRD_DOWN, s), s, 0) - tone::semitones(0, s, 0) == -3);  // C -> A
    }

    // In every scale: an octave is exactly 12 semitones, and every interval goes the right way.
    for (uint8_t s = 0; s < tone::kScaleCount; s++) {
        for (int d = -10; d <= 10; d++) {
            const int base = tone::semitones(d, s, 0);
            CHECK(tone::semitones(harmony::note(static_cast<int8_t>(d), harmony::OCTAVE_UP, s), s, 0) - base == 12);
            CHECK(tone::semitones(harmony::note(static_cast<int8_t>(d), harmony::OCTAVE_DOWN, s), s, 0) - base == -12);
            for (uint8_t i = 0; i < harmony::kIntervalCount; i++) {
                const int h = tone::semitones(harmony::note(static_cast<int8_t>(d), i, s), s, 0);
                if (i < harmony::THIRD_DOWN) CHECK(h > base);
                else CHECK(h < base);
            }
        }
    }

    // At the edge of the range the note is clamped, never wraps around.
    for (uint8_t s = 0; s < tone::kScaleCount; s++) {
        const int8_t top = tone::maxDegree(s);
        CHECK(harmony::note(top, harmony::OCTAVE_UP, s) == top);
        CHECK(harmony::note(tone::minDegree(s), harmony::OCTAVE_DOWN, s) == tone::minDegree(s));
    }
    CHECK(harmony::safeInterval(200) == harmony::THIRD_UP);
}

static void testFollowChoice() {
    // Layer 3 (index 2): OFF -> L1 -> L2 -> L4 (skips itself) ... -> L10, stops at the ends.
    int8_t f = harmony::kNone;
    f = harmony::stepFollow(f, true, 2, 10);
    CHECK(f == 0);
    f = harmony::stepFollow(f, true, 2, 10);
    CHECK(f == 1);
    f = harmony::stepFollow(f, true, 2, 10);
    CHECK(f == 3);
    for (int i = 0; i < 20; i++) f = harmony::stepFollow(f, true, 2, 10);
    CHECK(f == 9);
    f = harmony::stepFollow(f, false, 2, 10);
    CHECK(f == 8);
    for (int i = 0; i < 20; i++) f = harmony::stepFollow(f, false, 2, 10);
    CHECK(f == harmony::kNone);
    // Layer 10 can't follow past L9; layer 1 starts at L2.
    CHECK(harmony::stepFollow(8, true, 9, 10) == 8);
    CHECK(harmony::stepFollow(harmony::kNone, true, 0, 10) == 1);
    CHECK(harmony::stepFollow(1, false, 0, 10) == harmony::kNone);
    // Never itself, from any start.
    for (uint8_t self = 0; self < 10; self++) {
        for (int8_t start = -1; start < 10; start++) {
            CHECK(harmony::stepFollow(start, true, self, 10) != static_cast<int8_t>(self) || start == static_cast<int8_t>(self));
            CHECK(harmony::stepFollow(start, false, self, 10) != static_cast<int8_t>(self) || start == static_cast<int8_t>(self));
        }
    }
}

static void testStorage() {
    for (uint8_t self = 0; self < 10; self++) {
        for (int8_t f = -1; f < 10; f++) {
            if (f == static_cast<int8_t>(self)) continue;
            CHECK(harmony::decodeFollow(harmony::encodeFollow(f), self, 10) == f);
        }
        CHECK(harmony::decodeFollow(harmony::encodeFollow(static_cast<int8_t>(self)), self, 10) == harmony::kNone);  // itself
    }
    CHECK(harmony::decodeFollow(0, 0, 10) == harmony::kNone);
    CHECK(harmony::decodeFollow(11, 0, 10) == harmony::kNone);
    CHECK(harmony::decodeFollow(0xFF, 0, 10) == harmony::kNone);
}

static void testLabels() {
    char out[11];
    harmony::label(harmony::kNone, 0, out);
    CHECK(std::strcmp(out, "       OFF") == 0);
    harmony::label(2, harmony::THIRD_UP, out);
    CHECK(std::strcmp(out, "L3  3RD UP") == 0);
    harmony::label(9, harmony::OCTAVE_DOWN, out);
    CHECK(std::strcmp(out, "L10 OCT DN") == 0);
    for (int8_t f = -1; f < 10; f++) {
        for (uint8_t i = 0; i < harmony::kIntervalCount; i++) {
            harmony::label(f, i, out);
            CHECK(std::strlen(out) == 10);
        }
    }
}

int main() {
    testIntervalsInKey();
    testFollowChoice();
    testStorage();
    testLabels();
    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
