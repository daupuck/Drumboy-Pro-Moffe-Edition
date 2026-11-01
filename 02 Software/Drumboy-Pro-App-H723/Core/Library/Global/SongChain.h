#ifndef __SONG_CHAIN_H
#define __SONG_CHAIN_H

// Song mode: a chain of up to 16 steps, each "play part (song bank) N for R loops". Pure logic
// with no hardware dependencies; host-tested in 02 Software/Tests/test_song_chain.cpp.
//
// The sequencer calls advance() at the end of every loop; it says whether to keep the current
// part, switch to another one, or stop (ONCE mode after the last step).
//
// Stored in the project file (.rwf) inside the 1500-byte drumkit-style block, in bytes that no
// other code reads or writes (see kSongChainHeaderOffset / kSongChainStepsOffset in Global.h):
//   header : 0xA0 | mode   (high nibble = marker; anything else -> default chain, mode OFF)
//   step i : (part << 4) | (repeats - 1)      part 0..4 = bank 1..5, 7 = END

#include <stdint.h>

namespace songchain {

constexpr uint8_t kMaxSteps = 16;
constexpr uint8_t kParts = 5;        // song banks
constexpr uint8_t kPartEnd = 7;      // marks the end of the chain
constexpr uint8_t kMaxRepeats = 16;
constexpr uint8_t kHeaderMarker = 0xA0;
constexpr uint8_t kEncodedSize = 1 + kMaxSteps;

enum Mode : uint8_t { OFF = 0, LOOP = 1, ONCE = 2 };
enum Action : uint8_t { STAY = 0, SWITCH = 1, STOP = 2 };

struct Step {
    uint8_t part;     // 0..4, or kPartEnd
    uint8_t repeats;  // 1..16
};

struct Chain {
    uint8_t mode = OFF;
    Step steps[kMaxSteps];
};

struct Position {
    uint8_t step = 0;  // index of the step being played
    uint8_t loop = 0;  // loops already completed in this step
};

// Default: OFF, step 1 = part 1 once, the rest END.
inline void reset(Chain& c) {
    c.mode = OFF;
    for (uint8_t i = 0; i < kMaxSteps; i++) c.steps[i] = Step{kPartEnd, 1};
    c.steps[0] = Step{0, 1};
}

inline Chain defaultChain() {
    Chain c;
    reset(c);
    return c;
}

// Number of playable steps (up to the first END).
inline uint8_t length(const Chain& c) {
    uint8_t n = 0;
    while ((n < kMaxSteps) && (c.steps[n].part < kParts)) n++;
    return n;
}

// Start position; returns false (and leaves `part`) if the chain is empty.
inline bool start(const Chain& c, Position& pos, uint8_t& part) {
    pos = Position{};
    if (length(c) == 0) return false;
    part = c.steps[0].part;
    return true;
}

// Call once at the end of every loop. Updates `pos` and `part` (the part for the next loop).
inline Action advance(const Chain& c, Position& pos, uint8_t& part) {
    const uint8_t len = length(c);
    if (len == 0) return STAY;
    if (pos.step >= len) pos = Position{};  // the chain was shortened while playing

    const uint8_t repeats = (c.steps[pos.step].repeats == 0) ? 1 : c.steps[pos.step].repeats;
    pos.loop++;
    if (pos.loop < repeats) {
        part = c.steps[pos.step].part;
        return STAY;
    }

    pos.loop = 0;
    pos.step++;
    if (pos.step >= len) {
        if (c.mode == ONCE) {
            pos = Position{};
            return STOP;
        }
        pos.step = 0;  // LOOP (and OFF, which never runs)
    }
    part = c.steps[pos.step].part;
    return SWITCH;
}

inline void encode(const Chain& c, uint8_t* out) {
    out[0] = static_cast<uint8_t>(kHeaderMarker | (c.mode & 0x0F));
    for (uint8_t i = 0; i < kMaxSteps; i++) {
        const uint8_t part = (c.steps[i].part < kParts) ? c.steps[i].part : kPartEnd;
        uint8_t repeats = c.steps[i].repeats;
        if (repeats < 1) repeats = 1;
        if (repeats > kMaxRepeats) repeats = kMaxRepeats;
        out[1 + i] = static_cast<uint8_t>((part << 4) | (repeats - 1));
    }
}

// Returns false (and a default chain) when the bytes do not hold a valid chain, e.g. files
// saved by firmware without song mode, where these bytes are zero.
inline bool decode(const uint8_t* in, Chain& c) {
    reset(c);
    if ((in[0] & 0xF0) != kHeaderMarker) return false;
    const uint8_t mode = in[0] & 0x0F;
    if (mode > ONCE) return false;
    for (uint8_t i = 0; i < kMaxSteps; i++) {
        const uint8_t part = in[1 + i] >> 4;
        c.steps[i] = Step{(part < kParts) ? part : kPartEnd, static_cast<uint8_t>((in[1 + i] & 0x0F) + 1)};
    }
    c.mode = mode;
    return true;
}

}  // namespace songchain

#endif
