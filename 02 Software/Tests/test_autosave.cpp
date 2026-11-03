// Host tests for Global/Autosave.h (AUTOSAVE timing, settings byte, crash-safe commit record).
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_autosave.cpp -o test_autosave && ./test_autosave
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cstdio>
#include <cstring>

#include "Autosave.h"

static int checks = 0;
static int failures = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        checks++;                                                        \
        if (!(c)) {                                                      \
            failures++;                                                  \
            std::printf("FAIL line %d: %s\n", __LINE__, #c);             \
        }                                                                \
    } while (0)

static autosave::State base() {
    autosave::State s;
    s.enabled = true;
    s.sdReady = true;
    s.dirty = true;
    s.busy = false;
    s.playing = false;
    s.now = 100000;
    s.lastInput = 100000 - autosave::kIdleStoppedMs;
    s.lastSave = 0;
    s.savedOnce = false;
    return s;
}

static void testDue() {
    autosave::State s = base();
    CHECK(autosave::due(s));  // stopped, idle exactly 10 s

    s = base(); s.enabled = false;  CHECK(!autosave::due(s));
    s = base(); s.sdReady = false;  CHECK(!autosave::due(s));
    s = base(); s.dirty = false;    CHECK(!autosave::due(s));
    s = base(); s.busy = true;      CHECK(!autosave::due(s));
    s = base(); s.lastInput += 1;   CHECK(!autosave::due(s));  // 9.999 s idle

    // Playing: needs 30 s hands-off.
    s = base(); s.playing = true;
    CHECK(!autosave::due(s));
    s.lastInput = s.now - autosave::kIdlePlayingMs;
    CHECK(autosave::due(s));

    // Minimum gap after the last auto-save.
    s = base(); s.savedOnce = true; s.lastSave = s.now - 1000;
    CHECK(!autosave::due(s));
    s.lastSave = s.now - autosave::kMinGapMs;
    CHECK(autosave::due(s));

    // Tick counter wrap-around (HAL_GetTick after ~49 days).
    s = base();
    s.lastInput = 0xFFFFFFFFu - 5000;  // 5 s before the wrap
    s.now = 5001;                      // 10.002 s later
    CHECK(autosave::due(s));
    s.now = 4000;                      // 9.001 s later
    CHECK(!autosave::due(s));
}

static void testSettingByte() {
    CHECK(autosave::decodeEnabled(autosave::encodeEnabled(true)) == true);
    CHECK(autosave::decodeEnabled(autosave::encodeEnabled(false)) == false);
    // Bytes from older firmware (or factory files) -> default ON.
    for (int b = 0; b < 256; b++) {
        if ((b & 0xF0) == autosave::kSettingTag) continue;
        CHECK(autosave::decodeEnabled(static_cast<uint8_t>(b)) == autosave::kDefaultEnabled);
    }
    CHECK(autosave::decodePending(autosave::kPendingMark));
    CHECK(!autosave::decodePending(0));
    CHECK(!autosave::decodePending(1));
    CHECK(!autosave::decodePending(0xFF));
}

static void testRecord() {
    uint8_t rec[autosave::kRecordSize];
    char mainPath[autosave::kPathSize];
    char sidePath[autosave::kPathSize];
    uint8_t mode = 99;

    CHECK(autosave::encode("System/File/File_007.rwf", "System/File/File_007.rwn", autosave::kSideReplace, rec));
    CHECK(autosave::decode(rec, sizeof(rec), mainPath, sidePath, mode));
    CHECK(std::strcmp(mainPath, "System/File/File_007.rwf") == 0);
    CHECK(std::strcmp(sidePath, "System/File/File_007.rwn") == 0);
    CHECK(mode == autosave::kSideReplace);

    CHECK(autosave::encode("System/Drumkit/Drumkit_100.rwd", "", autosave::kSideKeep, rec));
    CHECK(autosave::decode(rec, sizeof(rec), mainPath, sidePath, mode));
    CHECK(mode == autosave::kSideKeep && sidePath[0] == 0);

    // Every torn record (power cut while writing it) is rejected.
    CHECK(autosave::encode("System/File/Autosave.rwf", "System/File/Autosave.rwn", autosave::kSideDelete, rec));
    for (uint16_t n = 0; n < autosave::kRecordSize; n++) CHECK(!autosave::decode(rec, n, mainPath, sidePath, mode));
    // ...and so is a torn record padded with zeros or 0xFF to full size.
    for (uint16_t n = 0; n < autosave::kRecordSize; n++) {
        for (int fill = 0; fill < 2; fill++) {
            uint8_t torn[autosave::kRecordSize];
            std::memcpy(torn, rec, n);
            std::memset(torn + n, fill ? 0xFF : 0x00, autosave::kRecordSize - n);
            CHECK(!autosave::decode(torn, sizeof(torn), mainPath, sidePath, mode));
        }
    }
    // Any single damaged byte is caught.
    int caught = 0;
    for (uint16_t i = 0; i < autosave::kRecordSize; i++) {
        for (int bit = 0; bit < 8; bit++) {
            uint8_t bad[autosave::kRecordSize];
            std::memcpy(bad, rec, sizeof(bad));
            bad[i] = static_cast<uint8_t>(bad[i] ^ (1 << bit));
            if (!autosave::decode(bad, sizeof(bad), mainPath, sidePath, mode)) caught++;
        }
    }
    CHECK(caught == autosave::kRecordSize * 8);

    // Paths that do not fit, an unknown mode, a missing side path.
    char longPath[64];
    std::memset(longPath, 'A', sizeof(longPath));
    longPath[63] = 0;
    CHECK(!autosave::encode(longPath, "", autosave::kSideKeep, rec));
    CHECK(!autosave::encode("System/File/File_001.rwf", "", 3, rec));
    CHECK(autosave::encode("System/File/File_001.rwf", "", autosave::kSideReplace, rec));
    CHECK(!autosave::decode(rec, sizeof(rec), mainPath, sidePath, mode));
}

static void testPlan() {
    using autosave::plan;
    // No valid record: discard temps, touch nothing else.
    for (int m = 0; m < 3; m++) {
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                const autosave::Plan p = plan(false, static_cast<uint8_t>(m), a, b);
                CHECK(!p.replaceSide && !p.deleteSide && !p.replaceMain && p.discardTemps);
            }
        }
    }
    // Committed, nothing replaced yet.
    autosave::Plan p = plan(true, autosave::kSideReplace, true, true);
    CHECK(p.replaceSide && !p.deleteSide && p.replaceMain);
    // Side already moved, main not yet (power cut in between).
    p = plan(true, autosave::kSideReplace, true, false);
    CHECK(!p.replaceSide && p.replaceMain);
    // Everything already moved (power cut before the record was deleted): nothing to redo.
    p = plan(true, autosave::kSideReplace, false, false);
    CHECK(!p.replaceSide && !p.deleteSide && !p.replaceMain);
    // Drum-only project: the old side file goes.
    p = plan(true, autosave::kSideDelete, true, false);
    CHECK(p.deleteSide && !p.replaceSide && p.replaceMain);
    // Drumkit: no side file at all.
    p = plan(true, autosave::kSideKeep, true, false);
    CHECK(!p.deleteSide && !p.replaceSide && p.replaceMain);
}

int main() {
    testDue();
    testSettingByte();
    testRecord();
    testPlan();
    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
