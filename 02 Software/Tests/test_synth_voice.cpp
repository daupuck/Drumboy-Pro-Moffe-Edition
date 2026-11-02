// Host unit tests for SYNTH layers: Core/Library/Global/SynthVoice.h.
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_synth_voice.cpp -o test_synth_voice && ./test_synth_voice
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "SynthPresets.h"
#include "SynthVoice.h"

using namespace synth;

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

constexpr float kFs = 44100.0f;
constexpr uint16_t kBlock = 32;

static NoteOn note(float hz, uint32_t gate, float velocity = 1.0f, float glide = 0.0f) {
    NoteOn n;
    n.increment = hz / kFs;
    n.gain = 1.0f;
    n.velocity = velocity;
    n.gateSamples = gate;
    n.glideCoef = glide;
    return n;
}

// Renders `blocks` blocks; returns the peak and the flags seen, and checks every sample is finite.
static float run(Voice& v, const Params& p, int blocks, uint8_t& flags, std::vector<float>* keep = nullptr) {
    float block[kBlock];
    float peak = 0.0f;
    flags = 0;
    for (int b = 0; b < blocks; b++) {
        flags |= render(v, p, block, kBlock, kFs);
        for (uint16_t i = 0; i < kBlock; i++) {
            CHECK(std::isfinite(block[i]), "finite output");
            peak = std::fmax(peak, std::fabs(block[i]));
            if (keep) keep->push_back(block[i]);
        }
    }
    return peak;
}

static void testOscillators() {
    // Every waveform stays within a small overshoot of +-1 and has (almost) no DC.
    for (uint8_t w = 0; w < kWaveCount; w++) {
        for (float hz : {30.0f, 110.0f, 440.0f, 2000.0f, 8000.0f}) {
            const float dt = hz / kFs;
            float t = 0.0f, sum = 0.0f, peak = 0.0f;
            const int n = static_cast<int>(kFs);  // one second
            for (int i = 0; i < n; i++) {
                const float s = oscillator(w, t, dt);
                sum += s;
                peak = std::fmax(peak, std::fabs(s));
                t += dt;
                if (t >= 1.0f) t -= 1.0f;
            }
            CHECK(peak <= 1.3f, "wave %d at %.0f Hz peak %f", w, hz, peak);
            CHECK(std::fabs(sum / n) < 0.02f, "wave %d at %.0f Hz DC %f", w, hz, sum / n);
        }
    }
    // The sine approximation is close to a real sine (of the same phase-shifted triangle).
    float worst = 0.0f;
    for (int i = 0; i < 1000; i++) {
        const float t = i / 1000.0f;
        const float x = (t < 0.5f) ? ((4.0f * t) - 1.0f) : (3.0f - (4.0f * t));
        worst = std::fmax(worst, std::fabs(oscillator(kSine, t, 0.0f) - std::sin(1.5707963f * x)));
    }
    CHECK(worst < 0.002f, "sine approximation error %f", worst);
    std::printf("sine approximation max error: %.5f\n", worst);
}

static void testPolyBlepReducesAliasing() {
    // A naive saw at a high pitch has much more energy in the sample-to-sample differences
    // (high frequencies) than the PolyBLEP saw.
    const float dt = 3520.0f / kFs;
    float t = 0.0f, prevNaive = 0.0f, prevBlep = 0.0f, eNaive = 0.0f, eBlep = 0.0f;
    for (int i = 0; i < 44100; i++) {
        const float naive = (2.0f * t) - 1.0f;
        const float blep = oscillator(kSaw, t, dt);
        eNaive += (naive - prevNaive) * (naive - prevNaive);
        eBlep += (blep - prevBlep) * (blep - prevBlep);
        prevNaive = naive;
        prevBlep = blep;
        t += dt;
        if (t >= 1.0f) t -= 1.0f;
    }
    CHECK(eBlep < eNaive * 0.8f, "PolyBLEP saw is smoother: %f vs %f", eBlep, eNaive);
}

static void testFilterStability() {
    // Worst cases: every wave, max resonance, max drive, max envelope, cutoff low/mid/max, low and
    // very high notes. States must stay bounded and the output finite.
    for (uint8_t w = 0; w < kWaveCount; w++) {
        for (uint8_t cutoff : {0, 50, 100}) {
            for (float hz : {20.0f, 220.0f, 4000.0f, 16000.0f}) {
                Params p;
                p.wave = w;
                p.cutoff = cutoff;
                p.resonance = 100;
                p.drive = 100;
                p.envAmount = 100;
                p.decay = 100;
                Voice v;
                start(v, note(hz, 44100));
                uint8_t flags = 0;
                const float peak = run(v, p, 1400, flags);
                CHECK(peak < 3.0f, "wave %d cutoff %d %.0f Hz: peak %f", w, cutoff, hz, peak);
                CHECK(std::fabs(v.s4) < 1.0f && std::fabs(v.s1) < 1.0f, "states inside (-1,1)");
            }
        }
    }
    // Garbage parameters are sanitised, not trusted.
    Params bad;
    bad.wave = 200;
    bad.cutoff = bad.resonance = bad.drive = bad.envAmount = bad.decay = bad.gate = 250;
    Voice v;
    start(v, note(440.0f, 1000));
    uint8_t flags = 0;
    run(v, bad, 100, flags);
    const Params s = sanitize(bad);
    CHECK(s.wave == kSaw && s.cutoff == 100 && s.gate == 100, "sanitize");
}

static void testFilterResponse() {
    // A closed filter is much quieter than an open one on a saw; resonance keeps it audible.
    auto level = [](uint8_t cutoff, uint8_t reso) {
        Params p;
        p.cutoff = cutoff;
        p.resonance = reso;
        p.envAmount = 0;
        p.drive = 0;
        Voice v;
        start(v, note(110.0f, 44100));
        std::vector<float> out;
        uint8_t flags = 0;
        run(v, p, 400, flags, &out);
        double e = 0.0;
        for (size_t i = out.size() / 2; i < out.size(); i++) e += out[i] * out[i];
        return std::sqrt(e / (out.size() / 2));
    };
    const double open = level(100, 0);
    const double closed = level(10, 0);
    CHECK(closed < open * 0.5, "closed filter is darker: %f vs %f", closed, open);
    CHECK(open > 0.1, "open filter passes the saw: %f", open);
}

static void testEnvelopeAndGate() {
    Params p;
    p.envAmount = 0;
    Voice v;
    // Gate of 10 blocks: Note Off reported once, then the release ends the voice.
    start(v, note(220.0f, 10 * kBlock));
    uint8_t flags = 0;
    run(v, p, 10, flags);
    CHECK(flags & kGateEnded, "gate ended after 10 blocks");
    CHECK(v.active, "still releasing");
    run(v, p, 200, flags);
    CHECK(flags & kVoiceEnded, "release finished");
    CHECK(!v.active, "voice off");

    // The attack ramps up (no click at note start).
    Voice a;
    start(a, note(220.0f, 44100));
    float block[kBlock];
    render(a, p, block, kBlock, kFs);
    CHECK(a.amp < 0.5f, "2 ms attack: after 32 samples amp %f", a.amp);

    // Retrigger keeps the amp envelope level (no click) and restarts the filter envelope.
    Voice r;
    start(r, note(220.0f, 44100));
    for (int i = 0; i < 20; i++) render(r, p, block, kBlock, kFs);
    const float ampBefore = r.amp;
    start(r, note(330.0f, 44100));
    CHECK(r.amp == ampBefore && r.filterEnv == 1.0f, "retrigger: amp kept, filter env restarted");

    // Filter envelope decays, faster for small DECAY.
    const float fast = decayBlockCoef(0, kFs / kBlock), slow = decayBlockCoef(100, kFs / kBlock);
    CHECK(fast > 0.0f && fast < slow && slow < 1.0f, "decay coefficients %f < %f", fast, slow);
    const float remaining = std::pow(slow, 2.0f * kFs / kBlock);  // after 2 s at DECAY 100
    CHECK(std::fabs(remaining - std::exp(-1.0f)) < 0.02f, "DECAY 100 time constant 2 s (%f)", remaining);
}

static void testGlide() {
    Params p;
    Voice v;
    const float coef = 0.95f;
    start(v, note(220.0f, 44100));
    start(v, note(440.0f, 44100, 1.0f, coef));
    CHECK(std::fabs(v.increment - 220.0f / kFs) < 1e-7f, "slide starts at the old pitch");
    float block[kBlock];
    for (int i = 0; i < 400; i++) render(v, p, block, kBlock, kFs);
    CHECK(std::fabs(v.increment - 440.0f / kFs) < 1e-7f, "slide arrives at the new pitch");
    CHECK(v.glideCoef == 0.0f, "slide finished");

    // Without glide (or from silence) the pitch jumps.
    Voice j;
    start(j, note(220.0f, 44100, 1.0f, coef));
    CHECK(std::fabs(j.increment - 220.0f / kFs) < 1e-7f, "first note never slides");

    // Regression (v1.12 hardware test: "GLIDE does not do anything"): with GATE < 100 the previous
    // note has ended (gate + release) before the next beat. GLIDE must still slide from its pitch,
    // as on TONE layers.
    Voice g;
    start(g, note(220.0f, 2000));  // 45 ms gate
    for (int i = 0; i < 400; i++) render(g, p, block, kBlock, kFs);  // 290 ms: gate over, release faded out
    CHECK(!g.active, "previous note has ended");
    start(g, note(440.0f, 2000, 1.0f, coef));
    CHECK(std::fabs(g.increment - 220.0f / kFs) < 1e-7f, "glide starts at the previous note's pitch after a gap");
    CHECK(g.glideCoef > 0.0f && std::fabs(g.incrementTarget - 440.0f / kFs) < 1e-7f, "and slides to the new one");
    // ...and with GLIDE 0 the next note still jumps.
    start(g, note(330.0f, 2000));
    CHECK(std::fabs(g.increment - 330.0f / kFs) < 1e-7f && g.glideCoef == 0.0f, "GLIDE 0 jumps");
}

static void testVelocity() {
    // GHOST (velocity 0.5) opens the filter less than FLAT (accent).
    auto brightness = [](float velocity) {
        Params p;
        p.cutoff = 20;
        p.envAmount = 100;
        p.resonance = 0;
        Voice v;
        start(v, note(110.0f, 44100, velocity));
        std::vector<float> out;
        uint8_t flags = 0;
        run(v, p, 40, flags, &out);
        double d = 0.0;
        for (size_t i = 1; i < out.size(); i++) d += (out[i] - out[i - 1]) * (out[i] - out[i - 1]);
        return d;
    };
    CHECK(brightness(0.5f) < brightness(1.0f), "accent is brighter");
}

static void testDetuneSubNoise() {
    // Everything at maximum on every wave and resonance extreme: still bounded and finite.
    for (uint8_t w = 0; w < kWaveCount; w++) {
        for (uint8_t reso : {0, 100}) {
            Params p;
            p.wave = w;
            p.detune = p.sub = p.noise = 100;
            p.resonance = reso;
            p.drive = 100;
            p.envAmount = 100;
            Voice v;
            start(v, note(110.0f, 44100));
            uint8_t flags = 0;
            const float peak = run(v, p, 700, flags);
            CHECK(peak < 3.0f && peak > 0.01f, "wave %d reso %d with detune/sub/noise: peak %f", w, reso, peak);
        }
    }
    // DETUNE actually detunes: the second phase runs ahead of the first.
    Params p;
    p.detune = 100;
    Voice v;
    start(v, note(220.0f, 44100));
    float block[kBlock];
    for (int i = 0; i < 200; i++) render(v, p, block, kBlock, kFs);
    CHECK(std::fabs(v.phase2 - v.phase) > 0.001f, "detuned oscillator drifts (%f vs %f)", v.phase2, v.phase);
    // SUB runs at half speed.
    Params ps;
    ps.sub = 100;
    Voice vs;
    start(vs, note(441.0f, 44100));
    for (int i = 0; i < 25; i++) render(vs, ps, block, kBlock, kFs);  // 800 samples = 8 cycles at 441 Hz
    CHECK(std::fabs(vs.phaseSub - 0.0f) < 0.01f || std::fabs(vs.phaseSub - 1.0f) < 0.01f, "sub: 4 whole cycles (phase %f)", vs.phaseSub);
    // Noise stays in [-1, 1) and averages out.
    uint32_t state = 1;
    double sum = 0.0;
    bool inRange = true;
    for (int i = 0; i < 100000; i++) {
        const float n = noise(state);
        inRange = inRange && (n >= -1.0f) && (n < 1.0f);
        sum += n;
    }
    CHECK(inRange, "noise range");
    CHECK(std::fabs(sum / 100000.0) < 0.02, "noise mean %f", sum / 100000.0);
}

static void testDetuneIsAudible() {
    // Regression (v1.12 hardware test: "DETUNE does not seem to work"): at DETUNE 100 a 110 Hz bass
    // must beat at least ~4.5 Hz, so it is heard within one short note, and the note stays in tune.
    const float spread = kMaxDetuneCents * 0.0005778f;
    const float dt = 110.0f / kFs;
    const float dt1 = dt * (1.0f - spread), dt2 = dt * (1.0f + spread);
    const float beatHz = (dt2 - dt1) * kFs;
    CHECK(beatHz >= 4.5f, "beating at 110 Hz: %.2f Hz", beatHz);
    const float centreCents = 1200.0f * std::log2(std::sqrt(dt1 * dt2) / dt);
    CHECK(std::fabs(centreCents) < 0.5f, "detuned pair stays in tune (%.3f cents)", centreCents);

    // The second oscillator starts at a different, random phase on every new note.
    Voice v;
    float phases[8];
    for (int i = 0; i < 8; i++) {
        start(v, note(110.0f, 44100));
        phases[i] = v.phase2;
        CHECK(phases[i] >= 0.0f && phases[i] < 1.0f, "phase in [0,1)");
    }
    int distinct = 0;
    for (int i = 1; i < 8; i++) distinct += (std::fabs(phases[i] - phases[i - 1]) > 0.01f) ? 1 : 0;
    CHECK(distinct >= 6, "random start phases (%d of 7 differ)", distinct);

    // ...but a SLIDE (legato) keeps it running continuously.
    Params p;
    float block[kBlock];
    for (int i = 0; i < 10; i++) render(v, p, block, kBlock, kFs);
    const float before = v.phase2;
    NoteOn s = note(165.0f, 44100);
    s.legato = true;
    start(v, s);
    CHECK(v.phase2 == before, "legato keeps the second oscillator's phase");
}

static void testSampleTable() {
    // A 600-sample sine cycle (like a single-cycle WAV) reproduces a sine at the note's pitch.
    std::vector<float> table(600);
    for (size_t i = 0; i < table.size(); i++) table[i] = std::sin(6.2831853f * i / table.size());
    float worst = 0.0f;
    for (int i = 0; i < 5000; i++) {
        const float t = i / 5000.0f;
        worst = std::fmax(worst, std::fabs(tableOscillator(table.data(), 600, t) - std::sin(6.2831853f * t)));
    }
    CHECK(worst < 0.001f, "table interpolation error %f", worst);
    CHECK(tableOscillator(table.data(), 600, 0.99999f) <= 1.0f, "wraps without reading past the end");

    // WAVE = SAMPLE through the voice: uses the table; without a table it falls back to saw.
    Params p;
    p.wave = kSample;
    p.cutoff = 100;
    p.envAmount = 0;
    Voice v;
    NoteOn n = note(220.0f, 44100);
    n.table = table.data();
    n.tableLength = 600;
    start(v, n);
    uint8_t flags = 0;
    const float peak = run(v, p, 200, flags);
    CHECK(peak > 0.1f && peak < 3.0f, "sample wave plays: peak %f", peak);
    Voice none;
    start(none, note(220.0f, 44100));
    CHECK(voiceOscillator(none, kSample, 0.25f, 0.0f) == oscillator(kSaw, 0.25f, 0.0f), "no table: saw");
    NoteOn tiny = note(220.0f, 44100);
    tiny.table = table.data();
    tiny.tableLength = 1;  // unusable
    start(none, tiny);
    CHECK(voiceOscillator(none, kSample, 0.25f, 0.0f) == oscillator(kSaw, 0.25f, 0.0f), "1-sample table: saw");
}

static void testLegato() {
    Params p;
    float block[kBlock];
    // A held note followed by a SLIDE note: envelopes continue, the pitch glides.
    Voice v;
    start(v, note(220.0f, 44100));
    for (int i = 0; i < 40; i++) render(v, p, block, kBlock, kFs);
    const float filterBefore = v.filterEnv;
    NoteOn s = note(330.0f, 44100);
    s.legato = true;
    start(v, s);
    CHECK(v.filterEnv == filterBefore, "slide keeps the filter envelope");
    CHECK(std::fabs(v.increment - 220.0f / kFs) < 1e-7f && std::fabs(v.incrementTarget - 330.0f / kFs) < 1e-7f, "slide glides from the old pitch");
    CHECK(v.gateLeft == 44100, "slide renews the gate");
    for (int i = 0; i < 400; i++) render(v, p, block, kBlock, kFs);  // note still held
    CHECK(v.active && std::fabs(v.increment - 330.0f / kFs) < 1e-6f, "slide arrives");

    // SLIDE after the previous note was already released: a normal (retriggered) note.
    Voice r;
    start(r, note(220.0f, 64));
    for (int i = 0; i < 10; i++) render(r, p, block, kBlock, kFs);  // gate over, releasing
    NoteOn s2 = note(330.0f, 2000);
    s2.legato = true;
    start(r, s2);
    CHECK(r.filterEnv == 1.0f && std::fabs(r.increment - 330.0f / kFs) < 1e-7f, "released note: slide retriggers");
}

static void testDriveIsAudible() {
    // Regression: DRIVE was hardly audible (the clip sat before the filter, which smoothed it away,
    // and the level makeup cancelled the rest). Full DRIVE must clearly thicken the sound: much
    // more sustained level (lower crest factor) at the same settings, and DRIVE 0 stays clean.
    auto measure = [](uint8_t drive, uint8_t cutoff, uint8_t wave, double& rms, double& crest) {
        Params p;
        p.wave = wave;
        p.cutoff = cutoff;
        p.resonance = 30;
        p.envAmount = 0;
        p.drive = drive;
        Voice v;
        start(v, note(110.0f, 44100));
        std::vector<float> out;
        uint8_t flags = 0;
        run(v, p, 400, flags, &out);
        double e = 0.0, peak = 0.0;
        for (size_t i = out.size() / 2; i < out.size(); i++) {
            e += out[i] * out[i];
            peak = std::fmax(peak, std::fabs(out[i]));
        }
        rms = std::sqrt(e / (out.size() / 2));
        crest = peak / rms;
    };
    for (uint8_t wave : {static_cast<uint8_t>(kSaw), static_cast<uint8_t>(kTriangle), static_cast<uint8_t>(kSine)}) {
        for (uint8_t cutoff : {30, 60, 100}) {
            double rms0, crest0, rms50, crest50, rms100, crest100;
            measure(0, cutoff, wave, rms0, crest0);
            measure(50, cutoff, wave, rms50, crest50);
            measure(100, cutoff, wave, rms100, crest100);
            CHECK(rms100 > rms0 * 1.6, "wave %d cutoff %d: DRIVE 100 louder/denser %f vs %f", wave, cutoff, rms100, rms0);
            CHECK(rms50 > rms0 * 1.2 && rms50 < rms100, "wave %d cutoff %d: DRIVE 50 in between", wave, cutoff);
            CHECK(crest100 < crest0 * 0.8, "wave %d cutoff %d: DRIVE 100 squashes peaks %f vs %f", wave, cutoff, crest100, crest0);
            CHECK(rms100 < 1.2, "wave %d cutoff %d: DRIVE 100 stays in range %f", wave, cutoff, rms100);
        }
    }
}

static void testLfoShapesAndRates() {
    // Every shape stays in [-1, 1] and starts where documented; rates lock to the tempo.
    for (uint8_t s = 0; s < kLfoShapeCount; s++) {
        for (int i = 0; i < 1000; i++) {
            const float v = lfoValue(s, i / 1000.0f, 0.3f);
            CHECK(v >= -1.0001f && v <= 1.0001f, "shape %d in range at %d: %f", s, i, v);
        }
    }
    CHECK(std::fabs(lfoValue(kLfoSine, 0.0f, 0.0f)) < 1e-6f && std::fabs(lfoValue(kLfoTriangle, 0.0f, 0.0f)) < 1e-6f, "sine/triangle start at the centre");
    CHECK(lfoValue(kLfoSaw, 0.0f, 0.0f) == 1.0f && lfoValue(kLfoSquare, 0.0f, 0.0f) == 1.0f, "saw/square start at the top");
    CHECK(std::fabs(lfoValue(kLfoTriangle, 0.25f, 0.0f) - 1.0f) < 1e-6f && std::fabs(lfoValue(kLfoTriangle, 0.75f, 0.0f) + 1.0f) < 1e-6f, "triangle peaks");
    CHECK(lfoValue(kLfoRandom, 0.4f, -0.7f) == -0.7f, "random holds its value");

    // 1/4 at 120 BPM = 2 cycles per second; 1 BAR in 4/4 = 0.5; 1 BAR in 3/4 = 2/3.
    const float quarter = lfoIncrement(5, 120.0f, 4.0f, kFs) * kFs;
    const float bar44 = lfoIncrement(7, 120.0f, 4.0f, kFs) * kFs;
    const float bar34 = lfoIncrement(7, 120.0f, 3.0f, kFs) * kFs;
    CHECK(std::fabs(quarter - 2.0f) < 1e-3f, "1/4 at 120 = 2 Hz: %f", quarter);
    CHECK(std::fabs(bar44 - 0.5f) < 1e-3f && std::fabs(bar34 - (2.0f / 3.0f)) < 1e-3f, "bars follow MEASURE: %f %f", bar44, bar34);
    for (uint8_t r = 0; r < kLfoRateCount; r++) {
        const float inc = lfoIncrement(r, 300.0f, 16.0f, kFs);
        CHECK(inc > 0.0f && inc < 0.01f && std::isfinite(inc), "rate %d sane", r);
    }
    CHECK(std::isfinite(lfoIncrement(200, NAN, NAN, kFs)) && lfoIncrement(200, NAN, NAN, kFs) > 0.0f, "garbage in -> defaults");

    // advanceLfo wraps, picks a new random value per cycle, ignores bad increments.
    Voice v;
    const float before = v.lfoHold;
    CHECK(advanceLfo(v, 0.01f, 150) && v.lfoPhase >= 0.0f && v.lfoPhase < 1.0f, "wraps: %f", v.lfoPhase);
    CHECK(v.lfoHold != before && v.lfoHold >= -1.0f && v.lfoHold <= 1.0f, "new random value");
    const float phase = v.lfoPhase;
    CHECK(!advanceLfo(v, NAN, 32) && !advanceLfo(v, -1.0f, 32) && !advanceLfo(v, 2.0f, 32) && v.lfoPhase == phase, "bad increments ignored");
}

// Renders a held 110 Hz note for `blocks` blocks and returns the output.
static std::vector<float> renderHeld(const Params& p, float lfoInc, int blocks, bool retrig = false) {
    Voice v;
    NoteOn n = note(110.0f, 1000000);
    n.lfoRetrig = retrig;
    start(v, n);
    std::vector<float> out;
    float block[kBlock];
    for (int b = 0; b < blocks; b++) {
        render(v, p, block, kBlock, kFs, lfoInc);
        for (uint16_t i = 0; i < kBlock; i++) {
            CHECK(std::isfinite(block[i]), "finite LFO output");
            out.push_back(block[i]);
        }
    }
    return out;
}

static void testLfoTargets() {
    const float inc = lfoIncrement(5, 120.0f, 4.0f, kFs);  // 2 Hz
    const int blocks = static_cast<int>(kFs / kBlock);      // 1 s = 2 LFO cycles

    // Brightness (first difference) per 1/8 s window: with the LFO on CUTOFF it swings a lot,
    // with DEPTH 0 it stays flat.
    auto swing = [&](const Params& p) {
        const std::vector<float> out = renderHeld(p, inc, blocks);
        const size_t window = static_cast<size_t>(kFs / 8);
        double lo = 1e30, hi = 0.0;
        for (size_t w = 1; (w + 1) * window <= out.size(); w++) {
            double e = 0.0;
            for (size_t i = w * window + 1; i < (w + 1) * window; i++) e += std::fabs(out[i] - out[i - 1]);
            lo = std::fmin(lo, e);
            hi = std::fmax(hi, e);
        }
        return hi / lo;
    };
    Params p;
    p.cutoff = 25;
    p.envAmount = 0;
    p.drive = 0;
    p.lfoTarget = kLfoCutoff;
    p.lfoShape = kLfoSine;
    p.lfoDepth = 0;
    const double flat = swing(p);
    p.lfoDepth = 100;
    const double moving = swing(p);
    CHECK(flat < 1.1 && moving > 3.0, "CUTOFF LFO is audible: flat %f, moving %f", flat, moving);

    // LEVEL: tremolo between full and silent at depth 100 (envelope of |x| per window).
    Params t;
    t.wave = kSine;  // smooth by itself, so any big sample-to-sample step would be the tremolo's
    t.cutoff = 100;
    t.envAmount = 0;
    t.drive = 0;
    t.lfoTarget = kLfoLevel;
    t.lfoShape = kLfoSquare;
    t.lfoDepth = 100;
    const std::vector<float> trem = renderHeld(t, inc, blocks);
    const size_t window = static_cast<size_t>(kFs / 16);
    double lo = 1e30, hi = 0.0;
    for (size_t w = 1; (w + 1) * window <= trem.size(); w++) {
        double peak = 0.0;
        for (size_t i = w * window; i < (w + 1) * window; i++) peak = std::fmax(peak, std::fabs(trem[i]));
        lo = std::fmin(lo, peak);
        hi = std::fmax(hi, peak);
    }
    CHECK(lo < hi * 0.05, "LEVEL LFO chops: %f vs %f", lo, hi);
    // No clicks: the gain is ramped per sample, so the biggest step stays small.
    double step = 0.0;
    for (size_t i = 1; i < trem.size(); i++) step = std::fmax(step, std::fabs(trem[i] - trem[i - 1]));
    CHECK(step < 0.06, "tremolo has no click steps: %f", step);

    // PITCH: zero crossings per window vary (vibrato); depth 100 = up to an octave each way, and
    // the oscillator never exceeds Nyquist even at high notes.
    Params vib;
    vib.cutoff = 100;
    vib.envAmount = 0;
    vib.drive = 0;
    vib.wave = kSine;
    vib.lfoTarget = kLfoPitch;
    vib.lfoShape = kLfoSquare;
    vib.lfoDepth = 100;
    const std::vector<float> out = renderHeld(vib, inc, blocks);
    auto crossings = [&](size_t from, size_t to) {
        int c = 0;
        for (size_t i = from + 1; i < to; i++) c += ((out[i - 1] < 0.0f) != (out[i] < 0.0f));
        return c;
    };
    const size_t quarter = static_cast<size_t>(kFs / 4);  // square LFO at 2 Hz: up for 1/4 s, down for 1/4 s
    const int up = crossings(quarter / 4, quarter - (quarter / 4));
    const int down = crossings(quarter + (quarter / 4), (2 * quarter) - (quarter / 4));
    CHECK(up > down * 3, "PITCH LFO bends an octave each way: %d vs %d crossings", up, down);
    Voice hi8;
    start(hi8, note(15000.0f, 100000));
    float block[kBlock];
    for (int b = 0; b < 300; b++) {
        render(hi8, vib, block, kBlock, kFs, inc);
        for (uint16_t i = 0; i < kBlock; i++) CHECK(std::isfinite(block[i]), "vibrato on a very high note stays finite");
    }
}

static void testLfoRetrigAndFree() {
    // RETRIG = NOTE: every note starts the LFO at phase 0 (SAW DOWN = top = filter open).
    // FREE: the phase keeps going, also between notes (advanceLfo), and through a note's end.
    const float inc = lfoIncrement(5, 120.0f, 4.0f, kFs);
    Voice v;
    v.lfoPhase = 0.6f;
    NoteOn n = note(110.0f, 2000);
    n.lfoRetrig = true;
    start(v, n);
    CHECK(v.lfoPhase == 0.0f && v.lfoJump, "retrig restarts the LFO");
    Params p;
    p.lfoDepth = 100;
    p.lfoShape = kLfoSaw;
    float block[kBlock];
    render(v, p, block, kBlock, kFs, inc);
    CHECK(v.lfoSmooth > 0.95f, "retrig takes the new value at once: %f", v.lfoSmooth);

    Voice f;
    f.lfoPhase = 0.6f;
    NoteOn free = note(110.0f, 2000);
    start(f, free);
    CHECK(std::fabs(f.lfoPhase - 0.6f) < 1e-6f, "FREE keeps the phase on a new note");
    uint8_t flags = 0;
    for (int b = 0; (b < 2000) && !(flags & kVoiceEnded); b++) flags |= render(f, p, block, kBlock, kFs, inc);
    CHECK((flags & kVoiceEnded) && f.lfoPhase != 0.6f, "the LFO survived the end of the note: %f", f.lfoPhase);

    // A slide (legato) never retriggers.
    Voice s;
    start(s, note(110.0f, 100000));
    s.lfoPhase = 0.4f;
    NoteOn slide = note(220.0f, 2000);
    slide.legato = true;
    slide.lfoRetrig = true;
    start(s, slide);
    CHECK(std::fabs(s.lfoPhase - 0.4f) < 1e-6f, "slide keeps the LFO running");
}

static void testPackUnpack() {
    // Round trip of every field, old shorter records, sanitizing, isDefault.
    Params p;
    p.wave = kSine;
    p.cutoff = 1;
    p.resonance = 2;
    p.envAmount = 3;
    p.decay = 4;
    p.gate = 5;
    p.drive = 6;
    p.detune = 7;
    p.sub = 8;
    p.noise = 9;
    p.lfoShape = kLfoRandom;
    p.lfoRate = 10;
    p.lfoDepth = 77;
    p.lfoTarget = kLfoLevel;
    p.lfoRetrig = 1;
    p.attack = 70;
    p.ampDecay = 20;
    p.sustain = 40;
    p.release = 90;
    p.filterType = kHighPass;
    p.padHold = 1;
    uint8_t d[kPackedSize];
    pack(p, d);
    const Params q = unpack(d, kPackedSize);
    uint8_t e[kPackedSize];
    pack(q, e);
    bool same = true;
    for (uint8_t i = 0; i < kPackedSize; i++) same = same && (d[i] == e[i]);
    CHECK(same, "pack/unpack round trip");
    CHECK(!isDefault(p) && isDefault(Params{}), "isDefault");

    const Params v12 = unpack(d, 10);  // "SYN2" (and the 10 used values of "SYN3"): LFO off
    CHECK(v12.noise == 9 && v12.lfoDepth == 0 && v12.lfoShape == kLfoTriangle, "SYN2 record");
    const Params v11 = unpack(d, 7);   // "SYN1": DETUNE/SUB/NOISE keep defaults
    CHECK(v11.drive == 6 && v11.detune == 0 && v11.sub == 0 && v11.noise == 0, "SYN1 record");

    uint8_t junk[kPackedSize];
    for (uint8_t i = 0; i < kPackedSize; i++) junk[i] = 0xEE;
    const Params j = unpack(junk, kPackedSize);
    CHECK(j.wave == kSaw && j.cutoff == kMaxParam && j.lfoShape == kLfoTriangle && j.lfoRate == 5 && j.lfoDepth == kMaxParam &&
              j.lfoTarget == kLfoCutoff && j.lfoRetrig == 0,
          "junk is sanitized");
    Params lfoOnly;
    lfoOnly.lfoDepth = 30;
    CHECK(!isDefault(lfoOnly), "an LFO alone makes the sound non-default (file is saved)");
    Params holdOnly;
    holdOnly.padHold = 1;
    CHECK(!isDefault(holdOnly), "PAD HOLD alone is saved too");
    const Params v20 = unpack(d, 20);  // v1.13 test build files: no PAD HOLD byte -> off
    CHECK(v20.filterType == kHighPass && v20.padHold == 0, "20-value record");
    CHECK(j.padHold == 0, "junk PAD HOLD is sanitized");
}

// Level envelope of a rendered note: peak |x| per 5 ms window.
static std::vector<float> envelopeOf(const Params& p, uint32_t gateSamples, int blocks) {
    Voice v;
    start(v, note(220.0f, gateSamples));
    std::vector<float> out;
    float block[kBlock];
    for (int b = 0; b < blocks; b++) {
        render(v, p, block, kBlock, kFs, 0.0f);
        for (uint16_t i = 0; i < kBlock; i++) out.push_back(block[i]);
    }
    const size_t window = static_cast<size_t>(kFs * 0.005f);
    std::vector<float> env;
    for (size_t w = 0; (w + 1) * window <= out.size(); w++) {
        float peak = 0.0f;
        for (size_t i = w * window; i < (w + 1) * window; i++) peak = std::fmax(peak, std::fabs(out[i]));
        env.push_back(peak);
    }
    return env;
}

static void testAmpEnvelope() {
    // Defaults reproduce the old envelope: ~2 ms attack and ~12 ms release.
    CHECK(std::fabs(attackSeconds(Params{}.attack) - 0.002f) < 0.0005f, "default attack ~2 ms: %f", attackSeconds(Params{}.attack));
    CHECK(std::fabs(decaySeconds(Params{}.release) - 0.012f) < 0.001f, "default release ~12 ms: %f", decaySeconds(Params{}.release));
    CHECK(std::fabs(attackSeconds(0) - 0.001f) < 1e-5f && std::fabs(attackSeconds(100) - 3.0f) < 0.05f, "attack range");
    CHECK(std::fabs(decaySeconds(0) - 0.005f) < 1e-5f && std::fabs(decaySeconds(100) - 5.0f) < 0.1f, "decay/release range");

    Params p;
    p.cutoff = 100;
    p.envAmount = 0;
    p.drive = 0;
    // Slow ATTACK: quiet at the start, full after ~the attack time.
    p.attack = 60;  // ~120 ms
    std::vector<float> env = envelopeOf(p, 44100, 1000);
    CHECK(env[1] < env[40] * 0.2f, "slow attack starts quiet: %f vs %f", env[1], env[40]);
    // DECAY to SUSTAIN 30: the held level settles near 30 % of the peak.
    p.attack = 0;
    p.ampDecay = 30;  // ~40 ms
    p.sustain = 30;
    env = envelopeOf(p, 44100, 1000);
    float peak = 0.0f;
    for (float e : env) peak = std::fmax(peak, e);
    CHECK(std::fabs(env[60] / peak - 0.3f) < 0.06f, "sustain level ~30 %%: %f", env[60] / peak);
    // SUSTAIN 0: a pluck that dies away while the note is still held.
    p.sustain = 0;
    env = envelopeOf(p, 44100, 1000);
    CHECK(env[60] < peak * 0.02f, "sustain 0 dies away: %f", env[60]);
    // Long RELEASE: still sounding 200 ms after the gate; short: silent.
    p.sustain = 100;
    p.release = 70;  // ~630 ms
    env = envelopeOf(p, 4410, 1000);  // 100 ms gate
    CHECK(env[60] > peak * 0.3f, "long release rings on: %f", env[60]);
    p.release = 0;
    env = envelopeOf(p, 4410, 1000);
    CHECK(env[60] < peak * 0.001f, "short release stops: %f", env[60]);
}

static void testFilterTypes() {
    // A 1 kHz cutoff: LP lets 110 Hz through and cuts 4 kHz; HP the opposite; BP cuts both
    // relative to its 1 kHz centre. Measured with sine tones (no harmonics).
    auto level = [](uint8_t type, float hz) {
        Params p;
        p.wave = kSine;
        p.cutoff = 56;  // ~1 kHz
        p.resonance = 0;
        p.envAmount = 0;
        p.drive = 0;
        p.filterType = type;
        Voice v;
        start(v, note(hz, 1000000));
        std::vector<float> out;
        float block[kBlock];
        for (int b = 0; b < 300; b++) {
            render(v, p, block, kBlock, kFs, 0.0f);
            for (uint16_t i = 0; i < kBlock; i++) {
                CHECK(std::isfinite(block[i]), "finite filter type %d", type);
                out.push_back(block[i]);
            }
        }
        double e = 0.0;
        for (size_t i = out.size() / 2; i < out.size(); i++) e += out[i] * out[i];
        return std::sqrt(e / (out.size() / 2));
    };
    const double lpLow = level(kLowPass24, 110.0f), lpHigh = level(kLowPass24, 4000.0f);
    const double hpLow = level(kHighPass, 110.0f), hpHigh = level(kHighPass, 4000.0f);
    const double bpLow = level(kBandPass, 110.0f), bpMid = level(kBandPass, 1000.0f), bpHigh = level(kBandPass, 4000.0f);
    const double lp12High = level(kLowPass12, 4000.0f);
    CHECK(lpHigh < lpLow * 0.05, "LP24 cuts highs: %f vs %f", lpHigh, lpLow);
    CHECK(lp12High > lpHigh * 3.0 && lp12High < lpLow * 0.5, "LP12 is gentler than LP24: %f", lp12High);
    CHECK(hpLow < hpHigh * 0.1, "HP cuts lows: %f vs %f", hpLow, hpHigh);
    CHECK(bpMid > bpLow * 2.0 && bpMid > bpHigh * 2.0, "BP keeps the middle: %f %f %f", bpLow, bpMid, bpHigh);
    CHECK(hpHigh > 0.1 && bpMid > 0.1, "HP/BP are not silent: %f %f", hpHigh, bpMid);
    // Every type at maximum resonance and drive stays bounded.
    for (uint8_t t = 0; t < kFilterTypeCount; t++) {
        Params p;
        p.filterType = t;
        p.resonance = 100;
        p.drive = 100;
        p.envAmount = 100;
        Voice v;
        start(v, note(110.0f, 44100));
        uint8_t flags = 0;
        CHECK(run(v, p, 1400, flags) < 3.0f, "type %d bounded", t);
    }
}

static void testPresetsAndRandom() {
    // Every factory sound: a name that fits the display, sane values, audible and bounded.
    CHECK(kFactoryPresetCount >= 30, "at least 30 factory sounds: %d", kFactoryPresetCount);
    CHECK(isDefault(kFactoryPresets[0].params), "the first preset is the default sound");
    for (uint8_t i = 0; i < kFactoryPresetCount; i++) {
        const Preset& pr = kFactoryPresets[i];
        CHECK(std::strlen(pr.name) >= 3 && std::strlen(pr.name) <= 10, "preset %d name length: %s", i, pr.name);
        for (uint8_t j = 0; j < i; j++) CHECK(std::strcmp(pr.name, kFactoryPresets[j].name) != 0, "preset %d name unique", i);
        uint8_t a[kPackedSize], b[kPackedSize];
        pack(pr.params, a);
        pack(sanitize(pr.params), b);
        CHECK(std::memcmp(a, b, kPackedSize) == 0, "preset %s is already sane", pr.name);
        Voice v;
        start(v, note(110.0f, 20000));
        uint8_t flags = 0;
        const float peak = run(v, pr.params, 1000, flags);
        CHECK(peak > 0.01f && peak < 3.0f, "preset %s audible and bounded: %f", pr.name, peak);
    }

    // Random sounds: always sane, audible, never WAVE = SAMPLE, varied, deterministic per seed.
    uint32_t seed = 12345;
    int waves[kWaveCount] = {};
    int types[kFilterTypeCount] = {};
    for (int n = 0; n < 2000; n++) {
        const Params p = randomSound(seed);
        uint8_t a[kPackedSize], b[kPackedSize];
        pack(p, a);
        pack(sanitize(p), b);
        CHECK(std::memcmp(a, b, kPackedSize) == 0, "random sound %d is sane", n);
        CHECK(p.wave != kSample, "random sound never needs a sample");
        waves[p.wave]++;
        types[p.filterType]++;
        if (n < 200) {
            Voice v;
            start(v, note(110.0f, 20000));
            uint8_t flags = 0;
            const float peak = run(v, p, 600, flags);
            CHECK(peak > 0.005f && peak < 3.0f, "random sound %d audible and bounded: %f", n, peak);
        }
    }
    for (uint8_t w = 0; w < kSample; w++) CHECK(waves[w] > 50, "wave %d used by the generator: %d", w, waves[w]);
    for (uint8_t t = 0; t < kFilterTypeCount; t++) CHECK(types[t] > 20, "filter type %d used: %d", t, types[t]);
    uint32_t s1 = 99, s2 = 99;
    uint8_t a[kPackedSize], b[kPackedSize];
    pack(randomSound(s1), a);
    pack(randomSound(s2), b);
    CHECK(std::memcmp(a, b, kPackedSize) == 0, "same seed, same sound");

    // Variations: small steps, stay sane, something changes.
    Params base = kFactoryPresets[1].params;
    uint32_t ms = 7;
    for (int n = 0; n < 1000; n++) {
        Params m = base;
        const uint8_t changed = mutateSound(m, ms);
        uint8_t x[kPackedSize], y[kPackedSize];
        pack(base, x);
        pack(m, y);
        int differ = 0;
        for (uint8_t i = 0; i < kPackedSize; i++) {
            if (x[i] != y[i]) {
                differ++;
                if (i != 0) CHECK(std::abs(static_cast<int>(x[i]) - static_cast<int>(y[i])) <= 15, "variation step %d small", i);
            }
        }
        CHECK(differ == changed && differ <= 4, "variation reports its changes: %d vs %d", differ, changed);
        CHECK(m.wave != kSample, "variation never picks SAMPLE");
    }
}

int main() {
    testAmpEnvelope();
    testFilterTypes();
    testPresetsAndRandom();
    testLfoShapesAndRates();
    testLfoTargets();
    testLfoRetrigAndFree();
    testPackUnpack();
    testDriveIsAudible();
    testDetuneIsAudible();
    testDetuneSubNoise();
    testSampleTable();
    testLegato();
    testOscillators();
    testPolyBlepReducesAliasing();
    testFilterStability();
    testFilterResponse();
    testEnvelopeAndGate();
    testGlide();
    testVelocity();
    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
