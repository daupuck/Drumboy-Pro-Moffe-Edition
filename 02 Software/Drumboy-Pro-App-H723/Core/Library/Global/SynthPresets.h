#ifndef __SYNTH_PRESETS_H
#define __SYNTH_PRESETS_H

// SYNTH layers: factory sounds, a random sound generator and small variations (v1.13).
// Pure code without hardware dependencies: host-tested in 02 Software/Tests/test_synth_voice.cpp.
//
// Names are at most 10 characters (one value field on the display). They are meant to make you
// smile; the comment next to each says what it really is.

#include <stdint.h>

#include "SynthVoice.h"

namespace synth {

// Builds a sound from all its values in file order (see pack()).
constexpr Params makeParams(uint8_t wave, uint8_t cutoff, uint8_t resonance, uint8_t envAmount, uint8_t decay, uint8_t gate,
                            uint8_t drive, uint8_t detune, uint8_t sub, uint8_t noise, uint8_t lfoShape, uint8_t lfoRate,
                            uint8_t lfoDepth, uint8_t lfoTarget, uint8_t lfoRetrig, uint8_t attack, uint8_t ampDecay,
                            uint8_t sustain, uint8_t release, uint8_t filterType) {
    Params p{};
    p.wave = wave;
    p.cutoff = cutoff;
    p.resonance = resonance;
    p.envAmount = envAmount;
    p.decay = decay;
    p.gate = gate;
    p.drive = drive;
    p.detune = detune;
    p.sub = sub;
    p.noise = noise;
    p.lfoShape = lfoShape;
    p.lfoRate = lfoRate;
    p.lfoDepth = lfoDepth;
    p.lfoTarget = lfoTarget;
    p.lfoRetrig = lfoRetrig;
    p.attack = attack;
    p.ampDecay = ampDecay;
    p.sustain = sustain;
    p.release = release;
    p.filterType = filterType;
    return p;
}

// v2.05 MOD page on top of a sound: WAVE = FM with ratio (index into kFmRatios), depth and decay;
// a pitch envelope; a filter envelope attack.
constexpr Params withFm(Params p, uint8_t ratio, uint8_t amount, uint8_t decay) {
    p.wave = kFm;
    p.fmRatio = ratio;
    p.fmAmount = amount;
    p.fmDecay = decay;
    return p;
}
constexpr Params withPitchEnv(Params p, uint8_t amount, uint8_t decay) {
    p.pitchEnv = amount;
    p.pitchDecay = decay;
    return p;
}
constexpr Params withFilterAttack(Params p, uint8_t attack) {
    p.filterAttack = attack;
    return p;
}

struct Preset {
    const char* name;
    Params params;
};

// Short names for readability below. LFO rates: 1 = 1/16, 3 = 1/8, 5 = 1/4, 7 = 1 bar, 9 = 4 bars.
constexpr uint8_t pSAW = kSaw, pSQR = kSquare, pTRI = kTriangle, pSIN = kSine;
constexpr uint8_t pLP24 = kLowPass24, pLP12 = kLowPass12, pBP = kBandPass, pHP = kHighPass;
constexpr uint8_t pLSIN = kLfoSine, pLTRI = kLfoTriangle, pLSAW = kLfoSaw, pLSQR = kLfoSquare, pLRND = kLfoRandom;
constexpr uint8_t pCUT = kLfoCutoff, pPIT = kLfoPitch, pLEV = kLfoLevel;

//                                       wave cut res env dec gate drv det sub noi | lfo: shp  rate dep tgt  rtg | att adec sus rel  filter
inline constexpr Preset kFactoryPresets[] = {
    // Alphabetical (as the PRESET knob shows them); USER 01..16 follow after the last one.
    // EMPTY MIND is the default sound (what a new SYNTH layer starts with).
    {"808 BOOM",   withPitchEnv(makeParams(pSIN, 100, 0, 0, 30, 30, 15, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 60, 0, 45, pLP24), 50, 22)}, // 808-style kick (v2.05 pitch envelope)
    {"ACID TRIP",  makeParams(pSAW, 20, 75, 75, 25, 40, 40, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 10, 50, 100, 13, pLP24)},     // 303-style acid bass
    {"ALIEN SPA",  makeParams(pTRI, 50, 60, 0, 40, 100, 0, 40, 0, 10, pLSIN, 8, 40, pPIT, 0, 70, 60, 90, 80, pBP)},      // warbling band-pass pad
    {"BAD PENNY",  makeParams(pSQR, 45, 30, 60, 20, 40, 20, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 40, 0, 30, pLP12)},        // hollow square pluck
    {"BANSHEE",    makeParams(pSAW, 55, 85, 50, 45, 85, 80, 40, 0, 0, pLTRI, 5, 0, pCUT, 0, 10, 50, 100, 30, pLP24)},    // screaming resonant lead
    {"BEE SWARM",  makeParams(pSAW, 50, 40, 0, 30, 100, 30, 100, 0, 20, pLRND, 0, 30, pPIT, 0, 30, 50, 100, 40, pBP)},   // buzzing jittery drone
    {"BELL TOWER", withFm(makeParams(pSIN, 100, 0, 0, 30, 30, 0, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 85, 0, 85, pLP24), 8, 55, 60)}, // FM church bell (3.5:1)
    {"BIG FUZZ",   makeParams(pSQR, 35, 20, 40, 40, 80, 100, 20, 50, 0, pLTRI, 5, 0, pCUT, 0, 5, 50, 100, 20, pLP24)},   // fuzz bass
    {"BLIP BLOOP", makeParams(pSIN, 70, 40, 60, 10, 20, 0, 0, 0, 0, pLSAW, 1, 50, pPIT, 1, 0, 25, 0, 15, pLP24)},        // pitch-drop blip
    {"BOOMSTICK",  makeParams(pSIN, 100, 0, 0, 30, 10, 20, 0, 0, 0, pLSAW, 1, 100, pPIT, 1, 0, 35, 0, 20, pLP24)},       // synth kick (pitch sweep)
    {"BRASS GOD",  makeParams(pSAW, 35, 15, 55, 45, 80, 30, 25, 0, 0, pLTRI, 5, 0, pCUT, 0, 45, 60, 80, 35, pLP24)},     // synth brass
    {"BUBBLEGUM",  makeParams(pSQR, 65, 30, 40, 30, 60, 10, 30, 0, 0, pLSIN, 4, 20, pPIT, 0, 5, 50, 80, 30, pLP12)},     // sweet pop lead
    {"CAT WALK",   makeParams(pTRI, 60, 20, 50, 25, 40, 10, 15, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 45, 0, 35, pLP24)},       // soft triangle pluck
    {"CHURCH BOI", makeParams(pSQR, 60, 0, 0, 30, 90, 0, 0, 60, 0, pLSIN, 2, 15, pLEV, 0, 5, 50, 100, 20, pLP24)},       // organ
    {"CLOUD NINE", makeParams(pSAW, 55, 10, 10, 60, 100, 0, 85, 0, 5, pLTRI, 9, 25, pCUT, 0, 80, 70, 90, 90, pLP12)},    // airy pad
    {"COCONUTS",   makeParams(pTRI, 70, 10, 20, 20, 20, 0, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 45, 0, 40, pLP24)},         // marimba
    {"COIN GET",   makeParams(pSQR, 100, 0, 0, 30, 50, 0, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 25, 0, 15, pLP24)},          // chiptune pluck
    {"COWBELL!",   makeParams(pSQR, 75, 50, 0, 30, 10, 20, 35, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 25, 0, 20, pBP)},          // more cowbell
    {"DAD STEP",   makeParams(pSAW, 25, 70, 0, 30, 100, 70, 40, 60, 0, pLSQR, 3, 80, pCUT, 1, 5, 50, 100, 15, pLP24)},   // square-wave wobble bass
    {"DARK LORD",  makeParams(pSAW, 45, 20, 10, 50, 95, 35, 90, 40, 0, pLSIN, 8, 25, pCUT, 0, 15, 50, 100, 30, pLP24)},  // reese bass
    {"DEEP SPACE", makeParams(pSIN, 30, 50, 0, 80, 100, 10, 50, 40, 20, pLSIN, 10, 40, pCUT, 0, 90, 70, 100, 95, pLP24)}, // slow dark drone
    {"DING DONG",  makeParams(pSIN, 100, 0, 0, 30, 30, 0, 25, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 80, 0, 80, pLP24)},         // bell
    {"DISCO DUCK", makeParams(pSAW, 45, 50, 60, 20, 50, 20, 0, 20, 0, pLTRI, 5, 0, pCUT, 0, 0, 40, 30, 20, pBP)},        // funky quacking bass
    {"DIZZY BOT",  makeParams(pSQR, 55, 40, 20, 30, 80, 20, 0, 0, 0, pLSIN, 1, 50, pPIT, 0, 5, 50, 100, 20, pLP24)},     // fast-vibrato robot
    {"DRAGONROAR", makeParams(pSAW, 25, 55, 65, 40, 90, 90, 60, 70, 15, pLSIN, 6, 20, pPIT, 1, 20, 60, 70, 45, pLP24)},  // huge distorted bass/lead
    {"DX PIANO",   withFm(makeParams(pSIN, 100, 0, 0, 30, 50, 0, 15, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 70, 20, 40, pLP24), 1, 45, 35)}, // DX-style electric piano (1:1)
    {"EARTHQUAKE", makeParams(pSIN, 40, 0, 10, 30, 80, 10, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 15, 50, 100, 25, pLP24)},      // clean sub bass
    {"EMPTY MIND", makeParams(pSAW, 35, 40, 60, 35, 50, 25, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 10, 50, 100, 13, pLP24)},     // the default sound
    {"EPIC MOVIE", makeParams(pSAW, 60, 5, 0, 30, 100, 0, 80, 0, 0, pLSIN, 3, 20, pPIT, 0, 70, 50, 100, 75, pLP12)},     // strings
    {"FAIRY DUST", makeParams(pSIN, 100, 0, 0, 30, 20, 0, 40, 0, 15, pLRND, 0, 25, pPIT, 0, 0, 40, 0, 70, pHP)},         // sparkles
    {"FM BRUISER", withFm(makeParams(pSIN, 60, 20, 30, 30, 50, 30, 0, 30, 0, pLTRI, 5, 0, pCUT, 0, 0, 50, 80, 15, pLP24), 0, 60, 25)}, // punchy FM bass (0.5:1)
    {"FOGHORN",    makeParams(pSAW, 20, 10, 10, 50, 100, 40, 30, 80, 0, pLTRI, 5, 0, pCUT, 0, 40, 60, 100, 50, pLP24)},  // low horn
    {"GHOST TOWN", makeParams(pTRI, 40, 60, 0, 40, 100, 0, 20, 0, 40, pLSIN, 9, 50, pCUT, 0, 80, 60, 80, 90, pBP)},      // eerie windy pad
    {"GLASS HARP", makeParams(pSIN, 90, 20, 0, 30, 30, 0, 10, 0, 0, pLSIN, 5, 10, pPIT, 0, 0, 85, 0, 85, pLP12)},        // glassy bell
    {"GOLD RUSH",  makeParams(pSAW, 70, 30, 40, 35, 70, 20, 60, 0, 0, pLSIN, 4, 15, pPIT, 0, 10, 60, 70, 45, pLP12)},    // bright detuned lead
    {"GONG SHOW",  withFm(makeParams(pSIN, 100, 0, 0, 30, 30, 0, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 95, 0, 95, pLP24), 2, 70, 75)}, // gong (1.41:1, inharmonic)
    {"HONKY BONK", makeParams(pSQR, 50, 15, 40, 30, 40, 10, 30, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 60, 0, 45, pLP24)},       // out-of-tune piano
    {"HYPERDRIVE", makeParams(pSAW, 40, 60, 80, 30, 80, 90, 50, 30, 0, pLSAW, 3, 50, pCUT, 1, 0, 50, 100, 25, pLP24)},   // aggressive driven lead
    {"ICE PALACE", withFm(makeParams(pSIN, 100, 0, 0, 30, 40, 0, 20, 0, 0, pLSIN, 5, 10, pPIT, 0, 0, 80, 0, 85, pLP24), 6, 35, 55)}, // crystal (2.76:1)
    {"JELLYFISH",  makeParams(pTRI, 45, 70, 30, 50, 100, 10, 30, 0, 0, pLSIN, 7, 40, pCUT, 0, 60, 60, 80, 80, pLP12)},   // wobbly soft pad
    {"KALIMBA",    withFm(makeParams(pSIN, 100, 0, 0, 30, 30, 0, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 45, 0, 40, pLP24), 10, 25, 15)}, // thumb piano (5:1, short)
    {"KAZOO KING", makeParams(pSQR, 55, 60, 20, 30, 80, 40, 10, 0, 10, pLSIN, 4, 15, pPIT, 0, 10, 50, 100, 15, pBP)},    // kazoo
    {"LASER CAT",  makeParams(pSAW, 85, 80, 0, 30, 30, 30, 0, 0, 0, pLSAW, 2, 80, pPIT, 1, 0, 30, 0, 20, pLP24)},        // slower laser
    {"LOG DRUM",   withPitchEnv(withFm(makeParams(pSIN, 100, 0, 0, 30, 20, 0, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 40, 0, 35, pLP24), 4, 30, 15), 12, 20)}, // amapiano log drum
    {"LOUNGE LIZ", makeParams(pTRI, 55, 10, 30, 40, 70, 10, 25, 0, 0, pLSIN, 6, 15, pLEV, 0, 5, 75, 30, 55, pLP24)},     // electric piano with a little tremolo
    {"METAL GURU", withFm(makeParams(pSIN, 100, 0, 0, 30, 20, 30, 0, 0, 10, pLTRI, 5, 0, pCUT, 0, 0, 40, 0, 40, pHP), 8, 90, 30)}, // metallic hit (3.5:1, deep)
    {"MOON BASS",  makeParams(pTRI, 35, 30, 40, 30, 70, 30, 20, 50, 0, pLTRI, 5, 0, pCUT, 0, 5, 50, 90, 20, pLP24)},     // round warm bass
    {"MORDOR",     makeParams(pTRI, 30, 30, 20, 80, 100, 20, 60, 30, 0, pLSIN, 10, 20, pCUT, 0, 85, 70, 90, 90, pLP24)}, // dark pad
    {"NINJA STAR", makeParams(pSQR, 90, 30, 0, 30, 10, 0, 50, 0, 50, pLSAW, 1, 100, pCUT, 1, 0, 20, 0, 25, pHP)},        // swoosh
    {"NOODLE ARM", makeParams(pSAW, 50, 40, 30, 40, 90, 20, 30, 0, 0, pLSIN, 3, 25, pPIT, 0, 30, 50, 100, 40, pLP24)},   // wobbly slow lead
    {"OCTOPUS",    makeParams(pSAW, 30, 80, 70, 30, 60, 30, 40, 0, 0, pLRND, 3, 40, pCUT, 0, 0, 50, 60, 30, pLP12)},     // squelchy random-filter bass
    {"PANIC BOT",  makeParams(pSQR, 60, 70, 0, 30, 60, 40, 0, 0, 0, pLSQR, 0, 90, pPIT, 0, 0, 50, 100, 15, pLP24)},      // alarm trill
    {"PEW PEW",    makeParams(pSQR, 80, 70, 0, 30, 30, 20, 0, 0, 0, pLSAW, 1, 100, pCUT, 1, 0, 30, 0, 20, pLP24)},       // laser zap
    {"PIXEL HERO", makeParams(pSQR, 100, 0, 0, 30, 50, 0, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 50, 100, 5, pLP24)},         // chiptune square
    {"R2 DEEDOO",  makeParams(pSQR, 60, 50, 30, 30, 50, 20, 0, 0, 0, pLRND, 1, 60, pCUT, 0, 5, 50, 100, 20, pLP24)},     // sample & hold bleeps
    {"ROBO CHOIR", makeParams(pSAW, 55, 50, 0, 30, 100, 10, 70, 0, 0, pLSIN, 7, 15, pCUT, 0, 65, 50, 100, 70, pBP)},    // vocal-ish band-pass pad
    {"RUBBERDUCK", makeParams(pSQR, 25, 40, 70, 15, 30, 30, 0, 30, 0, pLTRI, 5, 0, pCUT, 0, 5, 35, 0, 20, pLP24)},       // plucked square bass
    {"SAWSOME",    makeParams(pSAW, 65, 25, 30, 40, 80, 30, 50, 0, 0, pLSIN, 3, 25, pPIT, 0, 15, 50, 100, 35, pLP24)},   // detuned saw lead
    {"SEASICK",    makeParams(pTRI, 70, 10, 0, 30, 100, 0, 30, 0, 0, pLSQR, 2, 80, pLEV, 0, 20, 50, 100, 30, pLP24)},    // choppy tremolo keys
    {"SLOW BLOOM", withFilterAttack(makeParams(pSAW, 25, 30, 70, 70, 100, 10, 60, 0, 0, pLTRI, 5, 0, pCUT, 0, 50, 60, 90, 70, pLP24), 75)}, // pad that blooms open (filter attack)
    {"SNARE WARS", makeParams(pTRI, 75, 20, 30, 15, 10, 30, 0, 0, 90, pLTRI, 5, 0, pCUT, 0, 0, 30, 0, 25, pHP)},        // noise snare
    {"SPACE TAXI", makeParams(pSAW, 45, 40, 30, 40, 90, 20, 60, 20, 0, pLTRI, 7, 35, pCUT, 0, 20, 60, 90, 50, pLP24)},   // sweeping synth-pop lead
    {"SQUARE GUY", makeParams(pSQR, 60, 20, 25, 40, 70, 20, 20, 0, 0, pLTRI, 5, 0, pCUT, 0, 10, 50, 100, 30, pLP24)},    // square lead
    {"STARGATE",   makeParams(pSAW, 25, 60, 0, 30, 100, 10, 60, 0, 0, pLTRI, 10, 70, pCUT, 0, 70, 50, 100, 85, pLP24)},  // slow filter-sweep pad
    {"STORMBORN",  makeParams(pSIN, 50, 70, 0, 30, 100, 0, 0, 0, 100, pLSIN, 8, 50, pCUT, 0, 80, 50, 100, 85, pBP)},    // wind
    {"SUNNY DAY",  makeParams(pTRI, 70, 10, 20, 30, 70, 0, 30, 0, 0, pLSIN, 5, 10, pLEV, 0, 15, 50, 90, 40, pLP12)},     // bright gentle keys
    {"THICC BASS", makeParams(pSAW, 30, 30, 50, 35, 60, 50, 40, 60, 0, pLTRI, 5, 0, pCUT, 0, 10, 50, 100, 15, pLP24)},   // fat detuned bass + sub
    {"TOM TOM",    withPitchEnv(makeParams(pSIN, 100, 0, 0, 30, 20, 10, 0, 0, 5, pLTRI, 5, 0, pCUT, 0, 0, 45, 0, 35, pLP24), 25, 35)}, // synth tom
    {"TOY PIANO",  makeParams(pSQR, 85, 10, 30, 20, 30, 0, 15, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 40, 0, 35, pLP12)},        // tinkly toy keys
    {"TSS TSS",    makeParams(pSIN, 88, 10, 0, 15, 5, 0, 0, 0, 100, pLTRI, 5, 0, pCUT, 0, 0, 20, 0, 15, pHP)},          // closed hi-hat
    {"TUMBLEWEED", makeParams(pSIN, 80, 0, 0, 30, 90, 0, 0, 0, 10, pLSIN, 3, 30, pPIT, 0, 40, 50, 100, 50, pLP24)},      // whistle with vibrato
    {"UFO LANDED", makeParams(pSIN, 60, 40, 0, 30, 100, 0, 0, 0, 0, pLSIN, 2, 60, pPIT, 0, 40, 50, 100, 70, pLP24)},     // flying-saucer warble
    {"VIKING",     makeParams(pSAW, 30, 25, 40, 50, 100, 50, 70, 60, 5, pLTRI, 5, 0, pCUT, 0, 50, 60, 90, 60, pLP24)},   // epic war horn
    {"WAH WAH",    withFilterAttack(makeParams(pSAW, 20, 65, 75, 40, 60, 20, 20, 0, 0, pLTRI, 5, 0, pCUT, 0, 5, 50, 100, 20, pLP24), 45)}, // slow wah opening on every note
    {"WARM HUG",   makeParams(pSAW, 40, 15, 15, 70, 100, 10, 70, 0, 0, pLTRI, 9, 30, pCUT, 0, 75, 70, 80, 80, pLP12)},   // warm pad
    {"WUBZILLA",   makeParams(pSAW, 30, 60, 0, 30, 100, 60, 30, 50, 0, pLSIN, 3, 70, pCUT, 1, 10, 50, 100, 20, pLP24)},  // wobble bass
    {"ZAP ATTACK", withPitchEnv(makeParams(pSQR, 80, 40, 0, 30, 20, 20, 0, 0, 0, pLTRI, 5, 0, pCUT, 0, 0, 30, 0, 20, pLP24), 100, 18)}, // laser zap (pitch envelope)
    {"ZEN PLUCK",  makeParams(pSAW, 30, 35, 80, 20, 50, 20, 20, 0, 0, pLTRI, 5, 0, pCUT, 0, 5, 40, 0, 40, pLP24)},       // soft synth pluck
    {"ZOMBIE",     makeParams(pSAW, 30, 50, 20, 40, 100, 60, 90, 30, 30, pLSIN, 8, 30, pPIT, 0, 60, 60, 80, 70, pLP24)}, // groaning undead pad
};
constexpr uint8_t kFactoryPresetCount = sizeof(kFactoryPresets) / sizeof(kFactoryPresets[0]);
constexpr uint8_t kUserPresetCount = 16;  // USER 01..16, saved on the SD card

// Index of the default sound (EMPTY MIND) in the list above.
inline int16_t defaultPresetIndex() {
    for (uint8_t i = 0; i < kFactoryPresetCount; i++) {
        if (isDefault(kFactoryPresets[i].params)) return i;
    }
    return 0;
}

// Same sound? Compares everything that is stored with a sound except PAD HOLD (a playing
// setting that presets keep as they are).
inline bool sameSound(const Params& a, const Params& b) {
    uint8_t x[kPackedSize + kExtSize];
    uint8_t y[kPackedSize + kExtSize];
    pack(a, x);
    pack(b, y);
    packExt(a, x + kPackedSize);  // v2.05: the MOD page counts too
    packExt(b, y + kPackedSize);
    for (uint8_t i = 0; i < kPackedSize + kExtSize; i++) {
        if ((i != 20) && (x[i] != y[i])) return false;  // 20 = padHold
    }
    return true;
}

// Factory preset with exactly this sound, or -1.
inline int16_t findFactoryPreset(const Params& p) {
    for (uint8_t i = 0; i < kFactoryPresetCount; i++) {
        if (sameSound(p, kFactoryPresets[i].params)) return i;
    }
    return -1;
}

// xorshift32: the same small generator as the melody generator (never returns 0 for seed != 0).
inline uint32_t soundRandom(uint32_t& seed) {
    if (seed == 0) seed = 0x9E3779B9u;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

inline uint8_t randomBetween(uint32_t& seed, uint8_t lo, uint8_t hi) {
    return static_cast<uint8_t>(lo + (soundRandom(seed) % static_cast<uint32_t>(hi - lo + 1)));
}

inline bool chance(uint32_t& seed, uint8_t percent) { return (soundRandom(seed) % 100) < percent; }

// A new random sound that still makes musical sense: first a family (bass, lead, pluck, pad or
// effect), then values inside that family's useful ranges. Never WAVE = SAMPLE.
inline Params randomSound(uint32_t& seed) {
    Params p;
    const uint8_t family = randomBetween(seed, 0, 4);
    const uint8_t waves[6] = {kSaw, kSaw, kSquare, kTriangle, kSine, kSaw};
    p.wave = waves[soundRandom(seed) % 6];
    p.lfoDepth = 0;
    p.noise = 0;
    switch (family) {
        case 0:  // bass
            if (p.wave == kTriangle) p.wave = kSaw;
            p.cutoff = randomBetween(seed, 15, 40);
            p.resonance = randomBetween(seed, 20, 80);
            p.envAmount = randomBetween(seed, 40, 85);
            p.decay = randomBetween(seed, 10, 45);
            p.gate = randomBetween(seed, 30, 70);
            p.drive = randomBetween(seed, 20, 70);
            p.detune = chance(seed, 40) ? randomBetween(seed, 10, 60) : 0;
            p.sub = chance(seed, 50) ? randomBetween(seed, 20, 70) : 0;
            p.attack = randomBetween(seed, 0, 15);
            p.ampDecay = randomBetween(seed, 30, 60);
            p.sustain = chance(seed, 30) ? randomBetween(seed, 0, 20) : randomBetween(seed, 60, 100);
            p.release = randomBetween(seed, 10, 25);
            p.filterType = kLowPass24;
            break;
        case 1:  // lead
            p.cutoff = randomBetween(seed, 45, 75);
            p.resonance = randomBetween(seed, 10, 60);
            p.envAmount = randomBetween(seed, 10, 50);
            p.decay = randomBetween(seed, 30, 60);
            p.gate = randomBetween(seed, 60, 95);
            p.drive = randomBetween(seed, 10, 60);
            p.detune = randomBetween(seed, 20, 70);
            if (chance(seed, 40)) {  // vibrato
                p.lfoShape = kLfoSine;
                p.lfoRate = randomBetween(seed, 3, 5);
                p.lfoDepth = randomBetween(seed, 15, 30);
                p.lfoTarget = kLfoPitch;
            }
            p.attack = randomBetween(seed, 5, 35);
            p.sustain = randomBetween(seed, 70, 100);
            p.release = randomBetween(seed, 20, 45);
            p.filterType = chance(seed, 70) ? kLowPass24 : kLowPass12;
            break;
        case 2:  // pluck / keys
            p.cutoff = randomBetween(seed, 25, 60);
            p.resonance = randomBetween(seed, 10, 60);
            p.envAmount = randomBetween(seed, 50, 90);
            p.decay = randomBetween(seed, 10, 35);
            p.gate = randomBetween(seed, 30, 60);
            p.drive = randomBetween(seed, 0, 40);
            p.detune = chance(seed, 50) ? randomBetween(seed, 10, 40) : 0;
            p.attack = randomBetween(seed, 0, 10);
            p.ampDecay = randomBetween(seed, 30, 70);
            p.sustain = randomBetween(seed, 0, 20);
            p.release = randomBetween(seed, 25, 60);
            p.filterType = kLowPass24;
            break;
        case 3:  // pad
            if ((p.wave == kSquare) || (p.wave == kSine)) p.wave = chance(seed, 50) ? kSaw : kTriangle;
            p.cutoff = randomBetween(seed, 30, 60);
            p.resonance = randomBetween(seed, 5, 40);
            p.envAmount = randomBetween(seed, 0, 30);
            p.decay = randomBetween(seed, 50, 90);
            p.gate = 100;
            p.drive = randomBetween(seed, 0, 25);
            p.detune = randomBetween(seed, 50, 90);
            if (chance(seed, 60)) {
                p.lfoShape = chance(seed, 50) ? kLfoTriangle : kLfoSine;
                p.lfoRate = randomBetween(seed, 7, 10);
                p.lfoDepth = randomBetween(seed, 20, 50);
                p.lfoTarget = kLfoCutoff;
            }
            p.attack = randomBetween(seed, 60, 85);
            p.ampDecay = randomBetween(seed, 60, 80);
            p.sustain = randomBetween(seed, 60, 100);
            p.release = randomBetween(seed, 65, 90);
            p.filterType = chance(seed, 20) ? kBandPass : (chance(seed, 50) ? kLowPass12 : kLowPass24);
            break;
        default:  // effect
            p.cutoff = randomBetween(seed, 30, 90);
            p.resonance = randomBetween(seed, 30, 85);
            p.envAmount = randomBetween(seed, 0, 80);
            p.decay = randomBetween(seed, 10, 60);
            p.gate = randomBetween(seed, 20, 80);
            p.drive = randomBetween(seed, 0, 80);
            p.detune = randomBetween(seed, 0, 80);
            p.noise = chance(seed, 50) ? randomBetween(seed, 20, 100) : 0;
            p.lfoShape = randomBetween(seed, 0, kLfoShapeCount - 1);
            p.lfoRate = randomBetween(seed, 0, 6);
            p.lfoDepth = randomBetween(seed, 50, 100);
            p.lfoTarget = randomBetween(seed, 0, kLfoTargetCount - 1);
            p.lfoRetrig = chance(seed, 50) ? 1 : 0;
            p.attack = randomBetween(seed, 0, 30);
            p.ampDecay = randomBetween(seed, 20, 60);
            p.sustain = randomBetween(seed, 0, 50);
            p.release = randomBetween(seed, 15, 50);
            p.filterType = randomBetween(seed, 0, kFilterTypeCount - 1);
            // A HIGH or BAND filter set far above a note without noise filters it all away.
            if (((p.filterType == kHighPass) || (p.filterType == kBandPass)) && (p.noise == 0) && (p.cutoff > 60)) {
                p.cutoff = randomBetween(seed, 25, 60);
            }
            // v2.05: half of the effects get a pitch envelope (zaps, drops, blips).
            if (chance(seed, 50)) {
                p.pitchEnv = randomBetween(seed, 30, 100);
                p.pitchDecay = randomBetween(seed, 5, 50);
            }
            break;
    }

    // v2.05 FM: some basses, leads and plucks become FM sounds (keeping their envelopes), with a
    // harmonic ratio most of the time and an inharmonic (bell / metal) one now and then.
    if ((family <= 2) && chance(seed, 25)) {
        static constexpr uint8_t kHarmonic[6] = {0, 1, 1, 4, 7, 9};  // 0.5, 1, 1, 2, 3, 4
        static constexpr uint8_t kInharmonic[3] = {2, 6, 8};         // 1.41, 2.76, 3.5
        p.wave = kFm;
        p.fmRatio = chance(seed, 75) ? kHarmonic[soundRandom(seed) % 6] : kInharmonic[soundRandom(seed) % 3];
        p.fmAmount = randomBetween(seed, 20, 75);
        p.fmDecay = randomBetween(seed, 10, 70);
        p.cutoff = randomBetween(seed, 55, 100);  // FM makes its own brightness
    }
    // ...and some basses and plucks a short pitch drop at the start.
    if (((family == 0) || (family == 2)) && chance(seed, 15)) {
        p.pitchEnv = randomBetween(seed, 10, 40);
        p.pitchDecay = randomBetween(seed, 5, 30);
    }
    // ...and some pads a filter that blooms open.
    if ((family == 3) && chance(seed, 35)) p.filterAttack = randomBetween(seed, 40, 85);
    return sanitize(p);
}

// A small variation: three of the continuous settings move 5..15 steps (clamped to 0..100), and
// now and then the waveform changes. Returns how many settings changed.
inline uint8_t mutateSound(Params& p, uint32_t& seed) {
    // v2.05: an FM sound may also move its FM depth and decay.
    uint8_t* fields[14] = {&p.cutoff, &p.resonance, &p.envAmount, &p.decay, &p.drive, &p.detune, &p.sub,
                           &p.attack, &p.ampDecay, &p.sustain, &p.release, &p.lfoDepth, &p.fmAmount, &p.fmDecay};
    const uint8_t fieldCount = (p.wave == kFm) ? 14 : 12;
    uint8_t changed = 0;
    uint16_t used = 0;
    for (uint8_t c = 0; c < 3; c++) {
        uint8_t index = static_cast<uint8_t>(soundRandom(seed) % fieldCount);
        while (used & (1u << index)) index = static_cast<uint8_t>((index + 1) % fieldCount);
        used = static_cast<uint16_t>(used | (1u << index));
        const int step = 5 + static_cast<int>(soundRandom(seed) % 11);
        int value = *fields[index] + ((soundRandom(seed) & 1u) ? step : -step);
        if (value < 0) value = 0;
        if (value > kMaxParam) value = kMaxParam;
        if (value != *fields[index]) changed++;
        *fields[index] = static_cast<uint8_t>(value);
    }
    if (chance(seed, 15) && (p.wave < kSample)) {  // SAMPLE and FM keep their waveform
        const uint8_t wave = randomBetween(seed, 0, kSample - 1);
        if (wave != p.wave) changed++;
        p.wave = wave;
    }
    p = sanitize(p);
    return changed;
}

}  // namespace synth

#endif
