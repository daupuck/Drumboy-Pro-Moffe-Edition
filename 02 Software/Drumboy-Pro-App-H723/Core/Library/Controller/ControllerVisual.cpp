// VISUALS mode (v1.14): full-screen music visuals.
// Part of class Controller (declared in Controller.h).
//
// HOME (CENTER) on the home screen turns it on. LEFT / RIGHT pick one of seven styles, UP / DOWN
// one of six colour themes, HOME turns it off again; any key on the menu panel leaves and opens
// its menu. Pads, transport, MUTE/FILL/STYLE and the home-screen knobs keep working.
//
// How it draws: the panel has no frame buffer and is written over a parallel bus, so every
// style draws incrementally - each frame erases exactly what it drew before (in black) and draws
// the new state. While VISUALS is on the normal UI is muted in the LCD driver (it keeps its
// state but draws nothing); leaving redraws the whole home screen. The audio ISR copies its
// output and levels into visTap (DTCM) only while VISUALS is on; the maths is in Visuals.h.

// Global.h has a few unused helpers/locals that warn in every file that includes it; this new
// file keeps the firmware's warning count where it was.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "Controller.h"
#pragma GCC diagnostic pop

// What the audio ISR captures (DTCM: CPU-only RAM, unused before; RAM_D1 is nearly full).
__attribute__((section(".DTCM"))) vis::Tap visTap;

namespace {

constexpr int16_t kW = 960;
constexpr int16_t kH = 540;
constexpr int16_t kHintH = 12;          // top band reserved for the hint line
constexpr uint32_t kFrameMs = 33;       // ~30 frames per second
constexpr uint32_t kHintMs = 3500;

constexpr uint8_t kSceneCount = 13;
const char* const kSceneNames[kSceneCount] = {"SHOWTIME", "ORBIT", "PHASE", "SPECTRUM", "SPECTRO", "TUNNEL", "METERS",
                                              "RAIN", "FIREWORKS", "MANDALA", "DISCO", "RIDGES", "LIFE"};
constexpr uint8_t kThemeCount = 6;
const char* const kThemeNames[kThemeCount] = {"SYNTHWAVE", "ACID", "OCEAN", "FIRE", "ICE", "RAINBOW"};
const uint8_t kThemeBase[kThemeCount] = {213, 60, 110, 0, 140, 0};
const uint8_t kThemeSpread[kThemeCount] = {100, 50, 70, 45, 50, 255};

constexpr uint16_t kScopeCols = 432;    // 2 px each, x 48..911
constexpr uint16_t kPhasePoints = 360;
constexpr uint8_t kRipples = 12;
constexpr uint8_t kRings = 16;
constexpr uint8_t kStars = 90;
constexpr uint8_t kFloorLines = 6;
constexpr uint8_t kFloorVerticals = 17;
constexpr int16_t kNone = 0x7FFF;

constexpr uint8_t kRainCols = 64;       // 15 px each
constexpr uint8_t kRainRows = 34;       // 15 px each, from y 16
constexpr uint16_t kSparks = 320;
constexpr uint8_t kRockets = 16;
constexpr uint8_t kMandalaWave = 48;
constexpr uint8_t kMandalaBands = 32;
constexpr uint16_t kMandalaPoints = 8 * (kMandalaWave + kMandalaBands);
constexpr uint8_t kDiscoCols = 16;
constexpr uint8_t kDiscoRows = 8;
constexpr uint8_t kRidges = 20;
constexpr uint8_t kLifeW = 96;
constexpr uint8_t kLifeH = 52;

struct Ripple {
    int16_t x, y, r, rMax;
    uint16_t c;
    bool on;
};
struct Spoke {
    int16_t x0, y0, x1, y1;
    uint8_t life;
};
struct Ring {
    float s;
    int16_t x, y, w, h;
    uint16_t c;
    bool on, drawn, bold;
};
struct Star {
    float x, y, z;
    int16_t sx, sy;
    uint8_t size;
    bool drawn;
};

struct Spark {
    float x, y, vx, vy;
    int16_t sx, sy;
    uint16_t c;
    uint8_t life, maxLife;
    bool on, drawn;
};
struct Rocket {
    float x, y, vy, power;
    int16_t sx, sy;
    uint16_t c;
    bool on, drawn;
};

struct State {
    // Analysis
    float re[vis::kFftSize];
    float im[vis::kFftSize];
    float hann[vis::kFftSize];
    uint16_t f32[32], l32[32], f64[64], l64[64];
    float band32[32], band64[64];
    float vuL, vuR, bass, energy;
    float layer[vis::kLayers], flash[vis::kLayers], hitNow[vis::kLayers];
    // Timing
    bool playing, barStart;
    float beatPhase;
    uint32_t beatIdx, step8;
    // Segment bars / meters
    uint8_t seg[64], segB[64], cap[64];
    float capPos[64];
    uint8_t vuSeg[2], ladder[vis::kLayers];
    // SHOWTIME
    int16_t scopeTop[kScopeCols], scopeBot[kScopeCols];
    float scopeGain;
    int16_t floorY[kFloorLines];
    // ORBIT (orbs are also SHOWTIME's top row)
    int16_t orbX[vis::kLayers], orbY[vis::kLayers], orbR[vis::kLayers];
    uint16_t orbC[vis::kLayers];
    Ripple ripples[kRipples];
    Spoke spokes[vis::kLayers];
    int16_t coreR;
    float rot;
    // PHASE
    int16_t px[3][kPhasePoints], py[3][kPhasePoints];
    uint8_t gen;
    float phaseGain, phaseAngle;
    // SPECTRO
    uint16_t col;
    uint32_t lastBeatIdx;
    // TUNNEL
    Ring rings[kRings];
    Star stars[kStars];
    uint32_t lastStep, lastSpawnMs, ringCounter;
    int16_t glowR;
    // METERS
    float needle[2];
    int16_t needle0X[2], needle0Y[2], needle1X[2], needle1Y[2];
    // RAIN
    char rainChar[kRainCols][kRainRows];
    float rainHead[kRainCols], rainBoost[kRainCols];
    int16_t rainDrawn[kRainCols];
    uint8_t rainLen[kRainCols];
    // FIREWORKS
    Spark sparks[kSparks];
    Rocket rockets[kRockets];
    uint32_t lastRocketMs;
    // MANDALA
    int16_t mx[kMandalaPoints], my[kMandalaPoints];
    bool mDrawn;
    float mRot, mGain;
    // DISCO
    float tileLvl[kDiscoCols * kDiscoRows];
    uint8_t tileDrawn[kDiscoCols * kDiscoRows], tileHue[kDiscoCols * kDiscoRows];
    // RIDGES
    uint8_t ridge[kRidges][64];
    uint8_t ridgeHead, ridgeCount;
    // LIFE
    uint8_t life[kLifeW * kLifeH], lifeNext[kLifeW * kLifeH];
    vis::Rng rng;
};

__attribute__((section(".DTCM"))) State vs;

inline uint16_t themeColor(uint8_t theme_, uint32_t pos_, uint8_t value_ = 255) {
    const uint8_t pos = static_cast<uint8_t>(pos_ & 0xFF);
    return vis::hue(static_cast<uint8_t>(kThemeBase[theme_] + ((pos * kThemeSpread[theme_]) >> 8)), value_);
}

inline uint8_t to8(float x) {
    if (x <= 0.0f) return 0;
    if (x >= 1.0f) return 255;
    return static_cast<uint8_t>(x * 255.0f);
}

// Filled box, clipped to the screen (the driver takes unsigned coordinates).
inline void box(Lcd& d, int x, int y, int w, int h, uint16_t c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > kW) w = kW - x;
    if (y + h > kH) h = kH - y;
    if ((w <= 0) || (h <= 0)) return;
    d.setForeColor(c);
    d.fillRect(static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<uint32_t>(w), static_cast<uint32_t>(h));
}

inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }

// Line with both ends kept on the screen (an off-screen end would wrap in the driver).
inline void line(Lcd& d, int x0, int y0, int x1, int y1, uint16_t c) {
    d.setForeColor(c);
    d.drawLine(static_cast<uint16_t>(clampi(x0, 0, kW - 1)), static_cast<uint16_t>(clampi(y0, 0, kH - 1)),
               static_cast<uint16_t>(clampi(x1, 0, kW - 1)), static_cast<uint16_t>(clampi(y1, 0, kH - 1)));
}

// Circles need an on-screen centre; their pixels clip themselves.
inline void ring(Lcd& d, int x, int y, int r, uint16_t c) {
    if ((r <= 0) || (x < 0) || (x >= kW) || (y < 0) || (y >= kH)) return;
    d.setForeColor(c);
    d.drawCircle(static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<uint16_t>(r));
}
inline void disc(Lcd& d, int x, int y, int r, uint16_t c) {
    if ((r <= 0) || (x < 0) || (x >= kW) || (y < 0) || (y >= kH)) return;
    d.setForeColor(c);
    d.fillCircle(static_cast<uint16_t>(x), static_cast<uint16_t>(y), static_cast<uint16_t>(r));
}

// One LED-style bar: segments 0..target-1 lit, the rest dark; only the change is drawn.
// dir < 0 grows up from `base`, dir > 0 grows down.
template <typename F>
void segBar(Lcd& d, int x, int base, int dir, int w, int segH, int gap, uint8_t& drawn, uint8_t target, F color) {
    const int pitch = segH + gap;
    for (int i = drawn; i < target; i++) {
        const int y = (dir < 0) ? (base - (i * pitch) - segH) : (base + (i * pitch));
        box(d, x, y, w, segH, color(i));
    }
    for (int i = target; i < drawn; i++) {
        const int y = (dir < 0) ? (base - (i * pitch) - segH) : (base + (i * pitch));
        box(d, x, y, w, segH, BLACK);
    }
    drawn = target;
}

inline float mono(uint32_t i) {
    const uint32_t k = i & (vis::kScopeSize - 1);
    return 0.5f * (visTap.scopeL[k] + visTap.scopeR[k]);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
/* Enter / Exit --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::visual_enter() {
    /// @brief HOME on the home screen: starts the visuals over the whole screen.

    if (visualActive || (menu != GLOBAL_MENU)) return;

    // DTCM is not cleared at boot. (Plain byte clears: a State temporary would not fit the stack.)
    memset(static_cast<void*>(&visTap), 0, sizeof(visTap));
    memset(static_cast<void*>(&vs), 0, sizeof(vs));
    vs.rng.s = 0x9E3779B9u ^ HAL_GetTick();
    for (uint16_t i = 0; i < vis::kFftSize; i++) {
        vs.hann[i] = 0.5f - (0.5f * cosf(6.28318530718f * static_cast<float>(i) / static_cast<float>(vis::kFftSize - 1)));
    }
    vis::bandRanges(32, vis::kFftSize, kAudioSampleRate, 40.0f, 16000.0f, vs.f32, vs.l32);
    vis::bandRanges(64, vis::kFftSize, kAudioSampleRate, 40.0f, 16000.0f, vs.f64, vs.l64);

    visualSceneReady = false;
    visualTick = 0;
    visualHintUntil = HAL_GetTick() + kHintMs;
    visualHintShown = false;
    lcd.setMuted(true);   // from here on the normal UI draws nothing
    visualActive = true;  // last: the audio ISR starts capturing
}

void Controller::visual_exit() {
    /// @brief Leaves the visuals and redraws the home screen.

    if (!visualActive) return;
    visualActive = false;
    lcd.setMuted(false);
    visual_restoreScreen();
}

void Controller::visual_restoreScreen() {
    /// @brief Redraws everything the home screen shows: the page, all layer rows and songs, the
    /// bank tabs, the transport icons, the play bar and the home menu (plus a pending alert).

    lcd.setBackColor(BLACK);
    lcd.clearScreen();
    lcd_drawPage(false);
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) lcd_drawLayerTab(i);  // row + its song lane
    lcd_drawEncoderBankTab();
    lcd_drawSongBankTab(activeSongBank, bankShiftFlag);

    resetIcon.flag = true;
    playIcon.flag = true;
    stopIcon.flag = true;
    recordIcon.flag = true;

    // Play bar: the part already played this loop in the current colour, the rest in the other.
    const RGB16Color other = (playColor == kPlayColor0) ? kPlayColor1 : kPlayColor0;
    lcd.setForeColor(other);
    lcd.drawHLine(kPlayX, kPlayY, kPlayWidth);
    if (playX > 0) {
        lcd.setForeColor(playColor);
        lcd.drawHLine(kPlayX, kPlayY, (playX < kPlayWidth) ? playX : kPlayWidth);
    }

    lcd_drawGlobalMenu();
    if (playArmed || recordArmed) textArmedFlag = true;
    if (alertFlag) lcd_drawAlert();
}

void Controller::visual_filterKeys() {
    /// @brief Called first by keyboard_action while the visuals are on. Takes the keys that
    /// steer the visuals (they are then not seen by the normal handlers) and leaves the visuals
    /// before any menu-panel key, which then works as usual.

    if (!visualActive) return;

    if (keyboard.keyRight.read) {
        for (uint8_t i = 0; i < 16; i++) {
            if (keyboard.keyRight.changeState[i] == KEYSTATE_PRESS) {
                visual_exit();
                return;
            }
        }
    }

    if (keyboard.keyLeft.read) {
        for (uint8_t i = 0; i < 16; i++) {
            if (keyboard.keyLeft.changeState[i] != KEYSTATE_PRESS) continue;
            bool taken = true;
            switch (i) {
                case KEY_CENTER:
                    visual_exit();
                    break;
                case KEY_RIGHT:
                    visualScene = static_cast<uint8_t>((visualScene + 1) % kSceneCount);
                    visualSceneReady = false;
                    break;
                case KEY_LEFT:
                    visualScene = static_cast<uint8_t>((visualScene + kSceneCount - 1) % kSceneCount);
                    visualSceneReady = false;
                    break;
                case KEY_UP:
                    visualTheme = static_cast<uint8_t>((visualTheme + 1) % kThemeCount);
                    visualSceneReady = false;
                    break;
                case KEY_DOWN:
                    visualTheme = static_cast<uint8_t>((visualTheme + kThemeCount - 1) % kThemeCount);
                    visualSceneReady = false;
                    break;
                default:
                    taken = false;
                    break;
            }
            if (taken) {
                keyboard.keyLeft.changeState[i] = KEYSTATE_NONE;
                if (visualActive) {
                    visualHintUntil = HAL_GetTick() + kHintMs;
                    visualHintShown = false;
                }
                if (!visualActive) return;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Frame Loop ----------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::visual_service() {
    /// @brief Main loop: one frame of the visuals every ~33 ms while they are on.

    if (!visualActive) return;
    if (alertFlag) {  // an alert needs the screen and an answer
        visual_exit();
        return;
    }
    const uint32_t now = HAL_GetTick();
    if ((now - visualTick) < kFrameMs) return;
    visualTick = now;
    visualFrame++;

    // The normal UI keeps its own drawing state: save it around the frame.
    const RGB16Color fore = lcd.getForeColor();
    const RGB16Color back = lcd.getBackColor();
    const LcdFont font = lcd.getFont();
    const uint16_t spacing = lcd.getSpacing();
    const LcdAlign align = lcd.getAlignment();
    lcd.setMuted(false);

    // Timing: from the sequencer while it plays, from the tempo otherwise.
    vs.playing = playActive && !metronome.precountState;
    const uint32_t beatsPerBar = (rhythm.measure > 0) ? rhythm.measure : 4;
    if (vs.playing) {
        const uint32_t pi = playInterval;
        vs.beatPhase = static_cast<float>(pi % kMeasureInterval) / static_cast<float>(kMeasureInterval);
        vs.beatIdx = pi / kMeasureInterval;
        vs.step8 = pi / (kMeasureInterval / 2);
    } else {
        const uint32_t msPerBeat = 60000u / ((rhythm.tempo > 0) ? rhythm.tempo : 120u);
        vs.beatPhase = static_cast<float>(now % msPerBeat) / static_cast<float>(msPerBeat);
        vs.beatIdx = now / msPerBeat;
        vs.step8 = now / (msPerBeat / 2);
    }
    vs.barStart = (vs.beatIdx % beatsPerBar) == 0;

    if (!visualSceneReady) {
        visual_initScene();
        visualSceneReady = true;
    }
    visual_analyze();
    visual_drawScene();

    // Hint line: what this is and how to steer it, for a few seconds after any change.
    if (!visualHintShown && (static_cast<int32_t>(visualHintUntil - now) > 0)) {
        visual_drawHint(true);
        visualHintShown = true;
    } else if (visualHintShown && (static_cast<int32_t>(visualHintUntil - now) <= 0)) {
        visual_drawHint(false);
        visualHintShown = false;
        visualHintUntil = now;  // stays off until the next key
    }

    lcd.setMuted(true);
    lcd.setFont(font);
    lcd.setSpacing(spacing);
    lcd.setAlignment(align);
    lcd.setForeColor(fore);
    lcd.setBackColor(back);
}

void Controller::visual_drawHint(bool show_) {
    box(lcd, 0, 0, kW, kHintH, BLACK);
    if (!show_) return;
    char text[120];
    snprintf(text, sizeof(text), "VISUALS %u/%u  %s - %s      < > STYLE    UP/DOWN COLOUR    HOME: EXIT",
             static_cast<unsigned>(visualScene + 1), static_cast<unsigned>(kSceneCount), kSceneNames[visualScene], kThemeNames[visualTheme]);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(LEFT);
    lcd.setBackColor(BLACK);
    lcd.setForeColor(themeColor(visualTheme, 0));
    lcd.drawText(text, static_cast<uint16_t>(strlen(text)), 8, 2);
}

void Controller::visual_analyze() {
    /// @brief Levels, hits and the spectrum of the last 1024 output samples.

    State& s = vs;
    const float pl = visTap.peakL;
    const float pr = visTap.peakR;
    visTap.peakL = 0.0f;
    visTap.peakR = 0.0f;
    s.vuL = vis::follow(s.vuL, vis::level01(pl), 0.025f);
    s.vuR = vis::follow(s.vuR, vis::level01(pr), 0.025f);
    for (uint8_t l = 0; l < vis::kLayers; l++) {
        const float p = visTap.layerPeak[l];
        visTap.layerPeak[l] = 0.0f;
        s.layer[l] = vis::follow(s.layer[l], vis::level01(p * 2.0f), 0.04f);
        const float h = visTap.hit[l];
        visTap.hit[l] = 0.0f;
        s.hitNow[l] = h;
        s.flash[l] = (h > s.flash[l] * 0.82f) ? h : (s.flash[l] * 0.82f);
    }

    const uint32_t end = visTap.pos;
    for (uint16_t i = 0; i < vis::kFftSize; i++) {
        s.re[i] = mono(end - vis::kFftSize + i) * s.hann[i];
        s.im[i] = 0.0f;
    }
    vis::fft(s.re, s.im, vis::kFftSize);
    // A full-scale sine gives |X| = N/4 with the Hann window; highs get a gentle lift so a mix
    // does not look like it only has bass.
    const float norm = 4.0f / static_cast<float>(vis::kFftSize);
    for (uint8_t b = 0; b < 64; b++) {
        float m = 0.0f;
        for (uint16_t k = s.f64[b]; k <= s.l64[b]; k++) {
            const float mag = sqrtf((s.re[k] * s.re[k]) + (s.im[k] * s.im[k]));
            if (mag > m) m = mag;
        }
        s.band64[b] = vis::follow(s.band64[b], vis::level01(m * norm * (1.0f + (b * 0.05f))), 0.03f);
    }
    float sum = 0.0f;
    for (uint8_t b = 0; b < 32; b++) {
        float m = 0.0f;
        for (uint16_t k = s.f32[b]; k <= s.l32[b]; k++) {
            const float mag = sqrtf((s.re[k] * s.re[k]) + (s.im[k] * s.im[k]));
            if (mag > m) m = mag;
        }
        s.band32[b] = vis::follow(s.band32[b], vis::level01(m * norm * (1.0f + (b * 0.1f))), 0.035f);
        sum += s.band32[b];
    }
    s.energy = sum / 32.0f;
    s.bass = s.band32[0];
    for (uint8_t b = 1; b < 4; b++) {
        if (s.band32[b] > s.bass) s.bass = s.band32[b];
    }
}

void Controller::visual_initScene() {
    lcd.setBackColor(BLACK);
    lcd.fillScreen(BLACK);
    visualHintShown = false;  // the hint was wiped with the screen
    switch (visualScene) {
        case 0: visual_sceneShowtime(true); break;
        case 1: visual_sceneOrbit(true); break;
        case 2: visual_scenePhase(true); break;
        case 3: visual_sceneSpectrum(true); break;
        case 4: visual_sceneSpectro(true); break;
        case 5: visual_sceneTunnel(true); break;
        case 6: visual_sceneMeters(true); break;
        case 7: visual_sceneRain(true); break;
        case 8: visual_sceneFireworks(true); break;
        case 9: visual_sceneMandala(true); break;
        case 10: visual_sceneDisco(true); break;
        case 11: visual_sceneRidges(true); break;
        default: visual_sceneLife(true); break;
    }
}

void Controller::visual_drawScene() {
    switch (visualScene) {
        case 0: visual_sceneShowtime(false); break;
        case 1: visual_sceneOrbit(false); break;
        case 2: visual_scenePhase(false); break;
        case 3: visual_sceneSpectrum(false); break;
        case 4: visual_sceneSpectro(false); break;
        case 5: visual_sceneTunnel(false); break;
        case 6: visual_sceneMeters(false); break;
        case 7: visual_sceneRain(false); break;
        case 8: visual_sceneFireworks(false); break;
        case 9: visual_sceneMandala(false); break;
        case 10: visual_sceneDisco(false); break;
        case 11: visual_sceneRidges(false); break;
        default: visual_sceneLife(false); break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Styles --------------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::visual_sceneShowtime(bool init_) {
    /// @brief Everything at once: layer lights, VU columns, spectrum skyline, a triggered
    /// oscilloscope across the horizon and a neon floor rolling towards you on the beat.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kVx = 480, kHy = 400, kBy = 532, kFx0 = 40, kFx1 = 920;
    const uint16_t vertC = themeColor(t, 40, 120);
    const uint16_t horizonC = themeColor(t, 0, 255);
    auto vertX = [&](int k, int y) -> int {
        const int bx = kVx + ((k - 8) * 110);
        return kVx + (((bx - kVx) * (y - kHy)) / (kBy - kHy));
    };

    if (init_) {
        box(d, kFx0, kHy, kFx1 - kFx0, 1, horizonC);
        for (uint8_t k = 0; k < kFloorVerticals; k++) {
            int x1 = vertX(k, kBy);
            int y1 = kBy;
            if ((x1 < kFx0) || (x1 >= kFx1)) {
                const int bx = kVx + ((k - 8) * 110);
                const int xb = (x1 < kFx0) ? kFx0 : (kFx1 - 1);
                y1 = kHy + (((xb - kVx) * (kBy - kHy)) / (bx - kVx));
                x1 = xb;
            }
            line(d, kVx, kHy, x1, y1, vertC);
        }
        for (uint8_t k = 0; k < kFloorLines; k++) s.floorY[k] = kNone;
        for (uint16_t c = 0; c < kScopeCols; c++) s.scopeTop[c] = kNone;
        for (uint8_t i = 0; i < vis::kLayers; i++) s.orbR[i] = 0;
        memset(s.seg, 0, sizeof(s.seg));
        memset(s.cap, 0, sizeof(s.cap));
        memset(s.capPos, 0, sizeof(s.capPos));
        s.vuSeg[0] = s.vuSeg[1] = 0;
        s.scopeGain = 1.0f;
        return;
    }

    // 1. LAYER LIGHTS (top row): size and brightness follow the layer, a hit flashes white.
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        const int cx = 48 + (96 * i);
        const int cy = 38;
        const float lvl = (s.layer[i] > s.flash[i]) ? s.layer[i] : s.flash[i];
        const int16_t r = static_cast<int16_t>(5 + (lvl * 19.0f));
        const uint16_t c = (s.flash[i] > 0.6f) ? WHITE : themeColor(t, i * 25u, static_cast<uint8_t>(70 + (lvl * 185.0f)));
        if ((r != s.orbR[i]) || (c != s.orbC[i])) {
            if (r < s.orbR[i]) disc(d, cx, cy, s.orbR[i], BLACK);
            disc(d, cx, cy, r, c);
            s.orbR[i] = r;
            s.orbC[i] = c;
        }
    }

    // 2. VU COLUMNS at both edges.
    auto vuColor = [](int i) -> uint16_t { return (i < 30) ? vis::rgb(0, 220, 90) : ((i < 40) ? vis::rgb(255, 210, 0) : vis::rgb(255, 40, 40)); };
    segBar(d, 8, 530, -1, 22, 8, 2, s.vuSeg[0], static_cast<uint8_t>(s.vuL * 46.0f), vuColor);
    segBar(d, 930, 530, -1, 22, 8, 2, s.vuSeg[1], static_cast<uint8_t>(s.vuR * 46.0f), vuColor);

    // 3. SPECTRUM SKYLINE: 32 towers with falling white caps.
    for (uint8_t b = 0; b < 32; b++) {
        const int x = 48 + (b * 27);
        const uint8_t n = static_cast<uint8_t>(s.band32[b] * 25.0f);
        segBar(d, x, 285, -1, 22, 6, 2, s.seg[b], n, [&](int i) { return themeColor(t, (b * 8u) + (i * 3u), static_cast<uint8_t>(120 + (i * 5))); });
        s.capPos[b] = (s.capPos[b] - 0.3f > n) ? (s.capPos[b] - 0.3f) : n;
        const uint8_t ci = static_cast<uint8_t>(s.capPos[b]);
        if ((s.cap[b] != ci) && (s.cap[b] >= n) && (s.cap[b] < 25)) box(d, x, 285 - (s.cap[b] * 8) - 6, 22, 6, BLACK);
        if ((ci >= n) && (ci < 25)) box(d, x, 285 - (ci * 8) - 6, 22, 6, WHITE);
        s.cap[b] = ci;
    }

    // 4. OSCILLOSCOPE across the horizon, triggered on a rising zero crossing so it stands still.
    {
        constexpr int kTop = 297, kBot = 393, kMid = 345;
        constexpr uint16_t kSpan = kScopeCols * 2;
        const uint32_t end = visTap.pos;
        uint32_t start = end - kSpan - 1;
        for (uint32_t k = 0; k < 150; k++) {
            const uint32_t i = end - kSpan - 2 - k;
            if ((mono(i) <= 0.0f) && (mono(i + 1) > 0.0f)) {
                start = i;
                break;
            }
        }
        float peak = 0.02f;
        for (uint16_t i = 0; i < kSpan; i++) {
            const float a = fabsf(mono(start + i));
            if (a > peak) peak = a;
        }
        float target = 0.9f / peak;
        if (target > 6.0f) target = 6.0f;
        s.scopeGain += (target - s.scopeGain) * 0.2f;
        const float scale = s.scopeGain * 48.0f;
        int prevY = clampi(kMid - static_cast<int>(mono(start) * scale), kTop, kBot);
        for (uint16_t c = 0; c < kScopeCols; c++) {
            const int ya = clampi(kMid - static_cast<int>(mono(start + (2 * c)) * scale), kTop, kBot);
            const int yb = clampi(kMid - static_cast<int>(mono(start + (2 * c) + 1) * scale), kTop, kBot);
            int top = (ya < yb) ? ya : yb;
            int bot = (ya < yb) ? yb : ya;
            if (prevY < top) top = prevY;
            if (prevY > bot) bot = prevY;
            prevY = yb;
            if ((top != s.scopeTop[c]) || (bot != s.scopeBot[c])) {
                const int x = 48 + (2 * c);
                if (s.scopeTop[c] != kNone) box(d, x, s.scopeTop[c], 2, (s.scopeBot[c] - s.scopeTop[c]) + 1, BLACK);
                box(d, x, top, 2, (bot - top) + 1, themeColor(t, (c * 255u) / kScopeCols));
                s.scopeTop[c] = static_cast<int16_t>(top);
                s.scopeBot[c] = static_cast<int16_t>(bot);
            }
        }
    }

    // 5. NEON FLOOR: one line spacing per beat, so the floor rolls in time with the music.
    int16_t newY[kFloorLines];
    float depth[kFloorLines];
    for (uint8_t k = 0; k < kFloorLines; k++) {
        depth[k] = (static_cast<float>(k) + 1.0f - s.beatPhase) / kFloorLines;
        newY[k] = vis::floorY(kHy + 1, kBy, depth[k]);
    }
    for (uint8_t k = 0; k < kFloorLines; k++) {  // erase all moved lines first...
        if ((s.floorY[k] == kNone) || (s.floorY[k] == newY[k])) continue;
        const int y = s.floorY[k];
        box(d, kFx0, y, kFx1 - kFx0, 1, BLACK);
        for (uint8_t v = 0; v < kFloorVerticals; v++) {  // ...put back where the rails crossed
            const int x = vertX(v, y);
            if ((x >= kFx0) && (x < kFx1)) box(d, x, y, 1, 1, vertC);
        }
    }
    for (uint8_t k = 0; k < kFloorLines; k++) {  // ...then draw them all
        if (s.floorY[k] == newY[k]) continue;
        const uint8_t v = static_cast<uint8_t>(60 + ((1.0f - depth[k]) * 195.0f));
        box(d, kFx0, newY[k], kFx1 - kFx0, 1, vis::dim(horizonC, v));
        s.floorY[k] = newY[k];
    }
}

void Controller::visual_sceneOrbit(bool init_) {
    /// @brief The ten layers as planets circling a pulsing core. Every hit sends a ripple out
    /// of its planet and a flash of lightning to the core; the orbit turns one planet per bar.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCx = 480, kCy = 278;

    if (init_) {
        for (uint8_t i = 0; i < vis::kLayers; i++) s.orbR[i] = 0;
        for (uint8_t i = 0; i < kRipples; i++) s.ripples[i].on = false;
        for (uint8_t i = 0; i < vis::kLayers; i++) s.spokes[i].life = 0;
        s.coreR = 0;
        return;
    }

    // Turn: one planet (36 degrees) per bar while playing, a slow drift when stopped.
    const float tempo = static_cast<float>((rhythm.tempo > 0) ? rhythm.tempo : 120);
    const float barSeconds = static_cast<float>((rhythm.measure > 0) ? rhythm.measure : 4) * 60.0f / tempo;
    s.rot += s.playing ? (0.6283f * (kFrameMs / 1000.0f) / barSeconds) : 0.0025f;

    // Ripples: erase, grow, draw (they fade as they grow).
    for (uint8_t i = 0; i < kRipples; i++) {
        Ripple& rp = s.ripples[i];
        if (!rp.on) continue;
        ring(d, rp.x, rp.y, rp.r, BLACK);
        rp.r = static_cast<int16_t>(rp.r + 6);
        if (rp.r > rp.rMax) {
            rp.on = false;
            continue;
        }
        ring(d, rp.x, rp.y, rp.r, vis::dim(rp.c, static_cast<uint8_t>(255 - ((rp.r * 230) / rp.rMax))));
    }

    // Lightning: lives a few frames, then is erased exactly where it was drawn.
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        Spoke& sp = s.spokes[i];
        if (sp.life == 0) continue;
        if (--sp.life == 0) line(d, sp.x0, sp.y0, sp.x1, sp.y1, BLACK);
    }

    // Core: breathes with the bass, colour slowly cycling.
    const int16_t coreR = static_cast<int16_t>(12 + (s.bass * 70.0f));
    if (coreR < s.coreR) disc(d, kCx, kCy, s.coreR, BLACK);
    disc(d, kCx, kCy, coreR, themeColor(t, visualFrame * 3u, static_cast<uint8_t>(110 + (s.bass * 145.0f))));
    s.coreR = coreR;

    // Planets.
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        const float a = s.rot + (i * 0.6283f);
        const int x = kCx + static_cast<int>(330.0f * cosf(a));
        const int y = kCy + static_cast<int>(180.0f * sinf(a));
        const float lvl = (s.layer[i] > s.flash[i]) ? s.layer[i] : s.flash[i];
        const int16_t r = static_cast<int16_t>(7 + (lvl * 20.0f));
        const uint16_t base = themeColor(t, i * 25u);
        const uint16_t c = (s.flash[i] > 0.6f) ? WHITE : vis::dim(base, static_cast<uint8_t>(90 + (lvl * 165.0f)));
        if (s.orbR[i] > 0) disc(d, s.orbX[i], s.orbY[i], s.orbR[i], BLACK);
        disc(d, x, y, r, c);
        s.orbX[i] = static_cast<int16_t>(x);
        s.orbY[i] = static_cast<int16_t>(y);
        s.orbR[i] = r;
        s.orbC[i] = c;

        if (s.hitNow[i] > 0.0f) {
            for (uint8_t k = 0; k < kRipples; k++) {
                Ripple& rp = s.ripples[k];
                if (rp.on) continue;
                rp = Ripple{static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(r + 2),
                            static_cast<int16_t>(40 + (s.hitNow[i] * 80.0f)), base, true};
                break;
            }
            Spoke& sp = s.spokes[i];
            if (sp.life > 0) line(d, sp.x0, sp.y0, sp.x1, sp.y1, BLACK);
            sp = Spoke{static_cast<int16_t>(kCx), static_cast<int16_t>(kCy), static_cast<int16_t>(x), static_cast<int16_t>(y), 5};
            line(d, sp.x0, sp.y0, sp.x1, sp.y1, vis::dim(base, 200));
        }
    }
}

void Controller::visual_scenePhase(bool init_) {
    /// @brief The sound drawn against itself a moment later (a phase portrait): a pure tone is a
    /// clean loop, chords become knots, noise becomes a cloud. Three generations of dots give it
    /// a glowing trail; the picture slowly turns. Silence shows a gentle Lissajous figure.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr uint16_t kDelay = 14;
    constexpr int kCx = 480, kCy = 276;

    if (init_) {
        for (uint8_t g = 0; g < 3; g++) {
            for (uint16_t i = 0; i < kPhasePoints; i++) s.px[g][i] = kNone;
        }
        s.gen = 0;
        s.phaseGain = 200.0f;
        return;
    }

    const uint32_t end = visTap.pos;
    const uint32_t base = end - kPhasePoints - kDelay - 1;
    float peak = 0.0f;
    for (uint16_t i = 0; i < kPhasePoints + kDelay; i++) {
        const float a = fabsf(mono(base + i));
        if (a > peak) peak = a;
    }
    const bool idle = peak < 0.002f;
    float target = idle ? 220.0f : (230.0f / ((peak > 0.01f) ? peak : 0.01f));
    if (target > 2500.0f) target = 2500.0f;
    s.phaseGain += (target - s.phaseGain) * 0.15f;
    s.phaseAngle += 0.004f + (s.energy * 0.03f);
    const float ca = cosf(s.phaseAngle);
    const float sa = sinf(s.phaseAngle);

    const uint8_t oldest = static_cast<uint8_t>((s.gen + 1) % 3);
    const uint8_t newest = s.gen;
    for (uint16_t i = 0; i < kPhasePoints; i++) {  // the oldest trail goes...
        if (s.px[oldest][i] != kNone) box(d, s.px[oldest][i] - 1, s.py[oldest][i] - 1, 3, 3, BLACK);
    }
    for (uint16_t i = 0; i < kPhasePoints; i++) {  // ...the last frame dims...
        if (s.px[newest][i] != kNone) box(d, s.px[newest][i] - 1, s.py[newest][i] - 1, 3, 3, themeColor(t, (i * 255u) / kPhasePoints, 80));
    }
    for (uint16_t i = 0; i < kPhasePoints; i++) {  // ...and the new frame shines.
        float a;
        float b;
        if (idle) {
            const float ph = (visualFrame * 0.02f) + (i * 0.0175f);
            a = 0.8f * sinf((3.0f * ph) + (visualFrame * 0.01f));
            b = 0.8f * sinf(4.0f * ph);
        } else {
            a = mono(base + i);
            b = mono(base + i + kDelay);
        }
        const float x = (a * ca) - (b * sa);
        const float y = (a * sa) + (b * ca);
        const int sx = clampi(kCx + static_cast<int>(x * s.phaseGain), 2, kW - 3);
        const int sy = clampi(kCy - static_cast<int>(y * s.phaseGain), kHintH + 2, kH - 3);
        s.px[oldest][i] = static_cast<int16_t>(sx);
        s.py[oldest][i] = static_cast<int16_t>(sy);
        box(d, sx - 1, sy - 1, 3, 3, themeColor(t, (i * 255u) / kPhasePoints, 255));
    }
    s.gen = oldest;
}

void Controller::visual_sceneSpectrum(bool init_) {
    /// @brief 32 rainbow towers mirrored in a reflecting floor, with falling white caps.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kUp = 268, kDown = 273, kSegs = 30;

    if (init_) {
        memset(s.seg, 0, sizeof(s.seg));
        memset(s.segB, 0, sizeof(s.segB));
        memset(s.cap, 0xFF, sizeof(s.cap));
        memset(s.capPos, 0, sizeof(s.capPos));
        return;
    }

    for (uint8_t b = 0; b < 32; b++) {
        const int x = 48 + (b * 27);
        const uint8_t n = static_cast<uint8_t>(s.band32[b] * kSegs);
        segBar(d, x, kUp, -1, 22, 6, 2, s.seg[b], n, [&](int i) { return themeColor(t, (b * 8u) + (i * 4u), static_cast<uint8_t>(110 + (i * 4))); });
        segBar(d, x, kDown, 1, 22, 6, 2, s.segB[b], n, [&](int i) { return vis::dim(themeColor(t, (b * 8u) + (i * 4u)), static_cast<uint8_t>(80 - (i * 2))); });
        s.capPos[b] = (s.capPos[b] - 0.35f > n) ? (s.capPos[b] - 0.35f) : n;
        const uint8_t ci = static_cast<uint8_t>(s.capPos[b]);
        if ((s.cap[b] != ci) && (s.cap[b] >= n) && (s.cap[b] < kSegs)) box(d, x, kUp - (s.cap[b] * 8) - 6, 22, 6, BLACK);
        if ((ci >= n) && (ci < kSegs)) box(d, x, kUp - (ci * 8) - 6, 22, 6, WHITE);
        s.cap[b] = ci;
    }
}

void Controller::visual_sceneSpectro(bool init_) {
    /// @brief A spectrogram painting itself across the screen: time runs left to right, pitch
    /// bottom to top, loudness as heat. Ticks under it mark the beats (white on the bar).

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kX0 = 48, kCols = 288, kBottom = 500, kCell = 7;

    if (init_) {
        const uint16_t frame = themeColor(t, 0, 90);
        box(d, kX0 - 3, kBottom - (64 * kCell) - 3, (kCols * 3) + 6, 1, frame);
        box(d, kX0 - 3, kBottom + 1, (kCols * 3) + 6, 1, frame);
        s.col = 0;
        s.lastBeatIdx = s.beatIdx;
        return;
    }

    const int x = kX0 + (s.col * 3);
    for (uint8_t b = 0; b < 64; b++) {
        const uint8_t v = to8(s.band64[b]);
        const uint16_t c = (t == 0) ? vis::heat(v) : themeColor(t, v, v);
        box(d, x, kBottom - ((b + 1) * kCell), 3, kCell, c);
    }
    box(d, x, kBottom + 4, 3, 8, BLACK);
    if (s.beatIdx != s.lastBeatIdx) {
        box(d, x, kBottom + 4, 3, 8, s.barStart ? WHITE : themeColor(t, 0));
        s.lastBeatIdx = s.beatIdx;
    }
    s.col = static_cast<uint16_t>((s.col + 1) % kCols);
    box(d, kX0 + (s.col * 3), kBottom - (64 * kCell), 2, 64 * kCell, WHITE);  // the write head
}

void Controller::visual_sceneTunnel(bool init_) {
    /// @brief Flying through a tunnel of frames, one per eighth note (a bold one on each bar),
    /// through a starfield whose speed follows the music.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCx = 480, kCy = 276;

    if (init_) {
        for (uint8_t i = 0; i < kRings; i++) s.rings[i].on = false;
        for (uint8_t i = 0; i < kStars; i++) {
            Star& st = s.stars[i];
            st.x = static_cast<float>(s.rng.range(-1000, 1000)) / 1000.0f;
            st.y = static_cast<float>(s.rng.range(-1000, 1000)) / 1000.0f;
            st.z = static_cast<float>(s.rng.range(100, 1000)) / 1000.0f;
            st.drawn = false;
        }
        s.lastStep = s.step8;
        s.lastSpawnMs = HAL_GetTick();
        s.glowR = 0;
        return;
    }

    // Spawn: on every eighth note while playing; on bass hits (or a slow pulse) when stopped.
    const uint32_t now = HAL_GetTick();
    bool spawn = false;
    bool bold = false;
    if (s.playing) {
        if (s.step8 != s.lastStep) {
            spawn = true;
            const uint32_t eighthsPerBar = 2u * ((rhythm.measure > 0) ? rhythm.measure : 4u);
            bold = (s.step8 % eighthsPerBar) == 0;
            s.lastStep = s.step8;
        }
    } else if ((((now - s.lastSpawnMs) > 150) && (s.bass > 0.55f)) || ((now - s.lastSpawnMs) > 600)) {
        spawn = true;
    }
    if (spawn) {
        s.lastSpawnMs = now;
        for (uint8_t i = 0; i < kRings; i++) {
            Ring& r = s.rings[i];
            if (r.on) continue;
            r.on = true;
            r.drawn = false;
            r.s = 6.0f;
            r.bold = bold;
            r.c = bold ? WHITE : themeColor(t, s.ringCounter * 23u);
            s.ringCounter++;
            break;
        }
    }

    // Frames: erase, grow (faster when it is loud), draw.
    const float grow = 1.06f + (s.energy * 0.08f);
    for (uint8_t i = 0; i < kRings; i++) {
        Ring& r = s.rings[i];
        if (!r.on) continue;
        if (r.drawn) {
            d.setForeColor(BLACK);
            d.drawRect(static_cast<uint16_t>(r.x), static_cast<uint16_t>(r.y), static_cast<uint32_t>(r.w), static_cast<uint32_t>(r.h));
            if (r.bold && (r.w > 8) && (r.h > 8)) d.drawRect(static_cast<uint16_t>(r.x + 3), static_cast<uint16_t>(r.y + 3), static_cast<uint32_t>(r.w - 6), static_cast<uint32_t>(r.h - 6));
        }
        r.s *= grow;
        const int w = static_cast<int>(r.s * 1.78f);
        const int h = static_cast<int>(r.s);
        if ((w >= kW - 4) || (h >= kH - 4)) {
            r.on = false;
            continue;
        }
        r.x = static_cast<int16_t>(kCx - (w / 2));
        r.y = static_cast<int16_t>(kCy - (h / 2));
        r.w = static_cast<int16_t>(w);
        r.h = static_cast<int16_t>(h);
        const uint8_t v = static_cast<uint8_t>(90 + ((r.s * 0.6f > 165.0f) ? 165.0f : (r.s * 0.6f)));
        d.setForeColor(vis::dim(r.c, v));
        d.drawRect(static_cast<uint16_t>(r.x), static_cast<uint16_t>(r.y), static_cast<uint32_t>(r.w), static_cast<uint32_t>(r.h));
        if (r.bold && (w > 8) && (h > 8)) d.drawRect(static_cast<uint16_t>(r.x + 3), static_cast<uint16_t>(r.y + 3), static_cast<uint32_t>(w - 6), static_cast<uint32_t>(h - 6));
        r.drawn = true;
    }

    // Stars: closer = bigger and brighter.
    const float speed = 0.006f + (s.energy * 0.05f) + (s.bass * 0.02f);
    for (uint8_t i = 0; i < kStars; i++) {
        Star& st = s.stars[i];
        if (st.drawn) box(d, st.sx, st.sy, st.size, st.size, BLACK);
        st.drawn = false;
        st.z -= speed;
        if (st.z <= 0.04f) {
            st.x = static_cast<float>(s.rng.range(-1000, 1000)) / 1000.0f;
            st.y = static_cast<float>(s.rng.range(-1000, 1000)) / 1000.0f;
            st.z = 1.0f;
            continue;
        }
        const int sx = kCx + static_cast<int>((st.x / st.z) * 260.0f);
        const int sy = kCy + static_cast<int>((st.y / st.z) * 160.0f);
        if ((sx < 0) || (sx >= kW - 4) || (sy < kHintH) || (sy >= kH - 4)) {
            st.z = 1.0f;
            continue;
        }
        st.sx = static_cast<int16_t>(sx);
        st.sy = static_cast<int16_t>(sy);
        st.size = static_cast<uint8_t>(1 + ((1.0f - st.z) * 3.0f));
        const uint8_t g = static_cast<uint8_t>((1.0f - st.z) * 255.0f);
        box(d, sx, sy, st.size, st.size, vis::rgb(g, g, g));
        st.drawn = true;
    }

    // The light at the end of the tunnel.
    const int16_t glow = static_cast<int16_t>(3 + (s.bass * 14.0f));
    if (glow < s.glowR) disc(d, kCx, kCy, s.glowR, BLACK);
    disc(d, kCx, kCy, glow, themeColor(t, visualFrame * 2u));
    s.glowR = glow;
}

void Controller::visual_sceneMeters(bool init_) {
    /// @brief Two big analogue needle meters for the output (VU ballistics) over ten LED
    /// ladders, one per layer.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCy = 372;
    const int cx[2] = {250, 710};
    constexpr float kMinDb = -30.0f;
    constexpr float kSpan = 0.96f;  // +/- 55 degrees
    auto dbAngle = [&](float db) { return -kSpan + (((db - kMinDb) / -kMinDb) * 2.0f * kSpan); };

    if (init_) {
        static const float kTicks[] = {-30, -25, -20, -15, -10, -7, -5, -3, -2, -1, 0};
        static const char* const kLabels[] = {"30", "", "20", "", "10", "7", "5", "3", "", "", "0"};
        lcd.setFont(FONT_07x09);
        lcd.setAlignment(CENTER);
        lcd.setBackColor(BLACK);
        for (uint8_t m = 0; m < 2; m++) {
            for (int deg = -55; deg <= 55; deg++) {  // the scale arc
                const float a = deg * 0.017453f;
                const uint16_t c = (deg > 38) ? vis::rgb(255, 40, 40) : themeColor(t, 0, 160);
                box(d, cx[m] + static_cast<int>(192.0f * sinf(a)), kCy - static_cast<int>(192.0f * cosf(a)), 2, 2, c);
            }
            for (uint8_t k = 0; k < sizeof(kTicks) / sizeof(kTicks[0]); k++) {
                const float a = dbAngle(kTicks[k]);
                const bool major = kLabels[k][0] != 0;
                const uint16_t c = (kTicks[k] > -3.5f) ? vis::rgb(255, 40, 40) : WHITE;
                const int r0 = 176;
                const int r1 = major ? 192 : 186;
                line(d, cx[m] + static_cast<int>(r0 * sinf(a)), kCy - static_cast<int>(r0 * cosf(a)),
                     cx[m] + static_cast<int>(r1 * sinf(a)), kCy - static_cast<int>(r1 * cosf(a)), c);
                if (major) {
                    lcd.setForeColor(c);
                    lcd.drawText(kLabels[k], static_cast<uint16_t>(strlen(kLabels[k])), static_cast<uint16_t>(cx[m] + static_cast<int>(206.0f * sinf(a))),
                                 static_cast<uint16_t>(kCy - static_cast<int>(206.0f * cosf(a)) - 4));
                }
            }
            disc(d, cx[m], kCy, 12, themeColor(t, 0));
            lcd.setForeColor(themeColor(t, 0, 200));
            lcd.drawText((m == 0) ? "LEFT" : "RIGHT", (m == 0) ? 4 : 5, static_cast<uint16_t>(cx[m]), kCy + 18);
            s.needle[m] = dbAngle(kMinDb);
            s.needle0X[m] = kNone;
        }
        lcd.setForeColor(themeColor(t, 0, 160));
        for (uint8_t i = 0; i < vis::kLayers; i++) {
            char num[3];
            snprintf(num, sizeof(num), "%u", static_cast<unsigned>(i + 1));
            lcd.drawText(num, static_cast<uint16_t>(strlen(num)), static_cast<uint16_t>(52 + (i * 88) + 22), 524);
            s.ladder[i] = 0;
        }
        lcd.setAlignment(LEFT);
        return;
    }

    // Needles: VU-like ballistics (they swing, they do not jump).
    for (uint8_t m = 0; m < 2; m++) {
        const float level = (m == 0) ? s.vuL : s.vuR;  // 0..1 over 60 dB
        float db = (level * 60.0f) - 60.0f;
        if (db < kMinDb) db = kMinDb;
        s.needle[m] += (dbAngle(db) - s.needle[m]) * 0.35f;
        const float a = s.needle[m];
        const int x0 = cx[m] + static_cast<int>(16.0f * sinf(a));
        const int y0 = kCy - static_cast<int>(16.0f * cosf(a));
        const int x1 = cx[m] + static_cast<int>(168.0f * sinf(a));
        const int y1 = kCy - static_cast<int>(168.0f * cosf(a));
        if ((x1 != s.needle1X[m]) || (y1 != s.needle1Y[m]) || (s.needle0X[m] == kNone)) {
            if (s.needle0X[m] != kNone) line(d, s.needle0X[m], s.needle0Y[m], s.needle1X[m], s.needle1Y[m], BLACK);
            line(d, x0, y0, x1, y1, (db > -3.0f) ? vis::rgb(255, 60, 60) : WHITE);
            s.needle0X[m] = static_cast<int16_t>(x0);
            s.needle0Y[m] = static_cast<int16_t>(y0);
            s.needle1X[m] = static_cast<int16_t>(x1);
            s.needle1Y[m] = static_cast<int16_t>(y1);
        }
    }

    // Layer ladders; the top two segments light white.
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        const float lvl = (s.layer[i] > (s.flash[i] * 0.9f)) ? s.layer[i] : (s.flash[i] * 0.9f);
        segBar(d, 52 + (i * 88), 518, -1, 44, 9, 2, s.ladder[i], static_cast<uint8_t>(lvl * 10.0f),
               [&](int k) { return (k >= 8) ? WHITE : themeColor(t, i * 25u, static_cast<uint8_t>(120 + (k * 13))); });
    }
}

void Controller::visual_sceneRain(bool init_) {
    /// @brief Digital rain: 64 columns of falling characters, one per frequency band. Loud bands
    /// fall fast with long trails; a hit sends a burst down its layer's columns.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    static const char kGlyphs[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ<>+:-/?!#%";
    constexpr uint8_t kGlyphCount = sizeof(kGlyphs) - 1;
    auto cell = [&](uint8_t c, int r, uint16_t color) {
        if ((r < 0) || (r >= kRainRows)) return;
        d.setForeColor(color);
        d.drawText(&s.rainChar[c][r], 1, static_cast<uint16_t>((c * 15) + 2), static_cast<uint16_t>(16 + (r * 15)));
    };

    lcd.setFont(FONT_10x14);
    lcd.setAlignment(LEFT);
    lcd.setBackColor(BLACK);
    if (init_) {
        for (uint8_t c = 0; c < kRainCols; c++) {
            for (uint8_t r = 0; r < kRainRows; r++) s.rainChar[c][r] = kGlyphs[s.rng.next() % kGlyphCount];
            s.rainHead[c] = -static_cast<float>(s.rng.range(0, 30));
            s.rainDrawn[c] = static_cast<int16_t>(s.rainHead[c]);
            s.rainLen[c] = static_cast<uint8_t>(s.rng.range(6, 14));
            s.rainBoost[c] = 0.0f;
        }
        return;
    }

    for (uint8_t i = 0; i < vis::kLayers; i++) {  // a hit: a burst in the layer's columns
        if (s.hitNow[i] <= 0.0f) continue;
        const int c = clampi((i * 6) + s.rng.range(0, 5), 0, kRainCols - 1);
        s.rainBoost[c] = 1.2f + s.hitNow[i];
    }
    for (uint8_t c = 0; c < kRainCols; c++) {
        s.rainHead[c] += 0.15f + (s.band64[c] * 1.1f) + s.rainBoost[c];
        s.rainBoost[c] *= 0.9f;
        const int target = static_cast<int>(s.rainHead[c]);
        while (s.rainDrawn[c] < target) {
            const int h = ++s.rainDrawn[c];
            cell(c, h - 1, themeColor(t, c * 4u, 230));  // the old head joins the trail...
            cell(c, h - 5, themeColor(t, c * 4u, 100));  // ...which dims further up
            if ((h >= 0) && (h < kRainRows)) {
                s.rainChar[c][h] = kGlyphs[s.rng.next() % kGlyphCount];
                cell(c, h, WHITE);                        // the new head shines
            }
            const int tail = h - s.rainLen[c];
            if ((tail >= 0) && (tail < kRainRows)) box(d, c * 15, 16 + (tail * 15), 15, 15, BLACK);
            if (tail >= kRainRows) {  // this drop is gone: a new one, longer when the band is loud
                s.rainHead[c] = -static_cast<float>(s.rng.range(0, 12));
                s.rainDrawn[c] = static_cast<int16_t>(s.rainHead[c]);
                s.rainLen[c] = static_cast<uint8_t>(6 + s.rng.range(0, 6) + static_cast<int>(s.band64[c] * 14.0f));
                break;
            }
        }
    }
}

void Controller::visual_sceneFireworks(bool init_) {
    /// @brief Every hit launches a rocket from its layer's place on the ground; it bursts at the
    /// top into sparks in the layer's colour. Accents fly higher and burst bigger.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;

    if (init_) {
        for (uint16_t i = 0; i < kSparks; i++) s.sparks[i].on = false;
        for (uint8_t i = 0; i < kRockets; i++) s.rockets[i].on = false;
        s.lastRocketMs = HAL_GetTick();
        return;
    }

    auto launch = [&](int x, float power, uint16_t c) {
        for (uint8_t k = 0; k < kRockets; k++) {
            Rocket& r = s.rockets[k];
            if (r.on) continue;
            r = Rocket{static_cast<float>(x), 530.0f, -(8.0f + (power * 4.5f) + (s.bass * 2.5f)), power, 0, 0, c, true, false};
            return;
        }
    };
    const uint32_t now = HAL_GetTick();
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        if (s.hitNow[i] <= 0.0f) continue;
        launch(48 + (i * 96) + s.rng.range(-24, 24), s.hitNow[i], themeColor(t, i * 25u));
        s.lastRocketMs = now;
    }
    if ((now - s.lastRocketMs) > 1500) {  // quiet: now and then one on its own
        launch(s.rng.range(80, kW - 80), 0.6f, themeColor(t, s.rng.next()));
        s.lastRocketMs = now;
    }

    for (uint8_t k = 0; k < kRockets; k++) {
        Rocket& r = s.rockets[k];
        if (!r.on) continue;
        if (r.drawn) box(d, r.sx - 1, r.sy - 2, 3, 5, BLACK);
        r.y += r.vy;
        r.vy += 0.26f;
        if ((r.vy >= -0.6f) || (r.y < 40.0f)) {  // the burst
            const int n = 18 + static_cast<int>(r.power * 40.0f);
            for (int p = 0, made = 0; (p < kSparks) && (made < n); p++) {
                Spark& sp = s.sparks[p];
                if (sp.on) continue;
                const float a = static_cast<float>(s.rng.range(0, 6283)) / 1000.0f;
                const float v = (static_cast<float>(s.rng.range(15, 65)) / 10.0f) * (0.6f + (r.power * 0.6f));
                sp = Spark{r.x, r.y, v * cosf(a), v * sinf(a), 0, 0, (s.rng.range(0, 5) == 0) ? WHITE : r.c,
                           static_cast<uint8_t>(s.rng.range(25, 50)), 0, true, false};
                sp.maxLife = sp.life;
                made++;
            }
            r.on = false;
            continue;
        }
        r.sx = static_cast<int16_t>(r.x);
        r.sy = static_cast<int16_t>(r.y);
        box(d, r.sx - 1, r.sy - 2, 3, 5, WHITE);
        r.drawn = true;
    }

    for (uint16_t p = 0; p < kSparks; p++) {
        Spark& sp = s.sparks[p];
        if (!sp.on) continue;
        if (sp.drawn) box(d, sp.sx, sp.sy, 2, 2, BLACK);
        sp.vy += 0.09f;
        sp.vx *= 0.985f;
        sp.vy *= 0.985f;
        sp.x += sp.vx;
        sp.y += sp.vy;
        if ((--sp.life == 0) || (sp.x < 0.0f) || (sp.x >= kW - 2) || (sp.y < kHintH) || (sp.y >= kH - 2)) {
            sp.on = false;
            continue;
        }
        sp.sx = static_cast<int16_t>(sp.x);
        sp.sy = static_cast<int16_t>(sp.y);
        const uint8_t v = static_cast<uint8_t>(40 + ((sp.life * 215) / sp.maxLife));
        box(d, sp.sx, sp.sy, 2, 2, vis::dim(sp.c, v));
        sp.drawn = true;
    }
}

void Controller::visual_sceneMandala(bool init_) {
    /// @brief A kaleidoscope: the waveform (outer ring) and the spectrum (inner ring) mirrored
    /// into eight wedges around the centre, slowly turning; louder = wider and faster.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCx = 480, kCy = 278;
    constexpr float kWedge = 0.785398f;

    if (init_) {
        s.mDrawn = false;
        s.mGain = 1.0f;
        s.glowR = 0;
        return;
    }

    if (s.mDrawn) {
        for (uint16_t i = 0; i < kMandalaPoints; i++) box(d, s.mx[i] - 1, s.my[i] - 1, 3, 3, BLACK);
    }
    s.mRot += 0.008f + (s.energy * 0.04f);

    const uint32_t end = visTap.pos;
    const uint32_t start = end - (kMandalaWave * 6) - 1;
    float peak = 0.02f;
    for (uint16_t k = 0; k < kMandalaWave * 6; k++) {
        const float a = fabsf(mono(start + k));
        if (a > peak) peak = a;
    }
    float target = 1.0f / peak;
    if (target > 10.0f) target = 10.0f;
    s.mGain += (target - s.mGain) * 0.2f;

    uint16_t n = 0;
    for (uint8_t w = 0; w < 8; w++) {
        const float base = (w * kWedge) + s.mRot;
        const bool mirror = (w & 1) != 0;
        for (uint8_t k = 0; k < kMandalaWave; k++) {
            float a = ((k + 0.5f) / kMandalaWave) * kWedge;
            if (mirror) a = kWedge - a;
            const float r = 120.0f + (fabsf(mono(start + (k * 6))) * s.mGain * 130.0f);
            s.mx[n] = static_cast<int16_t>(clampi(kCx + static_cast<int>(r * cosf(base + a)), 1, kW - 2));
            s.my[n] = static_cast<int16_t>(clampi(kCy + static_cast<int>(r * sinf(base + a)), kHintH + 1, kH - 2));
            box(d, s.mx[n] - 1, s.my[n] - 1, 3, 3, themeColor(t, (k * 5u) + visualFrame));
            n++;
        }
        for (uint8_t k = 0; k < kMandalaBands; k++) {
            float a = ((k + 0.5f) / kMandalaBands) * kWedge;
            if (mirror) a = kWedge - a;
            const float r = 24.0f + (s.band32[k] * 95.0f);
            s.mx[n] = static_cast<int16_t>(kCx + static_cast<int>(r * cosf(base - a)));
            s.my[n] = static_cast<int16_t>(kCy + static_cast<int>(r * sinf(base - a)));
            box(d, s.mx[n] - 1, s.my[n] - 1, 3, 3, themeColor(t, 128u + (k * 4u), static_cast<uint8_t>(120 + (s.band32[k] * 135.0f))));
            n++;
        }
    }
    s.mDrawn = true;

    const int16_t glow = static_cast<int16_t>(4 + (s.bass * 16.0f));
    if (glow < s.glowR) disc(d, kCx, kCy, s.glowR, BLACK);
    disc(d, kCx, kCy, glow, WHITE);
    s.glowR = glow;
}

void Controller::visual_sceneDisco(bool init_) {
    /// @brief A light-up dance floor: patterns flash on every eighth note (a checkerboard on each
    /// bar), every hit lights a tile in its layer's part of the floor, and tiles fade out.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr uint8_t kTiles = kDiscoCols * kDiscoRows;
    const uint16_t dark = vis::rgb(18, 18, 26);
    auto tileBox = [&](uint8_t i, uint16_t c) { box(d, ((i % kDiscoCols) * 60) + 2, 40 + ((i / kDiscoCols) * 60) + 2, 56, 56, c); };

    if (init_) {
        for (uint8_t i = 0; i < kTiles; i++) {
            s.tileLvl[i] = 0.0f;
            s.tileDrawn[i] = 0;
            s.tileHue[i] = 0;
            tileBox(i, dark);
        }
        s.lastStep = s.step8;
        s.lastSpawnMs = HAL_GetTick();
        return;
    }

    auto light = [&](uint8_t i, float lvl, uint8_t hue) {
        if (lvl > s.tileLvl[i]) {
            s.tileLvl[i] = lvl;
            s.tileHue[i] = hue;
        }
    };
    const uint32_t now = HAL_GetTick();
    bool step = false;
    if (s.playing) {
        step = (s.step8 != s.lastStep);
    } else {
        step = ((now - s.lastSpawnMs) > 400) || (((now - s.lastSpawnMs) > 150) && (s.bass > 0.6f));
    }
    if (step) {
        s.lastStep = s.step8;
        s.lastSpawnMs = now;
        const uint8_t hue = static_cast<uint8_t>(s.step8 * 19u);
        if (s.barStart && ((s.step8 & 1u) == 0)) {  // the bar: a checkerboard
            for (uint8_t i = 0; i < kTiles; i++) {
                if ((((i % kDiscoCols) + (i / kDiscoCols) + (s.beatIdx / 4u)) & 1u) == 0) light(i, 1.0f, hue);
            }
        } else {
            switch (s.step8 % 4) {
                case 0:  // a column sweeps across
                    for (uint8_t r = 0; r < kDiscoRows; r++) light(static_cast<uint8_t>((r * kDiscoCols) + ((s.step8 / 4) % kDiscoCols)), 0.9f, hue);
                    break;
                case 2:  // a diagonal
                    for (uint8_t r = 0; r < kDiscoRows; r++) light(static_cast<uint8_t>((r * kDiscoCols) + ((r + s.step8) % kDiscoCols)), 0.8f, hue);
                    break;
                default:  // sparkles
                    for (uint8_t k = 0; k < 6; k++) light(static_cast<uint8_t>(s.rng.range(0, kTiles - 1)), 0.7f + (s.energy * 0.3f), static_cast<uint8_t>(hue + (k * 30)));
                    break;
            }
        }
    }
    for (uint8_t i = 0; i < vis::kLayers; i++) {  // hits: the layer's part of the floor
        if (s.hitNow[i] <= 0.0f) continue;
        const uint8_t col = static_cast<uint8_t>(clampi(static_cast<int>((i * 1.6f) + s.rng.range(0, 1)), 0, kDiscoCols - 1));
        light(static_cast<uint8_t>((s.rng.range(0, kDiscoRows - 1) * kDiscoCols) + col), s.hitNow[i], static_cast<uint8_t>(i * 25));
    }

    for (uint8_t i = 0; i < kTiles; i++) {
        s.tileLvl[i] *= 0.86f;
        const uint8_t q = static_cast<uint8_t>(s.tileLvl[i] * 7.99f);
        if (q == s.tileDrawn[i]) continue;
        tileBox(i, (q == 0) ? dark : themeColor(t, s.tileHue[i], static_cast<uint8_t>(40 + (q * 30))));
        s.tileDrawn[i] = q;
    }
}

void Controller::visual_sceneRidges(bool init_) {
    /// @brief Ridge lines of the spectrum's recent past, stacked like mountains (the famous album
    /// cover look): the newest line at the front, older ones moving back and up.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kX0 = 192, kStep = 9, kAmp = 110;

    if (init_) {
        memset(s.ridge, 0, sizeof(s.ridge));
        s.ridgeHead = 0;
        s.ridgeCount = 0;
        return;
    }
    if ((visualFrame & 1u) != 0) return;  // a new line every other frame

    s.ridgeHead = static_cast<uint8_t>((s.ridgeHead + 1) % kRidges);
    for (uint8_t b = 0; b < 64; b++) s.ridge[s.ridgeHead][b] = to8(s.band64[b]);
    if (s.ridgeCount < kRidges) s.ridgeCount++;

    int16_t y[64];
    for (int j = kRidges - 1; j >= 0; j--) {  // back to front: each line hides what is behind it
        const uint8_t idx = static_cast<uint8_t>((s.ridgeHead + kRidges - j) % kRidges);
        const int baseline = 512 - (j * 22);
        for (uint8_t b = 0; b < 64; b++) {
            const float win = sinf(3.14159f * (b + 0.5f) / 64.0f);
            const float h = (j < s.ridgeCount) ? ((s.ridge[idx][b] / 255.0f) * kAmp * win * win) : 0.0f;
            y[b] = static_cast<int16_t>(baseline - static_cast<int>(h) - static_cast<int>(s.rng.range(0, 2) * win));
        }
        const uint16_t c = (j == 0) ? WHITE : vis::dim(themeColor(t, j * 12u), static_cast<uint8_t>(255 - (j * 9)));
        for (uint8_t b = 0; b < 63; b++) {
            const int x = kX0 + (b * kStep);
            const int top = (j == kRidges - 1) ? kHintH : ((y[b] < y[b + 1]) ? y[b] : y[b + 1]);
            const int bottom = (j == 0) ? (kH - 1) : (baseline + 21);
            box(d, x, top, kStep, bottom - top + 1, BLACK);
            line(d, x, y[b], x + kStep, y[b + 1], c);
        }
    }
}

void Controller::visual_sceneLife(bool init_) {
    /// @brief Conway's Game of Life, one generation per eighth note. Every hit drops a glider
    /// (an accent: a wild R-pentomino) into its layer's part of the world; newborn cells flash.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kTotal = kLifeW * kLifeH;
    auto bucket = [](uint8_t age) -> uint8_t { return (age == 0) ? 0 : ((age == 1) ? 1 : ((age < 5) ? 2 : 3)); };
    auto colorOf = [&](uint8_t b) -> uint16_t {
        return (b == 0) ? BLACK : ((b == 1) ? WHITE : ((b == 2) ? themeColor(t, 40) : themeColor(t, 170, 150)));
    };
    auto cellBox = [&](int x, int y, uint16_t c) { box(d, x * 10, 16 + (y * 10), 9, 9, c); };
    auto set = [&](int x, int y) {
        x = (x + kLifeW) % kLifeW;
        y = (y + kLifeH) % kLifeH;
        uint8_t& v = s.life[(y * kLifeW) + x];
        if (v == 0) {
            v = 1;
            cellBox(x, y, WHITE);
        }
    };

    if (init_) {
        memset(s.life, 0, sizeof(s.life));
        for (int i = 0; i < kTotal; i++) {
            if (s.rng.range(0, 99) < 14) {
                s.life[i] = 1;
                cellBox(i % kLifeW, i / kLifeW, WHITE);
            }
        }
        s.lastStep = s.step8;
        s.lastSpawnMs = HAL_GetTick();
        return;
    }

    for (uint8_t i = 0; i < vis::kLayers; i++) {  // hits drop shapes into the layer's part
        if (s.hitNow[i] <= 0.0f) continue;
        const int x = static_cast<int>(i * 9.6f) + s.rng.range(0, 8);
        const int y = s.rng.range(0, kLifeH - 1);
        if (s.hitNow[i] > 0.9f) {  // R-pentomino
            set(x + 1, y); set(x + 2, y); set(x, y + 1); set(x + 1, y + 1); set(x + 1, y + 2);
        } else {                    // a glider, heading a random way
            const int dx = s.rng.range(0, 1) ? 1 : -1;
            const int dy = s.rng.range(0, 1) ? 1 : -1;
            set(x + dx, y); set(x + (2 * dx), y + dy); set(x, y + (2 * dy)); set(x + dx, y + (2 * dy)); set(x + (2 * dx), y + (2 * dy));
        }
    }

    const uint32_t now = HAL_GetTick();
    bool step;
    if (s.playing) {
        step = (s.step8 != s.lastStep);
    } else {
        step = (now - s.lastSpawnMs) > 180;
    }
    if (!step) return;
    s.lastStep = s.step8;
    s.lastSpawnMs = now;

    int population = 0;
    for (int y = 0; y < kLifeH; y++) {
        const int yu = ((y + kLifeH - 1) % kLifeH) * kLifeW;
        const int yc = y * kLifeW;
        const int yd = ((y + 1) % kLifeH) * kLifeW;
        for (int x = 0; x < kLifeW; x++) {
            const int xl = (x + kLifeW - 1) % kLifeW;
            const int xr = (x + 1) % kLifeW;
            const int n = (s.life[yu + xl] != 0) + (s.life[yu + x] != 0) + (s.life[yu + xr] != 0) + (s.life[yc + xl] != 0) +
                          (s.life[yc + xr] != 0) + (s.life[yd + xl] != 0) + (s.life[yd + x] != 0) + (s.life[yd + xr] != 0);
            const uint8_t old = s.life[yc + x];
            uint8_t next = 0;
            if (old != 0) {
                if ((n == 2) || (n == 3)) next = (old < 255) ? static_cast<uint8_t>(old + 1) : old;
            } else if (n == 3) {
                next = 1;
            }
            s.lifeNext[yc + x] = next;
            if (next != 0) population++;
            if (bucket(next) != bucket(old)) cellBox(x, y, colorOf(bucket(next)));
        }
    }
    memcpy(s.life, s.lifeNext, sizeof(s.life));
    if (population < 25) {  // nearly extinct: new life rains in
        for (int k = 0; k < 120; k++) set(s.rng.range(0, kLifeW - 1), s.rng.range(0, kLifeH - 1));
    }
}
