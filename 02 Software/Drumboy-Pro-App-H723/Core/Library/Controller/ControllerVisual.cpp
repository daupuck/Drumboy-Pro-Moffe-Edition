// VISUALS mode (v1.14): full-screen music visuals.
// Part of class Controller (declared in Controller.h).
//
// HOME (CENTER) on the home screen turns it on. LEFT / RIGHT pick one of 16 styles, UP / DOWN one
// of 9 colour themes, ADD toggles AUTO, CLEAR the INFO overlay (v2.06), HOME turns it off again;
// any key on the menu panel leaves and opens its menu. Pads, transport, MUTE/FILL/STYLE and the
// home-screen knobs keep working.
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

// v2.06/v2.07: media-player classics, crazy characters, retro demo styles and music views.
enum Scene : uint8_t {
    SC_CLASSIC = 0, SC_SHOWTIME, SC_SUPERSCOPE, SC_GEISS, SC_MILK, SC_MOUTH, SC_EYEBALLS, SC_SPEAKER, SC_ROBOT, SC_CROWD,
    SC_DEMO, SC_SUNSET, SC_STEPGRID, SC_PIANO, SC_KEYWHEEL, SC_SPECTRO, SC_KALEIDO, SC_ORBIT, SC_RIDGES, SC_LIFE, kSceneCount
};
const char* const kSceneNames[kSceneCount] = {"CLASSIC", "SHOWTIME", "SUPERSCOPE", "GEISS", "MILK TUNNEL", "MOUTH", "EYEBALLS", "SPEAKER",
                                              "ROBOT DJ", "DISCO CROWD", "DEMO SCENE", "SUNSET DRIVE", "STEP GRID", "PIANO ROLL", "KEY WHEEL",
                                              "SPECTRO", "KALEIDO", "ORBIT", "RIDGES", "LIFE"};
// GEISS and MILK TUNNEL draw a 160 x 88 buffer as 6 x 6 blocks under the hint band.
constexpr int kPixW = 160, kPixH = 88, kPixScale = 6, kPixY = 12;
constexpr uint16_t kRollNotes = 200;  // PIANO ROLL
constexpr uint8_t kThemeCount = 9;
const char* const kThemeNames[kThemeCount] = {"GOLD RUSH", "SYNTHWAVE", "ACID", "OCEAN", "FIRE", "ICE", "RAINBOW", "WINAMP", "C64"};
const uint8_t kThemeBase[kThemeCount] = {20, 213, 60, 110, 0, 140, 0, 85, 150};   // GOLD RUSH: orange to gold
const int16_t kThemeSpread[kThemeCount] = {26, 100, 50, 70, 45, 50, 255, -85, 60};  // WINAMP: green -> yellow -> red
constexpr uint8_t kThemeWinamp = 7;

constexpr uint32_t kAutoBars = 16;        // AUTO: a new style every 16 bars while playing...
constexpr uint32_t kAutoStoppedMs = 20000; // ...or every 20 s when stopped

constexpr uint16_t kScopeCols = 432;    // 2 px each, x 48..911
constexpr uint16_t kSuperPoints = 160;  // SUPERSCOPE
constexpr uint8_t kMouthCols = 120;     // MOUTH: 6 px columns
constexpr uint8_t kTeeth = 10;
constexpr uint8_t kEyes = 10;
constexpr uint8_t kGlitter = 96;        // SPEAKER
constexpr uint8_t kFizz = 16;
constexpr uint8_t kGridCols = 64;       // STEP GRID
constexpr uint8_t kCopperRows = 220;    // DEMO SCENE: rows 180..399
constexpr uint8_t kScrollSlots = 28;
constexpr uint8_t kDemoStars = 48;
constexpr uint8_t kSunRows = 150;       // SUNSET DRIVE: rows above the horizon the sun can cover
constexpr uint8_t kTwinkles = 28;
constexpr uint8_t kRipples = 12;
constexpr uint8_t kFloorLines = 6;
constexpr uint8_t kFloorVerticals = 17;
constexpr int16_t kNone = 0x7FFF;

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
struct RollNote {  // PIANO ROLL
    uint16_t start, len;
    int8_t semis;
    uint8_t layer, drawn;
    bool outline;
    float flash;
};
// The pixel buffers of GEISS (two frames) and MILK TUNNEL (tables + output) share memory.
union PixelBuffers {
    struct {
        uint8_t a[kPixW * kPixH];
        uint8_t b[kPixW * kPixH];
    } g;
    struct {
        uint8_t dist[kPixW * kPixH];
        uint8_t angle[kPixW * kPixH];
        uint8_t shade[kPixW * kPixH];
        uint8_t out[kPixW * kPixH];
    } t;
};

struct Spark {
    float x, y, vx, vy;
    int16_t sx, sy;
    uint16_t c;
    uint8_t life, maxLife;
    bool on, drawn;
};
struct Eye {
    int16_t r, ix, iy, lid;  // drawn: white radius, iris centre, lid depth
    uint16_t iris;
    bool veins, drawn;
    float lookX, lookY, pop;
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
    // SUPERSCOPE
    int16_t spx[kSuperPoints], spy[kSuperPoints];
    bool spDrawn;
    uint8_t spShape;
    uint32_t spBar;
    float spGain, spAx, spAy;
    // CLASSIC
    uint32_t clMs, clLastSec;
    int16_t clThumb, clVol;
    uint16_t clScroll;
    int8_t clPlay;
    uint8_t clTempo, clBank;
    // MOUTH
    int16_t mouthTop[kMouthCols], mouthBot[kMouthCols];
    int16_t mouthH;
    int16_t toothLen[2 * kTeeth];
    int16_t tongueX, tongueY, tongueR;
    // EYEBALLS
    Eye eyes[kEyes];
    float blink;
    uint32_t lastBlinkMs;
    // SPEAKER
    int8_t coneQ;
    int16_t capR, ringR[6], portR;
    uint8_t fizzOn[kFizz];
    // STEP GRID
    uint8_t gridDrawn[vis::kLayers][kGridCols];
    float gridFlash[vis::kLayers][kGridCols];
    uint64_t gridMap[vis::kLayers];
    uint8_t gridCols;
    int16_t gridHead;
    uint32_t gridStep;
    // DEMO SCENE
    uint16_t copper[kCopperRows];
    int16_t scrX[kScrollSlots], scrY[kScrollSlots];
    char scrText[160];
    uint16_t scrLen;
    uint32_t scrPos;
    int16_t logoX, logoY;
    bool logoDrawn;
    float starX[kDemoStars];
    int16_t starY[kDemoStars], starDX[kDemoStars];
    uint8_t starSpeed[kDemoStars];
    bool starOn[kDemoStars];
    // SUNSET DRIVE
    int16_t sunHw[kSunRows];
    uint8_t sunGap[kSunRows];
    int16_t sunR;
    int16_t twX[kTwinkles], twY[kTwinkles];
    uint8_t twOn[kTwinkles];
    uint8_t horizonLvl;
    // SPECTRO
    uint16_t col;
    uint32_t lastBeatIdx;
    // Shared timers / glow
    uint32_t lastStep, lastSpawnMs;
    int16_t glowR;
    // GEISS / MILK TUNNEL (v2.07)
    PixelBuffers pix;
    uint16_t pal[256];
    uint8_t gFlip;
    float gTime, gFlash, tShift, tRot;
    // ROBOT DJ
    uint8_t eqSeg[8];
    int16_t headY, arm[2][6], laser[2][4];
    bool armDrawn[2];
    uint8_t laserLife[2], scratch, armsUp;
    float platAngle;
    // DISCO CROWD
    float diverX;
    int16_t diverDX;
    bool diverOn, diverDrawn;
    // PIANO ROLL
    RollNote roll[kRollNotes];
    uint16_t rollCount;
    uint32_t rollSig;
    int16_t rollLo, rollHi, rollHead;
    int8_t rollTr;
    uint8_t rollBanner;
    // KEY WHEEL
    uint8_t wheelLit[24];
    float needleA;
    int16_t needle[4];
    bool needleDrawn;
    char keyText[64];
    // SPEAKER glitter
    Spark sparks[kGlitter];
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
    const int pos = static_cast<int>(pos_ & 0xFF);
    return vis::hue(static_cast<uint8_t>(kThemeBase[theme_] + ((pos * kThemeSpread[theme_]) / 256)), value_);
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
                // v2.06: ADD = AUTO (a new style every 16 bars), CLEAR = the info overlay. With
                // MUTE / FILL / STYLE held they keep their normal meaning (e.g. MUTE + ADD).
                case KEY_ADD:
                    if (keyboard.muteKeyPress || keyboard.fillKeyPress || keyboard.styleKeyPress) {
                        taken = false;
                    } else {
                        visualAuto = !visualAuto;
                        visualAutoBars = 0;
                        visualAutoMs = HAL_GetTick();
                    }
                    break;
                case KEY_CLEAR:
                    if (keyboard.muteKeyPress || keyboard.fillKeyPress || keyboard.styleKeyPress) {
                        taken = false;
                    } else {
                        visualOverlay = !visualOverlay;
                        if (!visualOverlay) visualSceneReady = false;  // repaint without it
                    }
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

    // AUTO (v2.06, the Winamp/MilkDrop classic): a new random style every kAutoBars bars, right
    // on the bar, or every kAutoStoppedMs when stopped.
    if (visualAuto) {
        const uint32_t bar = vs.beatIdx / beatsPerBar;
        bool next = false;
        if (vs.playing) {
            if (bar != visualLastBar) {
                visualLastBar = bar;
                if (++visualAutoBars >= kAutoBars) next = true;
            }
        } else if ((now - visualAutoMs) >= kAutoStoppedMs) {
            next = true;
        }
        if (next) {
            visualAutoBars = 0;
            visualAutoMs = now;
            const uint8_t step = static_cast<uint8_t>(1 + (vs.rng.next() % (kSceneCount - 1)));
            visualScene = static_cast<uint8_t>((visualScene + step) % kSceneCount);
            visualSceneReady = false;
            visualHintUntil = now + kHintMs;
            visualHintShown = false;
        }
    }

    if (!visualSceneReady) {
        visual_initScene();
        visualSceneReady = true;
    }
    visual_analyze();
    visual_drawScene();
    // (GEISS and MILK TUNNEL repaint the whole screen every frame, so the overlay goes on top each time.)
    if (visualOverlay && (((visualFrame % 6) == 0) || (visualScene == SC_GEISS) || (visualScene == SC_MILK))) visual_drawOverlay();

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
    char text[140];
    // Longest case is 104 characters = 936 px of the 960.
    snprintf(text, sizeof(text), "VISUALS %u/%u  %s - %s%s  <> STYLE  UP/DN COLOUR  ADD AUTO  CLEAR INFO  HOME EXIT",
             static_cast<unsigned>(visualScene + 1), static_cast<unsigned>(kSceneCount), kSceneNames[visualScene], kThemeNames[visualTheme],
             visualAuto ? "  [AUTO]" : "");
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
    visual_runScene(true);
}

void Controller::visual_drawScene() { visual_runScene(false); }

void Controller::visual_runScene(bool init_) {
    switch (visualScene) {
        case SC_CLASSIC: visual_sceneClassic(init_); break;
        case SC_SHOWTIME: visual_sceneShowtime(init_); break;
        case SC_SUPERSCOPE: visual_sceneSuperscope(init_); break;
        case SC_GEISS: visual_sceneGeiss(init_); break;
        case SC_MILK: visual_sceneMilk(init_); break;
        case SC_MOUTH: visual_sceneMouth(init_); break;
        case SC_EYEBALLS: visual_sceneEyeballs(init_); break;
        case SC_SPEAKER: visual_sceneSpeaker(init_); break;
        case SC_ROBOT: visual_sceneRobot(init_); break;
        case SC_CROWD: visual_sceneCrowd(init_); break;
        case SC_DEMO: visual_sceneDemo(init_); break;
        case SC_SUNSET: visual_sceneSunset(init_); break;
        case SC_STEPGRID: visual_sceneStepGrid(init_); break;
        case SC_PIANO: visual_scenePiano(init_); break;
        case SC_KEYWHEEL: visual_sceneKeyWheel(init_); break;
        case SC_SPECTRO: visual_sceneSpectro(init_); break;
        case SC_KALEIDO: visual_sceneMandala(init_); break;
        case SC_ORBIT: visual_sceneOrbit(init_); break;
        case SC_RIDGES: visual_sceneRidges(init_); break;
        default: visual_sceneLife(init_); break;
    }
}

void Controller::visual_drawOverlay() {
    /// @brief CLEAR in VISUALS (v2.06): tempo, bar.beat, song step and the project name in the
    /// bottom-right corner, over any style.

    const uint32_t beatsPerBar = (rhythm.measure > 0) ? rhythm.measure : 4;
    char name[pname::kLength + 1];
    memcpy(name, projectName, sizeof(name));
    for (int i = pname::kLength - 1; (i >= 0) && (name[i] == ' '); i--) name[i] = 0;  // trim
    char song[16] = "";
    if (songChainRunning) snprintf(song, sizeof(song), "  SONG %u/%u", static_cast<unsigned>(songPosition.step + 1), static_cast<unsigned>(songchain::length(songChain)));
    char text[64];
    snprintf(text, sizeof(text), " %s  %u BPM  BAR %u.%u%s ", (name[0] != 0) ? name : "MOFFE", static_cast<unsigned>(rhythm.tempo),
             static_cast<unsigned>((vs.beatIdx / beatsPerBar) + 1), static_cast<unsigned>((vs.beatIdx % beatsPerBar) + 1), song);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(RIGHT);
    lcd.setBackColor(BLACK);
    lcd.setForeColor(themeColor(visualTheme, 0));
    lcd.drawText(text, static_cast<uint16_t>(strlen(text)), kW - 4, kH - 12);
    lcd.setAlignment(LEFT);
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
        const uint16_t c = (t == 1) ? vis::heat(v) : themeColor(t, v, v);  // SYNTHWAVE: the classic heat map
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

////////////////////////////////////////////////////////////////////////////////
/* v2.06 styles --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

namespace {

inline void label(Lcd& d, const char* text, LcdFont font, LcdAlign align, int x, int y, uint16_t fg, uint16_t bg) {
    d.setFont(font);
    d.setAlignment(align);
    d.setBackColor(bg);
    d.setForeColor(fg);
    d.drawText(text, static_cast<uint16_t>(strlen(text)), static_cast<uint16_t>(x), static_cast<uint16_t>(y));
}

// Raised / sunken 1 px bevel, the Winamp 2 way.
inline void bevel(Lcd& d, int x, int y, int w, int h, bool raised, uint16_t light, uint16_t dark) {
    box(d, x, y, w, 1, raised ? light : dark);
    box(d, x, y, 1, h, raised ? light : dark);
    box(d, x, y + h - 1, w, 1, raised ? dark : light);
    box(d, x + w - 1, y, 1, h, raised ? dark : light);
}

// The sky of SUNSET DRIVE at row y (0 .. horizon): deep violet at the top to a hot orange glow.
inline uint16_t skyColor(int y, int horizon) {
    const float k = static_cast<float>(clampi(y, 0, horizon)) / static_cast<float>(horizon);
    if (k < 0.55f) {
        const float u = k / 0.55f;
        return vis::rgb(static_cast<uint8_t>(18 + (u * 100.0f)), 0, static_cast<uint8_t>(40 + (u * 70.0f)));
    }
    const float u = (k - 0.55f) / 0.45f;
    return vis::rgb(static_cast<uint8_t>(118 + (u * 137.0f)), static_cast<uint8_t>(u * 70.0f), static_cast<uint8_t>(110 - (u * 70.0f)));
}

// The sun's colour at row y of a sun of radius r centred on the horizon: yellow top, magenta base.
inline uint16_t sunColor(int y, int horizon, int r) {
    const float k = static_cast<float>(clampi(horizon - y, 0, r)) / static_cast<float>((r > 0) ? r : 1);  // 1 at the top
    return vis::rgb(255, static_cast<uint8_t>(40 + (k * 190.0f)), static_cast<uint8_t>(140 - (k * 100.0f)));
}

}  // namespace

void Controller::visual_sceneClassic(bool init_) {
    /// @brief CLASSIC: a tribute to the 1998 media-player look - a bevelled grey window with a
    /// ribbed title bar, an LCD with the time, a scrolling marquee and an oscilloscope, a big
    /// spectrum analyser with falling peak dots, a position bar and transport buttons.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    const uint16_t panel = vis::rgb(58, 58, 74);
    const uint16_t light = vis::rgb(140, 140, 160);
    const uint16_t dark = vis::rgb(16, 16, 24);
    const uint16_t titleC = vis::rgb(26, 26, 60);
    const uint16_t lcdC = (t == kThemeWinamp) ? vis::rgb(0, 235, 70) : themeColor(t, 0);
    constexpr int kWx = 40, kWy = 18, kWw = 880, kWh = 510;
    constexpr int kDx = 56, kDy = 56, kDw = 848, kDh = 146;
    constexpr int kAx = 66, kBase = 466, kSegs = 42;
    constexpr int kScX = 330, kScMid = 148, kScTop = 118, kScBot = 178, kScCols = 280;
    constexpr int kPosX = 72, kPosY = 186, kPosW = 816, kThumbW = 28;
    constexpr int kBtnY = 480;
    constexpr int kVolX = 640, kVolW = 250;

    if (init_) {
        box(d, kWx, kWy, kWw, kWh, panel);
        bevel(d, kWx, kWy, kWw, kWh, true, light, dark);
        // Title bar: ribs either side of the name, three little buttons on the right.
        box(d, kWx + 6, kWy + 5, kWw - 12, 15, titleC);
        for (int k = 0; k < 4; k++) {
            box(d, kWx + 14, kWy + 7 + (k * 3), 370, 1, themeColor(t, 40, 150));
            box(d, kWx + 496, kWy + 7 + (k * 3), 330, 1, themeColor(t, 40, 150));
        }
        label(d, "MOFFE AMP", FONT_07x09, CENTER, 480, kWy + 8, themeColor(t, 0), titleC);
        for (int k = 0; k < 3; k++) {
            box(d, kWx + 836 + (k * 13), kWy + 7, 10, 10, panel);
            bevel(d, kWx + 836 + (k * 13), kWy + 7, 10, 10, true, light, dark);
        }
        // The LCD and the analyser well.
        box(d, kDx, kDy, kDw, kDh, BLACK);
        bevel(d, kDx - 1, kDy - 1, kDw + 2, kDh + 2, false, light, dark);
        box(d, 56, 210, 848, 262, BLACK);
        bevel(d, 55, 209, 850, 264, false, light, dark);
        box(d, kPosX, kPosY + 4, kPosW, 4, dark);  // position groove
        // Buttons: prev, play, pause, stop, next, then SHUFFLE (= AUTO) and REPEAT (= song loop).
        label(d, "VOL", FONT_05x07, LEFT, kVolX - 30, kBtnY + 9, light, panel);
        box(d, kVolX, kBtnY + 10, kVolW, 4, dark);
        memset(s.seg, 0, sizeof(s.seg));
        memset(s.cap, 0xFF, sizeof(s.cap));
        memset(s.capPos, 0, sizeof(s.capPos));
        for (uint16_t c = 0; c < kScopeCols; c++) s.scopeTop[c] = kNone;
        s.clMs = 0;
        s.clLastSec = 0xFFFFFFFFu;
        s.clThumb = -1;
        s.clVol = -1;
        s.clPlay = -1;
        s.clScroll = 0;
        s.clTempo = 0;
        s.clBank = 0xFF;
        s.ladder[0] = s.ladder[1] = 0;
        s.scopeGain = 1.0f;
        s.lastSpawnMs = HAL_GetTick();
        return;
    }

    const uint32_t now = HAL_GetTick();
    const uint32_t dt = now - s.lastSpawnMs;
    s.lastSpawnMs = now;

    // 1. TRANSPORT GLYPH + TIME (elapsed while playing; back to 0:00 after a stop at the top).
    const int8_t state = s.playing ? 1 : ((playInterval == 0) ? 0 : 2);
    if (s.playing) s.clMs += dt;
    if ((state == 0) && !playActive) s.clMs = 0;
    const uint32_t sec = s.clMs / 1000u;
    const int8_t buttons = static_cast<int8_t>(state + (visualAuto ? 4 : 0) + ((songChain.mode != songchain::ONCE) ? 8 : 0));
    if (buttons != s.clPlay) {
        box(d, 66, 72, 22, 26, BLACK);
        if (state == 1) {
            d.setForeColor(lcdC);
            d.fillTriangle(68, 74, 68, 94, 86, 84);
        } else if (state == 2) {
            box(d, 68, 74, 6, 20, lcdC);
            box(d, 78, 74, 6, 20, lcdC);
        } else {
            box(d, 68, 76, 16, 16, lcdC);
        }
        static const char* const kBtn[5] = {"|<<", " > ", " ||", "[ ]", ">>|"};
        for (int k = 0; k < 5; k++) {
            const int x = 60 + (k * 52);
            box(d, x, kBtnY, 46, 26, panel);
            bevel(d, x, kBtnY, 46, 26, true, light, dark);
            const bool lit = ((k == 1) && (state == 1)) || ((k == 2) && (state == 2)) || ((k == 3) && (state == 0));
            label(d, kBtn[k], FONT_07x09, CENTER, x + 23, kBtnY + 9, lit ? themeColor(t, 0) : light, panel);
        }
        const char* const kSw[2] = {"SHUFFLE", "REPEAT"};
        for (int k = 0; k < 2; k++) {
            const int x = 336 + (k * 120);
            box(d, x, kBtnY, 110, 26, panel);
            bevel(d, x, kBtnY, 110, 26, true, light, dark);
            const bool lit = (k == 0) ? visualAuto : (songChain.mode != songchain::ONCE);
            box(d, x + 8, kBtnY + 10, 6, 6, lit ? vis::rgb(0, 255, 60) : dark);  // the little LED
            label(d, kSw[k], FONT_07x09, LEFT, x + 22, kBtnY + 9, light, panel);
        }
        s.clPlay = buttons;
    }
    if (sec != s.clLastSec) {
        char time[8];
        snprintf(time, sizeof(time), "%02u:%02u", static_cast<unsigned>((sec / 60u) % 100u), static_cast<unsigned>(sec % 60u));
        box(d, 96, 66, 186, 38, BLACK);
        d.setFont(FONT_14x18);
        d.setAlignment(LEFT);
        d.setForeColor(lcdC);
        d.drawTextScaled(time, 5, 96, 66, 2);
        s.clLastSec = sec;
    }

    // 2. MARQUEE + INFO LINE.
    if ((s.clTempo != rhythm.tempo) || (s.clBank != activeSongBank)) {
        char info[64];
        uint8_t active = 0;
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            if (layerLibrary[i].bankLibrary[activeSongBank].lastActiveBeatNum >= 0) active++;
        }
        snprintf(info, sizeof(info), "%3u BPM    44 KHZ    STEREO    BANK %u    %u/10 LAYERS", static_cast<unsigned>(rhythm.tempo),
                 static_cast<unsigned>(activeSongBank + 1), static_cast<unsigned>(active));
        box(d, kScX, 90, 560, 9, BLACK);
        label(d, info, FONT_05x07, LEFT, kScX, 91, lcdC, BLACK);
        s.clTempo = rhythm.tempo;
        s.clBank = activeSongBank;
    }
    if ((visualFrame % 4) == 0) {
        char name[pname::kLength + 1];
        memcpy(name, projectName, sizeof(name));
        for (int i = pname::kLength - 1; (i >= 0) && (name[i] == ' '); i--) name[i] = 0;
        char msg[96];
        const int n = snprintf(msg, sizeof(msg), "+++ %s +++ %u BPM +++ BANK %u +++ MOFFE EDITION +++ ", (name[0] != 0) ? name : "UNTITLED",
                               static_cast<unsigned>(rhythm.tempo), static_cast<unsigned>(activeSongBank + 1));
        if (n > 0) {
            const int len = (n < static_cast<int>(sizeof(msg))) ? n : static_cast<int>(sizeof(msg)) - 1;
            char view[64];
            for (int k = 0; k < 62; k++) view[k] = msg[(s.clScroll + k) % len];
            view[62] = 0;
            label(d, view, FONT_07x09, LEFT, kScX, 70, lcdC, BLACK);
            s.clScroll = static_cast<uint16_t>((s.clScroll + 1) % len);
        }
    }

    // 3. L / R LEVEL BARS under the time.
    for (uint8_t m = 0; m < 2; m++) {
        const uint8_t w = static_cast<uint8_t>(((m == 0) ? s.vuL : s.vuR) * 115.0f);  // x2 px
        const int y = 120 + (m * 26);
        if (w > s.ladder[m]) box(d, 70 + (s.ladder[m] * 2), y, (w - s.ladder[m]) * 2, 14, themeColor(t, w * 2u));
        if (w < s.ladder[m]) box(d, 70 + (w * 2), y, (s.ladder[m] - w) * 2, 14, BLACK);
        s.ladder[m] = w;
    }

    // 4. OSCILLOSCOPE in the LCD.
    {
        const uint32_t end = visTap.pos;
        uint32_t start = end - (kScCols * 2) - 1;
        for (uint32_t k = 0; k < 150; k++) {
            const uint32_t i = end - (kScCols * 2) - 2 - k;
            if ((mono(i) <= 0.0f) && (mono(i + 1) > 0.0f)) {
                start = i;
                break;
            }
        }
        float peak = 0.02f;
        for (uint16_t i = 0; i < kScCols * 2; i++) peak = fmaxf(peak, fabsf(mono(start + i)));
        s.scopeGain += (fminf(0.9f / peak, 6.0f) - s.scopeGain) * 0.2f;
        const float scale = s.scopeGain * 28.0f;
        for (uint16_t c = 0; c < kScCols; c++) {
            const int y = clampi(kScMid - static_cast<int>(mono(start + (2 * c)) * scale), kScTop, kScBot);
            if (y == s.scopeTop[c]) continue;
            const int x = kScX + (2 * c);
            if (s.scopeTop[c] != kNone) box(d, x, s.scopeTop[c], 2, 2, BLACK);
            box(d, x, y, 2, 2, (t == kThemeWinamp) ? vis::rgb(255, 210, 0) : themeColor(t, 128));
            s.scopeTop[c] = static_cast<int16_t>(y);
        }
    }

    // 5. POSITION BAR + VOLUME.
    const int thumb = (songInterval > 0) ? static_cast<int>((static_cast<float>(playInterval) / static_cast<float>(songInterval)) * (kPosW - kThumbW)) : 0;
    if (thumb != s.clThumb) {
        if (s.clThumb >= 0) {
            box(d, kPosX + s.clThumb, kPosY, kThumbW, 12, BLACK);
            box(d, kPosX + s.clThumb, kPosY + 4, kThumbW, 4, dark);
        }
        box(d, kPosX + thumb, kPosY, kThumbW, 12, themeColor(t, 0, 200));
        bevel(d, kPosX + thumb, kPosY, kThumbW, 12, true, WHITE, dark);
        s.clThumb = static_cast<int16_t>(thumb);
    }
    const int vol = (mixer.masterVolume * (kVolW - 18)) / kMaxMixerMasterVolume;
    if (vol != s.clVol) {
        if (s.clVol >= 0) {
            box(d, kVolX + s.clVol, kBtnY + 6, 18, 12, panel);
            box(d, kVolX + s.clVol, kBtnY + 10, 18, 4, dark);
        }
        box(d, kVolX + vol, kBtnY + 6, 18, 12, light);
        bevel(d, kVolX + vol, kBtnY + 6, 18, 12, true, WHITE, dark);
        s.clVol = static_cast<int16_t>(vol);
    }

    // 6. THE ANALYSER: 32 chunky bars, the colour climbing with the height, falling peak dots.
    for (uint8_t b = 0; b < 32; b++) {
        const int x = kAx + (b * 26);
        const uint8_t n = static_cast<uint8_t>(s.band32[b] * kSegs);
        segBar(d, x, kBase, -1, 22, 5, 1, s.seg[b], n, [&](int i) { return themeColor(t, static_cast<uint32_t>((i * 255) / kSegs)); });
        s.capPos[b] = (s.capPos[b] - 0.25f > n) ? (s.capPos[b] - 0.25f) : n;
        const uint8_t ci = static_cast<uint8_t>(s.capPos[b]);
        if ((s.cap[b] != ci) && (s.cap[b] >= n) && (s.cap[b] < kSegs)) box(d, x, kBase - (s.cap[b] * 6) - 3, 22, 2, BLACK);
        if ((ci >= n) && (ci < kSegs)) box(d, x, kBase - (ci * 6) - 3, 22, 2, light);
        s.cap[b] = ci;
    }
}

void Controller::visual_sceneSuperscope(bool init_) {
    /// @brief SUPERSCOPE: the waveform as a glowing 3D ribbon that slowly tumbles; every bar it
    /// takes a new shape (ring, spiral, figure eight, wave carpet), and the bass pumps its size.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCx = 480, kCy = 280;

    if (init_) {
        s.spDrawn = false;
        s.spGain = 1.0f;
        s.spBar = 0xFFFFFFFFu;
        s.spShape = 0;
        return;
    }
    const uint32_t beatsPerBar = (rhythm.measure > 0) ? rhythm.measure : 4;
    const uint32_t bar = s.beatIdx / beatsPerBar;
    if (bar != s.spBar) {
        if (s.spBar != 0xFFFFFFFFu) s.spShape = static_cast<uint8_t>((s.spShape + 1) % 4);
        s.spBar = bar;
    }

    const uint32_t end = visTap.pos;
    const uint32_t start = end - (kSuperPoints * 3) - 1;
    float peak = 0.02f;
    for (uint16_t i = 0; i < kSuperPoints; i++) peak = fmaxf(peak, fabsf(mono(start + (i * 3))));
    s.spGain += (fminf(1.0f / peak, 8.0f) - s.spGain) * 0.2f;
    s.spAx += 0.011f + (s.energy * 0.03f);
    s.spAy += 0.017f + (s.bass * 0.03f);
    const float cx = cosf(s.spAx), sx = sinf(s.spAx), cy = cosf(s.spAy), sy = sinf(s.spAy);
    const float size = 230.0f * (0.85f + (s.bass * 0.35f));

    int16_t nx[kSuperPoints], ny[kSuperPoints];
    for (uint16_t i = 0; i < kSuperPoints; i++) {
        const float w = mono(start + (i * 3)) * s.spGain;
        const float u = static_cast<float>(i) / static_cast<float>(kSuperPoints - 1);
        float x, y, z;
        switch (s.spShape) {
            case 0: {  // ring
                const float a = u * 6.2831853f;
                const float r = 0.62f + (w * 0.3f);
                x = r * cosf(a);
                y = r * sinf(a);
                z = w * 0.2f;
                break;
            }
            case 1: {  // spiral
                const float a = u * 18.85f;
                const float r = 0.12f + (u * 0.78f);
                x = r * cosf(a);
                y = r * sinf(a);
                z = w * 0.5f;
                break;
            }
            case 2: {  // figure eight
                const float a = u * 6.2831853f;
                x = (0.85f * sinf(a)) + (w * 0.15f);
                y = 0.55f * sinf(2.0f * a);
                z = w * 0.6f;
                break;
            }
            default: {  // wave carpet
                x = (u * 2.0f) - 1.0f;
                y = w * 0.7f;
                z = 0.45f * sinf((u * 12.566f) + (visualFrame * 0.05f));
                break;
            }
        }
        // Tumble: around Y, then X; then perspective.
        const float x1 = (x * cy) + (z * sy);
        const float z1 = (-x * sy) + (z * cy);
        const float y1 = (y * cx) - (z1 * sx);
        const float z2 = (y * sx) + (z1 * cx);
        const float p = size / (2.2f + z2);
        nx[i] = static_cast<int16_t>(clampi(kCx + static_cast<int>(x1 * p * 1.55f), 1, kW - 2));
        ny[i] = static_cast<int16_t>(clampi(kCy + static_cast<int>(y1 * p), kHintH + 1, kH - 2));
    }
    if (s.spDrawn) {
        for (uint16_t i = 1; i < kSuperPoints; i++) line(d, s.spx[i - 1], s.spy[i - 1], s.spx[i], s.spy[i], BLACK);
    }
    for (uint16_t i = 1; i < kSuperPoints; i++) {
        line(d, nx[i - 1], ny[i - 1], nx[i], ny[i], themeColor(t, static_cast<uint32_t>((i * 255) / kSuperPoints) + (visualFrame * 2u)));
    }
    memcpy(s.spx, nx, sizeof(nx));
    memcpy(s.spy, ny, sizeof(ny));
    s.spDrawn = true;
}

void Controller::visual_sceneMouth(bool init_) {
    /// @brief MOUTH: a huge cartoon mouth. The jaw opens with the bass; the ten upper teeth are
    /// the layers' meters (they stretch down with each layer and flash on its hits), the ten lower
    /// teeth are the spectrum; the tongue wobbles with the music.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCx = 480, kCy = 300, kHalfW = 360, kColW = 6, kLip = 26;
    const uint16_t lipC = vis::rgb(225, 30, 70);
    const uint16_t lipHi = vis::rgb(255, 110, 140);
    const uint16_t inside = vis::rgb(70, 0, 22);
    const uint16_t tongueC = vis::rgb(255, 105, 135);
    auto edge = [&](int c, int h, int& top, int& bot, int& lip) {
        const float dx = ((c * kColW) + (kColW / 2) - kHalfW) / static_cast<float>(kHalfW);
        const float e = sqrtf(fmaxf(0.0f, 1.0f - (dx * dx)));
        const int grin = static_cast<int>(34.0f * dx * dx);  // the corners turn up
        top = kCy - static_cast<int>(h * e) - grin;
        bot = kCy + static_cast<int>(h * e) - grin;
        lip = static_cast<int>(kLip * (0.35f + (0.65f * e)));
    };

    if (init_) {
        for (uint8_t c = 0; c < kMouthCols; c++) s.mouthTop[c] = kNone;
        for (uint8_t k = 0; k < 2 * kTeeth; k++) s.toothLen[k] = 0;
        s.mouthH = -1;
        s.tongueR = 0;
        return;
    }

    // 1. THE JAW (quantised, so the lips only move when the bass really changes).
    const int h = 18 + (static_cast<int>(s.bass * 26.0f) * 6);
    const bool moved = (h != s.mouthH);
    if (moved) {
        for (uint8_t c = 0; c < kMouthCols; c++) {
            int top, bot, lip;
            edge(c, h, top, bot, lip);
            const int x = (kCx - kHalfW) + (c * kColW);
            const int newTop = top - lip, newBot = bot + lip;
            if (s.mouthTop[c] != kNone) {  // what the old mouth covered and the new one does not
                if (s.mouthTop[c] < newTop) box(d, x, s.mouthTop[c], kColW, newTop - s.mouthTop[c], BLACK);
                if (s.mouthBot[c] > newBot) box(d, x, newBot, kColW, s.mouthBot[c] - newBot, BLACK);
            }
            box(d, x, newTop, kColW, lip, lipC);
            box(d, x, newTop + 2, kColW, 3, lipHi);  // a shine on the upper lip
            box(d, x, top, kColW, (bot - top) + 1, inside);
            box(d, x, bot + 1, kColW, lip, lipC);
            s.mouthTop[c] = static_cast<int16_t>(newTop);
            s.mouthBot[c] = static_cast<int16_t>(newBot);
        }
        s.mouthH = static_cast<int16_t>(h);
        s.tongueR = 0;  // repainted with the inside
        for (uint8_t k = 0; k < 2 * kTeeth; k++) s.toothLen[k] = 0;
    }

    // 2. THE TONGUE (only when the mouth is open enough).
    {
        const int r = (h > 60) ? clampi(20 + static_cast<int>(s.energy * 40.0f), 20, (h * 2) / 5) : 0;
        const int x = kCx + static_cast<int>(sinf(visualFrame * 0.09f) * 60.0f * (0.3f + s.energy));
        const int y = kCy + h - r - 6;
        if ((r != s.tongueR) || (x != s.tongueX) || (y != s.tongueY)) {
            if (s.tongueR > 0) disc(d, s.tongueX, s.tongueY, s.tongueR, inside);
            if (r > 0) {
                disc(d, x, y, r, tongueC);
                line(d, x, y - r + 4, x, y + (r / 2), vis::rgb(200, 60, 90));  // the middle crease
            }
            s.tongueR = static_cast<int16_t>(r);
            s.tongueX = static_cast<int16_t>(x);
            s.tongueY = static_cast<int16_t>(y);
            for (uint8_t k = kTeeth; k < 2 * kTeeth; k++) s.toothLen[k] = 0;  // lower teeth repainted
        }
    }

    // 3. THE TEETH: upper = layers (meters), lower = spectrum. Only the change is drawn.
    for (uint8_t k = 0; k < 2 * kTeeth; k++) {
        const bool upper = k < kTeeth;
        const uint8_t i = upper ? k : static_cast<uint8_t>(k - kTeeth);
        const int x = kCx - 252 + (i * 51);
        const int c = clampi((x + 20 - (kCx - kHalfW)) / kColW, 0, kMouthCols - 1);
        int top, bot, lip;
        edge(c, h, top, bot, lip);
        const int room = ((bot - top) / 2) - 4;
        if (room < 6) continue;
        const float lvl = upper ? fmaxf(s.layer[i], s.flash[i] * 0.9f) : s.band32[i * 3];
        const int len = clampi(6 + static_cast<int>(lvl * (room - 6)), 6, room);
        const uint16_t c0 = (upper && (s.flash[i] > 0.6f)) ? themeColor(t, i * 25u) : WHITE;
        const int y0 = upper ? (top + 1) : (bot - len);
        if (len < s.toothLen[k]) {
            if (upper) box(d, x, top + 1 + len, 40, s.toothLen[k] - len, inside);
            else box(d, x, bot - s.toothLen[k], 40, s.toothLen[k] - len, inside);
        }
        box(d, x, y0, 40, len, c0);
        box(d, x + 38, y0, 2, len, vis::rgb(200, 200, 210));  // a little shade on each tooth
        s.toothLen[k] = static_cast<int16_t>(len);
    }
}

void Controller::visual_sceneEyeballs(bool init_) {
    /// @brief EYEBALLS: ten eyes, one per layer. They pop wide on their layer's hits, all stare
    /// at the loudest layer, the pupils widen with the bass, they blink when it is quiet and turn
    /// bloodshot while the limiter works.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kR = 60, kIris = 25;
    auto centre = [](uint8_t i, int& x, int& y) {
        x = 112 + ((i % 5) * 184);
        y = (i < 5) ? 160 : 384;
    };

    if (init_) {
        for (uint8_t i = 0; i < kEyes; i++) {
            s.eyes[i] = Eye{};
            s.eyes[i].drawn = false;
        }
        s.blink = 0.0f;
        s.lastBlinkMs = HAL_GetTick();
        return;
    }

    // Who is the loudest? Everybody looks there (the loudest one looks at you).
    uint8_t loud = 0;
    for (uint8_t i = 1; i < kEyes; i++) {
        if (s.layer[i] > s.layer[loud]) loud = i;
    }
    int lx, ly;
    centre(loud, lx, ly);

    // Blinking: in the quiet parts, now and then all eyes close for a few frames.
    const uint32_t now = HAL_GetTick();
    if ((s.blink <= 0.0f) && (s.energy < 0.12f) && ((now - s.lastBlinkMs) > 2500) && ((s.rng.next() % 40) == 0)) {
        s.blink = 1.0f;
        s.lastBlinkMs = now;
    }
    const float lidAmount = (s.blink > 0.0f) ? (1.0f - fabsf((s.blink * 2.0f) - 1.0f)) : 0.0f;  // 0 -> 1 -> 0
    if (s.blink > 0.0f) s.blink -= 0.15f;
    const bool veins = limitAlertActive;

    for (uint8_t i = 0; i < kEyes; i++) {
        Eye& e = s.eyes[i];
        int x, y;
        centre(i, x, y);
        e.pop = fmaxf(s.hitNow[i], e.pop * 0.8f);
        const int r = kR + static_cast<int>(e.pop * 16.0f);
        float tx = 0.0f, ty = 0.0f;
        if (i != loud) {
            const float vx = static_cast<float>(lx - x), vy = static_cast<float>(ly - y);
            const float len = sqrtf((vx * vx) + (vy * vy));
            if (len > 1.0f) {
                tx = vx / len;
                ty = vy / len;
            }
        }
        e.lookX += (tx - e.lookX) * 0.3f;
        e.lookY += (ty - e.lookY) * 0.3f;
        const int reach = r - kIris - 6;
        const int ix = x + static_cast<int>(e.lookX * reach);
        const int iy = y + static_cast<int>(e.lookY * reach);
        const int lid = static_cast<int>(lidAmount * 2.0f * r);
        const uint16_t irisC = themeColor(t, i * 25u, static_cast<uint8_t>(150 + (s.layer[i] * 105.0f)));
        if (e.drawn && (r == e.r) && (ix == e.ix) && (iy == e.iy) && (lid == e.lid) && (irisC == e.iris) && (veins == e.veins)) continue;

        if (e.drawn && (e.r > r)) disc(d, x, y, e.r, BLACK);
        disc(d, x, y, r, vis::rgb(240, 238, 230));
        if (veins) {
            for (int k = 0; k < 6; k++) {
                const float a = k * 1.047f + 0.3f;
                line(d, x + static_cast<int>(cosf(a) * (r - 2)), y + static_cast<int>(sinf(a) * (r - 2)), x + static_cast<int>(cosf(a + 0.2f) * (r / 2)),
                     y + static_cast<int>(sinf(a + 0.2f) * (r / 2)), vis::rgb(220, 30, 30));
            }
        }
        disc(d, ix, iy, kIris, irisC);
        disc(d, ix, iy, 9 + static_cast<int>(s.bass * 9.0f), BLACK);  // the pupil widens with the bass
        disc(d, ix - 8, iy - 9, 4, WHITE);                           // the twinkle
        if (lid > 0) {  // the eyelid comes down from the top
            box(d, x - r - 1, y - r - 1, (2 * r) + 3, lid, BLACK);
            box(d, x - r, y - r - 1 + lid, 2 * r, 2, themeColor(t, i * 25u));
        }
        e.r = static_cast<int16_t>(r);
        e.ix = static_cast<int16_t>(ix);
        e.iy = static_cast<int16_t>(iy);
        e.lid = static_cast<int16_t>(lid);
        e.iris = irisC;
        e.veins = veins;
        e.drawn = true;
    }
}

void Controller::visual_sceneSpeaker(bool init_) {
    /// @brief SPEAKER: a big woofer pumping on the kick (cone rings and dust cap push out),
    /// glitter flying off on accents, the tweeter fizzing with the highs and the bass port
    /// puffing rings.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kWx = 360, kWy = 288, kCone = 196;
    constexpr int kTx = 790, kTy = 150, kPx = 790, kPy = 410;
    const uint16_t metal = vis::rgb(120, 120, 130);
    const uint16_t coneC = vis::rgb(34, 34, 40);
    const uint16_t plate = vis::rgb(50, 50, 58);

    if (init_) {
        disc(d, kWx, kWy, 236, metal);           // basket rim
        disc(d, kWx, kWy, 224, vis::rgb(20, 20, 24));  // rubber surround
        ring(d, kWx, kWy, 212, vis::rgb(60, 60, 66));
        disc(d, kWx, kWy, kCone, coneC);         // the cone
        for (int k = 0; k < 4; k++) {            // screws
            const float a = 0.785f + (k * 1.5708f);
            disc(d, kWx + static_cast<int>(cosf(a) * 230), kWy + static_cast<int>(sinf(a) * 230), 6, vis::rgb(200, 200, 210));
        }
        disc(d, kTx, kTy, 78, metal);
        disc(d, kTx, kTy, 70, plate);
        disc(d, kTx, kTy, 40, vis::rgb(180, 180, 190));  // tweeter dome
        disc(d, kPx, kPy, 86, metal);
        disc(d, kPx, kPy, 76, BLACK);                    // the bass port
        label(d, "MOFFE", FONT_10x14, CENTER, kTx, 516, themeColor(t, 0), BLACK);
        for (uint8_t k = 0; k < 6; k++) s.ringR[k] = 0;
        for (uint8_t k = 0; k < kGlitter; k++) s.sparks[k].on = false;
        for (uint8_t k = 0; k < kFizz; k++) s.fizzOn[k] = 0;
        s.coneQ = -1;
        s.capR = 0;
        s.portR = 0;
        return;
    }

    // 1. THE CONE: rings and the dust cap push out with the bass.
    const int8_t q = static_cast<int8_t>(s.bass * 12.0f);
    if (q != s.coneQ) {
        for (uint8_t k = 0; k < 6; k++) {
            if (s.ringR[k] > 0) ring(d, kWx, kWy, s.ringR[k], coneC);
            if (s.ringR[k] > 1) ring(d, kWx, kWy, s.ringR[k] - 1, coneC);
        }
        if (s.capR > 0) disc(d, kWx, kWy, s.capR, coneC);
        for (uint8_t k = 0; k < 6; k++) {
            const int r = 70 + (k * 22) + ((q * (6 - k)) / 2);
            const uint8_t v = static_cast<uint8_t>(60 + (q * 12) + (k * 6));
            ring(d, kWx, kWy, r, vis::rgb(v, v, static_cast<uint8_t>(v + 10)));
            ring(d, kWx, kWy, r - 1, vis::dim(themeColor(t, k * 30u), static_cast<uint8_t>(40 + (q * 14))));
            s.ringR[k] = static_cast<int16_t>(r);
        }
        const int cap = 52 + (q * 2);
        disc(d, kWx, kWy, cap, themeColor(t, 0, static_cast<uint8_t>(90 + (q * 13))));
        disc(d, kWx - (cap / 3), kWy - (cap / 3), cap / 5, vis::rgb(255, 255, 255));  // the shine
        s.capR = static_cast<int16_t>(cap);
        s.coneQ = q;
    }

    // 2. GLITTER off the left side on accents.
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        if (s.hitNow[i] < 0.8f) continue;
        for (int made = 0, p = 0; (p < kGlitter) && (made < 10); p++) {
            Spark& sp = s.sparks[p];
            if (sp.on) continue;
            const float a = 1.9f + (static_cast<float>(s.rng.range(0, 2500)) / 1000.0f);  // ~110..250 degrees
            const float v = 3.0f + (static_cast<float>(s.rng.range(0, 60)) / 10.0f);
            sp = Spark{kWx + (cosf(a) * 244.0f), kWy + (sinf(a) * 244.0f), cosf(a) * v, sinf(a) * v, 0, 0,
                       (s.rng.range(0, 3) == 0) ? WHITE : themeColor(t, s.rng.next()), static_cast<uint8_t>(s.rng.range(15, 35)), 0, true, false};
            sp.maxLife = sp.life;
            made++;
        }
    }
    for (uint8_t p = 0; p < kGlitter; p++) {
        Spark& sp = s.sparks[p];
        if (!sp.on) continue;
        if (sp.drawn) box(d, sp.sx, sp.sy, 3, 3, BLACK);
        sp.x += sp.vx;
        sp.y += sp.vy;
        sp.vy += 0.15f;
        const float ddx = sp.x - kWx, ddy = sp.y - kWy;
        if ((--sp.life == 0) || (sp.x < 0.0f) || (sp.x >= 600.0f) || (sp.y < kHintH) || (sp.y >= kH - 3) || (((ddx * ddx) + (ddy * ddy)) < (240.0f * 240.0f))) {
            sp.on = false;
            continue;
        }
        sp.sx = static_cast<int16_t>(sp.x);
        sp.sy = static_cast<int16_t>(sp.y);
        box(d, sp.sx, sp.sy, 3, 3, vis::dim(sp.c, static_cast<uint8_t>(60 + ((sp.life * 195) / sp.maxLife))));
        sp.drawn = true;
    }

    // 3. THE TWEETER fizzes with the highs.
    float highs = 0.0f;
    for (uint8_t b = 22; b < 32; b++) highs += s.band32[b];
    highs /= 10.0f;
    for (uint8_t k = 0; k < kFizz; k++) {
        const uint8_t on = ((s.rng.next() % 100) < static_cast<uint32_t>(highs * 140.0f)) ? 1 : 0;
        if (on == s.fizzOn[k]) continue;
        const float a = k * 0.3927f;
        const int x = kTx + static_cast<int>(cosf(a) * 55.0f);
        const int y = kTy + static_cast<int>(sinf(a) * 55.0f);
        box(d, x - 2, y - 2, 5, 5, on ? themeColor(t, k * 16u) : plate);
        s.fizzOn[k] = on;
    }

    // 4. THE PORT puffs a ring on every kick.
    if (s.portR > 0) ring(d, kPx, kPy, s.portR, BLACK);
    if ((s.portR == 0) && (s.bass > 0.55f)) s.portR = 8;
    if (s.portR > 0) {
        s.portR = static_cast<int16_t>(s.portR + 7);
        if (s.portR >= 72) {
            s.portR = 0;
        } else {
            ring(d, kPx, kPy, s.portR, themeColor(t, 60, static_cast<uint8_t>(255 - (s.portR * 3))));
        }
    }
}

void Controller::visual_sceneStepGrid(bool init_) {
    /// @brief STEP GRID: the pattern itself - ten rows (layers) of steps across the bank, a
    /// play head sweeping along, and every step lighting up as it plays.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kX0 = 64, kWdt = 876, kY0 = 56, kRowH = 46;
    const uint16_t empty = vis::rgb(26, 26, 34);

    uint32_t stepLen = 200;  // 1/16
    uint8_t cols = static_cast<uint8_t>((songInterval / stepLen) ? (songInterval / stepLen) : 1);
    if (cols > kGridCols) {
        cols = kGridCols;
        stepLen = (songInterval + kGridCols - 1) / kGridCols;
    }
    const int cw = kWdt / cols;
    auto cellColor = [&](uint8_t l, uint8_t c) -> uint8_t {  // 0 empty, 1 beat, 2..7 flash
        if (!((s.gridMap[l] >> c) & 1u)) return 0;
        const uint8_t f = static_cast<uint8_t>(s.gridFlash[l][c] * 5.99f);
        return static_cast<uint8_t>(1 + f);
    };

    if (init_ || (cols != s.gridCols)) {
        if (!init_) box(d, 0, kY0 - 14, kW, kH - kY0 + 14, BLACK);
        for (uint8_t l = 0; l < vis::kLayers; l++) {
            char num[4];
            snprintf(num, sizeof(num), "%02u", static_cast<unsigned>(l + 1));
            label(d, num, FONT_07x09, LEFT, 30, kY0 + (l * kRowH) + 14, themeColor(t, l * 25u), BLACK);
            s.gridMap[l] = 0;
            for (uint8_t c = 0; c < kGridCols; c++) {
                s.gridDrawn[l][c] = 0xFF;
                s.gridFlash[l][c] = 0.0f;
            }
        }
        s.gridCols = cols;
        s.gridHead = -1;
        s.gridStep = 0xFFFFFFFFu;
        if (init_) return;
    }

    // 1. WHICH STEPS HAVE BEATS (read a few times per second; they change when you edit).
    if ((visualFrame % 8) == 0) {
        for (uint8_t l = 0; l < vis::kLayers; l++) {
            const Bank& bank = layerLibrary[l].bankLibrary[activeSongBank];
            uint64_t map = 0;
            for (int16_t k = 0; k <= bank.lastActiveBeatNum; k++) {
                if (!bank.beatLibrary[k].getActive()) continue;
                const uint32_t c = bank.beatLibrary[k].getStartInterval() / stepLen;
                if (c < cols) map |= (1ull << c);
            }
            s.gridMap[l] = map;
        }
    }

    // 2. THE PLAY HEAD: entering a step flashes its beats.
    const uint32_t step = playInterval / stepLen;
    if (s.playing && (step != s.gridStep) && (step < cols)) {
        for (uint8_t l = 0; l < vis::kLayers; l++) {
            if ((s.gridMap[l] >> step) & 1u) s.gridFlash[l][step] = layerLibrary[l].mute ? 0.3f : 1.0f;
        }
        s.gridStep = step;
    }
    const int head = s.playing ? static_cast<int>(step) : -1;
    if (head != s.gridHead) {
        if (s.gridHead >= 0) box(d, kX0 + (s.gridHead * cw), kY0 - 10, cw - 2, 4, BLACK);
        if (head >= 0) box(d, kX0 + (head * cw), kY0 - 10, cw - 2, 4, themeColor(t, 0));
        s.gridHead = static_cast<int16_t>(head);
    }

    // 3. THE CELLS: only the changed ones are drawn.
    for (uint8_t l = 0; l < vis::kLayers; l++) {
        for (uint8_t c = 0; c < cols; c++) {
            s.gridFlash[l][c] *= 0.82f;
            const uint8_t v = cellColor(l, c);
            if (v == s.gridDrawn[l][c]) continue;
            uint16_t col = empty;
            if (v == 1) col = themeColor(t, l * 25u, layerLibrary[l].mute ? 60 : 140);
            if (v >= 2) col = vis::dim(WHITE, static_cast<uint8_t>(120 + (v * 22)));
            box(d, kX0 + (c * cw), kY0 + (l * kRowH), cw - 2, kRowH - 8, col);
            if ((c % 4) == 0) box(d, kX0 + (c * cw), kY0 + (l * kRowH) + kRowH - 6, cw - 2, 2, vis::rgb(60, 60, 70));  // beat marks
            s.gridDrawn[l][c] = v;
        }
    }
}

void Controller::visual_sceneDemo(bool init_) {
    /// @brief DEMO SCENE: a home-computer demo - copper bars rolling through the middle (they
    /// swing wider with the bass), a sine-wave text scroller with the project and tempo, a
    /// parallax starfield and a chrome MOFFE logo bouncing on the beat.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCopperY = 180, kScrollY = 448, kCell = 36;  // FONT_14x18 x2

    if (init_) {
        for (uint16_t r = 0; r < kCopperRows; r++) s.copper[r] = BLACK;
        for (uint8_t k = 0; k < kScrollSlots; k++) s.scrX[k] = kNone;
        char name[pname::kLength + 1];
        memcpy(name, projectName, sizeof(name));
        for (int i = pname::kLength - 1; (i >= 0) && (name[i] == ' '); i--) name[i] = 0;
        const int n = snprintf(s.scrText, sizeof(s.scrText), "MOFFE EDITION PRESENTS ... %s ... %u BPM ... GREETINGS TO EVERY DRUMBOY PRO OWNER ... ADD = AUTO ... CLEAR = INFO ...       ",
                               (name[0] != 0) ? name : "A NEW SONG", static_cast<unsigned>(rhythm.tempo));
        s.scrLen = static_cast<uint16_t>((n > 0) ? ((n < static_cast<int>(sizeof(s.scrText))) ? n : static_cast<int>(sizeof(s.scrText)) - 1) : 1);
        s.scrPos = 0;
        s.logoDrawn = false;
        for (uint8_t k = 0; k < kDemoStars; k++) {
            s.starX[k] = static_cast<float>(s.rng.range(0, kW - 1));
            s.starY[k] = static_cast<int16_t>(s.rng.range(kHintH + 2, kCopperY - 8));
            s.starSpeed[k] = static_cast<uint8_t>(1 + (k % 3));
            s.starOn[k] = false;
        }
        return;
    }

    // 1. COPPER BARS: five bars on sine paths; each row takes the colour of the bar in front.
    const float tempo = static_cast<float>((rhythm.tempo > 0) ? rhythm.tempo : 120) / 120.0f;
    const float time = visualFrame * 0.05f * tempo;
    const float swing = 70.0f + (s.bass * 40.0f);
    uint16_t rows[kCopperRows];
    for (uint16_t r = 0; r < kCopperRows; r++) rows[r] = BLACK;
    for (uint8_t b = 0; b < 5; b++) {
        const int centre = (kCopperRows / 2) + static_cast<int>(sinf(time + (b * 0.55f)) * swing);
        for (int k = -13; k <= 13; k++) {
            const int r = centre + k;
            if ((r < 0) || (r >= kCopperRows)) continue;
            const uint8_t v = static_cast<uint8_t>(255 - (((k < 0) ? -k : k) * 17));
            rows[r] = themeColor(t, (b * 51u) + (visualFrame / 2u), v);
        }
    }
    for (uint16_t r = 0; r < kCopperRows; r++) {
        if (rows[r] == s.copper[r]) continue;
        box(d, 0, kCopperY + r, kW, 1, rows[r]);
        s.copper[r] = rows[r];
    }

    // 2. THE LOGO: bounces on the beat, swings side to side, its colour cycling.
    const int logoW = 5 * 18 * 4;
    const int lx = 300 + static_cast<int>(sinf(visualFrame * 0.025f) * 200.0f);
    const int ly = 96 - static_cast<int>(fabsf(sinf(s.beatPhase * 3.14159f)) * 76.0f);
    if (s.logoDrawn) box(d, s.logoX, s.logoY, logoW + 4, 76, BLACK);

    // 3. STARS: three speeds, not over the logo.
    for (uint8_t k = 0; k < kDemoStars; k++) {
        if (s.starOn[k]) box(d, s.starDX[k], s.starY[k], 2, 2, BLACK);
        s.starX[k] -= s.starSpeed[k] * (1.0f + (s.energy * 2.0f));
        if (s.starX[k] < 0.0f) {
            s.starX[k] = kW - 2;
            s.starY[k] = static_cast<int16_t>(s.rng.range(kHintH + 2, kCopperY - 8));
        }
        const int x = static_cast<int>(s.starX[k]);
        const bool hidden = (x >= lx - 4) && (x <= lx + logoW + 8) && (s.starY[k] >= ly - 4) && (s.starY[k] <= ly + 80);
        s.starOn[k] = !hidden;
        s.starDX[k] = static_cast<int16_t>(x);
        if (!hidden) {
            const uint8_t g = static_cast<uint8_t>(80 + (s.starSpeed[k] * 58));
            box(d, x, s.starY[k], 2, 2, vis::rgb(g, g, g));
        }
    }
    d.setFont(FONT_14x18);
    d.setAlignment(LEFT);
    d.setForeColor(themeColor(t, 0, 90));
    d.drawTextScaled("MOFFE", 5, static_cast<uint16_t>(lx + 4), static_cast<uint16_t>(ly + 4), 4);  // shadow
    d.setForeColor(themeColor(t, visualFrame * 3u));
    d.drawTextScaled("MOFFE", 5, static_cast<uint16_t>(lx), static_cast<uint16_t>(ly), 4);
    s.logoX = static_cast<int16_t>(lx);
    s.logoY = static_cast<int16_t>(ly);
    s.logoDrawn = true;

    // 4. THE SCROLLER: letters ride a sine wave right to left.
    s.scrPos += 4;
    const uint32_t first = s.scrPos / kCell;
    const int offset = static_cast<int>(s.scrPos % kCell);
    for (uint8_t k = 0; k < kScrollSlots; k++) {
        if (s.scrX[k] != kNone) box(d, s.scrX[k], s.scrY[k], kCell, 36, BLACK);
        s.scrX[k] = kNone;
        const int x = (k * kCell) - offset;
        if ((x < 0) || (x > kW - kCell)) continue;
        const char ch = s.scrText[(first + k) % s.scrLen];
        const int y = kScrollY + static_cast<int>(sinf((x * 0.012f) + (visualFrame * 0.12f)) * 30.0f) - 18;
        if (ch != ' ') {
            d.setForeColor(themeColor(t, static_cast<uint32_t>(x / 4) + visualFrame));
            d.drawTextScaled(&ch, 1, static_cast<uint16_t>(x), static_cast<uint16_t>(y), 2);
        }
        s.scrX[k] = static_cast<int16_t>(x);
        s.scrY[k] = static_cast<int16_t>(y);
    }
}

void Controller::visual_sceneSunset(bool init_) {
    /// @brief SUNSET DRIVE: an 80s neon sunset - a violet-to-orange sky with twinkling stars, a
    /// striped sun that breathes with the bass (its stripes sliding down), mountain silhouettes,
    /// palm trees and a neon grid road rolling towards you on the beat.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kHorizon = 330, kCx = 480, kBy = 532, kFx0 = 0, kFx1 = 960;
    const uint16_t gridC = themeColor(t, 200, 255);
    const uint16_t mountC = vis::rgb(36, 0, 56);
    auto vertX = [&](int k, int y) -> int {
        const int bx = kCx + ((k - 8) * 130);
        return kCx + (((bx - kCx) * (y - kHorizon)) / (kBy - kHorizon));
    };
    auto behindPalm = [](int x) { return ((x >= 88) && (x < 176)) || ((x >= 818) && (x < 884)); };
    auto mountainTop = [&](int x) -> int {  // two ranges, leaving room for the sun in the middle
        if ((x > 330) && (x < 630)) return kHorizon;
        const float u = static_cast<float>(x);
        const float h = 40.0f + (28.0f * sinf(u * 0.021f)) + (18.0f * sinf((u * 0.057f) + 1.3f)) + (10.0f * sinf(u * 0.13f));
        const float fade = (x <= 330) ? fminf(1.0f, (330.0f - x) / 60.0f) : fminf(1.0f, (x - 630.0f) / 60.0f);
        return kHorizon - static_cast<int>(h * fade);
    };

    if (init_) {
        for (int y = kHintH; y < kHorizon; y += 2) box(d, 0, y, kW, 2, skyColor(y, kHorizon));
        for (int x = 0; x < kW; x += 4) {  // mountains with a neon edge
            const int top = mountainTop(x);
            if (top < kHorizon) {
                box(d, x, top, 4, kHorizon - top, mountC);
                box(d, x, top, 4, 2, themeColor(t, 220, 200));
            }
        }
        // The road grid (vertical rails) under the horizon.
        for (uint8_t k = 0; k < kFloorVerticals; k++) {
            int x1 = vertX(k, kBy);
            int y1 = kBy;
            if ((x1 < kFx0) || (x1 >= kFx1)) {
                const int bx = kCx + ((k - 8) * 130);
                const int xb = (x1 < kFx0) ? kFx0 : (kFx1 - 1);
                y1 = kHorizon + (((xb - kCx) * (kBy - kHorizon)) / (bx - kCx));
                x1 = xb;
            }
            line(d, kCx, kHorizon, x1, y1, vis::dim(gridC, 150));  // (redrawn under the palms below)
        }
        // Palm trees: a leaning trunk and drooping fronds, in black.
        const int palms[2][2] = {{120, 1}, {850, -1}};
        for (const auto& p : palms) {
            const int bx = p[0], dir = p[1];
            for (int k = 0; k < 12; k++) {
                const int y0 = 540 - (k * 26), y1 = y0 - 26;
                const int x0 = bx + (dir * ((k * k) / 3)), x1 = bx + (dir * (((k + 1) * (k + 1)) / 3));
                for (int w = -6; w <= 6; w++) line(d, x0 + w, y0, x1 + w, y1, BLACK);
            }
            const int tx = bx + (dir * 48), ty = 540 - (12 * 26);
            for (int f = 0; f < 7; f++) {
                const float a = -2.9f + (f * 0.55f);
                int px = tx, py = ty;
                for (int k = 1; k <= 10; k++) {
                    const int nx = tx + static_cast<int>(cosf(a) * k * 12.0f);
                    const int ny = ty + static_cast<int>(sinf(a) * k * 9.0f) + ((k * k) / 3);
                    for (int w = -2; w <= 2; w++) line(d, px, py + w, nx, ny + w, BLACK);
                    px = nx;
                    py = ny;
                }
            }
        }
        for (uint8_t r = 0; r < kSunRows; r++) {
            s.sunHw[r] = 0;
            s.sunGap[r] = 0;
        }
        for (uint8_t k = 0; k < kFloorLines; k++) s.floorY[k] = kNone;
        for (uint8_t k = 0; k < kTwinkles; k++) {
            s.twX[k] = static_cast<int16_t>(s.rng.range(0, kW - 3));
            s.twY[k] = static_cast<int16_t>(s.rng.range(kHintH + 2, 120));  // above the palm fronds
            s.twOn[k] = 0;
        }
        s.sunR = 0;
        s.horizonLvl = 0xFF;
        return;
    }

    // 1. THE SUN: radius breathes with the bass; the stripes in its lower half slide down.
    const int r = 120 + static_cast<int>(s.bass * 22.0f);
    const int stripeTop = kHorizon - static_cast<int>(r * 0.55f);
    const int slide = static_cast<int>(visualFrame / 2u) % 16;
    for (uint8_t i = 0; i < kSunRows; i++) {
        const int y = kHorizon - 1 - i;  // row i above the horizon
        const int dy = kHorizon - y;
        const int hw = (dy <= r) ? static_cast<int>(sqrtf(static_cast<float>((r * r) - (dy * dy)))) : 0;
        uint8_t gap = 0;
        if ((y > stripeTop) && (hw > 0)) {
            const int depth = y - stripeTop;  // 0 at the first stripe, growing to the horizon
            const int width = 1 + (depth / 14);
            gap = (((depth + 16 - slide) % 16) < width) ? 1 : 0;
        }
        if ((hw == s.sunHw[i]) && (gap == s.sunGap[i])) continue;
        const int wide = (hw > s.sunHw[i]) ? hw : s.sunHw[i];
        if (wide > 0) box(d, kCx - wide, y, 2 * wide, 1, skyColor(y, kHorizon));
        if ((hw > 0) && !gap) box(d, kCx - hw, y, 2 * hw, 1, sunColor(y, kHorizon, r));
        s.sunHw[i] = static_cast<int16_t>(hw);
        s.sunGap[i] = gap;
    }

    // 2. THE HORIZON flashes with the kick.
    const uint8_t hl = static_cast<uint8_t>(s.bass * 6.0f);
    if (hl != s.horizonLvl) {
        box(d, 0, kHorizon, kW, 2, vis::dim(gridC, static_cast<uint8_t>(120 + (hl * 22))));
        s.horizonLvl = hl;
    }

    // 3. THE ROAD rolls one line per beat (as on SHOWTIME).
    int16_t newY[kFloorLines];
    float depth[kFloorLines];
    for (uint8_t k = 0; k < kFloorLines; k++) {
        depth[k] = (static_cast<float>(k) + 1.0f - s.beatPhase) / kFloorLines;
        newY[k] = vis::floorY(kHorizon + 2, kBy, depth[k]);
    }
    for (uint8_t k = 0; k < kFloorLines; k++) {
        if ((s.floorY[k] == kNone) || (s.floorY[k] == newY[k])) continue;
        const int y = s.floorY[k];
        box(d, kFx0, y, kFx1 - kFx0, 1, BLACK);
        for (uint8_t v = 0; v < kFloorVerticals; v++) {
            const int x = vertX(v, y);
            if ((x >= kFx0) && (x < kFx1) && !behindPalm(x)) box(d, x, y, 1, 1, vis::dim(gridC, 150));
        }
    }
    for (uint8_t k = 0; k < kFloorLines; k++) {
        if (s.floorY[k] == newY[k]) continue;
        const uint16_t c = vis::dim(gridC, static_cast<uint8_t>(80 + ((1.0f - depth[k]) * 175.0f)));
        box(d, 0, newY[k], 88, 1, c);       // the road lines pass behind the palm trunks
        box(d, 176, newY[k], 642, 1, c);
        box(d, 884, newY[k], 76, 1, c);
        s.floorY[k] = newY[k];
    }

    // 4. STARS twinkle in the upper sky (with the highs).
    for (uint8_t k = 0; k < kTwinkles; k++) {
        const uint8_t on = ((s.rng.next() % 100) < static_cast<uint32_t>(20 + (s.energy * 60.0f))) ? 1 : 0;
        if (on == s.twOn[k]) continue;
        box(d, s.twX[k], s.twY[k], 2, 2, on ? WHITE : skyColor(s.twY[k], kHorizon));
        s.twOn[k] = on;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* v2.07 styles --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

namespace {

inline void unpack565(uint16_t c, int& r, int& g, int& b) {
    r = ((c >> 11) & 0x1F) << 3;
    g = ((c >> 5) & 0x3F) << 2;
    b = (c & 0x1F) << 3;
}

// The 256-colour palette of GEISS / MILK TUNNEL: black -> the theme's colours (cycling) -> white.
inline void makePalette(uint16_t* pal, uint8_t theme, uint32_t cycle, float flash) {
    for (int i = 0; i < 256; i++) {
        uint16_t c;
        if (i < 176) {
            c = themeColor(theme, static_cast<uint32_t>(i / 2) + cycle, static_cast<uint8_t>((i * 255) / 176));
        } else {
            int r, g, b;
            unpack565(themeColor(theme, 88u + cycle), r, g, b);
            const int k = ((i - 176) * 255) / 79;
            c = vis::rgb(static_cast<uint8_t>(r + (((255 - r) * k) >> 8)), static_cast<uint8_t>(g + (((255 - g) * k) >> 8)),
                         static_cast<uint8_t>(b + (((255 - b) * k) >> 8)));
        }
        if (flash > 0.01f) {  // a flash on the bar lifts everything towards white
            int r, g, b;
            unpack565(c, r, g, b);
            const int k = static_cast<int>(flash * 160.0f);
            c = vis::rgb(static_cast<uint8_t>(r + (((255 - r) * k) >> 8)), static_cast<uint8_t>(g + (((255 - g) * k) >> 8)),
                         static_cast<uint8_t>(b + (((255 - b) * k) >> 8)));
        }
        pal[i] = c;
    }
}

constexpr const char* kRoman[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};

}  // namespace

void Controller::visual_sceneGeiss(bool init_) {
    /// @brief GEISS: a feedback plasma in the spirit of the classic plug-ins. Every frame the
    /// last picture is zoomed, turned, blurred and faded a little, and the waveform, the hits and
    /// a ring on every bar are painted into it - so everything smears into flowing trails. Drawn
    /// in a 160 x 88 buffer, shown as 6 x 6 blocks over the whole screen.

    State& s = vs;
    if (init_) {
        memset(s.pix.g.a, 0, sizeof(s.pix.g.a));
        memset(s.pix.g.b, 0, sizeof(s.pix.g.b));
        s.gFlip = 0;
        s.gTime = 0.0f;
        s.gFlash = 0.0f;
        s.lastBeatIdx = s.beatIdx;
        return;
    }
    const uint8_t* src = s.gFlip ? s.pix.g.b : s.pix.g.a;
    uint8_t* dst = s.gFlip ? s.pix.g.a : s.pix.g.b;
    s.gTime += 0.033f * (1.0f + s.energy);

    // 1. WARP: zoom in (the picture flows outwards), turn, blur 2 x 2, fade.
    const float zoom = 0.955f - (s.bass * 0.035f);
    const float rot = (0.035f * sinf(s.gTime * 0.4f)) + (0.02f * sinf(s.gTime * 0.13f));
    const int32_t cz = static_cast<int32_t>(cosf(rot) * zoom * 65536.0f);
    const int32_t sz = static_cast<int32_t>(sinf(rot) * zoom * 65536.0f);
    const int decay = 3;
    for (int y = 0; y < kPixH; y++) {
        const int32_t dy = (y - (kPixH / 2)) * 65536;
        for (int x = 0; x < kPixW; x++) {
            const int32_t dx = (x - (kPixW / 2)) * 65536;
            const int32_t fx = static_cast<int32_t>(((static_cast<int64_t>(dx) * cz) - (static_cast<int64_t>(dy) * sz)) >> 16) + ((kPixW / 2) * 65536);
            const int32_t fy = static_cast<int32_t>(((static_cast<int64_t>(dx) * sz) + (static_cast<int64_t>(dy) * cz)) >> 16) + ((kPixH / 2) * 65536);
            const int sx = clampi(fx >> 16, 0, kPixW - 2);
            const int sy = clampi(fy >> 16, 0, kPixH - 2);
            const uint8_t* p = src + (sy * kPixW) + sx;
            const int v = (p[0] + p[1] + p[kPixW] + p[kPixW + 1]) >> 2;
            dst[(y * kPixW) + x] = static_cast<uint8_t>((v > decay) ? (v - decay) : 0);
        }
    }

    // 2. PAINT: the waveform across the middle, sparks for hits, a ring on every bar.
    const uint32_t end = visTap.pos;
    const uint32_t start = end - (kPixW * 4) - 1;
    float peak = 0.02f;
    for (int x = 0; x < kPixW; x++) peak = fmaxf(peak, fabsf(mono(start + (x * 4))));
    const float gain = fminf(1.0f / peak, 8.0f) * 26.0f;
    for (int x = 0; x < kPixW; x++) {
        const int y = clampi((kPixH / 2) + static_cast<int>(mono(start + (x * 4)) * gain), 1, kPixH - 2);
        dst[(y * kPixW) + x] = 255;
        if (dst[((y - 1) * kPixW) + x] < 190) dst[((y - 1) * kPixW) + x] = 190;
        if (dst[((y + 1) * kPixW) + x] < 190) dst[((y + 1) * kPixW) + x] = 190;
    }
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        if (s.hitNow[i] <= 0.0f) continue;
        const int x = clampi(8 + (i * 16) + s.rng.range(-4, 4), 1, kPixW - 2);
        const int y = s.rng.range(4, kPixH - 5);
        for (int k = -1; k <= 1; k++) {
            dst[((y + k) * kPixW) + x] = 255;
            dst[(y * kPixW) + x + k] = 255;
        }
    }
    if (s.beatIdx != s.lastBeatIdx) {
        if (s.barStart) {
            s.gFlash = 1.0f;
            for (int k = 0; k < 64; k++) {
                const float a = k * 0.0982f;
                const int x = (kPixW / 2) + static_cast<int>(cosf(a) * 34.0f);
                const int y = (kPixH / 2) + static_cast<int>(sinf(a) * 22.0f);
                if ((x >= 0) && (x < kPixW) && (y >= 0) && (y < kPixH)) dst[(y * kPixW) + x] = 240;
            }
        }
        s.lastBeatIdx = s.beatIdx;
    }
    s.gFlash *= 0.8f;

    // 3. SHOW IT.
    makePalette(s.pal, visualTheme, visualFrame, s.gFlash * 0.5f);
    lcd.drawIndexedScaled(dst, s.pal, 0, kPixY, kPixW, kPixH, kPixScale);
    s.gFlip ^= 1;
}

void Controller::visual_sceneMilk(bool init_) {
    /// @brief MILK TUNNEL: flying down a warping, colour-cycling tunnel (the classic demo-scene
    /// texture tunnel): the kick pushes you forward, the hi-hats twist it, it breathes with the
    /// music and flashes on every bar. Same 160 x 88 buffer as GEISS.

    State& s = vs;
    if (init_) {
        for (int y = 0; y < kPixH; y++) {
            for (int x = 0; x < kPixW; x++) {
                const float dx = (x - (kPixW / 2)) + 0.5f;
                const float dy = (y - (kPixH / 2)) + 0.5f;
                const float dist = sqrtf((dx * dx) + (dy * dy));
                const int i = (y * kPixW) + x;
                s.pix.t.dist[i] = static_cast<uint8_t>(static_cast<int>(1400.0f / (dist + 0.5f)) & 0xFF);
                s.pix.t.angle[i] = static_cast<uint8_t>(static_cast<int>((atan2f(dy, dx) + 3.14159265f) * (128.0f / 3.14159265f)) & 0xFF);
                s.pix.t.shade[i] = static_cast<uint8_t>(fminf(255.0f, dist * 5.0f));
            }
        }
        s.tShift = 0.0f;
        s.tRot = 0.0f;
        s.gFlash = 0.0f;
        s.lastBeatIdx = s.beatIdx;
        return;
    }

    float highs = 0.0f;
    for (uint8_t b = 20; b < 32; b++) highs += s.band32[b];
    highs /= 12.0f;
    s.tShift += 1.2f + (s.bass * 7.0f);
    s.tRot += 0.4f + (highs * 6.0f);
    if (s.beatIdx != s.lastBeatIdx) {
        if (s.barStart) s.gFlash = 1.0f;
        s.lastBeatIdx = s.beatIdx;
    }
    s.gFlash *= 0.82f;
    // The warp: each depth ring is pushed sideways by its own wave.
    int8_t warp[256];
    for (int d = 0; d < 256; d++) warp[d] = static_cast<int8_t>(sinf((d * 0.06f) + (visualFrame * 0.07f)) * (10.0f + (s.energy * 30.0f)));
    const int shift = static_cast<int>(s.tShift);
    const int turn = static_cast<int>(s.tRot);
    for (int i = 0; i < kPixW * kPixH; i++) {
        const int d = s.pix.t.dist[i];
        const int u = d + shift;
        const int v = s.pix.t.angle[i] + turn + warp[d];
        const int tex = (((u ^ v) & 0x3F) * 3) + 32;  // the checker / XOR texture
        s.pix.t.out[i] = static_cast<uint8_t>((tex * s.pix.t.shade[i]) >> 8);
    }
    makePalette(s.pal, visualTheme, visualFrame / 2u, s.gFlash * 0.6f);
    lcd.drawIndexedScaled(s.pix.t.out, s.pal, 0, kPixY, kPixW, kPixH, kPixScale);
}

void Controller::visual_sceneRobot(bool init_) {
    /// @brief ROBOT DJ: a pixel robot behind two turntables and a mixer. Its head nods on the
    /// beat, the chest light thumps with the bass, the mixer's meters dance, it scratches on big
    /// hits, throws its arms up on a loud bar and fires laser eyes on accents.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kTable = 392, kPlatR = 72, kHeadW = 120, kHeadH = 90;
    const int platX[2] = {210, 750};
    constexpr int kPlatY = 456;
    const uint16_t metal = vis::rgb(110, 110, 126);
    const uint16_t dark = vis::rgb(40, 40, 46);
    auto thick = [&](int x0, int y0, int x1, int y1, uint16_t c) {
        for (int w = -2; w <= 2; w++) {
            line(d, x0 + w, y0, x1 + w, y1, c);
            line(d, x0, y0 + w, x1, y1 + w, c);
        }
    };

    if (init_) {
        box(d, 400, 404, 160, 98, vis::rgb(70, 70, 80));  // the mixer
        bevel(d, 400, 404, 160, 98, true, vis::rgb(150, 150, 160), vis::rgb(20, 20, 26));
        label(d, "DJ MOFFE", FONT_10x14, CENTER, 480, 516, themeColor(t, 0), BLACK);
        memset(s.eqSeg, 0, sizeof(s.eqSeg));
        s.headY = kNone;
        for (int k = 0; k < 2; k++) {
            s.armDrawn[k] = false;
            s.laserLife[k] = 0;
        }
        s.platAngle = 0.0f;
        s.scratch = 0;
        s.armsUp = 0;
        s.lastBeatIdx = s.beatIdx;
        return;
    }

    // 1. ERASE what moves: arms, lasers, the head if it moved.
    for (int k = 0; k < 2; k++) {
        if (s.armDrawn[k]) {
            thick(s.arm[k][0], s.arm[k][1], s.arm[k][2], s.arm[k][3], BLACK);
            thick(s.arm[k][2], s.arm[k][3], s.arm[k][4], s.arm[k][5], BLACK);
            disc(d, s.arm[k][4], s.arm[k][5], 9, BLACK);  // the hand
            s.armDrawn[k] = false;
        }
        if (s.laserLife[k] > 0) {
            if (--s.laserLife[k] == 0) line(d, s.laser[k][0], s.laser[k][1], s.laser[k][2], s.laser[k][3], BLACK);
        }
    }
    const int nod = static_cast<int>(fabsf(sinf(s.beatPhase * 3.14159f)) * 14.0f);
    const int headY = 140 + (s.playing ? nod : 0);
    if ((s.headY != kNone) && (s.headY != headY)) box(d, 480 - (kHeadW / 2), s.headY - 34, kHeadW, kHeadH + 34, BLACK);

    // 2. MOODS: scratch on big hits, arms up on a loud bar.
    for (uint8_t i = 0; i < vis::kLayers; i++) {
        if (s.hitNow[i] > 0.85f) s.scratch = 8;
    }
    if (s.beatIdx != s.lastBeatIdx) {
        if (s.barStart && (s.energy > 0.5f)) s.armsUp = 10;
        s.lastBeatIdx = s.beatIdx;
    }

    // 3. TURNTABLES (spinning with the tempo, jittering while scratching), table edge.
    const float tempo = static_cast<float>((rhythm.tempo > 0) ? rhythm.tempo : 120) / 120.0f;
    s.platAngle += s.playing ? (0.14f * tempo) : 0.02f;
    for (int k = 0; k < 2; k++) {
        const float a = s.platAngle + ((s.scratch > 0) ? (sinf(visualFrame * 1.7f) * 0.8f) : 0.0f) + (k * 1.3f);
        disc(d, platX[k], kPlatY, kPlatR, dark);
        ring(d, platX[k], kPlatY, kPlatR - 10, vis::rgb(60, 60, 68));
        ring(d, platX[k], kPlatY, kPlatR - 22, vis::rgb(60, 60, 68));
        disc(d, platX[k], kPlatY, 22, themeColor(t, k * 128u));
        line(d, platX[k], kPlatY, platX[k] + static_cast<int>(cosf(a) * (kPlatR - 4)), kPlatY + static_cast<int>(sinf(a) * (kPlatR - 4)), WHITE);
    }
    box(d, 0, kTable, kW, 2, themeColor(t, 0, 160));

    // 4. MIXER METERS.
    for (uint8_t b = 0; b < 8; b++) {
        segBar(d, 412 + (b * 18), 494, -1, 12, 4, 2, s.eqSeg[b], static_cast<uint8_t>(s.band32[b * 4] * 13.0f),
               [&](int i) { return (i >= 10) ? vis::rgb(255, 50, 50) : themeColor(t, static_cast<uint32_t>(i * 20)); });
    }

    // 5. BODY with a chest light, HEAD with eyes, mouth LEDs and an antenna.
    box(d, 430, 250, 100, 136, metal);
    bevel(d, 430, 250, 100, 136, true, vis::rgb(170, 170, 185), vis::rgb(50, 50, 60));
    disc(d, 480, 300, 26, dark);
    disc(d, 480, 300, 8 + static_cast<int>(s.bass * 18.0f), themeColor(t, visualFrame * 2u));
    for (int k = 0; k < 4; k++) box(d, 446 + (k * 18), 350, 12, 6, ((k * 0.25f) < s.energy) ? themeColor(t, k * 40u) : dark);
    box(d, 480 - (kHeadW / 2), headY, kHeadW, kHeadH, metal);
    bevel(d, 480 - (kHeadW / 2), headY, kHeadW, kHeadH, true, vis::rgb(170, 170, 185), vis::rgb(50, 50, 60));
    bool flashEye = false;
    for (uint8_t i = 0; i < vis::kLayers; i++) flashEye = flashEye || (s.flash[i] > 0.7f);
    const uint16_t eyeC = flashEye ? WHITE : themeColor(t, 0);
    box(d, 440, headY + 26, 28, 16, eyeC);
    box(d, 492, headY + 26, 28, 16, eyeC);
    for (int k = 0; k < 5; k++) box(d, 446 + (k * 14), headY + 62, 10, 8, ((k * 0.2f) < s.vuL) ? themeColor(t, 120u + (k * 20u)) : dark);
    line(d, 480, headY, 480, headY - 26, metal);
    disc(d, 480, headY - 30, 6, s.barStart ? WHITE : themeColor(t, 60));
    s.headY = static_cast<int16_t>(headY);

    // 6. ARMS: on the decks (scratching), or up in the air.
    for (int k = 0; k < 2; k++) {
        const int sx = (k == 0) ? 432 : 528, sy = 266;
        int hx, hy, ex, ey;
        if (s.armsUp > 0) {
            hx = (k == 0) ? 360 : 600;
            hy = 60;
            ex = (k == 0) ? 380 : 580;
            ey = 170;
        } else {
            const int wobble = (s.scratch > 0) ? static_cast<int>(sinf((visualFrame * 1.9f) + k) * 20.0f) : 0;
            hx = ((k == 0) ? (platX[0] + 40) : (platX[1] - 40)) + wobble;
            hy = kPlatY - 30;
            ex = (k == 0) ? 360 : 600;
            ey = 330;
        }
        thick(sx, sy, ex, ey, metal);
        thick(ex, ey, hx, hy, metal);
        disc(d, hx, hy, 9, vis::rgb(200, 200, 210));
        const int16_t a[6] = {static_cast<int16_t>(sx), static_cast<int16_t>(sy), static_cast<int16_t>(ex), static_cast<int16_t>(ey), static_cast<int16_t>(hx), static_cast<int16_t>(hy)};
        memcpy(s.arm[k], a, sizeof(a));
        s.armDrawn[k] = true;
    }
    if (s.scratch > 0) s.scratch--;
    if (s.armsUp > 0) s.armsUp--;

    // 7. LASER EYES on accents.
    bool accent = false;
    for (uint8_t i = 0; i < vis::kLayers; i++) accent = accent || (s.hitNow[i] > 0.95f);
    if (accent) {
        for (int k = 0; k < 2; k++) {
            if (s.laserLife[k] > 0) line(d, s.laser[k][0], s.laser[k][1], s.laser[k][2], s.laser[k][3], BLACK);
            const int x0 = (k == 0) ? 454 : 506, y0 = headY + 34;
            const int x1 = s.rng.range(20, kW - 20), y1 = s.rng.range(kHintH + 2, 100);
            line(d, x0, y0, x1, y1, vis::rgb(255, 40, 40));
            const int16_t l[4] = {static_cast<int16_t>(x0), static_cast<int16_t>(y0), static_cast<int16_t>(x1), static_cast<int16_t>(y1)};
            memcpy(s.laser[k], l, sizeof(l));
            s.laserLife[k] = 3;
        }
    }
}

void Controller::visual_sceneCrowd(bool init_) {
    /// @brief DISCO CROWD: a mirror ball throwing sparkles, eight pixel dancers (one per layer)
    /// jumping on their hits and waving with the hi-hats, a stage-diver surfing over them every
    /// four bars, and the light-up floor flashing patterns under their feet.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kFeet = 318, kFloorY = 330, kCols = 16, kRows = 4, kBallX = 480, kBallY = 64, kBallR = 34;
    const uint16_t dark = vis::rgb(18, 18, 26);
    auto tile = [&](uint8_t i, uint16_t c) { box(d, ((i % kCols) * 60) + 2, kFloorY + ((i / kCols) * 52) + 2, 56, 48, c); };

    if (init_) {
        for (uint8_t i = 0; i < kCols * kRows; i++) {
            s.tileLvl[i] = 0.0f;
            s.tileDrawn[i] = 0;
            s.tileHue[i] = 0;
            tile(i, dark);
        }
        line(d, kBallX, kHintH, kBallX, kBallY - kBallR, vis::rgb(120, 120, 130));
        for (uint8_t k = 0; k < kTwinkles; k++) {
            s.twX[k] = static_cast<int16_t>(s.rng.range(20, kW - 24));
            s.twY[k] = static_cast<int16_t>(s.rng.range(kHintH + 4, 160));
            s.twOn[k] = 0;
        }
        s.diverOn = false;
        s.diverDrawn = false;
        s.lastStep = s.step8;
        s.lastSpawnMs = HAL_GetTick();
        s.lastBeatIdx = s.beatIdx;
        return;
    }

    // 1. THE MIRROR BALL (every other frame) and its sparkles.
    if ((visualFrame & 1u) == 0) {
        disc(d, kBallX, kBallY, kBallR, vis::rgb(150, 150, 165));
        const float ph = visualFrame * 0.06f;
        for (int k = 0; k < 8; k++) {  // turning meridians
            const float a = ph + (k * 0.3927f);
            if (cosf(a) < 0.0f) continue;
            const int x = kBallX + static_cast<int>(sinf(a) * kBallR);
            line(d, x, kBallY - kBallR + 6, x, kBallY + kBallR - 6, vis::rgb(90, 90, 105));
        }
        for (int k = -2; k <= 2; k++) box(d, kBallX - kBallR + 6, kBallY + (k * 12), (2 * kBallR) - 12, 1, vis::rgb(90, 90, 105));
        for (int k = 0; k < 6; k++) box(d, kBallX + s.rng.range(-24, 20), kBallY + s.rng.range(-24, 20), 5, 5, (k < 2) ? WHITE : themeColor(t, s.rng.next()));
    }
    for (uint8_t k = 0; k < kTwinkles; k++) {
        const uint8_t on = ((s.rng.next() % 100) < static_cast<uint32_t>(15 + (s.energy * 70.0f))) ? 1 : 0;
        if (on == s.twOn[k]) continue;
        const int dx = s.twX[k] - kBallX, dy = s.twY[k] - kBallY;
        if (((dx * dx) + (dy * dy)) < ((kBallR + 6) * (kBallR + 6))) continue;  // not on the ball
        box(d, s.twX[k], s.twY[k], 3, 3, on ? themeColor(t, k * 9u) : BLACK);
        s.twOn[k] = on;
    }

    // 2. THE STAGE-DIVER: every four bars someone surfs across over the crowd.
    const uint32_t beatsPerBar = (rhythm.measure > 0) ? rhythm.measure : 4;
    if (s.beatIdx != s.lastBeatIdx) {
        if (s.barStart && (((s.beatIdx / beatsPerBar) % 4) == 0) && !s.diverOn) {
            s.diverOn = true;
            s.diverX = -60.0f;
        }
        s.lastBeatIdx = s.beatIdx;
    }
    if (s.diverDrawn) box(d, s.diverDX - 40, 168, 84, 30, BLACK);
    s.diverDrawn = false;

    // 3. THE DANCERS: jump with their layer, arms wave with the beat and the highs.
    float highs = 0.0f;
    for (uint8_t b = 20; b < 32; b++) highs += s.band32[b];
    highs /= 12.0f;
    for (uint8_t i = 0; i < 8; i++) {
        const int x = 90 + (i * 112);
        const int jump = static_cast<int>((s.flash[i] * 24.0f) + (s.bass * 6.0f));
        const int feet = kFeet - jump;
        box(d, x - 36, kFeet - 150, 72, 152, BLACK);
        const uint16_t c = themeColor(t, i * 32u, static_cast<uint8_t>(150 + (s.layer[i] * 105.0f)));
        disc(d, x, feet - 96, 11, c);                       // head
        box(d, x - 9, feet - 84, 18, 40, c);                // body
        line(d, x - 6, feet - 44, x - 12, feet, c);         // legs
        line(d, x + 6, feet - 44, x + 12, feet, c);
        line(d, x - 5, feet - 44, x - 11, feet, c);
        line(d, x + 5, feet - 44, x + 11, feet, c);
        const float wave = sinf((s.beatPhase * 6.2832f) + (i * 0.8f)) * (0.4f + highs);
        for (int k = 0; k < 2; k++) {                       // arms: raised 0.2 .. 1.4 rad, waving
            const int sgn = (k == 0) ? -1 : 1;
            const float up = fminf(1.4f, fmaxf(0.2f, 0.8f + (wave * sgn)));
            const int sx = x + (sgn * 9), sy = feet - 80;
            const int hx = sx + (sgn * static_cast<int>(cosf(up) * 30.0f));
            const int hy = sy - static_cast<int>(sinf(up) * 30.0f);
            line(d, sx, sy, hx, hy, c);
            line(d, sx, sy + 1, hx, hy + 1, c);
        }
    }
    if (s.diverOn) {
        s.diverX += 14.0f;
        if (s.diverX > kW + 60) {
            s.diverOn = false;
        } else {
            const int x = static_cast<int>(s.diverX);
            const uint16_t c = WHITE;
            disc(d, x + 30, 182, 10, c);       // head first
            box(d, x - 20, 176, 42, 14, c);    // body flat
            line(d, x - 20, 178, x - 38, 172, c);
            line(d, x - 20, 188, x - 38, 194, c);
            line(d, x + 14, 176, x + 24, 168, c);
            s.diverDX = static_cast<int16_t>(x);
            s.diverDrawn = true;
        }
    }

    // 4. THE FLOOR (as DISCO, four rows under the dancers).
    const uint32_t now = HAL_GetTick();
    bool step = s.playing ? (s.step8 != s.lastStep) : (((now - s.lastSpawnMs) > 400) || (((now - s.lastSpawnMs) > 150) && (s.bass > 0.6f)));
    auto light = [&](uint8_t i, float lvl, uint8_t hue) {
        if (lvl > s.tileLvl[i]) {
            s.tileLvl[i] = lvl;
            s.tileHue[i] = hue;
        }
    };
    if (step) {
        s.lastStep = s.step8;
        s.lastSpawnMs = now;
        const uint8_t hue = static_cast<uint8_t>(s.step8 * 19u);
        if (s.barStart && ((s.step8 & 1u) == 0)) {
            for (uint8_t i = 0; i < kCols * kRows; i++) {
                if ((((i % kCols) + (i / kCols) + (s.beatIdx / 4u)) & 1u) == 0) light(i, 1.0f, hue);
            }
        } else if ((s.step8 % 2) == 0) {
            for (uint8_t r = 0; r < kRows; r++) light(static_cast<uint8_t>((r * kCols) + ((r + s.step8) % kCols)), 0.85f, hue);
        } else {
            for (uint8_t k = 0; k < 5; k++) light(static_cast<uint8_t>(s.rng.range(0, (kCols * kRows) - 1)), 0.7f + (s.energy * 0.3f), static_cast<uint8_t>(hue + (k * 30)));
        }
    }
    for (uint8_t i = 0; i < 8; i++) {  // a dancer's hit lights the tile under its feet
        if (s.hitNow[i] > 0.0f) light(static_cast<uint8_t>((i * 2) + (s.rng.range(0, 1))), s.hitNow[i], static_cast<uint8_t>(i * 32));
    }
    for (uint8_t i = 0; i < kCols * kRows; i++) {
        s.tileLvl[i] *= 0.86f;
        const uint8_t q = static_cast<uint8_t>(s.tileLvl[i] * 7.99f);
        if (q == s.tileDrawn[i]) continue;
        tile(i, (q == 0) ? dark : themeColor(t, s.tileHue[i], static_cast<uint8_t>(40 + (q * 30))));
        s.tileDrawn[i] = q;
    }
}

void Controller::visual_scenePiano(bool init_) {
    /// @brief PIANO ROLL: every note of the bank's melody layers (TONE / SYNTH) at its pitch and
    /// length across the loop, in its layer's colour; harmony layers as outlined notes beside
    /// their leader. Notes light up as the play head passes; a song TRANSPOSE moves them all and
    /// shows the new key step.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kX0 = 40, kWd = 880, kTop = 40, kBot = 500;

    // 1. COLLECT THE NOTES (a few times a second; a change redraws the roll).
    if (init_ || ((visualFrame % 12) == 0)) {
        const int8_t tr = song_transposeNow();
        uint32_t sig = 2166136261u ^ static_cast<uint32_t>(tr) ^ (activeSongBank << 8) ^ (songInterval << 12);
        uint16_t n = 0;
        int lo = 1000, hi = -1000;
        for (uint8_t l = 0; (l < kLayerLibrarySize) && (n < kRollNotes); l++) {
            const Layer& layer = layerLibrary[l];
            if (!tone::playsNotes(layer.toneMode) || layer_follows(l)) continue;
            const Bank& bank = layer.bankLibrary[activeSongBank];
            for (int16_t k = 0; (k <= bank.lastActiveBeatNum) && (n < kRollNotes); k++) {
                const Beat& beat = bank.beatLibrary[k];
                if (!beat.getActive()) continue;
                const uint16_t st = beat.getStartInterval();
                uint16_t len = (beat.getHold() > 0) ? beat.getHold() : ((beat.getEndInterval() > st) ? (beat.getEndInterval() - st) : 200);
                auto add = [&](uint8_t layerIdx, int semis, bool outline) {
                    if (n >= kRollNotes) return;
                    RollNote& r = s.roll[n++];
                    r.start = st;
                    r.len = len;
                    r.semis = static_cast<int8_t>(clampi(semis, -60, 60));
                    r.layer = layerIdx;
                    r.outline = outline;
                    lo = (semis < lo) ? semis : lo;
                    hi = (semis > hi) ? semis : hi;
                    sig = (sig ^ (st + (static_cast<uint32_t>(semis + 64) << 16) + (layerIdx << 24))) * 16777619u;
                };
                add(l, tone::semitones(beat.getNote() + tr, layer.toneScale, layer.toneRoot), false);
                for (uint8_t h = 0; h < kLayerLibrarySize; h++) {  // its harmony layers
                    const Layer& f = layerLibrary[h];
                    if ((h == l) || (f.followLayer != static_cast<int8_t>(l)) || !tone::playsNotes(f.toneMode)) continue;
                    add(h, tone::semitones(harmony::note(beat.getNote(), f.harmonyInterval, layer.toneScale) + tr, layer.toneScale, layer.toneRoot), true);
                }
            }
        }
        if (init_ || (sig != s.rollSig)) {
            box(d, 0, kTop - 12, kW, (kBot - kTop) + 24, BLACK);
            s.rollCount = n;
            s.rollSig = sig;
            if (n == 0) {
                label(d, "NO MELODY LAYERS: SET A LAYER TO TONE OR SYNTH ON ITS NOTE PAGE", FONT_07x09, CENTER, 480, 270, themeColor(t, 0), BLACK);
            } else {
                if (hi - lo < 12) {  // at least an octave of room
                    const int mid = (hi + lo) / 2;
                    lo = mid - 6;
                    hi = mid + 6;
                }
                s.rollLo = static_cast<int16_t>(lo - 1);
                s.rollHi = static_cast<int16_t>(hi + 1);
                const int range = s.rollHi - s.rollLo;
                for (int semi = s.rollLo; semi <= s.rollHi; semi++) {  // octave lines on every C
                    if (((semi % 12) + 12) % 12 != 0) continue;
                    const int y = kBot - (((semi - s.rollLo) * (kBot - kTop)) / range);
                    box(d, kX0, y, kWd, 1, vis::rgb(40, 40, 52));
                }
                for (uint16_t i = 0; i < n; i++) s.roll[i].drawn = 0xFF;
            }
            if ((tr != s.rollTr) && !init_) {
                char msg[24];
                snprintf(msg, sizeof(msg), "TRANSPOSE %s%d", (tr > 0) ? "+" : "", tr);
                label(d, msg, FONT_10x14, CENTER, 480, kBot + 14, themeColor(t, 0), BLACK);
                s.rollBanner = 30;
            }
            s.rollTr = tr;
            s.rollHead = -1;
        }
    }
    if (init_) {
        s.rollBanner = 0;
        return;
    }
    if (s.rollBanner > 0) {
        if (--s.rollBanner == 0) box(d, 0, kBot + 12, kW, 18, BLACK);
    }
    if (s.rollCount == 0) return;

    // 2. THE PLAY HEAD (a marker above and below the roll) and the notes it passes.
    const int range = (s.rollHi > s.rollLo) ? (s.rollHi - s.rollLo) : 1;
    const int noteH = clampi((kBot - kTop) / range, 4, 16);
    const int head = (s.playing && (songInterval > 0)) ? static_cast<int>((static_cast<uint32_t>(playInterval) * kWd) / songInterval) : -1;
    const int prevHead = s.rollHead;
    if (head != s.rollHead) {
        if (s.rollHead >= 0) {
            box(d, kX0 + s.rollHead - 2, kTop - 12, 5, 6, BLACK);
            box(d, kX0 + s.rollHead - 2, kBot + 6, 5, 6, BLACK);
        }
        if (head >= 0) {
            box(d, kX0 + head - 2, kTop - 12, 5, 6, themeColor(t, 0));
            box(d, kX0 + head - 2, kBot + 6, 5, 6, themeColor(t, 0));
        }
        s.rollHead = static_cast<int16_t>(head);
    }
    for (uint16_t i = 0; i < s.rollCount; i++) {
        RollNote& r = s.roll[i];
        const int x = kX0 + static_cast<int>((static_cast<uint32_t>(r.start) * kWd) / songInterval);
        // Flash every note whose start the head passed since the last frame (it moves several
        // pixels per frame, and wraps at the loop end).
        const int rel = x - kX0;
        if ((head >= 0) && (prevHead >= 0) && (head != prevHead)) {
            const bool passed = (head > prevHead) ? ((rel > prevHead) && (rel <= head)) : ((rel > prevHead) || (rel <= head));
            if (passed) r.flash = 1.0f;
        }
        r.flash *= 0.85f;
        const uint8_t q = static_cast<uint8_t>(r.flash * 4.99f);
        if (q == r.drawn) continue;
        int w = static_cast<int>((static_cast<uint32_t>(r.len) * kWd) / songInterval) - 1;
        w = clampi(w, 3, kWd);
        const int y = kBot - (((r.semis - s.rollLo) * (kBot - kTop)) / range) - (noteH / 2);
        const uint16_t c = (q > 0) ? vis::dim(WHITE, static_cast<uint8_t>(140 + (q * 28))) : themeColor(t, r.layer * 25u, 200);
        if (r.outline) {
            box(d, x, y, w, noteH, BLACK);
            d.setForeColor(c);
            d.drawRect(static_cast<uint16_t>(clampi(x, 0, kW - 1)), static_cast<uint16_t>(clampi(y, 0, kH - 1)), static_cast<uint32_t>(w), static_cast<uint32_t>(noteH));
        } else {
            box(d, x, y, w, noteH, c);
        }
        r.drawn = q;
    }
}

void Controller::visual_sceneKeyWheel(bool init_) {
    /// @brief KEY WHEEL: the circle of fifths (major keys outside, their relative minors inside)
    /// with the song's key lit, a needle pointing at the chord the song is on right now (song
    /// TRANSPOSE walks it round), the chord's notes marked, and a heart beating with the bass.

    State& s = vs;
    Lcd& d = lcd;
    const uint8_t t = visualTheme;
    constexpr int kCx = 480, kCy = 270, kOuter = 200, kInner = 128;
    auto posAngle = [](int k) { return -1.5708f + (k * 0.5236f); };
    auto posOfMajor = [](int pc) { return ((pc * 7) % 12); };  // circle-of-fifths position of a major key

    // 1. WHICH KEY: the first melody layer that is not a harmony layer.
    int root = 0;
    uint8_t scale = tone::kDefaultScale;
    bool found = false;
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        if (tone::playsNotes(layerLibrary[l].toneMode) && !layer_follows(l)) {
            root = layerLibrary[l].toneRoot % 12;
            scale = tone::safeScale(layerLibrary[l].toneScale);
            found = true;
            break;
        }
    }
    const tone::Scale& sc = tone::kScales[scale];
    const bool minorKey = (sc.size >= 3) && (sc.steps[2] == 3);
    const int keyPos = minorKey ? posOfMajor((root + 3) % 12) : posOfMajor(root);
    // The chord: the scale step the song is on (TRANSPOSE), with its third deciding major / minor.
    const int size = sc.size;
    const int deg = ((song_transposeNow() % size) + size) % size;
    const int chordRoot = (root + sc.steps[deg]) % 12;
    int third = (size >= 3) ? (sc.steps[(deg + 2) % size] - sc.steps[deg]) : 4;
    if (third < 0) third += 12;
    const bool minorChord = (third == 3);
    const int chordPos = minorChord ? posOfMajor((chordRoot + 3) % 12) : posOfMajor(chordRoot);

    if (init_) {
        label(d, "MAJOR KEYS OUTSIDE - MINOR KEYS INSIDE", FONT_05x07, CENTER, 480, 20, vis::rgb(120, 120, 140), BLACK);
        for (int k = 0; k < 24; k++) s.wheelLit[k] = 0xFF;
        s.needleA = posAngle(chordPos);
        s.needleDrawn = false;
        s.keyText[0] = 0;
        s.glowR = 0;
        return;
    }

    // 2. THE DISCS: 12 majors outside, 12 minors inside; key = theme colour, chord = white/gold.
    for (int k = 0; k < 24; k++) {
        const bool inner = k >= 12;
        const int p = k % 12;
        const uint8_t lit = (p == chordPos) && (inner == minorChord) ? 2 : (((p == keyPos) && (inner == minorKey)) ? 1 : 0);
        const uint8_t state = static_cast<uint8_t>(lit + ((s.barStart && (lit == 2)) ? 3 : 0));
        if (state == s.wheelLit[k]) continue;
        const float a = posAngle(p);
        const int r = inner ? kInner : kOuter;
        const int x = kCx + static_cast<int>(cosf(a) * r * 1.25f);
        const int y = kCy + static_cast<int>(sinf(a) * r);
        const int rad = inner ? 24 : 30;
        const uint16_t c = (lit == 2) ? ((state > 2) ? WHITE : themeColor(t, 0)) : ((lit == 1) ? themeColor(t, 128, 200) : vis::rgb(36, 36, 48));
        disc(d, x, y, rad, c);
        const int pc = inner ? (((p * 7) + 9) % 12) : ((p * 7) % 12);
        label(d, tone::kNoteNames[pc], inner ? FONT_07x09 : FONT_10x14, CENTER, x, y - (inner ? 4 : 7), (lit > 0) ? BLACK : vis::rgb(170, 170, 190), c);
        s.wheelLit[k] = state;
    }

    // 3. THE NEEDLE glides to the chord.
    float target = posAngle(chordPos);
    while (target - s.needleA > 3.14159f) target -= 6.28318f;
    while (target - s.needleA < -3.14159f) target += 6.28318f;
    const float a = s.needleA + ((target - s.needleA) * 0.2f);
    const int x0 = kCx + static_cast<int>(cosf(a) * 48.0f * 1.25f), y0 = kCy + static_cast<int>(sinf(a) * 48.0f);
    const int x1 = kCx + static_cast<int>(cosf(a) * 96.0f * 1.25f), y1 = kCy + static_cast<int>(sinf(a) * 96.0f);
    if (!s.needleDrawn || (x1 != s.needle[2]) || (y1 != s.needle[3])) {
        if (s.needleDrawn) {
            for (int w = -1; w <= 1; w++) line(d, s.needle[0] + w, s.needle[1], s.needle[2] + w, s.needle[3], BLACK);
        }
        for (int w = -1; w <= 1; w++) line(d, x0 + w, y0, x1 + w, y1, themeColor(t, 0));
        const int16_t n[4] = {static_cast<int16_t>(x0), static_cast<int16_t>(y0), static_cast<int16_t>(x1), static_cast<int16_t>(y1)};
        memcpy(s.needle, n, sizeof(n));
        s.needleDrawn = true;
    }
    s.needleA = a;

    // 4. THE HEART beats with the bass.
    const int16_t glow = static_cast<int16_t>(14 + (s.bass * 26.0f));
    if (glow < s.glowR) disc(d, kCx, kCy, s.glowR, BLACK);
    disc(d, kCx, kCy, glow, themeColor(t, visualFrame * 2u, static_cast<uint8_t>(120 + (s.bass * 135.0f))));
    s.glowR = glow;

    // 5. KEY + CHORD in words.
    char text[64];
    if (!found) {
        snprintf(text, sizeof(text), "NO MELODY LAYERS - SHOWING C MAJOR");
    } else if (size == 7) {
        snprintf(text, sizeof(text), "KEY %s %s     CHORD %s %s %s", tone::kNoteNames[root], sc.name, kRoman[deg], tone::kNoteNames[chordRoot], minorChord ? "MINOR" : "MAJOR");
    } else {
        snprintf(text, sizeof(text), "KEY %s %s     STEP %d  %s", tone::kNoteNames[root], sc.name, deg + 1, tone::kNoteNames[chordRoot]);
    }
    if (strcmp(text, s.keyText) != 0) {
        box(d, 0, 506, kW, 18, BLACK);
        label(d, text, FONT_10x14, CENTER, 480, 508, themeColor(t, 0), BLACK);
        static_assert(sizeof(s.keyText) == sizeof(text), "same size: a plain copy keeps the terminator");
        memcpy(s.keyText, text, sizeof(s.keyText));
    }
}
