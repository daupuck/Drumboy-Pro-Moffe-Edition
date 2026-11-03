// System/encoder presets, project files and drumkits.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Preset Functions ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::loadSystemPreset() {
    /// @brief Retrieves and applies global device settings from the SD card.
    /// Loads mixer levels, metronome configurations, and I/O synchronization parameters.

    // 1. FILE ACCESS
    // Open system preset file with read/write permissions.
    char refRead[strlen(kSystemPresetRef) + 1];
    if (f_open(&sd.file, "System/Preset/System.rwp", FA_READ | FA_WRITE) == FR_OK) {
        // 2. HEADER VERIFICATION
        // Validate file identity using preset reference header.
        if (sd_read(refRead, strlen(kSystemPresetRef)) == FR_OK) {
            if (strncmp(kSystemPresetRef, refRead, strlen(kSystemPresetRef)) == 0) {
                if (f_lseek(&sd.file, 25) == FR_OK) {
                    // 3. DATA RETRIEVAL
                    // Load persisted system preset payload into RAM buffer.
                    // The first 21 bytes are required (format up to v1.04); the preview bytes
                    // [21..22] are optional so a shorter file from older firmware still loads.
                    const UINT kLegacySystemPresetDataSize = 21;
                    UINT systemPresetBytes = 0;
                    if ((f_read(&sd.file, dataSystemPreset, kSystemPresetDataSize, &systemPresetBytes) == FR_OK) &&
                        (systemPresetBytes >= kLegacySystemPresetDataSize)) {
                        // 4. MIXER/AMPLIFIER PARAMETER APPLY
                        // Apply persisted mixer and amplifier values through setter APIs.
                        mixer_setMasterVolume(dataSystemPreset[0]);
                        mixer_setHpVolume(dataSystemPreset[1]);
                        mixer_setLoVolume(dataSystemPreset[2]);
                        mixer_setLiVolume(dataSystemPreset[3]);
                        mixer_setLiIn(dataSystemPreset[4]);
                        mixer_setPan(dataSystemPreset[5]);
                        mixer_setLimiter(dataSystemPreset[6]);
                        mixer_setNormalize((bool)dataSystemPreset[7]);
                        mixer_setMasterMute((bool)dataSystemPreset[8]);
                        mixer_setHpMute((bool)dataSystemPreset[9]);
                        mixer_setLoMute((bool)dataSystemPreset[10]);
                        // mixer_setLiMute((bool)dataSystemPreset[11]);

                        // 5. METRONOME PARAMETER APPLY
                        // Apply persisted metronome state and related parameters.
                        metro_setActive((bool)dataSystemPreset[12]);
                        metro_setPrecount((bool)dataSystemPreset[13]);
                        metro_setSample(dataSystemPreset[14]);
                        metro_setVolume(dataSystemPreset[15]);

                        // 6. I/O & SYNC PARAMETER APPLY
                        // Apply persisted sync and MIDI routing parameters.
                        io_setSyncIn(dataSystemPreset[16]);
                        io_setSyncOut(dataSystemPreset[17]);
                        io_setMidiIn((dataSystemPreset[18] >> 4) & 0x07);
                        io_setMidiInChannel(dataSystemPreset[18] & 0x0F);
                        io_setMidiOut((dataSystemPreset[19] >> 4) & 0x07);
                        io_setSyncInPpqn((dataSystemPreset[20] >> 4) & 0x0F);
                        io_setSyncOutPpqn(dataSystemPreset[20] & 0x0F);
                        io_setMidiOutChannel(dataSystemPreset[19] & 0x0F);

                        // 7. SAMPLE PREVIEW (v1.05+). Files written by older firmware hold
                        // unrelated bytes here, so anything out of range falls back to defaults.
                        if (systemPresetBytes >= kSystemPresetPreviewSize) {
                            const uint8_t previewByte = dataSystemPreset[21];
                            const uint8_t levelByte = dataSystemPreset[22];
                            previewActive = (previewByte <= 1) ? (previewByte == 1) : kInitialPreviewActive;
                            previewLevel = (levelByte <= kMaxPreviewLevel) ? levelByte : kInitialPreviewLevel;
                        } else {
                            previewActive = kInitialPreviewActive;
                            previewLevel = kInitialPreviewLevel;
                        }
                        previewPlay.gain = kNumberDataLibrary[previewLevel].powMultiplier;

                        // 8. AUTOSAVE (v2.01): on/off and "a session waits to be resumed". Tagged
                        // bytes, so whatever older firmware left here reads as ON / nothing pending.
                        if (systemPresetBytes >= kSystemPresetDataSize) {
                            autosaveEnabled = autosave::decodeEnabled(static_cast<uint8_t>(dataSystemPreset[23]));
                            autosavePending = autosave::decodePending(static_cast<uint8_t>(dataSystemPreset[24]));
                        } else {
                            autosaveEnabled = autosave::kDefaultEnabled;
                            autosavePending = false;
                        }

                        // Enable persistence engine for future change tracking.
                        systemPresetSaveActive = true;
                    }
                }
            }
        }
    }

    // 7. FILE HANDLE CLOSE
    // Ensure file handle is closed after load attempt.
    f_close(&sd.file);
}

void Controller::saveSystemPreset() {
    /// @brief Serializes current system parameters (Mixer, Metronome, I/O) and commits them to the SD card.
    /// Utilizes a deferred write strategy to minimize SD wear and CPU overhead.

    if (sd.ready) {
        // 1. CRITICAL SECTION ENTRY
        // Temporarily disable keyboard input during serialization.
        keyboard_disable();
        bool saveFailed = false;

        // 2. CHANGE DETECTION
        // Continue only when preset save engine is active and dirty flag is set.
        if ((systemPresetSaveActive) && (systemPresetSaveFlag)) {
            // 3. DATA PACKAGING (SNAPSHOT)
            // Capture mixer, metronome, and I/O fields into preset buffer.
            // Mixer & Audio Engine States
            dataSystemPreset[0] = mixer.masterVolume;
            dataSystemPreset[1] = mixer.hpVolume;
            dataSystemPreset[2] = mixer.loVolume;
            dataSystemPreset[3] = mixer.liVolume;
            dataSystemPreset[4] = mixer.liMixerIn;
            dataSystemPreset[5] = mixer.pan;
            dataSystemPreset[6] = mixer.limiter;
            dataSystemPreset[7] = (uint8_t)mixer.normalize;
            dataSystemPreset[8] = (uint8_t)mixer.masterMute;
            dataSystemPreset[9] = (uint8_t)mixer.hpMute;
            dataSystemPreset[10] = (uint8_t)mixer.loMute;
            dataSystemPreset[11] = (uint8_t)mixer.liMute;

            // Metronome Configurations
            dataSystemPreset[12] = (uint8_t)metronome.active;
            dataSystemPreset[13] = (uint8_t)metronome.precount;
            dataSystemPreset[14] = metronome.sample;
            dataSystemPreset[15] = metronome.volume;

            // I/O & Hardware Sync Settings
            dataSystemPreset[16] = io.syncIn;
            dataSystemPreset[17] = io.syncOut;
            dataSystemPreset[18] = (uint8_t)((io.midiIn << 4) | io.midiInChannel);
            dataSystemPreset[19] = (uint8_t)((io.midiOut << 4) | io.midiOutChannel);
            dataSystemPreset[20] = (uint8_t)((io.syncInPpqn << 4) | io.syncOutPpqn);

            // Sample preview (v1.05+)
            dataSystemPreset[21] = (uint8_t)previewActive;
            dataSystemPreset[22] = previewLevel;

            // AUTOSAVE (v2.01)
            dataSystemPreset[23] = static_cast<char>(autosave::encodeEnabled(autosaveEnabled));
            dataSystemPreset[24] = static_cast<char>(autosavePending ? autosave::kPendingMark : 0);

            // 4. SD CARD WRITE
            // Seek payload offset and write serialized preset data.
            // A failed write keeps the dirty flag set so the next save cycle retries
            // instead of silently dropping the change.
            if (f_open(&sd.file, "System/Preset/System.rwp", FA_READ | FA_WRITE) == FR_OK) {
                // Seek to offset 25 (skipping the file header/magic number)
                if ((f_lseek(&sd.file, 25) != FR_OK) || (sd_write(dataSystemPreset, kSystemPresetDataSize) != FR_OK)) {
                    saveFailed = true;
                }
                // f_close flushes the cached sector; its result is part of the write.
                if (f_close(&sd.file) != FR_OK) saveFailed = true;
            } else {
                saveFailed = true;
            }
        }

        // 5. CLEANUP & STATE RESET
        // Clear save flag (unless the write failed) and restore keyboard input handling.
        systemPresetSaveFlag = saveFailed;  // Clear the "dirty" flag only on success
        keyboard_enable();             // Re-enable user interaction
    }
}

void Controller::loadEncoderPreset() {
    /// @brief Loads global encoder function mappings from the SD card.
    /// Distributes function assignments across all encoder banks and initializes the active tab.

    // 1. FILE ACCESS
    // Open encoder preset file with read/write permissions.
    char refRead[strlen(kEncoderPresetRef) + 1];
    if (f_open(&sd.file, "System/Preset/Encoder.rwp", FA_READ | FA_WRITE) == FR_OK) {
        // 2. HEADER VERIFICATION
        // Validate file identity using encoder preset reference header.
        if (sd_read(refRead, strlen(kEncoderPresetRef)) == FR_OK) {
            if (strncmp(kEncoderPresetRef, refRead, strlen(kEncoderPresetRef)) == 0) {
                // 3. DATA RETRIEVAL
                // Seek payload offset and load global encoder mapping data.
                f_lseek(&sd.file, 25);

                // Read the raw function mapping array into RAM
                if (sd_read(dataEncoderGlobalPreset, kEncoderGlobalPresetDataSize) == FR_OK) {
                    // 4. MATRIX DISTRIBUTION
                    // Map flat preset array back into bank/encoder matrix.
                    for (uint8_t i = 0; i < kEncoderBankSize; i++) {  // For each Bank/Tab
                        for (uint8_t j = 0; j < kEncoderSize; j++) {  // For each physical Encoder

                            uint8_t function = dataEncoderGlobalPreset[(i * kEncoderSize) + j];

                            // Validate loaded function ID before assignment.
                            if ((function >= kMinEncoderGlobalFunction) && (function <= kMaxEncoderGlobalFunction)) {
                                encoderGlobalFunction[i][j] = (EncoderFunc)function;
                            }

                            // 5. ACTIVE TAB INITIALIZATION
                            // Apply mapping immediately for default bank at startup. Use the
                            // validated matrix entry, not the raw file byte, so a corrupt preset
                            // cannot push an out-of-range function ID into the encoder engine.
                            if (i == 0) {
                                encoder_setGlobalFunction(i, j, encoderGlobalFunction[i][j]);
                            }
                        }
                    }
                }

                // Enable persistence engine for future encoder changes.
                encoderPresetSaveActive = true;
            }
        }
    }

    // FILE HANDLE CLOSE
    // Ensure file handle is closed after load attempt.
    f_close(&sd.file);
}

void Controller::saveEncoderPreset() {
    /// @brief Serializes and saves both Global and Local encoder function mappings to the SD card.
    /// Ensures that user-defined knob assignments and menu-specific controls are persistent across power cycles.

    if (sd.ready) {
        // 1. CRITICAL SECTION ENTRY
        // Temporarily disable keyboard input during serialization.
        keyboard_disable();
        bool saveFailed = false;

        // 2. CHANGE DETECTION
        // Continue only when encoder save engine is active and dirty flag is set.
        if ((encoderPresetSaveActive) && (encoderPresetSaveFlag)) {
            // 3. GLOBAL MAPPING PACKAGING
            // Flatten global encoder matrix into file payload array.
            for (uint8_t i = 0; i < kEncoderBankSize; i++) {
                for (uint8_t j = 0; j < kEncoderSize; j++) {
                    dataEncoderGlobalPreset[(i * kEncoderSize) + j] = (uint8_t)encoderGlobalFunction[i][j];
                }
            }

            // 4. LOCAL MAPPING PACKAGING
            // Flatten local menu-specific encoder mappings into payload array.
            const uint8_t localMenuCount = sizeof(encoderLocalFunction) / sizeof(encoderLocalFunction[0]);
            const uint8_t localStoredEncoderCount = kEncoderLocalPresetDataSize / localMenuCount;
            for (uint8_t i = 0; i < localMenuCount; i++) {
                // Local preset binary layout stores a compact encoder subset per menu.
                for (uint8_t j = 0; (j < localStoredEncoderCount) && (j < kEncoderSize); j++) {
                    dataEncoderLocalPreset[(i * localStoredEncoderCount) + j] = (uint8_t)encoderLocalFunction[i][j];
                }
            }

            // 5. FILE SYSTEM COMMIT
            // Write serialized global/local mapping payloads to SD file.
            // A failed write keeps the dirty flag set so the next save cycle retries.
            if (f_open(&sd.file, "System/Preset/Encoder.rwp", FA_READ | FA_WRITE) == FR_OK) {
                // Write Global data at offset 25 (directly after the 25-char header)
                if ((f_lseek(&sd.file, 25) != FR_OK) || (sd_write(dataEncoderGlobalPreset, kEncoderGlobalPresetDataSize) != FR_OK)) {
                    saveFailed = true;
                }

                // Write Local data at offset 67
                if ((f_lseek(&sd.file, 67) != FR_OK) || (sd_write(dataEncoderLocalPreset, kEncoderLocalPresetDataSize) != FR_OK)) {
                    saveFailed = true;
                }

                // Finalize FAT32 write sequence and release file handle.
                // f_close flushes the cached sector; its result is part of the write.
                if (f_close(&sd.file) != FR_OK) saveFailed = true;
            } else {
                saveFailed = true;
            }
        }

        // 6. CLEANUP & STATE RESET
        // Clear save flag (unless the write failed) and restore keyboard input handling.
        encoderPresetSaveFlag = saveFailed;  // Reset the "dirty" flag only on success
        keyboard_enable();              // Restore user interaction
    }
}

////////////////////////////////////////////////////////////////////////////////
/* File Functions ------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::file_select() {
    /// @brief Switches to File menu and prepares UI/encoder context for file operations.

    if (menu != FILE_MENU) {
        // 1. MENU STATE TRANSITION
        // Store previous menu context and activate FILE_MENU tab root.
        preMenu = menu;
        menu = FILE_MENU;
        preMenuTab = menuTab;
        menuTab = 0;

        // 2. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 3. UI TRANSITION & RENDER
        // Animate menu change and render file screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawFileMenu();

        // 4. ENCODER CONTEXT UPDATE
        // Rebind encoders to File menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::file_menuRight() {
    /// @brief Advances File menu tab to the right with wrap-around and redraws tab content.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    (menuTab < kMaxMenu4Tab) ? menuTab += 1 : menuTab = kMinMenu4Tab;

    // 2. FILE SLOT RESET & STATUS CHECK
    // Start at the slot of the current project (v2.01; slot 1 when there is none).
    fileMenuCounter = (currentFileSlot >= 0) ? currentFileSlot : 0;
    sd_checkFile(fileMenuCounter);

    // 3. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();

    // 4. TAB CONTENT REDRAW
    // Redraw tab-specific file boxes and text payloads.
    // Tab map: 0=New, 1=Load, 2=Save, 3=Clear.
    switch (menuTab) {
        case 0:
            lcd_clearFileBox(3);
            lcd_drawFile_NewData();
            lcd_drawFile_ClearData();
            break;

        case 1:
            lcd_drawFileBox(1, fileStatus);
            lcd_drawFile_NewData();
            lcd_drawFile_LoadData();
            break;

        case 2:
            lcd_clearFileBox(1);
            lcd_drawFileBox(2, fileStatus);
            lcd_drawFile_LoadData();
            lcd_drawFile_SaveData();
            break;

        case 3:
            lcd_clearFileBox(2);
            lcd_drawFileBox(3, fileStatus);
            lcd_drawFile_SaveData();
            lcd_drawFile_ClearData();
            break;
    }
}

void Controller::file_menuLeft() {
    /// @brief Moves File menu tab to the left with wrap-around and redraws tab content.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    (menuTab > kMinMenu4Tab) ? menuTab -= 1 : menuTab = kMaxMenu4Tab;

    // 2. FILE SLOT RESET & STATUS CHECK
    // Start at the slot of the current project (v2.01; slot 1 when there is none).
    fileMenuCounter = (currentFileSlot >= 0) ? currentFileSlot : 0;
    sd_checkFile(fileMenuCounter);

    // 3. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();

    // 4. TAB CONTENT REDRAW
    // Redraw tab-specific file boxes and text payloads.
    // Tab map: 0=New, 1=Load, 2=Save, 3=Clear.
    switch (menuTab) {
        case 0:
            lcd_clearFileBox(1);
            lcd_drawFile_NewData();
            lcd_drawFile_LoadData();
            break;

        case 1:
            lcd_clearFileBox(2);
            lcd_drawFileBox(1, fileStatus);
            lcd_drawFile_LoadData();
            lcd_drawFile_SaveData();
            break;

        case 2:
            lcd_clearFileBox(3);
            lcd_drawFileBox(2, fileStatus);
            lcd_drawFile_SaveData();
            lcd_drawFile_ClearData();
            break;

        case 3:
            lcd_drawFileBox(3, fileStatus);
            lcd_drawFile_NewData();
            lcd_drawFile_ClearData();
            break;
    }
}

void Controller::file_menuUp() {
    /// @brief Increments file slot selection with wrap-around and refreshes active tab panel.

    // 1. FILE SLOT NAVIGATION
    // Increment slot pointer and wrap after reaching last index.
    (fileMenuCounter < kMaxFile) ? fileMenuCounter += 1 : fileMenuCounter = kMinFile;

    // 2. SLOT STATUS VALIDATION
    // Recompute slot state (active/inactive/empty/corrupt) for current index.
    sd_checkFile(fileMenuCounter);

    // 3. TAB-SPECIFIC REDRAW
    // Refresh visible data block for currently selected File tab.
    // Tab 0 has no indexed file box, so no slot-box redraw is required.
    switch (menuTab) {
        case 0:
            break;

        case 1:
            lcd_drawFileBox(menuTab, fileStatus);
            lcd_drawFile_LoadData();
            break;

        case 2:
            lcd_drawFileBox(menuTab, fileStatus);
            lcd_drawFile_SaveData();
            break;

        case 3:
            lcd_drawFileBox(menuTab, fileStatus);
            lcd_drawFile_ClearData();
            break;
    }
}

void Controller::file_menuDown() {
    /// @brief Decrements file slot selection with wrap-around and refreshes active tab panel.

    // 1. FILE SLOT NAVIGATION
    // Decrement slot pointer and wrap before first index.
    (fileMenuCounter > kMinFile) ? fileMenuCounter -= 1 : fileMenuCounter = kMaxFile;

    // 2. SLOT STATUS VALIDATION
    // Recompute slot state (active/inactive/empty/corrupt) for current index.
    sd_checkFile(fileMenuCounter);

    // 3. TAB-SPECIFIC REDRAW
    // Refresh visible data block for currently selected File tab.
    // Tab 0 has no indexed file box, so no slot-box redraw is required.
    switch (menuTab) {
        case 0:
            break;

        case 1:
            lcd_drawFileBox(menuTab, fileStatus);
            lcd_drawFile_LoadData();
            break;

        case 2:
            lcd_drawFileBox(menuTab, fileStatus);
            lcd_drawFile_SaveData();
            break;

        case 3:
            lcd_drawFileBox(menuTab, fileStatus);
            lcd_drawFile_ClearData();
            break;
    }
}

void Controller::file_newSelect() {
    /// @brief Opens confirmation alert for creating a new file state.

    // 1. ALERT STATE CONFIGURATION
    // Arm alert subsystem for New File confirmation flow.
    alertFlag = true;
    alertType = ALERT_NEWFILE;

    // 2. ALERT RENDER
    // Draw confirmation overlay on top of current menu.
    lcd_drawAlert();
}

void Controller::file_newAction() {
    /// @brief Executes New File flow by resetting layer/song/modulation/system runtime state.

    // 1. ALERT CLEANUP
    // Remove pending alert before executing state reset flow.
    lcd_clearAlert();

    // 2. TRANSPORT RESET
    // Reset song transport and playback runtime state.
    reset();

    // 3. BANK STATE RESET
    // Return active song bank to default and redraw bank tab indicator.
    activeSongBank = 0;
    lcd_drawSongBankTab(activeSongBank, false);

    // 4. LAYER & SONG RESET
    // Reinitialize all layer instruments and per-bank song data.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        layerInst_reset(i);
        for (uint8_t j = 0; j < kBankLibrarySize; j++) {
            layerSong_reset(i, j);
        }
    }

    // 5. MODULATION RESET
    // Reset all LFO instances to default state.
    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
        lfo_reset(i);
    }

    // 6. GLOBAL DSP MODULE RESET
    // Reset rhythm, EQ, filters, effects, and reverb blocks.
    rhythm_reset();
    songChain = songchain::defaultChain();  // song mode OFF, empty chain
    tone_resetAll();                        // every layer a plain DRUM layer again
    songEditStep = 0;
    song_rewind();
    eq_reset();
    filter_reset(0);
    filter_reset(1);
    effect_reset(0);
    effect_reset(1);
    reverb_reset();

    // 7. A new project belongs to no slot yet (v2.01).
    currentFileSlot = -1;
}

void Controller::file_loadSelect() {
    /// @brief Opens load confirmation alert when the selected slot is active.

    // 1. PRECONDITION CHECK
    // Show load confirmation only for physically active file slots.
    if (fileStatus == FILE_ACTIVE) {
        // 2. ALERT STATE CONFIGURATION
        // Configure alert type and enable alert rendering.
        alertFlag = true;
        alertType = ALERT_LOADFILE;

        // 3. ALERT RENDER
        // Draw confirmation overlay on top of current menu.
        lcd_drawAlert();
    }
}

void Controller::file_loadAction() {
    /// @brief Loads selected file from SD and maps result to user-visible alert state.

    // 1. PRE-LOAD CLEANUP
    // Clear existing alerts and reset runtime playback context.
    lcd_clearAlert();
    reset();
    lcd_resetPlay();

    // 2. LOAD TRANSACTION WINDOW
    // Mark that load workflow is in progress for dependent logic.
    loadAction = true;

    // 3. SD LOAD EXECUTION & RESULT MAPPING
    // Execute SD load and map result code to alert outcome.
    switch (sd_loadFile(fileMenuCounter)) {
        case SD_OK:

            // File content and linked resources loaded successfully.
            alertType = ALERT_LOADSUCCESS;
            break;

        case SD_ERROR:

            // Generic load failure (format/read/validation level).
            alertType = ALERT_LOADERROR;
            break;

        case SD_ERROR_DETECT:

            // Load succeeded partially but one or more samples are missing.
            alertType = ALERT_MISSINGSAMPLE;
            break;
    }

    // 4. TRANSACTION FLAG RESTORE
    // Release load-in-progress flag after SD transaction completes.
    loadAction = false;
    currentFileSlot = fileMenuCounter;  // v2.01: SAVE and AUTOSAVE now belong to this slot

    // 5. RESULT ALERT PRESENTATION
    // Display result alert and keep it visible for user acknowledgment.
    lcd_drawAlert();

    // Keep result visible long enough to be read by user.
    HAL_Delay(1000);

    // 6. ALERT CLEANUP
    // Remove result alert and return to normal menu rendering.
    lcd_clearAlert();
}

void Controller::file_saveSelect() {
    /// @brief Opens save/overwrite confirmation alert based on selected slot occupancy.

    // 1. ALERT STATE CONFIGURATION
    // Select Save or Overwrite prompt based on slot occupancy.
    // Existing slot -> overwrite warning, empty slot -> normal save prompt.
    alertFlag = true;
    (fileStatus == FILE_ACTIVE) ? alertType = ALERT_OVERWRITEFILE : alertType = ALERT_SAVEFILE;

    // 2. ALERT RENDER
    // Draw save confirmation overlay.
    lcd_drawAlert();
}

void Controller::file_saveAction() {
    /// @brief Saves current runtime state to selected slot and refreshes file status/library data.

    // 1. PRE-SAVE CLEANUP
    // Clear confirmation alert before save transaction.
    lcd_clearAlert();

    // 2. SAVE TRANSACTION WINDOW
    // Mark save workflow active for UI/logic synchronization.
    saveAction = true;

    // 3. SD SAVE EXECUTION & RESULT MAPPING
    // Execute SD save and map operation result to alert type.
    (sd_saveFile(fileMenuCounter) == SD_OK) ? alertType = ALERT_SAVESUCCESS : alertType = ALERT_SAVEERROR;

    // 4. TRANSACTION FLAG RESTORE
    // Release save-in-progress flag after SD transaction completes.
    saveAction = false;
    if (alertType == ALERT_SAVESUCCESS) currentFileSlot = fileMenuCounter;  // v2.01

    // 5. STATUS & LIBRARY REFRESH
    // Re-evaluate slot status and refresh indexed file library cache.
    sd_checkFile(fileMenuCounter);
    lcd_drawFileBox(menuTab, fileStatus);
    sd_getFileLibrary();

    // 6. RESULT ALERT PRESENTATION
    // Display operation result to user for a short interval.
    lcd_drawAlert();

    // Keep result visible long enough to be read by user.
    HAL_Delay(1000);

    // 7. ALERT CLEANUP
    // Remove result alert and resume standard UI flow.
    lcd_clearAlert();
}

void Controller::file_clearSelect() {
    /// @brief Opens clear confirmation alert when selected slot is not inactive.

    // 1. PRECONDITION CHECK
    // Show clear confirmation only for non-empty/non-inactive slots.
    if (fileStatus != FILE_INACTIVE) {
        // 2. ALERT STATE CONFIGURATION
        // Configure clear action prompt and enable alert rendering.
        alertFlag = true;
        alertType = ALERT_CLEARFILE;

        // 3. ALERT RENDER
        // Draw clear confirmation overlay.
        lcd_drawAlert();
    }
}

void Controller::file_clearAction() {
    /// @brief Clears selected file slot on SD and updates status/library UI state.

    // 1. PRE-CLEAR CLEANUP
    // Clear confirmation alert before SD delete transaction.
    lcd_clearAlert();

    // 2. SD CLEAR EXECUTION & RESULT MAPPING
    // Execute slot clear on SD and map result to alert type.
    (sd_clearFile(fileMenuCounter) == SD_OK) ? alertType = ALERT_CLEARSUCCESS : alertType = ALERT_CLEARERROR;

    // 3. STATUS & LIBRARY REFRESH
    // Re-evaluate slot status and refresh indexed file library cache.
    sd_checkFile(fileMenuCounter);
    lcd_drawFileBox(menuTab, fileStatus);
    sd_getFileLibrary();

    // 4. RESULT ALERT PRESENTATION
    // Display operation result to user for a short interval.
    lcd_drawAlert();

    // Keep result visible long enough to be read by user.
    HAL_Delay(1000);

    // 5. ALERT CLEANUP
    // Remove result alert and resume standard UI flow.
    lcd_clearAlert();
}

////////////////////////////////////////////////////////////////////////////////
/* Drumkit Functions ---------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::drumkit_select() {
    /// @brief Switches to Drumkit menu and prepares UI/encoder context for drumkit operations.
    if (menu != DRUMKIT_MENU) {
        // 1. MENU STATE TRANSITION
        // Store previous menu context and activate DRUMKIT_MENU tab root.
        preMenu = menu;
        menu = DRUMKIT_MENU;
        preMenuTab = menuTab;
        menuTab = 0;

        // 2. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 3. UI TRANSITION & RENDER
        // Animate menu change and render drumkit screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawDrumkitMenu();

        // 4. ENCODER CONTEXT UPDATE
        // Rebind encoders to Drumkit menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::drumkit_menuRight() {
    /// @brief Advances Drumkit menu tab to the right with wrap-around and redraws tab content.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    (menuTab < kMaxMenu4Tab) ? menuTab += 1 : menuTab = kMinMenu4Tab;

    // 2. DRUMKIT SLOT RESET & STATUS CHECK
    // Reset slot cursor and evaluate selected drumkit slot status.
    drumkitMenuCounter = 0;
    sd_checkDrumkit(drumkitMenuCounter);

    // 3. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();

    // 4. TAB CONTENT REDRAW
    // Redraw tab-specific drumkit file boxes and text payloads.
    // Tab map: 0=New, 1=Load, 2=Save, 3=Clear.
    switch (menuTab) {
        case 0:
            lcd_clearFileBox(3);
            lcd_drawDrumkit_NewData();
            lcd_drawDrumkit_ClearData();
            break;

        case 1:
            lcd_drawFileBox(1, drumkitStatus);
            lcd_drawDrumkit_NewData();
            lcd_drawDrumkit_LoadData();
            break;

        case 2:
            lcd_clearFileBox(1);
            lcd_drawFileBox(2, drumkitStatus);
            lcd_drawDrumkit_LoadData();
            lcd_drawDrumkit_SaveData();
            break;

        case 3:
            lcd_clearFileBox(2);
            lcd_drawFileBox(3, drumkitStatus);
            lcd_drawDrumkit_SaveData();
            lcd_drawDrumkit_ClearData();
            break;
    }
}

void Controller::drumkit_menuLeft() {
    /// @brief Moves Drumkit menu tab to the left with wrap-around and redraws tab content.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    (menuTab > kMinMenu4Tab) ? menuTab -= 1 : menuTab = kMaxMenu4Tab;

    // 2. DRUMKIT SLOT RESET & STATUS CHECK
    // Reset slot cursor and evaluate selected drumkit slot status.
    drumkitMenuCounter = 0;
    sd_checkDrumkit(drumkitMenuCounter);

    // 3. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();

    // 4. TAB CONTENT REDRAW
    // Redraw tab-specific drumkit file boxes and text payloads.
    // Tab map: 0=New, 1=Load, 2=Save, 3=Clear.
    switch (menuTab) {
        case 0:
            lcd_clearFileBox(1);
            lcd_drawDrumkit_NewData();
            lcd_drawDrumkit_LoadData();
            break;

        case 1:
            lcd_clearFileBox(2);
            lcd_drawFileBox(1, drumkitStatus);
            lcd_drawDrumkit_LoadData();
            lcd_drawDrumkit_SaveData();
            break;

        case 2:
            lcd_clearFileBox(3);
            lcd_drawFileBox(2, drumkitStatus);
            lcd_drawDrumkit_SaveData();
            lcd_drawDrumkit_ClearData();
            break;

        case 3:
            lcd_drawFileBox(3, drumkitStatus);
            lcd_drawDrumkit_NewData();
            lcd_drawDrumkit_ClearData();
            break;
    }
}

void Controller::drumkit_menuUp() {
    /// @brief Increments drumkit slot selection with wrap-around and refreshes active tab panel.

    // 1. DRUMKIT SLOT NAVIGATION
    // Increment slot pointer and wrap after reaching last index.
    (drumkitMenuCounter < kMaxDrumkit) ? drumkitMenuCounter += 1 : drumkitMenuCounter = kMinFile;

    // 2. SLOT STATUS VALIDATION
    // Recompute slot state (active/inactive/empty/corrupt) for current index.
    sd_checkDrumkit(drumkitMenuCounter);

    // 3. TAB-SPECIFIC REDRAW
    // Refresh visible data block for currently selected Drumkit tab.
    // Tab 0 has no indexed drumkit box, so no slot-box redraw is required.
    switch (menuTab) {
        case 0:
            break;

        case 1:
            lcd_drawFileBox(menuTab, drumkitStatus);
            lcd_drawDrumkit_LoadData();
            break;

        case 2:
            lcd_drawFileBox(menuTab, drumkitStatus);
            lcd_drawDrumkit_SaveData();
            break;

        case 3:
            lcd_drawFileBox(menuTab, drumkitStatus);
            lcd_drawDrumkit_ClearData();
            break;
    }
}

void Controller::drumkit_menuDown() {
    /// @brief Decrements drumkit slot selection with wrap-around and refreshes active tab panel.

    // 1. DRUMKIT SLOT NAVIGATION
    // Decrement slot pointer and wrap before first index.
    (drumkitMenuCounter > kMinDrumkit) ? drumkitMenuCounter -= 1 : drumkitMenuCounter = kMaxFile;

    // 2. SLOT STATUS VALIDATION
    // Recompute slot state (active/inactive/empty/corrupt) for current index.
    sd_checkDrumkit(drumkitMenuCounter);

    // 3. TAB-SPECIFIC REDRAW
    // Refresh visible data block for currently selected Drumkit tab.
    // Tab 0 has no indexed drumkit box, so no slot-box redraw is required.
    switch (menuTab) {
        case 0:
            break;

        case 1:
            lcd_drawFileBox(menuTab, drumkitStatus);
            lcd_drawDrumkit_LoadData();
            break;

        case 2:
            lcd_drawFileBox(menuTab, drumkitStatus);
            lcd_drawDrumkit_SaveData();
            break;

        case 3:
            lcd_drawFileBox(menuTab, drumkitStatus);
            lcd_drawDrumkit_ClearData();
            break;
    }
}

void Controller::drumkit_newSelect() {
    /// @brief Opens confirmation alert for creating a new drumkit state.

    // 1. ALERT STATE CONFIGURATION
    // Arm alert subsystem for New Drumkit confirmation flow.
    alertFlag = true;
    alertType = ALERT_NEWDRUMKIT;

    // 2. ALERT RENDER
    // Draw confirmation overlay on top of current menu.
    lcd_drawAlert();
}

void Controller::drumkit_newAction() {
    /// @brief Executes New Drumkit flow by resetting transport, layers, LFOs, and DSP modules.

    // 1. ALERT CLEANUP
    // Remove pending alert before executing state reset flow.
    lcd_clearAlert();

    // 2. TRANSPORT RESET
    // Reset song transport and playback runtime state.
    reset();

    // 3. LAYER RESET
    // Reinitialize all layer instruments to default state.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        layerInst_reset(i);
    }

    // 4. MODULATION RESET
    // Reset all LFO instances to default state.
    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
        lfo_reset(i);
    }

    // 5. GLOBAL DSP MODULE RESET
    // Reset EQ, filters, effects, and reverb blocks.
    eq_reset();
    filter_reset(0);
    filter_reset(1);
    effect_reset(0);
    effect_reset(1);
    reverb_reset();
}

void Controller::drumkit_loadSelect() {
    /// @brief Opens load confirmation alert when the selected drumkit slot is active.

    // 1. PRECONDITION CHECK
    // Show load confirmation only for physically active drumkit slots.
    if (drumkitStatus == FILE_ACTIVE) {
        // 2. ALERT STATE CONFIGURATION
        // Configure alert type and enable alert rendering.
        alertFlag = true;
        alertType = ALERT_LOADDRUMKIT;

        // 3. ALERT RENDER
        // Draw confirmation overlay on top of current menu.
        lcd_drawAlert();
    }
}

void Controller::drumkit_loadAction() {
    /// @brief Loads selected drumkit from SD and maps result to user-visible alert state.

    // 1. PRE-LOAD CLEANUP
    // Clear existing alerts and reset runtime playback context.
    lcd_clearAlert();
    reset();
    lcd_resetPlay();

    // 2. LOAD TRANSACTION WINDOW
    // Mark that load workflow is in progress for dependent logic.
    loadAction = true;

    // 3. SD LOAD EXECUTION & RESULT MAPPING
    // Execute SD load and map result code to alert outcome.
    switch (sd_loadDrumkit(drumkitMenuCounter)) {
        case SD_OK:

            // Drumkit content and linked resources loaded successfully.
            alertType = ALERT_LOADSUCCESS;
            break;

        case SD_ERROR:

            // Generic load failure (format/read/validation level).
            alertType = ALERT_LOADERROR;
            break;

        case SD_ERROR_DETECT:

            // Load succeeded partially but one or more samples are missing.
            alertType = ALERT_MISSINGSAMPLE;
            break;
    }

    // 4. TRANSACTION FLAG RESTORE
    // Release load-in-progress flag after SD transaction completes.
    loadAction = false;

    // 5. RESULT ALERT PRESENTATION
    // Display result alert and keep it visible for user acknowledgment.
    lcd_drawAlert();

    // Keep result visible long enough to be read by user.
    HAL_Delay(1000);

    // 6. ALERT CLEANUP
    // Remove result alert and return to normal menu rendering.
    lcd_clearAlert();
}

void Controller::drumkit_saveSelect() {
    /// @brief Opens save/overwrite confirmation alert based on selected drumkit slot occupancy.

    // 1. ALERT STATE CONFIGURATION
    // Select Save or Overwrite prompt based on slot occupancy.
    // Existing slot -> overwrite warning, empty slot -> normal save prompt.
    alertFlag = true;
    (drumkitStatus == FILE_ACTIVE) ? alertType = ALERT_OVERWRITEDRUMKIT : alertType = ALERT_SAVEDRUMKIT;

    // 2. ALERT RENDER
    // Draw save confirmation overlay.
    lcd_drawAlert();
}

void Controller::drumkit_saveAction() {
    /// @brief Saves current drumkit state to selected slot and refreshes status/library data.

    // 1. PRE-SAVE CLEANUP
    // Clear confirmation alert before save transaction.
    lcd_clearAlert();

    // 2. SAVE TRANSACTION WINDOW
    // Mark save workflow active for UI/logic synchronization.
    saveAction = true;

    // 3. SD SAVE EXECUTION & RESULT MAPPING
    // Execute SD save and map operation result to alert type.
    (sd_saveDrumkit(drumkitMenuCounter) == SD_OK) ? alertType = ALERT_SAVESUCCESS : alertType = ALERT_SAVEERROR;

    // 4. TRANSACTION FLAG RESTORE
    // Release save-in-progress flag after SD transaction completes.
    saveAction = false;

    // 5. STATUS & LIBRARY REFRESH
    // Re-evaluate slot status and refresh indexed drumkit library cache.
    sd_checkDrumkit(drumkitMenuCounter);
    lcd_drawFileBox(menuTab, drumkitStatus);
    sd_getDrumkitLibrary();

    // 6. RESULT ALERT PRESENTATION
    // Display operation result to user for a short interval.
    lcd_drawAlert();

    // Keep result visible long enough to be read by user.
    HAL_Delay(1000);

    // 7. ALERT CLEANUP
    // Remove result alert and resume standard UI flow.
    lcd_clearAlert();
}

void Controller::drumkit_clearSelect() {
    /// @brief Opens clear confirmation alert when selected drumkit slot is not inactive.

    // 1. PRECONDITION CHECK
    // Show clear confirmation only for non-empty/non-inactive slots.
    if (drumkitStatus != FILE_INACTIVE) {
        // 2. ALERT STATE CONFIGURATION
        // Configure clear action prompt and enable alert rendering.
        alertFlag = true;
        alertType = ALERT_CLEARDRUMKIT;

        // 3. ALERT RENDER
        // Draw clear confirmation overlay.
        lcd_drawAlert();
    }
}

void Controller::drumkit_clearAction() {
    /// @brief Clears selected drumkit slot on SD and updates status/library UI state.

    // 1. PRE-CLEAR CLEANUP
    // Clear confirmation alert before SD delete transaction.
    lcd_clearAlert();

    // 2. SD CLEAR EXECUTION & RESULT MAPPING
    // Execute slot clear on SD and map result to alert type.
    (sd_clearDrumkit(drumkitMenuCounter) == SD_OK) ? alertType = ALERT_CLEARSUCCESS : alertType = ALERT_CLEARERROR;

    // 3. STATUS & LIBRARY REFRESH
    // Re-evaluate slot status and refresh indexed drumkit library cache.
    sd_checkDrumkit(drumkitMenuCounter);
    lcd_drawFileBox(menuTab, drumkitStatus);
    sd_getDrumkitLibrary();

    // 4. RESULT ALERT PRESENTATION
    // Display operation result to user for a short interval.
    lcd_drawAlert();

    // Keep result visible long enough to be read by user.
    HAL_Delay(1000);

    // 5. ALERT CLEANUP
    // Remove result alert and resume standard UI flow.
    lcd_clearAlert();
}

