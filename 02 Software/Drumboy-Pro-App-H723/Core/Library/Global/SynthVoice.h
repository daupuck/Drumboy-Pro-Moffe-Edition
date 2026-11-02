#ifndef __SYNTH_VOICE_H
#define __SYNTH_VOICE_H

// SYNTH layers: one monophonic subtractive voice per layer (step 4a of the melody-layers plan).
// No hardware, HAL or Controller dependencies, so the exact same code runs on the device (in the
// I2S audio interrupt) and in the host unit test (02 Software/Tests/test_synth_voice.cpp).
//
//   oscillator (PolyBLEP saw/square, triangle, sine, sample cycle) + DETUNE, SUB, NOISE
//     -> 4-pole "ladder" with saturated resonance feedback and DRIVE; FILTER TYPE taps it as
//        low-pass 24 or 12 dB, band-pass or high-pass (v1.13)
//     -> amp envelope: ATTACK, DECAY to SUSTAIN while the note is held (GATE), RELEASE (v1.13;
//        before: fixed 2 ms attack, full level, 12 ms release - still the default)
//     -> the layer's LFO on cutoff, pitch or level (v1.13)
//   filter envelope: jumps to 1 at every note and decays with DECAY; ENV AMT sets how far it
//   opens the filter. Velocity (the beat's FLAT/GHOST setting) scales it: accent for free.
//
// Cost: the filter coefficients are computed once per 32-sample block; per sample it is one
// oscillator, four one-pole stages, one soft clip and two multiplies (~50 cycles on the M7).
//
// Stability: every filter stage is a one-pole low-pass with 0 < g < 1 fed by a soft-clipped
// signal (|x| < 1), so all states stay inside (-1, 1) for any setting; the test checks it.

#include <math.h>
#include <stdint.h>

namespace synth {

// kSample: the layer's loaded sample is played as one cycle of the waveform (single-cycle WAVs).
enum Wave : uint8_t { kSaw = 0, kSquare, kTriangle, kSine, kSample, kWaveCount };

inline constexpr const char* kWaveNames[kWaveCount] = {"SAW", "SQUARE", "TRIANGLE", "SINE", "SAMPLE"};

// FILTER TYPE: which output of the ladder is heard.
enum FilterType : uint8_t { kLowPass24 = 0, kLowPass12, kBandPass, kHighPass, kFilterTypeCount };
inline constexpr const char* kFilterTypeNames[kFilterTypeCount] = {"LP 24", "LP 12", "BAND", "HIGH"};

// The layer's own LFO (Layer Instrument LFO page of a SYNTH layer, v1.13).
enum LfoShape : uint8_t { kLfoSine = 0, kLfoTriangle, kLfoSaw, kLfoSquare, kLfoRandom, kLfoShapeCount };
inline constexpr const char* kLfoShapeNames[kLfoShapeCount] = {"SINE", "TRIANGLE", "SAW DOWN", "SQUARE", "RANDOM"};

// What the LFO moves. PITCH = vibrato, LEVEL = tremolo.
enum LfoTarget : uint8_t { kLfoCutoff = 0, kLfoPitch, kLfoLevel, kLfoTargetCount };
inline constexpr const char* kLfoTargetNames[kLfoTargetCount] = {"CUTOFF", "PITCH", "LEVEL"};

// RATE: one LFO cycle lasts this many quarter notes, or bars (bars follow the Rhythm menu's
// MEASURE setting), so it always locks to the tempo.
struct LfoRate {
    const char* name;
    float amount;
    bool bars;
};
inline constexpr LfoRate kLfoRates[] = {
    {"1/32", 0.125f, false}, {"1/16", 0.25f, false}, {"1/8T", 1.0f / 3.0f, false}, {"1/8", 0.5f, false},
    {"1/4T", 2.0f / 3.0f, false}, {"1/4", 1.0f, false}, {"1/2", 2.0f, false}, {"1 BAR", 1.0f, true},
    {"2 BARS", 2.0f, true}, {"4 BARS", 4.0f, true}, {"8 BARS", 8.0f, true}};
constexpr uint8_t kLfoRateCount = sizeof(kLfoRates) / sizeof(kLfoRates[0]);

// Phase step per sample for a RATE at a tempo (BPM) and bar length (quarter notes per bar).
inline float lfoIncrement(uint8_t rate, float bpm, float quartersPerBar, float sampleRate) {
    if (rate >= kLfoRateCount) rate = 5;
    if (!(bpm >= 1.0f)) bpm = 120.0f;  // also catches NaN
    if (!(quartersPerBar >= 1.0f)) quartersPerBar = 4.0f;
    const float quarters = kLfoRates[rate].amount * (kLfoRates[rate].bars ? quartersPerBar : 1.0f);
    const float samplesPerQuarter = (60.0f / bpm) * sampleRate;
    return 1.0f / (quarters * samplesPerQuarter);
}

// Sound settings of a layer (0..100 except the choices). Saved in the project's .rwn file and
// in drumkits.
struct Params {
    uint8_t wave = kSaw;
    uint8_t cutoff = 35;
    uint8_t resonance = 40;
    uint8_t envAmount = 60;
    uint8_t decay = 35;
    uint8_t gate = 50;   // % of the time until the next beat
    uint8_t drive = 25;
    uint8_t detune = 0;  // 0 = one oscillator; 1..100 = two, spread up to +-40 cents around the note
    uint8_t sub = 0;     // level of a square one octave down
    uint8_t noise = 0;   // level of white noise
    uint8_t lfoShape = kLfoTriangle;
    uint8_t lfoRate = 5;           // index into kLfoRates (1/4)
    uint8_t lfoDepth = 0;          // 0 = LFO off
    uint8_t lfoTarget = kLfoCutoff;
    uint8_t lfoRetrig = 0;         // 0 = FREE (keeps running), 1 = NOTE (restarts with every note)
    // AMP page (v1.13). The defaults give the v1.11/v1.12 sound: ~2 ms attack, full level while
    // held, ~12 ms release.
    uint8_t attack = 10;           // 1 ms .. 3 s
    uint8_t ampDecay = 50;         // 5 ms .. 5 s, towards SUSTAIN
    uint8_t sustain = 100;         // level while the note is held after the decay
    uint8_t release = 13;          // 5 ms .. 5 s after the note ends
    uint8_t filterType = kLowPass24;
    // PAD HOLD (v1.13): 1 = a pad note sounds while the pad is pressed (TONE and SYNTH layers);
    // 0 = a short note as before. A playing setting, kept when presets or random sounds load.
    uint8_t padHold = 0;
};

// Envelope times in seconds for AMP 0..100 (exponential: fine control at the short end).
inline float attackSeconds(uint8_t x) { return 0.001f * exp2f((static_cast<float>(x) / 100.0f) * 11.55f); }   // 1 ms .. 3 s
inline float decaySeconds(uint8_t x) { return 0.005f * exp2f((static_cast<float>(x) / 100.0f) * 9.97f); }     // 5 ms .. 5 s

constexpr uint8_t kMaxParam = 100;
// LFO depth 100: CUTOFF +-3 octaves; PITCH +-12 semitones with a cubic curve (depth 20 is a
// gentle vibrato of +-0.1, 50 is +-1.5); LEVEL from full down to silence.
constexpr float kLfoCutoffOctaves = 3.0f;
constexpr float kLfoPitchSemitones = 12.0f;
// DETUNE 100 = the two oscillators 40 cents below and above the note (80 cents apart): about
// 5 Hz of beating on a 110 Hz bass. (v1.12's first take was 25 cents in one direction, which was
// too subtle to hear on short bass notes.)
constexpr float kMaxDetuneCents = 40.0f;

inline Params sanitize(const Params& p) {
    Params s = p;
    if (s.wave >= kWaveCount) s.wave = kSaw;
    uint8_t* fields[14] = {&s.cutoff, &s.resonance, &s.envAmount, &s.decay, &s.gate, &s.drive, &s.detune,
                           &s.sub, &s.noise, &s.lfoDepth, &s.attack, &s.ampDecay, &s.sustain, &s.release};
    for (uint8_t* f : fields) {
        if (*f > kMaxParam) *f = kMaxParam;
    }
    if (s.filterType >= kFilterTypeCount) s.filterType = kLowPass24;
    if (s.padHold > 1) s.padHold = 0;
    if (s.lfoShape >= kLfoShapeCount) s.lfoShape = kLfoTriangle;
    if (s.lfoRate >= kLfoRateCount) s.lfoRate = 5;
    if (s.lfoTarget >= kLfoTargetCount) s.lfoTarget = kLfoCutoff;
    if (s.lfoRetrig > 1) s.lfoRetrig = 0;
    return s;
}

// LFO shape value in [-1, 1] at phase 0..1 (RANDOM: the value held for this cycle). Phase 0 is
// where a retriggered LFO starts: SINE/TRIANGLE at the centre going up, SAW DOWN and SQUARE at
// the top (so on CUTOFF they open the filter and then close it, like an extra envelope).
inline float lfoValue(uint8_t shape, float phase, float hold) {
    switch (shape) {
        case kLfoSine:
            return sinf(6.2831853f * phase);
        case kLfoSaw:
            return 1.0f - (2.0f * phase);
        case kLfoSquare:
            return (phase < 0.5f) ? 1.0f : -1.0f;
        case kLfoRandom:
            return hold;
        case kLfoTriangle:
        default:
            if (phase < 0.25f) return 4.0f * phase;
            if (phase < 0.75f) return 2.0f - (4.0f * phase);
            return (4.0f * phase) - 4.0f;
    }
}

// File layout of one layer's sound (project .rwn "SYN5", drumkit "SYK3", user presets):
// wave, cutoff, resonance, env amount, decay, gate, drive, detune, sub, noise,
// LFO shape, rate, depth, target, retrig, attack, amp decay, sustain, release, filter type,
// pad hold. Older files hold fewer values ("SYN1" 7, "SYN2" 10, test builds 15/20); the rest
// keep their defaults.
constexpr uint8_t kPackedSize = 21;

inline void pack(const Params& p, uint8_t* d) {
    d[0] = p.wave;
    d[1] = p.cutoff;
    d[2] = p.resonance;
    d[3] = p.envAmount;
    d[4] = p.decay;
    d[5] = p.gate;
    d[6] = p.drive;
    d[7] = p.detune;
    d[8] = p.sub;
    d[9] = p.noise;
    d[10] = p.lfoShape;
    d[11] = p.lfoRate;
    d[12] = p.lfoDepth;
    d[13] = p.lfoTarget;
    d[14] = p.lfoRetrig;
    d[15] = p.attack;
    d[16] = p.ampDecay;
    d[17] = p.sustain;
    d[18] = p.release;
    d[19] = p.filterType;
    d[20] = p.padHold;
}

// Reads `count` values (7..21); never trusts them (sanitized: a damaged file can't index a table).
inline Params unpack(const uint8_t* d, uint8_t count) {
    Params p;
    uint8_t* fields[kPackedSize] = {&p.wave,      &p.cutoff,  &p.resonance, &p.envAmount, &p.decay,   &p.gate,       &p.drive,
                                    &p.detune,    &p.sub,     &p.noise,     &p.lfoShape,  &p.lfoRate, &p.lfoDepth,   &p.lfoTarget,
                                    &p.lfoRetrig, &p.attack,  &p.ampDecay,  &p.sustain,   &p.release, &p.filterType, &p.padHold};
    for (uint8_t i = 0; (i < count) && (i < kPackedSize); i++) *fields[i] = d[i];
    return sanitize(p);
}

inline bool isDefault(const Params& p) {
    const Params def;
    uint8_t a[kPackedSize], b[kPackedSize];
    pack(p, a);
    pack(def, b);
    for (uint8_t i = 0; i < kPackedSize; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// A note as prepared by the sequencer (or a pad) for the audio interrupt.
struct NoteOn {
    float increment = 0.0f;    // oscillator phase step per sample (frequency / sample rate)
    float gain = 0.0f;         // level, already including layer level, velocity and master volume
    float velocity = 1.0f;     // 1 = FLAT (accent), 0.5 = GHOST
    uint32_t gateSamples = 0;  // how long the note is held
    float glideCoef = 0.0f;    // > 0: slide from the previous pitch (per-block coefficient)
    bool legato = false;       // SLIDE: if the previous note is still held, keep its envelopes and only glide
    const float* table = nullptr;  // WAVE = SAMPLE: one cycle (the layer's sample in SDRAM)
    uint32_t tableLength = 0;
    bool lfoRetrig = false;        // LFO RETRIG = NOTE: the LFO restarts with this note (not on a slide)
};

struct Voice {
    bool active = false;
    float phase = 0.0f;
    float phase2 = 0.0f;    // DETUNE oscillator
    float phaseSub = 0.0f;  // SUB oscillator (half speed)
    uint32_t noiseState = 0x12345678u;
    float increment = 0.0f;
    float incrementTarget = 0.0f;
    float glideCoef = 0.0f;
    float gain = 0.0f;
    float velocity = 1.0f;
    uint32_t gateLeft = 0;  // samples until release
    float amp = 0.0f;       // amp envelope 0..1
    float filterEnv = 0.0f; // filter envelope 0..1
    // LFO: keeps running between notes (FREE), so it is kept when a note ends.
    float lfoPhase = 0.0f;
    float lfoHold = 0.0f;   // RANDOM: value of the current cycle
    float lfoSmooth = 0.0f; // shape value smoothed per block (no zipper on SQUARE/RANDOM steps)
    float lfoGain = 1.0f;   // LEVEL target: gain at the end of the last block (ramped per sample)
    bool lfoJump = false;   // retriggered: take the new value at once instead of smoothing to it
    bool ampDecaying = false;  // amp envelope: false = ATTACK rising, true = DECAY/SUSTAIN
    float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;  // filter stages
    const float* table = nullptr;
    uint32_t tableLength = 0;
};

// Render result flags.
constexpr uint8_t kGateEnded = 0x01;  // the held part of the note ended in this block (MIDI Note Off)
constexpr uint8_t kVoiceEnded = 0x02; // the release finished; the voice is silent now

// Starts (or retriggers) a note. The phase and the filter keep running, and the amp envelope
// starts from where it is, so a retrigger never clicks.
inline void start(Voice& v, const NoteOn& n) {
    v.table = n.table;
    v.tableLength = n.tableLength;

    // SLIDE (legato): the previous note is still held, so it simply glides on to the new pitch
    // with its envelopes untouched; only the gate is renewed.
    if (n.legato && v.active && (v.gateLeft > 0) && (v.increment > 0.0f)) {
        v.incrementTarget = n.increment;
        v.glideCoef = (n.glideCoef > 0.0f) ? n.glideCoef : 0.9f;
        v.gateLeft = (n.gateSamples > 0) ? n.gateSamples : 1;
        v.gain = n.gain;
        return;
    }

    // GLIDE: slide from the previous note's pitch, also when that note has already ended (the
    // voice keeps its last pitch for this; only the very first note jumps).
    if ((n.glideCoef > 0.0f) && (v.increment > 0.0f)) {
        v.incrementTarget = n.increment;
        v.glideCoef = n.glideCoef;
    } else {
        v.increment = n.increment;
        v.incrementTarget = n.increment;
        v.glideCoef = 0.0f;
    }
    v.gain = n.gain;
    v.velocity = n.velocity;
    v.gateLeft = (n.gateSamples > 0) ? n.gateSamples : 1;
    v.filterEnv = 1.0f;
    v.ampDecaying = false;  // ATTACK again, from wherever the level is now (no click)
    v.active = true;

    // DETUNE: the second oscillator starts at a random phase (like a free-running analog
    // oscillator), so the two never start in unison and the sound is thick from the first cycle.
    v.noiseState ^= v.noiseState << 13;
    v.noiseState ^= v.noiseState >> 17;
    v.noiseState ^= v.noiseState << 5;
    v.phase2 = static_cast<float>(v.noiseState >> 8) * (1.0f / 16777216.0f);  // [0, 1)

    // LFO RETRIG = NOTE: every new note (not a slide, see above) starts the LFO cycle again.
    if (n.lfoRetrig) {
        v.lfoPhase = 0.0f;
        v.lfoHold = (v.phase2 * 2.0f) - 1.0f;  // a fresh random value for RANDOM
        v.lfoJump = true;
    }
}

// Moves the LFO on by `samples` (also between notes, so a FREE LFO keeps its place in time).
// Returns true when a new cycle began (RANDOM then holds a new value).
inline bool advanceLfo(Voice& v, float increment, uint16_t samples) {
    if (!(increment > 0.0f) || (increment > 0.5f)) return false;  // also catches NaN
    v.lfoPhase += increment * static_cast<float>(samples);
    if (v.lfoPhase < 1.0f) return false;
    v.lfoPhase -= static_cast<float>(static_cast<uint32_t>(v.lfoPhase));
    v.noiseState ^= v.noiseState << 13;
    v.noiseState ^= v.noiseState >> 17;
    v.noiseState ^= v.noiseState << 5;
    v.lfoHold = (static_cast<float>(v.noiseState >> 8) * (2.0f / 16777216.0f)) - 1.0f;
    return true;
}

inline void stop(Voice& v) { v = Voice{}; }

// PolyBLEP: subtracts the aliasing part of a step at phase 0 (t in [0,1), dt = phase step).
inline float polyBlep(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return t + t - (t * t) - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return (t * t) + t + t + 1.0f;
    }
    return 0.0f;
}

inline float oscillator(uint8_t wave, float t, float dt) {
    switch (wave) {
        case kSquare: {
            float u = t + 0.5f;
            if (u >= 1.0f) u -= 1.0f;
            return ((t < 0.5f) ? 1.0f : -1.0f) + polyBlep(t, dt) - polyBlep(u, dt);
        }
        case kTriangle:
            return 1.0f - (4.0f * fabsf(t - 0.5f));  // gentle harmonics: little aliasing
        case kSine: {
            // sin(pi/2 * triangle): a parabola per half wave plus the usual refinement step
            // (accuracy checked in test_synth_voice.cpp).
            const float x = (t < 0.5f) ? ((4.0f * t) - 1.0f) : (3.0f - (4.0f * t));  // -1..1 triangle
            const float y = x * (2.0f - fabsf(x));
            return y * (0.775f + (0.225f * fabsf(y)));
        }
        case kSaw:
        default:
            return ((2.0f * t) - 1.0f) - polyBlep(t, dt);
    }
}

// WAVE = SAMPLE: one cycle read from a table with linear interpolation (wrapping).
inline float tableOscillator(const float* table, uint32_t length, float t) {
    const float position = t * static_cast<float>(length);
    uint32_t i = static_cast<uint32_t>(position);
    if (i >= length) i = length - 1;
    const uint32_t j = (i + 1 < length) ? (i + 1) : 0;
    const float frac = position - static_cast<float>(i);
    return table[i] + ((table[j] - table[i]) * frac);
}

// One oscillator sample for the voice's waveform (falls back to saw without a usable table).
inline float voiceOscillator(const Voice& v, uint8_t wave, float t, float dt) {
    if (wave == kSample) {
        return ((v.table != nullptr) && (v.tableLength >= 2)) ? tableOscillator(v.table, v.tableLength, t) : oscillator(kSaw, t, dt);
    }
    return oscillator(wave, t, dt);
}

// White noise in [-1, 1).
inline float noise(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<float>(state >> 8) * (2.0f / 16777216.0f)) - 1.0f;
}

// Soft clip with |result| < 1 for any input (x / (1 + |x|)).
inline float softClip(float x) { return x / (1.0f + fabsf(x)); }

// Smoother saturation for the output drive stage, |result| < 1, close to tanh (x / sqrt(1 + x^2)).
inline float saturate(float x) { return x / sqrtf(1.0f + (x * x)); }

// Cutoff in Hz for CUTOFF 0..100 plus the envelope (in octaves): 30 Hz .. ~18 kHz.
inline float cutoffHz(uint8_t cutoff, float envOctaves) {
    const float octaves = ((static_cast<float>(cutoff) / 100.0f) * 9.2f) + envOctaves;
    return 30.0f * exp2f(octaves);
}

// Per-block filter envelope decay for DECAY 0..100: time constant 20 ms .. 2 s.
inline float decayBlockCoef(uint8_t decay, float blocksPerSecond) {
    const float d = static_cast<float>((decay > kMaxParam) ? kMaxParam : decay) / 100.0f;
    const float seconds = 0.02f * exp2f(d * 6.64f);  // 20 ms * 2^(0..6.64) = 20 ms .. 2 s
    return expf(-1.0f / (seconds * blocksPerSecond));
}

// Renders `count` samples into out (overwriting). Returns kGateEnded / kVoiceEnded flags.
// lfoIncrement: the layer LFO's phase step per sample (lfoIncrement() from RATE and tempo).
inline uint8_t render(Voice& v, const Params& rawParams, float* out, uint16_t count, float sampleRate, float lfoIncrement = 0.0f) {
    uint8_t events = 0;
    if (!v.active) {
        for (uint16_t i = 0; i < count; i++) out[i] = 0.0f;
        return events;
    }
    const Params p = sanitize(rawParams);
    const float blocksPerSecond = sampleRate / static_cast<float>(count);

    // 1. BLOCK-RATE PARAMETERS
    if (v.glideCoef > 0.0f) {
        v.increment = v.incrementTarget + ((v.increment - v.incrementTarget) * v.glideCoef);
        if (fabsf(v.increment - v.incrementTarget) < (v.incrementTarget * 0.0005f)) {
            v.increment = v.incrementTarget;
            v.glideCoef = 0.0f;
        }
    }
    // LFO: one value per block (32 samples = 0.7 ms), smoothed so SQUARE/RANDOM steps don't click.
    // A retriggered LFO (RETRIG = NOTE) starts on its new value at once.
    advanceLfo(v, lfoIncrement, count);
    const float lfoRaw = lfoValue(p.lfoShape, v.lfoPhase, v.lfoHold);
    v.lfoSmooth = v.lfoJump ? lfoRaw : (v.lfoSmooth + ((lfoRaw - v.lfoSmooth) * 0.5f));
    v.lfoJump = false;
    const float lfoDepth = static_cast<float>(p.lfoDepth) / 100.0f;
    float cutoffLfo = 0.0f;
    float pitchRatio = 1.0f;
    float lfoGainEnd = 1.0f;
    if (p.lfoDepth > 0) {
        if (p.lfoTarget == kLfoCutoff) {
            cutoffLfo = v.lfoSmooth * lfoDepth * kLfoCutoffOctaves;
        } else if (p.lfoTarget == kLfoPitch) {
            pitchRatio = exp2f((v.lfoSmooth * lfoDepth * lfoDepth * lfoDepth * kLfoPitchSemitones) / 12.0f);
        } else {
            lfoGainEnd = 1.0f - (lfoDepth * 0.5f * (1.0f - v.lfoSmooth));  // 1 at the top, 1 - depth at the bottom
        }
    }
    const float lfoGainStart = v.lfoGain;
    const float lfoGainStep = (lfoGainEnd - lfoGainStart) / static_cast<float>(count);
    v.lfoGain = lfoGainEnd;

    const float envOctaves = (static_cast<float>(p.envAmount) / 100.0f) * 6.0f * v.filterEnv * (0.4f + (0.6f * v.velocity));
    float fc = cutoffHz(p.cutoff, envOctaves + cutoffLfo);
    const float maxFc = sampleRate * 0.42f;
    if (fc > maxFc) fc = maxFc;
    const float w = 6.2831853f * fc / sampleRate;
    const float g = w / (1.0f + w);  // one-pole coefficient, always 0 < g < 1
    const float k = (static_cast<float>(p.resonance) / 100.0f) * 3.6f;
    const float drive = 1.0f + ((static_cast<float>(p.drive) / 100.0f) * 4.0f);
    const float makeup = (1.0f + (k * 0.6f)) / (0.6f + (0.4f * drive));  // keeps level roughly even
    // DRIVE also saturates AFTER the filter (like a distortion pedal after a 303), so the added
    // harmonics are not filtered away: gain 1..30x (quadratic feel), normalised so a full-scale
    // signal stays at full scale while quieter parts are pushed up and squashed. DRIVE 0 = clean.
    const float driveAmount = static_cast<float>(p.drive) / 100.0f;
    const bool postDriveOn = (p.drive > 0);
    const float postGain = 1.0f + (29.0f * driveAmount * driveAmount);
    const float postNorm = 1.0f / saturate(postGain);
    const float postWet = (driveAmount * 4.0f > 1.0f) ? 1.0f : (driveAmount * 4.0f);  // fades in over 0..25
    v.filterEnv *= decayBlockCoef(p.decay, blocksPerSecond);

    // Amp envelope (AMP page): linear ATTACK to full, exponential DECAY towards SUSTAIN while
    // held, exponential RELEASE after the gate.
    const float attackStep = 1.0f / (attackSeconds(p.attack) * sampleRate);
    const float sustainLevel = static_cast<float>(p.sustain) / 100.0f;
    const float ampDecayCoef = expf(-1.0f / (decaySeconds(p.ampDecay) * sampleRate));
    const float releaseCoef = expf(-1.0f / (decaySeconds(p.release) * sampleRate));
    // FILTER TYPE: weights of the input and the four stages (classic ladder mode mixing).
    float wx = 0.0f, w1 = 0.0f, w2 = 0.0f, w4 = 1.0f, typeGain = 1.0f;
    switch (p.filterType) {
        case kLowPass12:
            w2 = 1.0f;
            w4 = 0.0f;
            break;
        case kBandPass:  // 2 * (y1 - y2): 6 dB per octave on each side, +- the resonance peak
            w1 = 2.0f;
            w2 = -2.0f;
            w4 = 0.0f;
            typeGain = 1.6f;
            break;
        case kHighPass:  // x - 2 y1 + y2: 12 dB per octave below the cutoff
            wx = 1.0f;
            w1 = -2.0f;
            w2 = 1.0f;
            w4 = 0.0f;
            break;
        default:
            break;
    }
    float dt = v.increment * pitchRatio;  // LFO on PITCH = vibrato
    if (dt > 0.45f) dt = 0.45f;            // stays below Nyquist even at the top of a big swing

    // Oscillator mix: DETUNE adds a second oscillator, the two spread symmetrically below and above
    // the note so it stays in tune (both then at 0.6); SUB a square one octave down, NOISE white
    // noise. Parts that are off cost nothing per sample.
    const bool detuneOn = (p.detune > 0);
    const float spread = (static_cast<float>(p.detune) / 100.0f) * kMaxDetuneCents * 0.0005778f;  // ~ln(2)/1200 per cent
    const float dt1 = detuneOn ? dt * (1.0f - spread) : dt;
    const float dt2 = dt * (1.0f + spread);
    const float mainLevel = detuneOn ? 0.6f : 1.0f;
    const float subLevel = (static_cast<float>(p.sub) / 100.0f) * 0.8f;
    const float noiseLevel = (static_cast<float>(p.noise) / 100.0f) * 0.6f;
    const float dtSub = dt * 0.5f;

    // 2. SAMPLE LOOP
    for (uint16_t i = 0; i < count; i++) {
        float osc = voiceOscillator(v, p.wave, v.phase, dt1) * mainLevel;
        v.phase += dt1;
        if (v.phase >= 1.0f) v.phase -= 1.0f;
        if (detuneOn) {
            osc += voiceOscillator(v, p.wave, v.phase2, dt2) * 0.6f;
            v.phase2 += dt2;
            if (v.phase2 >= 1.0f) v.phase2 -= 1.0f;
        }
        if (subLevel > 0.0f) {
            osc += oscillator(kSquare, v.phaseSub, dtSub) * subLevel;
            v.phaseSub += dtSub;
            if (v.phaseSub >= 1.0f) v.phaseSub -= 1.0f;
        }
        if (noiseLevel > 0.0f) osc += noise(v.noiseState) * noiseLevel;

        const float x = softClip((osc * drive) - (k * v.s4));
        v.s1 += g * (x - v.s1);
        v.s2 += g * (v.s1 - v.s2);
        v.s3 += g * (v.s2 - v.s3);
        v.s4 += g * (v.s3 - v.s4);

        if (v.gateLeft > 0) {
            v.gateLeft--;
            if (v.gateLeft == 0) events |= kGateEnded;
            if (!v.ampDecaying) {
                v.amp += attackStep;
                if (v.amp >= 1.0f) {
                    v.amp = 1.0f;
                    v.ampDecaying = true;
                }
            } else {
                v.amp = sustainLevel + ((v.amp - sustainLevel) * ampDecayCoef);
            }
        } else {
            v.amp *= releaseCoef;
        }
        const float filtered = (p.filterType == kLowPass24) ? v.s4 : (((wx * x) + (w1 * v.s1) + (w2 * v.s2) + (w4 * v.s4)) * typeGain);
        float y = filtered * makeup;
        if (postDriveOn) y += postWet * ((saturate(y * postGain) * postNorm) - y);
        out[i] = y * v.amp * v.gain * (lfoGainStart + (lfoGainStep * static_cast<float>(i + 1)));
    }

    // Between notes: idle voices are not rendered, so the controller calls advanceLfo() for them.

    // 3. END OF NOTE
    if ((v.gateLeft == 0) && (v.amp < 0.0001f)) {
        const float phase = v.phase, phase2 = v.phase2, phaseSub = v.phaseSub;
        const float pitch = v.incrementTarget;  // where the note ended up (after any glide)
        const uint32_t noiseState = v.noiseState;
        const float lfoPhase = v.lfoPhase, lfoHold = v.lfoHold, lfoSmooth = v.lfoSmooth, lfoGain = v.lfoGain;
        stop(v);
        v.lfoPhase = lfoPhase;  // the LFO keeps its place for the next note (FREE)
        v.lfoHold = lfoHold;
        v.lfoSmooth = lfoSmooth;
        v.lfoGain = lfoGain;
        v.phase = phase;  // oscillators keep running in phase: no click on the next note
        v.phase2 = phase2;
        v.phaseSub = phaseSub;
        v.noiseState = noiseState;
        v.increment = pitch;  // GLIDE on the next note starts here
        v.incrementTarget = pitch;
        events |= kVoiceEnded;
    }
    return events;
}

}  // namespace synth

#endif
