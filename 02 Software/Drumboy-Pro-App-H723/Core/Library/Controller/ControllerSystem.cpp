// Mixer, rhythm, metronome, MIDI/sync I/O.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Sync Functions ------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

bool Controller::enqueueMidiTx(const uint8_t* data, uint8_t len) {
    /// @brief Pushes a MIDI message into the TX ring buffer and triggers transmission if idle.
    /// Safe to call from both main context and ISR context.
    /// @return True if enqueued, false if the ring buffer was full.

    // 1. CRITICAL SECTION ENTRY
    // Disable all maskable interrupts to protect shared ring buffer state.
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    // 2. OVERFLOW GUARD
    // Reject message when ring buffer has no free slot.
    uint8_t nextHead = (io.midi.txRingHead + 1) & (Midi::kTxRingSize - 1);
    if (nextHead == io.midi.txRingTail) {
        __set_PRIMASK(primask);
        return false;
    }

    // 3. ENQUEUE
    // Write message bytes and length into the next ring buffer slot.
    MidiTxMsg& slot = io.midi.txRing[io.midi.txRingHead];
    for (uint8_t i = 0; i < len; i++) slot.data[i] = data[i];
    slot.len = len;
    io.midi.txRingHead = nextHead;

    // 4. TRANSMISSION KICKOFF
    // Start DMA immediately if the UART is currently idle.
    if (!io.midi.txBusy) startMidiTx();

    // 5. CRITICAL SECTION EXIT
    __set_PRIMASK(primask);
    return true;
}

void Controller::startMidiTx() {
    /// @brief Dequeues the next MIDI message and starts a DMA transfer.
    /// Must be called within a critical section or from the UART TX ISR.

    // 1. EMPTY CHECK
    if (io.midi.txRingTail == io.midi.txRingHead) return;

    // 2. IN-FLIGHT BUFFER LOAD
    // Copy next message into a static buffer that persists across the async DMA transfer.
    static uint8_t txDmaBuf[8];
    MidiTxMsg& msg = io.midi.txRing[io.midi.txRingTail];
    uint8_t len = msg.len;
    for (uint8_t i = 0; i < len; i++) txDmaBuf[i] = msg.data[i];
    io.midi.txRingTail = (io.midi.txRingTail + 1) & (Midi::kTxRingSize - 1);

    // 3. TRANSMITTER LOCK AND DMA START
    io.midi.txBusy = true;
    if (HAL_UART_Transmit_DMA(&huart6, txDmaBuf, len) != HAL_OK) {
        io.midi.txBusy = false;
    }
}

void Controller::serviceMidiTx() {
    /// @brief Called from HAL_UART_TxCpltCallback. Releases the TX lock and drains the queue.

    // 1. TRANSMITTER RELEASE
    io.midi.txBusy = false;

    // 2. QUEUE DRAIN
    // Start the next queued message if one is waiting.
    startMidiTx();
}

void Controller::receiveMidiCommand() {
    /// @brief Parses incoming MIDI bytes one at a time (called from UART ISR).
    /// Real-time system bytes (0xF8–0xFF) are dispatched immediately without
    /// interrupting the running channel-message state machines.

    // 1. RX ACTIVITY CHECK
    // Continue parsing only when MIDI RX engine is enabled.
    if (!io.midi.rxActive) return;

    uint8_t rxData = midiRxData;

    // 2. REAL-TIME MESSAGE HANDLER (0xF8–0xFF)
    // Single-byte real-time messages may appear anywhere in the byte stream.
    // Clock and transport are gated independently so CLOCK ONLY and
    // CLK+TRNS/FULL can behave differently.
    if (rxData >= 0xF8) {
        switch (rxData) {
            case 0xF8: {
                if (!io.midi.rxClock) return;
                // MIDI Clock: accumulate 48 ticks (2 beats) then calculate BPM.
                // Using 2-beat window + rounding to suppress 1ms HAL_GetTick jitter.
                uint32_t now = HAL_GetTick();
                io.midi.rxClockTickPhase = (io.midi.rxClockTickPhase + 1) % 24;
                if (io.midi.rxClockCount == 0) {
                    // First tick received after idle — anchor the beat start.
                    io.midi.rxClockBeatStart = now;
                    io.midi.rxClockCount = 1;
                } else if (++io.midi.rxClockCount > 48) {
                    // 48 ticks elapsed: two full beats measured.
                    uint32_t interval = now - io.midi.rxClockBeatStart;
                    if ((interval > 0) && (interval < 8000)) {  // Guard: 15–400 BPM
                        io.midi.rxClockBpm = (uint16_t)((120000UL + interval / 2) / interval);
                        io.midi.rxClockFlag = true;
                    }
                    io.midi.rxClockBeatStart = now;
                    io.midi.rxClockCount = 1;
                }

                if (io.midi.rxTransport && io.midi.rxTransportRunning && io.midi.rxClockPhaseValid &&
                    (io.midi.rxClockTickPhase == 0) && (playArmed || recordArmed) && !playActive) {
                    io.midi.rxPlayFlag = true;
                }
                break;
            }

            case 0xFA:  // MIDI Start
                if (!io.midi.rxTransport) return;
                io.midi.rxClockCount = 0;
                io.midi.rxClockTickPhase = 0;
                io.midi.rxTransportRunning = true;
                io.midi.rxClockPhaseValid = true;
                io.midi.rxStartFlag = true;
                io.midi.rxPlayFlag = true;
                break;

            case 0xFB:  // MIDI Continue
                if (!io.midi.rxTransport) return;
                io.midi.rxClockCount = 0;
                io.midi.rxClockTickPhase = 0;
                io.midi.rxTransportRunning = true;
                io.midi.rxClockPhaseValid = true;
                io.midi.rxContinueFlag = true;
                break;

            case 0xFC:  // MIDI Stop
                if (!io.midi.rxTransport) return;
                io.midi.rxClockCount = 0;
                io.midi.rxClockTickPhase = 0;
                io.midi.rxTransportRunning = false;
                io.midi.rxClockPhaseValid = false;
                io.midi.rxStopFlag = true;
                break;

            case 0xFF:  // MIDI System Reset
                if (!io.midi.rxTransport) return;
                io.midi.rxClockCount = 0;
                io.midi.rxClockTickPhase = 0;
                io.midi.rxTransportRunning = false;
                io.midi.rxClockPhaseValid = false;
                io.midi.rxResetFlag = true;
                break;
        }
        return;
    }

    if (rxData == kMidiSysExStart) {
        io.midi.rxSysExStage = 1;
        io.midi.rxSysExCmd = 0;
        return;
    }

    if (io.midi.rxSysExStage != 0) {
        switch (io.midi.rxSysExStage) {
            case 1:
                io.midi.rxSysExStage = (rxData == kMidiSysExManufacturerNonCommercial) ? 2 : 0;
                break;

            case 2:
                io.midi.rxSysExStage = (rxData == kMidiSysExTransportFamily) ? 3 : 0;
                break;

            case 3:
                io.midi.rxSysExCmd = rxData;
                io.midi.rxSysExStage = 4;
                break;

            case 4:
                if ((rxData == kMidiSysExEnd) && io.midi.rxTransport) {
                    if (io.midi.rxSysExCmd == kMidiSysExTrigStop)
                        io.midi.rxTrigStopFlag = true;
                    else if (io.midi.rxSysExCmd == kMidiSysExTrigReset)
                        io.midi.rxTrigResetFlag = true;
                    else if (io.midi.rxSysExCmd == kMidiSysExTrigRecord)
                        io.midi.rxTrigRecordFlag = true;
                }
                io.midi.rxSysExStage = 0;
                io.midi.rxSysExCmd = 0;
                break;

            default:
                io.midi.rxSysExStage = 0;
                io.midi.rxSysExCmd = 0;
                break;
        }
        return;
    }

    // 3. NOTE ON PARSER (0x90 + CHANNEL)
    // Only processed when rxNotes is enabled (NOTES or SYN+NOT mode).
    if (io.midi.rxNotes) {
        if ((io.midi.rxNoteOnReadStage == 0) && (rxData == (0x90 + io.midi.rxChannel))) {
            // Stage 0: Valid Status byte detected for the selected channel.
            io.midi.rxNoteOnReadStage = 1;
        } else if (io.midi.rxNoteOnReadStage == 1) {
            // Stage 1: Capture the Note Number (0–127).
            io.midi.rxNoteOnKey = rxData;
            io.midi.rxNoteOnReadStage = 2;
        } else if (io.midi.rxNoteOnReadStage == 2) {
            // Stage 2: Capture the Velocity and finalize the message.
            io.midi.rxNoteOnVelocity = rxData;
            io.midi.rxNoteOnReadStage = 0;
            io.midi.rxNoteOnWriteFlag = 1;
        }

        // 4. NOTE OFF PARSER (0x80 + CHANNEL)
        if ((io.midi.rxNoteOffReadStage == 0) && (rxData == (0x80 + io.midi.rxChannel))) {
            // Stage 0: Valid Note Off Status byte detected.
            io.midi.rxNoteOffReadStage = 1;
        } else if (io.midi.rxNoteOffReadStage == 1) {
            // Stage 1: Capture the Note Number.
            io.midi.rxNoteOffKey = rxData;
            io.midi.rxNoteOffReadStage = 2;
        } else if (io.midi.rxNoteOffReadStage == 2) {
            // Stage 2: Capture the Release Velocity and finalize.
            io.midi.rxNoteOffVelocity = rxData;
            io.midi.rxNoteOffReadStage = 0;
            io.midi.rxNoteOffWriteFlag = 1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Mixer Functions -----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::mixer_select() {
    /// @brief Switches to Mixer menu and prepares UI/encoder context for mixer operations.

    if (menu != MIXER_MENU) {
        // 1. MENU STATE TRANSITION
        // Store previous menu context and activate MIXER_MENU tab root.
        preMenu = menu;
        preMenuTab = menuTab;
        menuTab = 0;
        menu = MIXER_MENU;

        // 2. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 3. UI TRANSITION & RENDER
        // Animate menu change and render mixer screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawMixerMenu();

        // 4. ENCODER CONTEXT UPDATE
        // Rebind encoders to Mixer menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::mixer_reset() {
    /// @brief Restores mixer mute, level, routing, and dynamics parameters to default values.

    // 1. MUTE STATE RESET
    // Restore all output/input mute flags to initial preset values.
    mixer_setMasterMute(kInitialMixerMasterMute);
    mixer_setHpMute(kInitialMixerHpMute);
    mixer_setLoMute(kInitialMixerLoMute);
    mixer_setLiMute(kInitialMixerLiMute);

    // 2. LEVEL & ROUTING RESET
    // Restore volumes, line-in routing, pan, limiter, and normalize state.
    mixer_setMasterVolume(kInitialMixerMasterVolume);
    mixer_setHpVolume(kInitialMixerHpVolume);
    mixer_setLoVolume(kInitialMixerLoVolume);
    mixer_setLiVolume(kInitialMixerLiVolume);
    mixer_setLiIn(kInitialMixerLiMixerIn);
    mixer_setPan(kInitialMixerPan);
    mixer_setLimiter(kInitialMixerLimiter);
    mixer_setNormalize(kInitialMixerNormalize);
}

void Controller::mixer_menuRight() {
    /// @brief Advances Mixer menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab < kMaxMenu8Tab) ? (menuTab + 1) : kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::mixer_menuLeft() {
    /// @brief Moves Mixer menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab > kMinMenu8Tab) ? (menuTab - 1) : kMaxMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::mixer_menuUp() {
    /// @brief Increments the parameter mapped to the active Mixer tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific mixer parameter with upper-bound checks.
    switch (menuTab) {
        case 0:
            if (mixer.masterVolume < kMaxMixerMasterVolume) {
                mixer_setMasterVolume(mixer.masterVolume + 1);
            }
            break;

        case 1:
            if (mixer.hpVolume < kMaxMixerHpVolume) {
                mixer_setHpVolume(mixer.hpVolume + 1);
            }
            break;

        case 2:
            if (mixer.loVolume < kMaxMixerLoVolume) {
                mixer_setLoVolume(mixer.loVolume + 1);
            }
            break;

        case 3:
            if (mixer.liVolume < kMaxMixerLiVolume) {
                mixer_setLiVolume(mixer.liVolume + 1);
            }
            break;

        case 4:
            if (mixer.liMixerIn < kMaxMixerLiMixerIn) {
                mixer_setLiIn(mixer.liMixerIn + 1);
            }
            break;

        case 5:
            if (mixer.pan < kMaxMixerPan) {
                mixer_setPan(mixer.pan + 1);
            }
            break;

        case 6:
            if (mixer.limiter < kMaxMixerLimiter) {
                mixer_setLimiter(mixer.limiter + 1);
            }
            break;

        case 7:
            mixer_setNormalize(!mixer.normalize);
            break;
    }
}

void Controller::mixer_menuDown() {
    /// @brief Decrements the parameter mapped to the active Mixer tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific mixer parameter with lower-bound checks.
    switch (menuTab) {
        case 0:
            if (mixer.masterVolume > kMinMixerMasterVolume) {
                mixer_setMasterVolume(mixer.masterVolume - 1);
            }
            break;

        case 1:
            if (mixer.hpVolume > kMinMixerHpVolume) {
                mixer_setHpVolume(mixer.hpVolume - 1);
            }
            break;

        case 2:
            if (mixer.loVolume > kMinMixerLoVolume) {
                mixer_setLoVolume(mixer.loVolume - 1);
            }
            break;

        case 3:
            if (mixer.liVolume > kMinMixerLiVolume) {
                mixer_setLiVolume(mixer.liVolume - 1);
            }
            break;

        case 4:
            if (mixer.liMixerIn > kMinMixerLiMixerIn) {
                mixer_setLiIn(mixer.liMixerIn - 1);
            }
            break;

        case 5:
            if (mixer.pan > kMinMixerPan) {
                mixer_setPan(mixer.pan - 1);
            }
            break;

        case 6:
            if (mixer.limiter > kMinMixerLimiter) {
                mixer_setLimiter(mixer.limiter - 1);
            }
            break;

        case 7:
            mixer_setNormalize(!mixer.normalize);
            break;
    }
}

void Controller::mixer_setMasterVolume(uint8_t volume_) {
    /// @brief Applies master volume change and starts smooth gain transition.

    // 1. INPUT VALIDATION
    // Accept only values inside configured master-volume limits.
    if ((volume_ >= kMinMixerMasterVolume) && (volume_ <= kMaxMixerMasterVolume)) {
        // 2. DATA UPDATE & TRANSITION START
        // Store new value, convert to amplitude target, and start ramp transition.
        mixer.masterVolume = volume_;
        float targetVolume = kNumberDataLibrary[mixer.masterVolume].powMultiplier;
        mixer_masterVolumeTransition(targetVolume);

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_MasterVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_MASTERVOLUME);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setHpVolume(uint8_t volume_) {
    /// @brief Sets headphone output volume and propagates update to DAC/UI state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured headphone-volume limits.
    if ((volume_ >= kMinMixerHpVolume) && (volume_ <= kMaxMixerHpVolume)) {
        // 2. DATA & DAC UPDATE
        // Store value and apply new gain to headphone path.
        mixer.hpVolume = volume_;
        dac.setHeadphoneVolume(volume_);

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_HpVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_HPVOLUME);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setLoVolume(uint8_t volume_) {
    /// @brief Sets line-out volume and propagates update to DAC/UI state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured line-out volume limits.
    if ((volume_ >= kMinMixerLoVolume) && (volume_ <= kMaxMixerLoVolume)) {
        // 2. DATA & DAC UPDATE
        // Store value and apply new gain to line-out path.
        mixer.loVolume = volume_;
        dac.setLineoutVolume(volume_);

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_LoVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_LOVOLUME);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setLiVolume(uint8_t volume_) {
    /// @brief Sets line-in return volume for mixer path and refreshes UI state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured line-in volume limits.
    if ((volume_ >= kMinMixerLiVolume) && (volume_ <= kMaxMixerLiVolume)) {
        // 2. DATA UPDATE
        // Store line-in mixer volume value.
        mixer.liVolume = volume_;

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_LiVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_LIVOLUME);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setLiIn(uint8_t mode_) {
    /// @brief Sets line-in routing destination in mixer signal chain.

    // 1. INPUT VALIDATION
    // Accept only values inside configured line-in route index limits.
    if ((mode_ >= kMinMixerLiMixerIn) && (mode_ <= kMaxMixerLiMixerIn)) {
        // 2. ROUTE INDEX UPDATE
        // Store selected routing index before mapping to route enum.
        mixer.liMixerIn = mode_;

        // 3. ROUTE MODE MAPPING
        // Translate menu index to concrete mixer input destination.
        switch (mixer.liMixerIn) {
            case 0:
                mixer.liMixerInMode = MIXER_IN_EQ;
                break;

            case 1:
                mixer.liMixerInMode = MIXER_IN_FILTER_0;
                break;

            case 2:
                mixer.liMixerInMode = MIXER_IN_FILTER_1;
                break;

            case 3:
                mixer.liMixerInMode = MIXER_IN_EFFECT_0;
                break;

            case 4:
                mixer.liMixerInMode = MIXER_IN_EFFECT_1;
                break;

            case 5:
                mixer.liMixerInMode = MIXER_IN_REVERB;
                break;

            case 6:
                mixer.liMixerInMode = MIXER_IN_SEND;
                break;
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_LiInData();
        lcd_updateGlobalMenuData(ENC_MIXER_LIMIXERIN);

        // 5. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setPan(uint8_t pan_) {
    /// @brief Sets stereo pan position and starts left/right gain transitions.

    // 1. INPUT VALIDATION
    // Accept only values inside configured pan range.
    if ((pan_ >= kMinMixerPan) && (pan_ <= kMaxMixerPan)) {
        // 2. DATA UPDATE & TARGET DERIVATION
        // Store pan index and derive left/right target amplitudes.
        mixer.pan = pan_;
        float targetVolumeLeft = kNumberDataLibrary[kMixerPanDataLibrary[mixer.pan].left].powMultiplier;
        float targetVolumeRight = kNumberDataLibrary[kMixerPanDataLibrary[mixer.pan].right].powMultiplier;
        mixer_panTransition(targetVolumeLeft, targetVolumeRight);

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_PanData();
        lcd_updateGlobalMenuData(ENC_MIXER_PAN);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setLimiter(uint8_t limiter_) {
    /// @brief Sets limiter threshold/index and refreshes dependent UI/global state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured limiter range.
    if ((limiter_ >= kMinMixerLimiter) && (limiter_ <= kMaxMixerLimiter)) {
        // 2. DATA UPDATE
        // Store limiter parameter value.
        mixer.limiter = limiter_;

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) {
            lcd_drawMixer_LimiterData();
        }
        lcd_updateGlobalMenuData(ENC_MIXER_LIMITER);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setNormalize(bool active_) {
    /// @brief Toggles mixer normalization mode and refreshes dependent UI/global state.

    // 1. DATA UPDATE
    // Store normalization enable state.
    mixer.normalize = active_;

    // 2. UI & GLOBAL REFRESH
    // Refresh Mixer tab data (if visible) and global summary field.
    if (menu == MIXER_MENU) lcd_drawMixer_NormalizeData();
    lcd_updateGlobalMenuData(ENC_MIXER_NORMALIZE);

    // 3. DEFERRED PRESET PERSISTENCE
    // Restart system preset autosave timer when persistence is enabled.
    if (systemPresetSaveActive) {
        stopSystemPresetTimer();
        startSystemPresetTimer();
    }
}

void Controller::mixer_setMasterMute(bool mute_) {
    /// @brief Updates master mute state only when value changes.

    // 1. CHANGE DETECTION
    // Skip downstream updates when requested state equals current state.
    if (mixer.masterMute != mute_) {
        // 2. DATA UPDATE
        // Store new master mute state.
        mixer.masterMute = mute_;

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_MasterVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_MASTERVOLUME);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setHpMute(bool mute_) {
    /// @brief Updates headphone mute state and synchronizes DAC mute gate.

    // 1. CHANGE DETECTION
    // Skip downstream updates when requested state equals current state.
    if (mixer.hpMute != mute_) {
        // 2. DATA UPDATE
        // Store new headphone mute state.
        mixer.hpMute = mute_;

        // 3. DAC GATE CONTROL
        // Apply mute immediately; unmute only when non-zero headphone volume exists.
        if (mute_) {
            dac.muteHeadphone();
        } else if (mixer.hpVolume)
            dac.unmuteHeadphone();

        // 4. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_HpVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_HPVOLUME);

        // 5. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setLoMute(bool mute_) {
    /// @brief Updates line-out mute state and synchronizes DAC mute gate.

    // 1. CHANGE DETECTION
    // Skip downstream updates when requested state equals current state.
    if (mixer.loMute != mute_) {
        // 2. DATA UPDATE
        // Store new line-out mute state.
        mixer.loMute = mute_;

        // 3. DAC GATE CONTROL
        // Apply mute immediately; unmute only when non-zero line-out volume exists.
        if (mute_) {
            dac.muteLineout();
        } else if (mixer.loVolume)
            dac.unmuteLineout();

        // 4. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_LoVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_LOVOLUME);

        // 5. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_setLiMute(bool mute_) {
    /// @brief Updates line-in mute state and toggles DAC input path accordingly.

    // 1. CHANGE DETECTION
    // Skip downstream updates when requested state equals current state.
    if (mixer.liMute != mute_) {
        // 2. DATA UPDATE & INPUT GATE
        // Enable the codec path while still muted and only then let the audio interrupt
        // through, so the ADC power-up settle is never processed. Muting is the reverse:
        // stop processing first, then power the path down.
        if (mute_) {
            mixer.liMute = true;
            dac.disableLineIn();
        } else {
            dac.enableLineIn();
            mixer.liMute = false;
        }

        // 3. UI & GLOBAL REFRESH
        // Refresh Mixer tab data (if visible) and global summary field.
        if (menu == MIXER_MENU) lcd_drawMixer_LiVolumeData();
        lcd_updateGlobalMenuData(ENC_MIXER_LIVOLUME);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::mixer_masterVolumeTransition(float volumeFloat_) {
    /// @brief Queues master-volume ramp target and activates transition engine.

    MixerVolumeTransition& vTransition = mixer.masterVolumeTransition;

    // 1. TARGET UPDATE
    // Store requested target amplitude for master channel ramp.
    vTransition.targetVolume = volumeFloat_;

    // 2. TRANSITION PRE-CALC & ARM
    // Calculate direction/step and mark transition as active.
    mixer_calculateMasterVolumeTransition();
    vTransition.active = true;
}

void Controller::mixer_panTransition(float volumeLeftFloat_, float volumeRightFloat_) {
    /// @brief Queues stereo pan ramp targets and activates transition engine.

    MixerPanTransition& pTransition = mixer.panTransition;

    // 1. TARGET UPDATE
    // Store requested left/right target amplitudes for pan ramp.
    pTransition.targetVolumeLeft = volumeLeftFloat_;
    pTransition.targetVolumeRight = volumeRightFloat_;

    // 2. TRANSITION PRE-CALC & ARM
    // Calculate channel directions/steps and mark transition as active.
    mixer_calculatePanTransition();
    pTransition.active = true;
}

void Controller::mixer_calculateMasterVolumeTransition() {
    /// @brief Determines master-volume transition direction against current float state.

    MixerVolumeTransition& vTransition = mixer.masterVolumeTransition;

    // 1. DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN action enum.
    if (mixer.masterVolumeFloat == vTransition.targetVolume) {
        vTransition.actionVolume = MIXER_ACTION_NONE;
    } else if (mixer.masterVolumeFloat < vTransition.targetVolume) {
        vTransition.actionVolume = MIXER_ACTION_UP;
    } else if (mixer.masterVolumeFloat > vTransition.targetVolume) {
        vTransition.actionVolume = MIXER_ACTION_DOWN;
    }
}

void Controller::mixer_calculatePanTransition() {
    /// @brief Determines left/right pan transition directions against current float states.

    MixerPanTransition& pTransition = mixer.panTransition;

    // 1. LEFT CHANNEL DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for left channel.
    if (mixer.volumeLeftFloat == pTransition.targetVolumeLeft) {
        pTransition.actionVolumeLeft = MIXER_ACTION_NONE;
    } else if (mixer.volumeLeftFloat < pTransition.targetVolumeLeft) {
        pTransition.actionVolumeLeft = MIXER_ACTION_UP;
    } else if (mixer.volumeLeftFloat > pTransition.targetVolumeLeft) {
        pTransition.actionVolumeLeft = MIXER_ACTION_DOWN;
    }

    // 2. RIGHT CHANNEL DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for right channel.
    if (mixer.volumeRightFloat == pTransition.targetVolumeRight) {
        pTransition.actionVolumeRight = MIXER_ACTION_NONE;
    } else if (mixer.volumeRightFloat < pTransition.targetVolumeRight) {
        pTransition.actionVolumeRight = MIXER_ACTION_UP;
    } else if (mixer.volumeRightFloat > pTransition.targetVolumeRight) {
        pTransition.actionVolumeRight = MIXER_ACTION_DOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Rhythm Functions ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::rhythm_select() {
    /// @brief Switches to Rhythm menu and prepares UI/encoder context for rhythm operations.
    if (menu != RHYTHM_MENU) {
        // 1. MENU STATE TRANSITION
        // Store previous menu context and activate RHYTHM_MENU tab root.
        preMenu = menu;
        menu = RHYTHM_MENU;
        preMenuTab = menuTab;
        menuTab = 0;

        // 2. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 3. UI TRANSITION & RENDER
        // Animate menu change and render rhythm screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawRhythmMenu();

        // 4. ENCODER CONTEXT UPDATE
        // Rebind encoders to Rhythm menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::rhythm_reset() {
    /// @brief Restores rhythm timing parameters to default values.

    // 1. RHYTHM PARAMETER RESET
    // Re-apply initial tempo, measure, bar, and quantize settings.
    rhythm_setTempo(kInitialTempo);
    rhythm_setMeasure(kInitialMeasure);
    rhythm_setBar(kInitialBar);
    rhythm_setQuantize(kInitialQuantize);
}

void Controller::rhythm_menuRight() {
    /// @brief Advances Rhythm menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    (menuTab < kMaxMenu4Tab) ? menuTab += 1 : menuTab = kMinMenu4Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::rhythm_menuLeft() {
    /// @brief Moves Rhythm menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    (menuTab > kMinMenu4Tab) ? menuTab -= 1 : menuTab = kMaxMenu4Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::rhythm_menuUp() {
    /// @brief Increments the parameter mapped to the active Rhythm tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific rhythm parameter with guards.
    switch (menuTab) {
        case 0:

            // Tempo cannot be edited while external sync slave mode is active.
            if ((rhythm.tempo < kMaxTempo) && (!io.sync.slaveMode)) {
                rhythm_setTempo(rhythm.tempo + 1);
            }
            break;

        case 1:

            // Measure edits are blocked when measure lock is enabled.
            if (!rhythm.measureLock) {
                if (rhythm.measure < kMaxMeasure) {
                    // During playback, show alert instead of changing song length.
                    if (playActive) {
                        alertFlag = true;
                        alertType = ALERT_MEASUREUP;
                        lcd_drawAlert();
                    } else {
                        rhythm_setMeasure(rhythm.measure + 1);
                    }
                }
            }
            break;

        case 2:

            // Bar edits are blocked when bar lock is enabled.
            if (!rhythm.barLock) {
                if (rhythm.bar < kMaxBar) {
                    // During playback, show alert instead of changing song length.
                    if (playActive) {
                        alertFlag = true;
                        alertType = ALERT_BARUP;
                        lcd_drawAlert();
                    } else {
                        rhythm_setBar(rhythm.bar + 1);
                    }
                }
            }
            break;

        case 3:

            // Quantize edits are blocked when quantize lock is enabled.
            if (!rhythm.quantizeLock) {
                if (rhythm.quantize < kMaxQuantize) {
                    // During playback, show alert instead of altering quantize grid.
                    if (playActive) {
                        alertFlag = true;
                        alertType = ALERT_QUANTIZEUP;
                        lcd_drawAlert();
                    } else {
                        rhythm_setQuantize(rhythm.quantize + 1);
                    }
                }
            }
            break;
    }
}

void Controller::rhythm_menuDown() {
    /// @brief Decrements the parameter mapped to the active Rhythm tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific rhythm parameter with guards.
    switch (menuTab) {
        case 0:

            // Tempo cannot be edited while external sync slave mode is active.
            if ((rhythm.tempo > kMinTempo) && (!io.sync.slaveMode)) {
                rhythm_setTempo(rhythm.tempo - 1);
            }
            break;

        case 1:

            // Measure edits are blocked when measure lock is enabled.
            if (!rhythm.measureLock) {
                if (rhythm.measure > kMinMeasure) {
                    // During playback, show alert instead of changing song length.
                    if (playActive) {
                        alertFlag = true;
                        alertType = ALERT_MEASUREDOWN;
                        lcd_drawAlert();
                    } else {
                        rhythm_setMeasure(rhythm.measure - 1);
                    }
                }
            }
            break;

        case 2:

            // Bar edits are blocked when bar lock is enabled.
            if (!rhythm.barLock) {
                if (rhythm.bar > kMinBar) {
                    // During playback, show alert instead of changing song length.
                    if (playActive) {
                        alertFlag = true;
                        alertType = ALERT_BARDOWN;
                        lcd_drawAlert();
                    } else {
                        rhythm_setBar(rhythm.bar - 1);
                    }
                }
            }
            break;

        case 3:

            // Quantize edits are blocked when quantize lock is enabled.
            if (!rhythm.quantizeLock) {
                if (rhythm.quantize > kMinQuantize) {
                    // During playback, show alert instead of altering quantize grid.
                    if (playActive) {
                        alertFlag = true;
                        alertType = ALERT_QUANTIZEDOWN;
                        lcd_drawAlert();
                    } else {
                        rhythm_setQuantize(rhythm.quantize - 1);
                    }
                }
            }
            break;
    }
}

void Controller::rhythm_setTempo(uint8_t tempo_) {
    /// @brief Sets rhythm tempo and propagates timing updates to transport/effects/UI.

    // 1. INPUT VALIDATION
    // Accept only values inside configured tempo limits.
    if ((tempo_ >= kMinTempo) && (tempo_ <= kMaxTempo)) {
        // 2. CORE TIMING UPDATE
        // Store tempo and refresh play timer and tempo-synced effect delays.
        rhythm.tempo = tempo_;
        updatePlayTimerPeriod();
        effect[0].delay.update(tempo_);
        effect[1].delay.update(tempo_);

        // 3. UI & GLOBAL REFRESH
        // Refresh Rhythm/LFO screens (if visible) and global summary field.
        if (menu == RHYTHM_MENU) lcd_drawRhythm_TempoData();
        if (menu == LFO_MENU) lcd_drawLfo_GraphData(selectedLfoNum);
        lcd_updateGlobalMenuData(ENC_RHYTHM_TEMPO);
    }
}

void Controller::rhythm_setMeasure(uint8_t measure_) {
    /// @brief Sets measure count and rebuilds song timing/layout state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured measure limits and total song length cap.
    if ((measure_ >= kMinMeasure) && (measure_ <= kMaxMeasure) && (measure_ * rhythm.bar <= kMaxSongMeasureTotal)) {
        // 2. CORE TIMING UPDATE
        // Store measure count and recalculate transport timing relationships.
        rhythm.measure = measure_;
        calculateSongInterval();
        adjustMeasureBarTiming();

        // 3. RHYTHM UI REFRESH
        // Refresh measure field when Rhythm menu is active.
        if (menu == RHYTHM_MENU)
            lcd_drawRhythm_MeasureData();

        // 4. SONG CANVAS REBUILD
        // Recompute song grid geometry and redraw all visible layer lanes.
        lcd_calculateSongX();
        lcd_resetPlay();
        lcd_drawMeasureBar();
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            lcd_drawSong(i, activeSongBank);
        }

        // 5. TRANSPORT RESET
        // Restart playback state to apply new measure structure safely.
        reset();
    }
}

void Controller::rhythm_setBar(uint8_t bar_) {
    /// @brief Sets bar count and rebuilds song timing/layout state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured bar limits and total song length cap.
    if ((bar_ >= kMinBar) && (bar_ <= kMaxBar) && (rhythm.measure * bar_ <= kMaxSongMeasureTotal)) {
        // 2. CORE TIMING UPDATE
        // Store bar count and recalculate transport timing relationships.
        rhythm.bar = bar_;
        calculateSongInterval();
        adjustMeasureBarTiming();

        // 3. RHYTHM UI REFRESH
        // Refresh bar field when Rhythm menu is active.
        if (menu == RHYTHM_MENU)
            lcd_drawRhythm_BarData();

        // 4. SONG CANVAS REBUILD
        // Recompute song grid geometry and redraw all visible layer lanes.
        lcd_calculateSongX();
        lcd_resetPlay();
        lcd_drawMeasureBar();
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            lcd_drawSong(i, activeSongBank);
        }

        // 5. TRANSPORT RESET
        // Restart playback state to apply new bar structure safely.
        reset();
    }
}

void Controller::rhythm_setQuantize(uint8_t quantize_) {
    /// @brief Sets quantize grid and re-quantizes active song beats across banks.

    // 1. INPUT VALIDATION
    // Accept only values inside configured quantize limits.
    if ((quantize_ >= kMinQuantize) && (quantize_ <= kMaxQuantize)) {
        // 2. DATA UPDATE
        // Store quantize resolution value.
        rhythm.quantize = quantize_;

        // 3. RHYTHM UI REFRESH
        // Refresh quantize field when Rhythm menu is active.
        if (menu == RHYTHM_MENU)
            lcd_drawRhythm_QuantizeData();

        // 4. BEAT DATA RE-QUANTIZATION
        // Recompute active beat positions for all layers and banks.
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            for (uint8_t j = 0; j < kBankLibrarySize; j++) {
                layerSong_quantizeActiveBeats(i, j);
            }
        }

        // 5. SONG UI REPAINT
        // Redraw layer lanes so quantized beat positions are visible.
        for (uint8_t j = 0; j < kLayerLibrarySize; j++) {
            lcd_drawSong(j, activeSongBank);
        }

        // 6. TRANSPORT RESET
        // Restart playback state to apply new quantize grid safely.
        reset();
    }
}

/* Song Mode Functions (Rhythm menu, knobs 5-8) -------------------------------*/

// The chain (songChain) is project data, saved in the .rwf file. The sequencer ISR advances it
// at every loop end (song_onLoopEnd); bank switches use the same end-of-loop mechanism as the
// manual SONG BANK trigger (bankShiftFlag / targetSongBank).

void Controller::song_setMode(uint8_t mode_) {
    /// @brief Sets OFF / LOOP / ONCE. Turning it on while playing starts the chain at the
    /// next loop end; turning it off leaves the current bank playing.

    if (mode_ > songchain::ONCE) return;
    songChain.mode = mode_;
    if (mode_ == songchain::OFF) {
        songChainRunning = false;
        songChainPendingStart = false;
    } else if (playActive && !songChainRunning) {
        songChainPendingStart = true;
    }
    if (menu == RHYTHM_MENU) lcd_drawRhythm_SongData();
}

void Controller::song_change(EncoderFunc function_, bool up_) {
    /// @brief Handles the song mode knobs in the Rhythm menu.

    songchain::Step& step = songChain.steps[songEditStep];
    switch (function_) {
        case ENC_RHYTHM_SONG:
            if (up_ && (songChain.mode < songchain::ONCE)) song_setMode(songChain.mode + 1);
            if (!up_ && (songChain.mode > songchain::OFF)) song_setMode(songChain.mode - 1);
            return;

        case ENC_RHYTHM_SONG_STEP:
            if (up_ && (songEditStep < songchain::kMaxSteps - 1)) songEditStep++;
            if (!up_ && (songEditStep > 0)) songEditStep--;
            break;

        case ENC_RHYTHM_SONG_PART:
            // 1..5, then END
            if (up_) {
                if (step.part < songchain::kParts - 1) {
                    step.part++;
                } else if (step.part < songchain::kParts) {
                    step.part = songchain::kPartEnd;
                }
            } else {
                if (step.part >= songchain::kParts) {
                    step.part = songchain::kParts - 1;
                } else if (step.part > 0) {
                    step.part--;
                }
            }
            if ((step.repeats < 1) || (step.repeats > songchain::kMaxRepeats)) step.repeats = 1;
            break;

        case ENC_RHYTHM_SONG_REPEAT:
            if (up_ && (step.repeats < songchain::kMaxRepeats)) step.repeats++;
            if (!up_ && (step.repeats > 1)) step.repeats--;
            break;

        default:
            return;
    }

    // Step/part changes alter the chain length, shown in STEP and SONG.
    if (menu == RHYTHM_MENU) {
        lcd_drawRhythm_SongData();
        lcd_drawRhythm_SongStepData();
        lcd_drawRhythm_SongPartData();
        lcd_drawRhythm_SongRepeatData();
    }
}

void Controller::song_startIfAtTop() {
    /// @brief Called from play(): from the top of the song, start the chain at step 1 and switch
    /// straight to its part. Resuming a paused song keeps its place in the chain.

    if ((songChain.mode == songchain::OFF) || songChainRunning) return;
    if (playInterval != 0) {
        songChainPendingStart = true;  // started mid-loop: begin at the next loop end
        return;
    }
    uint8_t part = activeSongBank;
    if (!songchain::start(songChain, songPosition, part)) return;
    songChainRunning = true;
    songChainPendingStart = false;
    if (part != activeSongBank) {
        activeSongBank = part;
        targetSongBank = part;
        bankShiftFlag = false;
        bankActionFlag = true;  // main loop redraws the bank tab and lanes
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) layerSong_calculatePlayBeatNum(i, activeSongBank, playInterval);
    }
    songChainDisplayFlag = true;
}

void Controller::song_rewind() {
    /// @brief Called from reset(): the next PLAY starts the chain at step 1.

    songPosition = songchain::Position{};
    songChainRunning = false;
    songChainPendingStart = false;
    songChainDisplayFlag = true;
}

void Controller::song_onLoopEnd(bool& stop_) {
    /// @brief Sequencer ISR, at the end of every loop (before the bank shift is applied).
    /// Chooses the part for the next loop, or asks the caller to stop (ONCE, after the last
    /// loop of the last step). Must not draw: it only sets flags for the main loop.

    stop_ = false;
    if (songChain.mode == songchain::OFF) {
        songChainRunning = false;
        songChainPendingStart = false;
        return;
    }

    uint8_t part = activeSongBank;
    if (songChainPendingStart) {
        songChainPendingStart = false;
        songChainRunning = songchain::start(songChain, songPosition, part);
    } else if (songChainRunning) {
        if (songchain::advance(songChain, songPosition, part) == songchain::STOP) {
            stop_ = true;
            songChainDisplayFlag = true;
            return;
        }
    } else {
        return;
    }

    if (songChainRunning && (part != activeSongBank)) {
        targetSongBank = part;
        bankShiftFlag = true;
    }
    songChainDisplayFlag = true;
}

void Controller::song_onFileLoaded() {
    /// @brief After a project file is loaded: the chain restarts from step 1 (at the next loop
    /// end if the transport is running).

    songPosition = songchain::Position{};
    songChainRunning = false;
    songChainPendingStart = playActive && (songChain.mode != songchain::OFF);
    songEditStep = 0;
    songChainDisplayFlag = true;
}

void Controller::song_service() {
    /// @brief Main loop: redraws the SONG field after the sequencer moved through the chain.

    if (songChainDisplayFlag) {
        songChainDisplayFlag = false;
        if (menu == RHYTHM_MENU) lcd_drawRhythm_SongData();
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Metronome Functions -------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::metro_select() {
    /// @brief Switches to Metronome menu and prepares UI/encoder context for metronome operations.
    if (menu != METRO_MENU) {
        // 1. MENU STATE TRANSITION
        // Store previous menu context and activate METRO_MENU tab root.
        preMenu = menu;
        menu = METRO_MENU;
        preMenuTab = menuTab;

        // 2. TAB INITIALIZATION
        // Reset tab cursor to first metronome page.
        menuTab = 0;

        // 3. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 4. UI TRANSITION & RENDER
        // Animate menu change and render metronome screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawMetroMenu();

        // 5. ENCODER CONTEXT UPDATE
        // Rebind encoders to Metronome menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::metro_reset() {
    /// @brief Restores metronome state parameters to default values.

    // 1. METRONOME PARAMETER RESET
    // Re-apply initial active, precount, sample, and volume settings.
    metro_setActive(kInitialMetroActive);
    metro_setPrecount(kInitialMetroPrecount);
    metro_setSample(kInitialMetroSample);
    metro_setVolume(kInitialMetroVolume);
}

void Controller::metro_menuRight() {
    /// @brief Advances Metronome menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab < kMaxMenu4Tab) ? (menuTab + 1) : kMinMenu4Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::metro_menuLeft() {
    /// @brief Moves Metronome menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab > kMinMenu4Tab) ? (menuTab - 1) : kMaxMenu4Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::metro_menuUp() {
    /// @brief Increments or toggles the parameter mapped to active Metronome tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific metronome setting.
    switch (menuTab) {
        case 0:
            metro_setActive(!metronome.active);
            break;

        case 1:
            metro_setPrecount(!metronome.precount);
            break;

        case 2:
            (metronome.sample < kMaxMetroSample) ? metro_setSample(metronome.sample + 1) : metro_setSample(kMinMetroSample);
            break;

        case 3:
            if (metronome.volume < kMaxMetroVolume) metro_setVolume(metronome.volume + 1);
            break;
    }
}

void Controller::metro_menuDown() {
    /// @brief Decrements or toggles the parameter mapped to active Metronome tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific metronome setting.
    switch (menuTab) {
        case 0:
            metro_setActive(!metronome.active);
            break;

        case 1:
            metro_setPrecount(!metronome.precount);
            break;

        case 2:
            (metronome.sample > kMinMetroSample) ? metro_setSample(metronome.sample - 1) : metro_setSample(kMaxMetroSample);
            break;

        case 3:
            if (metronome.volume > kMinMetroVolume) metro_setVolume(metronome.volume - 1);
            break;
    }
}

void Controller::metro_setActive(bool active_) {
    /// @brief Sets metronome active state and refreshes dependent UI/global state.

    // 1. DATA UPDATE
    // Store active enable state.
    metronome.active = active_;

    // 2. UI & GLOBAL REFRESH
    // Refresh Metronome tab data (if visible) and global summary field.
    if (menu == METRO_MENU) lcd_drawMetro_ActiveData();
    lcd_updateGlobalMenuData(ENC_METRO_ACTIVE);

    // 3. DEFERRED PRESET PERSISTENCE
    // Restart system preset autosave timer when persistence is enabled.
    if (systemPresetSaveActive) {
        stopSystemPresetTimer();
        startSystemPresetTimer();
    }
}

void Controller::metro_setPrecount(bool precount_) {
    /// @brief Sets metronome precount state and recalculates song interval timing.

    // 1. DATA & TIMING UPDATE
    // Store precount state and recompute interval math used by transport.
    metronome.precount = precount_;
    calculateSongInterval();

    // 2. UI & GLOBAL REFRESH
    // Refresh Metronome tab data (if visible) and global summary field.
    if (menu == METRO_MENU) lcd_drawMetro_PrecountData();
    lcd_updateGlobalMenuData(ENC_METRO_PRECOUNT);

    // 3. DEFERRED PRESET PERSISTENCE
    // Restart system preset autosave timer when persistence is enabled.
    if (systemPresetSaveActive) {
        stopSystemPresetTimer();
        startSystemPresetTimer();
    }
}

void Controller::metro_setSample(uint8_t sample_) {
    /// @brief Sets metronome sample index and refreshes dependent UI/global state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured metronome sample limits.
    if ((sample_ >= kMinMetroSample) && (sample_ <= kMaxMetroSample)) {
        // 2. DATA UPDATE
        // Store sample index used by metronome playback.
        metronome.sample = sample_;

        // 3. UI & GLOBAL REFRESH
        // Refresh Metronome tab data (if visible) and global summary field.
        if (menu == METRO_MENU) lcd_drawMetro_SampleData();
        lcd_updateGlobalMenuData(ENC_METRO_SAMPLE);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::metro_setVolume(uint8_t volume_) {
    /// @brief Sets metronome volume index and starts smooth gain transition.

    // 1. INPUT VALIDATION
    // Accept only values inside configured metronome volume limits.
    if ((volume_ >= kMinMetroVolume) && (volume_ <= kMaxMetroVolume)) {
        // 2. DATA UPDATE & TRANSITION START
        // Store value, derive amplitude target, and start ramp transition.
        metronome.volume = volume_;
        float targetVolume = kNumberDataLibrary[metronome.volume].powMultiplier;
        metro_volumeTransition(targetVolume);

        // 3. UI & GLOBAL REFRESH
        // Refresh Metronome tab data (if visible) and global summary field.
        if (menu == METRO_MENU) lcd_drawMetro_VolumeData();
        lcd_updateGlobalMenuData(ENC_METRO_VOLUME);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::metro_setPreviewActive(bool active_) {
    /// @brief Turns the Layer Inst sample preview on/off (Metronome menu, knob 5).

    previewActive = active_;
    if (!previewActive) preview_stop();

    if (menu == METRO_MENU) lcd_drawMetro_PreviewData();

    if (systemPresetSaveActive) {
        stopSystemPresetTimer();
        startSystemPresetTimer();
    }
}

void Controller::metro_setPreviewLevel(uint8_t level_) {
    /// @brief Sets the sample preview level (Metronome menu, knob 6); applies immediately,
    /// also to a preview that is playing.

    if ((level_ >= kMinPreviewLevel) && (level_ <= kMaxPreviewLevel)) {
        previewLevel = level_;
        previewPlay.gain = kNumberDataLibrary[previewLevel].powMultiplier;

        if (menu == METRO_MENU) lcd_drawMetro_PreviewLevelData();

        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::metro_volumeTransition(float volumeFloat_) {
    /// @brief Queues metronome volume ramp target and activates transition engine.

    MetronomeVolumeTransition& vTransition = metronome.volumeTransition;

    // 1. TARGET UPDATE
    // Store requested target amplitude for metronome gain ramp.
    vTransition.targetVolume = volumeFloat_;

    // 2. TRANSITION PRE-CALC & ARM
    // Calculate direction/step and mark transition as active.
    metro_calculateVolumeTransition();
    vTransition.active = true;
}

void Controller::metro_calculateVolumeTransition() {
    /// @brief Determines metronome volume transition direction against current float state.

    MetronomeVolumeTransition& vTransition = metronome.volumeTransition;

    // 1. DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN action enum.
    if (metronome.volumeFloat == vTransition.targetVolume) {
        vTransition.actionVolume = MET_ACTION_NONE;
    } else if (metronome.volumeFloat < vTransition.targetVolume) {
        vTransition.actionVolume = MET_ACTION_UP;
    } else if (metronome.volumeFloat > vTransition.targetVolume) {
        vTransition.actionVolume = MET_ACTION_DOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Io Functions --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::cpu_service() {
    /// @brief Main loop: redraws the CPU meter in the IO menu four times per second (only when
    /// a value changed, so the bit-banged LCD is not kept busy).

    if (menu != IO_MENU) return;
    const uint32_t now = HAL_GetTick();
    if ((now - cpuDisplayTick) < 250) return;
    cpuDisplayTick = now;
    lcd_drawIo_CpuData();
    lcd_drawIo_CpuPeakData();
}

void Controller::cpu_resetPeak() {
    /// @brief Turning CPU PEAK starts a new peak measurement from now.

    cpuPeakPermille = 0;
    if (menu == IO_MENU) lcd_drawIo_CpuPeakData();
}

void Controller::io_select() {
    /// @brief Switches to I/O menu and prepares UI/encoder context for interface routing.

    if (menu != IO_MENU) {
        // 1. MENU STATE TRANSITION
        // Store previous menu context and activate IO_MENU tab root.
        preMenu = menu;
        preMenuTab = menuTab;
        menuTab = 0;
        menu = IO_MENU;

        // 2. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 3. UI TRANSITION & RENDER
        // Animate menu change and render I/O screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawIoMenu();

        // 4. ENCODER CONTEXT UPDATE
        // Rebind encoders to I/O menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::io_reset() {
    /// @brief Restores MIDI and Sync routing parameters to default values.

    // 1. SYNC & MIDI RESET
    // Re-apply initial sync in/out and MIDI in/out mode and channel selections.
    io_setSyncIn(kInitialIoSyncIn);
    io_setSyncOut(kInitialIoSyncOut);
    io_setMidiIn(kInitialIoMidiIn);
    io_setMidiInChannel(kInitialIoMidiInChannel);
    io_setMidiOut(kInitialIoMidiOut);
    io_setMidiOutChannel(kInitialIoMidiOutChannel);
}

void Controller::io_menuRight() {
    /// @brief Advances I/O menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab < kMaxMenu4Tab) ? (menuTab + 1) : kMinMenu4Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::io_menuLeft() {
    /// @brief Moves I/O menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab > kMinMenu4Tab) ? (menuTab - 1) : kMaxMenu4Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::io_menuUp() {
    /// @brief Increments the parameter mapped to active I/O tab.
    /// For MIDI tabs (2/3): ADD held → channel up; plain → mode up (wraps).

    // 1. TAB-BASED PARAMETER DISPATCH
    switch (menuTab) {
        case 0:
            if (keyboard.shiftKeyPress) {
                // ADD held: change Sync-In PPQN (MODE-A, wraps).
                if (io.syncIn > 0) {
                    if (io.syncInPpqn < kMaxIoSyncPpqn) io_setSyncInPpqn(io.syncInPpqn + 1);
                }
            } else {
                // Normal: change Sync-In mode (wraps).
                if (io.syncIn < kMaxIoSyncIn)
                    io_setSyncIn(io.syncIn + 1);
                else
                    io_setSyncIn(kMinIoSyncIn);
            }
            break;

        case 1:
            if (keyboard.shiftKeyPress) {
                // ADD held: change Sync-Out PPQN (MODE-A/MODE-B, wraps).
                if (io.syncOut > 0) {
                    if (io.syncOutPpqn < kMaxIoSyncPpqn) io_setSyncOutPpqn(io.syncOutPpqn + 1);
                }
            } else {
                // Normal: change Sync-Out mode (wraps).
                if (io.syncOut < kMaxIoSyncOut)
                    io_setSyncOut(io.syncOut + 1);
                else
                    io_setSyncOut(kMinIoSyncOut);
            }
            break;

        case 2:
            if (keyboard.shiftKeyPress) {
                // ADD held: change MIDI-IN channel (clamped, no wrap).
                if (io.midiInChannel < kMaxIoMidiChannel)
                    io_setMidiInChannel(io.midiInChannel + 1);
            } else {
                // Normal: change MIDI-IN mode (wraps).
                if (io.midiIn < kMaxIoMidiIn)
                    io_setMidiIn(io.midiIn + 1);
                else
                    io_setMidiIn(kMinIoMidiIn);
            }
            break;

        case 3:
            if (keyboard.shiftKeyPress) {
                // ADD held: change MIDI-OUT channel (clamped, no wrap).
                if (io.midiOutChannel < kMaxIoMidiChannel)
                    io_setMidiOutChannel(io.midiOutChannel + 1);
            } else {
                // Normal: change MIDI-OUT mode (wraps).
                if (io.midiOut < kMaxIoMidiOut)
                    io_setMidiOut(io.midiOut + 1);
                else
                    io_setMidiOut(kMinIoMidiOut);
            }
            break;
    }
}

void Controller::io_menuDown() {
    /// @brief Decrements the parameter mapped to active I/O tab.
    /// For tabs 0/1: ADD held → PPQN down; plain → mode down (wraps).
    /// For MIDI tabs (2/3): ADD held → channel down; plain → mode down (wraps).

    // 1. TAB-BASED PARAMETER DISPATCH
    switch (menuTab) {
        case 0:
            if (keyboard.shiftKeyPress) {
                // ADD held: change Sync-In PPQN (MODE-A, wraps).
                if (io.syncIn > 0) {
                    if (io.syncInPpqn > kMinIoSyncPpqn) io_setSyncInPpqn(io.syncInPpqn - 1);
                }
            } else {
                if (io.syncIn > kMinIoSyncIn)
                    io_setSyncIn(io.syncIn - 1);
                else
                    io_setSyncIn(kMaxIoSyncIn);
            }
            break;

        case 1:
            if (keyboard.shiftKeyPress) {
                // ADD held: change Sync-Out PPQN (MODE-A/MODE-B, wraps).
                if (io.syncOut > 0) {
                    if (io.syncOutPpqn > kMinIoSyncPpqn) io_setSyncOutPpqn(io.syncOutPpqn - 1);
                }
            } else {
                if (io.syncOut > kMinIoSyncOut)
                    io_setSyncOut(io.syncOut - 1);
                else
                    io_setSyncOut(kMaxIoSyncOut);
            }
            break;

        case 2:
            if (keyboard.shiftKeyPress) {
                // ADD held: change MIDI-IN channel (clamped, no wrap).
                if (io.midiInChannel > kMinIoMidiChannel)
                    io_setMidiInChannel(io.midiInChannel - 1);
            } else {
                // Normal: change MIDI-IN mode (wraps).
                if (io.midiIn > kMinIoMidiIn)
                    io_setMidiIn(io.midiIn - 1);
                else
                    io_setMidiIn(kMaxIoMidiIn);
            }
            break;

        case 3:
            if (keyboard.shiftKeyPress) {
                // ADD held: change MIDI-OUT channel (clamped, no wrap).
                if (io.midiOutChannel > kMinIoMidiChannel)
                    io_setMidiOutChannel(io.midiOutChannel - 1);
            } else {
                // Normal: change MIDI-OUT mode (wraps).
                if (io.midiOut > kMinIoMidiOut)
                    io_setMidiOut(io.midiOut - 1);
                else
                    io_setMidiOut(kMaxIoMidiOut);
            }
            break;
    }
}

void Controller::io_setSyncIn(uint8_t mode_) {
    /// @brief Sets Sync-In mode and recomputes master/slave clock role.

    if (mode_ > kMaxIoSyncIn) mode_ = kMaxIoSyncIn;

    // 1. INPUT VALIDATION
    // Accept only values inside configured Sync-In mode limits.
    if ((mode_ >= kMinIoSyncIn) && (mode_ <= kMaxIoSyncIn)) {
        // Route sync sleeve line for Sync-In operation.
        // Sleeve is always forced to GND in sync operation.
        GPIO_InitTypeDef gpioInitStruct = {0};

        gpioInitStruct.Pin = SYNC_IN_GND_Pin;
        gpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        gpioInitStruct.Pull = GPIO_NOPULL;
        gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(SYNC_IN_GND_GPIO_Port, &gpioInitStruct);
        HAL_GPIO_WritePin(SYNC_IN_GND_GPIO_Port, SYNC_IN_GND_Pin, GPIO_PIN_RESET);

        // 2. DATA MAPPING
        // Store mode index and map to tempo/play/beat trigger flags.
        io.syncIn = mode_;
        if (io.syncIn == 0) {
            LED1_ON;
        }
        io.sync.syncInTempo = kIoSyncInDataLibrary[io.syncIn].tempoTrigger;
        io.sync.syncInPlay = kIoSyncInDataLibrary[io.syncIn].playTrigger;
        io.sync.syncInBeat = kIoSyncInDataLibrary[io.syncIn].beatTrigger;

        // 3. STD PULSE TRACKING RESET
        // Clear STD pulse state on any mode change to prevent stale timestamps
        // and false auto-play or auto-stop triggers.
        io.sync.rxPulseLastTick = 0;
        io.sync.rxPulseInterval = 0;
        io.sync.rxPulseBpm = 0;
        io.sync.rxPulseFlag = false;
        io.sync.rxPlayFlag = false;
        io.sync.rxRecordFlag = false;
        io.sync.rxStopFlag = false;
        io.sync.rxResetFlag = false;
        io.sync.rxGateHigh = false;

        // 3a. ARMED STATE RESET
        // Clear any pending armed play/record state when sync mode changes.
        if (playArmed || recordArmed) {
            playArmed = false;
            recordArmed = false;
            playIcon.flag = true;
            playIcon.mode = false;
            recordIcon.flag = true;
            recordIcon.mode = false;
            textClearFlag = true;
        }

        // 4. CLOCK ROLE RESOLUTION
        // Sync-In or MIDI clock active → slave (external tempo source has priority).
        // Otherwise Sync-Out active → master. Neither → standalone.
        if (io.syncIn || io.midi.rxClock) {
            io.sync.slaveMode = true;
            io.sync.masterMode = false;
        } else if (io.syncOut) {
            io.sync.slaveMode = false;
            io.sync.masterMode = true;
        } else {
            io.sync.slaveMode = false;
            io.sync.masterMode = false;
        }

        // 5. UI & GLOBAL REFRESH
        // Refresh I/O tab data (if visible) and global summary field.
        if (menu == IO_MENU) lcd_drawIo_SyncInData();
        lcd_updateGlobalMenuData(ENC_IO_SYNCIN);

        // 5. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::io_setSyncInPpqn(uint8_t ppqnIdx_) {
    /// @brief Sets Sync-In PPQN index and refreshes the display.
    if ((ppqnIdx_ >= kMinIoSyncPpqn) && (ppqnIdx_ <= kMaxIoSyncPpqn)) {
        io.syncInPpqn = ppqnIdx_;
        // Reset pulse tracking so BPM is recalculated with new PPQN.
        io.sync.rxPulseLastTick = 0;
        io.sync.rxPulseInterval = 0;
        io.sync.rxPulseBpm = 0;
        if (menu == IO_MENU) lcd_drawIo_SyncInPpqnData();
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::io_setSyncOut(uint8_t mode_) {
    /// @brief Sets Sync-Out mode, updates hardware line state, and recomputes clock role.

    if (mode_ > kMaxIoSyncOut) mode_ = kMaxIoSyncOut;

    // 1. INPUT VALIDATION
    // Accept only values inside configured Sync-Out mode limits.
    if ((mode_ >= kMinIoSyncOut) && (mode_ <= kMaxIoSyncOut)) {
        // Stop any in-flight pulse before mode mutation to avoid timer/output races.
        stopBeatSyncTimer();
        HAL_GPIO_WritePin(SYNC_OUT_PULSE_GPIO_Port, SYNC_OUT_PULSE_Pin, GPIO_PIN_RESET);

        if (io.syncOutPpqn > kMaxIoSyncPpqn) {
            io.syncOutPpqn = kInitialIoSyncOutPpqn;
        }

        // 2. MODE UPDATE & HW PIN ROUTING
        // Store mode index and apply required GPIO line state.
        io.syncOut = mode_;
        if (io.syncOut == 0) {
            LED2_ON;
        }
        switch (mode_) {
            case 0:  // off
                HAL_GPIO_WritePin(SYNC_OUT_GND_GPIO_Port, SYNC_OUT_GND_Pin, GPIO_PIN_RESET);
                break;

            case 1:  // MODE-A
                HAL_GPIO_WritePin(SYNC_OUT_GND_GPIO_Port, SYNC_OUT_GND_Pin, GPIO_PIN_RESET);
                break;

            case 2:  // MODE-B
                HAL_GPIO_WritePin(SYNC_OUT_GND_GPIO_Port, SYNC_OUT_GND_Pin, GPIO_PIN_RESET);
                break;

            case 3:  // MODE-C
                HAL_GPIO_WritePin(SYNC_OUT_GND_GPIO_Port, SYNC_OUT_GND_Pin, GPIO_PIN_RESET);
                break;
        }

        GPIO_InitTypeDef gpioInitStruct = {0};
        gpioInitStruct.Pin = SYNC_OUT_GATE_Pin;
        gpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        gpioInitStruct.Pull = GPIO_NOPULL;
        gpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(SYNC_OUT_GATE_GPIO_Port, &gpioInitStruct);
        HAL_GPIO_WritePin(SYNC_OUT_GATE_GPIO_Port, SYNC_OUT_GATE_Pin, (mode_ == 2 && playActive) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        io.sync.txPulseCounter = 0;
        io.sync.txTrsTickCounter = 0;

        // 3. TRIGGER MAP UPDATE
        // Map Sync-Out mode to tempo/play/beat trigger flags.
        io.sync.syncOutTempo = kIoSyncOutDataLibrary[io.syncOut].tempoTrigger;
        io.sync.syncOutPlay = kIoSyncOutDataLibrary[io.syncOut].playTrigger;
        io.sync.syncOutBeat = kIoSyncOutDataLibrary[io.syncOut].beatTrigger;

        // 4. CLOCK ROLE RESOLUTION
        // Sync-In or MIDI clock active → slave (external tempo source has priority).
        // Otherwise Sync-Out active → master. Neither → standalone.
        if (io.syncIn || io.midi.rxClock) {
            io.sync.slaveMode = true;
            io.sync.masterMode = false;
        } else if (io.syncOut) {
            io.sync.slaveMode = false;
            io.sync.masterMode = true;
        } else {
            io.sync.slaveMode = false;
            io.sync.masterMode = false;
        }

        // 5. UI & GLOBAL REFRESH
        // Refresh I/O tab data (if visible) and global summary field.
        if (menu == IO_MENU) lcd_drawIo_SyncOutData();
        lcd_updateGlobalMenuData(ENC_IO_SYNCOUT);

        // 6. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::io_setSyncOutPpqn(uint8_t ppqnIdx_) {
    /// @brief Sets Sync-Out PPQN index and refreshes the display.
    if ((ppqnIdx_ >= kMinIoSyncPpqn) && (ppqnIdx_ <= kMaxIoSyncPpqn)) {
        uint8_t effectivePpqnIdx = ppqnIdx_;
        if ((io.syncOut == 2) && (kIoSyncPpqnDataLibrary[effectivePpqnIdx].ppqn > 24)) {
            effectivePpqnIdx = 5;
        }

        io.syncOutPpqn = effectivePpqnIdx;
        // Recalculate txPulseCounter for the new PPQN at the current playInterval so the
        // Bresenham continues from the correct position without a gap or burst.
        if (playActive && (io.syncOut > 0)) {
            uint8_t newPpqn = kIoSyncPpqnDataLibrary[effectivePpqnIdx].ppqn;
            io.sync.txPulseCounter = (uint32_t)(((uint64_t)playInterval * newPpqn + measureInterval / 2) / measureInterval);
        } else {
            io.sync.txPulseCounter = 0;
        }
        io.sync.txTrsTickCounter = 0;
        if (menu == IO_MENU) lcd_drawIo_SyncOutPpqnData();
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::io_setMidiIn(uint8_t mode_) {
    /// @brief Sets MIDI input mode and derives clock/transport/notes receive flags.

    // 1. INPUT VALIDATION
    // Accept only values inside configured MIDI-IN mode limits.
    if ((mode_ >= kMinIoMidiIn) && (mode_ <= kMaxIoMidiIn)) {
        // 2. DATA MAPPING
        // Store mode index and map to active clock/transport/notes runtime flags.
        io.midiIn = mode_;
        const IoMidiModeData& modeData = kIoMidiModeDataLibrary[mode_];
        io.midi.rxClock = modeData.clockEnabled;
        io.midi.rxTransport = modeData.transportEnabled;
        io.midi.rxNotes = modeData.notesEnabled;
        io.midi.rxActive = modeData.clockEnabled || modeData.transportEnabled || modeData.notesEnabled;

        io.midi.rxClockCount = 0;
        io.midi.rxClockTickPhase = 0;
        io.midi.rxClockBeatStart = 0;
        io.midi.rxClockBpm = 0;
        io.midi.rxClockFlag = false;
        io.midi.rxTransportRunning = false;
        io.midi.rxClockPhaseValid = false;
        io.midi.rxStartFlag = false;
        io.midi.rxPlayFlag = false;
        io.midi.rxContinueFlag = false;
        io.midi.rxStopFlag = false;
        io.midi.rxResetFlag = false;
        io.midi.rxTrigStopFlag = false;
        io.midi.rxTrigResetFlag = false;
        io.midi.rxTrigRecordFlag = false;

        // 3. CLOCK ROLE RESOLUTION
        // Sync-In or MIDI clock active → slave (external tempo source has priority).
        // Otherwise Sync-Out active → master. Neither → standalone.
        if (io.syncIn || io.midi.rxClock) {
            io.sync.slaveMode = true;
            io.sync.masterMode = false;
        } else if (io.syncOut) {
            io.sync.slaveMode = false;
            io.sync.masterMode = true;
        } else {
            io.sync.slaveMode = false;
            io.sync.masterMode = false;
        }

        // 3a. ARMED STATE RESET
        // Clear any pending armed play/record state when MIDI mode changes.
        clearMidiArmedState(playArmed, recordArmed, playIcon, recordIcon, textClearFlag);

        if (!modeData.clockEnabled && !modeData.transportEnabled && playActive) {
            stop();
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh I/O tab data (if visible) and global summary field.
        if (menu == IO_MENU) lcd_drawIo_MidiInData();
        lcd_updateGlobalMenuData(ENC_IO_MIDIIN);

        // 5. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::io_setMidiOut(uint8_t mode_) {
    /// @brief Sets MIDI output mode and derives clock/transport/notes transmit flags.

    // 1. INPUT VALIDATION
    // Accept only values inside configured MIDI-OUT mode limits.
    if ((mode_ >= kMinIoMidiOut) && (mode_ <= kMaxIoMidiOut)) {
        // 2. DATA MAPPING
        // Store mode index and map to active clock/transport/notes runtime flags.
        io.midiOut = mode_;
        const IoMidiModeData& modeData = kIoMidiModeDataLibrary[mode_];
        io.midi.txClock = modeData.clockEnabled;
        io.midi.txTransport = modeData.transportEnabled;
        io.midi.txNotes = modeData.notesEnabled;
        io.midi.txActive = modeData.clockEnabled || modeData.transportEnabled || modeData.notesEnabled;

        // 3. UI & GLOBAL REFRESH
        // Refresh I/O tab data (if visible) and global summary field.
        if (menu == IO_MENU) lcd_drawIo_MidiOutData();
        lcd_updateGlobalMenuData(ENC_IO_MIDIOUT);

        // 4. DEFERRED PRESET PERSISTENCE
        // Restart system preset autosave timer when persistence is enabled.
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::io_setMidiInChannel(uint8_t channel_) {
    /// @brief Sets MIDI input channel (0-based internally, 1-16 displayed).

    // 1. INPUT VALIDATION
    if ((channel_ >= kMinIoMidiChannel) && (channel_ <= kMaxIoMidiChannel)) {
        // 2. DATA MAPPING
        io.midiInChannel = channel_;
        io.midi.rxChannel = channel_;

        // 3. UI & GLOBAL REFRESH
        if (menu == IO_MENU) lcd_drawIo_MidiInData();
        lcd_updateGlobalMenuData(ENC_IO_MIDIIN);

        // 4. DEFERRED PRESET PERSISTENCE
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

void Controller::io_setMidiOutChannel(uint8_t channel_) {
    /// @brief Sets MIDI output channel (0-based internally, 1-16 displayed).

    // 1. INPUT VALIDATION
    if ((channel_ >= kMinIoMidiChannel) && (channel_ <= kMaxIoMidiChannel)) {
        // 2. DATA MAPPING
        io.midiOutChannel = channel_;
        io.midi.txChannel = channel_;

        // 3. UI & GLOBAL REFRESH
        if (menu == IO_MENU) lcd_drawIo_MidiOutData();
        lcd_updateGlobalMenuData(ENC_IO_MIDIOUT);

        // 4. DEFERRED PRESET PERSISTENCE
        if (systemPresetSaveActive) {
            stopSystemPresetTimer();
            startSystemPresetTimer();
        }
    }
}

