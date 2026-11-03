// Keyboard, encoders and LEDs.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Keyboard Functions --------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::keyboard_initialize() {
    /// @brief Initializes keyboard MCP23017 expanders and primes baseline key states.
    /// Configures input direction, pull-ups, interrupt-on-change, and startup state capture.

    // 1. DEVICE ADDRESS ASSIGNMENT
    // Shift 7-bit MCP23017 addresses to STM32 HAL 8-bit transfer format.
    keyboard.keyLayer.address = (MCP23017_I2C_ADDRESS | 0x25) << 1;
    keyboard.keyLeft.address = (MCP23017_I2C_ADDRESS | 0x20) << 1;
    keyboard.keyRight.address = (MCP23017_I2C_ADDRESS | 0x21) << 1;

    // 2. REGISTER TEMPLATE PREPARATION
    uint8_t data0 = 0x00;        // Used for Polarity/Control
    uint8_t data1 = 0xFF;        // Used for Input Direction/Pull-ups
    uint8_t data2 = 0b01000000;  // IOCON configuration (Mirroring, etc.)
    uint8_t tempState[2];

    // 3. EXPANDER REGISTER CONFIGURATION
    // Same register sequence is applied to Layer, Left, and Right key expanders.

    /* IODIRA/B    (0xFF): Set all 16 pins as Inputs.
       IPOLA/B     (0x00): Maintain normal polarity (not inverted).
       GPPUA/B     (0xFF): Enable 100kΩ internal pull-up resistors.
       DEFVALA/B   (0xFF): Set default comparison value for interrupts to HIGH.
       INTCONA/B   (0x00): Compare against previous pin state for interrupts.
       GPINTENA/B  (0xFF): Enable interrupt-on-change for all pins.
       IOCONA/B    (0x40): Configure hardware (e.g., BANK bit, SEQOP).
    */

    // Configuration for keyLayer
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_IODIRA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_IODIRB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_IPOLA, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_IPOLB, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_GPPUA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_GPPUB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_DEFVALA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_DEFVALB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_INTCONA, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_INTCONB, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_GPINTENA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_GPINTENB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_IOCONA, I2C_MEMADD_SIZE_8BIT, &data2, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLayer.address, MCP23017_IOCONB, I2C_MEMADD_SIZE_8BIT, &data2, 1, 1);
    HAL_I2C_Mem_Read(&hi2c3, keyboard.keyLayer.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, tempState, 2, 1);
    keyboard.keyLayer.previousState = (tempState[1] << 8) + tempState[0];

    // Configuration for keyLeft
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_IODIRA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_IODIRB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_IPOLA, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_IPOLB, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_GPPUA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_GPPUB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_DEFVALA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_DEFVALB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_INTCONA, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_INTCONB, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_GPINTENA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_GPINTENB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_IOCONA, I2C_MEMADD_SIZE_8BIT, &data2, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyLeft.address, MCP23017_IOCONB, I2C_MEMADD_SIZE_8BIT, &data2, 1, 1);
    HAL_I2C_Mem_Read(&hi2c3, keyboard.keyLeft.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, tempState, 2, 1);
    keyboard.keyLeft.previousState = (tempState[1] << 8) + tempState[0];

    // Configuration for keyRight
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_IODIRA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_IODIRB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_IPOLA, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_IPOLB, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_GPPUA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_GPPUB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_DEFVALA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_DEFVALB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_INTCONA, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_INTCONB, I2C_MEMADD_SIZE_8BIT, &data0, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_GPINTENA, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_GPINTENB, I2C_MEMADD_SIZE_8BIT, &data1, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_IOCONA, I2C_MEMADD_SIZE_8BIT, &data2, 1, 1);
    HAL_I2C_Mem_Write(&hi2c3, keyboard.keyRight.address, MCP23017_IOCONB, I2C_MEMADD_SIZE_8BIT, &data2, 1, 1);
    HAL_I2C_Mem_Read(&hi2c3, keyboard.keyRight.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, tempState, 2, 1);
    keyboard.keyRight.previousState = (tempState[1] << 8) + tempState[0];

    // 4. INPUT RESTORATION
    // Enable keyboard event processing after hardware setup is complete.
    keyboard_enable();
}

void Controller::keyboard_read() {
    /// @brief Executes non-blocking keyboard matrix scan over I2C3 using DMA.
    /// Reads Layer/Left/Right expanders through an IDLE-SETTLING-READING state machine.

    enum ScanState {
        STATE_IDLE,      // Waiting for an interrupt/read request
        STATE_SETTLING,  // Waiting for signal stability (Debounce)
        STATE_READING    // DMA transfer in progress
    };

    static ScanState currentState = STATE_IDLE;
    static uint32_t delayStart = 0;
    static uint8_t targetKeypad = 0;  // 0: Layer, 1: Left, 2: Right

    // 1. REQUEST ARBITRATION (IDLE)
    if (currentState == STATE_IDLE) {
        // Layer keypad has priority over Left and Right keypads.
        if (keyboard.keyLayer.readFlag) {
            targetKeypad = 0;
            keyboard.keyLayer.readFlag = false;
        } else if (keyboard.keyLeft.readFlag) {
            targetKeypad = 1;
            keyboard.keyLeft.readFlag = false;
        } else if (keyboard.keyRight.readFlag) {
            targetKeypad = 2;
            keyboard.keyRight.readFlag = false;
        } else {
            return;  // No active requests; exit immediately to keep the main loop fast
        }

        // Start debounce/settling timer before issuing DMA read.
        delayStart = HAL_GetTick();
        currentState = STATE_SETTLING;
    }

    // 2. SIGNAL SETTLING & DMA REQUEST (SETTLING)
    if (currentState == STATE_SETTLING) {
        // Wait at least 1 ms for stable input state.
        if (HAL_GetTick() - delayStart >= 1) {
            // Issue DMA read only when I2C bus is ready.
            if (HAL_I2C_GetState(&hi2c3) == HAL_I2C_STATE_READY) {
                Keypad* kp = (targetKeypad == 0) ? &keyboard.keyLayer : (targetKeypad == 1) ? &keyboard.keyLeft
                                                                                            : &keyboard.keyRight;

                // Start asynchronous GPIO register read via DMA.
                if (HAL_I2C_Mem_Read_DMA(&hi2c3, kp->address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c3_rxData, 2) == HAL_OK) {
                    currentState = STATE_READING;
                } else {
                    // 2.1 ERROR HANDLING
                    // Recover from bus lock/failure and return to IDLE.
                    keyboard_resetI2C();
                    currentState = STATE_IDLE;
                }
            }
        }
        return;  // Yield while DMA transfer is in progress.
    }

    // 3. DMA RESULT PARSING (READING)
    if (currentState == STATE_READING) {
        // Parse data only after DMA and I2C transaction complete.
        if (HAL_I2C_GetState(&hi2c3) == HAL_I2C_STATE_READY) {
            Keypad* kp = (targetKeypad == 0) ? &keyboard.keyLayer : (targetKeypad == 1) ? &keyboard.keyLeft
                                                                                        : &keyboard.keyRight;

            // Reconstruct 16-bit GPIO state from port A/B bytes.
            uint16_t newState = (i2c3_rxData[1] << 8) | i2c3_rxData[0];

            // Continue only when key state changed.
            if (newState != kp->previousState) {
                kp->currentMillis = HAL_GetTick();

                // Avoid overwriting event buffer while consumer still processes it.
                if (!kp->read) {
                    for (uint8_t i = 0; i < 16; i++) {
                        uint8_t preBit = (kp->previousState >> i) & 0x01;
                        uint8_t curBit = (newState >> i) & 0x01;
                        kp->changeState[i] = KEYSTATE_NONE;

                        // Edge detection (press/release transition)
                        if (preBit != curBit) {
                            // Active-low mapping: 0 = pressed, 1 = released.
                            (curBit == 0) ? kp->changeState[i] = KEYSTATE_PRESS
                                          : kp->changeState[i] = KEYSTATE_RELEASE;
                        }
                    }
                    kp->read = true;  // Notify keyboard_action() that new key events are ready.
                }

                // STATE UPDATE
                kp->previousState = newState;
                kp->previousMillis = kp->currentMillis;
            }

            // RESOURCE CLEANUP: cycle complete, return to IDLE.
            currentState = STATE_IDLE;
        }
    }
}

void Controller::keyboard_action() {
    /// @brief Dispatches keyboard events to UI navigation and engine control actions.
    /// Processes Layer, Left, and Right keypad event buffers in priority order.

    // 1. INPUT GATE CHECK
    if (keyboardActive) {
        // AUTOSAVE: any key counts as a possible change and restarts the idle wait.
        if (keyboard.keyLayer.read || keyboard.keyLeft.read || keyboard.keyRight.read) autosave_noteInput();

        // VISUALS mode takes its steering keys first (and leaves before a menu key).
        visual_filterKeys();

        // 2. LAYER KEYPAD DISPATCH
        if (keyboard.keyLayer.read) {
            for (uint8_t layerNum = 0; layerNum < kLayerLibrarySize; layerNum++) {
                switch (keyboard.keyLayer.changeState[layerNum]) {
                    case KEYSTATE_PRESS:
                        layerSongAddPending = false;  // ADD was used as a modifier, not a tap
                        synthAddPending = false;
                        if (keyboard.muteKeyPress) {
                            layerInst_setMute(layerNum, !layerLibrary[layerNum].mute);
                        } else if (keyboard.fillKeyPress) {
                            layerInst_setFill(layerNum, !layerLibrary[layerNum].fill);
                        } else if (keyboard.styleKeyPress) {
                            layerInst_setStyle(layerNum, !layerLibrary[layerNum].style);
                        } else if ((keyboard.lfoKeyPress) && (!alertFlag)) {
                            lfo_select(layerNum);
                        } else if ((keyboard.layerInstKeyPress) && (!alertFlag)) {
                            layerInst_select(layerNum);
                        } else if ((keyboard.layerSongKeyPress) && (!alertFlag)) {
                            layerSong_select(layerNum, activeSongBank);
                        } else if ((keyboard.bankKeyPress) && (!alertFlag)) {
                            uint8_t selectedSongBank = layerNum / 2;
                            (playActive) ? layerSong_triggerBank(selectedSongBank) : layerSong_selectBank(selectedSongBank);
                            ledLayerPlayMode = false;
                            ledLayerSelectFlag = true;
                            ledLayerSelectData = 0x03 << (selectedSongBank * 2);
                        } else if ((keyboard.encKeyPress) && (!alertFlag)) {
                            uint8_t selectedEncoderBank = layerNum / 2;
                            encoder_selectBank(selectedEncoderBank);
                            ledLayerPlayMode = false;
                            ledLayerSelectFlag = true;
                            ledLayerSelectData = 0x03 << (selectedEncoderBank * 2);
                        } else if (noteKeysActive() && (!alertFlag)) {
                            // NOTE page of a TONE layer: the 10 pads are a scale keyboard for it.
                            note_padPressed(layerNum);
                        } else {
                            Layer& layer = layerLibrary[layerNum];
                            // Layer Inst menu: the selected layer's pad auditions the *selected*
                            // sample while it differs from the loaded one. Not while recording,
                            // which writes beats that play the loaded sound.
                            const bool inInstMenu = (menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU);
                            const bool auditionSelected = inInstMenu && previewActive && (!recordActive) && (layerNum == selectedLayerNum) &&
                                                          (layer.toneMode != tone::kModeSynth) &&
                                                          (layer.sampleSelected != -1) &&
                                                          (strcmp(layer.sampleSelectedData.fileName, layer.sampleLoadedData.fileName) != 0);
                            if (auditionSelected) {
                                preview_start(layerNum);
                            } else if ((!layer.mute) && layerSounds(layerNum)) {
                                // TONE layers: on the NOTE page the pad plays the selected beat's note.
                                layer_playBeat(layerNum, 1.0f, note_auditionNote(layerNum));
                            }
                            if (recordActive) {
                                if (!metronome.precountState) {
                                    layerSong_setBeat(layerNum, activeSongBank, playInterval, 0, 0, 0, 0, true);
                                } else {
                                    // Hit landed during precount; commit it to position 0 once playback starts.
                                    precountPendingBeat[layerNum] = true;
                                }
                            }
                        }
                        break;

                    case KEYSTATE_RELEASE:
                        note_padReleased(layerNum);  // PAD HOLD: the held note ends with its pad
                        break;

                    case KEYSTATE_NONE:
                        break;
                }
            }
            keyboard.keyLayer.read = false;
        }

        // 3. LEFT KEYPAD DISPATCH
        if (keyboard.keyLeft.read) {
            for (uint8_t i = 0; i < 16; i++) {
                switch (keyboard.keyLeft.changeState[i]) {
                    case KEYSTATE_PRESS:
                        if (i != KEY_ADD) layerSongAddPending = false;  // ADD used as a modifier, not a tap
                        switch (i) {
                            case KEY_RESET:
                                if (!alertFlag) {
                                    (playActive) ? triggerReset() : reset();
                                }
                                break;

                            case KEY_PLAYSTOP:
                                if (!alertFlag) {
                                    if ((playActive) && (!stopFlag)) {
                                        triggerStop();
                                    } else if (io.sync.slaveMode && (io.syncIn == 2)) {
                                        playArmed = !playArmed;
                                        playIcon.flag = true;
                                        playIcon.mode = playArmed;
                                        (playArmed || recordArmed) ? textArmedFlag = true : textClearFlag = true;
                                    } else if (io.sync.slaveMode && io.syncIn) {
                                        playArmed = !playArmed;
                                        playIcon.flag = true;
                                        playIcon.mode = playArmed;
                                        (playArmed || recordArmed) ? textArmedFlag = true : textClearFlag = true;
                                    } else if (io.sync.slaveMode && io.midi.rxTransport) {
                                        playArmed = !playArmed;
                                        playIcon.flag = true;
                                        playIcon.mode = playArmed;
                                        (playArmed || recordArmed) ? textArmedFlag = true : textClearFlag = true;
                                    } else if (io.sync.slaveMode && io.midi.rxClock) {
                                        play();
                                    } else {
                                        play();
                                    }
                                }
                                break;


                            case KEY_REC:
                                if (!alertFlag) {
                                    if (io.sync.slaveMode && io.syncIn) {
                                        if (playActive) {
                                            record();
                                        } else {
                                            recordArmed = !recordArmed;
                                            recordIcon.flag = true;
                                            recordIcon.mode = recordArmed;
                                            (playArmed || recordArmed) ? textArmedFlag = true : textClearFlag = true;
                                        }
                                    } else if (io.sync.slaveMode && io.midi.rxTransport) {
                                        if (playActive) {
                                            record();
                                        } else {
                                            recordArmed = !recordArmed;
                                            recordIcon.flag = true;
                                            recordIcon.mode = recordArmed;
                                            (playArmed || recordArmed) ? textArmedFlag = true : textClearFlag = true;
                                        }
                                    } else if (io.sync.slaveMode && io.midi.rxClock) {
                                        record();
                                    } else {
                                        record();
                                    }
                                }
                                break;
                            case KEY_UP:
                                if (!alertFlag) {
                                    if (keyboard.shiftKeyPress) synthAddPending = false;  // ADD + UP/DOWN: a modifier
                                    if (upDownKeyLongActive) {
                                        keyboard.upKeyCounter = 0;
                                        keyboard.upKeyFlag = true;
                                        startUpKeyTimer();
                                    }
                                    switch (menu) {
                                        case FILE_MENU:
                                            file_menuUp();
                                            break;

                                        case DRUMKIT_MENU:
                                            drumkit_menuUp();
                                            break;

                                        case RHYTHM_MENU:
                                            rhythm_menuUp();
                                            break;

                                        case MIXER_MENU:
                                            mixer_menuUp();
                                            break;

                                        case METRO_MENU:
                                            metro_menuUp();
                                            break;

                                        case IO_MENU:
                                            io_menuUp();
                                            break;

                                        case FILTER_0_MENU:

                                        case FILTER_1_MENU:
                                            filter_menuUp();
                                            break;

                                        case EQ_MENU:
                                            eq_menuUp();
                                            break;

                                        case EFFECT_0_MENU:

                                        case EFFECT_1_MENU:
                                            effect_menuUp();
                                            break;

                                        case REVERB_MENU:
                                            reverb_menuUp();
                                            break;

                                        case LFO_MENU:
                                            lfo_menuUp();
                                            break;

                                        case LAYER_INST_0_MENU:

                                        case LAYER_INST_1_MENU:

                                        case LAYER_INST_2_MENU:
                                            layerInst_menuUp();
                                            break;

                                        case LAYER_SONG_MENU:
                                            if (layerSongPage == 0) layerSong_menuUp(0);  // fill type; not on the EUCLID page
                                            if (layerSongPage == 2) {
                                                // NOTE page: UP = keyboard octave up; ADD + UP = SLIDE on for the selected beat.
                                                keyboard.shiftKeyPress ? note_setSlide(true) : note_shiftKeys(true);
                                            }
                                            break;
                                    }
                                }
                                break;

                            case KEY_DOWN:
                                if (!alertFlag) {
                                    if (keyboard.shiftKeyPress) synthAddPending = false;  // ADD + UP/DOWN: a modifier
                                    if (upDownKeyLongActive) {
                                        keyboard.downKeyCounter = 0;
                                        keyboard.downKeyFlag = true;
                                        startDownKeyTimer();
                                    }
                                    switch (menu) {
                                        case FILE_MENU:
                                            file_menuDown();
                                            break;

                                        case DRUMKIT_MENU:
                                            drumkit_menuDown();
                                            break;

                                        case RHYTHM_MENU:
                                            rhythm_menuDown();
                                            break;

                                        case MIXER_MENU:
                                            mixer_menuDown();
                                            break;

                                        case METRO_MENU:
                                            metro_menuDown();
                                            break;

                                        case IO_MENU:
                                            io_menuDown();
                                            break;

                                        case FILTER_0_MENU:

                                        case FILTER_1_MENU:
                                            filter_menuDown();
                                            break;

                                        case EQ_MENU:
                                            eq_menuDown();
                                            break;

                                        case EFFECT_0_MENU:

                                        case EFFECT_1_MENU:
                                            effect_menuDown();
                                            break;

                                        case REVERB_MENU:
                                            reverb_menuDown();
                                            break;

                                        case LFO_MENU:
                                            lfo_menuDown();
                                            break;

                                        case LAYER_INST_0_MENU:

                                        case LAYER_INST_1_MENU:

                                        case LAYER_INST_2_MENU:
                                            layerInst_menuDown();
                                            break;

                                        case LAYER_SONG_MENU:
                                            if (layerSongPage == 0) layerSong_menuDown(0);  // fill type; not on the EUCLID page
                                            if (layerSongPage == 2) {
                                                // NOTE page: DOWN = keyboard octave down; ADD + DOWN = SLIDE off for the selected beat.
                                                keyboard.shiftKeyPress ? note_setSlide(false) : note_shiftKeys(false);
                                            }
                                            break;
                                    }
                                }
                                break;

                            case KEY_LEFT:
                                if (!alertFlag) {
                                    switch (menu) {
                                        case FILE_MENU:
                                            file_menuLeft();
                                            break;

                                        case DRUMKIT_MENU:
                                            drumkit_menuLeft();
                                            break;

                                        case RHYTHM_MENU:
                                            rhythm_menuLeft();
                                            break;

                                        case MIXER_MENU:
                                            mixer_menuLeft();
                                            break;

                                        case METRO_MENU:
                                            metro_menuLeft();
                                            break;

                                        case IO_MENU:
                                            io_menuLeft();
                                            break;

                                        case FILTER_0_MENU:

                                        case FILTER_1_MENU:
                                            filter_menuLeft();
                                            break;

                                        case EQ_MENU:
                                            eq_menuLeft();
                                            break;

                                        case EFFECT_0_MENU:

                                        case EFFECT_1_MENU:
                                            effect_menuLeft();
                                            break;

                                        case REVERB_MENU:
                                            reverb_menuLeft();
                                            break;

                                        case LFO_MENU:
                                            lfo_menuLeft();
                                            break;

                                        case LAYER_INST_0_MENU:

                                        case LAYER_INST_1_MENU:

                                        case LAYER_INST_2_MENU:
                                            layerInst_menuLeft();
                                            break;

                                        case LAYER_SONG_MENU:
                                            (keyboard.shiftKeyPress) ? layerSong_moveSelectedBeat(false) : layerSong_beatLeft();
                                            break;
                                    }
                                }
                                break;

                            case KEY_RIGHT:
                                if (!alertFlag) {
                                    switch (menu) {
                                        case FILE_MENU:
                                            file_menuRight();
                                            break;

                                        case DRUMKIT_MENU:
                                            drumkit_menuRight();
                                            break;

                                        case RHYTHM_MENU:
                                            rhythm_menuRight();
                                            break;

                                        case MIXER_MENU:
                                            mixer_menuRight();
                                            break;

                                        case METRO_MENU:
                                            metro_menuRight();
                                            break;

                                        case IO_MENU:
                                            io_menuRight();
                                            break;

                                        case FILTER_0_MENU:

                                        case FILTER_1_MENU:
                                            filter_menuRight();
                                            break;

                                        case EQ_MENU:
                                            eq_menuRight();
                                            break;

                                        case EFFECT_0_MENU:

                                        case EFFECT_1_MENU:
                                            effect_menuRight();
                                            break;

                                        case REVERB_MENU:
                                            reverb_menuRight();
                                            break;

                                        case LFO_MENU:
                                            lfo_menuRight();
                                            break;

                                        case LAYER_INST_0_MENU:

                                        case LAYER_INST_1_MENU:

                                        case LAYER_INST_2_MENU:
                                            layerInst_menuRight();
                                            break;

                                        case LAYER_SONG_MENU:
                                            (keyboard.shiftKeyPress) ? layerSong_moveSelectedBeat(true) : layerSong_beatRight();
                                            break;
                                    }
                                }
                                break;

                            case KEY_CENTER:
                                // HOME on the home screen: VISUALS mode (v1.14).
                                if (!alertFlag) (menu == GLOBAL_MENU) ? visual_enter() : global_select();
                                break;

                            case KEY_ADD:
                                if (alertFlag) {
                                    alertFlag = false;
                                    switch (alertType) {
                                        case ALERT_MEASUREUP:
                                            lcd_clearAlert();
                                            rhythm_setMeasure(rhythm.measure + 1);
                                            break;

                                        case ALERT_MEASUREDOWN:
                                            lcd_clearAlert();
                                            rhythm_setMeasure(rhythm.measure - 1);
                                            break;

                                        case ALERT_BARUP:
                                            lcd_clearAlert();
                                            rhythm_setBar(rhythm.bar + 1);
                                            break;

                                        case ALERT_BARDOWN:
                                            lcd_clearAlert();
                                            rhythm_setBar(rhythm.bar - 1);
                                            break;

                                        case ALERT_QUANTIZEUP:
                                            lcd_clearAlert();
                                            rhythm_setQuantize(rhythm.quantize + 1);
                                            break;

                                        case ALERT_QUANTIZEDOWN:
                                            lcd_clearAlert();
                                            rhythm_setQuantize(rhythm.quantize - 1);
                                            break;

                                        case ALERT_NEWFILE:
                                            file_newAction();
                                            break;

                                        case ALERT_LOADFILE:
                                            file_loadAction();
                                            break;

                                        case ALERT_SAVEFILE:

                                        case ALERT_OVERWRITEFILE:
                                            file_saveAction();
                                            break;

                                        case ALERT_CLEARFILE:
                                            file_clearAction();
                                            break;

                                        case ALERT_NEWDRUMKIT:
                                            drumkit_newAction();
                                            break;

                                        case ALERT_LOADDRUMKIT:
                                            drumkit_loadAction();
                                            break;

                                        case ALERT_SAVEDRUMKIT:

                                        case ALERT_OVERWRITEDRUMKIT:
                                            drumkit_saveAction();
                                            break;

                                        case ALERT_CLEARDRUMKIT:
                                            drumkit_clearAction();
                                            break;

                                        case ALERT_RESUME:
                                            autosave_answerResume(true);
                                            break;
                                    }
                                } else if (keyboard.muteKeyPress) {
                                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                                        if (!layerLibrary[i].mute) layerInst_setMute(i, true);
                                    }
                                } else if (keyboard.fillKeyPress) {
                                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                                        if (!layerLibrary[i].fill) layerInst_setFill(i, true);
                                    }
                                } else if (keyboard.styleKeyPress) {
                                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                                        if (!layerLibrary[i].style) layerInst_setStyle(i, true);
                                    }
                                } else {
                                    keyboard.shiftKeyPress = true;
                                    switch (menu) {
                                        case FILE_MENU:
                                            switch (menuTab) {
                                                case 0:
                                                    file_newSelect();
                                                    break;

                                                case 1:
                                                    file_loadSelect();
                                                    break;

                                                case 2:
                                                    file_saveSelect();
                                                    break;

                                                case 3:
                                                    file_clearSelect();
                                                    break;
                                            }
                                            break;

                                        case DRUMKIT_MENU:
                                            switch (menuTab) {
                                                case 0:
                                                    drumkit_newSelect();
                                                    break;

                                                case 1:
                                                    drumkit_loadSelect();
                                                    break;

                                                case 2:
                                                    drumkit_saveSelect();
                                                    break;

                                                case 3:
                                                    drumkit_clearSelect();
                                                    break;
                                            }
                                            break;

                                        case RHYTHM_MENU:
                                            rhythmLockKeyFlag = true;
                                            startLongKeyTimer();
                                            break;

                                        case MIXER_MENU:
                                            switch (menuTab) {
                                                case 0:
                                                    if (mixer.masterMute) {
                                                        masterUnmuteKeyFlag = true;
                                                        masterMuteKeyFlag = false;
                                                        startLongKeyTimer();
                                                    }
                                                    break;

                                                case 1:
                                                    if (mixer.hpMute) {
                                                        hpUnmuteKeyFlag = true;
                                                        hpMuteKeyFlag = false;
                                                        startLongKeyTimer();
                                                    }
                                                    break;

                                                case 2:
                                                    if (mixer.loMute) {
                                                        loUnmuteKeyFlag = true;
                                                        loMuteKeyFlag = false;
                                                        startLongKeyTimer();
                                                    }
                                                    break;

                                                case 3:
                                                    if (mixer.liMute) {
                                                        liUnmuteKeyFlag = true;
                                                        liMuteKeyFlag = false;
                                                        startLongKeyTimer();
                                                    }
                                                    break;
                                            }
                                            break;

                                        case EQ_MENU:
                                            break;

                                        case LAYER_INST_0_MENU:

                                        case LAYER_INST_1_MENU:

                                        case LAYER_INST_2_MENU:
                                            if (synthLayerSelected()) {
                                                // SYNTH: SOUND page = save to USER slot; other sound
                                                // pages = random sound (hold) / variation (tap).
                                                synth_addPressed();
                                                break;
                                            }
                                            if (menuTab < 4) {
                                                if (!((strcmp(layerLibrary[selectedLayerNum].instSelectedData.fileName, layerLibrary[selectedLayerNum].instLoadedData.fileName) == 0) && (strcmp(layerLibrary[selectedLayerNum].sampleSelectedData.fileName, layerLibrary[selectedLayerNum].sampleLoadedData.fileName) == 0))) {
                                                    layerInst_setInstLoaded(selectedLayerNum);
                                                    layerInst_setSampleLoaded(selectedLayerNum);
                                                }
                                            } else {
                                                lcd_transitionSelect();
                                            }
                                            break;

                                        case LAYER_SONG_MENU:
                                            // A tap (release without long press or another key) inserts one
                                            // beat; holding ADD on an empty layer still runs the generator.
                                            layerSongAddPending = true;
                                            if (layerSongPage == 2) {
                                                // NOTE page: tap = melody variation, hold = new melody
                                                // (both on release); ADD + pad = step entry.
                                                noteMelodyLongFlag = false;
                                                if (layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum != -1) {
                                                    noteMelodyKeyFlag = true;
                                                    startLongKeyTimer();
                                                }
                                            } else if (layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum == -1) {
                                                layerBeatKeyFlag = true;
                                                layerBeatKeyStage = 0;
                                                startLongKeyTimer();
                                            }
                                            break;

                                        default:
                                            break;
                                    }
                                }
                                break;

                            case KEY_CLEAR:
                                if (alertFlag) {
                                    lcd_clearAlert();
                                    alertFlag = false;
                                    if (alertType == ALERT_RESUME) autosave_answerResume(false);
                                } else if (keyboard.muteKeyPress) {
                                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                                        if (layerLibrary[i].mute) layerInst_setMute(i, false);
                                    }
                                } else if (keyboard.fillKeyPress) {
                                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                                        if (layerLibrary[i].fill) layerInst_setFill(i, false);
                                    }
                                } else if (keyboard.styleKeyPress) {
                                    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                                        if (layerLibrary[i].style) layerInst_setStyle(i, false);
                                    }
                                } else {
                                    switch (menu) {
                                        case RHYTHM_MENU:
                                            rhythmUnlockKeyFlag = true;
                                            startLongKeyTimer();
                                            break;

                                        case MIXER_MENU:
                                            switch (menuTab) {
                                                case 0:
                                                    if (!mixer.masterMute) {
                                                        masterUnmuteKeyFlag = false;
                                                        masterMuteKeyFlag = true;
                                                        startLongKeyTimer();
                                                    }
                                                    break;

                                                case 1:
                                                    if (!mixer.hpMute) {
                                                        hpUnmuteKeyFlag = false;
                                                        hpMuteKeyFlag = true;
                                                        startLongKeyTimer();
                                                    }
                                                    break;

                                                case 2:
                                                    if (!mixer.loMute) {
                                                        loUnmuteKeyFlag = false;
                                                        loMuteKeyFlag = true;
                                                        startLongKeyTimer();
                                                    }
                                                    break;

                                                case 3:
                                                    if (!mixer.liMute) {
                                                        liUnmuteKeyFlag = false;
                                                        liMuteKeyFlag = true;
                                                        startLongKeyTimer();
                                                    }
                                                    break;
                                            }
                                            break;

                                        case LFO_MENU:
                                            lfoResetKeyFlag = true;
                                            startLongKeyTimer();
                                            break;

                                        case LAYER_INST_0_MENU:

                                        case LAYER_INST_1_MENU:

                                        case LAYER_INST_2_MENU:
                                            layerInstResetKeyFlag = true;
                                            startLongKeyTimer();
                                            break;

                                        case LAYER_SONG_MENU:
                                            // On every page, holding CLEAR erases all of the layer's beats in this
                                            // bank. A tap erases the selected beat (edit page) or undoes the page's
                                            // edits (EUCLID: beats, NOTE: notes); see the release handler.
                                            if (layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum != -1) {
                                                layerSongResetKeyFlag = true;
                                                startLongKeyTimer();
                                            } else if (layerSongPage == 1) {
                                                euclid_restore();  // empty layer: nothing to erase, undo at once
                                            } else if (layerSongPage == 2) {
                                                note_restore();
                                            }
                                            break;

                                        default:
                                            break;
                                    }
                                }
                                break;

                            case KEY_COPY:
                                switch (menu) {
                                    case LAYER_INST_0_MENU:

                                    case LAYER_INST_1_MENU:

                                    case LAYER_INST_2_MENU:
                                        layerInstCopyKeyFlag = true;
                                        startLongKeyTimer();
                                        break;

                                    case LAYER_SONG_MENU:
                                        layerFillCopyKeyFlag = true;
                                        layerSongCopyKeyFlag = true;
                                        startLongKeyTimer();
                                        break;

                                    case LFO_MENU:
                                        lfoCopyKeyFlag = true;
                                        startLongKeyTimer();
                                        break;
                                }
                                break;

                            case KEY_PASTE:
                                switch (menu) {
                                    case LAYER_INST_0_MENU:

                                    case LAYER_INST_1_MENU:

                                    case LAYER_INST_2_MENU:
                                        layerInstPasteKeyFlag = true;
                                        startLongKeyTimer();
                                        break;

                                    case LAYER_SONG_MENU:
                                        layerFillPasteKeyFlag = true;
                                        layerSongPasteKeyFlag = true;
                                        startLongKeyTimer();
                                        break;

                                    case LFO_MENU:
                                        lfoPasteKeyFlag = true;
                                        startLongKeyTimer();
                                        break;
                                }
                                break;

                            case KEY_MUTE:
                                keyboard.muteKeyPress = true;
                                break;

                            case KEY_FILL:
                                keyboard.fillKeyPress = true;
                                encoderFastMode = true;
                                break;

                            case KEY_STYLE:
                                keyboard.styleKeyPress = true;
                                encoderSlowMode = true;
                                break;

                            case KEY_KNOB:
                                keyboard.encKeyPress = true;
                                ledLayerPlayMode = false;
                                ledLayerSelectFlag = true;
                                // Guard against corrupted bank index before bit-shift operation.
                                uint8_t bankNum = activeEncoderBank;
                                if (bankNum >= kEncoderBankSize) bankNum = kMinEncoderGlobalTab;
                                activeEncoderBank = bankNum;
                                ledLayerSelectData = 0x03 << (bankNum * 2);
                                break;
                        }
                        break;

                    case KEYSTATE_RELEASE:
                        switch (i) {
                            case KEY_UP:
                                if (keyboard.upKeyFlag) {
                                    stopUpKeyTimer();
                                    keyboard.upKeyFlag = false;
                                    keyboard.upKeyCounter = 0;
                                }
                                break;

                            case KEY_DOWN:
                                if (keyboard.downKeyFlag) {
                                    stopDownKeyTimer();
                                    keyboard.downKeyFlag = false;
                                    keyboard.downKeyCounter = 0;
                                }
                                break;

                            case KEY_ADD: {
                                // Decide on the tap-insert before the long-press flags are cleared.
                                // Not on the EUCLID page: the next knob turn would regenerate over it.
                                const bool insertBeat = layerSongAddPending && (menu == LAYER_SONG_MENU) && (layerSongPage == 0) && !alertFlag && (selectedLayerNum != -1);
                                // NOTE page: a tap (no long press, no other key) varies the melody.
                                const bool mutateMelody = layerSongAddPending && (menu == LAYER_SONG_MENU) && (layerSongPage == 2) && !alertFlag && (selectedLayerNum != -1);
                                layerSongAddPending = false;
                                if (noteMelodyKeyFlag) {
                                    keyboard.longKeyCounter = 0;
                                    noteMelodyKeyFlag = false;
                                }
                                stopLongKeyTimer();
                                keyboard.shiftKeyPress = false;
                                masterUnmuteKeyFlag = false;
                                hpUnmuteKeyFlag = false;
                                loUnmuteKeyFlag = false;
                                liUnmuteKeyFlag = false;
                                if (layerBeatKeyFlag) {
                                    keyboard.longKeyCounter = 0;
                                    layerBeatKeyFlag = false;
                                    layerBeatKeyStage = 0;
                                }
                                if (rhythmLockKeyFlag) {
                                    keyboard.longKeyCounter = 0;
                                    rhythmLockKeyFlag = false;
                                }
                                synth_addReleased();  // SYNTH pages: random sound / variation, if ADD was not a modifier
                                if (insertBeat) layerSong_insertBeat();
                                // NOTE page: held long = a new melody, short = a variation; nothing
                                // if ADD was used with a pad or UP/DOWN (mutateMelody is false then).
                                if (mutateMelody) note_melody(noteMelodyLongFlag);
                                noteMelodyLongFlag = false;
                            } break;

                            case KEY_CLEAR:
                                stopLongKeyTimer();
                                masterMuteKeyFlag = false;
                                hpMuteKeyFlag = false;
                                loMuteKeyFlag = false;
                                liMuteKeyFlag = false;
                                if (lfoResetKeyFlag) {
                                    keyboard.longKeyCounter = 0;
                                    lfoResetKeyFlag = false;
                                }
                                if (layerInstResetKeyFlag) {
                                    // Released before the long press (reset): a tap. On a synth page
                                    // it undoes back to the sound from when the menu was opened.
                                    keyboard.longKeyCounter = 0;
                                    layerInstResetKeyFlag = false;
                                    if (synthInstPageActive()) synth_undo();
                                }
                                if (layerSongResetKeyFlag) {
                                    // Released before the long press: a tap.
                                    keyboard.longKeyCounter = 0;
                                    layerSongResetKeyFlag = false;
                                    if (layerSongPage == 1) {
                                        euclid_restore();  // EUCLID page: back to the beats from when the page opened
                                    } else if (layerSongPage == 2) {
                                        note_restore();    // NOTE page: back to the notes from when the page opened
                                    } else {
                                        layerSong_resetSelectedBeat();
                                    }
                                }
                                if (rhythmUnlockKeyFlag) {
                                    keyboard.longKeyCounter = 0;
                                    rhythmUnlockKeyFlag = false;
                                }
                                break;

                            case KEY_COPY:
                                if (layerInstCopyKeyFlag) {
                                    stopLongKeyTimer();
                                    keyboard.longKeyCounter = 0;
                                    layerInstCopyKeyFlag = false;
                                }
                                if (layerFillCopyKeyFlag) {
                                    stopLongKeyTimer();
                                    keyboard.longKeyCounter = 0;
                                    if (layerSongCopyKeyFlag) {
                                        layerFillCopyKeyFlag = false;
                                        layerSongCopyKeyFlag = false;
                                        if ((layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum != -1) && (selectedBeatNum != -1)) {
                                            Beat& beatRef = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].beatLibrary[selectedBeatNum];
                                            fillCopy.active = true;
                                            fillCopy.type = beatRef.getFillType();
                                            fillCopy.pattern = beatRef.getFillPattern();
                                            fillCopy.time = beatRef.getFillTime();
                                            fillCopy.level = beatRef.getFillLevel();
                                        }
                                    } else {
                                        layerFillCopyKeyFlag = false;
                                    }
                                }
                                if (lfoCopyKeyFlag) {
                                    stopLongKeyTimer();
                                    keyboard.longKeyCounter = 0;
                                    lfoCopyKeyFlag = false;
                                }
                                break;

                            case KEY_PASTE:
                                if (layerInstPasteKeyFlag) {
                                    stopLongKeyTimer();
                                    keyboard.longKeyCounter = false;
                                    layerInstPasteKeyFlag = false;
                                }
                                if (layerFillPasteKeyFlag) {
                                    stopLongKeyTimer();
                                    keyboard.longKeyCounter = 0;
                                    if (layerSongPasteKeyFlag) {
                                        if ((fillCopy.active) && (layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum != -1) && (selectedBeatNum != -1)) {
                                            layerSong_setSelectedBeatFill(fillCopy.type, fillCopy.pattern, fillCopy.time, fillCopy.level);
                                        }
                                        layerFillPasteKeyFlag = false;
                                        layerSongPasteKeyFlag = false;
                                    } else {
                                        layerFillPasteKeyFlag = false;
                                    }
                                }
                                if (lfoPasteKeyFlag) {
                                    stopLongKeyTimer();
                                    keyboard.longKeyCounter = 0;
                                    lfoPasteKeyFlag = false;
                                }
                                break;

                            case KEY_MUTE:
                                keyboard.muteKeyPress = false;
                                break;

                            case KEY_FILL:
                                keyboard.fillKeyPress = false;
                                encoderFastMode = false;
                                break;

                            case KEY_STYLE:
                                keyboard.styleKeyPress = false;
                                encoderSlowMode = false;
                                break;

                            case KEY_KNOB:
                                keyboard.encKeyPress = false;
                                switch (menu) {
                                    case LFO_MENU:
                                        ledLayerPlayMode = false;
                                        ledLayerSelectFlag = true;
                                        ledLayerSelectData = 0x01 << selectedLfoNum;
                                        break;

                                    case LAYER_INST_0_MENU:

                                    case LAYER_INST_1_MENU:

                                    case LAYER_INST_2_MENU:

                                    case LAYER_SONG_MENU:
                                        ledLayerPlayMode = false;
                                        ledLayerSelectFlag = true;
                                        ledLayerSelectData = 0x01 << selectedLayerNum;
                                        break;

                                    default:
                                        ledLayerPlayMode = true;
                                        ledLayerSelectFlag = true;
                                        ledLayerSelectData = 0x00;
                                        break;
                                }
                                break;
                        }
                        break;

                    case KEYSTATE_NONE:
                        break;
                }
            }
            keyboard.keyLeft.read = false;
        }

        // 4. RIGHT KEYPAD DISPATCH
        if (keyboard.keyRight.read) {
            for (uint8_t i = 0; i < 16; i++) {
                switch (keyboard.keyRight.changeState[i]) {
                    case KEYSTATE_PRESS:
                        layerSongAddPending = false;  // ADD used as a modifier, not a tap
                        switch (i) {
                            case KEY_FILE:
                                if (!alertFlag) file_select();
                                break;

                            case KEY_DRUMKIT:
                                if (!alertFlag) drumkit_select();
                                break;

                            case KEY_IO:
                                if (!alertFlag) io_select();
                                break;

                            case KEY_RHYTHM:
                                if (!alertFlag) rhythm_select();
                                break;

                            case KEY_METRONOME:
                                if (!alertFlag) metro_select();
                                break;

                            case KEY_MIXER:
                                if (!alertFlag) mixer_select();
                                break;

                            case KEY_FILTER_0:
                                if (!alertFlag) filter_select(0);
                                break;

                            case KEY_FILTER_1:
                                if (!alertFlag) filter_select(1);
                                break;

                            case KEY_EQ:
                                if (!alertFlag) eq_select();
                                break;

                            case KEY_EFFECT_0:
                                if (!alertFlag) effect_select(0);
                                break;

                            case KEY_EFFECT_1:
                                if (!alertFlag) effect_select(1);
                                break;

                            case KEY_REVERB:
                                if (!alertFlag) reverb_select();
                                break;

                            case KEY_LFO:
                                keyboard.lfoKeyPress = true;
                                break;

                            case KEY_LAYER_INST:
                                keyboard.layerInstKeyPress = true;
                                break;

                            case KEY_LAYER_SONG:
                                keyboard.layerSongKeyPress = true;
                                break;

                            case KEY_SONG:
                                keyboard.bankKeyPress = true;
                                ledLayerPlayMode = false;
                                ledLayerSelectFlag = true;
                                ledLayerSelectData = 0x03 << (activeSongBank * 2);
                                break;
                        }
                        break;

                    case KEYSTATE_RELEASE:
                        switch (i) {
                            case KEY_LFO:
                                keyboard.lfoKeyPress = false;
                                break;

                            case KEY_LAYER_INST:
                                keyboard.layerInstKeyPress = false;
                                break;

                            case KEY_LAYER_SONG:
                                keyboard.layerSongKeyPress = false;
                                break;

                            case KEY_SONG:
                                keyboard.bankKeyPress = false;
                                switch (menu) {
                                    case LFO_MENU:
                                        ledLayerPlayMode = false;
                                        ledLayerSelectFlag = true;
                                        ledLayerSelectData = 0x01 << selectedLfoNum;
                                        break;

                                    case LAYER_INST_0_MENU:

                                    case LAYER_INST_1_MENU:

                                    case LAYER_INST_2_MENU:

                                    case LAYER_SONG_MENU:
                                        ledLayerPlayMode = false;
                                        ledLayerSelectFlag = true;
                                        ledLayerSelectData = 0x01 << selectedLayerNum;
                                        break;

                                    default:
                                        ledLayerPlayMode = true;
                                        ledLayerSelectFlag = true;
                                        ledLayerSelectData = 0x00;
                                        break;
                                }
                                break;

                            default:
                                break;
                        }
                        break;

                    case KEYSTATE_NONE:
                        break;
                }
            }
            keyboard.keyRight.read = false;
        }
    }
}

void Controller::keyboard_test() {
    /// @brief Displays live keyboard press diagnostics on LCD test coordinates.
    /// Highlights pressed keys for Layer, Left, and Right keypads.

    if (keyboardActive) {
        // 1. LAYER KEYPAD VISUALIZATION
        if (keyboard.keyLayer.read) {
            for (uint8_t layerNum = 0; layerNum < kLayerLibrarySize; layerNum++) {
                switch (keyboard.keyLayer.changeState[layerNum]) {
                    case KEYSTATE_PRESS:

                        // Visual feedback: Highlight the key in green
                        lcd.setForeColor(GREEN);
                        lcd.drawRect(kTestKeyLayerPos[layerNum][0], kTestKeyLayerPos[layerNum][1],
                                     kTestKeySize, kTestKeySize);
                        break;

                    case KEYSTATE_RELEASE:

                    case KEYSTATE_NONE:
                        break;
                }
            }

            // STATE UPDATE
            keyboard.keyLayer.read = false;
        }

        // 2. LEFT KEYPAD VISUALIZATION
        if (keyboard.keyLeft.read) {
            for (uint8_t i = 0; i < 16; i++) {
                switch (keyboard.keyLeft.changeState[i]) {
                    case KEYSTATE_PRESS:

                        // Visual feedback: Highlight the key in green
                        lcd.setForeColor(GREEN);
                        lcd.drawRect(kTestKeyLeftPos[i][0], kTestKeyLeftPos[i][1],
                                     kTestKeySize, kTestKeySize);
                        break;

                    case KEYSTATE_RELEASE:

                    case KEYSTATE_NONE:
                        break;
                }
            }

            // STATE UPDATE
            keyboard.keyLeft.read = false;
        }

        // 3. RIGHT KEYPAD VISUALIZATION
        if (keyboard.keyRight.read) {
            for (uint8_t i = 0; i < 16; i++) {
                switch (keyboard.keyRight.changeState[i]) {
                    case KEYSTATE_PRESS:

                        // Visual feedback: Highlight the key in green
                        lcd.setForeColor(GREEN);
                        lcd.drawRect(kTestKeyRightPos[i][0], kTestKeyRightPos[i][1],
                                     kTestKeySize, kTestKeySize);
                        break;

                    case KEYSTATE_RELEASE:

                    case KEYSTATE_NONE:
                        break;
                }
            }

            // STATE UPDATE
            keyboard.keyRight.read = false;
        }
    }
}

void Controller::keyboard_enable() {
    /// @brief Enables keyboard event processing.

    // 1. FLAG UPDATE
    // Restore key scan/action handling in the main loop.

    keyboardActive = true;
}

void Controller::keyboard_disable() {
    /// @brief Disables keyboard event processing.

    // 1. FLAG UPDATE
    // Prevent key handling during critical sections and heavy I/O operations.

    keyboardActive = false;
}

void Controller::keyboard_upLong() {
    /// @brief Handles long-press UP navigation with menu-specific guard conditions.
    /// Skips non-scrollable tabs and dispatches to the active menu handler.

    // 1. MENU DISPATCH
    switch (menu) {
        case FILE_MENU:
            file_menuUp();
            break;

        case DRUMKIT_MENU:
            drumkit_menuUp();
            break;

        case RHYTHM_MENU:
            rhythm_menuUp();
            break;

        case MIXER_MENU:
            if (menuTab != 7)
                mixer_menuUp();
            break;

        case METRO_MENU:
            if ((menuTab != 0) && (menuTab != 1))
                metro_menuUp();
            break;

        case IO_MENU:
            io_menuUp();
            break;

        case FILTER_0_MENU:

        case FILTER_1_MENU:
            if ((menuTab != 0) && (menuTab != 2)) filter_menuUp();
            break;

        case EQ_MENU:
            if (menuTab != 0)
                eq_menuUp();
            break;

        case EFFECT_0_MENU:

        case EFFECT_1_MENU:
            if (menuTab != 0)
                effect_menuUp();
            break;

        case REVERB_MENU:
            if (menuTab != 0)
                reverb_menuUp();
            break;

        case LFO_MENU:
            if (menuTab != 0)
                lfo_menuUp();
            break;

        case LAYER_INST_0_MENU:

        case LAYER_INST_1_MENU:
            layerInst_menuUp();
            break;

        case LAYER_INST_2_MENU:
            break;

        case LAYER_SONG_MENU:
            if (layerSongPage == 0) layerSong_menuUp(0);  // fill type; not on the EUCLID page
            break;
    }
}

void Controller::keyboard_downLong() {
    /// @brief Handles long-press DOWN navigation with menu-specific guard conditions.
    /// Skips non-scrollable tabs and dispatches to the active menu handler.

    // 1. MENU DISPATCH
    switch (menu) {
        case FILE_MENU:
            file_menuDown();
            break;

        case DRUMKIT_MENU:
            drumkit_menuDown();
            break;

        case RHYTHM_MENU:
            rhythm_menuDown();
            break;

        case MIXER_MENU:
            if (menuTab != 7)
                mixer_menuDown();
            break;

        case METRO_MENU:
            if ((menuTab != 0) && (menuTab != 1))
                metro_menuDown();
            break;

        case IO_MENU:
            io_menuDown();
            break;

        case FILTER_0_MENU:

        case FILTER_1_MENU:
            if ((menuTab != 0) && (menuTab != 2))
                filter_menuDown();
            break;

        case EQ_MENU:
            if (menuTab != 0)
                eq_menuDown();
            break;

        case EFFECT_0_MENU:

        case EFFECT_1_MENU:
            if (menuTab != 0)
                effect_menuDown();
            break;

        case REVERB_MENU:
            if (menuTab != 0)
                reverb_menuDown();
            break;

        case LFO_MENU:
            if (menuTab != 0)
                lfo_menuDown();
            break;

        case LAYER_INST_0_MENU:

        case LAYER_INST_1_MENU:
            layerInst_menuDown();
            break;

        case LAYER_INST_2_MENU:
            break;

        case LAYER_SONG_MENU:
            if (layerSongPage == 0) layerSong_menuDown(0);  // fill type; not on the EUCLID page
            break;
    }
}

void Controller::keyboard_resetI2C() {
    /// @brief Resets I2C3 peripheral hardware to recover from keyboard bus lockups.
    /// Used when MCP23017 transactions fail and the bus state machine stalls.

    // 1. FORCE RESET ASSERT
    // Clear internal peripheral state via RCC reset line.
    __HAL_RCC_I2C3_FORCE_RESET();

    // 2. STABILIZATION DELAY
    // Short wait to guarantee reset propagation.
    HAL_Delay(1);

    // 3. RESET RELEASE
    // Return peripheral to operational state for subsequent HAL transactions.
    __HAL_RCC_I2C3_RELEASE_RESET();
}

////////////////////////////////////////////////////////////////////////////////
/* Encoder Functions ---------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::encoder_initialize() {
    /// @brief Initializes encoder GPIO mappings and default global function matrix.
    /// Prepares all 8 rotary encoders for runtime read/action processing.

    // 1. HARDWARE GPIO MAPPING
    // Assign physical Port/Pin pairs for quadrature channels A and B.
    encoder[0].aPort = ENCO_0A_GPIO_Port;
    encoder[0].aPin = ENCO_0A_Pin;
    encoder[0].bPort = ENCO_0B_GPIO_Port;
    encoder[0].bPin = ENCO_0B_Pin;

    encoder[1].aPort = ENCO_1A_GPIO_Port;
    encoder[1].aPin = ENCO_1A_Pin;
    encoder[1].bPort = ENCO_1B_GPIO_Port;
    encoder[1].bPin = ENCO_1B_Pin;

    encoder[2].aPort = ENCO_2A_GPIO_Port;
    encoder[2].aPin = ENCO_2A_Pin;
    encoder[2].bPort = ENCO_2B_GPIO_Port;
    encoder[2].bPin = ENCO_2B_Pin;

    encoder[3].aPort = ENCO_3A_GPIO_Port;
    encoder[3].aPin = ENCO_3A_Pin;
    encoder[3].bPort = ENCO_3B_GPIO_Port;
    encoder[3].bPin = ENCO_3B_Pin;

    encoder[4].aPort = ENCO_4A_GPIO_Port;
    encoder[4].aPin = ENCO_4A_Pin;
    encoder[4].bPort = ENCO_4B_GPIO_Port;
    encoder[4].bPin = ENCO_4B_Pin;

    encoder[5].aPort = ENCO_5A_GPIO_Port;
    encoder[5].aPin = ENCO_5A_Pin;
    encoder[5].bPort = ENCO_5B_GPIO_Port;
    encoder[5].bPin = ENCO_5B_Pin;

    encoder[6].aPort = ENCO_6A_GPIO_Port;
    encoder[6].aPin = ENCO_6A_Pin;
    encoder[6].bPort = ENCO_6B_GPIO_Port;
    encoder[6].bPin = ENCO_6B_Pin;

    encoder[7].aPort = ENCO_7A_GPIO_Port;
    encoder[7].aPin = ENCO_7A_Pin;
    encoder[7].bPort = ENCO_7B_GPIO_Port;
    encoder[7].bPin = ENCO_7B_Pin;

    // 2. GLOBAL FUNCTION MATRIX RESET
    // Clear global encoder-function map to remove stale assignments.
    for (uint8_t i = 0; i < kEncoderBankSize; i++) {
        for (uint8_t j = 0; j < kEncoderSize; j++) {
            encoderGlobalFunction[i][j] = ENC_FNC_NONE;
        }
    }

    // Sync physical state and reset internal logic counters
    for (uint8_t i = 0; i < kEncoderSize; i++) {
        Encoder& enc = encoder[i];

        // Direct register read to sync quadrature state immediately
        uint8_t a = (enc.aPort->IDR & enc.aPin) ? 2 : 0;
        uint8_t b = (enc.bPort->IDR & enc.bPin) ? 1 : 0;
        enc.abPrevious = a | b;
        enc.incCounter = 0;
        enc.decCounter = 0;
        enc.read = false;
        enc.readFlag = false;
    }

    // 3. INITIAL STATE SYNC & ACTIVATION
    // Sync mappings/resolutions and enable encoder event handling.
    encoder_update();
    encoder_enable();
}

void Controller::encoder_read() {
    /// @brief Velocity-Adaptive Reader with Directional Momentum.
    /// Provides 1-to-1 responsiveness for continuous movement while
    /// requiring a 2-step confirmation only during direction reversals to kill jitter.

    // 1. ASYMMETRIC TRANSITION LUT
    // Mapped for: CW (1<->2) and CCW (0<->3).
    static const int8_t ENCODER_LUT[] = {
        0, 0, 0, -1,
        0, 0, 1, 0,
        0, 1, 0, 0,
        -1, 0, 0, 0};

    // 2. TIMING PARAMETERS
    const uint32_t kIdleTimeoutUs = 2000000;
    const uint32_t kDebounceUs = 700;  // Sweet spot for H723 + 10nF caps

    // Persistence for momentum-based tracking
    static int8_t lastConfirmedDir[8] = {0};  // 1 for CW, -1 for CCW
    static uint8_t reversalCounter[8] = {0};  // Counter to confirm direction change

    for (uint8_t i = 0; i < kEncoderSize; i++) {
        Encoder& enc = encoder[i];
        if (!enc.readFlag) continue;

        // Atomic grab of the captured state from ISR
        uint8_t currentAB = enc.capturedAB;
        enc.readFlag = false;

        enc.millisCurrent = dwt_getMicros();
        uint32_t rawDiff = enc.millisCurrent - enc.millisPrevious;

        // 1. IDLE RESET
        if (rawDiff > kIdleTimeoutUs) {
            enc.abPrevious = currentAB;
            lastConfirmedDir[i] = 0;
            reversalCounter[i] = 0;
        }

        // 2. DEBOUNCE FILTER
        if (rawDiff > kDebounceUs) {
            int8_t direction = ENCODER_LUT[(enc.abPrevious << 2) | currentAB];

            if (direction != 0) {
                // 3. DIRECTIONAL MOMENTUM (The Fix)
                // If moving in the same direction, process immediately (High speed).
                // If direction reverses, require 2 steps to confirm it's not a jitter/vibration.
                bool processStep = false;

                if (direction == lastConfirmedDir[i]) {
                    // Continuing same direction: Instant response
                    processStep = true;
                    reversalCounter[i] = 0;
                } else {
                    // Direction reversal OR first movement after idle
                    if (++reversalCounter[i] >= 2) {
                        processStep = true;
                        lastConfirmedDir[i] = direction;
                        reversalCounter[i] = 0;
                    }
                    // If reversalCounter is 1, we wait (this kills the 15->14->15 jitter)
                }

                if (processStep) {
                    // 4. RESOLUTION SCALING
                    uint8_t res = (keyboard.encKeyPress || keyboard.shiftKeyPress) ? ENC_RES_M : enc.resolution;

                    if (encoderFastMode) {
                        if (res == ENC_RES_S)
                            res = ENC_RES_M;
                        else if (res == ENC_RES_M)
                            res = ENC_RES_F;
                    }

                    // 5. ACTION EMISSION
                    if (direction == 1) {  // CW
                        enc.decCounter = 0;
                        if (++enc.incCounter >= res) {
                            enc.incCounter = 0;
                            enc.actionCurrent = ENC_INC;
                            enc.read = true;
                        }
                    } else {  // CCW
                        enc.incCounter = 0;
                        if (++enc.decCounter >= res) {
                            enc.decCounter = 0;
                            enc.actionCurrent = ENC_DEC;
                            enc.read = true;
                        }
                    }
                }
            }

            // 6. STATE UPDATE
            enc.millisPrevious = enc.millisCurrent;
            enc.abPrevious = currentAB;
        }
    }
}

void Controller::encoder_action() {
    /// @brief Dispatches encoder events to parameter edits or global function reassignment.
    /// Supports local mode editing and global mode remapping workflows.

    // 1. SYSTEM GUARD
    // Skip processing when encoder subsystem is disabled.
    if (encoderActive) {
        if (activeEncoderBank >= kEncoderBankSize) activeEncoderBank = kMinEncoderGlobalTab;

        for (uint8_t encNum_ = 0; encNum_ < kEncoderSize; encNum_++) {
            Encoder& enc = encoder[encNum_];
            uint8_t function = (uint8_t)enc.function;

            // 2. ACTION DISPATCH
            // Process only events promoted by encoder_read().
            if (enc.read) {
                autosave_noteInput();  // AUTOSAVE: a turned knob may have changed the session
                switch (enc.mode) {
                        // --- LOCAL MODE ---
                        // Use menu-bound function mapping.

                    case ENC_MODE_LOCAL:
                        switch (enc.actionCurrent) {
                            case ENC_INC:
                                if (!alertFlag) encoder_incValue(encNum_);
                                break;

                            case ENC_DEC:
                                if (!alertFlag) encoder_decValue(encNum_);
                                break;

                            case ENC_NONE:
                                break;
                        }
                        break;

                        // --- GLOBAL MODE ---
                        // Support dynamic function remap via modifier key.

                    case ENC_MODE_GLOBAL:

                        // Modifier path: update assigned function ID.
                        if (keyboard.encKeyPress) {
                            const uint8_t minFunction = kMinEncoderGlobalFunction;
                            const uint8_t maxFunction = kMaxEncoderGlobalFunction;
                            const uint8_t searchCount = (maxFunction - minFunction) + 1;

                            auto findNextActiveFunction = [&](uint8_t currentFunction, bool increment) {
                                uint8_t candidate = currentFunction;

                                // Wrap through global function range until an active entry is found.
                                for (uint8_t i = 0; i < searchCount; i++) {
                                    if (increment)
                                        candidate = (candidate < maxFunction) ? (candidate + 1) : minFunction;
                                    else
                                        candidate = (candidate > minFunction) ? (candidate - 1) : maxFunction;

                                    if (kEncoderFuncDataLibrary[candidate].active) return candidate;
                                }

                                return currentFunction;
                            };

                            switch (enc.actionCurrent) {
                                case ENC_INC:
                                    function = findNextActiveFunction(function, true);
                                    encoder_setGlobalFunction(activeEncoderBank, encNum_, (EncoderFunc)function);
                                    break;

                                case ENC_DEC:
                                    function = findNextActiveFunction(function, false);
                                    encoder_setGlobalFunction(activeEncoderBank, encNum_, (EncoderFunc)function);
                                    break;

                                case ENC_NONE:
                                    break;
                            }

                            // STATE UPDATE: persist mapping in active global bank.
                            encoderGlobalFunction[activeEncoderBank][encNum_] = (EncoderFunc)function;
                        }

                        // Normal path: adjust value of currently assigned function.
                        else {
                            switch (enc.actionCurrent) {
                                case ENC_INC:
                                    if (!alertFlag) encoder_incValue(encNum_);
                                    break;

                                case ENC_DEC:
                                    if (!alertFlag) encoder_decValue(encNum_);
                                    break;

                                case ENC_NONE:
                                    break;
                            }
                        }
                        break;
                }

                // STATE UPDATE
                enc.actionPrevious = enc.actionCurrent;
            }

            // 3. RESOURCE CLEANUP
            // Clear transient flags for next scan cycle.
            enc.busy = false;
            enc.read = false;
            enc.actionCurrent = ENC_NONE;
        }
    }
}

void Controller::encoder_update() {
    /// @brief Updates encoder mode/function bindings and synchronizes runtime state.

    if (activeEncoderBank >= kEncoderBankSize) activeEncoderBank = kMinEncoderGlobalTab;
    const uint8_t encoderFuncCount = sizeof(kEncoderFuncDataLibrary) / sizeof(kEncoderFuncDataLibrary[0]);
    const Menu menuSafe = ((menu >= INIT_MENU) && (menu <= TEST_MENU)) ? menu : INIT_MENU;

    // 1. MODE/FUNCTION BINDING
    // Bind each encoder to global or local function map based on active menu.
    const bool isGlobal = (menuSafe == INIT_MENU || menuSafe == GLOBAL_MENU);
    for (uint8_t i = 0; i < kEncoderSize; i++) {
        Encoder& enc = encoder[i];
        if (isGlobal) {
            enc.mode = ENC_MODE_GLOBAL;
            enc.function = encoderGlobalFunction[activeEncoderBank][i];
        } else {
            enc.mode = ENC_MODE_LOCAL;
            // Layer Song has two more pages (EUCLID, NOTE) with their own knob maps.
            if ((menuSafe == LAYER_SONG_MENU) && (layerSongPage == 1)) {
                enc.function = kEncoderEuclidFunction[i];
            } else if ((menuSafe == LAYER_SONG_MENU) && (layerSongPage == 2)) {
                enc.function = kEncoderNoteFunction[i];
            } else if (synthInstPageActive()) {
                enc.function = kEncoderSynthFunction[layerInstPage()][i];  // SYNTH layer: OSC / FILTER / routing
            } else {
                enc.function = encoderLocalFunction[menuSafe][i];
            }
        }

        // 2. COUNTER STATE RESET
        // Refresh resolution and clear per-encoder step accumulators.
        uint8_t functionIndex = static_cast<uint8_t>(enc.function);
        if (functionIndex >= encoderFuncCount) {
            enc.function = ENC_FNC_NONE;
            functionIndex = static_cast<uint8_t>(ENC_FNC_NONE);
        }
        enc.resolution = kEncoderFuncDataLibrary[functionIndex].resolution;
        enc.incCounter = 0;
        enc.decCounter = 0;

        // 3. HARDWARE PHASE SYNC
        // Latch current AB pin phase to avoid false deltas after context switch.
        uint8_t a = (enc.aPort->IDR & enc.aPin) ? 2 : 0;
        uint8_t b = (enc.bPort->IDR & enc.bPin) ? 1 : 0;
        enc.abPrevious = a | b;
    }

    // 4. UI INDICATOR REFRESH
    // Redraw encoder activity boxes after mapping/state update.
    for (uint8_t i = 0; i < kEncoderSize; i++) lcd_drawEncoderBox(i);
}

void Controller::encoder_test() {
    /// @brief Renders live encoder direction diagnostics on LCD test view.
    /// Draws green/red indicators for CW/CCW events.

    // 1. SYSTEM GUARD
    if (encoderActive) {
        for (uint8_t encNum_ = 0; encNum_ < kEncoderSize; encNum_++) {
            Encoder& enc = encoder[encNum_];

            // 2. EVENT VISUALIZATION
            // Draw only when a new encoder event exists.
            if (enc.read) {
                switch (enc.actionCurrent) {
                    case ENC_INC:

                        // CW event: draw green marker.
                        lcd.setForeColor(GREEN);
                        lcd.drawCircle(kTestEncoderPos[encNum_][0], kTestEncoderPos[encNum_][1], kTestEncoderSize);
                        break;

                    case ENC_DEC:

                        // CCW event: draw red marker.
                        lcd.setForeColor(RED);
                        lcd.drawCircle(kTestEncoderPos[encNum_][0], kTestEncoderPos[encNum_][1], kTestEncoderSize);
                        break;

                    case ENC_NONE:
                        break;
                }

                // STATE UPDATE
                enc.actionPrevious = enc.actionCurrent;
            }

            // 3. RESOURCE CLEANUP
            // Reset transient state for next diagnostic cycle.
            enc.busy = false;
            enc.read = false;
            enc.actionCurrent = ENC_NONE;
        }
    }
}

void Controller::encoder_enable() {
    /// @brief Enables encoder event processing.
    /// Allows encoder read/action pipeline to run.

    // 1. STATE UPDATE
    encoderActive = true;
}

void Controller::encoder_disable() {
    /// @brief Disables encoder event processing.
    /// Prevents encoder read/action pipeline from consuming hardware events.

    // 1. STATE UPDATE
    encoderActive = false;
}

void Controller::encoder_selectBank(uint8_t bankNum_) {
    /// @brief Selects active global encoder bank and refreshes mappings/UI.
    /// Applies bank-level function table to all encoders.

    // 1. INPUT VALIDATION
    // Accept only bank indices in global tab range.
    if (bankNum_ >= kMinEncoderGlobalTab && bankNum_ <= kMaxEncoderGlobalTab) {
        // 2. STATE UPDATE
        activeEncoderBank = bankNum_;

        // 3. MODE/FUNCTION REMAP
        encoder_update();

        // 4. UI SYNCHRONIZATION
        lcd_drawEncoderBankTab();
        for (uint8_t i = 0; i < kEncoderSize; i++) {
            lcd_drawGlobalMenuFunction(i);
            lcd_drawGlobalMenuData(i);
        }
    }
}

void Controller::encoder_setGlobalFunction(uint8_t bankNum_, uint8_t encoderNum_, EncoderFunc function_) {
    /// @brief Assigns a global function to an encoder and updates runtime/UI persistence state.
    /// Resets transient counters and refreshes display for active bank.

    // 1. INPUT VALIDATION
    // Accept only function IDs in the allowed global range.
    if (function_ >= kMinEncoderGlobalFunction && function_ <= kMaxEncoderGlobalFunction && kEncoderFuncDataLibrary[function_].active && bankNum_ < kEncoderBankSize && encoderNum_ < kEncoderSize) {
        // 2. ENCODER CONFIGURATION
        Encoder& enc = encoder[encoderNum_];
        enc.mode = ENC_MODE_GLOBAL;
        enc.function = function_;

        // Load sensitivity profile for assigned function.
        enc.resolution = kEncoderFuncDataLibrary[function_].resolution;

        // 3. STATE RESET
        // Clear transient counters/history to avoid carry-over.
        enc.incCounter = 0;
        enc.decCounter = 0;
        enc.actionPrevious = ENC_NONE;
        enc.actionCurrent = ENC_NONE;

        // 4. MAPPING PERSISTENCE
        // Persist assignment in global function table.
        encoderGlobalFunction[bankNum_][encoderNum_] = (EncoderFunc)function_;

        // 5. UI SYNCHRONIZATION
        // Refresh UI only if edited bank is currently active.
        if (bankNum_ == activeEncoderBank) {
            lcd_drawGlobalMenuFunction(encoderNum_);
            lcd_drawGlobalMenuData(encoderNum_);
            lcd_drawEncoderBox(encoderNum_);
        }

        // 6. AUTOSAVE TIMER REFRESH
        // Restart delayed save timer after assignment change.
        if (encoderPresetSaveActive) {
            stopEncoderPresetTimer();
            startEncoderPresetTimer();
        }
    }
}

void Controller::encoder_incValue(uint8_t encoderNum_) {
    if (keyboard.shiftKeyPress) synthAddPending = false;  // ADD + knob: ADD was a modifier
    /// @brief Increments parameter mapped to selected encoder function.
    /// Applies bounds checks and target-selection guards before dispatch.

    // 1. CONTEXTUAL REFERENCES & SAFETY GUARDS
    Effect& effect_0 = effect[0];
    uint8_t type_0 = effect[0].type;
    SubEffect& subEffect_0 = effect[0].subEffect[type_0];

    Effect& effect_1 = effect[1];
    uint8_t type_1 = effect[1].type;
    SubEffect& subEffect_1 = effect[1].subEffect[type_1];

    // INPUT VALIDATION: reject LFO/Layer edits when no target is selected.
    if ((encoder[encoderNum_].function >= ENC_LFO_ACTIVE) && (encoder[encoderNum_].function <= ENC_LFO_DEPTH) && (selectedLfoNum == -1)) return;
    if ((encoder[encoderNum_].function >= ENC_INST_0_INST) && (encoder[encoderNum_].function <= ENC_SONG_SHIFT) && (selectedLayerNum == -1)) return;
    if ((encoder[encoderNum_].function >= ENC_EUCLID_STEPS) && (encoder[encoderNum_].function <= ENC_EUCLID_MODE) && (selectedLayerNum == -1)) return;
    if ((encoder[encoderNum_].function >= ENC_NOTE_MODE) && (encoder[encoderNum_].function <= ENC_SYNTH_NOISE) && (selectedLayerNum == -1)) return;

    // 2. PARAMETER INCREMENT DISPATCH
    switch (encoder[encoderNum_].function) {
        case ENC_FNC_NONE:
            break;

            // Mixer functions

        case ENC_MIXER_MASTERVOLUME:
            if (mixer.masterVolume < kMaxMixerMasterVolume)
                mixer_setMasterVolume(mixer.masterVolume + 1);
            break;

        case ENC_MIXER_HPVOLUME:
            if (mixer.hpVolume < kMaxMixerHpVolume)
                mixer_setHpVolume(mixer.hpVolume + 1);
            break;

        case ENC_MIXER_LOVOLUME:
            if (mixer.loVolume < kMaxMixerLoVolume)
                mixer_setLoVolume(mixer.loVolume + 1);
            break;

        case ENC_MIXER_LIVOLUME:
            if (mixer.liVolume < kMaxMixerLiVolume)
                mixer_setLiVolume(mixer.liVolume + 1);
            break;

        case ENC_MIXER_LIMIXERIN:
            if (mixer.liMixerIn < kMaxMixerLiMixerIn)
                mixer_setLiIn(mixer.liMixerIn + 1);
            break;

        case ENC_MIXER_PAN:
            if (mixer.pan < kMaxMixerPan)
                mixer_setPan(mixer.pan + 1);
            break;

        case ENC_MIXER_LIMITER:
            if (mixer.limiter < kMaxMixerLimiter)
                mixer_setLimiter(mixer.limiter + 1);
            break;

        case ENC_MIXER_NORMALIZE:
            mixer_setNormalize(!mixer.normalize);
            break;

            // Rhythm functions

        case ENC_RHYTHM_TEMPO:
            if ((rhythm.tempo < kMaxTempo) && (!io.sync.slaveMode))
                rhythm_setTempo(rhythm.tempo + 1);
            break;

        case ENC_RHYTHM_MEASURE:
            if ((!rhythm.measureLock) && (rhythm.measure < kMaxMeasure))
                rhythm_setMeasure(rhythm.measure + 1);
            break;

        case ENC_RHYTHM_BAR:
            if ((!rhythm.barLock) && (rhythm.bar < kMaxBar))
                rhythm_setBar(rhythm.bar + 1);
            break;

        case ENC_RHYTHM_QUANTIZE:
            if ((!rhythm.quantizeLock) && (rhythm.quantize < kMaxQuantize))
                rhythm_setQuantize(rhythm.quantize + 1);
            break;

            // Io functions

        case ENC_IO_SYNCIN:
            if (keyboard.shiftKeyPress) {
                if (io.syncIn > 0) {
                    if (io.syncInPpqn < kMaxIoSyncPpqn) io_setSyncInPpqn(io.syncInPpqn + 1);
                }
            } else {
                (io.syncIn < kMaxIoSyncIn) ? io_setSyncIn(io.syncIn + 1) : io_setSyncIn(kMinIoSyncIn);
            }
            break;

        case ENC_IO_SYNCOUT:
            if (keyboard.shiftKeyPress) {
                if (io.syncOut > 0) {
                    if (io.syncOutPpqn < kMaxIoSyncPpqn) io_setSyncOutPpqn(io.syncOutPpqn + 1);
                }
            } else {
                (io.syncOut < kMaxIoSyncOut) ? io_setSyncOut(io.syncOut + 1) : io_setSyncOut(kMinIoSyncOut);
            }
            break;

        case ENC_IO_MIDIIN:
            if (keyboard.shiftKeyPress) {
                if (io.midiInChannel < kMaxIoMidiChannel) io_setMidiInChannel(io.midiInChannel + 1);
            } else {
                (io.midiIn < kMaxIoMidiIn) ? io_setMidiIn(io.midiIn + 1) : io_setMidiIn(kMinIoMidiIn);
            }
            break;

        case ENC_IO_MIDIOUT:
            if (keyboard.shiftKeyPress) {
                if (io.midiOutChannel < kMaxIoMidiChannel) io_setMidiOutChannel(io.midiOutChannel + 1);
            } else {
                (io.midiOut < kMaxIoMidiOut) ? io_setMidiOut(io.midiOut + 1) : io_setMidiOut(kMinIoMidiOut);
            }
            break;

            // Metronome functions

        case ENC_METRO_ACTIVE:
            metro_setActive(!metronome.active);
            break;

        case ENC_METRO_PRECOUNT:
            metro_setPrecount(!metronome.precount);
            break;

        case ENC_METRO_SAMPLE:
            (metronome.sample < kMaxMetroSample) ? metro_setSample(metronome.sample + 1) : metro_setSample(kMinMetroSample);
            break;

        case ENC_METRO_VOLUME:
            if (metronome.volume < kMaxMetroVolume) metro_setVolume(metronome.volume + 1);
            break;

        case ENC_METRO_PREVIEW:
            metro_setPreviewActive(!previewActive);
            break;

        case ENC_METRO_PREVIEW_LEVEL:
            if (previewLevel < kMaxPreviewLevel) metro_setPreviewLevel(previewLevel + 1);
            break;

            // Filter 0 functions

        case ENC_FILTER_0_ACTIVE:
            filter_setActive(0, !filter[0].active);
            break;

        case ENC_FILTER_0_TYPE:
            (filter[0].type < kMaxFilterType) ? filter_setType(0, filter[0].type + 1) : filter_setType(0, kMinFilterType);
            break;

        case ENC_FILTER_0_FREQ:
            if (filter[0].frequency < kMaxFilterFrequency)
                filter_setFrequency(0, filter[0].frequency + 1);
            break;

        case ENC_FILTER_0_RESON:
            if (filter[0].resonance < kMaxFilterResonance)
                filter_setResonance(0, filter[0].resonance + 1);
            break;

        case ENC_FILTER_0_SLOPE:
            if (filter[0].slope < kMaxFilterSlope)
                filter_setSlope(0, filter[0].slope + 1);
            break;

        case ENC_FILTER_0_DRIVE:
            if (filter[0].drive < kMaxFilterDrive)
                filter_setDrive(0, filter[0].drive + 1);
            break;

        case ENC_FILTER_0_DRY:
            if (filter[0].dry < kMaxFilterDry) {
                filter_setDry(0, filter[0].dry + 1);
            } else if ((filter[0].dry == kMaxFilterDry) && (filter[0].wet > kMinFilterWet)) {
                filter_setWet(0, filter[0].wet - 1);
            }
            break;

        case ENC_FILTER_0_WET:
            if (filter[0].wet < kMaxFilterWet) {
                filter_setWet(0, filter[0].wet + 1);
            } else if ((filter[0].wet == kMaxFilterWet) && (filter[0].dry > kMinFilterDry)) {
                filter_setDry(0, filter[0].dry - 1);
            }
            break;

            // Filter 1 functions

        case ENC_FILTER_1_ACTIVE:
            filter_setActive(1, !filter[1].active);
            break;

        case ENC_FILTER_1_TYPE:
            (filter[1].type < kMaxFilterType) ? filter_setType(1, filter[1].type + 1) : filter_setType(1, kMinFilterType);
            break;

        case ENC_FILTER_1_FREQ:
            if (filter[1].frequency < kMaxFilterFrequency)
                filter_setFrequency(1, filter[1].frequency + 1);
            break;

        case ENC_FILTER_1_RESON:
            if (filter[1].resonance < kMaxFilterResonance)
                filter_setResonance(1, filter[1].resonance + 1);
            break;

        case ENC_FILTER_1_SLOPE:
            if (filter[1].slope < kMaxFilterSlope)
                filter_setSlope(1, filter[1].slope + 1);
            break;

        case ENC_FILTER_1_DRIVE:
            if (filter[1].drive < kMaxFilterDrive)
                filter_setDrive(1, filter[1].drive + 1);
            break;

        case ENC_FILTER_1_DRY:
            if (filter[1].dry < kMaxFilterDry) {
                filter_setDry(1, filter[1].dry + 1);
            } else if ((filter[1].dry == kMaxFilterDry) && (filter[1].wet > kMinFilterWet)) {
                filter_setWet(1, filter[1].wet - 1);
            }
            break;

        case ENC_FILTER_1_WET:
            if (filter[1].wet < kMaxFilterWet) {
                filter_setWet(1, filter[1].wet + 1);
            } else if ((filter[1].wet == kMaxFilterWet) && (filter[1].dry > kMinFilterDry)) {
                filter_setDry(1, filter[1].dry - 1);
            }
            break;

            // Eq functions

        case ENC_EQ_ACTIVE:
            eq_setActive(!eq.active);
            break;

        case ENC_EQ_Q:
            if (eq.q < kMaxEqQ)
                eq_setQ(eq.q + 1);
            break;

        case ENC_EQ_LOWSHELF:
            if (keyboard.shiftKeyPress) {
                if (eq.freqLowShelf < kMaxEqFreq)
                    eq_setFreqLowShelf(eq.freqLowShelf + 1);
            } else if (eq.gainLowShelf < kMaxEqGain)
                eq_setGainLowShelf(eq.gainLowShelf + 1);
            break;

        case ENC_EQ_HIGHSHELF:
            if (keyboard.shiftKeyPress) {
                if (eq.freqHighShelf < kMaxEqFreq)
                    eq_setFreqHighShelf(eq.freqHighShelf + 1);
            } else if (eq.gainHighShelf < kMaxEqGain)
                eq_setGainHighShelf(eq.gainHighShelf + 1);
            break;

        case ENC_EQ_PEAK0:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[0] < kMaxEqFreq)
                    eq_setFreqPeak(0, eq.freqPeak[0] + 1);
            } else if (eq.gainPeak[0] < kMaxEqGain)
                eq_setGainPeak(0, eq.gainPeak[0] + 1);
            break;

        case ENC_EQ_PEAK1:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[1] < kMaxEqFreq)
                    eq_setFreqPeak(1, eq.freqPeak[1] + 1);
            } else if (eq.gainPeak[1] < kMaxEqGain)
                eq_setGainPeak(1, eq.gainPeak[1] + 1);
            break;

        case ENC_EQ_PEAK2:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[2] < kMaxEqFreq)
                    eq_setFreqPeak(2, eq.freqPeak[2] + 1);
            } else if (eq.gainPeak[2] < kMaxEqGain)
                eq_setGainPeak(2, eq.gainPeak[2] + 1);
            break;

        case ENC_EQ_PEAK3:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[3] < kMaxEqFreq)
                    eq_setFreqPeak(3, eq.freqPeak[3] + 1);
            } else if (eq.gainPeak[3] < kMaxEqGain)
                eq_setGainPeak(3, eq.gainPeak[3] + 1);
            break;

            // Effect 0 functions

        case ENC_EFFECT_0_ACTIVE:
            effect_setActive(0, !effect[0].active);
            break;

        case ENC_EFFECT_0_TYPE:
            (effect[0].type < kMaxEffectType) ? effect_setType(0, effect[0].type + 1) : effect_setType(0, kMinEffectType);
            break;

        case ENC_EFFECT_0_PARAM_A:
            if (subEffect_0.aData < subEffect_0.kMaxAData)
                effect_setAData(0, type_0, subEffect_0.aData + 1);
            break;

        case ENC_EFFECT_0_PARAM_B:
            if (subEffect_0.bData < subEffect_0.kMaxBData)
                effect_setBData(0, type_0, subEffect_0.bData + 1);
            break;

        case ENC_EFFECT_0_PARAM_C:
            if (subEffect_0.cData < subEffect_0.kMaxCData)
                effect_setCData(0, type_0, subEffect_0.cData + 1);
            break;

        case ENC_EFFECT_0_PARAM_D:
            if (subEffect_0.dData < subEffect_0.kMaxDData)
                effect_setDData(0, type_0, subEffect_0.dData + 1);
            break;

        case ENC_EFFECT_0_PARAM_E:
            if (subEffect_0.eData < subEffect_0.kMaxEData) {
                effect_setEData(0, type_0, subEffect_0.eData + 1);
            } else if ((subEffect_0.eData == subEffect_0.kMaxEData) && (subEffect_0.fData > subEffect_0.kMinFData)) {
                effect_setFData(0, type_0, subEffect_0.fData - 1);
            }
            break;

        case ENC_EFFECT_0_PARAM_F:
            if (subEffect_0.fData < subEffect_0.kMaxFData) {
                effect_setFData(0, type_0, subEffect_0.fData + 1);
            } else if ((subEffect_0.fData == subEffect_0.kMaxFData) && (subEffect_0.eData > subEffect_0.kMinEData)) {
                effect_setEData(0, type_0, subEffect_0.eData - 1);
            }
            break;

            // Effect 1 functions

        case ENC_EFFECT_1_ACTIVE:
            effect_setActive(1, !effect[1].active);
            break;

        case ENC_EFFECT_1_TYPE:
            (effect[1].type < kMaxEffectType) ? effect_setType(1, effect[1].type + 1) : effect_setType(1, kMinEffectType);
            break;

        case ENC_EFFECT_1_PARAM_A:
            if (subEffect_1.aData < subEffect_1.kMaxAData)
                effect_setAData(1, type_1, subEffect_1.aData + 1);
            break;

        case ENC_EFFECT_1_PARAM_B:
            if (subEffect_1.bData < subEffect_1.kMaxBData)
                effect_setBData(1, type_1, subEffect_1.bData + 1);
            break;

        case ENC_EFFECT_1_PARAM_C:
            if (subEffect_1.cData < subEffect_1.kMaxCData)
                effect_setCData(1, type_1, subEffect_1.cData + 1);
            break;

        case ENC_EFFECT_1_PARAM_D:
            if (subEffect_1.dData < subEffect_1.kMaxDData)
                effect_setDData(1, type_1, subEffect_1.dData + 1);
            break;

        case ENC_EFFECT_1_PARAM_E:
            if (subEffect_1.eData < subEffect_1.kMaxEData) {
                effect_setEData(1, type_1, subEffect_1.eData + 1);
            } else if ((subEffect_1.eData == subEffect_1.kMaxEData) && (subEffect_1.fData > subEffect_1.kMinFData)) {
                effect_setFData(1, type_1, subEffect_1.fData - 1);
            }
            break;

        case ENC_EFFECT_1_PARAM_F:
            if (subEffect_1.fData < subEffect_1.kMaxFData) {
                effect_setFData(1, type_1, subEffect_1.fData + 1);
            } else if ((subEffect_1.fData == subEffect_1.kMaxFData) && (subEffect_1.eData > subEffect_1.kMinEData)) {
                effect_setEData(1, type_1, subEffect_1.eData - 1);
            }
            break;

            // Reverb functions

        case ENC_REVERB_ACTIVE:
            reverb_setActive(!reverb.active);
            break;

        case ENC_REVERB_SIZE:
            if (reverb.size < kMaxReverbSize)
                reverb_setSize(reverb.size + 1);
            break;

        case ENC_REVERB_DECAY:
            if (reverb.decay < kMaxReverbDecay)
                reverb_setDecay(reverb.decay + 1);
            break;

        case ENC_REVERB_DAMPING:
            if (reverb.damping < kMaxReverbDamping)
                reverb_setDamping(reverb.damping + 1);
            break;

        case ENC_REVERB_PREDELAY:
            if (reverb.preDelay < kMaxReverbPreDelay)
                reverb_setPreDelay(reverb.preDelay + 1);
            break;

        case ENC_REVERB_SURROUND:
            if (reverb.surround < kMaxReverbSurround)
                reverb_setSurround(reverb.surround + 1);
            break;

        case ENC_REVERB_DRY:
            if (reverb.dry < kMaxReverbDry) {
                reverb_setDry(reverb.dry + 1);
            } else if ((reverb.dry == kMaxReverbDry) && (reverb.wet > kMinReverbWet)) {
                reverb_setWet(reverb.wet - 1);
            }
            break;

        case ENC_REVERB_WET:
            if (reverb.wet < kMaxReverbWet) {
                reverb_setWet(reverb.wet + 1);
            } else if ((reverb.wet == kMaxReverbWet) && (reverb.dry > kMinReverbDry)) {
                reverb_setDry(reverb.dry - 1);
            }
            break;

            // Lfo functions

        case ENC_LFO_ACTIVE:
            lfo_setActive(selectedLfoNum, !lfoLibrary[selectedLfoNum].active);
            break;

        case ENC_LFO_TYPE:
            (lfoLibrary[selectedLfoNum].type < kMaxLfoType) ? lfo_setType(selectedLfoNum, lfoLibrary[selectedLfoNum].type + 1) : lfo_setType(selectedLfoNum, kMinLfoType);
            break;

        case ENC_LFO_RATE:
            if (lfoLibrary[selectedLfoNum].rate < kMaxLfoRate)
                lfo_setRate(selectedLfoNum, lfoLibrary[selectedLfoNum].rate + 1);
            break;

        case ENC_LFO_PHASE:
            if (lfoLibrary[selectedLfoNum].phase < kMaxLfoPhase)
                lfo_setPhase(selectedLfoNum, lfoLibrary[selectedLfoNum].phase + 1);
            break;

        case ENC_LFO_INVERT:
            lfo_setInvert(selectedLfoNum, !lfoLibrary[selectedLfoNum].invert);
            break;

        case ENC_LFO_DEPTH:
            if (lfoLibrary[selectedLfoNum].depth < kMaxLfoDepth)
                lfo_setDepth(selectedLfoNum, lfoLibrary[selectedLfoNum].depth + 1);
            break;

            // Layer inst functions

        case ENC_INST_0_INST:
        case ENC_INST_1_INST:
        case ENC_INST_2_INST:
            if (instLibrarySize > 0)
                (layerLibrary[selectedLayerNum].instSelected < (instLibrarySize - 1)) ? layerInst_setInstSelected(selectedLayerNum, layerLibrary[selectedLayerNum].instSelected + 1) : layerInst_setInstSelected(selectedLayerNum, -1);
            preview_request(selectedLayerNum);  // audition the instrument's first sample once the knob rests
            break;

        case ENC_INST_0_SAMPLE:
        case ENC_INST_1_SAMPLE:
        case ENC_INST_2_SAMPLE:
            if ((layerLibrary[selectedLayerNum].instSelected != -1) && (sampleLibrarySize[layerLibrary[selectedLayerNum].instSelected] > 0))
                (layerLibrary[selectedLayerNum].sampleSelected < (sampleLibrarySize[layerLibrary[selectedLayerNum].instSelected] - 1)) ? layerInst_setSampleSelected(selectedLayerNum, layerLibrary[selectedLayerNum].sampleSelected + 1) : layerInst_setSampleSelected(selectedLayerNum, 0);
            preview_request(selectedLayerNum);  // audition once the knob rests
            break;

        case ENC_INST_0_LEVEL:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoLevel < kMaxLfo) ? layerInst_setLfoLevel(selectedLayerNum, layerLibrary[selectedLayerNum].lfoLevel + 1) : layerInst_setLfoLevel(selectedLayerNum, kMinLfo);
            } else if (layerLibrary[selectedLayerNum].level < kMaxLayerLevel)
                layerInst_setLevel(selectedLayerNum, layerLibrary[selectedLayerNum].level + 1);
            break;

        case ENC_INST_0_PITCH:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoPitch < kMaxLfo) ? layerInst_setLfoPitch(selectedLayerNum, layerLibrary[selectedLayerNum].lfoPitch + 1) : layerInst_setLfoPitch(selectedLayerNum, kMinLfo);
            } else if (layerLibrary[selectedLayerNum].pitch < kMaxLayerPitch)
                layerInst_setPitch(selectedLayerNum, layerLibrary[selectedLayerNum].pitch + 1);
            break;

        case ENC_INST_0_DIRECT:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoDirection < kMaxLfo) ? layerInst_setLfoDirection(selectedLayerNum, layerLibrary[selectedLayerNum].lfoDirection + 1) : layerInst_setLfoDirection(selectedLayerNum, kMinLfo);
            } else {
                (layerLibrary[selectedLayerNum].direction < kMaxLayerDirection) ? layerInst_setDirection(selectedLayerNum, layerLibrary[selectedLayerNum].direction + 1) : layerInst_setDirection(selectedLayerNum, kMinLayerDirection);
            }
            break;

        case ENC_INST_0_RESOL:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoResolution < kMaxLfo) ? layerInst_setLfoResolution(selectedLayerNum, layerLibrary[selectedLayerNum].lfoResolution + 1) : layerInst_setLfoResolution(selectedLayerNum, kMinLfo);
            } else if (layerLibrary[selectedLayerNum].resolution < kMaxLayerResolution)
                layerInst_setResolution(selectedLayerNum, layerLibrary[selectedLayerNum].resolution + 1);
            break;

        case ENC_INST_1_START:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoStart < kMaxLfo) ? layerInst_setLfoStart(selectedLayerNum, layerLibrary[selectedLayerNum].lfoStart + 1) : layerInst_setLfoStart(selectedLayerNum, kMinLfo);
            } else if (layerLibrary[selectedLayerNum].start < kMaxLayerPosition)
                layerInst_setStart(selectedLayerNum, layerLibrary[selectedLayerNum].start + 1);
            break;

        case ENC_INST_1_END:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoEnd < kMaxLfo) ? layerInst_setLfoEnd(selectedLayerNum, layerLibrary[selectedLayerNum].lfoEnd + 1) : layerInst_setLfoEnd(selectedLayerNum, kMinLfo);
            } else if (layerLibrary[selectedLayerNum].end < kMaxLayerPosition)
                layerInst_setEnd(selectedLayerNum, layerLibrary[selectedLayerNum].end + 1);
            break;

        case ENC_INST_1_TIMING:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoTiming < kMaxLfo) ? layerInst_setLfoTiming(selectedLayerNum, layerLibrary[selectedLayerNum].lfoTiming + 1) : layerInst_setLfoTiming(selectedLayerNum, kMinLfo);
            } else if (layerLibrary[selectedLayerNum].timing < kMaxLayerTiming)
                layerInst_setTiming(selectedLayerNum, layerLibrary[selectedLayerNum].timing + 1);
            break;

        case ENC_INST_1_PROB:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoProbability < kMaxLfo) ? layerInst_setLfoProbability(selectedLayerNum, layerLibrary[selectedLayerNum].lfoProbability + 1) : layerInst_setLfoProbability(selectedLayerNum, kMinLfo);
            } else if (layerLibrary[selectedLayerNum].probability < kMaxLayerProbability)
                layerInst_setProbability(selectedLayerNum, layerLibrary[selectedLayerNum].probability + 1);
            break;

        case ENC_INST_2_EQ:
            (layerLibrary[selectedLayerNum].eq < kMaxLayerEq) ? layerInst_setEq(selectedLayerNum, layerLibrary[selectedLayerNum].eq + 1) : layerInst_setEq(selectedLayerNum, kMinLayerEq);
            break;

        case ENC_INST_2_FILTER:
            (layerLibrary[selectedLayerNum].filter < kMaxLayerFilter) ? layerInst_setFilter(selectedLayerNum, layerLibrary[selectedLayerNum].filter + 1) : layerInst_setFilter(selectedLayerNum, kMinLayerFilter);
            break;

        case ENC_INST_2_EFFECT:
            (layerLibrary[selectedLayerNum].effect < kMaxLayerEffect) ? layerInst_setEffect(selectedLayerNum, layerLibrary[selectedLayerNum].effect + 1) : layerInst_setEffect(selectedLayerNum, kMinLayerEffect);
            break;

        case ENC_INST_2_REVERB:
            (layerLibrary[selectedLayerNum].reverb < kMaxLayerReverb) ? layerInst_setReverb(selectedLayerNum, layerLibrary[selectedLayerNum].reverb + 1) : layerInst_setReverb(selectedLayerNum, kMinLayerReverb);
            break;

            // Layer song functions

        case ENC_SONG_FILL:
            layerSong_menuUp(0);
            break;

        case ENC_SONG_PATTERN:
            layerSong_menuUp(1);
            break;

        case ENC_SONG_TIMING:
            layerSong_menuUp(4);
            break;

        case ENC_SONG_VELOCITY:
            layerSong_menuUp(5);
            break;

        case ENC_SONG_MOVE:
            layerSongAddPending = false;
            layerSong_moveSelectedBeat(true);
            break;

        case ENC_SONG_SHIFT:
            layerSongAddPending = false;
            layerSong_shiftAllBeats(true);
            break;

        case ENC_EUCLID_STEPS:
        case ENC_EUCLID_HITS:
        case ENC_EUCLID_ROTATE:
        case ENC_EUCLID_GRID:
        case ENC_EUCLID_LENGTH:
        case ENC_EUCLID_MODE:
            euclid_change(encoder[encoderNum_].function, true);
            break;

        case ENC_RHYTHM_SONG:
        case ENC_RHYTHM_SONG_STEP:
        case ENC_RHYTHM_SONG_PART:
        case ENC_RHYTHM_SONG_REPEAT:
            song_change(encoder[encoderNum_].function, true);
            break;

        case ENC_IO_CPU:
            break;  // read-only

        case ENC_IO_CPU_PEAK:
            cpu_resetPeak();
            break;

        case ENC_NOTE_MODE:
        case ENC_NOTE_NOTE:
        case ENC_NOTE_ROOT:
        case ENC_NOTE_SCALE:
        case ENC_NOTE_GLIDE:
        case ENC_NOTE_TRANSPOSE:
        case ENC_NOTE_FOLLOW:
            note_change(encoder[encoderNum_].function, true);
            break;

        case ENC_SYNTH_WAVE:
        case ENC_SYNTH_CUTOFF:
        case ENC_SYNTH_RESO:
        case ENC_SYNTH_ENVAMT:
        case ENC_SYNTH_DECAY:
        case ENC_SYNTH_GATE:
        case ENC_SYNTH_DRIVE:
        case ENC_SYNTH_DETUNE:
        case ENC_SYNTH_SUB:
        case ENC_SYNTH_NOISE:
        case ENC_SYNTH_LFO_SHAPE:
        case ENC_SYNTH_LFO_RATE:
        case ENC_SYNTH_LFO_DEPTH:
        case ENC_SYNTH_LFO_TARGET:
        case ENC_SYNTH_LFO_RETRIG:
        case ENC_SYNTH_PRESET:
        case ENC_SYNTH_SAVE_SLOT:
        case ENC_SYNTH_ATTACK:
        case ENC_SYNTH_AMP_DECAY:
        case ENC_SYNTH_SUSTAIN:
        case ENC_SYNTH_RELEASE:
        case ENC_SYNTH_FILTER_TYPE:
        case ENC_SYNTH_PAD_HOLD:
        case ENC_SYNTH_FM_RATIO:
        case ENC_SYNTH_FM_AMOUNT:
        case ENC_SYNTH_FM_DECAY:
        case ENC_SYNTH_PITCH_ENV:
        case ENC_SYNTH_PITCH_DECAY:
        case ENC_SYNTH_FILT_ATTACK:
            synth_change(encoder[encoderNum_].function, true);
            break;

        case ENC_FILE_AUTOSAVE:
            autosave_setEnabled(true);
            break;

        case ENC_NAME_CHAR:
            name_changeChar(true);
            break;

        case ENC_NAME_CURSOR:
            name_moveCursor(true);
            break;

        case ENC_NAME_RANDOM:
            name_random();
            break;

        default:
            break;
    }
}

void Controller::encoder_decValue(uint8_t encoderNum_) {
    if (keyboard.shiftKeyPress) synthAddPending = false;  // ADD + knob: ADD was a modifier
    /// @brief Decrements parameter mapped to selected encoder function.
    /// Applies lower-bound checks and target-selection guards before dispatch.

    // 1. CONTEXTUAL REFERENCES & SAFETY GUARDS
    Effect& effect_0 = effect[0];
    uint8_t type_0 = effect[0].type;
    SubEffect& subEffect_0 = effect[0].subEffect[type_0];

    Effect& effect_1 = effect[1];
    uint8_t type_1 = effect[1].type;
    SubEffect& subEffect_1 = effect[1].subEffect[type_1];

    // INPUT VALIDATION: reject LFO/Layer edits when no target is selected.
    if ((encoder[encoderNum_].function >= ENC_LFO_ACTIVE) && (encoder[encoderNum_].function <= ENC_LFO_DEPTH) && (selectedLfoNum == -1)) return;
    if ((encoder[encoderNum_].function >= ENC_INST_0_INST) && (encoder[encoderNum_].function <= ENC_SONG_SHIFT) && (selectedLayerNum == -1)) return;
    if ((encoder[encoderNum_].function >= ENC_EUCLID_STEPS) && (encoder[encoderNum_].function <= ENC_EUCLID_MODE) && (selectedLayerNum == -1)) return;
    if ((encoder[encoderNum_].function >= ENC_NOTE_MODE) && (encoder[encoderNum_].function <= ENC_SYNTH_NOISE) && (selectedLayerNum == -1)) return;

    // 2. PARAMETER DECREMENT DISPATCH
    switch (encoder[encoderNum_].function) {
        case ENC_FNC_NONE:
            break;

            // Mixer functions

        case ENC_MIXER_MASTERVOLUME:
            if (mixer.masterVolume > kMinMixerMasterVolume)
                mixer_setMasterVolume(mixer.masterVolume - 1);
            break;

        case ENC_MIXER_HPVOLUME:
            if (mixer.hpVolume > kMinMixerHpVolume)
                mixer_setHpVolume(mixer.hpVolume - 1);
            break;

        case ENC_MIXER_LOVOLUME:
            if (mixer.loVolume > kMinMixerLoVolume)
                mixer_setLoVolume(mixer.loVolume - 1);
            break;

        case ENC_MIXER_LIVOLUME:
            if (mixer.liVolume > kMinMixerLiVolume)
                mixer_setLiVolume(mixer.liVolume - 1);
            break;

        case ENC_MIXER_LIMIXERIN:
            if (mixer.liMixerIn > kMinMixerLiMixerIn)
                mixer_setLiIn(mixer.liMixerIn - 1);
            break;

        case ENC_MIXER_PAN:
            if (mixer.pan > kMinMixerPan)
                mixer_setPan(mixer.pan - 1);
            break;

        case ENC_MIXER_LIMITER:
            if (mixer.limiter > kMinMixerLimiter) mixer_setLimiter(mixer.limiter - 1);
            break;

        case ENC_MIXER_NORMALIZE:
            mixer_setNormalize(!mixer.normalize);
            break;

            // Rhythm functions

        case ENC_RHYTHM_TEMPO:
            if ((rhythm.tempo > kMinTempo) && (!io.sync.slaveMode))
                rhythm_setTempo(rhythm.tempo - 1);
            break;

        case ENC_RHYTHM_MEASURE:
            if ((!rhythm.measureLock) && (rhythm.measure > kMinMeasure))
                rhythm_setMeasure(rhythm.measure - 1);
            break;

        case ENC_RHYTHM_BAR:
            if ((!rhythm.barLock) && (rhythm.bar > kMinBar))
                rhythm_setBar(rhythm.bar - 1);
            break;

        case ENC_RHYTHM_QUANTIZE:
            if ((!rhythm.quantizeLock) && (rhythm.quantize > kMinQuantize))
                rhythm_setQuantize(rhythm.quantize - 1);
            break;

            // Io functions

        case ENC_IO_SYNCIN:
            if (keyboard.shiftKeyPress) {
                if (io.syncIn > 0) {
                    if (io.syncInPpqn > kMinIoSyncPpqn) io_setSyncInPpqn(io.syncInPpqn - 1);
                }
            } else {
                (io.syncIn > kMinIoSyncIn) ? io_setSyncIn(io.syncIn - 1) : io_setSyncIn(kMaxIoSyncIn);
            }
            break;

        case ENC_IO_SYNCOUT:
            if (keyboard.shiftKeyPress) {
                if (io.syncOut > 0) {
                    if (io.syncOutPpqn > kMinIoSyncPpqn) io_setSyncOutPpqn(io.syncOutPpqn - 1);
                }
            } else {
                (io.syncOut > kMinIoSyncOut) ? io_setSyncOut(io.syncOut - 1) : io_setSyncOut(kMaxIoSyncOut);
            }
            break;

        case ENC_IO_MIDIOUT:
            if (keyboard.shiftKeyPress) {
                if (io.midiOutChannel > kMinIoMidiChannel) io_setMidiOutChannel(io.midiOutChannel - 1);
            } else {
                (io.midiOut > kMinIoMidiOut) ? io_setMidiOut(io.midiOut - 1) : io_setMidiOut(kMaxIoMidiOut);
            }
            break;

        case ENC_IO_MIDIIN:
            if (keyboard.shiftKeyPress) {
                if (io.midiInChannel > kMinIoMidiChannel) io_setMidiInChannel(io.midiInChannel - 1);
            } else {
                (io.midiIn > kMinIoMidiIn) ? io_setMidiIn(io.midiIn - 1) : io_setMidiIn(kMaxIoMidiIn);
            }
            break;

            // Metronome functions

        case ENC_METRO_ACTIVE:
            metro_setActive(!metronome.active);
            break;

        case ENC_METRO_PRECOUNT:
            metro_setPrecount(!metronome.precount);
            break;

        case ENC_METRO_SAMPLE:
            (metronome.sample > kMinMetroSample) ? metro_setSample(metronome.sample - 1) : metro_setSample(kMaxMetroSample);
            break;

        case ENC_METRO_VOLUME:
            if (metronome.volume > kMinMetroVolume)
                metro_setVolume(metronome.volume - 1);
            break;

        case ENC_METRO_PREVIEW:
            metro_setPreviewActive(!previewActive);
            break;

        case ENC_METRO_PREVIEW_LEVEL:
            if (previewLevel > kMinPreviewLevel) metro_setPreviewLevel(previewLevel - 1);
            break;

            // Filter 0 functions

        case ENC_FILTER_0_ACTIVE:
            filter_setActive(0, !filter[0].active);
            break;

        case ENC_FILTER_0_TYPE:
            (filter[0].type > kMinFilterType) ? filter_setType(0, filter[0].type - 1) : filter_setType(0, kMaxFilterType);
            break;

        case ENC_FILTER_0_FREQ:
            if (filter[0].frequency > kMinFilterFrequency)
                filter_setFrequency(0, filter[0].frequency - 1);
            break;

        case ENC_FILTER_0_RESON:
            if (filter[0].resonance > kMinFilterResonance)
                filter_setResonance(0, filter[0].resonance - 1);
            break;

        case ENC_FILTER_0_DRIVE:
            if (filter[0].drive > kMinFilterDrive)
                filter_setDrive(0, filter[0].drive - 1);
            break;

        case ENC_FILTER_0_SLOPE:
            if (filter[0].slope > kMinFilterSlope)
                filter_setSlope(0, filter[0].slope - 1);
            break;

        case ENC_FILTER_0_DRY:
            if (filter[0].dry > kMinFilterDry)
                filter_setDry(0, filter[0].dry - 1);
            break;

        case ENC_FILTER_0_WET:
            if (filter[0].wet > kMinFilterWet)
                filter_setWet(0, filter[0].wet - 1);
            break;

            // Filter 1 functions

        case ENC_FILTER_1_ACTIVE:
            filter_setActive(1, !filter[1].active);
            break;

        case ENC_FILTER_1_TYPE:
            (filter[1].type > kMinFilterType) ? filter_setType(1, filter[1].type - 1) : filter_setType(1, kMaxFilterType);
            break;

        case ENC_FILTER_1_FREQ:
            if (filter[1].frequency > kMinFilterFrequency)
                filter_setFrequency(1, filter[1].frequency - 1);
            break;

        case ENC_FILTER_1_RESON:
            if (filter[1].resonance > kMinFilterResonance)
                filter_setResonance(1, filter[1].resonance - 1);
            break;

        case ENC_FILTER_1_SLOPE:
            if (filter[1].slope > kMinFilterSlope)
                filter_setSlope(1, filter[1].slope - 1);
            break;

        case ENC_FILTER_1_DRIVE:
            if (filter[1].drive > kMinFilterDrive)
                filter_setDrive(1, filter[1].drive - 1);
            break;

        case ENC_FILTER_1_DRY:
            if (filter[1].dry > kMinFilterDry)
                filter_setDry(1, filter[1].dry - 1);
            break;

        case ENC_FILTER_1_WET:
            if (filter[1].wet > kMinFilterWet)
                filter_setWet(1, filter[1].wet - 1);
            break;

            // Eq functions

        case ENC_EQ_ACTIVE:
            eq_setActive(!eq.active);
            break;

        case ENC_EQ_Q:
            if (eq.q > kMinEqQ)
                eq_setQ(eq.q - 1);
            break;

        case ENC_EQ_LOWSHELF:
            if (keyboard.shiftKeyPress) {
                if (eq.freqLowShelf > kMinEqFreq)
                    eq_setFreqLowShelf(eq.freqLowShelf - 1);
            } else if (eq.gainLowShelf > kMinEqGain)
                eq_setGainLowShelf(eq.gainLowShelf - 1);
            break;

        case ENC_EQ_HIGHSHELF:
            if (keyboard.shiftKeyPress) {
                if (eq.freqHighShelf > kMinEqFreq)
                    eq_setFreqHighShelf(eq.freqHighShelf - 1);
            } else if (eq.gainHighShelf > kMinEqGain)
                eq_setGainHighShelf(eq.gainHighShelf - 1);
            break;

        case ENC_EQ_PEAK0:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[0] > kMinEqFreq)
                    eq_setFreqPeak(0, eq.freqPeak[0] - 1);
            } else if (eq.gainPeak[0] > kMinEqGain)
                eq_setGainPeak(0, eq.gainPeak[0] - 1);
            break;

        case ENC_EQ_PEAK1:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[1] > kMinEqFreq)
                    eq_setFreqPeak(1, eq.freqPeak[1] - 1);
            } else if (eq.gainPeak[1] > kMinEqGain)
                eq_setGainPeak(1, eq.gainPeak[1] - 1);
            break;

        case ENC_EQ_PEAK2:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[2] > kMinEqFreq)
                    eq_setFreqPeak(2, eq.freqPeak[2] - 1);
            } else if (eq.gainPeak[2] > kMinEqGain)
                eq_setGainPeak(2, eq.gainPeak[2] - 1);
            break;

        case ENC_EQ_PEAK3:
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[3] > kMinEqFreq)
                    eq_setFreqPeak(3, eq.freqPeak[3] - 1);
            } else if (eq.gainPeak[3] > kMinEqGain)
                eq_setGainPeak(3, eq.gainPeak[3] - 1);
            break;

            // Effect 0 functions

        case ENC_EFFECT_0_ACTIVE:
            effect_setActive(0, !effect[0].active);
            break;

        case ENC_EFFECT_0_TYPE:
            (effect[0].type > kMinEffectType) ? effect_setType(0, effect[0].type - 1) : effect_setType(0, kMaxEffectType);
            break;

        case ENC_EFFECT_0_PARAM_A:
            if (subEffect_0.aData > subEffect_0.kMinAData)
                effect_setAData(0, type_0, subEffect_0.aData - 1);
            break;

        case ENC_EFFECT_0_PARAM_B:
            if (subEffect_0.bData > subEffect_0.kMinBData)
                effect_setBData(0, type_0, subEffect_0.bData - 1);
            break;

        case ENC_EFFECT_0_PARAM_C:
            if (subEffect_0.cData > subEffect_0.kMinCData)
                effect_setCData(0, type_0, subEffect_0.cData - 1);
            break;

        case ENC_EFFECT_0_PARAM_D:
            if (subEffect_0.dData > subEffect_0.kMinDData)
                effect_setDData(0, type_0, subEffect_0.dData - 1);
            break;

        case ENC_EFFECT_0_PARAM_E:
            if (subEffect_0.eData > subEffect_0.kMinEData)
                effect_setEData(0, type_0, subEffect_0.eData - 1);
            break;

        case ENC_EFFECT_0_PARAM_F:
            if (subEffect_0.fData > subEffect_0.kMinFData)
                effect_setFData(0, type_0, subEffect_0.fData - 1);
            break;

            // Effect 1 functions

        case ENC_EFFECT_1_ACTIVE:
            effect_setActive(1, !effect[1].active);
            break;

        case ENC_EFFECT_1_TYPE:
            (effect[1].type > kMinEffectType) ? effect_setType(1, effect[1].type - 1) : effect_setType(1, kMaxEffectType);
            break;

        case ENC_EFFECT_1_PARAM_A:
            if (subEffect_1.aData > subEffect_1.kMinAData)
                effect_setAData(1, type_1, subEffect_1.aData - 1);
            break;

        case ENC_EFFECT_1_PARAM_B:
            if (subEffect_1.bData > subEffect_1.kMinBData)
                effect_setBData(1, type_1, subEffect_1.bData - 1);
            break;

        case ENC_EFFECT_1_PARAM_C:
            if (subEffect_1.cData > subEffect_1.kMinCData)
                effect_setCData(1, type_1, subEffect_1.cData - 1);
            break;

        case ENC_EFFECT_1_PARAM_D:
            if (subEffect_1.dData > subEffect_1.kMinDData)
                effect_setDData(1, type_1, subEffect_1.dData - 1);
            break;

        case ENC_EFFECT_1_PARAM_E:
            if (subEffect_1.eData > subEffect_1.kMinEData)
                effect_setEData(1, type_1, subEffect_1.eData - 1);
            break;

        case ENC_EFFECT_1_PARAM_F:
            if (subEffect_1.fData > subEffect_1.kMinFData)
                effect_setFData(1, type_1, subEffect_1.fData - 1);
            break;

            // Reverb functions

        case ENC_REVERB_ACTIVE:
            reverb_setActive(!reverb.active);
            break;

        case ENC_REVERB_SIZE:
            if (reverb.size > kMinReverbSize)
                reverb_setSize(reverb.size - 1);
            break;

        case ENC_REVERB_DECAY:
            if (reverb.decay > kMinReverbDecay)
                reverb_setDecay(reverb.decay - 1);
            break;

        case ENC_REVERB_DAMPING:
            if (reverb.damping > kMinReverbDamping)
                reverb_setDamping(reverb.damping - 1);
            break;

        case ENC_REVERB_PREDELAY:
            if (reverb.preDelay > kMinReverbPreDelay)
                reverb_setPreDelay(reverb.preDelay - 1);
            break;

        case ENC_REVERB_SURROUND:
            if (reverb.surround > kMinReverbSurround)
                reverb_setSurround(reverb.surround - 1);
            break;

        case ENC_REVERB_DRY:
            if (reverb.dry > kMinReverbDry)
                reverb_setDry(reverb.dry - 1);
            break;

        case ENC_REVERB_WET:
            if (reverb.wet > kMinReverbWet)
                reverb_setWet(reverb.wet - 1);
            break;

            // Lfo functions

        case ENC_LFO_ACTIVE:
            lfo_setActive(selectedLfoNum, !lfoLibrary[selectedLfoNum].active);
            break;

        case ENC_LFO_TYPE:
            (lfoLibrary[selectedLfoNum].type > kMinLfoType) ? lfo_setType(selectedLfoNum, lfoLibrary[selectedLfoNum].type - 1) : lfo_setType(selectedLfoNum, kMaxLfoType);
            break;

        case ENC_LFO_RATE:
            if (lfoLibrary[selectedLfoNum].rate > kMinLfoRate)
                lfo_setRate(selectedLfoNum, lfoLibrary[selectedLfoNum].rate - 1);
            break;

        case ENC_LFO_PHASE:
            if (lfoLibrary[selectedLfoNum].phase > kMinLfoPhase)
                lfo_setPhase(selectedLfoNum, lfoLibrary[selectedLfoNum].phase - 1);
            break;

        case ENC_LFO_INVERT:
            lfo_setInvert(selectedLfoNum, !lfoLibrary[selectedLfoNum].invert);
            break;

        case ENC_LFO_DEPTH:
            if (lfoLibrary[selectedLfoNum].depth > kMinLfoDepth)
                lfo_setDepth(selectedLfoNum, lfoLibrary[selectedLfoNum].depth - 1);
            break;

            // Layer inst functions

        case ENC_INST_0_INST:
        case ENC_INST_1_INST:
        case ENC_INST_2_INST:
            if (instLibrarySize > 0)
                (layerLibrary[selectedLayerNum].instSelected > -1) ? layerInst_setInstSelected(selectedLayerNum, layerLibrary[selectedLayerNum].instSelected - 1) : layerInst_setInstSelected(selectedLayerNum, instLibrarySize - 1);
            preview_request(selectedLayerNum);  // audition the instrument's first sample once the knob rests
            break;

        case ENC_INST_0_SAMPLE:
        case ENC_INST_1_SAMPLE:
        case ENC_INST_2_SAMPLE:
            if ((layerLibrary[selectedLayerNum].instSelected != -1) && (sampleLibrarySize[layerLibrary[selectedLayerNum].instSelected] > 0))
                (layerLibrary[selectedLayerNum].sampleSelected > 0) ? layerInst_setSampleSelected(selectedLayerNum, layerLibrary[selectedLayerNum].sampleSelected - 1) : layerInst_setSampleSelected(selectedLayerNum, sampleLibrarySize[layerLibrary[selectedLayerNum].instSelected] - 1);
            preview_request(selectedLayerNum);  // audition once the knob rests
            break;

        case ENC_INST_0_LEVEL:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoLevel > kMinLfo) ? layerInst_setLfoLevel(selectedLayerNum, layerLibrary[selectedLayerNum].lfoLevel - 1) : layerInst_setLfoLevel(selectedLayerNum, kMaxLfo);
            } else if (layerLibrary[selectedLayerNum].level > kMinLayerLevel)
                layerInst_setLevel(selectedLayerNum, layerLibrary[selectedLayerNum].level - 1);
            break;

        case ENC_INST_0_PITCH:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoPitch > kMinLfo) ? layerInst_setLfoPitch(selectedLayerNum, layerLibrary[selectedLayerNum].lfoPitch - 1) : layerInst_setLfoPitch(selectedLayerNum, kMaxLfo);
            } else if (layerLibrary[selectedLayerNum].pitch > kMinLayerPitch)
                layerInst_setPitch(selectedLayerNum, layerLibrary[selectedLayerNum].pitch - 1);
            break;

        case ENC_INST_0_DIRECT:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoDirection > kMinLfo) ? layerInst_setLfoDirection(selectedLayerNum, layerLibrary[selectedLayerNum].lfoDirection - 1) : layerInst_setLfoDirection(selectedLayerNum, kMaxLfo);
            } else {
                (layerLibrary[selectedLayerNum].direction > kMinLayerDirection) ? layerInst_setDirection(selectedLayerNum, layerLibrary[selectedLayerNum].direction - 1) : layerInst_setDirection(selectedLayerNum, kMaxLayerDirection);
            }
            break;

        case ENC_INST_0_RESOL:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoResolution > kMinLfo) ? layerInst_setLfoResolution(selectedLayerNum, layerLibrary[selectedLayerNum].lfoResolution - 1) : layerInst_setLfoResolution(selectedLayerNum, kMaxLfo);
            } else if (layerLibrary[selectedLayerNum].resolution > kMinLayerResolution)
                layerInst_setResolution(selectedLayerNum, layerLibrary[selectedLayerNum].resolution - 1);
            break;

        case ENC_INST_1_START:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoStart > kMinLfo) ? layerInst_setLfoStart(selectedLayerNum, layerLibrary[selectedLayerNum].lfoStart - 1) : layerInst_setLfoStart(selectedLayerNum, kMaxLfo);
            } else if (layerLibrary[selectedLayerNum].start > kMinLayerPosition)
                layerInst_setStart(selectedLayerNum, layerLibrary[selectedLayerNum].start - 1);
            break;

        case ENC_INST_1_END:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoEnd > kMinLfo) ? layerInst_setLfoEnd(selectedLayerNum, layerLibrary[selectedLayerNum].lfoEnd - 1) : layerInst_setLfoEnd(selectedLayerNum, kMaxLfo);
            } else if (layerLibrary[selectedLayerNum].end > kMinLayerPosition)
                layerInst_setEnd(selectedLayerNum, layerLibrary[selectedLayerNum].end - 1);
            break;

        case ENC_INST_1_TIMING:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoTiming > kMinLfo) ? layerInst_setLfoTiming(selectedLayerNum, layerLibrary[selectedLayerNum].lfoTiming - 1) : layerInst_setLfoTiming(selectedLayerNum, kMaxLfo);
            } else if (layerLibrary[selectedLayerNum].timing > kMinLayerTiming)
                layerInst_setTiming(selectedLayerNum, layerLibrary[selectedLayerNum].timing - 1);
            break;

        case ENC_INST_1_PROB:
            if (keyboard.shiftKeyPress) {
                (layerLibrary[selectedLayerNum].lfoProbability > kMinLfo) ? layerInst_setLfoProbability(selectedLayerNum, layerLibrary[selectedLayerNum].lfoProbability - 1) : layerInst_setLfoProbability(selectedLayerNum, kMaxLfo);
            } else if (layerLibrary[selectedLayerNum].probability > kMinLayerProbability)
                layerInst_setProbability(selectedLayerNum, layerLibrary[selectedLayerNum].probability - 1);
            break;

        case ENC_INST_2_EQ:
            (layerLibrary[selectedLayerNum].eq > kMinLayerEq) ? layerInst_setEq(selectedLayerNum, layerLibrary[selectedLayerNum].eq - 1) : layerInst_setEq(selectedLayerNum, kMaxLayerEq);
            break;

        case ENC_INST_2_FILTER:
            (layerLibrary[selectedLayerNum].filter > kMinLayerFilter) ? layerInst_setFilter(selectedLayerNum, layerLibrary[selectedLayerNum].filter - 1) : layerInst_setFilter(selectedLayerNum, kMaxLayerFilter);
            break;

        case ENC_INST_2_EFFECT:
            (layerLibrary[selectedLayerNum].effect > kMinLayerEffect) ? layerInst_setEffect(selectedLayerNum, layerLibrary[selectedLayerNum].effect - 1) : layerInst_setEffect(selectedLayerNum, kMaxLayerEffect);
            break;

        case ENC_INST_2_REVERB:
            (layerLibrary[selectedLayerNum].reverb > kMinLayerReverb) ? layerInst_setReverb(selectedLayerNum, layerLibrary[selectedLayerNum].reverb - 1) : layerInst_setReverb(selectedLayerNum, kMaxLayerReverb);
            break;

            // Layer song functions

        case ENC_SONG_FILL:
            layerSong_menuDown(0);
            break;

        case ENC_SONG_PATTERN:
            layerSong_menuDown(1);
            break;

        case ENC_SONG_TIMING:
            layerSong_menuDown(4);
            break;

        case ENC_SONG_VELOCITY:
            layerSong_menuDown(5);
            break;

        case ENC_SONG_MOVE:
            layerSongAddPending = false;
            layerSong_moveSelectedBeat(false);
            break;

        case ENC_SONG_SHIFT:
            layerSongAddPending = false;
            layerSong_shiftAllBeats(false);
            break;

        case ENC_EUCLID_STEPS:
        case ENC_EUCLID_HITS:
        case ENC_EUCLID_ROTATE:
        case ENC_EUCLID_GRID:
        case ENC_EUCLID_LENGTH:
        case ENC_EUCLID_MODE:
            euclid_change(encoder[encoderNum_].function, false);
            break;

        case ENC_RHYTHM_SONG:
        case ENC_RHYTHM_SONG_STEP:
        case ENC_RHYTHM_SONG_PART:
        case ENC_RHYTHM_SONG_REPEAT:
            song_change(encoder[encoderNum_].function, false);
            break;

        case ENC_IO_CPU:
            break;  // read-only

        case ENC_IO_CPU_PEAK:
            cpu_resetPeak();
            break;

        case ENC_NOTE_MODE:
        case ENC_NOTE_NOTE:
        case ENC_NOTE_ROOT:
        case ENC_NOTE_SCALE:
        case ENC_NOTE_GLIDE:
        case ENC_NOTE_TRANSPOSE:
        case ENC_NOTE_FOLLOW:
            note_change(encoder[encoderNum_].function, false);
            break;

        case ENC_SYNTH_WAVE:
        case ENC_SYNTH_CUTOFF:
        case ENC_SYNTH_RESO:
        case ENC_SYNTH_ENVAMT:
        case ENC_SYNTH_DECAY:
        case ENC_SYNTH_GATE:
        case ENC_SYNTH_DRIVE:
        case ENC_SYNTH_DETUNE:
        case ENC_SYNTH_SUB:
        case ENC_SYNTH_NOISE:
        case ENC_SYNTH_LFO_SHAPE:
        case ENC_SYNTH_LFO_RATE:
        case ENC_SYNTH_LFO_DEPTH:
        case ENC_SYNTH_LFO_TARGET:
        case ENC_SYNTH_LFO_RETRIG:
        case ENC_SYNTH_PRESET:
        case ENC_SYNTH_SAVE_SLOT:
        case ENC_SYNTH_ATTACK:
        case ENC_SYNTH_AMP_DECAY:
        case ENC_SYNTH_SUSTAIN:
        case ENC_SYNTH_RELEASE:
        case ENC_SYNTH_FILTER_TYPE:
        case ENC_SYNTH_PAD_HOLD:
        case ENC_SYNTH_FM_RATIO:
        case ENC_SYNTH_FM_AMOUNT:
        case ENC_SYNTH_FM_DECAY:
        case ENC_SYNTH_PITCH_ENV:
        case ENC_SYNTH_PITCH_DECAY:
        case ENC_SYNTH_FILT_ATTACK:
            synth_change(encoder[encoderNum_].function, false);
            break;

        case ENC_FILE_AUTOSAVE:
            autosave_setEnabled(false);
            break;

        case ENC_NAME_CHAR:
            name_changeChar(false);
            break;

        case ENC_NAME_CURSOR:
            name_moveCursor(false);
            break;

        case ENC_NAME_RANDOM:
            name_random();
            break;

        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Led Functions -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::led_initialize(bool animate_) {
    /// @brief Configures MCP23017 I2C port expanders to drive Layer and EQ LEDs.
    /// Sets up I/O direction, pull-ups, and polarity before optionally playing a startup sequence.

    uint8_t dataOut = 0x00;     // 0 = Output / Normal Polarity
    uint8_t dataPullUp = 0xFF;  // 1 = Enable internal pull-up resistors

    // 1. LAYER LED CONTROLLER SETUP (I2C Address: 0x22)
    // Shifted left for 8-bit I2C addressing protocol
    ledLayer.address = (MCP23017_I2C_ADDRESS | 0x22) << 1;

    // Set all Port A and Port B pins as Outputs
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_IODIRA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_IODIRB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);

    // Set Normal Polarity (logic high = LED on)
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_IPOLA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_IPOLB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);

    // Enable Pull-ups for stability on all pins
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_GPPUA, I2C_MEMADD_SIZE_8BIT, &dataPullUp, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_GPPUB, I2C_MEMADD_SIZE_8BIT, &dataPullUp, 1, 1);

    // Disable Interrupt-on-change for LED driving mode
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_INTCONA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_INTCONB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);

    // Turn off all Layer LEDs initially
    i2c4_txData[0] = 0x00;
    i2c4_txData[1] = 0x00;
    HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);

    // 2. EQ LED CONTROLLERS SETUP (Left: 0x23, Right: 0x24)
    ledEqLeft.address = (MCP23017_I2C_ADDRESS | 0x23) << 1;
    ledEqRight.address = (MCP23017_I2C_ADDRESS | 0x24) << 1;

    // Helper Lambda/Block to initialize EQ hardware registers
    // Configure Left Channel Expander
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_IODIRA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_IODIRB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_IPOLA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_IPOLB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_GPPUA, I2C_MEMADD_SIZE_8BIT, &dataPullUp, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_GPPUB, I2C_MEMADD_SIZE_8BIT, &dataPullUp, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_INTCONA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_INTCONB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);

    // Configure Right Channel Expander
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_IODIRA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_IODIRB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_IPOLA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_IPOLB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_GPPUA, I2C_MEMADD_SIZE_8BIT, &dataPullUp, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_GPPUB, I2C_MEMADD_SIZE_8BIT, &dataPullUp, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_INTCONA, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_INTCONB, I2C_MEMADD_SIZE_8BIT, &dataOut, 1, 1);

    // 3. STARTUP ANIMATION (Optional)
    if (animate_) {
        // Rise Stage: Fill VU meter bars based on pre-defined library data
        for (int8_t i = 0; i < 12; i++) {
            HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&kLedEqDataLibrary[i], 2, 1);
            HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&kLedEqDataLibrary[i], 2, 1);
            HAL_Delay(50);
        }

        // Fall Stage: Drain VU meter bars
        for (int8_t i = 12; i >= 0; i--) {
            HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&kLedEqDataLibrary[i], 2, 1);
            HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&kLedEqDataLibrary[i], 2, 1);
            HAL_Delay(50);
        }
    }

    // 4. FINAL STAGE: Ensure all EQ LEDs are off and registers are cleared
    i2c4_txData[0] = 0x00;
    i2c4_txData[1] = 0x00;
    HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);
    HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);
}

void Controller::led_action() {
    /// @brief Orchestrates the LED update cycle via I2C DMA.
    /// Priority: 1. Layer Selection > 2. Beat Playback > 3. EQ Level Meters.

    // 1. BUS AVAILABILITY CHECK
    // Exit if I2C4 bus is not ready for DMA
    if (HAL_I2C_GetState(&hi2c4) != HAL_I2C_STATE_READY) {
        return;
    }

    // 2. LAYER SELECTION LEDS (Highest Priority)
    // Updates when the user switches between different instrument layers.
    if (ledLayerSelectFlag) {
        i2c4_txData[0] = ledLayerSelectData & 0xFF;
        i2c4_txData[1] = ledLayerSelectData >> 8;
        if (HAL_I2C_Mem_Write_DMA(&hi2c4, ledLayer.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2) == HAL_OK) {
            ledLayerSelectFlag = false;
            return;
        }
    }

    // 3. BEAT PLAYBACK LEDS
    // Visualizes real-time hits for each layer on the sequencer grid.
    if (ledLayerPlayMode) {
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            // Set bit if the sequencer triggered a start flag.
            if (layerPlayData[i].beatStartFlag) {
                layerPlayData[i].beatStartFlag = false;
                ledLayerPlayData |= (1U << i);
                ledLayerPlayFlag = true;
            }

            // Clear bit when the hit finishes.
            if (layerPlayData[i].beatEndFlag) {
                layerPlayData[i].beatEndFlag = false;
                ledLayerPlayData &= ~(1U << i);
                ledLayerPlayFlag = true;
            }
        }

        if (ledLayerPlayFlag) {
            i2c4_txData[0] = ledLayerPlayData & 0xFF;
            i2c4_txData[1] = ledLayerPlayData >> 8;

            if (HAL_I2C_Mem_Write_DMA(&hi2c4, ledLayer.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2) == HAL_OK) {
                ledLayerPlayFlag = false;
                return;
            }
        }
    }

    // 4. EQ LEVEL METERS (Lowest Priority)
    // Simulates analog VU meter physics with peak detection and linear decay.
    if (ledEqPlayMode) {
        // --- Phase A: Level Processing ---
        for (uint8_t i = 0; i < 2; i++) {
            if (ledEqReadFlag[i]) {
                float inputVal = (i == 0) ? audioPeak_L : audioPeak_R;

                // Clear peak registers after reading.
                if (i == 0)
                    audioPeak_L = 0.0f;
                else
                    audioPeak_R = 0.0f;

                // Trigger UI clipping alert if signal exceeds headroom.
                if (inputVal > 0.95f) limitAlertShowFlag = true;

                // Physics: Peak Hold vs Decay.
                float current = ledEqCurrentLevel[i];
                if (inputVal > current) {
                    current = inputVal;
                } else {
                    current -= kLedEqDecayRate;
                    if (current < 0.0f) current = 0.0f;
                }

                ledEqCurrentLevel[i] = current;
                ledEqReadFlag[i] = false;
                ledEqDrawFlag[i] = true;
            }
        }

        // --- Phase B: Hardware Transmission ---
        // Uses ledEqPriorityChannel to alternate between L/R to ensure both update evenly.
        for (uint8_t k = 0; k < 2; k++) {
            uint8_t i = (ledEqPriorityChannel + k) % 2;

            if (ledEqDrawFlag[i]) {
                // Map the 0.0-1.0 float to a 12-step LED ladder index.
                float level = fminf(ledEqCurrentLevel[i], 1.0f);
                uint16_t refValue = (uint16_t)(level * 32767.0f);
                uint8_t ledIndex = 12;

                for (uint8_t j = 0; j < 12; j++) {
                    if (refValue < kLedEqThresholdDataLibrary[j]) {
                        ledIndex = j;
                        break;
                    }
                }

                // Delta Check: Only transmit if the LED pattern has actually changed.
                uint16_t bitmask = kLedEqDataLibrary[ledIndex];
                Led* targetLed = (i == 0) ? &ledEqLeft : &ledEqRight;
                uint8_t targetAddress = (i == 0) ? ledEqLeft.address : ledEqRight.address;

                if (bitmask != targetLed->previousState) {
                    i2c4_txData[0] = bitmask & 0xFF;
                    i2c4_txData[1] = (bitmask >> 8) & 0xFF;

                    if (HAL_I2C_Mem_Write_DMA(&hi2c4, targetAddress, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2) == HAL_OK) {
                        targetLed->previousState = bitmask;
                        ledEqDrawFlag[i] = false;
                        ledEqPriorityChannel = (i == 0) ? 1 : 0;
                        return;
                    }
                } else {
                    ledEqDrawFlag[i] = false;
                }
            }
        }
    }
}

