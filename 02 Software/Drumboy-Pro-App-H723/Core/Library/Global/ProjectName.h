#ifndef __PROJECT_NAME_H
#define __PROJECT_NAME_H

// Names for projects and drumkits (v2.02): 10 characters, edited on the device with three knobs,
// or rolled at random from synth-flavoured words ("NEON YAK", "TURBO DUCK", "DJ BLOOP").
// Pure C++ with no hardware access, so it is host-tested (02 Software/Tests/test_project_name.cpp).
//
// On the card the name lives in 10 bytes of the 1500-byte drumkit block that no firmware uses
// (kNameOffset, after the AUTOSAVE origin bytes). Slots saved before v2.02 hold zeros there
// (saved) or spaces (cleared): both read as "no name", and the menu shows FILE_007 as before.

#include <stdint.h>
#include <string.h>

namespace pname {

constexpr uint8_t kLength = 10;  // = one menu value cell
constexpr uint16_t kNameOffset = 490;  // in the drumkit block of .rwf and .rwd files

// The characters the screen font has, in knob order (space first = "erase").
constexpr char kCharset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!-+.?#";
constexpr uint8_t kCharsetSize = sizeof(kCharset) - 1;

inline int charIndex(char c) {
    for (uint8_t i = 0; i < kCharsetSize; i++) {
        if (kCharset[i] == c) return i;
    }
    return -1;
}

// Next / previous character for the NAME knob (wraps around).
inline char stepChar(char c, bool up) {
    int i = charIndex(c);
    if (i < 0) i = 0;
    i = up ? (i + 1) % kCharsetSize : (i + kCharsetSize - 1) % kCharsetSize;
    return kCharset[i];
}

// A name in RAM is always kLength characters (padded with spaces) plus a terminating 0.
inline void clear(char* name) {
    memset(name, ' ', kLength);
    name[kLength] = 0;
}

inline bool isEmpty(const char* name) {
    for (uint8_t i = 0; i < kLength; i++) {
        if (name[i] != ' ') return false;
    }
    return true;
}

// Card bytes -> name. Anything that is not a name (old files, damage) gives an empty name.
inline void decode(const uint8_t* bytes, char* name) {
    clear(name);
    for (uint8_t i = 0; i < kLength; i++) {
        const char c = (bytes[i] == 0) ? ' ' : static_cast<char>(bytes[i]);
        if (charIndex(c) < 0) {
            clear(name);
            return;
        }
        name[i] = c;
    }
}

inline void encode(const char* name, uint8_t* bytes) {
    for (uint8_t i = 0; i < kLength; i++) bytes[i] = static_cast<uint8_t>(name[i]);
}

// ---- Random names ----------------------------------------------------------------------------

constexpr const char* kFirst[] = {
    "ACID",  "NEON",  "MEGA",  "TURBO", "LASER", "DISCO", "COSMIC", "ROBO",  "HYPER", "ULTRA",
    "FUZZY", "FUNKY", "LOFI",  "ZAPPY", "WONKY", "SPACE", "RETRO",  "SONIC", "ATOMIC", "GROOVY",
    "SNEAKY", "JAZZY", "DIRTY", "SHINY", "SLEEPY", "CRISPY", "BOUNCY", "WOBBLY", "BLEEPY", "SPICY",
    "TINY",  "GIANT", "MOODY", "LUCKY", "FANCY", "ANALOG", "CHROME", "NIGHT", "SUNNY", "FROSTY"};
constexpr const char* kSecond[] = {
    "YAK",   "DUCK",  "MOOSE", "LLAMA", "PANDA", "SQUID", "TOAD",  "GECKO", "GOAT",  "OTTER",
    "BLOOP", "WOMP",  "ZAP",   "BEEP",  "FUZZ",  "BOT",   "WAVE",  "PULSE", "ECHO",  "DRONE",
    "TACO",  "DONUT", "WAFFLE", "PICKLE", "NOODLE", "MANGO", "BAGEL", "MUFFIN", "WIZARD", "NINJA",
    "COMET", "ROCKET", "UFO",  "ALIEN", "GOBLIN", "DRAGON", "ROBOT", "PIRATE", "VIKING", "YETI"};
constexpr const char* kTitles[] = {"DJ", "MC", "LIL", "BIG", "DR", "MR", "MS", "SIR", "KING", "QUEEN"};
constexpr const char* kNumbers[] = {"303", "808", "909", "101", "2000", "3000", "64", "99", "XL", "MK2"};

constexpr uint8_t kFirstCount = sizeof(kFirst) / sizeof(kFirst[0]);
constexpr uint8_t kSecondCount = sizeof(kSecond) / sizeof(kSecond[0]);
constexpr uint8_t kTitleCount = sizeof(kTitles) / sizeof(kTitles[0]);
constexpr uint8_t kNumberCount = sizeof(kNumbers) / sizeof(kNumbers[0]);

inline uint32_t nextRandom(uint32_t& seed) {  // xorshift32; a zero seed is replaced
    if (seed == 0) seed = 0x6D2B79F5u;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

// "WORD WORD" if it fits in kLength, else false.
inline bool join(const char* a, const char* b, char* name) {
    const size_t la = strlen(a);
    const size_t lb = strlen(b);
    if (la + 1 + lb > kLength) return false;
    clear(name);
    memcpy(name, a, la);
    memcpy(name + la + 1, b, lb);
    return true;
}

// A new random name, different from `previous` (may be null). Patterns: ADJECTIVE THING
// (most), TITLE THING ("DJ BLOOP"), THING NUMBER ("YAK 303").
inline void generate(uint32_t& seed, const char* previous, char* name) {
    for (int attempt = 0; attempt < 64; attempt++) {
        const uint32_t pattern = nextRandom(seed) % 8;
        const char* thing = kSecond[nextRandom(seed) % kSecondCount];
        bool ok;
        if (pattern < 5) {
            ok = join(kFirst[nextRandom(seed) % kFirstCount], thing, name);
        } else if (pattern < 7) {
            ok = join(kTitles[nextRandom(seed) % kTitleCount], thing, name);
        } else {
            ok = join(thing, kNumbers[nextRandom(seed) % kNumberCount], name);
        }
        if (ok && ((previous == nullptr) || (memcmp(previous, name, kLength) != 0))) return;
    }
    join("NEW", "SONG", name);  // practically unreachable
}

}  // namespace pname

#endif
