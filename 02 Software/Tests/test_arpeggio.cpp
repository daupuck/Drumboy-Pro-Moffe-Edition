// Host tests for Global/Arpeggio.h (arpeggiator patterns) and tone::degreeForSemitone (MIDI in).
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_arpeggio.cpp -o test_arpeggio && ./test_arpeggio
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cstdio>
#include <cstring>
#include <set>

#include "Arpeggio.h"
#include "ToneScale.h"

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

static arp::Settings make(uint8_t mode, uint8_t chord, uint8_t octaves) {
    arp::Settings s;
    s.mode = mode;
    s.chord = chord;
    s.octaves = octaves;
    return s;
}

static void testPatterns() {
    // UP triad over 2 octaves in a 7-note scale: 0 2 4 7 9 11, then again.
    const arp::Settings up = make(arp::UP, arp::TRIAD, 2);
    const int want[6] = {0, 2, 4, 7, 9, 11};
    for (int i = 0; i < 12; i++) CHECK(arp::noteAt(up, i, 7) == want[i % 6]);
    CHECK(arp::cycleLength(up) == 6);
    // DOWN is UP backwards.
    const arp::Settings down = make(arp::DOWN, arp::TRIAD, 2);
    for (int i = 0; i < 6; i++) CHECK(arp::noteAt(down, i, 7) == want[5 - i]);
    // UP/DOWN: 0 2 4 7 9 11 9 7 4 2 | 0 ... (no repeated ends).
    const arp::Settings ud = make(arp::UPDOWN, arp::TRIAD, 2);
    const int wantUd[10] = {0, 2, 4, 7, 9, 11, 9, 7, 4, 2};
    CHECK(arp::cycleLength(ud) == 10);
    for (int i = 0; i < 20; i++) CHECK(arp::noteAt(ud, i, 7) == wantUd[i % 10]);
    // Chords.
    CHECK(arp::noteAt(make(arp::UP, arp::SEVENTH, 1), 3, 7) == 6);
    CHECK(arp::noteAt(make(arp::UP, arp::POWER, 1), 1, 7) == 4);
    CHECK(arp::noteAt(make(arp::UP, arp::SUS4, 1), 1, 7) == 3);
    CHECK(arp::noteAt(make(arp::UP, arp::OCTAVE, 3), 2, 7) == 14);
    // Octaves follow the scale size (pentatonic: 5).
    CHECK(arp::noteAt(make(arp::UP, arp::TRIAD, 2), 3, 5) == 5);
    // Small runs: UP/DOWN with 1 or 2 notes just alternates / repeats.
    CHECK(arp::cycleLength(make(arp::UPDOWN, arp::OCTAVE, 1)) == 1);
    CHECK(arp::cycleLength(make(arp::UPDOWN, arp::POWER, 1)) == 2);
    // RANDOM always lands on a chord note.
    const arp::Settings rnd = make(arp::RANDOM, arp::TRIAD, 3);
    std::set<int> seen;
    for (uint32_t r = 0; r < 500; r++) {
        const int n = arp::noteAt(rnd, 0, 7, r * 2654435761u);
        seen.insert(n);
        bool ok = false;
        for (int i = 0; i < 9; i++) ok = ok || (n == arp::noteOfRun(rnd, static_cast<uint8_t>(i), 7));
        CHECK(ok);
    }
    CHECK(seen.size() == 9);
}

static void testSettings() {
    arp::Settings junk;
    junk.mode = 99;
    junk.rate = 99;
    junk.octaves = 0;
    junk.chord = 77;
    junk.gate = 5;
    const arp::Settings s = arp::sanitize(junk);
    CHECK(s.mode == arp::OFF && s.rate == arp::kDefaultRate && s.octaves == 1 && s.chord == arp::TRIAD && s.gate == arp::kDefaultGate);
    // Note lengths: gate share of the step, never 0.
    arp::Settings g;
    g.rate = 2;  // 1/16 = 200
    g.gate = 50;
    CHECK(arp::noteLength(g) == 100);
    g.rate = 4;
    g.gate = 10;
    CHECK(arp::noteLength(g) == 10);
    for (uint8_t r = 0; r < arp::kRateCount; r++) CHECK(arp::kRateIntervals[r] > 0 && std::strlen(arp::kRateNames[r]) <= 10);
    for (uint8_t m = 0; m < arp::kModeCount; m++) CHECK(std::strlen(arp::kModeNames[m]) <= 10);
    for (uint8_t c = 0; c < arp::kChordCount; c++) CHECK(std::strlen(arp::kChordNames[c]) <= 10);

    // File form: round trip, and damaged bytes come back safe.
    CHECK(arp::isDefault(arp::Settings{}));
    arp::Settings a;
    a.mode = arp::UPDOWN;
    a.rate = 4;
    a.octaves = 3;
    a.chord = arp::SUS4;
    a.gate = 85;
    CHECK(!arp::isDefault(a));
    uint8_t bytes[arp::kPackedSize];
    arp::pack(a, bytes);
    const arp::Settings b = arp::unpack(bytes);
    CHECK(b.mode == a.mode && b.rate == a.rate && b.octaves == a.octaves && b.chord == a.chord && b.gate == a.gate);
    const uint8_t bad[arp::kPackedSize] = {200, 200, 9, 200, 255};
    CHECK(arp::isDefault(arp::unpack(bad)));
}

static void testMidiToDegree() {
    // Every scale note is found exactly, in every scale and key, over the whole range.
    for (uint8_t sc = 0; sc < tone::kScaleCount; sc++) {
        for (uint8_t root = 0; root < 12; root++) {
            for (int d = tone::minDegree(sc); d <= tone::maxDegree(sc); d++) {
                const int semis = tone::semitones(d, sc, root);
                CHECK(tone::semitones(tone::degreeForSemitone(semis, sc, root), sc, root) == semis);
            }
            // Any semitone in the middle range snaps to a scale note at most 2 semitones away.
            for (int target = -24; target <= 24; target++) {
                const int got = tone::semitones(tone::degreeForSemitone(target, sc, root), sc, root);
                const int diff = (got > target) ? (got - target) : (target - got);
                CHECK(diff <= 2);
            }
        }
    }
    // C major: C# (1) snaps to C (0), the lower one on a tie.
    int major = -1;
    for (uint8_t i = 0; i < tone::kScaleCount; i++) {
        if (std::strcmp(tone::kScales[i].name, "MAJOR") == 0) major = i;
    }
    CHECK(major >= 0);
    if (major >= 0) {
        CHECK(tone::degreeForSemitone(0, static_cast<uint8_t>(major), 0) == 0);
        CHECK(tone::degreeForSemitone(1, static_cast<uint8_t>(major), 0) == 0);
        CHECK(tone::degreeForSemitone(12, static_cast<uint8_t>(major), 0) == 7);
        CHECK(tone::degreeForSemitone(-1, static_cast<uint8_t>(major), 0) == -1);  // B below
    }
}

int main() {
    testPatterns();
    testSettings();
    testMidiToDegree();
    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
