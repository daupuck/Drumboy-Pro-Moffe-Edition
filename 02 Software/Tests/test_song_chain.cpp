// Host unit tests for song mode: Core/Library/Global/SongChain.h.
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_song_chain.cpp -o test_song_chain && ./test_song_chain
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cstdio>
#include <string>

#include "SongChain.h"

using namespace songchain;

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

// Plays the chain from the start for `loops` loop ends and returns the part of every loop
// ("1".."5"), with "|" where the sequencer would stop.
static std::string play(const Chain& c, int loops) {
    Position pos;
    uint8_t part = 99;
    std::string s;
    if (!start(c, pos, part)) return "empty";
    s += char('1' + part);
    for (int i = 1; i < loops; i++) {
        const Action a = advance(c, pos, part);
        if (a == STOP) {
            s += '|';
            if (!start(c, pos, part)) break;
        }
        s += char('1' + part);
    }
    return s;
}

static Chain make(Mode mode, std::initializer_list<Step> steps) {
    Chain c;
    reset(c);
    c.mode = mode;
    uint8_t i = 0;
    for (const Step& s : steps) c.steps[i++] = s;
    if (i < kMaxSteps) c.steps[i] = Step{kPartEnd, 1};
    return c;
}

static void testLoop() {
    // intro x2, verse x4, fill x1, chorus x2 -> then back to the start
    const Chain c = make(LOOP, {{0, 2}, {1, 4}, {2, 1}, {3, 2}});
    CHECK(length(c) == 4, "length %u", length(c));
    const std::string got = play(c, 12);
    CHECK(got == "112222344112", "loop: %s", got.c_str());
}

static void testOnce() {
    const Chain c = make(ONCE, {{0, 1}, {1, 2}});
    const std::string got = play(c, 7);
    CHECK(got == "122|122|1", "once: %s", got.c_str());

    // The STOP comes exactly at the end of the last loop of the last step.
    Position pos;
    uint8_t part = 0;
    start(c, pos, part);
    CHECK(advance(c, pos, part) == SWITCH && part == 1, "to step 2");
    CHECK(advance(c, pos, part) == STAY && part == 1, "repeat step 2");
    CHECK(advance(c, pos, part) == STOP, "stop after last repeat");
    CHECK(pos.step == 0 && pos.loop == 0, "position back at start");
}

static void testSwitchToSamePart() {
    // Consecutive steps with the same part still advance through the chain.
    const Chain c = make(LOOP, {{2, 1}, {2, 1}, {4, 1}});
    CHECK(play(c, 6) == "335335", "same part: %s", play(c, 6).c_str());
}

static void testEmptyAndEdgeCases() {
    Chain c = make(LOOP, {});
    c.steps[0] = Step{kPartEnd, 1};
    CHECK(length(c) == 0, "empty length");
    Position pos;
    uint8_t part = 3;
    CHECK(!start(c, pos, part) && part == 3, "empty start");
    CHECK(advance(c, pos, part) == STAY && part == 3, "empty advance changes nothing");

    // Full 16 steps.
    Chain f = make(LOOP, {});
    for (uint8_t i = 0; i < kMaxSteps; i++) f.steps[i] = Step{static_cast<uint8_t>(i % kParts), 1};
    CHECK(length(f) == 16, "full length %u", length(f));
    CHECK(play(f, 17) == "12345123451234511", "full: %s", play(f, 17).c_str());

    // Chain shortened while playing: position beyond the end restarts safely.
    Chain s = make(LOOP, {{0, 1}, {1, 1}, {2, 1}});
    Position p;
    start(s, p, part);
    advance(s, p, part);
    advance(s, p, part);  // now at step 3
    s.steps[1] = Step{kPartEnd, 1};  // length becomes 1
    CHECK(advance(s, p, part) == SWITCH && part == 0, "shortened chain restarts at step 1");

    // Repeats of 0 behave like 1.
    Chain z = make(LOOP, {{0, 0}, {1, 1}});
    CHECK(play(z, 4) == "1212", "zero repeats: %s", play(z, 4).c_str());
}

static void testEncodeDecode() {
    Chain c = make(ONCE, {{0, 2}, {4, 16}, {1, 1}});
    uint8_t bytes[kEncodedSize];
    encode(c, bytes);
    CHECK(bytes[0] == 0xA2, "header %02X", bytes[0]);
    CHECK(bytes[1] == 0x01 && bytes[2] == 0x4F && bytes[3] == 0x10 && bytes[4] == 0x70, "steps %02X %02X %02X %02X",
          bytes[1], bytes[2], bytes[3], bytes[4]);
    Chain d;
    CHECK(decode(bytes, d), "decode ok");
    CHECK(d.mode == ONCE && length(d) == 3 && d.steps[1].part == 4 && d.steps[1].repeats == 16, "round trip");

    // Files without song mode have zeros here -> default chain, OFF.
    uint8_t zeros[kEncodedSize] = {};
    Chain e;
    CHECK(!decode(zeros, e) && e.mode == OFF && length(e) == 1, "zeros -> default");
    // Bad mode -> default.
    uint8_t bad[kEncodedSize] = {0xA7};
    CHECK(!decode(bad, e) && e.mode == OFF, "bad mode -> default");
}

int main() {
    testLoop();
    testOnce();
    testSwitchToSamePart();
    testEmptyAndEdgeCases();
    testEncodeDecode();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
