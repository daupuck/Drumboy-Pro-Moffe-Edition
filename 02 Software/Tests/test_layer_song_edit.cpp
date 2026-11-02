// Host unit tests for the Layer Song edit arithmetic (MOVE, SHIFT ALL, single-beat ADD).
// Tests the same header the firmware uses: Core/Library/Global/LayerSongEdit.h.
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_layer_song_edit.cpp -o test_layer_song_edit && ./test_layer_song_edit
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)
//
// Units: 800 per quarter note; one 4/4 bar = 3200. Steps: 1:16 = 200, 1:8 = 400, 1:4 = 800.

#include <cstdio>
#include <vector>

#include "LayerSongEdit.h"

static int failures = 0;
static int checks = 0;

#define CHECK_EQ(actual, expected)                                                               \
    do {                                                                                         \
        checks++;                                                                                \
        const long long a_ = (long long)(actual), e_ = (long long)(expected);                    \
        if (a_ != e_) {                                                                          \
            failures++;                                                                          \
            std::printf("FAIL %s:%d  %s == %lld, expected %lld\n", __FILE__, __LINE__, #actual, a_, e_); \
        }                                                                                        \
    } while (0)

static std::vector<BeatSnapshot> beatsAt(std::initializer_list<uint16_t> starts) {
    std::vector<BeatSnapshot> v;
    for (uint16_t s : starts) v.push_back(BeatSnapshot{s, 0, 0, 0, 0, 0, false, 0});
    return v;
}

static std::vector<uint16_t> starts(const std::vector<BeatSnapshot>& v, uint8_t count) {
    std::vector<uint16_t> out;
    for (uint8_t i = 0; i < count; i++) out.push_back(v[i].startInterval);
    return out;
}

static void checkStarts(const std::vector<uint16_t>& actual, std::initializer_list<uint16_t> expected, int line) {
    checks++;
    std::vector<uint16_t> e(expected);
    if (actual != e) {
        failures++;
        std::printf("FAIL line %d: got {", line);
        for (auto x : actual) std::printf(" %u", x);
        std::printf(" } expected {");
        for (auto x : e) std::printf(" %u", x);
        std::printf(" }\n");
    }
}
#define CHECK_STARTS(actual, ...) checkStarts((actual), {__VA_ARGS__}, __LINE__)

const uint16_t BAR = 3200, SONG = 4 * BAR, S16 = 200, S8 = 400;

static void testMove() {
    auto b = beatsAt({0, 800, 1600, 2400});
    // Right / left by one 1:16 step.
    CHECK_EQ(layersong::moveTarget(b.data(), 4, 1, true, S16, SONG), 1000);
    CHECK_EQ(layersong::moveTarget(b.data(), 4, 1, false, S16, SONG), 600);
    // Passes a neighbour instead of stopping: 1:4 step from 800 lands on 1600 (taken) -> 2400 (taken) -> 3200.
    CHECK_EQ(layersong::moveTarget(b.data(), 4, 1, true, 800, SONG), 3200);
    // Left from 800 with 1:4 step: 0 is taken -> edge -> no move.
    CHECK_EQ(layersong::moveTarget(b.data(), 4, 1, false, 800, SONG), -1);
    // Stops at the song edges (no wrap).
    auto e = beatsAt({0, SONG - S16});
    CHECK_EQ(layersong::moveTarget(e.data(), 2, 0, false, S16, SONG), -1);
    CHECK_EQ(layersong::moveTarget(e.data(), 2, 1, true, S16, SONG), -1);
    // Off-grid beat (recorded live at 130) snaps to the neighbouring grid line.
    auto o = beatsAt({130});
    CHECK_EQ(layersong::moveTarget(o.data(), 1, 0, true, S16, SONG), 200);
    CHECK_EQ(layersong::moveTarget(o.data(), 1, 0, false, S16, SONG), 0);
    // Invalid input.
    CHECK_EQ(layersong::moveTarget(b.data(), 4, 9, true, S16, SONG), -1);
    CHECK_EQ(layersong::moveTarget(b.data(), 4, 0, true, 0, SONG), -1);
}

static void testShiftAll() {
    auto b = beatsAt({0, 800, 1600, SONG - S16});
    CHECK_EQ(layersong::shiftAll(b.data(), 4, true, S16, SONG), 1);
    CHECK_STARTS(starts(b, 4), 200, 1000, 1800, 0);  // last beat wrapped to the start
    CHECK_EQ(layersong::shiftAll(b.data(), 4, false, S16, SONG), 1);
    CHECK_STARTS(starts(b, 4), 0, 800, 1600, SONG - S16);  // reversible
    // Left from 0 wraps to the end.
    auto c = beatsAt({0});
    layersong::shiftAll(c.data(), 1, false, S8, SONG);
    CHECK_STARTS(starts(c, 1), SONG - S8);
    // Off-grid offsets are preserved.
    auto d = beatsAt({130, 930});
    layersong::shiftAll(d.data(), 2, true, S16, SONG);
    CHECK_STARTS(starts(d, 2), 330, 1130);
    // A full-loop step changes nothing.
    auto f = beatsAt({0, 800});
    CHECK_EQ(layersong::shiftAll(f.data(), 2, true, SONG, SONG), 0);
    // A full rotation returns to the original pattern.
    auto g = beatsAt({0, 600, 2200});
    for (int i = 0; i < SONG / S16; i++) layersong::shiftAll(g.data(), 3, true, S16, SONG);
    CHECK_STARTS(starts(g, 3), 0, 600, 2200);
}

static void testInsert() {
    // Empty layer -> start of the song.
    CHECK_EQ(layersong::insertPosition(nullptr, 0, -1, S16, SONG), 0);
    // Middle of the gap after the selected beat.
    auto b = beatsAt({0, 1600});
    CHECK_EQ(layersong::insertPosition(b.data(), 2, 0, S16, SONG), 800);
    // Middle not on the grid -> nearest grid line inside the gap: gap 0..600, middle 300 -> 400 (1:8) .
    auto c = beatsAt({0, 600});
    CHECK_EQ(layersong::insertPosition(c.data(), 2, 0, S8, SONG), 400);
    // Gap too small for the grid -> no insert.
    auto d = beatsAt({0, 200});
    CHECK_EQ(layersong::insertPosition(d.data(), 2, 0, S16, SONG), -1);
    // After the last beat the gap wraps to the first beat: beats at 800 and 9600,
    // gap 9600..(12800+800) = 4000 long, middle 9600 + 2000 = 11600.
    auto e = beatsAt({800, 9600});
    CHECK_EQ(layersong::insertPosition(e.data(), 2, 1, S16, SONG), 11600);
    // Wrap past the song end: beats at 3200 and 12000 -> gap 12000..16000, middle 14000 -> 1200 after wrap.
    auto f = beatsAt({3200, 12000});
    CHECK_EQ(layersong::insertPosition(f.data(), 2, 1, S16, SONG), 1200);
    // Invalid selection falls back to the first beat.
    CHECK_EQ(layersong::insertPosition(b.data(), 2, 7, S16, SONG), 800);
}

static void testSortUnique() {
    auto b = beatsAt({1600, 0, 800, 800, 2400});
    b[0].fillType = 3;  // follow a beat with data through the sort
    int8_t tracked = 0;  // the beat at 1600
    uint8_t n = layersong::sortUnique(b.data(), 5, tracked);
    CHECK_EQ(n, 4);
    CHECK_STARTS(starts(b, n), 0, 800, 1600, 2400);
    CHECK_EQ(tracked, 2);
    CHECK_EQ(b[tracked].fillType, 3);

    // When the tracked beat collides with another, the tracked one wins.
    auto c = beatsAt({800, 0, 800});
    c[2].fillPattern = 5;
    int8_t t2 = 2;
    n = layersong::sortUnique(c.data(), 3, t2);
    CHECK_EQ(n, 2);
    CHECK_EQ(t2, 1);
    CHECK_EQ(c[1].fillPattern, 5);

    // Move simulation: beat 1 jumps past beat 2; selection follows it.
    auto d = beatsAt({0, 800, 1600});
    int32_t target = layersong::moveTarget(d.data(), 3, 1, true, 800, SONG);
    CHECK_EQ(target, 2400);
    d[1].startInterval = (uint16_t)target;
    int8_t sel = 1;
    n = layersong::sortUnique(d.data(), 3, sel);
    CHECK_STARTS(starts(d, n), 0, 1600, 2400);
    CHECK_EQ(sel, 2);

    // Invalid tracked index stays -1.
    auto e = beatsAt({0, 800});
    int8_t t3 = -1;
    layersong::sortUnique(e.data(), 2, t3);
    CHECK_EQ(t3, -1);
}

int main() {
    testMove();
    testShiftAll();
    testInsert();
    testSortUnique();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
