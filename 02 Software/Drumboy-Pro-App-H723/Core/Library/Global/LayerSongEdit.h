#ifndef __LAYER_SONG_EDIT_H
#define __LAYER_SONG_EDIT_H

// Pure position arithmetic for the Layer Song edit functions (MOVE, SHIFT ALL, single-beat
// ADD). No hardware, HAL or Controller dependencies, so the exact same code runs on the
// device and in the host unit test (02 Software/Tests/test_layer_song_edit.cpp).
//
// Units: song positions ("intervals") as used by Beat::startInterval; 800 per quarter note.
// "step" is the quantize interval from the Rhythm menu (kQuantizeInterval).

#include <stdint.h>

// Everything needed to recreate one beat, independent of its slot index and of the shared
// BeatFill pool. Controller::layerSong_rebuildBank() turns a list of these back into a Bank.
struct BeatSnapshot {
    uint16_t startInterval;
    uint8_t fillType;
    uint8_t fillPattern;
    uint8_t fillTime;
    uint8_t fillLevel;
    int8_t note;  // TONE layers (last two, so brace-initialised snapshots default to the root...)
    bool slide;   // ...without SLIDE)
};

namespace layersong {

// Sorts beats by start interval and removes duplicate positions. `tracked` is an index into
// `beats` on entry and the index of the same beat on return (-1 if it was not valid). When
// two beats share a position the tracked one is kept. Returns the new count.
inline uint8_t sortUnique(BeatSnapshot* beats, uint8_t count, int8_t& tracked) {
    for (uint8_t i = 1; i < count; i++) {
        const BeatSnapshot key = beats[i];
        uint8_t k = i;
        while ((k > 0) && (beats[k - 1].startInterval > key.startInterval)) {
            beats[k] = beats[k - 1];
            k--;
        }
        beats[k] = key;
        if (tracked == static_cast<int8_t>(i)) {
            tracked = static_cast<int8_t>(k);
        } else if ((tracked >= static_cast<int8_t>(k)) && (tracked < static_cast<int8_t>(i))) {
            tracked += 1;
        }
    }

    uint8_t unique = 0;
    for (uint8_t i = 0; i < count; i++) {
        if ((unique > 0) && (beats[i].startInterval == beats[unique - 1].startInterval)) {
            if (tracked == static_cast<int8_t>(i)) {
                beats[unique - 1] = beats[i];
                tracked = static_cast<int8_t>(unique - 1);
            } else if (tracked > static_cast<int8_t>(i)) {
                tracked -= 1;
            }
            continue;
        }
        beats[unique] = beats[i];
        if (tracked == static_cast<int8_t>(i)) tracked = static_cast<int8_t>(unique);
        unique++;
    }

    if ((tracked < 0) || (tracked >= static_cast<int8_t>(unique))) tracked = -1;
    return unique;
}

// MOVE: target position for beat `sel` one grid line to the right (direction=true) or left.
// Off-grid beats snap to the neighbouring grid line first. Positions held by other beats are
// skipped, so a beat can pass its neighbours. Returns -1 at the song edge (no wrap).
inline int32_t moveTarget(const BeatSnapshot* beats, uint8_t count, uint8_t sel, bool direction,
                          uint16_t step, uint16_t song) {
    if ((sel >= count) || (step == 0) || (song == 0)) return -1;
    const int32_t s = step;
    const int32_t start = beats[sel].startInterval;

    int32_t candidate = direction ? ((start / s) + 1) * s : ((start % s) ? (start / s) * s : start - s);

    for (;;) {
        if ((candidate < 0) || (candidate >= song)) return -1;
        bool occupied = false;
        for (uint8_t i = 0; i < count; i++) {
            if ((i != sel) && (beats[i].startInterval == candidate)) {
                occupied = true;
                break;
            }
        }
        if (!occupied) return candidate;
        candidate += direction ? s : -s;
    }
}

// SHIFT ALL: rotates every position by one step, wrapping at the song length. Relative timing
// (including off-grid beats) is preserved; turning the other way restores the pattern.
// Returns false when the step is a whole multiple of the song length (nothing would change).
inline bool shiftAll(BeatSnapshot* beats, uint8_t count, bool direction, uint16_t step, uint16_t song) {
    if (song == 0) return false;
    const uint32_t len = song;
    const uint32_t s = static_cast<uint32_t>(step) % len;
    if (s == 0) return false;
    for (uint8_t i = 0; i < count; i++) {
        const uint32_t pos = beats[i].startInterval;
        beats[i].startInterval = static_cast<uint16_t>(direction ? (pos + s) % len : (pos + len - s) % len);
    }
    return true;
}

// ADD tap: position for a new beat in the gap after beat `sel` (beats sorted by position).
// Picks the grid line closest to the middle of the gap that lies strictly inside it; after
// the last beat the gap wraps around to the first beat. An empty bank gives 0.
// Returns -1 when the gap holds no free grid line.
inline int32_t insertPosition(const BeatSnapshot* beats, uint8_t count, int8_t sel, uint16_t step, uint16_t song) {
    if ((step == 0) || (song == 0)) return -1;
    if (count == 0) return 0;

    const uint32_t len = song;
    const uint32_t s = step;
    const uint8_t i = ((sel >= 0) && (sel < static_cast<int8_t>(count))) ? static_cast<uint8_t>(sel) : 0;
    const uint32_t gapStart = beats[i].startInterval;
    const uint32_t gapEnd = (i + 1u < count) ? beats[i + 1].startInterval : len + beats[0].startInterval;

    const uint32_t firstLine = ((gapStart / s) + 1) * s;  // first grid line after gapStart
    if (firstLine >= gapEnd) return -1;
    const uint32_t lastLine = ((gapEnd - 1) / s) * s;     // last grid line before gapEnd

    const uint32_t middle = gapStart + ((gapEnd - gapStart) / 2);
    uint32_t snapped = ((middle + (s / 2)) / s) * s;
    if (snapped < firstLine) snapped = firstLine;
    if (snapped > lastLine) snapped = lastLine;
    const uint32_t position = snapped % len;

    for (uint8_t k = 0; k < count; k++) {
        if (beats[k].startInterval == position) return -1;  // never stack two beats
    }
    return static_cast<int32_t>(position);
}

}  // namespace layersong

#endif
