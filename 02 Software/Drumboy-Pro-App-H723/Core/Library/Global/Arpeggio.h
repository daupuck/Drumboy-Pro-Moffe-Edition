#ifndef __ARPEGGIO_H
#define __ARPEGGIO_H

// ARPEGGIATOR (v2.08): every note of a TONE / SYNTH layer can become a fast arpeggio of a chord
// in the layer's scale - UP, DOWN, UP/DOWN or RANDOM, at 1/8 .. 1/32 (with triplets), over 1..3
// octaves, for as long as the note lasts (a beat until the next beat, a held pad or MIDI key until
// it is let go). Pure C++ with no hardware access, so it is host-tested
// (02 Software/Tests/test_arpeggio.cpp). The ticking is in ControllerLayer.cpp (arp_tick).
//
// Stored per layer in the project's .rwn note file as the "ARP1" section:
//   10 x [mode, rate, octaves, chord, gate] + "EOF". Older firmware plays plain notes.

#include <stdint.h>

namespace arp {

enum Mode : uint8_t { OFF = 0, UP, DOWN, UPDOWN, RANDOM, kModeCount };
constexpr const char* kModeNames[kModeCount] = {"OFF", "UP", "DOWN", "UP/DOWN", "RANDOM"};

// RATE: one arp step in sequencer intervals (800 per quarter note).
constexpr uint8_t kRateCount = 5;
constexpr uint16_t kRateIntervals[kRateCount] = {400, 267, 200, 133, 100};
constexpr const char* kRateNames[kRateCount] = {"1/8", "1/8T", "1/16", "1/16T", "1/32"};
constexpr uint8_t kDefaultRate = 2;  // 1/16

// CHORD: the notes, in scale steps above the played note.
enum Chord : uint8_t { TRIAD = 0, SEVENTH, POWER, SUS4, OCTAVE, kChordCount };
constexpr const char* kChordNames[kChordCount] = {"TRIAD", "7TH", "POWER", "SUS4", "OCTAVE"};
constexpr uint8_t kChordSize[kChordCount] = {3, 4, 2, 3, 1};
constexpr int8_t kChordSteps[kChordCount][4] = {{0, 2, 4, 0}, {0, 2, 4, 6}, {0, 4, 0, 0}, {0, 3, 4, 0}, {0, 0, 0, 0}};

constexpr uint8_t kMaxOctaves = 3;
constexpr uint8_t kMinGate = 10, kMaxGate = 100, kDefaultGate = 60;

struct Settings {
    uint8_t mode = OFF;
    uint8_t rate = kDefaultRate;
    uint8_t octaves = 1;
    uint8_t chord = TRIAD;
    uint8_t gate = kDefaultGate;  // % of a step
};

inline Settings sanitize(const Settings& in) {
    Settings s = in;
    if (s.mode >= kModeCount) s.mode = OFF;
    if (s.rate >= kRateCount) s.rate = kDefaultRate;
    if ((s.octaves < 1) || (s.octaves > kMaxOctaves)) s.octaves = 1;
    if (s.chord >= kChordCount) s.chord = TRIAD;
    if ((s.gate < kMinGate) || (s.gate > kMaxGate)) s.gate = kDefaultGate;
    return s;
}

inline bool isDefault(const Settings& s) {
    const Settings d;
    return (s.mode == d.mode) && (s.rate == d.rate) && (s.octaves == d.octaves) && (s.chord == d.chord) && (s.gate == d.gate);
}

// File form ("ARP1" section of the .rwn note file): 5 bytes per layer, sanitized when read.
constexpr uint8_t kPackedSize = 5;
inline void pack(const Settings& in, uint8_t* out) {
    const Settings s = sanitize(in);
    out[0] = s.mode;
    out[1] = s.rate;
    out[2] = s.octaves;
    out[3] = s.chord;
    out[4] = s.gate;
}
inline Settings unpack(const uint8_t* in) {
    Settings s;
    s.mode = in[0];
    s.rate = in[1];
    s.octaves = in[2];
    s.chord = in[3];
    s.gate = in[4];
    return sanitize(s);
}

// Notes in one ascending run: chord size x octaves.
inline uint8_t runLength(const Settings& s) { return static_cast<uint8_t>(kChordSize[sanitize(s).chord] * sanitize(s).octaves); }

// The i-th note of the ascending run, in scale steps above the played note (`octave` = the
// scale's size, so octaves stay in key).
inline int noteOfRun(const Settings& raw, uint8_t i, uint8_t octave) {
    const Settings s = sanitize(raw);
    const uint8_t size = kChordSize[s.chord];
    const uint8_t n = runLength(s);
    i = static_cast<uint8_t>(i % n);
    return kChordSteps[s.chord][i % size] + ((i / size) * octave);
}

// Steps in one full cycle of the pattern (UP/DOWN does not repeat its top and bottom note).
inline uint8_t cycleLength(const Settings& raw) {
    const Settings s = sanitize(raw);
    const uint8_t n = runLength(s);
    if ((s.mode == UPDOWN) && (n > 2)) return static_cast<uint8_t>((2 * n) - 2);
    return n;
}

// The step-th note of the pattern (scale steps above the played note). RANDOM takes a random
// number from the caller.
inline int noteAt(const Settings& raw, uint32_t step, uint8_t octave, uint32_t random = 0) {
    const Settings s = sanitize(raw);
    const uint8_t n = runLength(s);
    if (n == 0) return 0;
    switch (s.mode) {
        case DOWN:
            return noteOfRun(s, static_cast<uint8_t>((n - 1) - (step % n)), octave);
        case UPDOWN: {
            const uint8_t c = cycleLength(s);
            const uint8_t k = static_cast<uint8_t>(step % c);
            return noteOfRun(s, (k < n) ? k : static_cast<uint8_t>(c - k), octave);
        }
        case RANDOM:
            return noteOfRun(s, static_cast<uint8_t>(random % n), octave);
        case UP:
        default:
            return noteOfRun(s, static_cast<uint8_t>(step % n), octave);
    }
}

// Length of one arp note in intervals (at least 1).
inline uint16_t noteLength(const Settings& raw) {
    const Settings s = sanitize(raw);
    const uint32_t len = (static_cast<uint32_t>(kRateIntervals[s.rate]) * s.gate) / 100u;
    return static_cast<uint16_t>((len > 0) ? len : 1);
}

}  // namespace arp

#endif
