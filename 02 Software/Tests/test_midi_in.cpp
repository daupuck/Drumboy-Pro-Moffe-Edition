// Host tests for Global/MidiNoteIn.h (MIDI IN note parser and queue).
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_midi_in.cpp -o test_midi_in && ./test_midi_in
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cstdio>
#include <vector>

#include "MidiNoteIn.h"

static int checks = 0;
static int failures = 0;
#define CHECK(c)                                             \
    do {                                                     \
        checks++;                                            \
        if (!(c)) {                                          \
            failures++;                                      \
            std::printf("FAIL line %d: %s\n", __LINE__, #c); \
        }                                                    \
    } while (0)

static std::vector<midiin::Event> run(const std::vector<uint8_t>& bytes, uint8_t channel) {
    midiin::Parser p;
    std::vector<midiin::Event> out;
    for (uint8_t b : bytes) {
        midiin::Event e;
        if (p.feed(b, channel, e)) out.push_back(e);
    }
    return out;
}

static bool is(const midiin::Event& e, uint8_t on, uint8_t key, uint8_t vel) { return (e.on == on) && (e.key == key) && (e.velocity == vel); }

static void testParser() {
    // Plain on / off.
    auto ev = run({0x90, 60, 100, 0x80, 60, 64}, 0);
    CHECK(ev.size() == 2 && is(ev[0], 1, 60, 100) && is(ev[1], 0, 60, 64));
    // Running status, velocity 0 = off.
    ev = run({0x90, 60, 100, 64, 90, 60, 0, 64, 0}, 0);
    CHECK(ev.size() == 4 && is(ev[0], 1, 60, 100) && is(ev[1], 1, 64, 90) && is(ev[2], 0, 60, 0) && is(ev[3], 0, 64, 0));
    // Clock bytes in the middle of a message change nothing.
    ev = run({0x90, 0xF8, 62, 0xF8, 80, 0xFE}, 0);
    CHECK(ev.size() == 1 && is(ev[0], 1, 62, 80));
    // Other channels and other messages are ignored, and cancel running status.
    ev = run({0x91, 60, 100, 0x90, 61, 100, 0xB0, 7, 100, 62, 100, 0x90, 63, 1}, 0);
    CHECK(ev.size() == 2 && is(ev[0], 1, 61, 100) && is(ev[1], 1, 63, 1));
    ev = run({0x9F, 60, 100, 0x90, 61, 100}, 15);
    CHECK(ev.size() == 1 && is(ev[0], 1, 60, 100));
    // A new status in the middle of a message restarts cleanly.
    ev = run({0x90, 60, 0x80, 60, 10}, 0);
    CHECK(ev.size() == 1 && is(ev[0], 0, 60, 10));
    // SysEx data is not taken for notes.
    ev = run({0x90, 60, 0xF0, 0x7D, 0x01, 0xF7, 61, 100}, 0);
    CHECK(ev.empty());
}

static void testQueue() {
    midiin::Queue q;
    midiin::Event e;
    CHECK(!q.pop(e));
    int pushed = 0;
    for (int i = 0; i < 40; i++) {
        midiin::Event in;
        in.key = static_cast<uint8_t>(i);
        if (q.push(in)) pushed++;
    }
    CHECK(pushed == midiin::Queue::kSize - 1);
    for (int i = 0; i < pushed; i++) CHECK(q.pop(e) && e.key == i);
    CHECK(!q.pop(e));
    // Wraps around many times.
    for (int i = 0; i < 1000; i++) {
        midiin::Event in;
        in.key = static_cast<uint8_t>(i & 0x7F);
        CHECK(q.push(in));
        CHECK(q.pop(e) && e.key == (i & 0x7F));
    }
    midiin::Event x;
    q.push(x);
    q.clear();
    CHECK(!q.pop(e));
}

int main() {
    testParser();
    testQueue();
    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
