#include "Controller.h"

Controller::Controller()

    /* --- SECTION 1: HARDWARE PERIPHERALS & DSP MODULES -------------------- */

    : lcd(LANDSCAPE_1, WHITE, BLACK, FONT_07x09),
      dac(),
      sd(),
      keyboard(),
      ledEqLeft(),
      ledEqRight(),
      ledLayer(),
      rhythm(),
      metronome(),
      eq(),
      filter{{0}, {1}},
      effect{{0}, {1}},
      reverb(),
      testMode(TEST_NONE),

      /* --- SECTION 2: AUDIO BUFFERS (BLOCK PROCESSING) -------------------- */

      audioReceive{0.0f},
      audioMetronome{0.0f},
      audioLayer{0.0f},
      audioEq{0.0f},
      audioFilter{0.0f},
      audioEffect{0.0f},
      audioReverb_L{0.0f},
      audioReverb_R{0.0f},
      audioSend_L{0.0f},
      audioSend_R{0.0f},
      audioPeak_L(0.0f),
      audioPeak_R(0.0f),

      /* --- SECTION 3: DATA PRESETS & LIBRARIES ---------------------------- */

      dataSystemPreset{0},
      dataEncoderGlobalPreset{0},
      dataEncoderLocalPreset{0},
      lfoLibrary{{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}},
      layerLibrary{{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}},

      /* --- SECTION 4: NAVIGATION & UI SELECTION --------------------------- */

      selectedLayerNum(-1),
      selectedBeatNum(-1),
      selectedLfoNum(-1),
      activeEncoderBank(kInitialEncoderBank),
      activeSongBank(kInitialSongBank),
      targetSongBank(kInitialSongBank),
      bankShiftFlag(false),
      bankActionFlag(false),
      menu(INIT_MENU),
      preMenu(INIT_MENU),
      menuTab(-1),
      preMenuTab(-1),

      /* --- SECTION 5: SEQUENCER TIMING & INTERVALS ------------------------ */

      preBeatFillType(-1),
      preBeatFillPattern(-1),
      preBeatFillTime(-1),
      preBeatFillLevel(-1),
      fileLibrarySize(0),
      drumkitLibrarySize(0),
      instLibrarySize(0),
      sampleLibrarySize{},
      totalSampleLibrarySize(0),
      songInterval(kMeasureInterval * kInitialMeasure * kInitialBar),
      barInterval(kMeasureInterval * kInitialMeasure),
      measureInterval(kMeasureInterval),
      playInterval(0),
      stopInterval(0),
      resetInterval(0),
      stopFlag(false),
      resetFlag(false),

      /* --- SECTION 6: SYSTEM FLAGS & STATUS ------------------------------- */

      alertFlag(false),
      alertType(ALERT_OFF),
      sdInsertCheck(false),
      loadAction(false),
      saveAction(false),
      resetPlayFlag(false),
      fileStatus(FILE_NONE),
      drumkitStatus(FILE_NONE),
      playActive(false),
      recordActive(false),
      precountPendingBeat{false},
    ignoreNextMidiStopAfterSpecialCmd(false),
      triggerMode(TRIG_BAR),
      playTimerPeriod(0),
      measureTimerPeriod(0),
      midiClockPeriod(0),

      /* --- SECTION 7: UI VISUALS & ACTION FLAGS --------------------------- */

      playX(0),
      playXRatio((float)songInterval / kPlayWidth),
      playColor(kPlayColor0),
      // Clipboard and Command Logic Flags
      lfoCopyKeyFlag(false),
      lfoCopyActionFlag(false),
      lfoPasteKeyFlag(false),
      lfoPasteActionFlag(false),
      lfoResetKeyFlag(false),
      lfoResetActionFlag(false),
      layerInstCopyKeyFlag(false),
      layerInstCopyActionFlag(false),
      layerInstPasteKeyFlag(false),
      layerInstPasteActionFlag(false),
      layerInstResetKeyFlag(false),
      layerInstResetActionFlag(false),
      layerSongCopyKeyFlag(false),
      layerSongCopyActionFlag(false),
      layerSongPasteKeyFlag(false),
      layerSongPasteActionFlag(false),
      layerSongResetKeyFlag(false),
      layerSongResetActionFlag(false),
      layerBeatKeyFlag(false),
      layerSongAddPending(false),
      layerFillCopyKeyFlag(false),
      layerFillPasteKeyFlag(false),
      // Logic Control Flags (Rhythm, Mixer, UI)
      rhythmUnlockKeyFlag(false),
      rhythmUnlockActionFlag(false),
      rhythmLockKeyFlag(false),
      rhythmLockActionFlag(false),
      masterUnmuteKeyFlag(false),
      masterUnmuteActionFlag(false),
      masterMuteKeyFlag(false),
      masterMuteActionFlag(false),
      hpMuteKeyFlag(false),
      hpMuteActionFlag(false),
      hpUnmuteKeyFlag(false),
      hpUnmuteActionFlag(false),
      loMuteKeyFlag(false),
      loMuteActionFlag(false),
      loUnmuteKeyFlag(false),
      loUnmuteActionFlag(false),
      liMuteKeyFlag(false),
      liMuteActionFlag(false),
      liUnmuteKeyFlag(false),
      liUnmuteActionFlag(false),
      // Text and Alert UI Flags
      textCopyFlag(false),
      textPasteFlag(false),
      textClearFlag(false),
      textArmedFlag(false),
      limitAlertShowFlag(false),
      limitAlertClearFlag(false),
      limitAlertActive(false),
      systemPresetSaveFlag(false),
      systemPresetSaveActive(false),
      encoderPresetSaveFlag(false),
      encoderPresetSaveActive(false),
      // Menu Indexing and Animation Transitions
      fileMenuCounter(-1),
      drumkitMenuCounter(-1),
      transitionShowFlag(0),
      transitionClearFlag(false),
      transitionShowMenu(false),
      transitionClearMenu(false),
      transitionShowEncoder{false},
      transitionClearEncoder{false},
      layerBeatKeyStage(0),

      /* --- SECTION 8: INPUT & LED ENGINE STATE ---------------------------- */

      keyboardActive(false),
      encoderActive(false),
      encoderFastMode(false),
      encoderSlowMode(false),
      upDownKeyLongActive(true),
      ledEqPlayMode(false),
      ledLayerPlayMode(false),
      ledEqReadFlag{false},
      ledEqDrawFlag{false},
      ledEqReadData{0.0f},
      ledEqDrawData{0},
      ledLayerSelectFlag(false),
      ledLayerSelectData(0x00),
      ledLayerPlayFlag(false),
      ledLayerPlayData(0x00),
      ledEqCurrentLevel{0.0f},
      ledEqLastSentData{0},
      ledEqPriorityChannel(0),

      /* --- SECTION 9: MISCELLANEOUS --------------------------------------- */

      animation(true),
      textShow(false),
      sampleFadeOut(true) {}

Controller::~Controller() {}

void Controller::initialize() {
    /// @brief Orchestrates the complete cold-boot sequence.
    /// Initializes DSP, peripherals, and determines the operating mode (Normal vs Test).

    // 1. CORE TIMING & DSP CHAIN INITIALIZATION
    // Initialize timing base and DSP modules with startup defaults.
    dwt_initialize();  // High-resolution microsecond timing for profiling

    // Initialize individual DSP modules to default states
    eq.initialize();
    filter[0].initialize();
    filter[1].initialize();
    effect[0].initialize();
    effect[1].initialize();
    reverb.initialize();

    // Reset specific filter parameters and set master Low-Pass Filters (LPF)
    filter_reset(0);
    filter_reset(1);
    lpf[0].initialize(true, 8500, 0.00, 1.00);  // Primary anti-aliasing/tone LPF
    lpf[1].initialize(true, 8500, 0.00, 1.00);

    // Set initial Delay settings (Dry/Wet) for clean signal startup
    effect_setEData(0, EF_DELAY, kInitialDelayDry);
    effect_setFData(0, EF_DELAY, kInitialDelayWet);
    effect_setEData(1, EF_DELAY, kInitialDelayDry);
    effect_setFData(1, EF_DELAY, kInitialDelayWet);

    // 2. PERIPHERAL HARDWARE ACTIVATION
    // Power up display and audio output peripherals.
    dac_initialize();  // Setup Digital-to-Audio Converter
    lcd_initialize();  // Setup Display Controller
    lcd.displayOn();   // Backlight and pixels active
    dac.audioOn();     // Release amplifier/DAC mute circuits

    // 3. TIMER STATE SYNCHRONIZATION
    // Reset internal timers to known stopped states.
    startTransitionTimer();
    stopTransitionTimer();

    startTextTimer();
    stopTextTimer();

    startEqTimer();
    stopEqTimer();

    startBeatSyncTimer();
    stopBeatSyncTimer();

    startLimitAlertTimer();
    stopLimitAlertTimer();

    startSystemPresetTimer();
    stopSystemPresetTimer();

    startEncoderPresetTimer();
    stopEncoderPresetTimer();

    // Turn on status indicators to show the system is alive
    LED0_ON;
    LED1_ON;
    LED2_ON;

    // Initialize SD card storage interface
    sd_initialize();

    // 4. BOOT MODE SELECTION (TEST VS NORMAL)
    // Branch startup flow according to test button state.
    if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
        // 1. Enter Diagnostic State
        menu = TEST_MENU;
        testMode = TEST_SCREEN;
        lcd_test(testMode);  // Draw hardware test patterns

        // 2. Hardware Verification: Full LED Pulse
        // Forces all LEDs on to check for dead segments/solder bridges
        led_initialize(false);
        uint16_t testData = 0x0FFF;  // Bitmask for EQ LEDs
        i2c4_txData[0] = testData & 0xFF;
        i2c4_txData[1] = (testData >> 8) & 0xFF;
        HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);
        HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);

        testData = 0x03FF;  // Bitmask for Layer LEDs
        i2c4_txData[0] = testData & 0xFF;
        i2c4_txData[1] = (testData >> 8) & 0xFF;
        HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);

        // 3. Start Test Mode Clock
        updatePlayTimerPeriod();
        startPlayTimer();
        startMidiClockTimer();

        // 4. Enable Communication for Testing
        HAL_HalfDuplex_EnableReceiver(&huart1);
        HAL_HalfDuplex_EnableTransmitter(&huart6);
        HAL_UART_Receive_IT(&huart1, &midiRxData, 1);
    }

    // 5. NORMAL SYSTEM BOOT
    // Execute regular user-mode startup flow.
    else {
        // 1. Visual Branding & Filesystem Loading
        // The edition animation runs inside the existing 1 s post-logo pause, so boot time
        // does not change.
        lcd_drawLogo(animation);
        const uint32_t brandingStart = HAL_GetTick();
        lcd_drawEdition(animation);
        if (animation) {
            const uint32_t spent = HAL_GetTick() - brandingStart;
            if (spent < 1000) HAL_Delay(1000 - spent);
        }

        lcd_drawSdDataIntro();
        sd_getLibraries();  // Load instrument/sample lists from SD Card
        lcd_drawSdData();
        if (animation) HAL_Delay(2000);

        // Clean up UI and transition to main interface
        lcd_clearSdData();
        lcd_clearLogo(animation);
        lcd_clearEdition();
        if (animation) HAL_Delay(500);
        lcd_drawPage(animation);

        // 2. Interaction Hardware Init
        led_initialize(animation);  // Initialize LEDs with smooth PWM fade-in
        ledLayerPlayMode = true;
        ledEqPlayMode = true;

        updatePlayTimerPeriod();
        startEqTimer();          // Visualizer clock
        startSdTimer();          // SD card status monitor
        startTransitionTimer();  // UI animation clock
        HAL_Delay(250);

        encoder_initialize();   // Start rotary encoder tracking
        keyboard_initialize();  // Start I2C button matrix expanders

        // 3. Restore User Session
        loadSystemPreset();   // Restore Global settings
        loadEncoderPreset();  // Restore last-used knob values
        sd_loadSynthUser();   // USER 01..16 synth sounds (none yet = all empty)
        sd_recoverSave();     // v2.01: finish / discard a save a power cut interrupted
        name_initialize();    // v2.02: random names for the new session's project and drumkit

        // 4. Enable Global Communication Protocols
        HAL_HalfDuplex_EnableReceiver(&huart1);  // MIDI In (Interrupt)
        HAL_HalfDuplex_EnableTransmitter(&huart6);
        HAL_UART_Receive_IT(&huart1, &midiRxData, 1);
        HAL_Delay(100);

        // 5. Final Application Entry
        menu = GLOBAL_MENU;
        startPlayTimer();       // Start the main Sequencer Heartbeat
        startMidiClockTimer();  // Start the midi Sequencer Heartbeat

        // 6. AUTOSAVE (v2.01): offer to resume the last session (ADD = yes, CLEAR = no).
        autosave_askResume();
    }
}

void Controller::update() {
    /// @brief Main execution loop handling state transitions, I/O processing, and system tasks.
    /// Orchestrates high-level logic based on hardware flags and user input.

    if (menu == TEST_MENU) {
        // 1. DIAGNOSTIC OVERRIDE
        // Run test routines and skip normal runtime flow.
        test();
    } else {
        // 2. INPUT PROCESSING
        // Read keyboard and encoder sources, then dispatch actions.
        keyboard_read();
        keyboard_action();

        // Read physical rotary encoder counters and update corresponding parameters.
        encoder_read();
        encoder_action();

        // Sample preview: debounce + stream one SD chunk (non-blocking, Layer Inst audition).
        preview_service();

        // Layer Song EUCLID page: mark the step that is playing now.
        lcd_updateEuclidPlayhead();

        // Song mode: show where the chain is after the sequencer moved on.
        song_service();

        // IO menu: refresh the audio CPU meter.
        cpu_service();

        // SYNTH LFO page: move the position marker (25 times per second, only when it moved).
        if (synthLfoPageActive() && ((HAL_GetTick() - synthLfoCursorTick) >= 40)) {
            synthLfoCursorTick = HAL_GetTick();
            lcd_updateSynthLfoCursor();
        }

        // 3. OUTPUT & UI REFRESH
        // Refresh LED outputs and LCD frame content.
        led_action();

        // Push buffered graphical changes to the LCD.
        lcd_update();

        // VISUALS mode: one frame every ~33 ms (the normal UI is muted meanwhile).
        visual_service();

        // 4. FILESYSTEM MAINTENANCE
        // Recover SD subsystem if media access is lost.
        if (!sd.ready) {
            sd_reinitialize();
        }

        // 5. RHYTHM & QUANTIZATION LOCKS
        // Apply lock and unlock requests for rhythm parameters.
        if (rhythmUnlockActionFlag) {
            rhythmUnlockActionFlag = false;
            rhythm.measureLock = false;
            rhythm.barLock = false;
            rhythm.quantizeLock = false;
            if (menu == RHYTHM_MENU) {
                lcd_drawRhythm_MeasureData();
                lcd_drawRhythm_BarData();
                lcd_drawRhythm_QuantizeData();
            }
        }
        if (rhythmLockActionFlag) {
            rhythmLockActionFlag = false;
            rhythm.measureLock = true;
            rhythm.barLock = true;
            rhythm.quantizeLock = true;
            if (menu == RHYTHM_MENU) {
                lcd_drawRhythm_MeasureData();
                lcd_drawRhythm_BarData();
                lcd_drawRhythm_QuantizeData();
            }
        }

        // 6. AUDIO MIXER / AMPLIFIER CONTROL
        // Process pending mute or unmute actions for all outputs.
        if (masterUnmuteActionFlag) {
            masterUnmuteActionFlag = false;
            if (mixer.masterMute) mixer_setMasterMute(false);
        }
        if (masterMuteActionFlag) {
            masterMuteActionFlag = false;
            if (!mixer.masterMute) mixer_setMasterMute(true);
        }
        if (hpUnmuteActionFlag) {
            hpUnmuteActionFlag = false;
            if (mixer.hpMute) mixer_setHpMute(false);
        }
        if (hpMuteActionFlag) {
            hpMuteActionFlag = false;
            if (!mixer.hpMute) mixer_setHpMute(true);
        }
        if (loUnmuteActionFlag) {
            loUnmuteActionFlag = false;
            if (mixer.loMute) mixer_setLoMute(false);
        }
        if (loMuteActionFlag) {
            loMuteActionFlag = false;
            if (!mixer.loMute) mixer_setLoMute(true);
        }
        if (liUnmuteActionFlag) {
            liUnmuteActionFlag = false;
            if (mixer.liMute) mixer_setLiMute(false);
        }
        if (liMuteActionFlag) {
            liMuteActionFlag = false;
            if (!mixer.liMute) mixer_setLiMute(true);
        }

        // 7. PRESET PERSISTENCE
        // Commit dirty preset data to storage.
        if (systemPresetSaveFlag) saveSystemPreset();
        if (encoderPresetSaveFlag) saveEncoderPreset();

        // AUTOSAVE (v2.01): save the session after a quiet spell when it has changed.
        autosave_service();

        // 8. CLIPBOARD OPERATIONS (LFO)
        // Handle copy, paste, and reset actions for LFO data.
        if ((lfoCopyActionFlag) && (selectedLfoNum != -1)) {
            lfoCopyActionFlag = false;
            lfo_copy();
        }
        if ((lfoPasteActionFlag) && (lfoCopy.copyActive)) {
            lfoPasteActionFlag = false;
            lfo_paste();
        }
        if ((lfoResetActionFlag) && (selectedLfoNum != -1)) {
            lfoResetActionFlag = false;
            lfo_reset(selectedLfoNum);
        }

        // 9. CLIPBOARD OPERATIONS (INSTRUMENTS)
        // Handle copy, paste, and reset actions for layer instrument data.
        if ((layerInstCopyActionFlag) && (selectedLayerNum != -1)) {
            layerInstCopyActionFlag = false;
            layerInst_copy();
        }
        // copyActive is a bool, so the old "!= -1" test was always true; layerInst_paste()
        // checks copyActive itself. The flag must be consumed here either way, otherwise a
        // paste pressed with an empty clipboard would fire on the next copy.
        if (layerInstPasteActionFlag) {
            layerInstPasteActionFlag = false;
            layerInst_paste();
        }
        if ((layerInstResetActionFlag) && (selectedLayerNum != -1)) {
            layerInstResetActionFlag = false;
            layerInst_reset(selectedLayerNum);
        }

        // 10. CLIPBOARD OPERATIONS (SEQUENCER SONG)
        // Handle copy, paste, and reset actions for song pattern data.
        if ((layerSongCopyActionFlag) && (selectedLayerNum != -1)) {
            layerSongCopyActionFlag = false;
            layerSong_copy();
        }
        // See layerInstPasteActionFlag above; layerSong_paste() checks copyActive itself.
        if (layerSongPasteActionFlag) {
            layerSongPasteActionFlag = false;
            layerSong_paste();
        }
        if ((layerSongResetActionFlag) && (selectedLayerNum != -1)) {
            layerSongResetActionFlag = false;
            layerSong_resetAllBeats(selectedLayerNum, activeSongBank);
            // On the EUCLID / NOTE page the empty layer becomes the page's new starting point, so
            // the next knob turn (or CLEAR tap) cannot bring the erased beats back.
            if (euclidPageActive()) {
                euclid_captureBase();
                euclidTruncated = false;
                lcd_drawEuclidStrip();
            } else if (notePageActive()) {
                note_captureBase();
                noteTransposeTotal = 0;
                lcd_drawNoteData(7);
                lcd_redrawNotePage();
            }
        }
        if (noteMelodyActionFlag) {
            noteMelodyActionFlag = false;
            if (notePageActive()) note_melody(true);
        }

        // 11. SECONDARY NAVIGATION
        // Execute long-press navigation handlers.
        if (keyboard.upActionFlag) {
            keyboard.upActionFlag = false;
            keyboard_upLong();
        }
        if (keyboard.downActionFlag) {
            keyboard.downActionFlag = false;
            keyboard_downLong();
        }

        // 12. OUTBOUND MIDI TRANSMISSION
        // Enqueue pending MIDI note events for each layer into the TX ring buffer.
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            if (tone::playsNotes(layerLibrary[i].toneMode)) {
                // TONE layer: send the beat's own note. A new beat retriggers without its own
                // Note Off, so the held note is released first: external synths get a clean
                // monophonic line.
                if ((io.midi.txNotes) && (io.midi.txTriggerNoteOn[i]) && toneMidiLegato[i] && (toneMidiSounding[i] != 0xFF)) {
                    // SLIDE: overlap the notes (new On, then old Off) so a mono synth plays legato.
                    const uint8_t note = toneMidiNote[i];
                    const uint8_t old = toneMidiSounding[i];
                    uint8_t on[3] = {(uint8_t)(0x90 + io.midi.txChannel), note, 0x64};
                    uint8_t off[3] = {(uint8_t)(0x80 + io.midi.txChannel), old, 0x64};
                    if ((note == old) || enqueueMidiTx(on, 3)) {
                        if ((note == old) || enqueueMidiTx(off, 3)) {
                            io.midi.txTriggerNoteOn[i] = false;
                            toneMidiLegato[i] = false;
                            toneMidiSounding[i] = note;
                        }
                    }
                }
                if ((io.midi.txNotes) && (io.midi.txTriggerNoteOn[i])) {
                    toneMidiLegato[i] = false;
                    if (toneMidiSounding[i] != 0xFF) {
                        uint8_t off[3] = {(uint8_t)(0x80 + io.midi.txChannel), toneMidiSounding[i], 0x64};
                        if (enqueueMidiTx(off, 3)) toneMidiSounding[i] = 0xFF;
                    }
                    if (toneMidiSounding[i] == 0xFF) {
                        const uint8_t note = toneMidiNote[i];
                        uint8_t msg[3] = {(uint8_t)(0x90 + io.midi.txChannel), note, 0x64};
                        if (enqueueMidiTx(msg, 3)) {
                            io.midi.txTriggerNoteOn[i] = false;
                            toneMidiSounding[i] = note;
                        }
                    }
                }
                if ((io.midi.txNotes) && (io.midi.txTriggerNoteOff[i]) && (!io.midi.txTriggerNoteOn[i])) {
                    if (toneMidiSounding[i] != 0xFF) {
                        uint8_t msg[3] = {(uint8_t)(0x80 + io.midi.txChannel), toneMidiSounding[i], 0x64};
                        if (enqueueMidiTx(msg, 3)) {
                            io.midi.txTriggerNoteOff[i] = false;
                            toneMidiSounding[i] = 0xFF;
                        }
                    } else {
                        io.midi.txTriggerNoteOff[i] = false;
                    }
                }
                continue;
            }

            // Send Note On (0x90)
            if ((io.midi.txNotes) && (io.midi.txTriggerNoteOn[i])) {
                uint8_t msg[3] = {(uint8_t)(0x90 + io.midi.txChannel), io.midi.txData[i], 0x64};
                if (enqueueMidiTx(msg, 3)) io.midi.txTriggerNoteOn[i] = false;
            }

            // Send Note Off (0x80)
            if ((io.midi.txNotes) && (io.midi.txTriggerNoteOff[i])) {
                uint8_t msg[3] = {(uint8_t)(0x80 + io.midi.txChannel), io.midi.txData[i], 0x64};
                if (enqueueMidiTx(msg, 3)) io.midi.txTriggerNoteOff[i] = false;
            }
        }

        // 13. INBOUND MIDI PROCESSING
        // Translate incoming MIDI events to playback and recording actions.
        if (io.midi.rxNoteOnWriteFlag == 1) {
            uint8_t layerNum = io.midi.rxNoteOnKey % 12;  // Map MIDI key to layer (0-11)
            if (layerNum < kLayerLibrarySize) {
                Layer& layer = layerLibrary[layerNum];

                // Trigger audio if layer is active and unmuted
                if ((!layer.mute) && layerSounds(layerNum))
                    layer_playBeat(layerNum, 1.0f);

                // Write note to the current sequencer step if Recording is enabled
                if (recordActive) {
                    if (!metronome.precountState) {
                        layerSong_setBeat(layerNum, activeSongBank, playInterval, 0, 0, 0, 0, true);
                    } else {
                        // Note landed during precount; commit it to position 0 once playback starts.
                        precountPendingBeat[layerNum] = true;
                    }
                }
            }
            io.midi.rxNoteOnWriteFlag = 0;  // Clear software interrupt flag
        }

        // Clean up inbound Note Off logic
        if (io.midi.rxNoteOffWriteFlag == 1) {
            io.midi.rxNoteOffWriteFlag = 0;
        }

        // 14. INBOUND MIDI TRANSPORT AND CLOCK
        // Handle transport commands and tempo updates signaled by the MIDI RX ISR.
        // Executed here in main context so that LCD, timers, and state can be safely updated.
        if (io.midi.rxPlayFlag) {
            io.midi.rxPlayFlag = false;
            // Hardware Sync-In, when active, is always the transport master: ignore
            // MIDI transport while it is active so the two clock sources never race.
            if (io.syncIn == 0) {
                stopFlag = false;
                resetFlag = false;
                ignoreNextMidiStopAfterSpecialCmd = false;

                if (io.sync.slaveMode && io.midi.rxTransport) {
                    if (playArmed || recordArmed) {
                        if (io.midi.rxStartFlag) {
                            // Keep current song position when resuming from armed state
                            // after external STOP/START sequence.
                            stopInterval = 0;
                            resetInterval = 0;
                        }

                        bool wasPlayArmed = playArmed;
                        bool wasRecordArmed = recordArmed;
                        clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
                        if (wasPlayArmed) play();
                        if (wasRecordArmed) record();
                    }
                } else if (playArmed || recordArmed) {
                    bool wasPlayArmed = playArmed;
                    bool wasRecordArmed = recordArmed;
                    clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
                    if (wasPlayArmed) play();
                    if (wasRecordArmed) record();
                }
                io.midi.rxStartFlag = false;
            }
        }
        if (io.midi.rxContinueFlag) {
            io.midi.rxContinueFlag = false;
            if (io.syncIn == 0) {
                stopFlag = false;
                resetFlag = false;
                ignoreNextMidiStopAfterSpecialCmd = false;

                if (io.sync.slaveMode && io.midi.rxTransport) {
                    if (playArmed || recordArmed) {
                        bool wasPlayArmed = playArmed;
                        bool wasRecordArmed = recordArmed;
                        clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
                        if (wasPlayArmed) play();
                        if (wasRecordArmed) record();
                    }
                } else if (playArmed || recordArmed) {
                    bool wasPlayArmed = playArmed;
                    bool wasRecordArmed = recordArmed;
                    clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
                    if (wasPlayArmed) play();
                    if (wasRecordArmed) record();
                }
            }
        }
        if (io.midi.rxTrigStopFlag) {
            io.midi.rxTrigStopFlag = false;
            if ((io.syncIn == 0) && playActive) {
                ignoreNextMidiStopAfterSpecialCmd = true;
                triggerStop();
            }
        }
        if (io.midi.rxTrigResetFlag) {
            io.midi.rxTrigResetFlag = false;
            if (io.syncIn == 0) {
                if (playActive) {
                    ignoreNextMidiStopAfterSpecialCmd = true;
                    triggerReset();
                } else {
                    reset();
                }
            }
        }
        if (io.midi.rxTrigRecordFlag) {
            io.midi.rxTrigRecordFlag = false;
            if (io.syncIn == 0) {
                record();
            }
        }
        if (io.midi.rxStopFlag) {
            io.midi.rxStopFlag = false;
            if (io.syncIn == 0) {
                // MIDI transport mode: master stop always wins.
                if (io.sync.slaveMode && io.midi.rxTransport) {
                    triggerStop();
                    clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
                }
                // MIDI-IN inactive: armed/TRS logic
                else if (ignoreNextMidiStopAfterSpecialCmd) {
                    ignoreNextMidiStopAfterSpecialCmd = false;
                } else {
                    triggerStop();
                    clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
                }
            }
        }
        if (io.midi.rxResetFlag) {
            io.midi.rxResetFlag = false;

            if (io.syncIn == 0) {
                // MIDI-IN Reset should behave like Stop trigger; never hard reset transport.
                triggerStop();
                clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
            }
        }

        if (io.midi.rxClockFlag) {
            if (io.syncIn == 0) {
                // Clamp measured BPM to the supported tempo range before applying.
                uint16_t newTempo = io.midi.rxClockBpm;
                if (newTempo < kMinTempo) newTempo = kMinTempo;
                if (newTempo > kMaxTempo) newTempo = kMaxTempo;
                if ((uint8_t)newTempo != rhythm.tempo) {
                    rhythm_setTempo((uint8_t)newTempo);
                }
            }
            io.midi.rxClockFlag = false;
        }

        // 15. INBOUND SYNC HANDLING (MODE-A / MODE-B)
        // MODE-A: clock-only sync; local play/record can arm and start on next clock.
        // MODE-B: gate controls play/stop; clock continuously updates tempo.

        // EXTI line can be shared with another input on some hardware revisions,
        // so poll gate state every update to keep MODE-A start/stop reliable.
        interruptSyncInGate();

        if (io.sync.rxPlayFlag) {
            io.sync.rxPlayFlag = false;
            if ((io.syncIn == 2) && (playArmed || recordArmed)) {
                bool wasPlayArmed = playArmed;
                bool wasRecordArmed = recordArmed;
                clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
                if (wasPlayArmed) play();
                if (wasRecordArmed) record();
            } else if (playArmed || recordArmed) {
                if (playArmed) play();
                if (recordArmed) record();
                playArmed = false;
                recordArmed = false;
                textClearFlag = true;
            } else if ((io.syncIn != 2) && !playActive) {
                play();
            }
        }
        if (io.sync.rxRecordFlag) {
            io.sync.rxRecordFlag = false;
            if (recordArmed) {
                record();
                recordArmed = false;
                recordIcon.flag = true;
                recordIcon.mode = false;
                textClearFlag = true;
            } else if (playArmed) {
                record();
                playArmed = false;
                playIcon.flag = true;
                playIcon.mode = false;
                textClearFlag = true;
            }
        }
        if (io.sync.rxStopFlag) {
            io.sync.rxStopFlag = false;
            triggerStop();
            clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
        }
        if (io.sync.rxResetFlag) {
            io.sync.rxResetFlag = false;
            if (io.syncIn == 2) {
                triggerStop();
            } else {
                reset();
            }
            clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);
        }

        if (io.sync.rxPulseFlag) {
            io.sync.rxPulseFlag = false;
            uint16_t newTempo = io.sync.rxPulseBpm;
            if (newTempo < kMinTempo) newTempo = kMinTempo;
            if (newTempo > kMaxTempo) newTempo = kMaxTempo;
            if ((uint8_t)newTempo != rhythm.tempo) {
                rhythm_setTempo((uint8_t)newTempo);
            }
        }

    }
}

void Controller::test() {
    /// @brief Routes execution to the appropriate hardware diagnostic routine.
    /// Used for factory calibration and troubleshooting individual hardware components.

    // 1. TEST MODE DISPATCHER
    // Select and run the active diagnostic routine.
    switch (testMode) {
        case TEST_NONE:
            // No test routine is active.
            break;

        case TEST_SCREEN:
            // Display test placeholder.
            break;

        case TEST_KEY:
            // 2. INPUT SUBSYSTEM DIAGNOSTIC
            // Validate keyboard and encoder input paths.

            // 2.1. Scan the button matrix via I2C3/DMA
            keyboard_read();

            // 2.2. Draw visual feedback of button presses on the LCD
            keyboard_test();

            // 2.3. Scan the rotary encoder hardware counters
            encoder_read();

            // 2.4. Draw visual feedback of encoder rotation/clicks on the LCD
            encoder_test();
            break;

        case TEST_AUDIO:
            // Audio path test: metronome click, STD sync pulse, and MIDI note
            // fire every measure via interruptPlay while record() keeps the sequencer running.
            break;

        case TEST_SYNC:
            // Sync/MIDI test: STD sync pulse and MIDI note continue firing every
            // measure via interruptPlay; no additional action needed here.
            break;

        case TEST_END:
            // Exit test mode placeholder.
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Menu Functions ------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::preMenuLayerClear() {
    /// @brief Clears pre-menu layer-related selections and restores LED layer state.

    // 1. LFO MENU CLEANUP
    // Reset selected LFO and restore default layer LED selection mode.
    if (preMenu == LFO_MENU) {
        selectedLfoNum = -1;
        ledLayerPlayMode = true;
        ledLayerSelectFlag = true;
        ledLayerSelectData = 0x00;
    }

    // 2. LAYER INST MENU CLEANUP
    // Clear selected layer and redraw tab state when leaving layer instrument menus.
    if ((preMenu == LAYER_INST_0_MENU) || (preMenu == LAYER_INST_1_MENU) || (preMenu == LAYER_INST_2_MENU)) {
        uint8_t layerNum = selectedLayerNum;

        // Reset graph cache so next Layer Inst entry redraws waveform/meta from scratch.
        resetLayerInstGraphCache(layerNum);

        selectedLayerNum = -1;
        lcd_drawLayerTab(layerNum);
        ledLayerPlayMode = true;
        ledLayerSelectFlag = true;
        ledLayerSelectData = 0x00;
    }

    // 3. LAYER SONG MENU CLEANUP
    // Clear selected beat/layer and redraw layer tab when leaving layer song menu.
    if (preMenu == LAYER_SONG_MENU) {
        if ((layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum != -1) && (selectedBeatNum != -1)) lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, false);
        uint8_t layerNum = selectedLayerNum;
        selectedLayerNum = -1;
        selectedBeatNum = -1;
        lcd_drawLayerTab(layerNum);
        ledLayerPlayMode = true;
        ledLayerSelectFlag = true;
        ledLayerSelectData = 0x00;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Global Functions ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::global_select() {
    /// @brief Switches to Global menu and refreshes related UI/context.

    // 1. MENU TRANSITION GUARD
    // Apply transition only if current menu is different.
    if (menu != GLOBAL_MENU) {
        // 2. MENU STATE UPDATE
        // Store previous menu context and activate GLOBAL_MENU.
        preMenu = menu;
        menu = GLOBAL_MENU;
        preMenuTab = menuTab;
        menuTab = -1;

        // 3. ENCODER CONTEXT UPDATE
        // Rebind encoder functions for global context.
        encoder_update();

        // 4. PREVIOUS MENU CLEANUP
        // Clear layer/LFO selection artifacts.
        preMenuLayerClear();

        // 5. UI TRANSITION & RENDER
        // Animate and render global menu screen.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawGlobalMenu();
    }
}

