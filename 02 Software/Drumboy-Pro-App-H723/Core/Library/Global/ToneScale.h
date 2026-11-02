#ifndef __TONE_SCALE_H
#define __TONE_SCALE_H

// TONE layers: pure pitch arithmetic for melodic sample playback (Layer Song NOTE page).
// No hardware, HAL or Controller dependencies, so the exact same code runs on the device and
// in the host unit test (02 Software/Tests/test_tone_scale.cpp).
//
// A beat's note is stored as a SCALE DEGREE relative to the layer's root (0 = root,
// 1 = next note of the scale, -1 = the one below, ...). Changing the scale or root later
// therefore re-harmonises the melody instead of leaving notes out of key.
//
// The sample is assumed to be tuned to C at the layer's PITCH setting, shown as C4 / MIDI 60.
// The root moves the whole layer by at most a tritone (C..F# up, G..B down), so a melody in
// any key stays close to the sample's natural pitch.

#include <math.h>
#include <stdint.h>
#include <stdio.h>

namespace tone {

constexpr uint8_t kModeDrum = 0;
constexpr uint8_t kModeTone = 1;   // the sample, played at each beat's note
constexpr uint8_t kModeSynth = 2;  // the built-in synth voice (SynthVoice.h), played at each beat's note
constexpr uint8_t kModeCount = 3;

// TONE and SYNTH layers both play notes (scale, glide, transpose, pad keyboard, generator).
inline bool playsNotes(uint8_t mode) { return (mode == kModeTone) || (mode == kModeSynth); }

constexpr uint8_t kRootCount = 12;

struct Scale {
    const char* name;
    uint8_t size;       // notes per octave
    uint8_t steps[12];  // semitones above the root
};

constexpr uint8_t kScaleCount = 11;
constexpr uint8_t kDefaultScale = 2;  // MINOR

inline constexpr Scale kScales[kScaleCount] = {
    {"CHROMATIC", 12, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
    {"MAJOR", 7, {0, 2, 4, 5, 7, 9, 11}},
    {"MINOR", 7, {0, 2, 3, 5, 7, 8, 10}},
    {"DORIAN", 7, {0, 2, 3, 5, 7, 9, 10}},
    {"PHRYGIAN", 7, {0, 1, 3, 5, 7, 8, 10}},
    {"LYDIAN", 7, {0, 2, 4, 6, 7, 9, 11}},
    {"MIXOLYDIAN", 7, {0, 2, 4, 5, 7, 9, 10}},
    {"HARM MINOR", 7, {0, 2, 3, 5, 7, 8, 11}},
    {"PENTA MAJ", 5, {0, 2, 4, 7, 9}},
    {"PENTA MIN", 5, {0, 3, 5, 7, 10}},
    {"BLUES", 6, {0, 3, 5, 6, 7, 10}},
};

inline constexpr const char* kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Playback speed limits after note, root and the layer's PITCH are combined (1/16x .. 8x).
constexpr float kMinIncrement = 0.0625f;
constexpr float kMaxIncrement = 8.0f;

constexpr uint8_t kMaxGlide = 100;

inline uint8_t safeScale(uint8_t scale) { return (scale < kScaleCount) ? scale : kDefaultScale; }

// Two octaves of the scale either way: +-24 (chromatic), +-14 (7-note), +-10 (pentatonic).
inline int8_t maxDegree(uint8_t scale) { return static_cast<int8_t>(2 * kScales[safeScale(scale)].size); }
inline int8_t minDegree(uint8_t scale) { return static_cast<int8_t>(-maxDegree(scale)); }

inline int8_t clampDegree(int degree, uint8_t scale) {
    if (degree > maxDegree(scale)) return maxDegree(scale);
    if (degree < minDegree(scale)) return minDegree(scale);
    return static_cast<int8_t>(degree);
}

// TRANSPOSE: moves every note `delta` scale steps, keeping the intervals between them. Notes
// are first brought into the scale's range (that is how they sound). When the highest or lowest
// note would leave the range nothing changes and false is returned, so the melody stops at the
// edge as a whole; clamping each note on its own would pile them up on the same edge note and
// lose the melody for good.
inline bool transposeAll(int8_t* notes, uint8_t count, int delta, uint8_t scale) {
    if ((count == 0) || (delta == 0)) return false;
    int lowest = clampDegree(notes[0], scale);
    int highest = lowest;
    for (uint8_t i = 1; i < count; i++) {
        const int d = clampDegree(notes[i], scale);
        if (d < lowest) lowest = d;
        if (d > highest) highest = d;
    }
    if ((highest + delta > maxDegree(scale)) || (lowest + delta < minDegree(scale))) return false;
    for (uint8_t i = 0; i < count; i++) notes[i] = static_cast<int8_t>(clampDegree(notes[i], scale) + delta);
    return true;
}

/* Melody generator (NOTE page: hold ADD = new melody, tap ADD = variation) ----------------*/

// xorshift32: small, fast and deterministic, so the generator can be tested on a PC.
inline uint32_t nextRandom(uint32_t& state) {
    if (state == 0) state = 0x9E3779B9u;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

// One octave of the scale, in degrees: the generator stays within this distance of the root.
inline int melodySpan(uint8_t scale) { return kScales[safeScale(scale)].size; }

// A new melody for `count` beats (in time order): a random walk through the scale that mostly
// moves by steps, sometimes leaps, never leaves one octave either side of the root, and starts
// and (from 4 beats on) ends on the root, so it loops back home.
inline void generateMelody(int8_t* notes, uint8_t count, uint8_t scale, uint32_t& seed) {
    static constexpr int8_t kMoves[] = {0, 0, 1, 1, 1, -1, -1, -1, 2, -2, 3, -3, 4, -4};
    constexpr uint32_t kMoveCount = sizeof(kMoves) / sizeof(kMoves[0]);
    const int span = melodySpan(scale);
    int degree = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (i > 0) {
            degree += kMoves[nextRandom(seed) % kMoveCount];
            if (degree > span) degree = (2 * span) - degree;  // bounce off the top...
            if (degree < -span) degree = (-2 * span) - degree;  // ...and the bottom
        }
        notes[i] = static_cast<int8_t>(degree);
    }
    if (count >= 4) notes[count - 1] = 0;
}

// A variation: about a quarter of the notes (at least one, each a different beat) move one or
// two scale steps, staying within one octave of the root. Every chosen note really changes, so
// each tap is audible; tapping again and again lets the melody evolve.
inline uint8_t mutateMelody(int8_t* notes, uint8_t count, uint8_t scale, uint32_t& seed) {
    if (count == 0) return 0;
    static constexpr int8_t kSteps[4] = {-2, -1, 1, 2};
    const int span = melodySpan(scale);
    const uint8_t changes = (count >= 8) ? static_cast<uint8_t>(count / 4) : 1;
    uint64_t used = 0;
    for (uint8_t c = 0; c < changes; c++) {
        uint8_t index = static_cast<uint8_t>(nextRandom(seed) % count);
        while (used & (1ull << index)) index = static_cast<uint8_t>((index + 1) % count);
        used |= (1ull << index);

        int degree = notes[index];
        if (degree > span) degree = span;
        if (degree < -span) degree = -span;
        const int step = kSteps[nextRandom(seed) % 4];
        int next = degree + step;
        if ((next > span) || (next < -span)) next = degree - step;  // turn around at the edge
        notes[index] = static_cast<int8_t>(next);
    }
    return changes;
}

// Pads as a scale keyboard: pad 0..9 plays degree octave * size + pad. Octaves -2..+1 keep at
// least one full octave of the scale inside the two-octave range for every scale.
constexpr int8_t kMinKeysOctave = -2;
constexpr int8_t kMaxKeysOctave = 1;
constexpr uint8_t kKeyCount = 10;

inline int8_t keyDegree(uint8_t pad, int8_t octave, uint8_t scale) {
    return clampDegree((octave * kScales[safeScale(scale)].size) + pad, scale);
}

// Floor division that rounds towards minus infinity (C++ '/' rounds towards zero).
inline int floorDiv(int a, int b) { return (a >= 0) ? (a / b) : -((-a + b - 1) / b); }

// Root C..F# moves the layer up 0..6 semitones, G..B moves it down 5..1.
inline int rootOffset(uint8_t root) {
    const int r = root % kRootCount;
    return (r <= 6) ? r : r - 12;
}

// Semitones from the sample's natural pitch (C) for a scale degree.
inline int semitones(int degree, uint8_t scale, uint8_t root) {
    const Scale& s = kScales[safeScale(scale)];
    const int d = clampDegree(degree, scale);
    const int octave = floorDiv(d, s.size);
    const int index = d - (octave * s.size);
    return rootOffset(root) + (octave * 12) + s.steps[index];
}

// 2^(semitones/12) without powf: one table lookup and a few octave doublings/halvings.
inline float ratio(int semis) {
    static constexpr float kSemitoneRatio[12] = {1.0f,          1.059463094f, 1.122462048f, 1.189207115f,
                                                 1.259921050f, 1.334839854f, 1.414213562f, 1.498307077f,
                                                 1.587401052f, 1.681792831f, 1.781797436f, 1.887748625f};
    const int octave = floorDiv(semis, 12);
    float r = kSemitoneRatio[semis - (octave * 12)];
    for (int i = 0; i < octave; i++) r *= 2.0f;
    for (int i = 0; i > octave; i--) r *= 0.5f;
    return r;
}

inline float clampIncrement(float increment) {
    if (increment < kMinIncrement) return kMinIncrement;
    if (increment > kMaxIncrement) return kMaxIncrement;
    return increment;
}

// MIDI note sent for a TONE beat: the sample's natural pitch is C4 = 60.
inline uint8_t midiNote(int semis) {
    const int n = 60 + semis;
    return static_cast<uint8_t>((n < 0) ? 0 : ((n > 127) ? 127 : n));
}

// Note name with octave, e.g. "D#4" (C4 = the sample's natural pitch). out needs 5 bytes.
inline void noteName(int semis, char* out, size_t size) {
    const int n = 60 + semis;
    const int pitchClass = n - (floorDiv(n, 12) * 12);
    const int octave = floorDiv(n, 12) - 1;
    snprintf(out, size, "%s%d", kNoteNames[pitchClass], octave);
}

// GLIDE 0..100 -> per-audio-block smoothing coefficient for the playback speed (0 = off).
// The time constant grows quadratically from 5 ms (1) to 500 ms (100), so small values give
// a quick, 303-like slide and the top of the range a slow swoop.
inline float glideBlockCoef(uint8_t glide, float blocksPerSecond) {
    if ((glide == 0) || (blocksPerSecond <= 0.0f)) return 0.0f;
    const float g = static_cast<float>((glide > kMaxGlide) ? kMaxGlide : glide) / 100.0f;
    const float seconds = 0.005f + (0.495f * g * g);
    return expf(-1.0f / (seconds * blocksPerSecond));
}

}  // namespace tone

#endif
