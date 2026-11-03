#ifndef __AUTOSAVE_H
#define __AUTOSAVE_H

// AUTOSAVE (v2.01): when to save the session by itself, and the crash-safe way every save is
// written. Pure C++ with no hardware access, so it is host-tested (02 Software/Tests/
// test_autosave.cpp). The SD work is in ControllerAutosave.cpp.
//
// Safe save: a save first writes complete temporary files, then a small commit record that
// names where they go, and only then replaces the real files (delete + rename). A power cut
// at any moment leaves either the old files or the new ones:
//   - before the commit record is complete: the temporary files are thrown away, the old
//     files were never touched;
//   - after it: the replace is finished (again) at the next save or power-on.

#include <stdint.h>
#include <string.h>

namespace autosave {

// ---- When to auto-save ---------------------------------------------------------------------

constexpr uint32_t kIdleStoppedMs = 10000;   // stopped: 10 s after the last touch
constexpr uint32_t kIdlePlayingMs = 30000;   // playing: only after 30 s hands-off
constexpr uint32_t kMinGapMs = 30000;        // never more often than every 30 s

struct State {
    bool enabled;        // AUTOSAVE ON in the File menu
    bool sdReady;
    bool dirty;          // something changed since the last auto-save
    bool busy;           // alert shown, recording, file load/save running
    bool playing;
    uint32_t now;        // ms ticks (wrap-safe arithmetic below)
    uint32_t lastInput;  // last key / knob event
    uint32_t lastSave;   // last auto-save attempt (0 = none yet this session)
    bool savedOnce;
};

inline bool due(const State& s) {
    if (!s.enabled || !s.sdReady || !s.dirty || s.busy) return false;
    const uint32_t idle = s.now - s.lastInput;
    if (idle < (s.playing ? kIdlePlayingMs : kIdleStoppedMs)) return false;
    if (s.savedOnce && ((s.now - s.lastSave) < kMinGapMs)) return false;
    return true;
}

// ---- Settings byte in System.rwp -----------------------------------------------------------

// Stored with a tag in the high nibble: older files hold unrelated bytes at this position, and
// anything without the tag means "never set" -> the default (ON).
constexpr uint8_t kSettingTag = 0xA0;
constexpr bool kDefaultEnabled = true;

inline uint8_t encodeEnabled(bool on) { return static_cast<uint8_t>(kSettingTag | (on ? 1 : 0)); }
inline bool decodeEnabled(uint8_t b) {
    if ((b & 0xF0) != kSettingTag) return kDefaultEnabled;
    return (b & 0x0F) == 1;
}

// "An auto-save is waiting to be resumed": set by the first auto-save of a session, cleared
// once the RESUME question at power-on is answered.
constexpr uint8_t kPendingMark = 0xA5;
inline bool decodePending(uint8_t b) { return b == kPendingMark; }

// ---- Commit record ---------------------------------------------------------------------------

constexpr uint8_t kPathSize = 40;  // "System/Drumkit/Drumkit_100.rwd" = 30
constexpr uint8_t kSideKeep = 0;     // no second file
constexpr uint8_t kSideReplace = 1;  // second file from its temporary file
constexpr uint8_t kSideDelete = 2;   // second file is no longer needed (drum-only project)

// "RWCOMMIT" | main[40] | side[40] | side mode | checksum | "END"
constexpr uint16_t kRecordSize = 8 + kPathSize + kPathSize + 1 + 1 + 3;

inline uint8_t checksum(const uint8_t* rec) {
    uint8_t sum = 0x5A;
    for (uint16_t i = 8; i < 8 + kPathSize + kPathSize + 1; i++) sum = static_cast<uint8_t>((sum << 1 | sum >> 7) ^ rec[i]);
    return sum;
}

// Returns false when a path does not fit.
inline bool encode(const char* mainPath, const char* sidePath, uint8_t sideMode, uint8_t* rec) {
    if ((strlen(mainPath) >= kPathSize) || (strlen(sidePath) >= kPathSize) || (sideMode > kSideDelete)) return false;
    memset(rec, 0, kRecordSize);
    memcpy(rec, "RWCOMMIT", 8);
    memcpy(rec + 8, mainPath, strlen(mainPath));
    memcpy(rec + 8 + kPathSize, sidePath, strlen(sidePath));
    rec[8 + 2 * kPathSize] = sideMode;
    rec[8 + 2 * kPathSize + 1] = checksum(rec);
    memcpy(rec + 8 + 2 * kPathSize + 2, "END", 3);
    return true;
}

// A torn or damaged record (power cut while it was written) is rejected.
inline bool decode(const uint8_t* rec, uint16_t size, char* mainPath, char* sidePath, uint8_t& sideMode) {
    if (size != kRecordSize) return false;
    if (memcmp(rec, "RWCOMMIT", 8) != 0) return false;
    if (memcmp(rec + 8 + 2 * kPathSize + 2, "END", 3) != 0) return false;
    if (rec[8 + 2 * kPathSize + 1] != checksum(rec)) return false;
    if ((rec[8 + kPathSize - 1] != 0) || (rec[8 + 2 * kPathSize - 1] != 0)) return false;  // terminated
    sideMode = rec[8 + 2 * kPathSize];
    if (sideMode > kSideDelete) return false;
    if (rec[8] == 0) return false;  // no destination
    memcpy(mainPath, rec + 8, kPathSize);
    memcpy(sidePath, rec + 8 + kPathSize, kPathSize);
    if ((sideMode != kSideKeep) && (sidePath[0] == 0)) return false;
    return true;
}

// What to do with what is on the card (at power-on, and before every save).
struct Plan {
    bool replaceSide;  // delete side, rename temp side -> side
    bool deleteSide;   // delete side
    bool replaceMain;  // delete main, rename temp main -> main
    bool discardTemps; // delete the temporary files
};

inline Plan plan(bool recordValid, uint8_t sideMode, bool tempMainExists, bool tempSideExists) {
    Plan p = {false, false, false, true};
    if (!recordValid) return p;  // the save never got to its commit point: old files are intact
    // Side first, main last: while the temporary main file exists the replace is unfinished.
    if ((sideMode == kSideReplace) && tempSideExists) p.replaceSide = true;
    if (sideMode == kSideDelete) p.deleteSide = true;
    if (tempMainExists) p.replaceMain = true;
    return p;
}

}  // namespace autosave

#endif
