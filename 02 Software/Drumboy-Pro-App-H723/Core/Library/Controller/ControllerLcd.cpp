// LCD primitives, alerts, tabs, transitions and play screens.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Lcd Functions -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::lcd_initialize() {
    /// @brief Initializes LCD hardware driver and default display state.

    // 1. DRIVER INITIALIZATION
    // Initialize LCD peripheral and internal graphics context.
    lcd.initialize();
}

void Controller::lcd_test(TestMode mode_) {
    /// @brief Renders LCD diagnostics for selected hardware test mode.

    // 1. TEST MODE DISPATCH
    // Execute mode-specific diagnostic drawing routine.
    switch (mode_) {
        case TEST_NONE:
            break;

        case TEST_SCREEN: {
            // Display full-screen color and pattern test pattern
            lcd.clearScreen();
            lcd.setForeColor(GRAY_75);
            lcd.setBackColor(BLACK);
            uint16_t boxSize = 29;
            uint16_t xPos = 16;
            uint16_t yPos = 9;

            // Draw grid background reference lines
            for (uint8_t i = 0; i < 33; i++) {
                uint16_t x = xPos + (i * boxSize);
                lcd.drawVLine(x, yPos, 522);
            }
            for (uint8_t i = 0; i < 19; i++) {
                uint16_t y = yPos + (i * boxSize);
                lcd.drawHLine(16, y, 928);
            }

            // Draw white overlay grid for color test area
            lcd.setForeColor(WHITE);
            lcd.setBackColor(BLACK);
            for (uint8_t i = 4; i < 29; i++) {
                uint16_t x = xPos + (i * boxSize);
                lcd.drawVLine(x, yPos + (3 * boxSize), (12 * boxSize));
            }
            for (uint8_t i = 3; i < 16; i++) {
                uint16_t y = yPos + (i * boxSize);
                lcd.drawHLine(xPos + (4 * boxSize), y, (24 * boxSize));
            }

            // Fill corner markers to designate calibration points
            lcd.setForeColor(BLACK);
            lcd.fillRect(xPos + (0 * boxSize) + 1, yPos + (0 * boxSize) + 1, (boxSize * 3) - 1, (boxSize * 3) - 1);
            lcd.fillRect(xPos + (0 * boxSize) + 1, yPos + (15 * boxSize) + 1, (boxSize * 3) - 1, (boxSize * 3) - 1);
            lcd.fillRect(xPos + (29 * boxSize) + 1, yPos + (0 * boxSize) + 1, (boxSize * 3) - 1, (boxSize * 3) - 1);
            lcd.fillRect(xPos + (29 * boxSize) + 1, yPos + (15 * boxSize) + 1, (boxSize * 3) - 1, (boxSize * 3) - 1);

            // Draw solid color bars for RGB verification (primary colors)
            RGB16Color cArray01[] = {BLACK, RED, PINK, BLUE, CYAN, GREEN, YELLOW, WHITE};
            for (uint8_t i = 0; i < 8; i++) {
                lcd.setForeColor(cArray01[i]);
                lcd.fillRect(xPos + (4 * boxSize) + (i * 3 * boxSize) + 1, yPos + (3 * boxSize) + 1, (boxSize * 3) - 1, (boxSize * 3) - 1);
            }

            // Draw gradient bars to test color transition smoothness (24-step gradient)
            RGB16Color cArray02[] = {0x10A2, 0x18E3, 0x2945, 0x3186, 0x39E7, 0x4228, 0x528A, 0x5ACB, 0x632C, 0x6B6D, 0x7BCF, 0x8410, 0x8C71, 0x94B2, 0xA514, 0xAD55, 0xB5B6, 0xBDF7, 0xCE59, 0xD69A, 0xDEFB, 0xE73C, 0xF79E, 0xFFFF};
            for (uint8_t i = 0; i < 24; i++) {
                lcd.setForeColor(cArray02[i]);
                lcd.fillRect(xPos + (4 * boxSize) + (i * boxSize) + 1, yPos + (6 * boxSize) + 1, (boxSize * 1) - 1, (boxSize * 2) - 1);
            }

            // Draw black and white stripe pattern to test pixel accuracy and alignment
            lcd.setForeColor(BLACK);
            lcd.fillRect(xPos + (4 * boxSize) + 1, yPos + (8 * boxSize) + 1, (boxSize * 9) - 1, (boxSize * 2) - 1);
            lcd.fillRect(xPos + (4 * boxSize) + (9 * boxSize) + 1, yPos + (8 * boxSize) + 1, (boxSize * 6) - 1, (boxSize * 2) - 1);
            lcd.fillRect(xPos + (4 * boxSize) + (15 * boxSize) + 1, yPos + (8 * boxSize) + 1, (boxSize * 9) - 1, (boxSize * 2) - 1);
            for (uint8_t i = 0; i < 66; i++) {
                // Draw fine 2-pixel alternating stripes
                lcd.setForeColor(WHITE);
                lcd.fillRect(xPos + (4 * boxSize) + (i * 4), yPos + (8 * boxSize) + 1, 2, (boxSize * 2) - 1);
                lcd.setForeColor(BLACK);
                lcd.fillRect(xPos + (4 * boxSize) + (i * 4) + 2, yPos + (8 * boxSize) + 1, 2, (boxSize * 2) - 1);
            }
            for (uint8_t i = 0; i < 5; i++) {
                lcd.setForeColor(WHITE);
                lcd.fillRect(xPos + (4 * boxSize) + (15 * boxSize) + (i * (boxSize * 2)), yPos + (8 * boxSize) + 1, boxSize, (boxSize * 2) - 1);
            }

            // draw center text
            lcd.setAlignment(CENTER);
            lcd.setForeColor(WHITE);
            lcd.setFont(FONT_05x07);
            lcd.drawText("RANDOMWAVES", 11, 479, 269);
            lcd.setForeColor(WHITE);
            lcd.fillRect(xPos + (4 * boxSize) + 1, yPos + (10 * boxSize) + 1, (boxSize * 24) - 1, (boxSize * 2) - 1);

            // Draw individual RGB channel gradients to test component output separately
            RGB16Color redArray[] = {0xFD34, 0xFCB2, 0xFC30, 0xFBCF, 0xFB4D, 0xFACB, 0xFA49, 0xF9E7, 0xF965, 0xF8E3, 0xF861, 0xF800, 0xF800, 0xE000, 0xD000, 0xC800, 0xB800, 0xA800, 0x9800, 0x8800, 0x7800, 0x6800, 0x5800, 0x4000};
            RGB16Color greenArray[] = {0xAFF5, 0x97F2, 0x87F0, 0x7FEF, 0x6FED, 0x5FEB, 0x4FE9, 0x3FE7, 0x2FE5, 0x1FE3, 0x0FE1, 0x07E0, 0x07E0, 0x0780, 0x0700, 0x0680, 0x0600, 0x05A0, 0x0520, 0x04A0, 0x0420, 0x03C0, 0x0340, 0x02C0};
            RGB16Color blueArray[] = {0xA53F, 0x94BF, 0x843F, 0x7BDF, 0x6B5F, 0x5ADF, 0x4A5F, 0x39FF, 0x297F, 0x18FF, 0x087F, 0x001F, 0x001F, 0x001E, 0x001C, 0x001A, 0x0018, 0x0016, 0x0014, 0x0012, 0x0010, 0x000F, 0x000D, 0x000B};
            for (uint8_t i = 0; i < 24; i++) {
                lcd.setForeColor(redArray[i]);
                lcd.fillRect(xPos + (4 * boxSize) + (i * boxSize) + 1, yPos + (12 * boxSize) + 1, (boxSize * 1) - 1, (boxSize * 1) - 1);
                lcd.setForeColor(greenArray[i]);
                lcd.fillRect(xPos + (4 * boxSize) + (i * boxSize) + 1, yPos + (13 * boxSize) + 1, (boxSize * 1) - 1, (boxSize * 1) - 1);
                lcd.setForeColor(blueArray[i]);
                lcd.fillRect(xPos + (4 * boxSize) + (i * boxSize) + 1, yPos + (14 * boxSize) + 1, (boxSize * 1) - 1, (boxSize * 1) - 1);
            }

            // Add numeric corner calibration labels (01-04) for corner reference points
            lcd.setFont(FONT_10x14);
            lcd.setAlignment(CENTER);
            lcd.setForeColor(GRAY_75);
            lcd.drawText("01", 2, xPos + 46, yPos + 37);
            lcd.drawText("02", 2, xPos + 887, yPos + 37);
            lcd.drawText("03", 2, xPos + 46, yPos + 472);
            lcd.drawText("04", 2, xPos + 887, yPos + 472);
        } break;

        case TEST_KEY: {
            // Display keyboard and encoder test layout with all position indicators
            lcd.clearScreen();
            lcd.setBackColor(BLACK);
            lcd.setForeColor(GRAY_50);

            // Draw outer boundary frame and center reference box
            lcd.drawRect(68, 40, 825, 460);
            lcd.drawRect(313, 145, (7 * 50) + 35 - 50, 190 + 30 - 50);
            lcd.setForeColor(GRAY_75);

            // Draw all 16 left keyboard key positions
            for (uint8_t i = 0; i < 16; i++) {
                lcd.drawRect(kTestKeyLeftPos[i][0], kTestKeyLeftPos[i][1], kTestKeySize, kTestKeySize);
            }

            // Draw all 16 right keyboard key positions
            for (uint8_t i = 0; i < 16; i++) {
                lcd.drawRect(kTestKeyRightPos[i][0], kTestKeyRightPos[i][1], kTestKeySize, kTestKeySize);
            }

            // Draw all 10 layer selector key positions
            for (uint8_t i = 0; i < 10; i++) {
                lcd.drawRect(kTestKeyLayerPos[i][0], kTestKeyLayerPos[i][1], kTestKeySize, kTestKeySize);
            }

            // Draw all 8 encoder wheel positions
            for (uint8_t i = 0; i < 8; i++) {
                lcd.drawCircle(kTestEncoderPos[i][0], kTestEncoderPos[i][1], kTestEncoderSize);
            }
        } break;

        case TEST_AUDIO: {
            // Display audio filter signal path with level indicators
            uint16_t xPos;
            uint16_t yPos;
            lcd.setFont(FONT_05x07);
            lcd.setAlignment(CENTER);

            // Draw keyboard layout as background reference
            lcd.setForeColor(GRAY_50);
            for (uint8_t i = 0; i < 16; i++) {
                lcd.drawRect(kTestKeyLeftPos[i][0], kTestKeyLeftPos[i][1], kTestKeySize, kTestKeySize);
            }
            for (uint8_t i = 0; i < 16; i++) {
                lcd.drawRect(kTestKeyRightPos[i][0], kTestKeyRightPos[i][1], kTestKeySize, kTestKeySize);
            }
            for (uint8_t i = 0; i < 10; i++) {
                lcd.drawRect(kTestKeyLayerPos[i][0], kTestKeyLayerPos[i][1], kTestKeySize, kTestKeySize);
            }
            for (uint8_t i = 0; i < 8; i++) {
                lcd.drawCircle(kTestEncoderPos[i][0], kTestEncoderPos[i][1], kTestEncoderSize);
            }

            // Clear region for audio signal indicators
            lcd.clearRect(kTestKeyLeftPos[0][0], kTestKeyLeftPos[0][1], 160, 50);

            // Draw input reference signal (inactive gray)
            lcd.setForeColor(GRAY_50);
            xPos = kTestKeyLeftPos[0][0] + 25;
            yPos = kTestKeyLeftPos[0][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Draw low-pass filter output (active cyan)
            lcd.setForeColor(CYAN);
            xPos = kTestKeyLeftPos[1][0] + 25;
            yPos = kTestKeyLeftPos[1][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Draw high-pass filter output (active green)
            lcd.setForeColor(GREEN);
            xPos = kTestKeyLeftPos[2][0] + 25;
            yPos = kTestKeyLeftPos[2][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Draw bypass/passthrough path (inactive gray)
            lcd.setForeColor(GRAY_50);
            xPos = kTestKeyLeftPos[2][0] + 55 + 25;
            yPos = kTestKeyLeftPos[2][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Add filter mode text labels
            lcd.setForeColor(CYAN);
            lcd.drawText("LO", 2, kTestKeyLeftPos[1][0] + 27, kTestKeyLeftPos[1][1] + 40);
            lcd.setForeColor(GREEN);
            lcd.drawText("HP", 2, kTestKeyLeftPos[2][0] + 27, kTestKeyLeftPos[2][1] + 40);
        } break;

        case TEST_SYNC: {
            // Display sync timing output and master clock status
            uint16_t xPos;
            uint16_t yPos;
            lcd.setFont(FONT_05x07);
            lcd.setAlignment(CENTER);

            // Clear previous left sync indicator region
            lcd.clearRect(kTestKeyLeftPos[0][0], kTestKeyLeftPos[0][1], 205, 50);

            // Draw keyboard layout as background reference
            lcd.setForeColor(GRAY_50);
            for (uint8_t i = 0; i < 16; i++) {
                lcd.drawRect(kTestKeyLeftPos[i][0], kTestKeyLeftPos[i][1], kTestKeySize, kTestKeySize);
            }
            for (uint8_t i = 0; i < 16; i++) {
                lcd.drawRect(kTestKeyRightPos[i][0], kTestKeyRightPos[i][1], kTestKeySize, kTestKeySize);
            }
            for (uint8_t i = 0; i < 10; i++) {
                lcd.drawRect(kTestKeyLayerPos[i][0], kTestKeyLayerPos[i][1], kTestKeySize, kTestKeySize);
            }
            for (uint8_t i = 0; i < 8; i++) {
                lcd.drawCircle(kTestEncoderPos[i][0], kTestEncoderPos[i][1], kTestEncoderSize);
            }

            // Clear region for right-side sync indicators
            lcd.clearRect(kTestKeyRightPos[0][0], kTestKeyRightPos[0][1], 160, 50);

            // Draw incoming tempo sync reference (inactive gray)
            lcd.setForeColor(GRAY_50);
            xPos = kTestKeyRightPos[0][0] + 25 - 55;
            yPos = kTestKeyRightPos[0][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Draw sync output pulse (active magenta)
            lcd.setForeColor(MAGENTA);
            xPos = kTestKeyRightPos[0][0] + 25;
            yPos = kTestKeyRightPos[0][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Draw quantize clock reference (inactive gray)
            lcd.setForeColor(GRAY_50);
            xPos = kTestKeyRightPos[1][0] + 25;
            yPos = kTestKeyRightPos[1][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Draw master output clock (active green)
            lcd.setForeColor(GREEN);
            xPos = kTestKeyRightPos[2][0] + 25;
            yPos = kTestKeyRightPos[2][1];
            lcd.fillTriangle(xPos, yPos, xPos - 6, yPos + 10, xPos + 6, yPos + 10);
            lcd.drawVLine(xPos, yPos, 30);
            lcd.drawHLine(xPos - 6, yPos + 30, 12);

            // Add sync timing mode labels
            lcd.setForeColor(MAGENTA);
            lcd.drawText("SO", 2, kTestKeyRightPos[0][0] + 27, kTestKeyRightPos[0][1] + 40);
            lcd.setForeColor(GREEN);
            lcd.drawText("MO", 2, kTestKeyRightPos[2][0] + 27, kTestKeyRightPos[2][1] + 40);
        } break;

        case TEST_END:

            // Clean up test display and restore final state
            lcd.setForeColor(GRAY_50);

            // Clear all sync indicators from display
            lcd.clearRect(kTestKeyRightPos[0][0] - 55, kTestKeyRightPos[0][1], 205, 50);

            // Restore right keyboard key borders
            for (uint8_t i = 0; i < 3; i++) {
                lcd.drawRect(kTestKeyRightPos[i][0], kTestKeyRightPos[i][1], kTestKeySize, kTestKeySize);
            }
            break;
    }
}

void Controller::lcd_update() {
    /// @brief Updates dynamic LCD overlays and runtime visual indicators.

    // 1. PLAYBACK OVERLAY UPDATE
    // Refresh playhead/progress bar visuals.
    lcd_drawPlay();

    // 2. TRANSPORT ICON UPDATE
    // Refresh reset/play/stop/record icons.
    lcd_drawIcon();

    // 3. COPY/PASTE STATUS UPDATE
    // Refresh transient copy/paste text feedback.
    lcd_drawCopyPaste();

    // 4. LIMIT ALERT UPDATE
    // Refresh on-screen limit warning state.
    lcd_drawLimitAlert();

    // 5. SONG BANK SHIFT UPDATE
    // Refresh temporary bank shift overlay.
    lcd_drawSongBankShift();

    // 6. PRECOUNT UPDATE
    // Refresh count-down indicator during precount.
    lcd_drawCountDown();

    // 7. TRANSITION EFFECT UPDATE
    // Refresh UI transition animations/flags.
    lcd_drawTransition();
}

void Controller::lcd_drawLogo(bool animate_) {
    /// @brief Draws startup logo with optional fade-in animation.

    // 1. ASSET AND COLOR SETUP
    // Resolve logo image pointers and configure draw colors.
    const RGB16Color* indexPtr = (const RGB16Color*)(RAM_IMAGE_LOGO_PALETTE_ADDRESS);
    const uint8_t* dataPtr = (const uint8_t*)(RAM_IMAGE_LOGO_DATA_ADDRESS);
    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);

    // 2. LOGO RENDER
    // Draw directly or use fade animation depending on mode.
    if (animate_) {
        lcd.fadeRGB16Image(indexPtr, dataPtr, kImageLogoPalette, kImageLogoX, kImageLogoY, kImageLogoWidth, kImageLogoHeight, true, 50, 5);
    } else {
        lcd.drawRGB16Image(indexPtr, dataPtr, kImageLogoPalette, kImageLogoX, kImageLogoY, kImageLogoWidth, kImageLogoHeight);
    }
}

void Controller::lcd_restoreLogoRows(int16_t firstRow_, int16_t lastRow_) {
    /// @brief Redraws rows firstRow_..lastRow_ (inclusive) of the boot logo from SDRAM.
    /// The logo is stored row-major, so a row range is a contiguous slice of the pixel data.

    if (firstRow_ < 0) firstRow_ = 0;
    if (lastRow_ >= static_cast<int16_t>(kImageLogoHeight)) lastRow_ = kImageLogoHeight - 1;
    if (lastRow_ < firstRow_) return;

    const RGB16Color* indexPtr = (const RGB16Color*)(RAM_IMAGE_LOGO_PALETTE_ADDRESS);
    const uint8_t* dataPtr = (const uint8_t*)(RAM_IMAGE_LOGO_DATA_ADDRESS) + (firstRow_ * kImageLogoWidth);
    lcd.drawRGB16Image(indexPtr, dataPtr, kImageLogoPalette, kImageLogoX, kImageLogoY + firstRow_, kImageLogoWidth, (lastRow_ - firstRow_) + 1);
}

void Controller::lcd_drawEditionShades(int16_t offset_) {
    /// @brief Draws the edition pixel sunglasses over the logo's eyes, shifted vertically by
    /// offset_ logo rows (0 = resting on the eyes). Only fillRect, in logo coordinates.

    const int16_t x = kImageLogoX;
    const int16_t y = kImageLogoY + offset_;

    // 1. FRAME: top bar with an accent rim.
    lcd.setForeColor(BLACK);
    lcd.fillRect(x + 30, y + 96, 112, 6);
    lcd.setForeColor(kEditionAccentColor);
    lcd.fillRect(x + 30, y + 96, 112, 2);

    // 2. LENSES: stepped bottoms (pixel-art shades) with two white glints each.
    const int16_t lensX[2] = {34, 101};
    for (uint8_t i = 0; i < 2; i++) {
        const int16_t lx = x + lensX[i];
        lcd.setForeColor(BLACK);
        lcd.fillRect(lx, y + 102, 37, 12);
        lcd.fillRect(lx + 3, y + 114, 31, 4);
        lcd.fillRect(lx + 7, y + 118, 23, 3);
        lcd.setForeColor(WHITE);
        lcd.fillRect(lx + 5, y + 104, 4, 4);
        lcd.fillRect(lx + 9, y + 108, 4, 4);
    }
}

void Controller::lcd_drawEdition(bool animate_) {
    /// @brief Draws the edition branding on the boot screen: sunglasses on the logo, the edition
    /// name in large accent letters and the "EDITION" label. Call right after lcd_drawLogo().
    /// Animated: the shades slide down onto the eyes, the name appears letter by letter and the
    /// label fades in (about 0.95 s in total, inside the existing post-logo pause).

    // 1. SUNGLASSES
    if (animate_) {
        const uint8_t frames = sizeof(kEditionShadesDrop) / sizeof(kEditionShadesDrop[0]);
        int16_t previous = 0;
        for (uint8_t f = 0; f < frames; f++) {
            const int16_t offset = kEditionShadesDrop[f];
            // Restore the logo where the previous frame drew, then draw the new frame.
            if (f > 0) lcd_restoreLogoRows(kEditionShadesTop + previous, kEditionShadesBottom + previous);
            lcd_drawEditionShades(offset);
            previous = offset;
            HAL_Delay(kEditionShadesFrameMs);
        }
    } else {
        lcd_drawEditionShades(0);
    }

    // 2. NAME: black letters on a yellow block between two hazard-stripe bands (v1.14
    // "Bumblebee"), the letters one by one when animated.
    lcd_drawHazardStripes(kEditionStripeTopY);
    lcd_drawHazardStripes(kEditionStripeBottomY);
    lcd.setFont(FONT_14x18);
    lcd.setSpacing(kEditionNameSpacing);
    const uint16_t nameLength = strlen(kEditionName);
    {
        const uint16_t width = lcd.getTextScaledWidth(nameLength, kEditionNameScale);
        lcd.setForeColor(kEditionAccentColor);
        lcd.fillRect(kEditionCenterX - (width / 2) - kEditionBlockPad, kEditionNameY - kEditionBlockPad,
                     width + (2 * kEditionBlockPad), (kFontHeight_14x18 * kEditionNameScale) + (2 * kEditionBlockPad));
    }
    lcd.setForeColor(BLACK);
    lcd.setBackColor(BLACK);
    if (animate_) {
        lcd.setAlignment(LEFT);
        const uint16_t width = lcd.getTextScaledWidth(nameLength, kEditionNameScale);
        const uint16_t cell = (kFontWidth_14x18 + kEditionNameSpacing) * kEditionNameScale;
        const uint16_t xStart = kEditionCenterX - (width / 2);
        for (uint16_t i = 0; i < nameLength; i++) {
            lcd.drawTextScaled(&kEditionName[i], 1, xStart + (i * cell), kEditionNameY, kEditionNameScale);
            HAL_Delay(kEditionLetterMs);
        }
    } else {
        lcd.setAlignment(CENTER);
        lcd.drawTextScaled(kEditionName, nameLength, kEditionCenterX, kEditionNameY, kEditionNameScale);
    }

    // 3. LABEL: small, wide-spaced, fading in through three grey levels.
    lcd.setFont(FONT_14x18);
    lcd.setSpacing(kEditionLabelSpacing);
    lcd.setAlignment(CENTER);
    const uint16_t labelLength = strlen(kEditionLabel);
    if (animate_) {
        const RGB16Color steps[3] = {GRAY_50, GRAY_75, WHITE};
        for (uint8_t s = 0; s < 3; s++) {
            lcd.setForeColor(steps[s]);
            lcd.drawText(kEditionLabel, labelLength, kEditionCenterX, kEditionLabelY);
            HAL_Delay(kEditionLabelFadeMs);
        }
    } else {
        lcd.setForeColor(WHITE);
        lcd.drawText(kEditionLabel, labelLength, kEditionCenterX, kEditionLabelY);
    }

    // 4. RESTORE DEFAULT FONT STATE (setFont resets spacing).
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(LEFT);
}

void Controller::lcd_drawHazardStripes(uint16_t y_) {
    /// @brief A full-width band of diagonal yellow/black hazard stripes (boot screen, v1.14).
    /// Drawn row by row as short horizontal runs, so it costs a few hundred fills.

    lcd.setForeColor(kEditionAccentColor);
    for (uint16_t row = 0; row < kEditionStripeHeight; row++) {
        const int16_t shift = static_cast<int16_t>(kEditionStripeHeight - row);  // leaning right: "/"
        for (int16_t x = static_cast<int16_t>(shift - (2 * kEditionStripeWidth)); x < static_cast<int16_t>(lcd.getWidth()); x += 2 * kEditionStripeWidth) {
            int16_t x0 = x;
            int16_t w = kEditionStripeWidth;
            if (x0 < 0) {
                w = static_cast<int16_t>(w + x0);
                x0 = 0;
            }
            if (w > 0) lcd.drawHLine(static_cast<uint16_t>(x0), static_cast<uint16_t>(y_ + row), w);
        }
    }
}

void Controller::lcd_clearEdition() {
    /// @brief Clears the edition name block, label and stripes (the shades go with the logo
    /// fade-out).

    lcd.setBackColor(BLACK);
    const uint16_t top = kEditionNameY - kEditionBlockPad;
    const uint16_t bottom = kEditionLabelY + kFontHeight_14x18;
    lcd.clearRect(kEditionCenterX - 240, top, 480, bottom - top);
    lcd.clearRect(0, kEditionStripeTopY, lcd.getWidth(), kEditionStripeHeight);
    lcd.clearRect(0, kEditionStripeBottomY, lcd.getWidth(), kEditionStripeHeight);
}

void Controller::lcd_clearLogo(bool animate_) {
    /// @brief Clears startup logo with optional fade-out animation.

    // 1. ASSET AND COLOR SETUP
    // Resolve logo image pointers and configure clear colors.
    const RGB16Color* indexPtr = (const RGB16Color*)(RAM_IMAGE_LOGO_PALETTE_ADDRESS);
    const uint8_t* dataPtr = (const uint8_t*)(RAM_IMAGE_LOGO_DATA_ADDRESS);
    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);

    // 2. LOGO CLEAR
    // Clear directly or fade out depending on mode.
    if (animate_) {
        lcd.fadeRGB16Image(indexPtr, dataPtr, kImageLogoPalette, kImageLogoX, kImageLogoY, kImageLogoWidth, kImageLogoHeight, false, 50, 5);
    } else {
        lcd.clearRect(kImageLogoX, kImageLogoY, kImageLogoWidth, kImageLogoHeight);
    }
}

void Controller::lcd_drawPage(bool animate_) {
    /// @brief Draws main page layout including menu panels, layer rows, and labels.

    // 1. RENDER CONFIGURATION
    // Prepare shared asset pointers and animation timing.
    const RGB16Color* indexPtr;
    const uint8_t* dataPtr;
    uint8_t step = 20;
    uint8_t delay = 5;

    // 2. TOP PANEL RENDER
    // Draw top menu panel with optional fade animation.
    indexPtr = (const RGB16Color*)(RAM_IMAGE_MENU_A_PALETTE_ADDRESS);
    dataPtr = (const uint8_t*)(RAM_IMAGE_MENU_A_DATA_ADDRESS);
    if (animate_) {
        lcd.fadeRGB16Image(indexPtr, dataPtr, 64, kImageMenuAX, kImageMenuAY, kImageMenuAWidth, kImageMenuAHeight, true, step, delay);
    } else {
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kImageMenuAX, kImageMenuAY, kImageMenuAWidth, kImageMenuAHeight);
    }
    // The panel image (on the SD card) has the play bar in the factory cyan: repaint it in the
    // Moffe Edition accent, so an old SD card needs no new images.
    lcd.setForeColor(kPlayColor1);
    lcd.drawHLine(kPlayX, kPlayY, kPlayWidth);

    // 3. LAYER PANEL RENDER
    // Draw all layer rows by alternating A/B panel styles.
    step = 30;
    delay = 5;

    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        if ((i % 2) == 0) {
            // Draw layer A style (even-numbered layers)
            indexPtr = (const RGB16Color*)(RAM_IMAGE_LAYER_A_PALETTE_ADDRESS);
            dataPtr = (const uint8_t*)(RAM_IMAGE_LAYER_A_DATA_ADDRESS);
        } else {
            // Draw layer B style (odd-numbered layers)
            indexPtr = (const RGB16Color*)(RAM_IMAGE_LAYER_B_PALETTE_ADDRESS);
            dataPtr = (const uint8_t*)(RAM_IMAGE_LAYER_B_DATA_ADDRESS);
        }

        if (animate_) {
            lcd.fadeRGB16Image(indexPtr, dataPtr, 64, kImageLayerX, kImageLayerY[i], kImageLayerWidth, kImageLayerHeight, true, step, delay);
        } else {
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kImageLayerX, kImageLayerY[i], kImageLayerWidth, kImageLayerHeight);
        }
    }

    // 4. BOTTOM PANEL RENDER
    // Draw bottom menu panel with optional fade animation.
    step = 20;
    delay = 5;

    indexPtr = (const RGB16Color*)(RAM_IMAGE_MENU_B_PALETTE_ADDRESS);
    dataPtr = (const uint8_t*)(RAM_IMAGE_MENU_B_DATA_ADDRESS);

    if (animate_) {
        lcd.fadeRGB16Image(indexPtr, dataPtr, 64, kImageMenuBX, kImageMenuBY, kImageMenuBWidth, kImageMenuBHeight, true, step, delay);
    } else {
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kImageMenuBX, kImageMenuBY, kImageMenuBWidth, kImageMenuBHeight);
    }

    // 5. LAYER LABEL RENDER
    // Draw two-digit layer labels with alternating foreground color.
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(LEFT);

    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        // Use gray for even layers, white for odd layers
        (i % 2) ? lcd.setForeColor(GRAY_75) : lcd.setForeColor(WHITE);
        char num[3];
        sprintf(num, "%02d", (i + 1));
        lcd.drawText(num, 2, kLayerTextNumberX, kLayerTextY[i]);
    }

    // 6. FOOTER LABELS: the panel image has them in the factory magenta; the Moffe Edition
    // draws its own black-on-yellow chips over them.
    lcd_drawEncoderBankTab();
    lcd_drawSongBankTab(activeSongBank, false);
}

/* Lcd Alert Functions -------------------------------------------------------*/

void Controller::lcd_drawAlert() {
    /// @brief Draws alert notification box and message based on current alert type.

    // 1. STYLE SETUP
    // Configure text alignment, font, and colors for alert rendering.
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kAccentColor);
    lcd.setBackColor(BLACK);

    // 2. FRAME DRAW
    // Clear alert area and draw border frame.
    lcd.clearRect(355, 260, 250, 20);
    lcd.drawRect(355, 260, 250, 20);

    // 3. ALERT TEXT RESOLUTION
    // Select alert text constant according to active alert type.
    const char* alertText;
    switch (alertType) {
        case ALERT_MEASUREUP:

        case ALERT_MEASUREDOWN:

        case ALERT_BARUP:

        case ALERT_BARDOWN:

        case ALERT_QUANTIZEUP:

        case ALERT_QUANTIZEDOWN:
            alertText = kAlertTextResetPlay;
            break;

        case ALERT_NEWFILE:
            alertText = kAlertTextNewFile;
            break;

        case ALERT_LOADFILE:
            alertText = kAlertTextLoadFile;
            break;

        case ALERT_SAVEFILE:
            alertText = kAlertTextSaveFile;
            break;

        case ALERT_CLEARFILE:
            alertText = kAlertTextClearFile;
            break;

        case ALERT_OVERWRITEFILE:
            alertText = kAlertTextOverwriteFile;
            break;

        case ALERT_NEWDRUMKIT:
            alertText = kAlertTextNewDrumkit;
            break;

        case ALERT_LOADDRUMKIT:
            alertText = kAlertTextLoadDrumkit;
            break;

        case ALERT_SAVEDRUMKIT:
            alertText = kAlertTextSaveDrumkit;
            break;

        case ALERT_CLEARDRUMKIT:
            alertText = kAlertTextClearDrumkit;
            break;

        case ALERT_OVERWRITEDRUMKIT:
            alertText = kAlertTextOverwriteDrumkit;
            break;

        case ALERT_LOADSUCCESS:
            alertText = kAlertTextLoadSuccess;
            break;

        case ALERT_LOADERROR:
            alertText = kAlertTextLoadError;
            break;

        case ALERT_SAVESUCCESS:
            alertText = kAlertTextSaveSuccess;
            break;

        case ALERT_SAVEERROR:
            alertText = kAlertTextSaveError;
            break;

        case ALERT_CLEARSUCCESS:
            alertText = kAlertTextClearSuccess;
            break;

        case ALERT_CLEARERROR:
            alertText = kAlertTextClearError;
            break;

        case ALERT_MISSINGSAMPLE:
            alertText = kAlertTextMissingSample;
            break;

        case ALERT_MISSINGSAMPLES:
            alertText = kAlertTextMissingSamples;
            break;
    }
    // 4. ALERT TEXT DRAW
    // Render resolved message at alert center.
    lcd.drawText(alertText, strlen(alertText), 480, 265);
}

void Controller::lcd_clearAlert() {
    /// @brief Clears alert box and restores underlying song lane content.

    // 1. ALERT REGION CLEAR
    // Clear alert area with default colors.
    lcd.setForeColor(kAccentColor);
    lcd.setBackColor(BLACK);
    lcd.clearRect(355, 260, 250, 20);

    // 2. CONTENT RESTORE
    // Redraw song lane that can be overlapped by alert box.
    lcd_drawSong(4, activeSongBank);
}

/* Lcd Limit Alert Functions -------------------------------------------------*/

void Controller::lcd_drawLimitAlert() {
    /// @brief Draws or clears transient limit indicator based on alert flags.

    // 1. SHOW FLAG HANDLING
    // Start timer and draw indicator when show flag is armed.
    if (limitAlertShowFlag) {
        limitAlertShowFlag = false;
        stopLimitAlertTimer();
        startLimitAlertTimer();

        // Display alert symbol
        if (!limitAlertActive) {
            limitAlertActive = true;
            lcd.setBackColor(BLACK);
            lcd.setFont(FONT_07x09);
            lcd.setAlignment(CENTER);
            lcd.setForeColor(kAccent2Color);
            lcd.drawText("!", 1, kLimitAlertX, kLimitAlertY);
        }

        // 2. CLEAR FLAG HANDLING
        // Clear indicator when clear flag is armed.
    } else if (limitAlertClearFlag) {
        limitAlertClearFlag = false;
        limitAlertActive = false;

        // Clear alert symbol
        lcd.setFont(FONT_07x09);
        lcd.setAlignment(CENTER);
        lcd.setForeColor(kAccent2Color);
        lcd.setBackColor(BLACK);
        lcd.drawText(" ", 1, kLimitAlertX, kLimitAlertY);
    }
}

/* Lcd Sd Functions ----------------------------------------------------------*/

void Controller::lcd_drawSdDataIntro() {
    /// @brief Draws SD-card analysis progress message in footer area.

    // 1. STYLE SETUP
    // Configure footer text style for intro message.
    lcd.setForeColor(GRAY_75);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(CENTER);

    // 2. INTRO MESSAGE DRAW
    // Render static SD analyze text.
    lcd.drawText("[ ANALYZING SDCARD ]", 20, 480, kSdDataY);
}

void Controller::lcd_drawSdData() {
    /// @brief Draws SD-card library statistics footer text.

    // 1. STYLE SETUP
    // Configure footer text style for statistics string.
    lcd.setForeColor(GRAY_75);
    lcd.setBackColor(BLACK);
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_05x07);

    // 2. TEXT BUFFER PREP
    // Prepare static labels and output buffer.
    char tab[] = "  |  ";
    char textFile[] = " FILES";
    char textDrumkit[] = " DRUMKITS";
    char textInst[] = " INSTS";
    char textSample[] = " SAMPLES";
    char sdText[90] = {};

    // 3. FILE COUNT APPEND
    // Append file count or placeholder when SD is unavailable.
    if (sd.ready) {
        char numFile[4];
        sprintf(numFile, "%03d", fileLibrarySize);
        strcat(sdText, numFile);
    } else {
        strcat(sdText, "---");
    }
    strcat(sdText, textFile);
    strcat(sdText, tab);

    // 4. DRUMKIT COUNT APPEND
    // Append drumkit count or placeholder when SD is unavailable.
    if (sd.ready) {
        char numDrumkit[4];
        sprintf(numDrumkit, "%03d", drumkitLibrarySize);
        strcat(sdText, numDrumkit);
    } else {
        strcat(sdText, "---");
    }
    strcat(sdText, textDrumkit);
    strcat(sdText, tab);

    // 5. INSTRUMENT COUNT APPEND
    // Append instrument count or placeholder when SD is unavailable.
    if (sd.ready) {
        char numInst[4];
        sprintf(numInst, "%04d", instLibrarySize);
        strcat(sdText, numInst);
    } else {
        strcat(sdText, "----");
    }
    strcat(sdText, textInst);
    strcat(sdText, "  |  ");

    // 6. SAMPLE COUNT APPEND
    // Append sample count (or compact K format) or placeholder.
    if (sd.ready) {
        char numSample[4];
        if (totalSampleLibrarySize < 10000) {
            sprintf(numSample, "%04d", totalSampleLibrarySize);
            strcat(sdText, numSample);
        } else {
            sprintf(numSample, "%04d", totalSampleLibrarySize / 1000);
            strcat(sdText, numSample);
            strcat(sdText, "K");
        }
    } else {
        strcat(sdText, "----");
    }
    strcat(sdText, textSample);

    // 7. FOOTER DRAW
    // Render composed statistics string.
    lcd.drawText(sdText, strlen(sdText), 480, kSdDataY);
}

void Controller::lcd_clearSdData() {
    /// @brief Clears SD-card statistics footer area.

    // 1. FOOTER CLEAR
    // Clear footer rectangle where SD statistics are shown.
    lcd.setBackColor(BLACK);
    lcd.clearRect(130, kSdDataY, 700, 7);
}

void Controller::lcd_drawSdAlert(SdResult result_) {
    /// @brief Draws SD-card alert message based on SD operation result.

    // 1. ALERT FRAME SETUP
    // Configure style and draw SD alert frame.
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kAccentColor);
    lcd.setBackColor(BLACK);
    lcd.clearRect(355, 260, 250, 20);
    lcd.drawRect(355, 260, 250, 20);

    // 2. ALERT TEXT RESOLUTION
    // Map SD result code to corresponding alert message.
    const char* alertText;
    switch (result_) {
        case SD_ERROR_DETECT:
            alertText = kSdAlertTextInsert;
            break;

        case SD_ERROR_MOUNT:
            alertText = kSdAlertTextFormat;
            break;

        case SD_ERROR_SERIAL:
            alertText = kSdAlertTextSerial;
            break;

        case SD_ERROR_SYSTEMFOLDER:
            alertText = kSdAlertTextSystemFolder;
            break;

        case SD_ERROR_SAMPLEFOLDER:
            alertText = kSdAlertTextSampleFolder;
            break;

        case SD_ERROR_FILEFOLDER:
            alertText = kSdAlertTextFileFolder;
            break;

        case SD_ERROR_DRUMKITFOLDER:
            alertText = kSdAlertTextDrumkitFolder;
            break;

        case SD_ERROR_SOUNDFOLDER:
            alertText = kSdAlertTextSoundFolder;
            break;

        case SD_ERROR_IMAGEFOLDER:
            alertText = kSdAlertTextImageFolder;
            break;

        case SD_ERROR_FIRMWAREFOLDER:
            alertText = kSdAlertTextFirmwareFolder;
            break;

        case SD_ERROR_PRESETFOLDER:
            alertText = kSdAlertTextPresetFolder;
            break;

        case SD_ERROR_SYSTEMFILE:
            alertText = kSdAlertTextSystemFile;
            break;

        case SD_ANALYZE:
            alertText = kSdAlertTextAnalyze;
            break;
    }
    // 3. ALERT TEXT DRAW
    // Render resolved SD alert text.
    lcd.drawText(alertText, strlen(alertText), 480, 265);
}

void Controller::lcd_clearSdAlert() {
    /// @brief Clears SD alert box and restores underlying song lane content.

    // 1. ALERT REGION CLEAR
    // Clear SD alert region with default colors.
    lcd.setForeColor(kAccentColor);
    lcd.setBackColor(BLACK);
    lcd.clearRect(355, 260, 250, 20);

    // 2. CONTENT RESTORE
    // Redraw song lane hidden by SD alert box.
    lcd_drawSong(4, activeSongBank);
}

void Controller::lcd_drawInitSdAlert(SdResult result_) {
    /// @brief Draws startup SD alert during initialization phase.

    // 1. INIT ALERT DRAW
    // Forward initialization result to LCD alert renderer.
    lcd.drawInitSdAlert(result_);
}

void Controller::lcd_clearInitSdAlert() {
    /// @brief Clears startup SD alert region after initialization.

    // 1. INIT ALERT CLEAR
    // Clear initialization alert region from LCD.
    lcd.clearInitSdAlert();
}

/* Lcd Bank Functions --------------------------------------------------------*/

void Controller::lcd_drawLabelChip(const char* text_, uint16_t centerX_, uint16_t y_) {
    /// @brief A footer label as a black-on-yellow chip (v1.14 Moffe Edition "Bumblebee" look).
    /// Centred at centerX_, text top at y_ (FONT_07x09). Leaves the background colour black.

    const uint16_t length = static_cast<uint16_t>(strlen(text_));
    const uint16_t width = static_cast<uint16_t>((length * (kFontWidth_07x09 + 1)) + 8);
    lcd.setForeColor(kAccentColor);
    lcd.fillRect(static_cast<uint16_t>(centerX_ - (width / 2)), static_cast<uint16_t>(y_ - 2), width, kFontHeight_07x09 + 4);
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setBackColor(kAccentColor);
    lcd.setForeColor(BLACK);
    lcd.drawText(text_, length, centerX_, y_);
    lcd.setBackColor(BLACK);
}

void Controller::lcd_drawEncoderBankTab() {
    /// @brief Displays active global encoder bank number indicator.

    // 1. STYLE SETUP
    // Configure footer text style.
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);

    // 2. LABEL DRAW
    // Draw static KNOB footer label: a black-on-yellow chip (Moffe Edition palette).
    lcd_drawLabelChip("KNOB", 76, 463);

    // 3. BANK VALUE DRAW
    // Draw two-digit active encoder bank index.
    lcd.setForeColor(WHITE);
    char textData[3];
    sprintf(textData, "%02d", activeEncoderBank + 1);
    lcd.drawText(textData, 2, 76, 489);
}

void Controller::lcd_drawSongBankTab(uint8_t bank_, bool mode_) {
    /// @brief Displays song bank number with optional highlight color.

    // 1. STYLE SETUP
    // Configure footer text style.
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);

    // 2. LABEL DRAW
    // Draw static SONG footer label: a black-on-yellow chip (Moffe Edition palette).
    lcd_drawLabelChip("SONG", 886, 463);

    // 3. HIGHLIGHT STATE APPLY
    // Select highlighted color when requested (a bank change waiting for the loop end).
    (mode_) ? lcd.setForeColor(kAccentColor) : lcd.setForeColor(WHITE);

    // 4. BANK VALUE DRAW
    // Draw two-digit selected bank index.
    char textData[3];
    sprintf(textData, "%02d", bank_ + 1);
    lcd.drawText(textData, 2, 886, 489);
}

void Controller::lcd_drawSongBankShift() {
    /// @brief Handles deferred song-bank shift redraw when bank action is pending.

    // 1. BANK SHIFT GATE
    // Apply redraw flow only when bank action flag is armed.
    if (bankActionFlag) {
        bankActionFlag = false;

        // 2. BANK TAB REFRESH
        // Refresh bank tab indicator.
        lcd_drawSongBankTab(activeSongBank, false);

        // 3. SONG GRID REFRESH
        // Redraw all layer/bank song lanes.
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            for (uint8_t j = 0; j < kBankLibrarySize; j++) {
                lcd_drawSong(i, j);
            }
        }

        // 4. NOTE PAGE: show the notes of the bank that now plays.
        lcd_redrawNotePage();
    }
}

/* Lcd Menu Functions --------------------------------------------------------*/

void Controller::lcd_drawMenuIcon(Menu menu_) {
    /// @brief Draws left/right menu icon indicators based on active menu type.

    // 1. COMMON STYLE SETUP
    // Configure common icon text style and spacing.
    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_10x14);
    lcd.setAlignment(CENTER);
    lcd.setSpacing(4);

    // 2. LEFT ICON DRAW
    // Draw left icon as grid for global menu or two-letter code otherwise.
    if (menu_ == GLOBAL_MENU) {
        lcd.clearRect(kMenuIconLeftX - 14, kMenuIconLeftY - 2, 25, 20);
        for (uint8_t k = 0; k < 4; k++) {
            lcd.fillRect(kMenuIconLeftX - 6 + (k % 2 * 8), kMenuIconLeftY + 1 + (k / 2 * 8), 4, 4);
        }
    } else {
        const char* iconStr = "";
        switch (menu_) {
            case FILE_MENU:
                iconStr = "FI";
                break;

            case FILTER_0_MENU:

            case FILTER_1_MENU:
                iconStr = "FL";
                break;

            case RHYTHM_MENU:
                iconStr = "RH";
                break;

            case REVERB_MENU:
                iconStr = "RE";
                break;

            case EQ_MENU:
                iconStr = "PE";
                break;

            case EFFECT_0_MENU:

            case EFFECT_1_MENU:
                iconStr = "EF";
                break;

            case IO_MENU:
                iconStr = "SY";
                break;

            case LAYER_SONG_MENU:
                iconStr = "SO";
                break;

            case LAYER_INST_0_MENU:

            case LAYER_INST_1_MENU:

            case LAYER_INST_2_MENU:
                iconStr = "IN";
                break;

            case DRUMKIT_MENU:
                iconStr = "DR";
                break;

            case MIXER_MENU:
                iconStr = "MX";
                break;

            case METRO_MENU:
                iconStr = "ME";
                break;

            case LFO_MENU:
                iconStr = "LF";
                break;

            default:
                iconStr = "  ";
                break;
        }

        if (iconStr[0] != '\0') {
            lcd.drawText(iconStr, 1, kMenuIconLeftX, kMenuIconLeftY);
        }
    }

    // 3. RIGHT ICON DRAW
    // Draw right-side fixed four-block indicator.
    lcd.setSpacing(2);

    lcd.clearRect(kMenuIconRightX - 14, kMenuIconRightY - 2, 25, 20);
    for (uint8_t k = 0; k < 4; k++) {
        lcd.fillRect(kMenuIconRightX - 6 + (k % 2 * 8), kMenuIconRightY + 1 + (k / 2 * 8), 4, 4);
    }
}

void Controller::lcd_drawEncoderBox(uint8_t encoderNum_) {
    /// @brief Draws encoder activity box color based on encoder mode and function.

    // 1. MODE DISPATCH
    // Select color and draw path based on global/local encoder mode.
    RGB16Color color;
    Encoder& enc = encoder[encoderNum_];

    switch (enc.mode) {
        case ENC_MODE_GLOBAL:
            switch (encoder[encoderNum_].function) {
                case ENC_FNC_NONE:
                    color = BLACK;
                    break;

                case ENC_FNC_OFF:
                    color = kEncoderGlobalInactiveColor;
                    break;

                default:
                    color = kEncoderGlobalActiveColor;
                    break;
            }
            lcd.setForeColor(color);
            lcd.fillRect(kMenuEncoderBoxX[encoderNum_], kMenuEncoderBoxY[encoderNum_], kMenuEncoderBoxWidth, kMenuEncoderBoxHeight);
            break;

        case ENC_MODE_LOCAL: {
            switch (encoder[encoderNum_].function) {
                case ENC_FNC_NONE:
                    color = BLACK;
                    break;

                case ENC_FNC_OFF:
                    color = kEncoderLocalInactiveColor;
                    break;

                default:
                    color = kEncoderLocalActiveColor;
                    break;
            }
            lcd.setForeColor(color);
            if ((menu != LFO_MENU) && (menu != LAYER_INST_0_MENU) && (menu != LAYER_INST_1_MENU) && (menu != LAYER_INST_2_MENU) && (menu != LAYER_SONG_MENU)) {
                // Draw activity indicator
                lcd.fillRect(kMenuEncoderBoxX[encoderNum_], kMenuEncoderBoxY[encoderNum_], kMenuEncoderBoxWidth, kMenuEncoderBoxHeight);
            } else if ((encoderNum_ != 2) && (encoderNum_ != 3)) {
                bool active = (encoder[encoderNum_].function != ENC_FNC_NONE) ? true : false;

                // Draw activity indicator
                lcd.fillRect(kMenuEncoderBoxX[encoderNum_], kMenuEncoderBoxY[encoderNum_], kMenuEncoderBoxWidth, kMenuEncoderBoxHeight);
            }
        } break;
    }
}

void Controller::lcd_drawFileBox(uint8_t menuTab_, FileStatus status_) {
    /// @brief Draws file status indicator box using state-based color coding.

    // 1. POSITION CALCULATION
    // Resolve file-box position for selected menu tab.
    uint16_t xPos = kMenuFileBoxX[menuTab_];
    uint16_t yPos = kMenuFileBoxY[menuTab_];

    // 2. STATUS COLOR DRAW
    // Draw file box with color corresponding to file status.
    switch (status_) {
        case FILE_NONE:
            lcd.setForeColor(kAccent2Color);
            lcd.setBackColor(BLACK);
            lcd.clearRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            lcd.fillRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            break;

        case FILE_MISSING:
            lcd.setForeColor(kAccent2Color);
            lcd.setBackColor(BLACK);
            lcd.clearRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            lcd.fillRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            break;

        case FILE_INCOMPATIBLE:
            lcd.setForeColor(kAccent2Color);
            lcd.setBackColor(BLACK);
            lcd.clearRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            lcd.fillRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            break;

        case FILE_INACTIVE:
            lcd.setForeColor(GRAY_75);
            lcd.setBackColor(BLACK);
            lcd.clearRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            lcd.fillRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            break;

        case FILE_ACTIVE:
            lcd.setForeColor(GREEN);
            lcd.setBackColor(BLACK);
            lcd.fillRect(xPos, yPos, kMenuFileBoxWidth, kMenuFileBoxHeight);
            break;
    }
}

void Controller::lcd_clearMenuTab(uint8_t menuTab_) {
    /// @brief Clears full menu-tab region for given tab index.

    // 1. MENU TAB CLEAR
    // Clear menu tab rectangle with background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(kMenuHeaderX[menuTab_], kMenuHeaderY[menuTab_], 150, 50);
}

void Controller::lcd_clearHeader(uint8_t menuTab_) {
    /// @brief Clears menu header box area.

    // 1. HEADER CLEAR
    // Clear header rectangle using background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(kMenuHeaderX[menuTab_], kMenuHeaderY[menuTab_], kMenuHeaderWidth, kMenuHeaderHeight);
}

void Controller::lcd_clearData(uint8_t menuTab_) {
    /// @brief Clears menu data box area.

    // 1. DATA CLEAR
    // Clear data rectangle using background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(kMenuDataX[menuTab_], kMenuDataY[menuTab_], kMenuDataWidth, kMenuDataHeight);
}

void Controller::lcd_clearEncoderBox(uint8_t encoderNum_) {
    /// @brief Clears encoder indicator box area.

    // 1. ENCODER BOX CLEAR
    // Clear encoder box rectangle using background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(kMenuEncoderBoxX[encoderNum_], kMenuEncoderBoxY[encoderNum_], kMenuEncoderBoxWidth, kMenuEncoderBoxHeight);
}

void Controller::lcd_clearFileBox(uint8_t menuTab_) {
    /// @brief Clears file status box area.

    // 1. FILE BOX CLEAR
    // Clear file box rectangle using background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(kMenuFileBoxX[menuTab_], kMenuFileBoxY[menuTab_], kMenuFileBoxWidth, kMenuFileBoxHeight);
}

void Controller::lcd_clearTextBox(uint8_t menuTab_) {
    /// @brief Clears menu text box area.

    // 1. TEXT BOX CLEAR
    // Clear text-box rectangle using background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(kMenuTextBoxX[menuTab_] - kMenuTextBoxWidth, kMenuTextBoxY[menuTab_], kMenuTextBoxWidth, kMenuTextBoxHeight);
}

void Controller::lcd_clearSignBox(uint8_t menuTab_) {
    /// @brief Clears menu sign/symbol box area.

    // 1. SIGN BOX CLEAR
    // Clear sign-box rectangle using background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(kMenuSignBoxX[menuTab_] - 56, kMenuSignBoxY[menuTab_], kMenuSignBoxWidth, kMenuSignBoxHeight);
}

void Controller::lcd_transitionMenu() {
    /// @brief Applies menu transition cleanup and prepares target menu layout.

    // 1. PREVIOUS MENU CLEANUP
    // Clear UI artifacts based on previous menu type.
    switch (preMenu) {
        case GLOBAL_MENU:
            lcd.setBackColor(BLACK);
            for (uint8_t i = 0; i <= kMaxMenu8Tab; i++) {
                lcd.clearRect(kMenuHeaderX[i], kMenuSignBoxY[i], 42, 7);
            }
            break;

        case FILE_MENU:
        case DRUMKIT_MENU:
            for (uint8_t i = 0; i <= kMaxMenu4Tab; i++) {
                lcd_clearFileBox(i);
            }
            break;

        case MIXER_MENU:
            for (uint8_t i = 0; i <= kMaxMenu4Tab; i++) {
                lcd_clearTextBox(i);
            }
            break;

        case RHYTHM_MENU:
            for (uint8_t i = 0; i <= kMaxMenu4Tab; i++) {
                lcd_clearTextBox(i);
            }
            rhythm.measureLock = true;
            rhythm.barLock = true;
            rhythm.quantizeLock = true;
            break;

        case IO_MENU:
            break;

        case EQ_MENU:
            for (uint8_t i = 2; i <= kMaxMenu8Tab; i++) {
                lcd_clearSignBox(i);
                lcd.clearRect(kMenuTextBoxX[i] - 30, kMenuTextBoxY[i], 30, 7);
            }
            break;

        case LFO_MENU:

        case LAYER_SONG_MENU:
            if (menu != LFO_MENU) {
                lcd.setBackColor(BLACK);
                lcd.setForeColor(GRAY_50);
                lcd.clearRect(493, 30, 325, 60);
                lcd.drawVLine(kMenuLineX[3], kMenuLineY[3], kMenuLineHeight);
            }
            preBeatFillType = -1;
            preBeatFillPattern = -1;
            preBeatFillTime = -1;
            preBeatFillLevel = -1;
            break;

        case LAYER_INST_0_MENU:

        case LAYER_INST_1_MENU:

        case LAYER_INST_2_MENU:
            if ((menu != LAYER_INST_0_MENU) && (menu != LAYER_INST_1_MENU) && (menu != LAYER_INST_2_MENU)) {
                lcd.setBackColor(BLACK);
                lcd.setForeColor(GRAY_50);
                lcd.clearRect(493, 30, 325, 60);
                lcd.drawVLine(kMenuLineX[3], kMenuLineY[3], kMenuLineHeight);
                for (uint8_t i = 0; i < 2; i++) {
                    lcd_clearTextBox(i);
                    lcd.clearRect(kMenuSignBoxX[1] - 70, kMenuSignBoxY[1], 70, kMenuSignBoxHeight);
                }
            }
            break;

        default:
            break;
    }

    // 2. TARGET MENU PREP
    // Clear and prepare regions required by new menu type.
    switch (menu) {
        case FILE_MENU:

        case DRUMKIT_MENU:

        case RHYTHM_MENU:

        case METRO_MENU:

        case IO_MENU:
            for (uint8_t i = 4; i <= kMaxMenu8Tab; i++) {
                lcd_clearMenuTab(i);
            }
            break;

        case LFO_MENU:
            if (preMenu != LFO_MENU) {
                lcd.setBackColor(BLACK);
                lcd.clearRect(493, 29, 325, 60);
            }
            break;

        case LAYER_INST_0_MENU:

        case LAYER_INST_1_MENU:

        case LAYER_INST_2_MENU:
            if ((preMenu != LAYER_INST_0_MENU) && (preMenu != LAYER_INST_1_MENU) && (preMenu != LAYER_INST_2_MENU)) {
                lcd.setBackColor(BLACK);
                lcd.clearRect(493, 29, 325, 60);
            }
            break;

        case LAYER_SONG_MENU:
            lcd.setBackColor(BLACK);
            lcd.clearRect(493, 30, 325, 60);
            for (uint8_t i = 6; i < 8; i++) {
                lcd_clearMenuTab(i);
            }
            break;

        default:
            break;
    }
}

void Controller::lcd_transitionSelect() {
    /// @brief Updates menu selection indicator when active tab/menu changes.

    // 1. ICON ASSET SETUP
    // Resolve selection icon palette pointer.
    const RGB16Color* indexPtr;
    const uint8_t* dataPtr;

    indexPtr = (const RGB16Color*)(RAM_ICON_SELECT_PALETTE_ADDRESS);

    // 2. REFERENCE MENU RESOLUTION
    // Resolve reference menu used for clearing old selection icon.
    uint8_t refMenu;
    if (preMenu == menu) {
        refMenu = menu;
    } else {
        // Handle menu change - update preMenu and use previous as reference for clearing
        refMenu = preMenu;
        preMenu = menu;
    }

    // 3. PREVIOUS SELECTION CLEAR
    // Clear previous tab selection indicator according to menu layout.
    switch (refMenu) {
        case LAYER_INST_0_MENU:

        case LAYER_INST_1_MENU:

        case LAYER_INST_2_MENU:
            dataPtr = (const uint8_t*)(RAM_ICON_SELECT_OFF_DATA_ADDRESS);
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kMenuSelectX[preMenuTab], kMenuSelectY[preMenuTab], kIconSelectWidth, kIconSelectHeight);
            break;

        case MIXER_MENU:

        case LFO_MENU:

        case EQ_MENU:

        case FILTER_0_MENU:

        case FILTER_1_MENU:

        case EFFECT_0_MENU:

        case EFFECT_1_MENU:

        case REVERB_MENU:

        case LAYER_SONG_MENU:
            dataPtr = (const uint8_t*)(RAM_ICON_SELECT_OFF_DATA_ADDRESS);
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kMenuSelectX[preMenuTab], kMenuSelectY[preMenuTab], kIconSelectWidth, kIconSelectHeight);
            break;

        case FILE_MENU:

        case DRUMKIT_MENU:

        case IO_MENU:

        case RHYTHM_MENU:

        case METRO_MENU:
            dataPtr = (const uint8_t*)(RAM_ICON_SELECT_OFF_DATA_ADDRESS);
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kMenuSelectX[preMenuTab], kMenuSelectY[preMenuTab], kIconSelectWidth, kIconSelectHeight);
            break;
    }

    // 4. CURRENT SELECTION DRAW
    // Draw ON icon for currently selected tab when tab index is valid.
    if (menuTab != -1) {
        switch (menu) {
            case LAYER_INST_0_MENU:

            case LAYER_INST_1_MENU:

            case LAYER_INST_2_MENU:
                dataPtr = (const uint8_t*)(RAM_ICON_SELECT_ON_DATA_ADDRESS);
                lcd.drawRGB16Image(indexPtr, dataPtr, 64, kMenuSelectX[menuTab], kMenuSelectY[menuTab], kIconSelectWidth, kIconSelectHeight);
                break;

            case MIXER_MENU:

            case LFO_MENU:

            case EQ_MENU:

            case FILTER_0_MENU:

            case FILTER_1_MENU:

            case EFFECT_0_MENU:

            case EFFECT_1_MENU:

            case REVERB_MENU:

            case LAYER_SONG_MENU:

                // System menus: draw ON icon for current tab (8-slot layout)
                dataPtr = (const uint8_t*)(RAM_ICON_SELECT_ON_DATA_ADDRESS);
                lcd.drawRGB16Image(indexPtr, dataPtr, 64, kMenuSelectX[menuTab], kMenuSelectY[menuTab], kIconSelectWidth, kIconSelectHeight);
                break;

            case FILE_MENU:

            case DRUMKIT_MENU:

            case IO_MENU:

            case RHYTHM_MENU:

            case METRO_MENU:

                // File/utility menus: draw ON icon for current tab (4-slot layout)
                dataPtr = (const uint8_t*)(RAM_ICON_SELECT_ON_DATA_ADDRESS);
                lcd.drawRGB16Image(indexPtr, dataPtr, 64, kMenuSelectX[menuTab], kMenuSelectY[menuTab], kIconSelectWidth, kIconSelectHeight);
                break;
        }
    }
}

void Controller::lcd_setMenuHeaderState(RGB16Color color_) {
    /// @brief Sets LCD style for menu header text rendering.

    // 1. STYLE APPLY
    // Apply color, background, font, and alignment for headers.
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(LEFT);
}

void Controller::lcd_setMenuDataState(RGB16Color color_) {
    /// @brief Sets LCD style for menu data value rendering.

    // 1. STYLE APPLY
    // Apply color, background, font, and alignment for data values.
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(RIGHT);
}

void Controller::lcd_setMenuNumState(RGB16Color color_) {
    /// @brief Sets LCD style for compact numeric value rendering.

    // 1. STYLE APPLY
    // Apply color, background, font, and alignment for numeric values.
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(RIGHT);
}

void Controller::lcd_setMenuTextState(RGB16Color color_) {
    /// @brief Sets LCD style for compact text value rendering.

    // 1. STYLE APPLY
    // Apply color, background, font, and alignment for text values.
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(RIGHT);
}

void Controller::lcd_setMenuSignState(RGB16Color color_) {
    /// @brief Sets LCD style for unit-symbol/sign rendering.

    // 1. STYLE APPLY
    // Apply color, background, font, and alignment for signs.
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(RIGHT);
}

void Controller::lcd_setLayerTabState(uint8_t layerNum_) {
    /// @brief Sets LCD style for layer tab with selection-aware color.

    // 1. STYLE APPLY
    // Apply selected/default foreground color and shared text style.
    (layerNum_ == selectedLayerNum) ? lcd.setForeColor(kLayerSelectedColor) : lcd.setForeColor(kLayerColorPalette[layerNum_]);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(LEFT);
}

/* Lcd Layer Tab Functions ---------------------------------------------------*/

void Controller::lcd_drawLayerTab(uint8_t layerNum_) {
    /// @brief Draws complete layer-tab row including indicators and song lane.

    // 1. ROW BORDER DRAW
    // Draw top/bottom layer row borders.
    lcd_drawLayerBox(layerNum_);

    // 2. NUMBER DRAW
    // Draw layer index text.
    lcd_drawLayerTab_NumData(layerNum_);

    // 3. INSTRUMENT DRAW
    // Draw loaded instrument short name.
    lcd_drawLayerTab_InstData(layerNum_);

    // 4. MUTE BOX DRAW
    // Draw mute state indicator box.
    lcd_drawLayerTab_Mute(layerNum_);

    // 5. FILL BOX DRAW
    // Draw fill state indicator box.
    lcd_drawLayerTab_Fill(layerNum_);

    // 6. STYLE BOX DRAW
    // Draw style state indicator box.
    lcd_drawLayerTab_Style(layerNum_);

    // 7. SONG LANE DRAW
    // Draw song beat lane for active bank.
    lcd_drawSong(layerNum_, activeSongBank);
}

void Controller::lcd_drawLayerBox(uint8_t layerNum_) {
    /// @brief Draws top and bottom border lines for a layer row.

    // 1. COLOR SELECT
    // Use selected or default row color.
    (layerNum_ == selectedLayerNum) ? lcd.setForeColor(kAccentColor) : lcd.setForeColor(GRAY_75);

    // 2. BORDER DRAW
    // Draw top and bottom horizontal border lines.
    lcd.drawHLine(kLayerLineX, kLayerLineY[layerNum_][0], kLayerLineWidth);
    lcd.drawHLine(kLayerLineX, kLayerLineY[layerNum_][1], kLayerLineWidth);
}

void Controller::lcd_drawLayerTab_NumData(uint8_t layerNum_) {
    /// @brief Draws layer number text in the layer tab row.

    // 1. STYLE SETUP
    // Configure layer-tab text style.
    lcd_setLayerTabState(layerNum_);

    // 2. VALUE DRAW
    // Draw layer number label.
    lcd.drawText(kDataLayerNum[layerNum_], 2, kLayerTextNumberX, kLayerTextY[layerNum_]);
}

void Controller::lcd_drawLayerTab_InstData(uint8_t layerNum_) {
    /// @brief Draws loaded instrument short name for layer row.

    // 1. STYLE SETUP
    // Configure layer-tab text style.
    lcd_setLayerTabState(layerNum_);

    // 2. CONTEXT LOAD
    // Load layer reference for text lookup.
    Layer& layer = layerLibrary[layerNum_];

    // 3. VALUE DRAW
    // Draw loaded instrument short label. A SYNTH layer plays no sample: it shows its sound's
    // preset name instead (v1.14), padded so a longer old name is fully overwritten.
    if (layer.toneMode == tone::kModeSynth) {
        char name[16];
        synth_soundName(layerNum_, name, sizeof(name));
        char field[13];
        snprintf(field, sizeof(field), "%-12.12s", name);
        lcd.drawText(field, 12, kLayerTextInstX, kLayerTextY[layerNum_]);
        return;
    }
    lcd.drawText(layer.instLoadedData.nameShortL, 12, kLayerTextInstX, kLayerTextY[layerNum_]);
}

void Controller::lcd_drawLayerTab_Mute(uint8_t layerNum_) {
    /// @brief Draws mute state indicator box for layer row.

    // 1. STYLE SETUP
    // Configure box color from layer-tab state.
    lcd_setLayerTabState(layerNum_);

    // 2. OUTLINE DRAW
    // Draw mute indicator box outline.
    lcd.drawRect(kLayerBoxMuteX, kLayerBoxY[layerNum_], kLayerBoxWidth, kLayerBoxHeight);

    // 3. FILL STATE DRAW
    // Fill box when state is active, clear when inactive.
    if (!layerLibrary[layerNum_].mute) lcd.setForeColor(BLACK);
    lcd.fillRect(kLayerBoxMuteX + 1, kLayerBoxY[layerNum_] + 1, kLayerBoxWidth - 2, kLayerBoxHeight - 2);
}

void Controller::lcd_drawLayerTab_Fill(uint8_t layerNum_) {
    /// @brief Draws fill state indicator box for layer row.

    // 1. STYLE SETUP
    // Configure box color from layer-tab state.
    lcd_setLayerTabState(layerNum_);

    // 2. OUTLINE DRAW
    // Draw fill indicator box outline.
    lcd.drawRect(kLayerBoxFillX, kLayerBoxY[layerNum_], kLayerBoxWidth, kLayerBoxHeight);

    // 3. FILL STATE DRAW
    // Fill box when state is active, clear when inactive.
    if (!layerLibrary[layerNum_].fill) lcd.setForeColor(BLACK);
    lcd.fillRect(kLayerBoxFillX + 1, kLayerBoxY[layerNum_] + 1, kLayerBoxWidth - 2, kLayerBoxHeight - 2);
}

void Controller::lcd_drawLayerTab_Style(uint8_t layerNum_) {
    /// @brief Draws style state indicator box for layer row.

    // 1. STYLE SETUP
    // Configure box color from layer-tab state.
    lcd_setLayerTabState(layerNum_);

    // 2. OUTLINE DRAW
    // Draw style indicator box outline.
    lcd.drawRect(kLayerBoxStyleX, kLayerBoxY[layerNum_], kLayerBoxWidth, kLayerBoxHeight);

    // 3. FILL STATE DRAW
    // Fill box when state is active, clear when inactive.
    if (!layerLibrary[layerNum_].style) lcd.setForeColor(BLACK);
    lcd.fillRect(kLayerBoxStyleX + 1, kLayerBoxY[layerNum_] + 1, kLayerBoxWidth - 2, kLayerBoxHeight - 2);
}

/* Lcd Transition Functions --------------------------------------------------*/

void Controller::lcd_drawTransition() {
    /// @brief Draws and clears temporary transition indicators for menu/encoders.

    // 1. SHOW-PHASE HANDLING
    // Render transition indicators when show flag is armed.
    if (transitionShowFlag) {
        // Draw menu transition indicator
        if (transitionShowMenu) {
            lcd.setFont(FONT_05x07);
            lcd.setAlignment(RIGHT);
            (transitionShowFlag == 1) ? lcd.setForeColor(kAccent2Color) : lcd.setForeColor(GREEN);
            lcd.setBackColor(BLACK);
            lcd.drawText("    PASS", kMenuTextBoxTextSize, kMenuTextBoxX[0], kMenuTextBoxY[0]);
            transitionShowMenu = 0;
        }

        // Draw encoder transition indicators
        for (uint8_t i = 0; i < kEncoderSize; i++) {
            if (transitionShowEncoder[i]) {
                lcd.setFont(FONT_07x09);
                lcd.setAlignment(RIGHT);
                (transitionShowFlag == 1) ? lcd.setForeColor(kAccent2Color) : lcd.setForeColor(GREEN);

                // lcd.clearRect(kEncoderDataX[i] - 53, kEncoderDataY[i], 7, 7);
                // lcd.drawRect(kEncoderDataX[i] - 53, kEncoderDataY[i], 7, 7);
                transitionShowEncoder[i] = 0;
            }
        }
        transitionShowFlag = 0;
    }

    // 2. CLEAR-PHASE HANDLING
    // Clear transition indicators when clear flag is armed.
    if (transitionClearFlag) {
        // Clear menu transition indicator
        if (transitionClearMenu) {
            lcd.setFont(FONT_05x07);
            lcd.setAlignment(RIGHT);
            lcd.setForeColor(kAccent2Color);
            lcd.setBackColor(BLACK);
            lcd.drawText("        ", kMenuTextBoxTextSize, kMenuTextBoxX[0], kMenuTextBoxY[0]);
            transitionClearMenu = false;
        }

        // Clear encoder transition indicators
        for (uint8_t i = 0; i < kEncoderSize; i++) {
            if (transitionClearEncoder[i]) {
                lcd.setFont(FONT_07x09);
                lcd.setAlignment(RIGHT);
                lcd.setForeColor(kAccent2Color);

                // lcd.clearRect(kEncoderDataX[i] - 53, kEncoderDataY[i], 7, 7);
                // lcd.drawRect(kEncoderDataX[i] - 53, kEncoderDataY[i], 7, 7);
                transitionClearEncoder[i] = false;
            }
        }
        transitionClearFlag = false;
    }
}

void Controller::checkMenuTransition(Menu menu_, bool mode_) {
    /// @brief Updates menu transition show/clear flags for current menu.

    // 1. FLAG UPDATE
    // Set show or clear flag when current menu matches requested menu.
    if (menu == menu_) (mode_) ? transitionShowMenu = true : transitionClearMenu = true;
}

void Controller::checkEncoderTransition(EncoderFunc function_, bool mode_) {
    /// @brief Updates encoder transition flags for matching global encoder bindings.

    // 1. ENCODER SCAN
    // Set show or clear flag for each matching encoder slot.
    if (activeEncoderBank >= kEncoderBankSize) return;

    for (uint8_t i = 0; i < kEncoderSize; i++) {
        if (encoderGlobalFunction[activeEncoderBank][i] == function_) (mode_) ? transitionShowEncoder[i] = true : transitionClearEncoder[i] = true;
    }
}

/* Lcd Play Functions --------------------------------------------------------*/

void Controller::lcd_drawPlay() {
    /// @brief Draws playback head progression and handles reset redraw trigger.

    // 1. PLAYBACK HEAD DRAW
    // Draw incremental playhead position while transport is running.
    if ((playActive) && (!metronome.precountState)) {
        if (((playInterval / playXRatio) > playX) && (playX < kPlayWidth)) {
            // Draw playhead line
            lcd.setForeColor(playColor);
            lcd.drawVLine(kPlayX + playX, kPlayY, 1);
            playX += 1;
        }
    }

    // 2. RESET HANDLING
    // Apply deferred reset request and clear one-shot flag.
    if (resetPlayFlag) {
        lcd_resetPlay();
        resetPlayFlag = false;
    }

}

void Controller::lcd_drawIcon() {
    /// @brief Draws pending transport icon updates (reset/play/stop/record).

    // 1. ICON CONTEXT SETUP
    // Resolve icon palette and shared draw pointers.
    const RGB16Color* indexPtr;
    const uint8_t* dataPtr;
    uint16_t xPos;
    indexPtr = (const RGB16Color*)(RAM_ICON_PLAY_PALETTE_ADDRESS);

    // 2. RESET ICON UPDATE
    // Draw reset icon when refresh flag is set.
    if (resetIcon.flag) {
        (resetIcon.mode) ? dataPtr = (const uint8_t*)(RAM_ICON_PLAY_RESET_ON_DATA_ADDRESS) : dataPtr = (const uint8_t*)(RAM_ICON_PLAY_RESET_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconResetX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        resetIcon.flag = false;
    }

    // 3. PLAY ICON UPDATE
    // Draw play icon when refresh flag is set.
    if (playIcon.flag) {
        (playIcon.mode) ? dataPtr = (const uint8_t*)(RAM_ICON_PLAY_PLAY_ON_DATA_ADDRESS) : dataPtr = (const uint8_t*)(RAM_ICON_PLAY_PLAY_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconPlayX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        playIcon.flag = false;
    }

    // 4. STOP ICON UPDATE
    // Draw stop icon when refresh flag is set.
    if (stopIcon.flag) {
        (stopIcon.mode) ? dataPtr = (const uint8_t*)(RAM_ICON_PLAY_STOP_ON_DATA_ADDRESS) : dataPtr = (const uint8_t*)(RAM_ICON_PLAY_STOP_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconStopX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        stopIcon.flag = false;
    }

    // 5. RECORD ICON UPDATE
    // Draw record icon when refresh flag is set.
    if (recordIcon.flag) {
        (recordIcon.mode) ? dataPtr = (const uint8_t*)(RAM_ICON_PLAY_RECORD_ON_DATA_ADDRESS) : dataPtr = (const uint8_t*)(RAM_ICON_PLAY_RECORD_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconRecordX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        recordIcon.flag = false;
    }
}

void Controller::lcd_drawCopyPaste() {
    /// @brief Draws copy/paste text notifications and clears transient text state.

    // 1. TEXT STYLE SETUP
    // Configure shared style for short status text rendering.
    lcd.setBackColor(BLACK);
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_05x07);

    // 2. ARMED NOTIFICATION
    // Draw persistent ARMED text (no timer) when slave mode arming is active.
    if (textArmedFlag) {
        textArmedFlag = false;
        lcd.setForeColor(YELLOW);
        lcd.drawText("ARMED", 5, 170, 112);

        // 3. COPY NOTIFICATION
        // Draw COPY text and start auto-clear timer.
    } else if (textCopyFlag) {
        textCopyFlag = false;
        textShow = true;
        startTextTimer();
        lcd.setForeColor(kAccentColor);
        lcd.drawText("COPY", 4, 170, 112);

        // 4. PASTE NOTIFICATION
        // Draw PASTE text and start auto-clear timer.
    } else if (textPasteFlag) {
        textPasteFlag = false;
        textShow = true;
        startTextTimer();
        lcd.setForeColor(kAccentColor);
        lcd.drawText("PASTE", 5, 170, 112);

        // 5. CLEAR NOTIFICATION
        // Clear status text, or restore ARMED if slave arming is still active.
    } else if (textClearFlag) {
        textClearFlag = false;
        if (playArmed || recordArmed) {
            lcd.setForeColor(YELLOW);
            lcd.drawText("ARMED", 5, 170, 112);
        } else {
            lcd.drawText("     ", 5, 170, 112);
        }
    }
}

void Controller::lcd_drawCountDown() {
    /// @brief Draws precount countdown text and updates countdown state.

    // 1. COUNTDOWN GATE
    // Only render when precount countdown flag is armed.
    if (metronome.countDownFlag) {
        if (metronome.countDown > 0) {
            // 2. TEXT STYLE SETUP
            // Configure text style for countdown rendering.
            lcd.setAlignment(LEFT);
            lcd.setFont(FONT_05x07);
            lcd.setBackColor(BLACK);
            lcd.setForeColor(0xF80F);

            // 3. DRAW & DECREMENT
            // Draw current countdown value then decrement by one.
            char kCountDownData[2];
            sprintf(kCountDownData, "%02d", metronome.countDown);
            lcd.drawText(kCountDownData, 2, 163, 112);
            metronome.countDown -= 1;
        } else {
            // 4. TERMINATION
            // Clear countdown area when value reaches zero.
            lcd_clearCountDown();
        }

        // 5. FLAG RESET
        // Consume one-shot draw trigger for this update tick.
        metronome.countDownFlag = false;
    }
}

void Controller::lcd_clearCountDown() {
    /// @brief Clears countdown display region.

    // 1. REGION CLEAR
    // Reset countdown text area to background color.
    lcd.setBackColor(BLACK);
    lcd.clearRect(163, 112, 12, 7);
}

void Controller::lcd_restartPlay() {
    /// @brief Restarts playhead from loop start while keeping alternating color flow.

    // 1. COLOR FLIP
    // Toggle playhead color before restarting draw cycle.
    lcd_invertPlayColor();

    // 2. POSITION RESET
    // Move playhead position back to zero.
    playX = 0;
}

void Controller::lcd_resetPlay() {
    /// @brief Resets playback bar visuals and playhead state to defaults.

    // 1. POSITION RESET
    // Reset playhead position to start of timeline.
    playX = 0;

    // 2. SCALE RECALCULATION
    // Recompute playhead interval-to-pixel ratio.
    lcd_calculateSongX();

    // 3. BASELINE REDRAW
    // Redraw full baseline bar in default foreground color.
    lcd.setForeColor(kPlayColor1);
    lcd.drawHLine(kPlayX, kPlayY, kPlayWidth);

    // 4. COLOR STATE RESET
    // Restore deterministic color state for subsequent updates.
    lcd_resetPlayColor();
    lcd_invertPlayColor();
}

void Controller::lcd_redrawPlay() {
    /// @brief Redraws playback bar using current interval and alternating color state.

    // 1. PLAYHEAD POSITION UPDATE
    // Convert current interval position into horizontal pixel position.
    playX = playInterval / playXRatio;

    // 2. BAR REDRAW
    // Draw full playback bar with current active play color.
    lcd.setForeColor(playColor);
    lcd.drawHLine(kPlayX, kPlayY, kPlayWidth);

    // 3. NEXT-FRAME COLOR FLIP
    // Toggle color for alternating visual update behavior.
    lcd_invertPlayColor();
}

void Controller::lcd_cleanEndPlay() {
    /// @brief Clears trailing segment of playback bar after stop/end state.

    // 1. FOREGROUND SELECTION
    // Select opposite color to erase trailing section cleanly.
    (lcd.getForeColor() == kPlayColor0) ? lcd.setForeColor(kPlayColor1) : lcd.setForeColor(kPlayColor0);

    // 2. TAIL CLEANUP
    // Clear right-side tail from current playhead position to bar end.
    lcd.drawHLine(kPlayX + playX + 1, kPlayY, kPlayWidth - playX);
}

void Controller::lcd_invertPlayColor() {
    /// @brief Toggles playhead color between active playback colors.

    // 1. COLOR TOGGLE
    // Flip play color for alternating draw behavior.
    (playColor == kPlayColor0) ? playColor = kPlayColor1 : playColor = kPlayColor0;
}

void Controller::lcd_resetPlayColor() {
    /// @brief Resets playhead color to default active color.

    // 1. COLOR RESET
    // Force play color to initial default.
    playColor = kPlayColor1;
}

/* Lcd Song Functions --------------------------------------------------------*/

void Controller::lcd_calculateSongX() {
    /// @brief Calculates horizontal interval-to-pixel ratio for play/song rendering.

    // 1. SCALE CALCULATION
    // Convert full song interval domain into play bar pixel width domain.
    playXRatio = (float)songInterval / kPlayWidth;
}

void Controller::lcd_drawMeasureBar() {
    /// @brief Draws measure/bar grid markers across all layer song lanes.

    // 1. GRID SPACING PRE-CALC
    // Calculate bar and measure spacing in pixels.
    float barX = (float)kPlayWidth / rhythm.bar;                         // Bar width
    float measureX = (float)kPlayWidth / (rhythm.measure * rhythm.bar);  // Measure width

    // 2. DRAW STYLE SETUP
    // Configure foreground/background colors for guide lines.
    lcd.setForeColor(GRAY_60);
    lcd.setBackColor(BLACK);

    // 3. GRID RENDER LOOP
    // Clear previous guides and redraw bar/measure separators.
    for (uint8_t i = 1; i < 11; i++) {
        // Clear previous grid line
        lcd.clearRect(kPlayX + 2, 99 + ((1 + i) * 31) - 6, kPlayWidth - 4, 3);
        for (uint8_t j = 0; j <= (rhythm.measure * rhythm.bar); j++) {
            uint16_t posX = kSongX - 1 + int(j * measureX);

            // Draw thick line for bar, thin for measure
            if (j % rhythm.measure == 0) {
                lcd.fillRect(posX - 1, 99 + ((1 + i) * 31) - 6, 3, 3);
            } else {
                lcd.fillRect(posX, 99 + ((1 + i) * 31) - 6, 1, 3);
            }
        }
    }
}

void Controller::lcd_drawBeat(uint8_t layerNum_, uint8_t bankNum_, uint8_t beatNum_, bool selected_) {
    /// @brief Draws one beat cell for a layer/bank with style-aware fill rendering.

    // 1. ACTIVE BANK GUARD
    // Render only for currently active song bank.
    if (bankNum_ == activeSongBank) {
        Layer& layer = layerLibrary[layerNum_];
        Bank& bank = layer.bankLibrary[bankNum_];
        Beat& beat = bank.beatLibrary[beatNum_];
        bool active = beat.getActive();
        uint8_t fillType = beat.getFillType();
        uint8_t fillPattern = beat.getFillPattern();
        lcd.setBackColor(BLACK);

        // 2. COLOR SELECTION
        // Resolve draw color from selection and layer context.
        if (selected_) {
            lcd.setForeColor(kBeatSelectedColor);
        } else {
            (layerNum_ == selectedLayerNum) ? lcd.setForeColor(kLayerSelectedColor) : lcd.setForeColor(kLayerColorPalette[layerNum_]);
        }

        // 3. BEAT SHAPE RENDER
        // Convert beat interval to pixel coords and draw by fill type.
        if (active) {
            // Calculate beat coordinates
            uint16_t startInterval = beat.getStartInterval();
            uint16_t endInterval = beat.getEndInterval();
            uint16_t xStart = kSongX + (startInterval * kSongWidth / songInterval);
            uint16_t xEnd = kSongX + (endInterval * kSongWidth / songInterval) - 2;
            uint16_t yStart = kImageLayerY[layerNum_] + kSongOffsetY;

            // Draw beat shape based on fill type
            if ((fillType == 0) && (fillPattern < 2)) {
                lcd.clearRect(xStart, yStart, xEnd - xStart + 1, kSongHeight);
                lcd.drawRect(xStart, yStart, 2, kSongHeight);
            } else if ((fillType == 0) && (fillPattern == 2)) {
                lcd.clearRect(xStart, yStart, xEnd - xStart + 1, kSongHeight);
                for (uint8_t i = 0; i < 4; i++) {
                    lcd.drawPixel(xStart, yStart + (i * 3));
                }
            } else {
                lcd.fillRect(xStart, yStart, xEnd - xStart, kSongHeight);
            }
        }
    }
}

void Controller::lcd_clearBeat(uint8_t layerNum_, uint8_t bankNum_, uint8_t beatNum_) {
    /// @brief Clears rendered area of one beat cell for a layer/bank.

    // 1. ACTIVE BANK GUARD
    // Clear only when request targets currently active bank.
    if (bankNum_ == activeSongBank) {
        Layer& layer = layerLibrary[layerNum_];
        Bank& bank = layer.bankLibrary[bankNum_];
        Beat& beat = bank.beatLibrary[beatNum_];

        // 2. COORDINATE RESOLUTION
        // Convert beat interval to pixel rectangle bounds.
        uint16_t startInterval = beat.getStartInterval();
        uint16_t endInterval = beat.getEndInterval();
        uint16_t xStart = kSongX + (startInterval * kSongWidth / songInterval);
        uint16_t xEnd = kSongX + (endInterval * kSongWidth / songInterval);
        uint16_t yStart = kImageLayerY[layerNum_] + kSongOffsetY;

        // 3. REGION CLEAR
        // Clear beat rectangle to background color.
        lcd.setBackColor(BLACK);
        lcd.clearRect(xStart, yStart, xEnd - xStart, (kSongHeight + 1));
    }
}

void Controller::lcd_drawSong(uint8_t layerNum_, uint8_t bankNum_) {
    /// @brief Redraws all active beats for one layer in selected bank.

    // 1. ACTIVE BANK GUARD
    // Draw only for currently active bank.
    if (bankNum_ == activeSongBank) {
        Layer& layer = layerLibrary[layerNum_];
        Bank& bank = layer.bankLibrary[bankNum_];

        // 2. LANE CLEAR
        // Clear previous lane rendering before redraw.
        lcd_clearSong(layerNum_, bankNum_);

        // 3. ACTIVE BEAT REDRAW
        // Iterate beats and draw only active ones.
        if (bank.lastActiveBeatNum != -1) {
            for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                Beat& beat = bank.beatLibrary[i];
                if (beat.getActive()) {
                    bool selected = (layerNum_ == selectedLayerNum) && (i == selectedBeatNum);
                    lcd_drawBeat(layerNum_, bankNum_, i, selected);
                }
            }
        }
    }
}

void Controller::lcd_clearSong(uint8_t layerNum_, uint8_t bankNum_) {
    /// @brief Clears complete song lane region for one layer.

    // 1. ACTIVE BANK GUARD
    // Clear only when request belongs to active bank.
    if (bankNum_ == activeSongBank) {
        uint16_t yStart = kImageLayerY[layerNum_] + kSongOffsetY;

        // 2. LANE CLEAR
        // Clear full lane rectangle in song area.
        lcd.setBackColor(BLACK);
        lcd.clearRect(kSongX, yStart, kSongWidth + 1, kSongHeight + 1);
    }
}

void Controller::lcd_clearInterval(uint8_t layerNum_, uint8_t bankNum_, uint16_t startInterval_, uint16_t endInterval_) {
    /// @brief Clears a specific interval segment in one layer lane.

    // 1. ACTIVE BANK GUARD
    // Clear only when request belongs to active bank.
    if (bankNum_ == activeSongBank) {
        // 2. INTERVAL COORDINATES
        // Convert interval boundaries into pixel coordinates.
        uint16_t xStart = kSongX + (startInterval_ * kSongWidth / songInterval);
        uint16_t xEnd = kSongX + (endInterval_ * kSongWidth / songInterval);
        uint16_t yStart = kImageLayerY[layerNum_] + kSongOffsetY;

        // 3. SEGMENT CLEAR
        // Clear requested sub-region in lane.
        lcd.setBackColor(BLACK);
        lcd.clearRect(xStart, yStart, xEnd - xStart, kSongHeight + 1);
    }
}

