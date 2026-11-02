#ifndef __EUCLID_PATTERN_H
#define __EUCLID_PATTERN_H

// Euclidean rhythms for the Layer Song EUCLID page. Pure arithmetic with no hardware
// dependencies; host-tested in 02 Software/Tests/test_euclid.cpp.
//
// E(hits, steps) spreads `hits` onsets as evenly as possible over `steps` steps, using
// Bjorklund's algorithm (the construction used in Toussaint, "The Euclidean Algorithm Generates
// Traditional Musical Rhythms"): E(3,8) = x..x..x.  E(5,8) = x.xx.xx.  E(2,5) = x.x..
//
// Units: song positions as used by Beat::startInterval (800 per quarter note).

#include <stdint.h>

#include "LayerSongEdit.h"  // BeatSnapshot

namespace euclid {

constexpr uint8_t kMaxSteps = 32;

// Bjorklund: out[0..steps-1] = true where a hit falls. steps is clamped to 1..kMaxSteps,
// hits to 0..steps.
inline void pattern(uint8_t hits, uint8_t steps, bool* out) {
    if (steps == 0) return;
    if (steps > kMaxSteps) steps = kMaxSteps;
    if (hits >= steps) {
        for (uint8_t i = 0; i < steps; i++) out[i] = true;
        return;
    }
    if (hits == 0) {
        for (uint8_t i = 0; i < steps; i++) out[i] = false;
        return;
    }

    // Each group is a bit string, most significant bit first.
    struct Group {
        uint64_t bits;
        uint8_t len;
    };
    Group a[kMaxSteps], b[kMaxSteps], next[kMaxSteps], rest[kMaxSteps];
    uint8_t na = hits, nb = static_cast<uint8_t>(steps - hits);
    for (uint8_t i = 0; i < na; i++) a[i] = Group{1, 1};
    for (uint8_t i = 0; i < nb; i++) b[i] = Group{0, 1};

    while (nb > 1) {
        const uint8_t m = (na < nb) ? na : nb;
        for (uint8_t i = 0; i < m; i++) {
            next[i] = Group{(a[i].bits << b[i].len) | b[i].bits, static_cast<uint8_t>(a[i].len + b[i].len)};
        }
        uint8_t nrest = 0;
        if (na > m) {
            for (uint8_t i = m; i < na; i++) rest[nrest++] = a[i];
        } else {
            for (uint8_t i = m; i < nb; i++) rest[nrest++] = b[i];
        }
        for (uint8_t i = 0; i < m; i++) a[i] = next[i];
        for (uint8_t i = 0; i < nrest; i++) b[i] = rest[i];
        na = m;
        nb = nrest;
    }

    uint8_t pos = 0;
    auto emit = [&](const Group& g) {
        for (int8_t k = static_cast<int8_t>(g.len - 1); k >= 0; k--) out[pos++] = ((g.bits >> k) & 1u) != 0;
    };
    for (uint8_t i = 0; i < na; i++) emit(a[i]);
    for (uint8_t i = 0; i < nb; i++) emit(b[i]);
}

// Moves the pattern `amount` steps later (wrapping): out[(i + amount) % steps] = in[i].
inline void rotate(bool* p, uint8_t steps, uint8_t amount) {
    if ((steps == 0) || (steps > kMaxSteps)) return;
    amount %= steps;
    if (amount == 0) return;
    bool tmp[kMaxSteps];
    for (uint8_t i = 0; i < steps; i++) tmp[(i + amount) % steps] = p[i];
    for (uint8_t i = 0; i < steps; i++) p[i] = tmp[i];
}

// Song positions of the hits. One cycle is steps * grid long; with `repeat` it is laid down
// back to back until the song ends, otherwise once (both cut at the song end). At most maxOut
// positions are written; `truncated` is set if hits had to be dropped. Returns the count,
// in ascending order.
inline uint8_t positions(const bool* p, uint8_t steps, uint16_t grid, uint16_t song, bool repeat,
                         uint16_t* out, uint8_t maxOut, bool& truncated) {
    truncated = false;
    if ((steps == 0) || (grid == 0) || (song == 0)) return 0;
    const uint32_t cycle = static_cast<uint32_t>(steps) * grid;
    uint8_t count = 0;
    for (uint32_t start = 0; start < song; start += cycle) {
        for (uint8_t i = 0; i < steps; i++) {
            const uint32_t pos = start + static_cast<uint32_t>(i) * grid;
            if (pos >= song) break;
            if (!p[i]) continue;
            if (count >= maxOut) {
                truncated = true;
                return count;
            }
            out[count++] = static_cast<uint16_t>(pos);
        }
        if (!repeat) break;
    }
    return count;
}

// Builds the bank contents: REPLACE (merge = false) -> only the Euclid hits, as plain beats;
// MERGE -> the base beats (with their fills) plus every hit that does not land on a base beat.
// At most maxOut beats; `truncated` is set if hits had to be dropped. Unsorted.
inline uint8_t compose(const BeatSnapshot* base, uint8_t baseCount, const uint16_t* hits, uint8_t hitCount,
                       bool merge, BeatSnapshot* out, uint8_t maxOut, bool& truncated) {
    truncated = false;
    uint8_t count = 0;
    if (merge) {
        for (uint8_t i = 0; (i < baseCount) && (count < maxOut); i++) out[count++] = base[i];
    }
    for (uint8_t h = 0; h < hitCount; h++) {
        bool taken = false;
        for (uint8_t i = 0; i < count; i++) {
            if (out[i].startInterval == hits[h]) {
                taken = true;
                break;
            }
        }
        if (taken) continue;
        if (count >= maxOut) {
            truncated = true;
            break;
        }
        out[count++] = BeatSnapshot{hits[h], 0, 0, 0, 0, 0, false};
    }
    return count;
}

}  // namespace euclid

#endif
