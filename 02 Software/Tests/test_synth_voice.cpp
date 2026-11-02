// Host unit tests for SYNTH layers: Core/Library/Global/SynthVoice.h.
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_synth_voice.cpp -o test_synth_voice && ./test_synth_voice
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cmath>
#include <cstdio>
#include <vector>

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

int main() {
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
