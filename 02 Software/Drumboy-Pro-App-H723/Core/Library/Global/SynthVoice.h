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

enum Wave : uint8_t { kSaw = 0, kSquare, kTriangle, kSine, kWaveCount };

inline constexpr const char* kWaveNames[kWaveCount] = {"SAW", "SQUARE", "TRIANGLE", "SINE"};

// Sound settings of a layer (0..100 except wave). Saved in the project's .rwn file.
struct Params {
    uint8_t wave = kSaw;
    uint8_t cutoff = 35;
    uint8_t resonance = 40;
    uint8_t envAmount = 60;
    uint8_t decay = 35;
    uint8_t gate = 50;   // % of the time until the next beat
    uint8_t drive = 25;
};

constexpr uint8_t kMaxParam = 100;

inline Params sanitize(const Params& p) {
    Params s = p;
    if (s.wave >= kWaveCount) s.wave = kSaw;
    uint8_t* fields[6] = {&s.cutoff, &s.resonance, &s.envAmount, &s.decay, &s.gate, &s.drive};
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
};

struct Voice {
    bool active = false;
    float phase = 0.0f;
    float increment = 0.0f;
    float incrementTarget = 0.0f;
    float glideCoef = 0.0f;
    float gain = 0.0f;
    float velocity = 1.0f;
    uint32_t gateLeft = 0;  // samples until release
    float amp = 0.0f;       // amp envelope 0..1
    float filterEnv = 0.0f; // filter envelope 0..1
    float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;  // filter stages
};

// Render result flags.
constexpr uint8_t kGateEnded = 0x01;  // the held part of the note ended in this block (MIDI Note Off)
constexpr uint8_t kVoiceEnded = 0x02; // the release finished; the voice is silent now

// Starts (or retriggers) a note. The phase and the filter keep running, and the amp envelope
// starts from where it is, so a retrigger never clicks.
inline void start(Voice& v, const NoteOn& n) {
    if (v.active && (n.glideCoef > 0.0f) && (v.increment > 0.0f)) {
        v.incrementTarget = n.increment;  // slide from the current pitch
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

    // 2. SAMPLE LOOP
    for (uint16_t i = 0; i < count; i++) {
        const float osc = oscillator(p.wave, v.phase, dt);
        v.phase += dt;
        if (v.phase >= 1.0f) v.phase -= 1.0f;

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
        const float phase = v.phase;
        stop(v);
        v.phase = phase;
        events |= kVoiceEnded;
    }
    return events;
}

}  // namespace synth

#endif
