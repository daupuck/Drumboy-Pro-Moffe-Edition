// Real-time audio engine: DAC, transport and per-block DSP.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Dac Functions -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::dac_initialize() { dac.initialize(); }

////////////////////////////////////////////////////////////////////////////////
/* Play Functions ------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::record() {
    /// @brief Toggles recording state and synchronizes transport/icon state.

    // 1. TRANSPORT STATE BRANCH
    // Resolve behavior for idle, recording, and play-only states.
    if ((!playActive) && (!recordActive)) {
        bool usePrecount = false;

        // 3. IDLE -> RECORD+PLAY START
        // Start playback/recording and optionally arm metronome precount.
        // In slave mode the external clock defines the start point; skip precount.
        if ((metronome.active) && (metronome.precount) && (!io.sync.slaveMode)) {
            usePrecount = true;
            metronome.precountState = true;
            metronome.precounter = 0;
            metronome.countDown = rhythm.measure;

            // Clear any stale pending hits from a previous precount take.
            for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                precountPendingBeat[i] = false;
                precountPendingHasNote[i] = false;
            }
        }

        // Reset LFO counters to start modulation cycles from known phase.
        for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
            lfoLibrary[i].sampleCounter = 0;
        }

        // Enqueue MIDI Start only when playback actually begins.
        // If precount is active, defer transmission until precount completes.
        if (io.midi.txTransport) {
            if (usePrecount) {
                io.midi.txStartPending = true;
            } else {
                static const uint8_t kMidiStart = 0xFA;
                enqueueMidiTx(&kMidiStart, 1);
                io.midi.txStartPending = false;
            }
        }

        // Update transport flags and front-panel icons.
        recordActive = true;
        recordIcon.flag = true;
        recordIcon.mode = true;
        stopFlag = false;
        resetFlag = false;
        playActive = true;
        if (io.syncOut == 1) {
            HAL_GPIO_WritePin(SYNC_OUT_GATE_GPIO_Port, SYNC_OUT_GATE_Pin, GPIO_PIN_SET);
        }
        resetIcon.flag = true;
        resetIcon.mode = false;
        playIcon.flag = true;
        playIcon.mode = true;
        stopIcon.flag = true;
        stopIcon.mode = false;
    }

    // 4. PLAY+RECORD -> PLAY ONLY
    // Disable recording while preserving playback state.
    else if ((playActive) && (recordActive)) {
        // If still in precount, stop entire transport instead of toggling record.
        if (metronome.precountState) {
            stop();
        } else {
            recordActive = false;
            stopFlag = false;
            resetFlag = false;
            recordIcon.flag = true;
            recordIcon.mode = false;
        }
    }

    // 5. PLAY ONLY -> PLAY+RECORD
    // Re-enable recording while playback is already running.
    else if ((playActive) && (!recordActive)) {
        recordActive = true;
        stopFlag = false;
        resetFlag = false;
        recordIcon.flag = true;
        recordIcon.mode = true;
    }
}

void Controller::play() {
    /// @brief Starts playback transport and refreshes related transport visuals/state.

    // 1. MIDI START TRANSMISSION
    // Enqueue MIDI Start (0xFA) before the clock timer fires its first tick.
    if (io.midi.txTransport) {
        static const uint8_t kMidiStart = 0xFA;
        enqueueMidiTx(&kMidiStart, 1);
        io.midi.txStartPending = false;
    }

    // 3. HARDWARE TIMER SYNCHRONIZATION
    // Force midi clock (tim4) and sequencer (tim15) to reset phase for perfect alignment.
    htim4.Instance->EGR = TIM_EGR_UG;
    htim15.Instance->EGR = TIM_EGR_UG;

    // 2. LFO RESET
    // Reset LFO counters to keep modulation start deterministic.
    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
        lfoLibrary[i].sampleCounter = 0;
    }

    // 3. TRANSPORT STATE UPDATE
    // Set playback flags and transport icons for running state.
    stopFlag = false;
    resetFlag = false;
    playActive = true;
    resetIcon.flag = true;
    resetIcon.mode = false;
    playIcon.flag = true;
    playIcon.mode = true;
    stopIcon.flag = true;
    stopIcon.mode = false;

    // 4. MODE-B GATE
    // MODE-B: gate goes high while transport is running.
    if (io.syncOut == 2) {
        HAL_GPIO_WritePin(SYNC_OUT_GATE_GPIO_Port, SYNC_OUT_GATE_Pin, GPIO_PIN_SET);
        io.sync.txTrsTickCounter = 0;
    }

    // 5. PLAY POINTER RECALCULATION
    // Recompute initial play beat pointer from current play interval.
    layerSong_calculatePlayBeatNum(0, 0, playInterval);

    // Song mode: from the top, start the chain at step 1 (switches to its first part).
    song_startIfAtTop();
}

void Controller::stop() {
    /// @brief Stops transport, clears pending transitions, and resets transport icons.

    // 1. MIDI STOP TRANSMISSION
    // Enqueue MIDI Stop (0xFC) when MIDI sync TX is active.
    if (io.midi.txTransport) {
        static const uint8_t kMidiStop = 0xFC;
        enqueueMidiTx(&kMidiStop, 1);
    }

    // 3. PRECOUNT STATE RESET
    // Clear metronome precount counters and hide countdown UI.
    if (metronome.precountState) {
        metronome.precountState = false;
        metronome.precounter = 0;
        metronome.countDownFlag = false;
        metronome.countDown = 0;
        lcd_clearCountDown();
    }

    // 3. BANK SHIFT CANCEL
    // Cancel pending bank shift and restore active bank indicator.
    if (bankShiftFlag) {
        bankShiftFlag = false;
        targetSongBank = activeSongBank;
        lcd_drawSongBankTab(activeSongBank, false);
    }

    // 4. TRANSPORT FLAG RESET
    // Put transport into fully stopped state.
    stopFlag = false;
    resetFlag = false;
    playActive = false;
    recordActive = false;
    io.midi.txStartPending = false;
    io.sync.txPulseCounter = 0;
    io.sync.txTrsTickCounter = 0;
    if (io.syncOut == 2) {
        HAL_GPIO_WritePin(SYNC_OUT_GATE_GPIO_Port, SYNC_OUT_GATE_Pin, GPIO_PIN_RESET);
    }

    // 5. ICON STATE UPDATE
    // Update transport icons to reflect stopped mode.
    resetIcon.flag = true;
    resetIcon.mode = false;
    playIcon.flag = true;
    playIcon.mode = false;
    stopIcon.flag = true;
    stopIcon.mode = true;
    recordIcon.flag = true;
    recordIcon.mode = false;

    // 6. PLAY POINTER RECALCULATION
    // Recompute play beat pointers for all layers after transport stop.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        layerSong_calculatePlayBeatNum(i, activeSongBank, playInterval);
    }
}

void Controller::reset() {
    /// @brief Resets transport to loop start and normalizes related runtime states.

    // Song mode: back to step 1 of the chain (also when already at the top).
    song_rewind();

    // 1. MIDI RESET TRANSMISSION
    // Enqueue MIDI Stop (0xFC) + Song Position Pointer to 0 (0xF2 0x00 0x00)
    // to signal all connected devices to rewind to position 1.
    if (io.midi.txTransport) {
        if (!resetFlag) {
            static const uint8_t kMidiTrigResetSysEx[5] = {
                kMidiSysExStart,
                kMidiSysExManufacturerNonCommercial,
                kMidiSysExTransportFamily,
                kMidiSysExTrigReset,
                kMidiSysExEnd};
            enqueueMidiTx(kMidiTrigResetSysEx, 5);
        }

        static const uint8_t kMidiStop = 0xFC;
        static const uint8_t kMidiSpp[3] = {0xF2, 0x00, 0x00};
        enqueueMidiTx(&kMidiStop, 1);
        enqueueMidiTx(kMidiSpp, 3);
    }

    // 3. RESET CONDITION VALIDATION
    // Continue only when play head moved or precount is active.
    if ((playInterval != 0) || (metronome.precountState)) {
        // 3. PRECOUNT STATE RESET
        // Clear metronome precount counters and hide countdown UI.
        if (metronome.precountState) {
            metronome.precountState = false;
            metronome.precounter = 0;
            metronome.countDownFlag = false;
            metronome.countDown = 0;
            lcd_clearCountDown();
        }

        // 4. BANK SHIFT CANCEL
        // Cancel pending bank shift and restore active bank indicator.
        if (bankShiftFlag) {
            bankShiftFlag = false;
            targetSongBank = activeSongBank;
            lcd_drawSongBankTab(activeSongBank, false);
        }

        // 5. TRANSPORT FLAG RESET
        // Put transport into non-running state and clear trigger flags.
        stopFlag = false;
        resetFlag = false;
        playActive = false;
        recordActive = false;
        io.midi.txStartPending = false;

        // 6. ICON STATE UPDATE
        // Update transport icons to reflect reset mode.
        resetIcon.flag = true;
        resetIcon.mode = true;
        playIcon.flag = true;
        playIcon.mode = false;
        stopIcon.flag = true;
        stopIcon.mode = false;
        recordIcon.flag = true;
        recordIcon.mode = false;

        // 7. PLAYHEAD RESET
        // Reset play interval and arm restart logic for next cycle.
        playInterval = 0;
        resetPlayFlag = true;
        io.sync.txPulseCounter = 0;
        io.sync.txTrsTickCounter = 0;
        if (io.syncOut == 2) {
            HAL_GPIO_WritePin(SYNC_OUT_GATE_GPIO_Port, SYNC_OUT_GATE_Pin, GPIO_PIN_RESET);
        }

        // 8. PLAY POINTER RECALCULATION
        // Recompute play beat pointers for all layers at loop start.
        for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
            layerSong_calculatePlayBeatNum(i, activeSongBank, playInterval);
        }

        // 9. LFO RESET TO PHASE START
        // Reset each LFO counter to its configured phase-start sample.
        for (uint8_t j = 0; j < kLfoLibrarySize; j++) {
            lfoLibrary[j].sampleCounter = lfoLibrary[j].sampleStart;
        }
    }
}

void Controller::triggerStop() {
    /// @brief Arms stop at musical boundary or stops immediately during precount.

    if (io.midi.txTransport) {
        static const uint8_t kMidiTrigStopSysEx[5] = {
            kMidiSysExStart,
            kMidiSysExManufacturerNonCommercial,
            kMidiSysExTransportFamily,
            kMidiSysExTrigStop,
            kMidiSysExEnd};
        enqueueMidiTx(kMidiTrigStopSysEx, 5);
    }

    // 1. TRIGGER RESOLUTION
    // Stop immediately in precount; otherwise arm stop trigger interval.
    if (metronome.precountState) {
        stop();
    } else {
        stopFlag = true;
        resetFlag = false;
        stopInterval = calculateTriggerInterval();
        resetIcon.flag = true;
        resetIcon.mode = false;

        // stopIcon.flag = true;
        // stopIcon.mode = true;
    }
}

void Controller::triggerReset() {
    /// @brief Arms reset at musical boundary or resets immediately during precount.

    if (io.midi.txTransport) {
        static const uint8_t kMidiTrigResetSysEx[5] = {
            kMidiSysExStart,
            kMidiSysExManufacturerNonCommercial,
            kMidiSysExTransportFamily,
            kMidiSysExTrigReset,
            kMidiSysExEnd};
        enqueueMidiTx(kMidiTrigResetSysEx, 5);
    }

    // 1. TRIGGER RESOLUTION
    // Reset immediately in precount; otherwise arm reset trigger interval.
    if (metronome.precountState) {
        reset();
    } else {
        stopFlag = false;
        resetFlag = true;
        resetInterval = calculateTriggerInterval();

        // resetIcon.flag = true;
        // resetIcon.mode = true;
        stopIcon.flag = true;
        stopIcon.mode = false;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Audio Functions -----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::processAudioBlock(uint32_t* inputBuffer_, uint32_t* outputBuffer_) {
    /// @brief Orchestrates the complete real-time audio processing pipeline, managing signal generation, sequential DSP stages, and final output buffering.

    // Start of the CPU meter measurement (see the end of this function).
    const uint32_t cpuStart = DWT_CYCCNT;

    // 0. I2S ERROR FLAG POLL
    // Count and clear overrun/underrun. The HAL never enables these interrupts, so polling
    // here is the only visibility; an rx overrun can permanently shift I2S word alignment.
    uint32_t i2sStatus = SPI3->SR;
    if (i2sStatus & SPI_SR_OVR) {
        i2sOvrCount++;
        SPI3->IFCR = SPI_IFCR_OVRC;
    }
    if (i2sStatus & SPI_SR_UDR) {
        i2sUdrCount++;
        SPI3->IFCR = SPI_IFCR_UDRC;
    }

    // 1. SYSTEM READINESS CHECK
    // Bypass processing during initialization menu to prevent audio artifacts.
    if (menu != INIT_MENU) {
        // 2. SIGNAL GENERATION & INPUT ACQUISITION
        // Acquire line-in, metronome, and layer source buffers.
        processAudioReceive(inputBuffer_);

        // Generate internal metronome click signal.
        processAudioMetronome();

        // Process active layer sample playback.
        processAudioLayer();

        // 3. SERIAL DSP CHAIN
        // Run EQ, Filter, Effect, and Reverb stages in order.
        processAudioEq();

        // Dual-bus filtering stage.
        processAudioFilter();

        // Dual-bus effect stage.
        processAudioEffect();

        // Global reverb stage.
        processAudioReverb();

        // 4. FINAL OUTPUT PROCESSING
        // Apply final mix and write to I2S output buffer.
        processAudioSend(outputBuffer_);
    }

    // 5. CPU METER
    // Cycles spent in this block against the real time since the previous block started
    // (~400k cycles at 550 MHz). Interrupts that preempt the audio ISR count as load too,
    // since they delay it just the same. Costs two register reads and one division per block.
    uint32_t busy = DWT_CYCCNT - cpuStart;
    const uint32_t period = cpuStart - cpuLastStart;
    cpuLastStart = cpuStart;
    if ((period > 0) && (period < 4000000u)) {  // skips the very first block and long stalls
        if (busy > 4000000u) busy = 4000000u;   // keeps busy * 1000 inside 32 bits
        const uint32_t load = (busy * 1000u) / period;
        if (load > cpuPeakPermille) cpuPeakPermille = static_cast<uint16_t>((load > 9999u) ? 9999u : load);
        cpuBusyAcc += busy;
        cpuPeriodAcc += period;
        if (++cpuBlockCount >= 128) {
            cpuLoadPermille = static_cast<uint16_t>((static_cast<uint64_t>(cpuBusyAcc) * 1000u) / cpuPeriodAcc);
            cpuBusyAcc = 0;
            cpuPeriodAcc = 0;
            cpuBlockCount = 0;
        }
    }
}

// Scale applied to the summed L+R input. 2^-32 maps a full-scale ADC pair to 1.0, which is
// the natural mapping and costs nothing in SNR - it undoes an attenuation rather than adding
// gain. The gate thresholds in Global.h are referenced to the signal after this scale, so
// changing it means scaling them by the same factor.
//   4.65661287e-10f = 2^-31, +6 dB          2.32830644e-10f = 2^-32, unity
//   1.16415322e-10f = 2^-33, the original >> 2 behaviour
#define LINE_IN_INPUT_SCALE 2.32830644e-10f

#define LINE_IN_LPF_COEF 0.643923f
#define LINE_IN_LPF_ENABLE 1   // 0: bypass stage 1 (lpf), 1: one-pole (light), 2: two-pole cascade (original, steeper)
#define LINE_IN_GATE_ENABLE 1  // 0: bypass stage 2 (noise gate), 1: normal operation

void Controller::processAudioReceive(uint32_t* inputBuffer_) {
    static float dcX1 = 0, dcY1 = 0;              // dc blocker state
    static float lpfState1 = 0, lpfState2 = 0;    // low-pass state (stage 2 used only at LINE_IN_LPF_ENABLE == 2)
    static float env = 0.0f, gateGain = 0.0f;     // expander state
    static uint32_t holdCounter = 0;

    if (mixer.liMute) {
        // Clear the filter and expander state too. Leaving it stale means the next enable
        // rings the dc blocker and can start with the expander already open.
        memset(audioReceive, 0, I2S_BLOCK_SIZE * sizeof(float));
        dcX1 = 0.0f;
        dcY1 = 0.0f;
        lpfState1 = 0.0f;
        lpfState2 = 0.0f;
        env = 0.0f;
        gateGain = 0.0f;
        holdCounter = 0;
        return;
    }
    const float currentVol = mixer.masterVolumeFloat * kMixerLiVolumeDataLibrary[mixer.liVolume].data;
    for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
        int32_t rawL = static_cast<int32_t>(inputBuffer_[i * 2]);
        int32_t rawR = static_cast<int32_t>(inputBuffer_[i * 2 + 1]);
        float rawSignal = (static_cast<float>(rawL) + static_cast<float>(rawR)) * LINE_IN_INPUT_SCALE;

        // stage 0: dc blocker (high-pass filter at ~5hz)
        float signal = rawSignal - dcX1 + (0.995f * dcY1);
        dcX1 = rawSignal;
        dcY1 = signal;

        // stage 1: low-pass (0: bypass, 1: one-pole light, 2: two-pole cascade)
#if LINE_IN_LPF_ENABLE == 1
        lpfState1 += LINE_IN_LPF_COEF * (signal - lpfState1);
        signal = lpfState1;
#elif LINE_IN_LPF_ENABLE == 2
        lpfState1 += LINE_IN_LPF_COEF * (signal - lpfState1);
        lpfState2 += LINE_IN_LPF_COEF * (lpfState1 - lpfState2);
        signal = lpfState2;
#endif

        // stage 2: downward expander
#if LINE_IN_GATE_ENABLE
        float absSig = fabsf(signal);
        env = (absSig > env) ? (env * 0.95f + absSig * 0.05f) : (env * 0.9995f + absSig * 0.0005f);
        // hold keeps the target pinned open briefly after a transient
        if (env > LINE_IN_GATE_THRESH_OPEN) {
            holdCounter = LINE_IN_GATE_HOLD_SAMPLES;
        } else if (holdCounter > 0) {
            holdCounter--;
        }
        // target gain: proportional between the thresholds, pinned at the floor below close
        float target;
        if (holdCounter > 0 || env >= LINE_IN_GATE_THRESH_OPEN) {
            target = 1.0f;
        } else if (env <= LINE_IN_GATE_THRESH_CLOSE) {
            target = LINE_IN_GATE_FLOOR_GAIN;
        } else {
            float frac = (env - LINE_IN_GATE_THRESH_CLOSE) / (LINE_IN_GATE_THRESH_OPEN - LINE_IN_GATE_THRESH_CLOSE);
            target = LINE_IN_GATE_FLOOR_GAIN + (1.0f - LINE_IN_GATE_FLOOR_GAIN) * frac;
        }
        // rising is fast and linear; falling eases toward the target exponentially
        if (target > gateGain) {
            gateGain = fminf(target, gateGain + LINE_IN_GATE_ATTACK);
        } else {
            gateGain += (target - gateGain) * (1.0f - LINE_IN_GATE_RELEASE_COEF);
        }
#else
        gateGain = 1.0f;
#endif

        float out = signal * gateGain * currentVol;
        audioReceive[i] = clamp(out, -1.0f, 1.0f);
    }
}

void Controller::processAudioMetronome() {
    /// @brief Generates metronome click audio by fetching PCM data from a library, normalizing it to float, and managing playback state and sample indexing.

    // 1. BUFFER RESET & ACTIVITY CHECK
    // Clear metronome buffer and early-exit when inactive.
    memset(audioMetronome, 0, I2S_BLOCK_SIZE * sizeof(float));

    // Metronome inactive: skip block processing.
    if (!metroPlayData.active) return;
    // 2. SAMPLE LOOP PROCESSING
    // Fetch PCM data, normalize to float, and mix into output buffer.
    for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
        // --- PCM Data Acquisition and Normalization ---
        // Fetch raw 16-bit PCM data from the metronome library based on measure type
        int16_t data16Bit = (int16_t)kMetronomeDataLibrary[metroPlayData.sample][metroPlayData.barMeasure][metroPlayData.counter];

        // Convert 16-bit integer to float range [-1.0, 1.0] and apply volume multiplier
        float dataFloat = (float)data16Bit * NORMALIZE_FACTOR_16BIT;
        float sample = dataFloat * metroPlayData.volumeMultiplier;

        // Mix the processed sample into the local metronome buffer
        audioMetronome[i] += sample;

        // --- Playback State Management ---
        // Advance sample index and stop when end of click sample is reached.
        if (metroPlayData.counter < metroPlayData.counterMax) {
            metroPlayData.counter++;
        } else {
            // End of sample reached: Reset counter and deactivate metronome trigger
            metroPlayData.counter = 0;
            metroPlayData.active = false;

            // Terminate processing for the remainder of the block to save CPU cycles
            break;
        }
    }
}

void Controller::processAudioLayer() {
    /// @brief Manages real-time audio playback for all layers, featuring sample retriggering, bitcrushing, linear interpolation, and multiple playback direction modes.

    // 1. CONSTANT INITIALIZATION
    // Prepare constants for 24-bit conversion and bit-depth operations.
    const float kScale24 = 16777215.0f;
    const float kInvScale24 = 1.0f / 16777215.0f;

    // 2. LAYER ITERATION
    // Process playback pipeline for each layer independently.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        LayerPlayData& lD = layerPlayData[i];
        BeatPlayData& tBD = lD.tempBeatPlayData;
        BeatPlayData& aBD = lD.activeBeatPlayData;

        // Reset the audio buffer for the current layer
        memset(audioLayer[i], 0, I2S_BLOCK_SIZE * sizeof(float));

        // --- Handle Sample Retriggering ---
        if (aBD.newBeatFlag) {
            if (aBD.counterFade >= 49 || !aBD.active) {
                aBD.newBeatFlag = false;
                lD.beatStartFlag = true;
                lD.beatEndFlag = false;

                // Transfer temporary beat data to the active state
                memcpy(&aBD, &lD.tempBeatPlayData, sizeof(BeatPlayData));
                memset(&lD.tempBeatPlayData, 0x00, sizeof(BeatPlayData));

                // Deactivate layer if necessary and trigger MIDI Note Off
                if (!aBD.active) {
                    io.midi.txTriggerNoteOff[i] = true;
                    continue;
                }
            }
        }

        // Skip processing if the layer is inactive
        if (!aBD.active) continue;

        float* targetBuffer = audioLayer[i];

        // TONE glide: ease the playback speed towards the new note once per block (0.73 ms
        // steps are inaudible), so the per-sample loops below stay exactly as they were.
        if (aBD.glideCoef > 0.0f) {
            aBD.increment = aBD.glideTarget + ((aBD.increment - aBD.glideTarget) * aBD.glideCoef);
            if (fabsf(aBD.increment - aBD.glideTarget) < 0.0001f) {
                aBD.increment = aBD.glideTarget;
                aBD.glideCoef = 0.0f;
            }
        }

        // 3. PROCESSING PATH RESOLUTION
        // Use fast path for unity pitch + forward direction; else use interpolated slow path.
        bool isFastPath = (aBD.increment == 1.0f) && (aBD.directionMode == 0);

        if (isFastPath) {
            // --- Optimized Fast Path Processing ---
            uint32_t remainder = (uint32_t)(aBD.counterEnd - aBD.counter);
            uint16_t copySize = (remainder > I2S_BLOCK_SIZE) ? I2S_BLOCK_SIZE : (uint16_t)remainder;
            float* sourceAddress = (float*)(aBD.ramAddress + ((uint32_t)aBD.counter * 4));

            // Cache RAM data into local stack buffer for processing
            float localCache[I2S_BLOCK_SIZE];
            if (copySize > 0) memcpy(localCache, sourceAddress, copySize * sizeof(float));

            if (aBD.counter == aBD.counterStart) io.midi.txTriggerNoteOn[i] = true;

            bool bitCrushActive = (aBD.resolutionCoef != 0x00FFFFFF);

            for (uint16_t j = 0; j < copySize; j++) {
                float sample = localCache[j];

                // Apply Bitcrush effect if the resolution coefficient is modified
                if (bitCrushActive) {
                    float valShifted = (sample + 1.0f) * 0.5f;
                    uint32_t tempUnsigned = (uint32_t)(valShifted * kScale24);
                    tempUnsigned &= aBD.resolutionCoef;
                    float valBack = (float)tempUnsigned * kInvScale24;
                    sample = (valBack * 2.0f) - 1.0f;
                }

                // Handle fade-in/fade-out logic
                if ((aBD.newBeatFlag) && (!aBD.fadeAction)) aBD.fadeAction = true;
                if ((!aBD.fadeAction) && (aBD.fadeFlag) && ((aBD.counter + j) >= aBD.counterFadeStart)) {
                    aBD.fadeAction = true;
                }

                if (aBD.fadeAction) {
                    sample *= kFadeVolumeMultiplier[aBD.counterFade];
                    if (aBD.counterFade < 49) aBD.counterFade++;
                }

                // Apply final volume and store in output buffer
                targetBuffer[j] = sample * aBD.volumeMultiplier;
            }

            // Update sample counter and check for playback end
            aBD.counter += (float)copySize;
            if (copySize < I2S_BLOCK_SIZE) {
                lD.beatStartFlag = false;
                lD.beatEndFlag = true;
                aBD.active = false;
                aBD.counter = 0.0f;
                io.midi.txTriggerNoteOff[i] = true;
            }
        } else {
            // 4. SLOW PATH PROCESSING
            // Handle interpolated read and multi-direction playback modes.
            for (uint16_t k = 0; k < I2S_BLOCK_SIZE; k++) {
                bool endFlag = false;

                // Perform Linear Interpolation for pitch shifting
                float counterFloat = aBD.counter;
                uint32_t counterInt = (uint32_t)counterFloat;
                float counterDiff = counterFloat - (float)counterInt;
                uint32_t ramAddress = aBD.ramAddress + (counterInt * 4);

                // Helper lambda for reading RAM with integrated bitcrushing
                auto getSample = [&](uint32_t addr) -> float {
                    float val = *(volatile float*)addr;
                    if (aBD.resolutionCoef != 0x00FFFFFF) {
                        float valShifted = (val + 1.0f) * 0.5f;
                        uint32_t tempUnsigned = (uint32_t)(valShifted * kScale24);
                        tempUnsigned &= aBD.resolutionCoef;
                        float valBack = (float)tempUnsigned * kInvScale24;
                        val = (valBack * 2.0f) - 1.0f;
                    }
                    return val;
                };

                float currentSample;
                if (counterDiff == 0.0f) {
                    currentSample = getSample(ramAddress);
                } else {
                    // Linear interpolation between two samples
                    currentSample = getSample(ramAddress) + ((getSample(ramAddress + 4) - getSample(ramAddress)) * counterDiff);
                }

                // Handle Fading Triggers
                if ((aBD.newBeatFlag) && (!aBD.fadeAction)) aBD.fadeAction = true;
                if (!aBD.fadeAction) {
                    bool fadeTrigger = (aBD.directionMode == 0) ? (aBD.fadeFlag && aBD.counter >= aBD.counterFadeStart) : (aBD.fadeFlag && aBD.counter <= aBD.counterFadeStart);
                    if (fadeTrigger) aBD.fadeAction = true;
                }

                if (aBD.fadeAction) {
                    currentSample *= kFadeVolumeMultiplier[aBD.counterFade];
                    if (aBD.counterFade < 49)
                        aBD.counterFade++;
                    else if (!aBD.newBeatFlag)
                        endFlag = true;
                }

                // Store sample with volume scaling
                targetBuffer[k] = currentSample * aBD.volumeMultiplier;

                // --- Direction and Counter Management ---
                switch (aBD.directionMode) {
                    case 0:  // Forward playback
                        if (aBD.counter == aBD.counterStart) io.midi.txTriggerNoteOn[i] = true;
                        aBD.counter += aBD.increment;
                        if (aBD.counter > (aBD.counterEnd - 1.0f)) endFlag = true;
                        break;

                    case 1:  // Backward playback
                        if (aBD.counter == aBD.counterEnd - 1.0f) io.midi.txTriggerNoteOn[i] = true;
                        aBD.counter -= aBD.increment;
                        if (aBD.counter < aBD.counterStart) endFlag = true;
                        break;

                    case 2:  // Ping (Forward then Backward)
                        switch (aBD.directionPhase) {
                            case 0:  // Forward phase
                                if (aBD.counter == 0.0f) io.midi.txTriggerNoteOn[i] = true;
                                aBD.counter += aBD.increment;
                                if (aBD.counter > (aBD.counterEnd - 1.0f)) {
                                    aBD.counter = aBD.counterEnd - 1.0f;
                                    aBD.directionPhase = 1;
                                    aBD.fadeFlag = true;
                                }
                                break;

                            case 1:  // Backward phase
                                aBD.counter -= aBD.increment;
                                if (aBD.counter < aBD.counterStart) endFlag = true;
                                break;
                        }
                        break;

                    case 3:  // Pong (Backward then Forward)
                        switch (aBD.directionPhase) {
                            case 0:  // Backward phase
                                if (aBD.counter == aBD.counterEnd - 1.0f) io.midi.txTriggerNoteOn[i] = true;
                                aBD.counter -= aBD.increment;
                                if (aBD.counter < aBD.counterStart) {
                                    aBD.counter = 0.0f;
                                    aBD.directionPhase = 1;
                                    aBD.fadeFlag = true;
                                }
                                break;

                            case 1:  // Forward phase
                                aBD.counter += aBD.increment;
                                if (aBD.counter > (aBD.counterEnd - 1.0f)) endFlag = true;
                                break;
                        }
                        break;
                }

                // 5. END-OF-SAMPLE HANDLING
                // Finalize beat state and emit MIDI Note Off on playback end.
                if (endFlag) {
                    lD.beatStartFlag = false;
                    lD.beatEndFlag = true;
                    aBD.active = false;
                    aBD.counter = 0.0f;
                    io.midi.txTriggerNoteOff[i] = true;
                    break;
                }
            }
        }
    }
}

void Controller::processAudioEq() {
    /// @brief Accumulates multi-source audio and processes it through a sequential 6-band parametric EQ chain including Low Shelf, High Shelf, and four Peak filters.

    // 1. EQ BUS RESET
    // Clear EQ accumulation buffer for current block.
    memset(audioEq, 0, I2S_BLOCK_SIZE * sizeof(float));

    // 2. INPUT ACCUMULATION
    // Accumulate line-in, metronome, and layer signals routed to EQ bus.
    if (mixer.liMixerInMode == MIXER_IN_EQ) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            audioEq[i] += audioReceive[i];
        }
    }

    // Accumulate metronome signal
    if (metroPlayData.mixerInMode == MIXER_IN_EQ) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            audioEq[i] += audioMetronome[i];
        }
    }

    // Accumulate all active layer signals
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        if (layerPlayData[i].activeBeatPlayData.mixerInMode == MIXER_IN_EQ) {
            for (uint16_t k = 0; k < I2S_BLOCK_SIZE; k++) {
                audioEq[k] += audioLayer[i][k];
            }
        }
    }

    // 3. BYPASS CHECK
    // Early return when EQ and EQ transition engines are inactive.
    if (!eq.active && !eq.genTransition.active) return;

    // 4. ACTIVE BAND RESOLUTION
    // Mark active bands where gain differs from zero.
    bool activeLS = (eq.gainLowShelf != kEqGainZero);
    bool activeHS = (eq.gainHighShelf != kEqGainZero);
    bool activeP0 = (eq.gainPeak[0] != kEqGainZero);
    bool activeP1 = (eq.gainPeak[1] != kEqGainZero);
    bool activeP2 = (eq.gainPeak[2] != kEqGainZero);
    bool activeP3 = (eq.gainPeak[3] != kEqGainZero);

    // 5. BIQUAD CHAIN PROCESSING
    // Process sample-by-sample through enabled EQ stages.
    for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
        float input = audioEq[i];
        float processed = input;

        // Apply sequential biquad filters (Direct Form calculation)
        processed = processEqBiquad(eq.filterLowShelf, processed, activeLS);
        processed = processEqBiquad(eq.filterHighShelf, processed, activeHS);
        processed = processEqBiquad(eq.filterPeak[0], processed, activeP0);
        processed = processEqBiquad(eq.filterPeak[1], processed, activeP1);
        processed = processEqBiquad(eq.filterPeak[2], processed, activeP2);
        processed = processEqBiquad(eq.filterPeak[3], processed, activeP3);

        // Handle final output with wet/dry transition support
        if (eq.genTransition.active) {
            audioEq[i] = (processed * eq.genTransition.activeWet) + (input * eq.genTransition.activeDry);
        } else {
            audioEq[i] = processed;
        }
    }
}

void Controller::processAudioFilter() {
    /// @brief Accumulates audio from EQ, Line-in, Metronome, and Layers into two filter buses and processes them serially through biquad filter engines.

    // 1. FILTER BUS RESET
    // Clear both filter buses before accumulation.
    memset(audioFilter[0], 0, I2S_BLOCK_SIZE * sizeof(float));
    memset(audioFilter[1], 0, I2S_BLOCK_SIZE * sizeof(float));

    // 2. INPUT ACCUMULATION
    // Route EQ, line-in, metronome, and layer signals into filter buses.
    for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
        audioFilter[0][i] += audioEq[i];
    }

    // Accumulate line-in signal based on routing mode
    if (mixer.liMixerInMode == MIXER_IN_FILTER_0) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioFilter[0][i] += audioReceive[i];
    } else if (mixer.liMixerInMode == MIXER_IN_FILTER_1) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioFilter[1][i] += audioReceive[i];
    }

    // Accumulate metronome signal if active
    if (metroPlayData.active) {
        if (metroPlayData.mixerInMode == MIXER_IN_FILTER_0) {
            for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioFilter[0][i] += audioMetronome[i];
        } else if (metroPlayData.mixerInMode == MIXER_IN_FILTER_1) {
            for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioFilter[1][i] += audioMetronome[i];
        }
    }

    // Accumulate all active layer signals based on their routing
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        MixerInMode mode = (MixerInMode)layerPlayData[i].activeBeatPlayData.mixerInMode;
        if (mode == MIXER_IN_FILTER_0) {
            for (int j = 0; j < I2S_BLOCK_SIZE; j++) audioFilter[0][j] += audioLayer[i][j];
        } else if (mode == MIXER_IN_FILTER_1) {
            for (int j = 0; j < I2S_BLOCK_SIZE; j++) audioFilter[1][j] += audioLayer[i][j];
        }
    }

    // 3. FILTER ENGINE 0 PROCESSING
    // Process bus-0 through filter-0 and feed result into bus-1.
    if ((filter[0].active) || (filter[0].genTransition.active)) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            float audio = audioFilter[0][i];
            audio = processAudioFilterHelper(0, audio);
            audioFilter[0][i] = audio;
            audioFilter[1][i] += audio;  // Accumulate processed output into Bus 1
        }
    } else {
        // Bypass: Filter 0 is inactive, sum raw Bus 0 signal into Bus 1
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            audioFilter[1][i] += audioFilter[0][i];
        }
    }

    // 4. FILTER ENGINE 1 PROCESSING
    // Process bus-1 through filter-1 before next stage.
    if ((filter[1].active) || (filter[1].genTransition.active)) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            float audio = audioFilter[1][i];
            audio = processAudioFilterHelper(1, audio);
            audioFilter[1][i] = audio;
        }
    }
}

void Controller::processAudioEffect() {
    /// @brief Accumulates audio sources into two parallel effect buses and processes them serially through internal effect engines with transition support.

    // 1. EFFECT BUS RESET
    // Clear both effect buses before accumulation.
    memset(audioEffect[0], 0, I2S_BLOCK_SIZE * sizeof(float));
    memset(audioEffect[1], 0, I2S_BLOCK_SIZE * sizeof(float));

    // 2. INPUT ACCUMULATION
    // Route filter output, line-in, metronome, and layer signals into effect buses.
    for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
        audioEffect[0][i] += audioFilter[1][i];
    }

    // Accumulate line-in based on routing mode
    if (mixer.liMixerInMode == MIXER_IN_EFFECT_0) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioEffect[0][i] += audioReceive[i];
    } else if (mixer.liMixerInMode == MIXER_IN_EFFECT_1) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioEffect[1][i] += audioReceive[i];
    }

    // Accumulate metronome signal if active
    if (metroPlayData.active) {
        if (metroPlayData.mixerInMode == MIXER_IN_EFFECT_0) {
            for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioEffect[0][i] += audioMetronome[i];
        } else if (metroPlayData.mixerInMode == MIXER_IN_EFFECT_1) {
            for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioEffect[1][i] += audioMetronome[i];
        }
    }

    // Accumulate all active layer signals based on their routing
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        MixerInMode mode = (MixerInMode)layerPlayData[i].activeBeatPlayData.mixerInMode;
        if (mode == MIXER_IN_EFFECT_0) {
            for (int j = 0; j < I2S_BLOCK_SIZE; j++) audioEffect[0][j] += audioLayer[i][j];
        } else if (mode == MIXER_IN_EFFECT_1) {
            for (int j = 0; j < I2S_BLOCK_SIZE; j++) audioEffect[1][j] += audioLayer[i][j];
        }
    }

    // 3. EFFECT ENGINE 0 PROCESSING
    // Process bus-0 through effect-0 and feed result into bus-1.
    if ((effect[0].active) || (effect[0].genTransition.active)) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            float audio = audioEffect[0][i];
            audio = processAudioEffectHelper(0, audio);
            audioEffect[0][i] = audio;
            audioEffect[1][i] += audio;  // Send processed output to Bus 1
        }
    } else {
        // Bypass: Effect 0 is inactive, sum raw Bus 0 signal into Bus 1
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            audioEffect[1][i] += audioEffect[0][i];
        }
    }

    // 4. EFFECT ENGINE 1 PROCESSING
    // Process bus-1 through effect-1 before reverb stage.
    if ((effect[1].active) || (effect[1].genTransition.active)) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            float audio = audioEffect[1][i];
            audio = processAudioEffectHelper(1, audio);
            audioEffect[1][i] = audio;
        }
    }
}

void Controller::processAudioReverb() {
    /// @brief Accumulates inputs and applies a damped stereo reverb with pre-delay and surround enhancement.

    // 1. REVERB BUS RESET
    // Clear stereo reverb accumulation buffers for current block.
    memset(audioReverb_L, 0, I2S_BLOCK_SIZE * sizeof(float));
    memset(audioReverb_R, 0, I2S_BLOCK_SIZE * sizeof(float));

    // 2. INPUT ACCUMULATION
    // Sum effect, line-in, metronome, and layer signals into left reverb input.
    for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
        audioReverb_L[i] += audioEffect[1][i];
    }

    // Accumulate line-in
    if (mixer.liMixerInMode == MIXER_IN_REVERB) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioReverb_L[i] += audioReceive[i];
    }

    // Accumulate metronome
    if (metroPlayData.active && metroPlayData.mixerInMode == MIXER_IN_REVERB) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) audioReverb_L[i] += audioMetronome[i];
    }

    // Accumulate layers
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        if (layerPlayData[i].activeBeatPlayData.mixerInMode == MIXER_IN_REVERB) {
            for (uint16_t j = 0; j < I2S_BLOCK_SIZE; j++) audioReverb_L[j] += audioLayer[i][j];
        }
    }

    // 3. REVERB ENGINE PROCESSING
    // Run pre-delay, comb/all-pass, and surround output generation.
    if ((reverb.active) || (reverb.genTransition.active)) {
        // Cache parameters locally for faster access during the sample loop
        float cFB = reverb.combFeedback;
        float cD1 = reverb.combDecay1;
        float cD2 = reverb.combDecay2;
        float apFB = reverb.apassFeedback;

        // Pre-calculate scaler for comb filter inputs to prevent clipping
        const float kCombInputScale = 0.015f;

        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            float inputSample = audioReverb_L[i];

            // Apply anti-denormal noise to prevent CPU spikes on low signals
            inputSample += REVERB_ANTI_DENORMAL;

            // 3.1 PRE-DELAY PROCESSING
            reverb.preDelayBuffer[reverb.preDelay_recordInterval] = inputSample;
            float reverbInput = reverb.preDelayBuffer[reverb.preDelay_playInterval];

            if (++reverb.preDelay_playInterval >= reverb.kPreDelayBufferSize) reverb.preDelay_playInterval = 0;
            if (++reverb.preDelay_recordInterval >= reverb.kPreDelayBufferSize) reverb.preDelay_recordInterval = 0;

            // 3.2 COMB FILTERS (Parallel)
            float inputComb = reverbInput * kCombInputScale;

            reverb.comb1Out = processReverbComb(reverb.comb1Buffer, reverb.comb1Index, reverb.comb1Size, reverb.comb1Filter, inputComb, cFB, cD1, cD2);
            reverb.comb2Out = processReverbComb(reverb.comb2Buffer, reverb.comb2Index, reverb.comb2Size, reverb.comb2Filter, inputComb, cFB, cD1, cD2);
            reverb.comb3Out = processReverbComb(reverb.comb3Buffer, reverb.comb3Index, reverb.comb3Size, reverb.comb3Filter, inputComb, cFB, cD1, cD2);
            reverb.comb4Out = processReverbComb(reverb.comb4Buffer, reverb.comb4Index, reverb.comb4Size, reverb.comb4Filter, inputComb, cFB, cD1, cD2);
            reverb.comb5Out = processReverbComb(reverb.comb5Buffer, reverb.comb5Index, reverb.comb5Size, reverb.comb5Filter, inputComb, cFB, cD1, cD2);
            reverb.comb6Out = processReverbComb(reverb.comb6Buffer, reverb.comb6Index, reverb.comb6Size, reverb.comb6Filter, inputComb, cFB, cD1, cD2);
            reverb.comb7Out = processReverbComb(reverb.comb7Buffer, reverb.comb7Index, reverb.comb7Size, reverb.comb7Filter, inputComb, cFB, cD1, cD2);
            reverb.comb8Out = processReverbComb(reverb.comb8Buffer, reverb.comb8Index, reverb.comb8Size, reverb.comb8Filter, inputComb, cFB, cD1, cD2);

            float combSum = reverb.comb1Out + reverb.comb2Out + reverb.comb3Out + reverb.comb4Out + reverb.comb5Out + reverb.comb6Out + reverb.comb7Out + reverb.comb8Out;

            // 3.3 ALL-PASS FILTERS (Serial Diffusion)
            reverb.apass1Out = processReverbAllpass(reverb.apass1Buffer, reverb.apass1Index, reverb.apass1Size, combSum, apFB);
            reverb.apass2Out = processReverbAllpass(reverb.apass2Buffer, reverb.apass2Index, reverb.apass2Size, reverb.apass1Out, apFB);
            reverb.apass3Out = processReverbAllpass(reverb.apass3Buffer, reverb.apass3Index, reverb.apass3Size, reverb.apass2Out, apFB);
            reverb.apass4Out = processReverbAllpass(reverb.apass4Buffer, reverb.apass4Index, reverb.apass4Size, reverb.apass3Out, apFB);

            float processedSignal = reverb.apass4Out;

            // 3.4 OUTPUT MIXING & SURROUND
            float finalReverb;
            float outL, outR;

            // Apply manual wet/dry mix
            float mixedSignal = (processedSignal * reverb.wetFloat) + (inputSample * reverb.dryFloat);

            // Handle Parameter Transitions
            if (reverb.genTransition.active) {
                finalReverb = (mixedSignal * reverb.genTransition.activeWet) + (inputSample * reverb.genTransition.activeDry);
                reverb.surroundBuffer[reverb.surround_recordInterval] = finalReverb;

                if ((reverb.genTransition.mode == REV_MODE_ACTIVE) || (reverb.genTransition.mode == REV_MODE_SURROUND)) {
                    outL = finalReverb;
                    outR = (reverb.surroundBuffer[reverb.surround_playInterval] * reverb.genTransition.activeWet) + (finalReverb * reverb.genTransition.activeDry);
                } else {
                    outL = finalReverb;
                    outR = reverb.surroundBuffer[reverb.surround_playInterval];
                }
            } else {
                finalReverb = mixedSignal;
                reverb.surroundBuffer[reverb.surround_recordInterval] = finalReverb;
                outL = finalReverb;
                outR = reverb.surroundBuffer[reverb.surround_playInterval];
            }

            // Update surround circular buffer indices
            if (++reverb.surround_playInterval >= reverb.kSurroundBufferSize) reverb.surround_playInterval = 0;
            if (++reverb.surround_recordInterval >= reverb.kSurroundBufferSize) reverb.surround_recordInterval = 0;

            // Final block output write
            audioReverb_L[i] = outL;
            audioReverb_R[i] = outR;
        }
    } else {
        // Bypass mode: Effect is inactive, copy mono accumulation to stereo output
        memcpy(audioReverb_R, audioReverb_L, I2S_BLOCK_SIZE * sizeof(float));
    }
}

void Controller::processAudioSend(uint32_t* outputBuffer) {
    /// @brief Accumulates all audio sources, applies master volume and LPF, and performs final output processing including soft clipping and peak detection.

    // 1. SEND BUS ACCUMULATION
    // Merge reverb output with routed line-in, metronome, and layer signals.
    memcpy(audioSend_L, audioReverb_L, I2S_BLOCK_SIZE * sizeof(float));
    memcpy(audioSend_R, audioReverb_R, I2S_BLOCK_SIZE * sizeof(float));

    // Accumulate line-in signal
    if (mixer.liMixerInMode == MIXER_IN_SEND) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            audioSend_L[i] += audioReceive[i];
            audioSend_R[i] += audioReceive[i];
        }
    }

    // Accumulate metronome signal
    if (metroPlayData.active && metroPlayData.mixerInMode == MIXER_IN_SEND) {
        for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
            audioSend_L[i] += audioMetronome[i];
            audioSend_R[i] += audioMetronome[i];
        }
    }

    // Accumulate all active layer signals
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        if (layerPlayData[i].activeBeatPlayData.mixerInMode == MIXER_IN_SEND) {
            float* layerBuffer = audioLayer[i];
            for (uint16_t j = 0; j < I2S_BLOCK_SIZE; j++) {
                float audio = layerBuffer[j];
                audioSend_L[j] += audio;
                audioSend_R[j] += audio;
            }
        }
    }

    // Accumulate the sample preview (Layer Inst audition): dry, mono, before master
    // mute/volume/limiter. Plays only samples the main loop has already written (filled);
    // while streaming it simply waits. A stop request fades out across this block.
    if (previewPlay.playing) {
        const float* preview = reinterpret_cast<const float*>(RAM_PREVIEW_ADDRESS);
        const uint32_t filled = previewPlay.filled;
        const float gain = previewPlay.gain;
        const bool stopping = previewPlay.stopRequest;
        uint32_t counter = previewPlay.counter;
        for (uint16_t i = 0; (i < I2S_BLOCK_SIZE) && (counter < filled); i++) {
            const float fade = stopping ? (1.0f - (static_cast<float>(i + 1) / I2S_BLOCK_SIZE)) : 1.0f;
            const float audio = preview[counter++] * gain * fade;
            audioSend_L[i] += audio;
            audioSend_R[i] += audio;
        }
        previewPlay.counter = counter;
        if (stopping || ((counter >= filled) && !previewPlay.streaming)) {
            previewPlay.playing = false;
            previewPlay.stopRequest = false;
        }
    }

    // 2. MASTER MUTE CHECK
    // Output silence and return when master mute is enabled.
    if (mixer.masterMute) {
        memset(outputBuffer, 0, I2S_BLOCK_SIZE * I2S_CHANNEL_COUNT * sizeof(uint32_t));
        return;
    }

    // 3. FINAL OUTPUT FORMATTING
    // Apply gain/LPF/peak/clipping and write interleaved I2S samples.
    for (uint16_t i = 0; i < I2S_BLOCK_SIZE; i++) {
        float outL = audioSend_L[i];
        float outR = audioSend_R[i];

        // Apply master volume gains
        outL *= mixer.volumeLeftFloat;
        outR *= mixer.volumeRightFloat;

        // Apply low-pass filters if active
        if (lpf[0].active) outL = processAudioLpfHelper(0, outL);
        if (lpf[1].active) outR = processAudioLpfHelper(1, outR);

        // Calculate and update peak levels
        float absAudioMax_L = fabsf(outL);
        float absAudioMax_R = fabsf(outR);
        if (absAudioMax_L > audioPeak_L) audioPeak_L = absAudioMax_L;
        if (absAudioMax_R > audioPeak_R) audioPeak_R = absAudioMax_R;

        // Apply soft knee clipping to prevent digital distortion
        outL = softKneeClip(outL);
        outR = softKneeClip(outR);

        // Convert float samples to 32-bit integers
        int32_t sampleL = static_cast<int32_t>(outL * INT32_MAX_FLOAT);
        int32_t sampleR = static_cast<int32_t>(outR * INT32_MAX_FLOAT);

        // Write final samples to the interleaved I2S output buffer
        outputBuffer[i * 2] = static_cast<uint32_t>(sampleL);
        outputBuffer[i * 2 + 1] = static_cast<uint32_t>(sampleR);
    }
}

float Controller::processAudioFilterHelper(uint8_t filterNum_, float audio_) {
    /// @brief Computes a biquad filter sample with input drive saturation, slope selection, and smooth parameter transitions.

    // 1. FILTER STATE RESOLUTION
    // Resolve filter reference and initialize local input/output values.

    Filter& filter_ = filter[filterNum_];
    float input = audio_;
    float output;

    if ((filter_.active) || (filter_.genTransition.active)) {
        // 2. INPUT DRIVE APPLY
        // Convert drive control to gain multiplier and apply optional soft clip.

        float driveGain = 1.0f + ((float)filter_.drive * 0.04f);
        input *= driveGain;

        // Optional: Soft-clip the driven input to keep it within a musical range
        if (filter_.drive > 0) {
            input = softKneeClip(input);
        }

        // 3. BIQUAD CALCULATION
        // Compute filtered sample with anti-denormal protection.

        float filteredSample = (filter_.b0 * input) + (filter_.b1 * filter_.x1) + (filter_.b2 * filter_.x2) - (filter_.a1 * filter_.y1) - (filter_.a2 * filter_.y2) + FILTER_ANTI_DENORMAL;

        // 4. FILTER HISTORY UPDATE
        // Shift input/output delay-line history buffers.

        filter_.x2 = filter_.x1;
        filter_.x1 = input;
        filter_.y2 = filter_.y1;
        filter_.y1 = filteredSample;

        // 5. SLOPE & MIX RESOLUTION
        // Resolve slope mode and wet/dry transition mix.

        float finalFilterOut;

        // Determine filter slope (0 = 6dB, 1 = 12dB)
        if (filter_.slope == 0) {
            finalFilterOut = (filteredSample * 0.5f) + (input * 0.5f);
        } else {
            finalFilterOut = filteredSample;
        }

        // Apply transition mix or standard wet/dry levels
        float wetLevel, dryLevel;
        if (filter_.genTransition.active) {
            wetLevel = filter_.genTransition.activeWet;
            dryLevel = filter_.genTransition.activeDry;
        } else {
            wetLevel = filter_.wetFloat;
            dryLevel = filter_.dryFloat;
        }

        // Generate final mixed output
        output = (finalFilterOut * wetLevel) + (audio_ * dryLevel);
    } else {
        // 6. BYPASS OUTPUT
        // Return original input when filter path is inactive.

        output = input;
    }

    return output;
}

float Controller::processAudioEffectHelper(uint8_t effectNum_, float audio_) {
    /// @brief Routes and processes audio through the selected effect engine.
    /// Supports Delay, Chorus, Flanger, Phaser, Dynamics (Comp/Exp), and Distortion (OD/Dist/Crush).

    Effect& effect_ = effect[effectNum_];
    float input = audio_;
    float output = 0.0f;

    // 1. EFFECT STATE VALIDATION
    // Process only when effect engine is active or in transition.

    if ((effect_.active) || (effect_.genTransition.active)) {
        // 2. EFFECT TYPE RESOLUTION
        // Resolve runtime type using transition override when active.
        uint8_t type = (effect_.genTransition.active) ? effect_.genTransition.activeType : effect_.type;
        switch (type) {
            case EF_DELAY: {
                // 1. SETUP & CACHING
                Delay& delay_ = effect_.delay;
                float* delayBuf = (float*)effect_.delayAddress;

                uint32_t pIdx = delay_.playInterval;
                uint32_t rIdx = delay_.recordInterval;
                const float kInv100 = 0.01f;

                // 2. SIGNAL PROCESSING
                // Read the delayed signal from the buffer
                float delayedSample = delayBuf[pIdx];

                // --- DAMPING FILTER (Feedback Loop) ---
                // This one-pole filter softens the repeats.
                float feedbackTarget = (input * delay_.level) + (delayedSample * delay_.feedback);
                float filteredFeedback = delay_.lastFeedbackSample + delay_.dampingAlpha * (feedbackTarget - delay_.lastFeedbackSample);

                // Update filter state for next sample
                delay_.lastFeedbackSample = filteredFeedback;

                // Anti-denormal for stability
                if (fabsf(filteredFeedback) < 1.0e-18f) filteredFeedback = 0.0f;

                // Write the filtered feedback back to the buffer
                delayBuf[rIdx] = filteredFeedback;

                // 3. OUTPUT MIXING
                // mixData represents the "Wet" signal (the delay repeats).
                // Using eDry and fWet for final balance.
                float dryLevel = (float)delay_.eDry * kInv100;
                float wetLevel = (float)delay_.fWet * kInv100;

                // Wet is the delayed sound, Dry is the pure input.
                float mixData = (delayedSample * wetLevel) + (input * dryLevel);

                // 4. TRANSITION & STORE
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }

                // 5. UPDATE INDICES
                // Advance pointers and wrap around circular buffer
                if (++pIdx >= kDelaySize) pIdx = 0;
                if (++rIdx >= kDelaySize) rIdx = 0;

                delay_.playInterval = pIdx;
                delay_.recordInterval = rIdx;
            } break;

            case EF_CHORUS: {
                Chorus& chorus_ = effect_.chorus;
                float* chorusBuffer = (float*)effect_.chorusAddress;  // Access circular buffer

                const float bufSizeFloat = (float)kChorusBufferSize;
                const int32_t bufSizeInt = (int32_t)kChorusBufferSize;
                const float kInv100 = 0.01f;

                uint32_t recIdx = chorus_.recordInterval;
                float dataChorusAccum = 0.0f;

                // 1. DUAL VOICE PROCESSING LOOP
                // We process two independent delay taps modulated by triangle LFOs.
                for (uint8_t i = 0; i < 2; i++) {
                    ChorusDelay& cD = chorus_.chorusDelay[i];

                    // --- LFO Management ---
                    cD.shiftInterval += cD.shiftInc;
                    if (cD.shiftInterval <= cD.shiftMin) {
                        cD.shiftInc = fabsf(cD.shiftInc);
                        cD.shiftInterval = cD.shiftMin;
                    } else if (cD.shiftInterval >= cD.shiftMax) {
                        cD.shiftInc = -fabsf(cD.shiftInc);
                        cD.shiftInterval = cD.shiftMax;
                    }

                    // --- Fractional Delay Calculation ---
                    float readPos = (float)cD.playInterval + cD.shiftInterval;

                    // Circular Wrap
                    while (readPos < 0.0f) readPos += bufSizeFloat;
                    while (readPos >= bufSizeFloat) readPos -= bufSizeFloat;

                    // Linear Interpolation
                    int32_t indexA = (int32_t)readPos;
                    int32_t indexB = (indexA + 1) % bufSizeInt;
                    float frac = readPos - (float)indexA;

                    float sampleA = chorusBuffer[indexA];
                    float sampleB = chorusBuffer[indexB];
                    float interpolated = sampleA + ((sampleB - sampleA) * frac);

                    // Accumulate voice output
                    dataChorusAccum += interpolated * cD.mix;
                }

                // 2. FEEDBACK & BUFFER WRITE
                // Apply feedback and write to the circular buffer.
                float feedbackSignal = input + (dataChorusAccum * chorus_.feedback);

                // Anti-denormal for stability
                if (fabsf(feedbackSignal) < 1.0e-15f) feedbackSignal = 0.0f;

                if (effect_.genTransition.active) {
                    chorusBuffer[recIdx] = feedbackSignal * effect_.genTransition.activeRecordWet;
                } else {
                    chorusBuffer[recIdx] = feedbackSignal;
                }

                // 3. OUTPUT MIXING
                // Maps menu values (0-100) to linear dry/wet levels.
                float dryLevel = (float)chorus_.eDry * kInv100;
                float wetLevel = (float)chorus_.fWet * kInv100;

                // Mix the modulated signal (Wet) with the pure original input (Dry).
                float mixData = (dataChorusAccum * wetLevel) + (input * dryLevel);

                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }

                // 4. INDEX MANAGEMENT
                // Advance pointers for next sample
                recIdx = (recIdx + 1) % bufSizeInt;
                chorus_.recordInterval = recIdx;

                for (uint8_t i = 0; i < 2; i++) {
                    chorus_.chorusDelay[i].playInterval = (chorus_.chorusDelay[i].playInterval + 1) % bufSizeInt;
                }
            } break;

            case EF_FLANGER: {
                // 1. SETUP & CACHING
                Flanger& flanger_ = effect_.flanger;
                float* flangerBuf = effect_.flangerBuffer;  // Access external circular buffer

                const float bufSizeF = (float)kFlangerBufferSize;
                const int32_t bufSizeInt = (int32_t)kFlangerBufferSize;
                const float kInv100 = 0.01f;

                // Cache State Variables for local processing speed
                uint32_t pIdx = flanger_.playInterval;
                uint32_t rIdx = flanger_.recordInterval;
                float shiftInt = flanger_.shiftInterval;
                float shiftInc = flanger_.shiftInc;

                // 2. MODULATED READ POSITION (Fractional Delay)
                // Compute the modulated read head position based on triangle LFO.
                float readPos = (float)pIdx - shiftInt;  // Subtracting moves the read head "further" back

                // Circular Wrap for the fractional read position
                while (readPos < 0.0f) readPos += bufSizeF;
                while (readPos >= bufSizeF) readPos -= bufSizeF;

                // Calculate Interpolation Indices
                int32_t indexA = (int32_t)readPos;
                int32_t indexB = indexA + 1;
                if (indexB >= bufSizeInt) indexB = 0;

                // Fractional component for linear interpolation
                float frac = readPos - (float)indexA;

                // Perform Linear Interpolation between two buffer samples
                float sampleA = flangerBuf[indexA];
                float sampleB = flangerBuf[indexB];
                float dataFlanger = sampleA + ((sampleB - sampleA) * frac);

                // 3. FEEDBACK & WRITE
                // Apply feedback and store the sum in the buffer at the write head
                float feedbackSignal = dataFlanger * flanger_.feedback;
                float writeVal = input + feedbackSignal;

                // Handling transition wet/dry for recording
                if (effect_.genTransition.active) {
                    flangerBuf[rIdx] = writeVal * effect_.genTransition.activeRecordWet;
                } else {
                    flangerBuf[rIdx] = writeVal;
                }

                // 4. OUTPUT MIXING
                // Correct dry/wet mapping: input is pure dry, dataFlanger is pure wet.
                float dryLevel = (float)flanger_.eDry * kInv100;
                float wetLevel = (float)flanger_.fWet * kInv100;

                float mixData = (dataFlanger * wetLevel) + (input * dryLevel);

                // Apply global crossfade transition logic
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }

                // 5. UPDATE LFO & CIRCULAR INDICES
                // Advance the triangle LFO phase
                shiftInt += shiftInc;

                // Bounce the LFO when reaching min/max bounds (Triangle behavior)
                if (shiftInt <= flanger_.shiftMin) {
                    shiftInc = fabsf(shiftInc);  // Force positive increment
                    shiftInt = flanger_.shiftMin;
                } else if (shiftInt >= flanger_.shiftMax) {
                    shiftInc = -fabsf(shiftInc);  // Force negative increment
                    shiftInt = flanger_.shiftMax;
                }

                // Advance write and base read heads
                if (++pIdx >= bufSizeInt) pIdx = 0;
                if (++rIdx >= bufSizeInt) rIdx = 0;

                // Store updated state back to struct
                flanger_.playInterval = pIdx;
                flanger_.recordInterval = rIdx;
                flanger_.shiftInterval = shiftInt;
                flanger_.shiftInc = shiftInc;
            } break;

            case EF_PHASER: {
                // 1. SETUP & CACHING
                // Reference the phaser instance and cache essential values.
                Phaser& phaser_ = effect_.phaser;
                const float kInvSampleRate = 1.0f / AUDIO_SAMPLE_RATE;
                const float kInv100 = 0.01f;

                // 2. LFO & MODULATION
                // Advance LFO phase accumulator
                phaser_.dataX += phaser_.Ts;
                phaser_.dataY = phaser_.rate * phaser_.dataX;

                // Wrap LFO phase to stay within [0.0, 1.0]
                if (phaser_.dataY >= 1.0f) {
                    phaser_.dataX = 0.0f;
                    phaser_.dataY = 0.0f;
                }

                // Calculate Sine LFO and modulate the target frequency
                float lfoOsc = sinFast(phaser_.dataY);
                float lfoFreq = (phaser_.depthFreq * lfoOsc) + phaser_.centerFreq;

                // 3. FEEDBACK LOOP
                // Inject a portion of the previous output back into the input.
                // This creates the "peaks" and "valleys" characteristic of a resonant phaser.
                float feedbackSignal = phaser_.lastOutput * phaser_.feedback;
                float stage1Input = input + feedbackSignal;

                // 4. COEFFICIENT CALCULATION
                // Recalculate filter coefficients based on the current LFO frequency.
                float normFreq = lfoFreq * kInvSampleRate;
                float sinVal = sinFast(normFreq);
                float cosVal = cosFast(normFreq);

                // All-pass filter design: Standard Bi-quad coefficients
                float alpha = sinVal / (2.0f * phaser_.Q);
                float inv_a0 = 1.0f / (1.0f + alpha);
                float c_side = (1.0f - alpha) * inv_a0;
                float c_mid = (-2.0f * cosVal) * inv_a0;

                // 5. STAGE 1 PROCESSING (First All-Pass Filter)
                // Using the history buffers ff[0,1] and fb[0,1]
                float stage1Output = (c_side * stage1Input) + (c_mid * phaser_.ff[0]) + phaser_.ff[1] - (c_mid * phaser_.fb[0]) - (c_side * phaser_.fb[1]);

                // Update Stage 1 History
                phaser_.ff[1] = phaser_.ff[0];
                phaser_.ff[0] = stage1Input;
                phaser_.fb[1] = phaser_.fb[0];
                phaser_.fb[0] = stage1Output;

                // 6. STAGE 2 PROCESSING (Second All-Pass Filter)
                // Using history buffers ff[2,3] and fb[2,3].
                float stage2Output = (c_side * stage1Output) + (c_mid * phaser_.ff[2]) + phaser_.ff[3] - (c_mid * phaser_.fb[2]) - (c_side * phaser_.fb[3]);

                // Update Stage 2 History
                phaser_.ff[3] = phaser_.ff[2];
                phaser_.ff[2] = stage1Output;
                phaser_.fb[3] = phaser_.fb[2];
                phaser_.fb[2] = stage2Output;

                // Anti-denormal and State Storage for next iteration
                if (fabsf(stage2Output) < 1.0e-18f) stage2Output = 0.0f;  //
                phaser_.lastOutput = stage2Output;

                // 7. OUTPUT MIXING & TRANSITION
                // Mix Wet and Dry signals based on menu parameters (0-100 scale).
                float dryLevel = (float)phaser_.eDry * kInv100;
                float wetLevel = (float)phaser_.fWet * kInv100;
                float mixData = (stage2Output * wetLevel) + (input * dryLevel);

                // Apply global transition logic
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }
            } break;

            case EF_COMPRESSOR: {
                // 1. SETUP & DEFINITIONS
                // Resolve compressor parameters and local envelope variables.
                Compressor& comp_ = effect_.compressor;

                // Local Cache
                float threshold = comp_.threshold;
                float gainSmooth = comp_.gainSmoothPrev;
                float attackAlpha = comp_.attackAlpha;
                float releaseAlpha = comp_.releaseAlpha;

                // Signal Variables
                float absInput, in_dB;
                float gain_reduction = 0.0f;
                float attenuation = 1.0f;
                float mixData;

                // Constants
                const float kMinLogdB = -96.0f;
                const float kIdleThreshold = -0.001f;

                // 2. INPUT ANALYSIS (Level Detection)
                // Convert input magnitude to dB domain with floor protection.
                absInput = fabsf(input);

                // Avoid log(0)
                if (absInput < 1.0e-6f) absInput = 1.0e-6f;

                // Fast Linear to dB Conversion
                in_dB = fast_lin2db(absInput);
                if (in_dB < kMinLogdB) in_dB = kMinLogdB;

                // 3. GAIN CALCULATION (Optimized Math)
                // Compute gain reduction only when signal exceeds threshold.
                if (in_dB > threshold) {
                    float diff = in_dB - threshold;
                    float ratioInv = 1.0f / comp_.rate;
                    gain_reduction = diff * (ratioInv - 1.0f);
                }

                // else: gain_reduction remains 0.0f

                // 4. ENVELOPE (Ballistics)
                // Smooth gain changes using attack/release envelope behavior.
                if (gain_reduction >= kIdleThreshold && gainSmooth >= kIdleThreshold) {
                    gainSmooth = 0.0f;
                    attenuation = 1.0f;
                } else {
                    // Determine Alpha (Attack or Release)
                    float alpha = (gain_reduction < gainSmooth) ? attackAlpha : releaseAlpha;

                    // One-Pole Filter Application
                    gainSmooth = ((1.0f - alpha) * gain_reduction) + (alpha * gainSmooth);

                    // Convert dB back to Linear Gain
                    attenuation = fast_db2lin(gainSmooth);
                }

                // Write back state to struct
                comp_.gainSmoothPrev = gainSmooth;

                // 5. OUTPUT MIX & TRANSITION
                // Apply attenuation, mix wet/dry path, and handle transitions.
                mixData = (input * attenuation * effect_.wetFloat) + (input * effect_.dryFloat);
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }
            } break;

            case EF_EXPANDER: {
                // 1. SETUP & DEFINITIONS
                // Resolve expander parameters and local envelope variables.
                Expander& exp_ = effect_.expander;

                // Local Cache
                float threshold = exp_.threshold;
                float gainSmooth = exp_.gainSmoothPrev;
                float attackAlpha = exp_.attackAlpha;
                float releaseAlpha = exp_.releaseAlpha;
                float rate = exp_.rate;

                // Signal Variables
                float absInput, in_dB;
                float gain_change = 0.0f;
                float attenuation = 1.0f;
                float mixData;

                // Constants
                const float kMinLogdB = -144.0f;
                const float kIdleThreshold = -0.001f;

                // 2. INPUT ANALYSIS
                // Convert input magnitude to dB domain with floor protection.
                absInput = fabsf(input);

                // Prevent log(0)
                if (absInput < 1.0e-9f) absInput = 1.0e-9f;

                // Fast Linear to dB
                in_dB = fast_lin2db(absInput);
                if (in_dB < kMinLogdB) in_dB = kMinLogdB;

                // 3. GAIN CALCULATION (Optimized Math)
                // Compute attenuation when signal drops below threshold.
                if (in_dB < threshold) {
                    float diff = in_dB - threshold;
                    gain_change = diff * (rate - 1.0f);
                }

                // else: gain_change remains 0.0f (Unity Gain)

                // 4. ENVELOPE (Ballistics)
                // Smooth attenuation changes using attack/release envelope logic.
                if (gain_change >= kIdleThreshold && gainSmooth >= kIdleThreshold) {
                    gainSmooth = 0.0f;
                    attenuation = 1.0f;
                } else {
                    // Determine Alpha
                    float alpha;
                    if (gain_change > gainSmooth) {
                        alpha = attackAlpha;
                    } else {
                        alpha = releaseAlpha;
                    }

                    // One-Pole Filter
                    gainSmooth = ((1.0f - alpha) * gain_change) + (alpha * gainSmooth);

                    // Convert dB back to Linear
                    attenuation = fast_db2lin(gainSmooth);
                }

                // Write back state to struct
                exp_.gainSmoothPrev = gainSmooth;

                // 5. OUTPUT MIX & TRANSITION
                // Apply gain change, mix wet/dry path, and handle transitions.
                mixData = (input * attenuation * effect_.wetFloat) + (input * effect_.dryFloat);
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }
            } break;

            case EF_OVERDRIVE: {
                // 1. SETUP & CACHING
                // Reference the overdrive instance and cache values for performance.
                Overdrive& ov_ = effect_.overdrive;

                // Constants for cubic waveshaping and mixing
                const float kThird = 0.33333333f;      // 1/3
                const float kTwoThirds = 0.66666667f;  // 2/3
                const float kInv100 = 0.01f;           // 1/100 for percentage to linear conversion

                // Cache Parameters
                float gain = ov_.gain;
                float threshold = ov_.threshold;
                float alpha = ov_.preFilterAlpha;

                // 2. PRE-EMPHASIS (Drive Color Filter)
                // A One-Pole High-Pass filter applied BEFORE distortion.
                float hp_input = input;
                float hp_output = hp_input - ov_.hp_z1;
                ov_.hp_z1 = (hp_input * alpha) + (ov_.hp_z1 * (1.0f - alpha));

                // 3. CUBIC WAVESHAPING (Overdrive Core)
                // Apply gain and then the cubic soft-clipping curve.
                float driveSignal = hp_output * gain;
                float shapedSignal;

                // Calculate threshold limits based on the cubic algorithm
                float th_limit = kTwoThirds * threshold;

                if (driveSignal < -threshold) {
                    shapedSignal = -th_limit;
                } else if (driveSignal > threshold) {
                    shapedSignal = th_limit;
                } else {
                    // The core cubic function: x - (x^3 / 3)
                    shapedSignal = driveSignal - ((driveSignal * driveSignal * driveSignal) * kThird);
                }

                // 4. POST-FILTER (Tone Shaping)
                // Direct Form II Biquad Filter.
                float x0 = shapedSignal;
                float y0 = (ov_.a0 * x0) + (ov_.a1 * ov_.lp_x1) + (ov_.a2 * ov_.lp_x2) - (ov_.b1 * ov_.lp_y1) - (ov_.b2 * ov_.lp_y2);

                // Update Biquad Filter state history
                ov_.lp_x2 = ov_.lp_x1;
                ov_.lp_x1 = x0;
                ov_.lp_y2 = ov_.lp_y1;
                ov_.lp_y1 = y0;

                // 5. OUTPUT MIXING & TRANSITION
                float dryLevel = (float)ov_.eDry * kInv100;
                float wetLevel = (float)ov_.fWet * kInv100;

                // Combine processed (Wet) and original (Dry) signals
                float mixData = (y0 * wetLevel) + (input * dryLevel);

                // Apply global crossfade transition logic
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }
            } break;

            case EF_DISTORTION: {
                // 1. SETUP & CACHING
                // Reference the distortion instance and cache values for performance.
                Distortion& dist_ = effect_.distortion;
                const float kInv100 = 0.01f;

                // Parameters for waveshaping
                float gain = dist_.gain;
                float threshold = dist_.threshold;
                float bias = dist_.bias;

                // Filter Coefficients
                float a0 = dist_.a0, a1 = dist_.a1, a2 = dist_.a2;
                float b1 = dist_.b1, b2 = dist_.b2;

                // 2. ASYMMETRIC WAVESHAPING (The Distortion Algorithm)
                // First, we boost the input and apply the DC bias (Asymmetry).
                // Shifting the waveform before clipping creates even-order harmonics (warmth).
                float driveSignal = (input * gain) + bias;
                float shapedSignal;

                if (gain > 0.0001f) {
                    // ARCTAN SOFT CLIPPER:
                    // (2/PI) * atan(x) provides a smooth, musical transition into distortion.
                    // We multiply by threshold to set the final ceiling level.
                    shapedSignal = ((2.0f / M_PI) * atanFast(driveSignal)) * threshold;

                    // COMPENSATE DC OFFSET:
                    // Asymmetric clipping shifts the waveform's center.
                    // We subtract a portion of the bias to keep the signal centered around 0.
                    shapedSignal -= (bias * 0.5f);
                } else {
                    // Passthrough behavior if gain is nearly zero.
                    shapedSignal = input * threshold;
                }

                // 3. BIQUAD FILTER STAGE (Direct Form II)
                // Process the distorted signal through the Tone filter.
                // Using float states (x1, x2, y1, y2) for maximum precision.
                float x0 = shapedSignal;
                float y0 = (a0 * x0) + (a1 * dist_.x1) + (a2 * dist_.x2) - (b1 * dist_.y1) - (b2 * dist_.y2);

                // Update Filter State History
                dist_.x2 = dist_.x1;
                dist_.x1 = x0;
                dist_.y2 = dist_.y1;
                dist_.y1 = y0;

                // 4. OUTPUT MIXING
                // Maps menu values (0-100) to linear dry/wet levels.
                float dryLevel = (float)dist_.eDry * kInv100;
                float wetLevel = (float)dist_.fWet * kInv100;

                // Combine the processed (Wet) signal with the original (Dry) input.
                float mixData = (y0 * wetLevel) + (input * dryLevel);

                // 5. GLOBAL TRANSITION
                // Handle smooth crossfades between different effect presets or bypass states.
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }
            } break;

            case EF_BITCRUSHER: {
                // 1. REFERENCE & CACHING
                // Access the bitcrusher instance and cache variables for performance.
                Bitcrusher& bit_ = effect_.bitcrusher;

                const float kScale24Bit = 16777215.0f;        // 0xFFFFFF
                const float kInvScale24Bit = 5.96046448e-8f;  // 1.0 / 16777215.0
                const float kGateThresh = 0.002f;             // Silence gate threshold

                // Local cache for processing
                uint32_t counter = bit_.sampleCounter;
                uint32_t maxCount = bit_.sampleCounterMax;
                float currentSample = bit_.sampleData;
                float limPos = bit_.limitPos;
                float limNeg = bit_.limitNeg;
                uint32_t resMask = bit_.resModifier;

                // 2. DOWNSAMPLING & DISTORTION LOGIC
                // Increment local counter to handle Sample Rate Reduction.
                counter++;

                if (counter >= maxCount) {
                    counter = 0;  // Reset cadence counter
                    float processed = input;

                    // Silence Gate: Prevent processing low-level noise.
                    if (fabsf(processed) < kGateThresh) {
                        currentSample = 0.0f;
                    } else {
                        // --- DISTORTION MODE (dSmooth / Mode Selection) ---
                        if (bit_.mode == BIT_FOLD) {
                            // WAVEFOLDING: Reflects the signal back when it exceeds thresholds.
                            // Creates rich, complex harmonics compared to hard clipping.
                            while (processed > limPos || processed < limNeg) {
                                if (processed > limPos) {
                                    processed = limPos - (processed - limPos);
                                }
                                if (processed < limNeg) {
                                    processed = limNeg - (processed - limNeg);
                                }
                            }
                        } else {
                            // HARD CLIPPING: Simply clamps the signal at the thresholds.
                            if (processed > limPos)
                                processed = limPos;
                            else if (processed < limNeg)
                                processed = limNeg;
                        }

                        // --- BIT DEPTH REDUCTION (Quantization) ---
                        // Normalize [-1.0, 1.0] to [0.0, 1.0] for integer masking.
                        float normalized = (processed + 1.0f) * 0.5f;

                        // Scale to 24-bit integer space and apply the bitmask.
                        uint32_t scaled = (uint32_t)(normalized * kScale24Bit);
                        scaled &= resMask;

                        // Convert back to float and expand to audio range [-1.0, 1.0].
                        currentSample = ((float)scaled * kInvScale24Bit * 2.0f) - 1.0f;
                    }
                }

                // 3. SMOOTHING FILTER (One-Pole Low Pass)
                // Runs at full sample rate to smooth out the staircase edges from downsampling.
                // This reduces harsh digital "buzz" and makes the effect more musical.
                float filteredSample = bit_.lastOutput + bit_.smoothAlpha * (currentSample - bit_.lastOutput);
                bit_.lastOutput = filteredSample;  // Update filter state

                // 4. FINAL OUTPUT MIXING
                // Mix the processed (Wet) signal with the original (Dry) input.
                float mixData = (filteredSample * effect_.wetFloat) + (input * effect_.dryFloat);

                // Apply global transition blend if a preset/effect change is active.
                if (effect_.genTransition.active) {
                    output = (mixData * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
                } else {
                    output = mixData;
                }

                // 5. STATE UPDATE
                // Persist local values back to the struct for the next interrupt cycle.
                bit_.sampleCounter = counter;
                bit_.sampleData = currentSample;

            } break;

            default:
                output = input;
                break;
        }
    } else {
        // BYPASS OUTPUT
        // Return original input when effect path is inactive.
        output = input;
    }
    return output;
}

float Controller::processAudioLpfHelper(uint8_t lpfNum_, float audio_) {
    /// @brief Processes a single audio sample through a biquad low-pass filter (LPF) using Direct Form I calculation with history management and wet/dry mixing.

    // 1. LPF STATE RESOLUTION
    // Resolve LPF reference and bypass inactive path.
    Lpf& lpf_ = lpf[lpfNum_];
    if (!lpf_.active) return audio_;

    // 2. INPUT PREPARATION
    // Add anti-denormal noise for stable low-level processing.
    float input = audio_ + FILTER_ANTI_DENORMAL;

    // 3. BIQUAD CALCULATION
    // Compute LPF sample using Direct Form I coefficients.
    float filteredSignal = (lpf_.a0 * input) + (lpf_.a1 * lpf_.dataIn[1]) + (lpf_.a2 * lpf_.dataIn[2]) - (lpf_.b1 * lpf_.dataOut[1]) - (lpf_.b2 * lpf_.dataOut[2]);

    // 4. FILTER HISTORY UPDATE
    // Shift input and output delay-line history buffers.
    lpf_.dataIn[2] = lpf_.dataIn[1];
    lpf_.dataIn[1] = input;

    lpf_.dataOut[2] = lpf_.dataOut[1];
    lpf_.dataOut[1] = filteredSignal;

    // 5. WET/DRY MIX OUTPUT
    // Blend filtered and dry sample, then return final output.
    return (filteredSignal * lpf_.wet) + (audio_ * lpf_.dry);
}

