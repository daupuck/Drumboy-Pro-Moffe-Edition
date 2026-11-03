// Host tests for Global/ProjectName.h (project / drumkit names, random name generator).
//
//   g++ -std=c++17 -Wall -Wextra -Werror -I"../Drumboy-Pro-App-H723/Core/Library/Global"
//       test_project_name.cpp -o test_project_name && ./test_project_name
//   (one command; no line-continuation backslash here, it would make this a multi-line comment)

#include <cstdio>
#include <cstring>
#include <set>
#include <string>

#include "ProjectName.h"

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

static bool onlyCharset(const char* s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (pname::charIndex(s[i]) < 0) return false;
    }
    return true;
}

static void testCharset() {
    // Every character is unique and drawable (A-Z, 0-9 and the font's symbols).
    std::set<char> seen;
    for (uint8_t i = 0; i < pname::kCharsetSize; i++) CHECK(seen.insert(pname::kCharset[i]).second);
    CHECK(pname::kCharset[0] == ' ');
    CHECK(pname::charIndex('a') < 0);  // lower case is never stored
    CHECK(pname::charIndex(0) < 0);

    // Stepping walks the whole set and wraps both ways.
    char c = ' ';
    for (uint8_t i = 0; i < pname::kCharsetSize; i++) c = pname::stepChar(c, true);
    CHECK(c == ' ');
    CHECK(pname::stepChar(' ', false) == '#');
    CHECK(pname::stepChar(' ', true) == 'A');
    CHECK(pname::stepChar('Z', true) == '0');
    CHECK(pname::stepChar('~', true) == 'A');  // unknown -> treated as space
}

static void testCodec() {
    char name[pname::kLength + 1];
    uint8_t bytes[pname::kLength];

    // Old saved slots: zeros. Old cleared slots: spaces. Both = no name.
    std::memset(bytes, 0, sizeof(bytes));
    pname::decode(bytes, name);
    CHECK(pname::isEmpty(name) && std::strlen(name) == pname::kLength);
    std::memset(bytes, ' ', sizeof(bytes));
    pname::decode(bytes, name);
    CHECK(pname::isEmpty(name));

    // Round trip.
    char in[pname::kLength + 1];
    pname::clear(in);
    std::memcpy(in, "NEON YAK", 8);
    pname::encode(in, bytes);
    pname::decode(bytes, name);
    CHECK(std::memcmp(name, "NEON YAK  ", pname::kLength) == 0 && name[pname::kLength] == 0);

    // Garbage (lower case, control bytes, 0xFF) -> no name rather than odd glyphs.
    const uint8_t junk[][pname::kLength] = {{'n', 'e', 'o', 'n', 0, 0, 0, 0, 0, 0},
                                            {'A', 'B', 0x01, 'C', 0, 0, 0, 0, 0, 0},
                                            {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    for (const auto& j : junk) {
        pname::decode(j, name);
        CHECK(pname::isEmpty(name));
    }
    // Zeros inside a name read as spaces.
    const uint8_t mixed[pname::kLength] = {'D', 'J', 0, 'Y', 'A', 'K', 0, 0, 0, 0};
    pname::decode(mixed, name);
    CHECK(std::memcmp(name, "DJ YAK    ", pname::kLength) == 0);
}

static void testWordLists() {
    // Every word uses only drawable characters and leaves room for a partner word.
    for (const char* w : pname::kFirst) CHECK(onlyCharset(w, std::strlen(w)) && std::strlen(w) <= pname::kLength - 4);
    for (const char* w : pname::kSecond) CHECK(onlyCharset(w, std::strlen(w)) && std::strlen(w) <= pname::kLength - 3);
    for (const char* w : pname::kTitles) CHECK(onlyCharset(w, std::strlen(w)));
    for (const char* w : pname::kNumbers) CHECK(onlyCharset(w, std::strlen(w)));
}

static void testGenerate() {
    uint32_t seed = 12345;
    char name[pname::kLength + 1];
    char prev[pname::kLength + 1];
    pname::clear(prev);
    std::set<std::string> distinct;
    for (int i = 0; i < 20000; i++) {
        pname::generate(seed, prev, name);
        CHECK(std::strlen(name) == pname::kLength);
        CHECK(onlyCharset(name, pname::kLength));
        CHECK(!pname::isEmpty(name));
        CHECK(name[0] != ' ');                                   // starts with a word
        CHECK(std::strstr(name, "  ") == nullptr || name[pname::kLength - 1] == ' ');  // one space between words
        CHECK(std::memcmp(name, prev, pname::kLength) != 0);    // RANDOM always changes it
        distinct.insert(name);
        std::memcpy(prev, name, sizeof(prev));
    }
    CHECK(distinct.size() > 1000);  // plenty of variety

    // A zero seed still works; null previous is allowed.
    uint32_t zero = 0;
    pname::generate(zero, nullptr, name);
    CHECK(!pname::isEmpty(name) && zero != 0);
}

int main() {
    testCharset();
    testCodec();
    testWordLists();
    testGenerate();
    std::printf("%s: %d checks, %d failures\n", failures ? "FAILED" : "OK", checks, failures);
    return failures ? 1 : 0;
}
