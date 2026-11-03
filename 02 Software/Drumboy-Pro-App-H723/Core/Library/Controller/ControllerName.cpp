// Names for projects and drumkits (v2.02).
// Part of class Controller (declared in Controller.h). Name rules and the random generator are in
// Global/ProjectName.h (host-tested by 02 Software/Tests/test_project_name.cpp).
//
// The session has a project name and a drumkit name. They are saved with the project / drumkit
// and come back when it is loaded; NEW (and power-on) rolls a fresh random name. In the File and
// Drumkit menus, knob 6 NAME changes the character at the cursor (shown in gold), knob 7 CURSOR
// moves it, knob 8 RANDOM rolls a new name. LOAD / SAVE / CLEAR show the slots' names, with the
// slot number in the header.

// Global.h's unused helpers would warn once more in this new file.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "Controller.h"
#pragma GCC diagnostic pop

namespace {

constexpr uint16_t kCharCell = 9;  // FONT_07x09: 7 px glyph + 2 px spacing

}  // namespace

char* Controller::name_target() {
    return (menu == DRUMKIT_MENU) ? kitName : projectName;
}

void Controller::name_initialize() {
    /// @brief Power-on: a random name for the new session's project and drumkit.

    nameSeed ^= DWT_CYCCNT ^ (HAL_GetTick() << 16) ^ sd.serial;
    pname::generate(nameSeed, nullptr, projectName);
    pname::generate(nameSeed, nullptr, kitName);
    nameCursor = 0;
}

void Controller::name_changeChar(bool up_) {
    char* name = name_target();
    name[nameCursor] = pname::stepChar(name[nameCursor], up_);
    lcd_drawName_Data();
}

void Controller::name_moveCursor(bool right_) {
    if (right_ && (nameCursor < pname::kLength - 1)) nameCursor++;
    if (!right_ && (nameCursor > 0)) nameCursor--;
    lcd_drawName_Data();
}

void Controller::name_roll(char* name_) {
    /// @brief A new random name, never the same as the current one.

    char previous[pname::kLength + 1];
    memcpy(previous, name_, sizeof(previous));
    nameSeed ^= DWT_CYCCNT ^ (HAL_GetTick() << 16);  // your timing makes it unpredictable
    pname::generate(nameSeed, previous, name_);
    nameCursor = 0;
}

void Controller::name_random() {
    name_roll(name_target());
    lcd_drawName_Data();
}

/* Drawing --------------------------------------------------------------------*/

void Controller::lcd_drawName_Row() {
    /// @brief Knobs 6-8 of the File / Drumkit menu: NAME, CURSOR, RANDOM.

    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderName, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderCursor, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderRandom, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    lcd_drawName_Data();
}

void Controller::lcd_drawName_Data() {
    /// @brief The name with the cursor's character in gold (a gold "_" on a space), the cursor
    /// position, and the RANDOM hint.

    if ((menu != FILE_MENU) && (menu != DRUMKIT_MENU)) return;
    const char* name = name_target();

    lcd_setMenuDataState(WHITE);
    lcd.drawText(name, pname::kLength, kMenuDataX[5], kMenuDataY[5]);

    // Right-aligned: the text starts kLength cells left of the cell's right edge.
    const uint16_t left = kMenuDataX[5] - (pname::kLength * kCharCell);
    const char mark = (name[nameCursor] == ' ') ? '_' : name[nameCursor];
    lcd_setMenuDataState(kAccentColor);
    lcd.drawText(&mark, 1, left + ((nameCursor + 1) * kCharCell), kMenuDataY[5]);

    char pos[pname::kLength + 1] = "          ";  // position 1..10, right-aligned
    const uint8_t p = static_cast<uint8_t>(nameCursor + 1);
    if (p >= 10) pos[pname::kLength - 2] = static_cast<char>('0' + (p / 10));
    pos[pname::kLength - 1] = static_cast<char>('0' + (p % 10));
    lcd_setMenuDataState(WHITE);
    lcd.drawText(pos, pname::kLength, kMenuDataX[6], kMenuDataY[6]);
    lcd.drawText("      TURN", pname::kLength, kMenuDataX[7], kMenuDataY[7]);
}

void Controller::lcd_drawSlotLabel(uint8_t tab_, bool drumkit_) {
    /// @brief LOAD / SAVE / CLEAR (tab 1..3): on the active tab the header shows the slot number
    /// ("LOAD   007") and the value the slot's name, or FILE_007 / DRUM_007 when it has none;
    /// the other tabs show their plain header and a dash.

    if ((tab_ < 1) || (tab_ > 3)) return;
    const bool active = (menuTab == tab_);
    const int16_t counter = drumkit_ ? drumkitMenuCounter : fileMenuCounter;
    const uint16_t slot = (counter >= 0) ? static_cast<uint16_t>((counter + 1) % 1000) : 0;
    const char num[4] = {static_cast<char>('0' + (slot / 100)), static_cast<char>('0' + ((slot / 10) % 10)), static_cast<char>('0' + (slot % 10)), 0};

    // Header
    const char* base = (tab_ == 1) ? kHeaderLoad : ((tab_ == 2) ? kHeaderSave : kHeaderClear);
    char header[kMenuHeaderTextSize + 1];
    memcpy(header, base, kMenuHeaderTextSize);
    header[kMenuHeaderTextSize] = 0;
    if (active) memcpy(header + kMenuHeaderTextSize - 3, num, 3);
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(header, kMenuHeaderTextSize, kMenuHeaderX[tab_], kMenuHeaderY[tab_]);

    // Value
    lcd_setMenuDataState(WHITE);
    if (!active) {
        lcd.drawText(kDataDashR, kMenuDataTextSize, kMenuDataX[tab_], kMenuDataY[tab_]);
        return;
    }
    const char* slotName = drumkit_ ? kitSlotName : fileSlotName;
    const FileStatus status = drumkit_ ? drumkitStatus : fileStatus;
    char text[kMenuDataTextSize + 1];
    if ((status == FILE_ACTIVE) && !pname::isEmpty(slotName)) {
        memcpy(text, slotName, kMenuDataTextSize);
        text[kMenuDataTextSize] = 0;
    } else {
        memcpy(text, drumkit_ ? "  DRUM_" : "  FILE_", 7);  // "  FILE_007"
        memcpy(text + 7, num, 3);
        text[kMenuDataTextSize] = 0;
    }
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[tab_], kMenuDataY[tab_]);
}
