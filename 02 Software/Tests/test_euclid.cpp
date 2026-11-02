// Host unit tests for the Euclidean rhythm generator: Core/Library/Global/EuclidPattern.h.
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_euclid.cpp -o test_euclid && ./test_euclid
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cstdio>
#include <string>

#include "EuclidPattern.h"

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

static std::string str(uint8_t hits, uint8_t steps, uint8_t rot = 0) {
    bool p[euclid::kMaxSteps] = {};
    euclid::pattern(hits, steps, p);
    euclid::rotate(p, steps, rot);
    std::string s;
    for (uint8_t i = 0; i < steps; i++) s += p[i] ? 'x' : '.';
    return s;
}

static void testKnownPatterns() {
    struct Case { uint8_t k, n; const char* want; };
    // From Toussaint's paper (rotations as produced by Bjorklund's algorithm).
    const Case cases[] = {
        {3, 8, "x..x..x."},   // tresillo
        {5, 8, "x.xx.xx."},   // cinquillo
        {2, 5, "x.x.."},
        {3, 7, "x.x.x.."},
        {4, 9, "x.x.x.x.."},
        {5, 12, "x..x.x..x.x."},
        {1, 4, "x..."},
        {4, 4, "xxxx"},
        {0, 6, "......"},
        {4, 16, "x...x...x...x..."},
    };
    for (const Case& c : cases) {
        const std::string got = str(c.k, c.n);
        CHECK(got == c.want, "E(%u,%u) = %s, expected %s", c.k, c.n, got.c_str(), c.want);
    }
}

// For every steps 1..32 and hits 0..steps: exactly `hits` onsets, and the cyclic gaps between
// onsets differ by at most one step (maximally even).
static void testEvenness() {
    for (int n = 1; n <= euclid::kMaxSteps; n++) {
        for (int k = 0; k <= n; k++) {
            bool p[euclid::kMaxSteps] = {};
            euclid::pattern((uint8_t)k, (uint8_t)n, p);
            int count = 0, first = -1, prev = -1, minGap = 1000, maxGap = 0;
            for (int i = 0; i < n; i++) {
                if (!p[i]) continue;
                count++;
                if (first < 0) first = i;
                if (prev >= 0) {
                    const int gap = i - prev;
                    if (gap < minGap) minGap = gap;
                    if (gap > maxGap) maxGap = gap;
                }
                prev = i;
            }
            CHECK(count == k, "E(%d,%d) has %d hits", k, n, count);
            if (k >= 2) {
                const int wrap = first + n - prev;
                if (wrap < minGap) minGap = wrap;
                if (wrap > maxGap) maxGap = wrap;
                CHECK(maxGap - minGap <= 1, "E(%d,%d) gaps %d..%d not even", k, n, minGap, maxGap);
            }
            if (k > 0) CHECK(p[0], "E(%d,%d) does not start with a hit", k, n);
        }
    }
}

static void testRotate() {
    CHECK(str(3, 8, 1) == ".x..x..x", "rotate 1: %s", str(3, 8, 1).c_str());
    CHECK(str(3, 8, 8) == "x..x..x.", "rotate full cycle: %s", str(3, 8, 8).c_str());
    CHECK(str(3, 8, 10) == str(3, 8, 2), "rotate wraps");
}

static void testPositions() {
    bool p[euclid::kMaxSteps] = {};
    euclid::pattern(3, 8, p);  // x..x..x.
    uint16_t pos[64];
    bool trunc = false;
    // One 4/4 bar (3200) of 1:8 steps (400): exactly one cycle.
    uint8_t n = euclid::positions(p, 8, 400, 3200, true, pos, 64, trunc);
    CHECK(n == 3 && pos[0] == 0 && pos[1] == 1200 && pos[2] == 2400 && !trunc, "one bar: n=%u", n);
    // Four bars, repeat: 12 hits, the second cycle starts at 3200.
    n = euclid::positions(p, 8, 400, 12800, true, pos, 64, trunc);
    CHECK(n == 12 && pos[3] == 3200 && pos[11] == 12800 - 800, "four bars: n=%u last=%u", n, pos[n - 1]);
    // Once: only the first cycle.
    n = euclid::positions(p, 8, 400, 12800, false, pos, 64, trunc);
    CHECK(n == 3, "once: n=%u", n);
    // Polymeter: 7 steps of 1:16 (200) -> cycle 1400, cut at the song end.
    bool q[euclid::kMaxSteps] = {};
    euclid::pattern(2, 7, q);  // x.x....
    n = euclid::positions(q, 7, 200, 3200, true, pos, 64, trunc);
    CHECK(n == 5 && pos[2] == 1400 && pos[4] == 2800, "polymeter: n=%u", n);
    // Cap at 64.
    bool all[euclid::kMaxSteps] = {};
    euclid::pattern(16, 16, all);
    n = euclid::positions(all, 16, 200, 8 * 3200, true, pos, 64, trunc);
    CHECK(n == 64 && trunc, "cap: n=%u trunc=%d", n, trunc);
    // Song shorter than one cycle.
    n = euclid::positions(p, 8, 800, 3200, true, pos, 64, trunc);  // cycle 6400 > 3200
    CHECK(n == 2 && pos[1] == 2400, "short song: n=%u", n);
}

static void testCompose() {
    const BeatSnapshot base[] = {{0, 2, 1, 0, 0, 0, false, 0}, {800, 0, 0, 0, 0, 0, false, 0}};
    const uint16_t hits[] = {0, 400, 1200};
    BeatSnapshot out[64];
    bool trunc = false;
    // REPLACE: only the hits, plain.
    uint8_t n = euclid::compose(base, 2, hits, 3, false, out, 64, trunc);
    CHECK(n == 3 && out[0].fillType == 0 && !trunc, "replace n=%u", n);
    // MERGE: base kept (with its fill), colliding hit at 0 not duplicated.
    n = euclid::compose(base, 2, hits, 3, true, out, 64, trunc);
    CHECK(n == 4, "merge n=%u", n);
    CHECK(out[0].startInterval == 0 && out[0].fillType == 2, "merge keeps base fill");
    // MERGE respects the cap.
    n = euclid::compose(base, 2, hits, 3, true, out, 3, trunc);
    CHECK(n == 3 && trunc, "merge cap n=%u trunc=%d", n, trunc);
}

int main() {
    testKnownPatterns();
    testEvenness();
    testRotate();
    testPositions();
    testCompose();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
