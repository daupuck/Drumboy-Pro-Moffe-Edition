#ifndef __SYNTH_VOICE_H
#define __SYNTH_VOICE_H

// SYNTH layers: one monophonic subtractive voice per layer (step 4a of the melody-layers plan).
// No hardware, HAL or Controller dependencies, so the exact same code runs on the device (in the
// I2S audio interrupt) and in the host unit test (02 Software/Tests/test_synth_voice.cpp).
//
//   oscillator (PolyBLEP saw/square, triangle, sine)
//     -> 4-pole low-pass "ladder" with saturated resonance feedback and DRIVE
//     -> amp envelope (2 ms attack, held for GATE, 12 ms release)
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

// Sound settings of a layer (0..100 except wave). Saved in the project's .rwn file.
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
};

constexpr uint8_t kMaxParam = 100;
// DETUNE 100 = the two oscillators 40 cents below and above the note (80 cents apart): about
// 5 Hz of beating on a 110 Hz bass. (v1.12's first take was 25 cents in one direction, which was
// too subtle to hear on short bass notes.)
constexpr float kMaxDetuneCents = 40.0f;

inline Params sanitize(const Params& p) {
    Params s = p;
    if (s.wave >= kWaveCount) s.wave = kSaw;
    uint8_t* fields[9] = {&s.cutoff, &s.resonance, &s.envAmount, &s.decay, &s.gate, &s.drive, &s.detune, &s.sub, &s.noise};
    for (uint8_t* f : fields) {
        if (*f > kMaxParam) *f = kMaxParam;
    }
    return s;
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
    v.active = true;

    // DETUNE: the second oscillator starts at a random phase (like a free-running analog
    // oscillator), so the two never start in unison and the sound is thick from the first cycle.
    v.noiseState ^= v.noiseState << 13;
    v.noiseState ^= v.noiseState >> 17;
    v.noiseState ^= v.noiseState << 5;
    v.phase2 = static_cast<float>(v.noiseState >> 8) * (1.0f / 16777216.0f);  // [0, 1)
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
inline uint8_t render(Voice& v, const Params& rawParams, float* out, uint16_t count, float sampleRate) {
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
    const float envOctaves = (static_cast<float>(p.envAmount) / 100.0f) * 6.0f * v.filterEnv * (0.4f + (0.6f * v.velocity));
    float fc = cutoffHz(p.cutoff, envOctaves);
    const float maxFc = sampleRate * 0.42f;
    if (fc > maxFc) fc = maxFc;
    const float w = 6.2831853f * fc / sampleRate;
    const float g = w / (1.0f + w);  // one-pole coefficient, always 0 < g < 1
    const float k = (static_cast<float>(p.resonance) / 100.0f) * 3.6f;
    const float drive = 1.0f + ((static_cast<float>(p.drive) / 100.0f) * 4.0f);
    const float makeup = (1.0f + (k * 0.6f)) / (0.6f + (0.4f * drive));  // keeps level roughly even
    v.filterEnv *= decayBlockCoef(p.decay, blocksPerSecond);

    const float attackStep = 1.0f / (0.002f * sampleRate);  // 2 ms
    const float releaseCoef = expf(-1.0f / (0.012f * sampleRate));  // 12 ms
    const float dt = v.increment;

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
            v.amp += attackStep;
            if (v.amp > 1.0f) v.amp = 1.0f;
        } else {
            v.amp *= releaseCoef;
        }
        out[i] = v.s4 * makeup * v.amp * v.gain;
    }

    // 3. END OF NOTE
    if ((v.gateLeft == 0) && (v.amp < 0.0001f)) {
        const float phase = v.phase, phase2 = v.phase2, phaseSub = v.phaseSub;
        const float pitch = v.incrementTarget;  // where the note ended up (after any glide)
        const uint32_t noiseState = v.noiseState;
        stop(v);
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
