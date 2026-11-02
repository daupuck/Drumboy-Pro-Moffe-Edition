// Host unit tests for TONE layers: Core/Library/Global/ToneScale.h.
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_tone_scale.cpp -o test_tone_scale && ./test_tone_scale
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cmath>
#include <cstdio>
#include <cstring>

#include "ToneScale.h"

using namespace tone;

static int failures = 0;
static int checks = 0;

#define CHECK(cond, ...)                                     \
    do {                                                     \
        checks++;                                            \
        if (!(cond)) {                                       \
            failures++;                                      \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__); \
            std::printf(__VA_ARGS__);                        \
            std::printf("\n");                               \
        }                                                    \
    } while (0)

static int scaleIndex(const char* name) {
    for (int i = 0; i < kScaleCount; i++) {
        if (std::strcmp(kScales[i].name, name) == 0) return i;
    }
    return -1;
}

static void testScaleTables() {
    for (int s = 0; s < kScaleCount; s++) {
        const Scale& sc = kScales[s];
        CHECK((sc.size >= 5) && (sc.size <= 12), "scale %s size %u", sc.name, sc.size);
        CHECK(sc.steps[0] == 0, "scale %s starts on the root", sc.name);
        for (int i = 1; i < sc.size; i++) {
            CHECK(sc.steps[i] > sc.steps[i - 1], "scale %s rises at %d", sc.name, i);
            CHECK(sc.steps[i] < 12, "scale %s stays inside one octave at %d", sc.name, i);
        }
        CHECK(std::strlen(sc.name) <= 10, "scale name %s fits a 10-character field", sc.name);
    }
    CHECK(scaleIndex("MINOR") == kDefaultScale, "default scale is MINOR");
}

static void testSemitones() {
    const uint8_t major = scaleIndex("MAJOR");
    const uint8_t minor = scaleIndex("MINOR");
    const uint8_t chroma = scaleIndex("CHROMATIC");
    const uint8_t pentaMin = scaleIndex("PENTA MIN");

    // C major, one octave up and down.
    const int up[8] = {0, 2, 4, 5, 7, 9, 11, 12};
    for (int d = 0; d < 8; d++) CHECK(semitones(d, major, 0) == up[d], "C major degree %d -> %d", d, semitones(d, major, 0));
    const int down[8] = {0, -1, -3, -5, -7, -8, -10, -12};
    for (int d = 0; d < 8; d++) CHECK(semitones(-d, major, 0) == down[d], "C major degree %d -> %d", -d, semitones(-d, major, 0));

    // A minor: root A sits 3 semitones below C (G..B go down), degree 2 is C.
    CHECK(semitones(0, minor, 9) == -3, "A minor root");
    CHECK(semitones(2, minor, 9) == 0, "A minor degree 2 = C");
    CHECK(semitones(7, minor, 9) == 9, "A minor octave");

    // Root offsets: C..F# up 0..6, G..B down 5..1.
    const int offsets[12] = {0, 1, 2, 3, 4, 5, 6, -5, -4, -3, -2, -1};
    for (int r = 0; r < 12; r++) CHECK(rootOffset(r) == offsets[r], "root %d offset %d", r, rootOffset(r));

    // Chromatic is 1:1.
    for (int d = -24; d <= 24; d++) CHECK(semitones(d, chroma, 0) == d, "chromatic %d", d);

    // Pentatonic minor: two octaves = 10 degrees.
    CHECK(maxDegree(pentaMin) == 10, "penta range");
    CHECK(semitones(10, pentaMin, 0) == 24, "penta top");
    CHECK(semitones(-10, pentaMin, 0) == -24, "penta bottom");

    // Clamping: beyond two octaves the note stays at the edge (a smaller scale never wraps).
    CHECK(semitones(40, major, 0) == semitones(14, major, 0), "clamped high");
    CHECK(semitones(-40, major, 0) == semitones(-14, major, 0), "clamped low");
    CHECK(clampDegree(24, pentaMin) == 10, "chromatic degree clamped into penta");

    // Every degree of every scale and root stays within +-30 semitones and rises monotonically.
    for (int s = 0; s < kScaleCount; s++) {
        for (int r = 0; r < 12; r++) {
            int previous = -1000;
            for (int d = minDegree(s); d <= maxDegree(s); d++) {
                const int st = semitones(d, s, r);
                CHECK((st >= -30) && (st <= 30), "scale %d root %d degree %d -> %d", s, r, d, st);
                CHECK(st > previous, "scale %d root %d rises at %d", s, r, d);
                previous = st;
            }
        }
    }

    // A bad scale index never reads outside the table.
    CHECK(safeScale(200) == kDefaultScale, "safeScale");
    CHECK(semitones(3, 200, 0) == semitones(3, kDefaultScale, 0), "bad scale uses the default");
}

static void testRatio() {
    for (int s = -36; s <= 36; s++) {
        const float expected = std::pow(2.0f, s / 12.0f);
        CHECK(std::fabs(ratio(s) - expected) < expected * 1e-5f, "ratio %d = %f (expected %f)", s, ratio(s), expected);
    }
    CHECK(ratio(0) == 1.0f, "unity is exact (keeps the fast audio path)");
    CHECK(ratio(12) == 2.0f && ratio(-12) == 0.5f, "octaves are exact");
    CHECK(clampIncrement(100.0f) == kMaxIncrement, "increment max");
    CHECK(clampIncrement(0.0001f) == kMinIncrement, "increment min");
    CHECK(clampIncrement(1.5f) == 1.5f, "increment passes");
}

static void testNames() {
    char name[8];
    noteName(0, name, sizeof(name));
    CHECK(std::strcmp(name, "C4") == 0, "C4: %s", name);
    noteName(3, name, sizeof(name));
    CHECK(std::strcmp(name, "D#4") == 0, "D#4: %s", name);
    noteName(-3, name, sizeof(name));
    CHECK(std::strcmp(name, "A3") == 0, "A3: %s", name);
    noteName(-13, name, sizeof(name));
    CHECK(std::strcmp(name, "B2") == 0, "B2: %s", name);
    noteName(30, name, sizeof(name));
    CHECK(std::strcmp(name, "F#6") == 0, "F#6: %s", name);
    noteName(-30, name, sizeof(name));
    CHECK(std::strcmp(name, "F#1") == 0, "F#1: %s", name);

    CHECK(midiNote(0) == 60, "midi C4");
    CHECK(midiNote(-12) == 48, "midi C3");
    CHECK(midiNote(-100) == 0 && midiNote(100) == 127, "midi clamped");
}

static void testGlide() {
    const float bps = 44100.0f / 32.0f;
    CHECK(glideBlockCoef(0, bps) == 0.0f, "glide off");
    float previous = 0.0f;
    for (int g = 1; g <= 100; g++) {
        const float c = glideBlockCoef(g, bps);
        CHECK((c > 0.0f) && (c < 1.0f), "glide %d coef %f in (0,1)", g, c);
        CHECK(c > previous, "glide %d slower than %d", g, g - 1);
        previous = c;
    }
    // GLIDE 100: after 0.5 s (689 blocks) the speed has covered 1 - 1/e of the distance.
    const float c100 = glideBlockCoef(100, bps);
    const float remaining = std::pow(c100, 0.5f * bps);
    CHECK(std::fabs(remaining - std::exp(-1.0f)) < 0.01f, "glide 100 time constant (remaining %f)", remaining);
    CHECK(glideBlockCoef(250, bps) == c100, "glide clamped to 100");
}

// The old per-note clamping, kept to show the bug this guards against.
static void transposeEachClamped(int8_t* notes, uint8_t count, int delta, uint8_t scale) {
    for (uint8_t i = 0; i < count; i++) notes[i] = clampDegree(clampDegree(notes[i], scale) + delta, scale);
}

static void testTranspose() {
    const uint8_t minor = scaleIndex("MINOR");  // range -14..+14
    const int8_t melody[4] = {0, 4, -2, 7};

    // Regression: turning TRANSPOSE far up and back down must give the melody back.
    {
        int8_t old[4], fixed[4];
        for (int i = 0; i < 4; i++) old[i] = fixed[i] = melody[i];
        int moved = 0;
        for (int step = 0; step < 30; step++) {
            transposeEachClamped(old, 4, +1, minor);
            if (transposeAll(fixed, 4, +1, minor)) moved++;
        }
        for (int step = 0; step < 30; step++) {
            transposeEachClamped(old, 4, -1, minor);
            transposeAll(fixed, 4, -1, minor);
        }
        CHECK(old[0] == old[1] && old[1] == old[2] && old[2] == old[3], "the old code squashed the melody into one note");
        CHECK(moved == 7, "up stops when the highest note (7) reaches +14: moved %d", moved);
        // 30 steps down from the top: the melody goes down until its lowest note hits -14.
        const int8_t expected[4] = {-12, -8, -14, -5};
        for (int i = 0; i < 4; i++) CHECK(fixed[i] == expected[i], "note %d = %d after far down (expected %d)", i, fixed[i], expected[i]);
        int8_t back[4];
        for (int i = 0; i < 4; i++) back[i] = fixed[i];
        for (int step = 0; step < 12; step++) transposeAll(back, 4, +1, minor);
        for (int i = 0; i < 4; i++) CHECK(back[i] == melody[i], "note %d restored: %d (expected %d)", i, back[i], melody[i]);
    }

    // Intervals are kept on every step for every scale, up and down.
    for (int s = 0; s < kScaleCount; s++) {
        int8_t notes[5] = {0, 1, -1, 3, -3};
        for (int step = 0; step < 60; step++) {
            const int delta = (step < 30) ? 1 : -1;
            int8_t before[5];
            for (int i = 0; i < 5; i++) before[i] = notes[i];
            const bool moved = transposeAll(notes, 5, delta, s);
            for (int i = 0; i < 5; i++) {
                CHECK(notes[i] - notes[0] == before[i] - before[0], "scale %d step %d keeps intervals", s, step);
                CHECK((notes[i] >= minDegree(s)) && (notes[i] <= maxDegree(s)), "scale %d step %d stays in range", s, step);
                if (!moved) CHECK(notes[i] == before[i], "scale %d step %d: refused step changes nothing", s, step);
            }
        }
    }

    // Edge cases.
    int8_t one[1] = {0};
    CHECK(!transposeAll(one, 0, 1, minor), "empty bank");
    CHECK(!transposeAll(one, 1, 0, minor), "zero delta");
    int8_t wide[2] = {20, 0};  // 20 only exists in a wider scale; in MINOR it sounds as +14
    CHECK(!transposeAll(wide, 2, 1, minor) && wide[0] == 20, "already at the top: refused, untouched");
    CHECK(transposeAll(wide, 2, -1, minor) && wide[0] == 13 && wide[1] == -1, "down: brought into range first");
}

static void testMelody() {
    for (int s = 0; s < kScaleCount; s++) {
        const int span = melodySpan(s);
        for (uint32_t run = 1; run <= 200; run++) {
            for (uint8_t count = 1; count <= 64; count = static_cast<uint8_t>(count * 2)) {
                int8_t notes[64];
                uint32_t seed = run * 2654435761u;
                generateMelody(notes, count, s, seed);
                CHECK(notes[0] == 0, "scale %d: starts on the root", s);
                if (count >= 4) CHECK(notes[count - 1] == 0, "scale %d count %u: ends on the root", s, count);
                for (uint8_t i = 0; i < count; i++) {
                    CHECK((notes[i] >= -span) && (notes[i] <= span), "scale %d: note %d within one octave", s, notes[i]);
                    if (i > 0 && !((count >= 4) && (i == count - 1))) {
                        const int jump = notes[i] - notes[i - 1];
                        CHECK((jump >= -4) && (jump <= 4), "scale %d: no jump wider than 4 steps (%d)", s, jump);
                    }
                }

                int8_t before[64];
                for (uint8_t i = 0; i < count; i++) before[i] = notes[i];
                const uint8_t changes = mutateMelody(notes, count, s, seed);
                uint8_t changed = 0;
                for (uint8_t i = 0; i < count; i++) {
                    if (notes[i] != before[i]) {
                        changed++;
                        const int d = notes[i] - before[i];
                        CHECK((d >= -2) && (d <= 2), "scale %d: variation moves a note 1-2 steps (%d)", s, d);
                    }
                    CHECK((notes[i] >= -span) && (notes[i] <= span), "scale %d: variation stays within one octave", s);
                }
                CHECK(changed == changes, "scale %d count %u: %u notes changed, %u reported", s, count, changed, changes);
                CHECK(changes == ((count >= 8) ? count / 4 : 1), "scale %d count %u: a quarter of the notes", s, count);
            }
        }
    }

    // Same seed, same melody (the device seeds from the cycle counter for variety).
    int8_t a[16], b[16];
    uint32_t sa = 42, sb = 42;
    generateMelody(a, 16, kDefaultScale, sa);
    generateMelody(b, 16, kDefaultScale, sb);
    bool same = true;
    for (int i = 0; i < 16; i++) same = same && (a[i] == b[i]);
    CHECK(same, "deterministic for a seed");

    // A note outside one octave (from a wider scale) is pulled back in by a variation.
    int8_t wide[1] = {20};
    uint32_t sw = 7;
    mutateMelody(wide, 1, kDefaultScale, sw);
    CHECK((wide[0] >= -7) && (wide[0] <= 7), "wide note brought into range: %d", wide[0]);
    uint32_t zero = 0;
    CHECK(nextRandom(zero) != 0, "a zero seed still produces numbers");
}

static void testKeys() {
    const uint8_t major = scaleIndex("MAJOR");
    const uint8_t chroma = scaleIndex("CHROMATIC");
    for (int p = 0; p < kKeyCount; p++) CHECK(keyDegree(p, 0, major) == p, "pad %d = degree %d", p, p);
    CHECK(keyDegree(0, -1, major) == -7 && keyDegree(0, 1, chroma) == 12, "octave shift is one octave of the scale");
    // Every scale: at every octave the first full octave of pads is inside the range and rising.
    for (int s = 0; s < kScaleCount; s++) {
        for (int o = kMinKeysOctave; o <= kMaxKeysOctave; o++) {
            const int size = kScales[s].size;
            for (int p = 0; p < kKeyCount; p++) {
                const int d = keyDegree(p, o, s);
                CHECK((d >= minDegree(s)) && (d <= maxDegree(s)), "scale %d octave %d pad %d in range", s, o, p);
                if ((p > 0) && (p < size)) CHECK(d == keyDegree(p - 1, o, s) + 1, "scale %d octave %d pad %d rises", s, o, p);
            }
        }
    }
}

int main() {
    testScaleTables();
    testSemitones();
    testRatio();
    testNames();
    testGlide();
    testTranspose();
    testMelody();
    testKeys();
    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
