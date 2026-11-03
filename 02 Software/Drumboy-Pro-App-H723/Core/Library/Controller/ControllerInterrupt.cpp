// Timer/GPIO interrupt handlers.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Interrupt Functions -------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::interruptPlay() {
    /// @brief The central heartbeat of the sequencer. Manages metronome pre-counts, real-time beat triggering for all layers (simple and complex fills), hardware sync pulses, and LFO phase synchronization.

    if (playActive) {
        // 1. METRONOME PRE-COUNT PHASE
        // Handle the "get ready" timeline before normal song playback starts.

        if (metronome.precountState) {
            // Trigger Bar/Measure clicks based on pre-counter position
            if ((metronome.precounter % barInterval == 0) && (!io.sync.slaveMode)) {
                // Initialize Bar Click (Higher Pitch/Accent)
                metroPlayData.active = true;
                metroPlayData.sample = metronome.sample;
                metroPlayData.barMeasure = 0;
                metroPlayData.counter = 0;
                metroPlayData.counterMax = kMetroSize - 1;
                metroPlayData.volumeMultiplier = kLayerVolumeCoef * metronome.volumeFloat * mixer.masterVolumeFloat;
                metroPlayData.mixerInMode = metronome.mixerInMode;
                metronome.countDownFlag = true;
            } else if ((metronome.precounter % measureInterval == 0) && (!io.sync.slaveMode)) {
                // Initialize Measure Click (Standard Pitch)
                metroPlayData.active = true;
                metroPlayData.sample = metronome.sample;
                metroPlayData.barMeasure = 1;
                metroPlayData.counter = 0;
                metroPlayData.counterMax = kMetroSize - 1;
                metroPlayData.volumeMultiplier = kLayerVolumeCoef * metronome.volumeFloat * mixer.masterVolumeFloat;
                metroPlayData.mixerInMode = metronome.mixerInMode;
                metronome.countDownFlag = true;
            }

            // Increment pre-count or transition to active playback
            if (metronome.precounter < metronome.precounterMax) {
                metronome.precounter += 1;
            } else {
                metronome.precounter = 0;
                metronome.precountState = false;

                // Commit any hits captured during precount to wherever playback is
                // about to resume (playInterval — not necessarily 0, since the song
                // can start mid-loop), right now, so they land in bank.beatLibrary in
                // time for the very next tick's beat-trigger pass to play them in sync.
                for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                    if (precountPendingBeat[i]) {
                        precountPendingBeat[i] = false;
                        layerSong_setBeat(i, activeSongBank, playInterval, 0, 0, 0, 0, true, precountPendingHasNote[i], precountPendingNote[i]);
                        precountPendingHasNote[i] = false;
                    }
                }
            }
        }

        // 2. STANDARD PLAYBACK PHASE
        // Handle runtime metronome, sync pulses, beat triggering, and loop flow.

        else {
            if (io.midi.txStartPending && io.midi.txTransport) {
                static const uint8_t kMidiStart = 0xFA;
                enqueueMidiTx(&kMidiStart, 1);
                io.midi.txStartPending = false;
            }

            // Handle metronome clicks during active recording sessions
            if ((recordActive) && (metronome.active) && (playInterval != songInterval)) {
                if ((playInterval % barInterval == 0) && (!io.sync.slaveMode)) {
                    metroPlayData.active = true;
                    metroPlayData.sample = metronome.sample;
                    metroPlayData.barMeasure = 0;
                    metroPlayData.counter = 0;
                    metroPlayData.counterMax = kMetroSize - 1;
                    metroPlayData.volumeMultiplier = kLayerVolumeCoef * metronome.volumeFloat * mixer.masterVolumeFloat;
                } else if ((playInterval % measureInterval == 0) && (!io.sync.slaveMode)) {
                    metroPlayData.active = true;
                    metroPlayData.sample = metronome.sample;
                    metroPlayData.barMeasure = 1;
                    metroPlayData.counter = 0;
                    metroPlayData.counterMax = kMetroSize - 1;
                    metroPlayData.volumeMultiplier = kLayerVolumeCoef * metronome.volumeFloat * mixer.masterVolumeFloat;
                }
            }

            // Recorded held notes on TONE layers: fade out once their length has passed (a beat
            // starting on this tick sets a new count below).
            for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                if ((toneHoldLeft[i] > 0) && (--toneHoldLeft[i] == 0)) toneReleasePending[i] = true;
            }

            // 3. LAYER BEAT PROCESSING
            // Iterate through all layers and trigger simple/complex beat events.
            for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                Layer& layer = layerLibrary[i];
                Bank& bank = layer.bankLibrary[activeSongBank];
                int8_t playBeatNum = bank.playBeatNum;
                int8_t playBeatMicroNum = bank.playBeatMicroNum;
                float fillVolMultiplier = 0.0f;

                if ((bank.lastActiveBeatNum != -1) && (playBeatNum != -1)) {
                    Beat& beat = bank.beatLibrary[playBeatNum];

                    // SIMPLE BEAT HANDLING
                    // Trigger simple beats at the main beat interval.
                    if ((beat.getFillType() == 0) && (beat.getFillPattern() <= 2) && (beat.getFillTarget() == NULL_FILL_TARGET)) {
                        if (beat.getStartInterval() == playInterval) {
                            if ((!layer.mute) && layerSounds(layer.number)) {
                                switch (beat.getFillPattern()) {
                                    case 0:
                                        fillVolMultiplier = kNumberDataLibrary[100].powMultiplier;
                                        break;  // FLAT: 100% velocity

                                    case 1:
                                        fillVolMultiplier = kNumberDataLibrary[50].powMultiplier;
                                        break;  // GHOST: 50% velocity

                                    case 2:
                                        break;  // MUTED: 0% velocity
                                }
                            }
                            // Length until the next beat (the end interval of the last beat is the song end).
                            uint16_t length = (beat.getEndInterval() > beat.getStartInterval()) ? (beat.getEndInterval() - beat.getStartInterval()) : 0;
                            // SLIDE: this beat slides in, and/or the next beat (wrapping) slides in from this one.
                            const Beat& following = bank.beatLibrary[(playBeatNum < bank.lastActiveBeatNum) ? (playBeatNum + 1) : 0];
                            uint8_t beatFlags = beat.getSlide() ? kPlaySlideIn : 0;
                            if ((bank.lastActiveBeatNum > 0) && following.getSlide()) beatFlags |= kPlayTieOut;
                            // Recorded with PAD HOLD: sounds as long as the pad was held (a tie wins).
                            if ((beat.getHold() > 0) && !(beatFlags & kPlayTieOut)) {
                                length = beat.getHold();
                                beatFlags |= kPlayHeld;
                            }
                            // The beat being recorded right now with its pad still held is already
                            // sounding from the pad: triggering it again would cut it short.
                            const bool beingHeld = (holdRecLayer == static_cast<int8_t>(i)) && (holdRecBank == activeSongBank) &&
                                                   (holdRecStart == beat.getStartInterval());
                            if ((fillVolMultiplier > 0.0f) && !beingHeld && !layer_follows(i)) layer_playBeat(layer.number, fillVolMultiplier, beat.getNote(), length, beatFlags);  // v2.04: a harmony layer plays its leader's notes instead

                            // Advance to the next beat index
                            (playBeatNum < bank.lastActiveBeatNum) ? bank.playBeatNum += 1 : bank.playBeatNum = 0;
                            Beat& nextBeat = bank.beatLibrary[bank.playBeatNum];
                            ((nextBeat.getFillType() == 0) && (nextBeat.getFillPattern() <= 2)) ? bank.playBeatMicroNum = -1 : bank.playBeatMicroNum = 0;
                        }
                    }

                    // COMPLEX FILL HANDLING
                    // Trigger micro-step fill events using beat fill timing data.
                    else if ((beat.getFillTarget() != NULL_FILL_TARGET) && (playBeatMicroNum != -1)) {
                        BeatMicro& beatMicro = bank.beatFillLibrary[beat.getFillTarget()].beatMicroLibrary[playBeatMicroNum];
                        if (beatMicro.getInterval() == playInterval) {
                            if ((!layer.mute) && layerSounds(layer.number) && ((playBeatMicroNum == 0) || ((playBeatMicroNum != 0) && (layer.fill)))) {
                                fillVolMultiplier = kNumberDataLibrary[beatMicro.getLevel()].powMultiplier;
                                // A fill step lasts the beat's length divided by its number of steps.
                                const uint8_t steps = bank.beatFillLibrary[beat.getFillTarget()].getStepSize();
                                const uint16_t beatLength = (beat.getEndInterval() > beat.getStartInterval()) ? (beat.getEndInterval() - beat.getStartInterval()) : 0;
                                const uint16_t length = (steps > 0) ? static_cast<uint16_t>(beatLength / steps) : beatLength;
                                const uint8_t beatFlags = ((playBeatMicroNum == 0) && beat.getSlide()) ? kPlaySlideIn : 0;  // the fill's first hit slides in
                                if ((fillVolMultiplier > 0.0f) && !layer_follows(i)) layer_playBeat(layer.number, fillVolMultiplier, beat.getNote(), length, beatFlags);
                            }

                            // Advance micro-step index within the fill
                            if (playBeatMicroNum < (bank.beatFillLibrary[beat.getFillTarget()].getStepSize() - 1)) {
                                bank.playBeatMicroNum += 1;
                            } else {
                                (playBeatNum < bank.lastActiveBeatNum) ? bank.playBeatNum += 1 : bank.playBeatNum = 0;
                                Beat& nextBeat = bank.beatLibrary[bank.playBeatNum];
                                ((nextBeat.getFillType() == 0) && (nextBeat.getFillPattern() <= 2)) ? bank.playBeatMicroNum = -1 : bank.playBeatMicroNum = 0;
                            }
                        }
                    }
                }
            }

            // 4. SONG NAVIGATION & LOOP MANAGEMENT
            // Advance timeline, evaluate stop/reset triggers, and handle loop wrap.
            playInterval += 1;
            if (playInterval < songInterval) {
                // Handle pending Stop/Reset requests with visual feedback
                if (stopFlag) {
                    if ((io.syncOut == 2) && io.sync.syncOutPlay) {
                        constexpr uint16_t kSyncModeBStopGateLeadFrames = 2;
                        if ((stopInterval > playInterval) &&
                            ((stopInterval - playInterval) <= kSyncModeBStopGateLeadFrames)) {
                            HAL_GPIO_WritePin(SYNC_OUT_GATE_GPIO_Port, SYNC_OUT_GATE_Pin, GPIO_PIN_RESET);
                        }
                    }

                    if (stopInterval == playInterval)
                        stop();
                    else if (playInterval % (measureInterval / 2) == 0) {
                        stopIcon.flag = true;
                        stopIcon.mode = !stopIcon.mode;  // Toggle icon for blinking effect
                    }
                }
                if (resetFlag) {
                    if (resetInterval == playInterval)
                        reset();
                    else if (playInterval % (measureInterval / 2) == 0) {
                        resetIcon.flag = true;
                        resetIcon.mode = !resetIcon.mode;
                    }
                }
            } else {
                // SONG MODE: choose the part for the next loop (queued as a bank shift, applied
                // just below) or, in ONCE mode after the last step, stop and return to the top.
                // Runs before playInterval is cleared so reset() sees a running song.
                bool songStop = false;
                song_onLoopEnd(songStop);
                if (songStop) {
                    reset();
                    return;
                }

                // LOOP END PROCEDURES
                // Reset play interval and apply end-of-loop bank/play pointer updates.
                playInterval = 0;
                io.sync.txPulseCounter = 0;

                // Handle Bank Shifting at the end of the loop cycle
                if (bankShiftFlag) {
                    bankShiftFlag = false;
                    activeSongBank = targetSongBank;
                    bankActionFlag = true;
                }

                // Reset all layer playback pointers for the new loop
                for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                    Layer& layer = layerLibrary[i];
                    Bank& bank = layer.bankLibrary[activeSongBank];
                    if (bank.lastActiveBeatNum != -1) {
                        bank.playBeatNum = 0;
                        ((bank.beatLibrary[bank.playBeatNum].getFillType() == 0) && (bank.beatLibrary[bank.playBeatNum].getFillPattern() <= 2)) ? bank.playBeatMicroNum = -1 : bank.playBeatMicroNum = 0;
                    } else {
                        bank.playBeatNum = -1;
                        bank.playBeatMicroNum = -1;
                    }
                }
                lcd_restartPlay();
            }
        }
    }

    // 5. MICRO-TIMING & MODULATION UPDATES
    // Update temporary timing delays and LFO counters after transport operations.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        LayerPlayData& lD = layerPlayData[i];
        if (lD.tempBeatTimingFlag) {
            if (lD.tempBeatTimingCounter < lD.tempBeatTimingCounterMax) {
                lD.tempBeatTimingCounter += 1;
            } else {
                lD.tempBeatTimingFlag = false;
                uint32_t primaskTiming = __get_PRIMASK();
                __disable_irq();
                memcpy(&lD.activeBeatPlayData, &lD.tempBeatPlayData, sizeof(BeatPlayData));
                __set_PRIMASK(primaskTiming);
            }
        }
    }

    // LFO POSITION UPDATE
    // Increment and wrap LFO counters, then resynchronize phase at measure boundaries.
    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
        Lfo& lfo = lfoLibrary[i];
        lfo.sampleCounter += lfo.sampleInc;
        if (lfo.sampleCounter >= kLfoSampleSize) {
            lfo.sampleCounter -= kLfoSampleSize;
        }

        // Resynchronize LFO phase at every measure start to prevent drift
        if ((playActive) && (playInterval % measureInterval == 0)) {
            if ((lfo.sampleCounter <= 1.0f) || (lfo.sampleCounter > (kLfoSampleSize - 1))) lfo.sampleCounter = 0.0f;
        }
    }
}

void Controller::interruptSyncInGate() {
    /// @brief Handles SYNC-IN gate edges (MODE-B only) for external play/stop control.

    if (io.syncIn != 2) return;

    bool gateHigh = (HAL_GPIO_ReadPin(SYNC_IN_GATE_GPIO_Port, SYNC_IN_GATE_Pin) == GPIO_PIN_SET);

    if (gateHigh != io.sync.rxGateHigh) {
        io.sync.rxGateHigh = gateHigh;
        if (gateHigh) {
            io.sync.rxPlayFlag = true;
        } else {
            io.sync.rxStopFlag = true;
        }
    }
}

void Controller::interruptSyncInPulse() {
    /// @brief Handles an incoming sync pulse on SYNC_IN_Pin. Called from EXTI ISR.
    /// Measures interval between consecutive pulses to derive BPM.
    /// In MODE-A, pulses can start transport when play/record is armed.

    if (io.syncIn > 0) {
        LED1_TOGGLE;
    }

    // 1. MODE GUARD
    // Active for MODE-A/B; OFF mode exits immediately.
    if (io.syncIn == 0) return;

    // 2. HIGH-RES INTERVAL MEASUREMENT
    // Use microsecond timing to avoid 1 ms quantization jitter (e.g. 129<->130 BPM bouncing).
    static uint32_t rxPulseLastUs = 0;
    static uint32_t rxPulseFilteredUs = 0;

    uint32_t nowMs = HAL_GetTick();
    uint32_t nowUs = dwt_getMicros();

    // Reset local filter state when external logic resets pulse tracking.
    if (io.sync.rxPulseLastTick == 0) {
        rxPulseLastUs = 0;
        rxPulseFilteredUs = 0;
    }

    // 3. BPM DERIVATION
    // PPQN pulses per beat -> beat_us = pulse_us * ppqn -> BPM = 60,000,000 / beat_us.
    if (rxPulseLastUs > 0) {
        uint32_t intervalUs = nowUs - rxPulseLastUs;
        const uint8_t syncInPpqnIdx = (io.syncInPpqn <= kMaxIoSyncPpqn) ? io.syncInPpqn : kInitialIoSyncInPpqn;
        uint8_t ppqn = kIoSyncPpqnDataLibrary[syncInPpqnIdx].ppqn;

        uint32_t minBeatUs = 60000000UL / kMaxTempo;
        uint32_t maxBeatUs = 60000000UL / kMinTempo;
        uint32_t beatUsRaw = intervalUs * ppqn;

        if ((intervalUs > 0) && (beatUsRaw >= minBeatUs) && (beatUsRaw <= maxBeatUs)) {
            // 1st-order IIR low-pass: 75% previous + 25% new interval.
            if (rxPulseFilteredUs == 0) {
                rxPulseFilteredUs = intervalUs;
            } else {
                rxPulseFilteredUs = (rxPulseFilteredUs * 3UL + intervalUs) / 4UL;
            }

            uint32_t beatUsFiltered = rxPulseFilteredUs * ppqn;
            io.sync.rxPulseInterval = (rxPulseFilteredUs + 500UL) / 1000UL;  // keep timeout logic in ms domain
            io.sync.rxPulseBpm = (uint16_t)((60000000UL + beatUsFiltered / 2UL) / beatUsFiltered);
            io.sync.rxPulseFlag = true;
        }
    }

    // 4. ARMED START TRIGGER
    // MODE-A can start playback when armed by user action.
    // MODE-B start/stop is controlled by gate edges.
    if ((io.syncIn != 2) && (playArmed || recordArmed) && !playActive) io.sync.rxPlayFlag = true;

    // 5. TIMESTAMP UPDATE
    rxPulseLastUs = nowUs;
    io.sync.rxPulseLastTick = nowMs;
}

void Controller::interruptMidiTxClock() {
    /// @brief Transmits MIDI Clock (0xF8) on each timer tick. Called from TIM4 ISR at 24 PPQN.

    // 1. CLOCK TRANSMISSION
    // Enqueue real-time MIDI Clock byte when MIDI sync TX is active.
    if (io.midi.txClock) {
        static const uint8_t kMidiClock = 0xF8;
        enqueueMidiTx(&kMidiClock, 1);
    }

    // Sync-out clock policy:
    // MODE-A -> always running (no gate function)
    // MODE-B -> always running + gate play/stop
    // MODE-C -> running only while transport is active
    const bool syncClockEnabled = (io.syncOut == 1) || (io.syncOut == 2) || ((io.syncOut == 3) && playActive);

    if (syncClockEnabled && io.sync.syncOutBeat) {
        const uint8_t syncOutPpqnIdx = (io.syncOutPpqn <= kMaxIoSyncPpqn) ? io.syncOutPpqn : kInitialIoSyncOutPpqn;
        uint8_t ppqn = kIoSyncPpqnDataLibrary[syncOutPpqnIdx].ppqn;
        if (ppqn == 0) return;

        uint8_t ticksPerPulse = (ppqn >= 24) ? 1 : (24 / ppqn);
        if (ticksPerPulse == 0) ticksPerPulse = 1;

        io.sync.txTrsTickCounter += 1;
        if (io.sync.txTrsTickCounter >= ticksPerPulse) {
            io.sync.txTrsTickCounter = 0;
            HAL_GPIO_WritePin(SYNC_OUT_PULSE_GPIO_Port, SYNC_OUT_PULSE_Pin, GPIO_PIN_SET);
            if (io.syncOut > 0) {
                LED2_TOGGLE;
            }
            startBeatSyncTimer();
        }
    } else {
        io.sync.txTrsTickCounter = 0;
        HAL_GPIO_WritePin(SYNC_OUT_PULSE_GPIO_Port, SYNC_OUT_PULSE_Pin, GPIO_PIN_RESET);
    }
}

void Controller::interruptTransition() {
    /// @brief Global transition engine for smoothing parameter changes across the DSP pipeline.
    /// Manages linear ramps for volume, pan, and multi-phase crossfades for EQ, Filters, and Effects.

    static const float kChangeVal = 0.0002;

    // 1. SYSTEM VOLUME TRANSITION
    // Smoothly ramp master volume toward its target value.
    if (mixer.masterVolumeTransition.active) {
        MixerVolumeTransition& vTransition = mixer.masterVolumeTransition;
        switch (vTransition.actionVolume) {
            case MIXER_ACTION_UP:
                if (mixer.masterVolumeFloat >= vTransition.targetVolume) {
                    vTransition.actionVolume = MIXER_ACTION_NONE;
                } else {
                    mixer.masterVolumeFloat += kChangeVal;
                }
                break;

            case MIXER_ACTION_DOWN:
                if (mixer.masterVolumeFloat <= vTransition.targetVolume) {
                    vTransition.actionVolume = MIXER_ACTION_NONE;
                } else {
                    mixer.masterVolumeFloat -= kChangeVal;
                }
                break;
        }
        if (vTransition.actionVolume == MIXER_ACTION_NONE) {
            vTransition.active = false;
        }
    }

    // 2. SYSTEM PAN TRANSITION
    // Smoothly ramp left/right channel gains toward target pan values.
    if (mixer.panTransition.active) {
        MixerPanTransition& pTransition = mixer.panTransition;
        switch (pTransition.actionVolumeLeft) {
            case MIXER_ACTION_UP:
                if (mixer.volumeLeftFloat >= pTransition.targetVolumeLeft) {
                    pTransition.actionVolumeLeft = MIXER_ACTION_NONE;
                } else {
                    mixer.volumeLeftFloat += kChangeVal;
                }
                break;

            case MIXER_ACTION_DOWN:
                if (mixer.volumeLeftFloat <= pTransition.targetVolumeLeft) {
                    pTransition.actionVolumeLeft = MIXER_ACTION_NONE;
                } else {
                    mixer.volumeLeftFloat -= kChangeVal;
                }
                break;
        }
        switch (pTransition.actionVolumeRight) {
            case MIXER_ACTION_UP:
                if (mixer.volumeRightFloat >= pTransition.targetVolumeRight) {
                    pTransition.actionVolumeRight = MIXER_ACTION_NONE;
                } else {
                    mixer.volumeRightFloat += kChangeVal;
                }
                break;

            case MIXER_ACTION_DOWN:
                if (mixer.volumeRightFloat <= pTransition.targetVolumeRight) {
                    pTransition.actionVolumeRight = MIXER_ACTION_NONE;
                } else {
                    mixer.volumeRightFloat -= kChangeVal;
                }
                break;
        }
        if ((pTransition.actionVolumeLeft == MIXER_ACTION_NONE) && (pTransition.actionVolumeRight == MIXER_ACTION_NONE)) {
            pTransition.active = false;
        }
    }

    // 3. METRONOME VOLUME TRANSITION
    // Smoothly ramp metronome level toward its target value.
    if (metronome.volumeTransition.active) {
        MetronomeVolumeTransition& vTransition = metronome.volumeTransition;
        switch (vTransition.actionVolume) {
            case MET_ACTION_UP:
                if (metronome.volumeFloat >= vTransition.targetVolume) {
                    vTransition.actionVolume = MET_ACTION_NONE;
                } else {
                    metronome.volumeFloat += kChangeVal;
                }
                break;

            case MET_ACTION_DOWN:
                if (metronome.volumeFloat <= vTransition.targetVolume) {
                    vTransition.actionVolume = MET_ACTION_NONE;
                } else {
                    metronome.volumeFloat -= kChangeVal;
                }
                break;
        }
        if (vTransition.actionVolume == MET_ACTION_NONE) {
            vTransition.active = false;
        }
    }

    // 4. EQ CROSSFADE TRANSITION
    // Process EQ wet/dry crossfade and finalize mode transitions.
    if (eq.genTransition.active) {
        EqGenTransition& gTransition = eq.genTransition;
        switch (gTransition.actionDry) {
            case EQ_ACTION_UP:
                if (gTransition.activeDry >= gTransition.targetDry) {
                    gTransition.actionDry = EQ_ACTION_NONE;
                } else {
                    gTransition.activeDry += kChangeVal;
                }
                break;

            case EQ_ACTION_DOWN:
                if (gTransition.activeDry <= gTransition.targetDry) {
                    gTransition.actionDry = EQ_ACTION_NONE;
                } else {
                    gTransition.activeDry -= kChangeVal;
                }
                break;
        }
        switch (gTransition.actionWet) {
            case EQ_ACTION_UP:
                if (gTransition.activeWet >= gTransition.targetWet) {
                    gTransition.actionWet = EQ_ACTION_NONE;
                } else {
                    gTransition.activeWet += kChangeVal;
                }
                break;

            case EQ_ACTION_DOWN:
                if (gTransition.activeWet <= gTransition.targetWet) {
                    gTransition.actionWet = EQ_ACTION_NONE;
                } else {
                    gTransition.activeWet -= kChangeVal;
                }
                break;
        }
        if ((gTransition.actionDry == EQ_ACTION_NONE) && (gTransition.actionWet == EQ_ACTION_NONE)) {
            switch (gTransition.mode) {
                case EQ_MODE_NONE:
                    gTransition.active = false;
                    gTransition.mode = EQ_MODE_NONE;
                    gTransition.phase = EQ_PHASE_NONE;
                    transitionClearFlag = true;
                    break;

                case EQ_MODE_ACTIVE:
                    gTransition.active = false;
                    gTransition.mode = EQ_MODE_NONE;
                    gTransition.phase = EQ_PHASE_NONE;
                    transitionClearFlag = true;
                    checkMenuTransition(EQ_MENU, false);
                    checkEncoderTransition(ENC_EQ_ACTIVE, false);
                    break;
            }
        }
    }

    // 5. FILTER TRANSITIONS
    // Process filter generation/mix transitions for all filter engines.
    for (uint8_t i = 0; i < kFilterLibrarySize; i++) {
        if (filter[i].genTransition.active) {
            FilterGenTransition& gTransition = filter[i].genTransition;
            switch (gTransition.actionDry) {
                case FIL_ACTION_UP:
                    if (gTransition.activeDry >= gTransition.targetDry) {
                        gTransition.actionDry = FIL_ACTION_NONE;
                    } else {
                        gTransition.activeDry += kChangeVal;
                    }
                    break;

                case FIL_ACTION_DOWN:
                    if (gTransition.activeDry <= gTransition.targetDry) {
                        gTransition.actionDry = FIL_ACTION_NONE;
                    } else {
                        gTransition.activeDry -= kChangeVal;
                    }
                    break;
            }
            switch (gTransition.actionWet) {
                case FIL_ACTION_UP:
                    if (gTransition.activeWet >= gTransition.targetWet) {
                        gTransition.actionWet = FIL_ACTION_NONE;
                    } else {
                        gTransition.activeWet += kChangeVal;
                    }
                    break;

                case FIL_ACTION_DOWN:
                    if (gTransition.activeWet <= gTransition.targetWet) {
                        gTransition.actionWet = FIL_ACTION_NONE;
                    } else {
                        gTransition.activeWet -= kChangeVal;
                    }
                    break;
            }
            if ((gTransition.actionDry == FIL_ACTION_NONE) && (gTransition.actionWet == FIL_ACTION_NONE)) {
                switch (gTransition.mode) {
                    case FIL_MODE_NONE:
                        gTransition.active = false;
                        gTransition.mode = FIL_MODE_NONE;
                        gTransition.phase = FIL_PHASE_NONE;
                        transitionClearFlag = true;
                        break;

                    case FIL_MODE_ACTIVE:
                        gTransition.active = false;
                        gTransition.mode = FIL_MODE_NONE;
                        gTransition.phase = FIL_PHASE_NONE;
                        transitionClearFlag = true;
                        (i == 0) ? checkMenuTransition(FILTER_0_MENU, false) : checkMenuTransition(FILTER_1_MENU, false);
                        (i == 0) ? checkEncoderTransition(ENC_FILTER_0_ACTIVE, false) : checkEncoderTransition(ENC_FILTER_1_ACTIVE, false);
                        break;

                    case FIL_MODE_TYPE:
                        switch (gTransition.phase) {
                            case EF_PHASE_NONE:
                                gTransition.active = false;
                                gTransition.mode = FIL_MODE_NONE;
                                gTransition.phase = FIL_PHASE_NONE;
                                transitionClearFlag = true;
                                break;

                            case EF_PHASE_A:
                                filter[i].calculateCoef();
                                gTransition.phase = FIL_PHASE_B;
                                gTransition.activeDry = 1.0;
                                gTransition.targetDry = 0.0;
                                gTransition.activeWet = 0.0;
                                gTransition.targetWet = 1.0;
                                filter_calculateGenTransition(i);
                                transitionShowFlag = 2;
                                (i == 0) ? checkMenuTransition(FILTER_0_MENU, true) : checkMenuTransition(FILTER_1_MENU, true);
                                (i == 0) ? checkEncoderTransition(ENC_FILTER_0_TYPE, true) : checkEncoderTransition(ENC_FILTER_1_TYPE, true);
                                break;

                            case EF_PHASE_B:
                                gTransition.active = false;
                                gTransition.mode = FIL_MODE_NONE;
                                gTransition.phase = FIL_PHASE_NONE;
                                transitionClearFlag = true;
                                (i == 0) ? checkMenuTransition(FILTER_0_MENU, false) : checkMenuTransition(FILTER_1_MENU, false);
                                (i == 0) ? checkEncoderTransition(ENC_FILTER_0_TYPE, false) : checkEncoderTransition(ENC_FILTER_1_TYPE, false);
                                break;
                        }
                        break;
                }
            }
        }
        if (filter[i].mixTransition.active) {
            Filter& filter_ = filter[i];
            FilterMixTransition& mTransition = filter[i].mixTransition;
            switch (mTransition.actionDry) {
                case FIL_ACTION_UP:
                    if (filter_.dryFloat >= mTransition.targetDry) {
                        mTransition.actionDry = FIL_ACTION_NONE;
                    } else {
                        filter_.dryFloat += kChangeVal;
                    }
                    break;

                case FIL_ACTION_DOWN:
                    if (filter_.dryFloat <= mTransition.targetDry) {
                        mTransition.actionDry = FIL_ACTION_NONE;
                    } else {
                        filter_.dryFloat -= kChangeVal;
                    }
                    break;
            }
            switch (mTransition.actionWet) {
                case FIL_ACTION_UP:
                    if (filter_.wetFloat >= mTransition.targetWet) {
                        mTransition.actionWet = FIL_ACTION_NONE;
                    } else {
                        filter_.wetFloat += kChangeVal;
                    }
                    break;

                case FIL_ACTION_DOWN:
                    if (filter_.wetFloat <= mTransition.targetWet) {
                        mTransition.actionWet = FIL_ACTION_NONE;
                    } else {
                        filter_.wetFloat -= kChangeVal;
                    }
                    break;
            }
            if ((mTransition.actionDry == FIL_ACTION_NONE) && (mTransition.actionWet == FIL_ACTION_NONE)) {
                mTransition.active = false;
            }
        }
    }

    // 6. EFFECT TRANSITIONS
    // Process effect generation/mix transitions for all effect engines.
    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
        if (effect[i].genTransition.active) {
            EffectGenTransition& gTransition = effect[i].genTransition;
            switch (gTransition.actionDry) {
                case EF_ACTION_UP:
                    if (gTransition.activeDry >= gTransition.targetDry) {
                        gTransition.actionDry = EF_ACTION_NONE;
                    } else {
                        gTransition.activeDry += kChangeVal;
                    }
                    break;

                case EF_ACTION_DOWN:
                    if (gTransition.activeDry <= gTransition.targetDry) {
                        gTransition.actionDry = EF_ACTION_NONE;
                    } else {
                        gTransition.activeDry -= kChangeVal;
                    }
                    break;
            }
            switch (gTransition.actionWet) {
                case EF_ACTION_UP:
                    if (gTransition.activeWet >= gTransition.targetWet) {
                        gTransition.actionWet = EF_ACTION_NONE;
                    } else {
                        gTransition.activeWet += kChangeVal;
                    }
                    break;

                case EF_ACTION_DOWN:
                    if (gTransition.activeWet <= gTransition.targetWet) {
                        gTransition.actionWet = EF_ACTION_NONE;
                    } else {
                        gTransition.activeWet -= kChangeVal;
                    }
                    break;
            }
            switch (gTransition.actionRecordWet) {
                case EF_ACTION_UP:
                    if (gTransition.activeRecordWet >= gTransition.targetRecordWet) {
                        gTransition.actionRecordWet = EF_ACTION_NONE;
                    } else {
                        gTransition.activeRecordWet += kChangeVal;
                    }
                    break;

                case EF_ACTION_DOWN:
                    if (gTransition.activeRecordWet <= gTransition.targetRecordWet) {
                        gTransition.actionRecordWet = EF_ACTION_NONE;
                    } else {
                        gTransition.activeRecordWet -= kChangeVal;
                    }
                    break;
            }
            if ((gTransition.actionDry == EF_ACTION_NONE) && (gTransition.actionWet == EF_ACTION_NONE) && (gTransition.actionRecordWet == EF_ACTION_NONE)) {
                Menu targetMenu;
                EncoderFunc targetFunc;
                switch (gTransition.mode) {
                    case EF_MODE_NONE:
                        gTransition.active = false;
                        gTransition.mode = EF_MODE_NONE;
                        gTransition.phase = EF_PHASE_NONE;
                        transitionClearFlag = true;
                        break;

                    case EF_MODE_ACTIVE:
                        gTransition.active = false;
                        gTransition.mode = EF_MODE_NONE;
                        gTransition.phase = EF_PHASE_NONE;
                        transitionClearFlag = true;
                        (i == 0) ? checkMenuTransition(EFFECT_0_MENU, false) : checkMenuTransition(EFFECT_1_MENU, false);
                        (i == 0) ? checkEncoderTransition(ENC_EFFECT_0_ACTIVE, false) : checkEncoderTransition(ENC_EFFECT_1_ACTIVE, false);
                        break;

                    case EF_MODE_TYPE:

                    case EF_MODE_TIME:

                    case EF_MODE_FEEDBACK:
                        switch (gTransition.phase) {
                            case EF_PHASE_NONE:
                                gTransition.active = false;
                                gTransition.mode = EF_MODE_NONE;
                                gTransition.phase = EF_PHASE_NONE;
                                transitionClearFlag = true;
                                break;

                            case EF_PHASE_A:
                                switch (gTransition.mode) {
                                    case EF_MODE_TYPE:
                                        gTransition.activeType = gTransition.targetType;
                                        effect_updateMixFloats(i);
                                        break;

                                    case EF_MODE_TIME:
                                        switch (effect[i].type) {
                                            case EF_DELAY:
                                                effect[i].delay.time = kDelayTimeDataLibrary[effect[i].delay.aTime].data;
                                                effect[i].delay.update(rhythm.tempo);
                                                break;

                                            case EF_CHORUS:
                                                effect[i].chorus.time = kChorusTimeDataLibrary[effect[i].chorus.aTime].data;
                                                effect[i].chorus.update();
                                                break;

                                            case EF_FLANGER:
                                                effect[i].flanger.time = kFlangerTimeDataLibrary[effect[i].flanger.aTime].data;
                                                effect[i].flanger.update();
                                                break;
                                        }
                                        break;

                                    case EF_MODE_FEEDBACK:
                                        switch (effect[i].type) {
                                            case EF_CHORUS:
                                                effect[i].chorus.feedback = kChorusFeedbackDataLibrary[effect[i].chorus.bFeedback].data;
                                                effect[i].chorus.update();
                                                break;

                                            case EF_FLANGER:
                                                effect[i].flanger.feedback = kFlangerFeedbackDataLibrary[effect[i].flanger.bFeedback].data;
                                                effect[i].flanger.update();
                                                break;
                                        }
                                        break;
                                }
                                gTransition.phase = EF_PHASE_B;
                                gTransition.activeDry = 1.0;
                                gTransition.targetDry = 0.0;
                                gTransition.activeWet = 0.0;
                                gTransition.targetWet = 1.0;
                                gTransition.activeRecordWet = 0.0;
                                gTransition.targetRecordWet = 1.0;
                                effect_calculateGenTransition(i);
                                transitionShowFlag = 2;
                                if (i == 0) {
                                    targetMenu = EFFECT_0_MENU;
                                    switch (gTransition.mode) {
                                        case EF_MODE_TYPE:
                                            targetFunc = ENC_EFFECT_0_TYPE;
                                            break;

                                        case EF_MODE_TIME:
                                            targetFunc = ENC_EFFECT_0_PARAM_A;
                                            break;

                                        case EF_MODE_FEEDBACK:
                                            targetFunc = ENC_EFFECT_0_PARAM_B;
                                            break;
                                    }
                                } else {
                                    targetMenu = EFFECT_1_MENU;
                                    switch (gTransition.mode) {
                                        case EF_MODE_TYPE:
                                            targetFunc = ENC_EFFECT_1_TYPE;
                                            break;

                                        case EF_MODE_TIME:
                                            targetFunc = ENC_EFFECT_1_PARAM_A;
                                            break;

                                        case EF_MODE_FEEDBACK:
                                            targetFunc = ENC_EFFECT_1_PARAM_B;
                                            break;
                                    }
                                }
                                checkMenuTransition(targetMenu, true);
                                checkEncoderTransition(targetFunc, true);
                                break;

                            case EF_PHASE_B:
                                if (i == 0) {
                                    targetMenu = EFFECT_0_MENU;
                                    switch (gTransition.mode) {
                                        case EF_MODE_TYPE:
                                            targetFunc = ENC_EFFECT_0_TYPE;
                                            break;

                                        case EF_MODE_TIME:
                                            targetFunc = ENC_EFFECT_0_PARAM_A;
                                            break;

                                        case EF_MODE_FEEDBACK:
                                            targetFunc = ENC_EFFECT_0_PARAM_B;
                                            break;
                                    }
                                } else {
                                    targetMenu = EFFECT_1_MENU;
                                    switch (gTransition.mode) {
                                        case EF_MODE_TYPE:
                                            targetFunc = ENC_EFFECT_1_TYPE;
                                            break;

                                        case EF_MODE_TIME:
                                            targetFunc = ENC_EFFECT_1_PARAM_A;
                                            break;

                                        case EF_MODE_FEEDBACK:
                                            targetFunc = ENC_EFFECT_1_PARAM_B;
                                            break;
                                    }
                                }
                                gTransition.active = false;
                                gTransition.mode = EF_MODE_NONE;
                                gTransition.phase = EF_PHASE_NONE;
                                transitionClearFlag = true;
                                checkMenuTransition(targetMenu, false);
                                checkEncoderTransition(targetFunc, false);
                                break;
                        }
                        break;
                }
            }
        }
        if (effect[i].mixTransition.active) {
            Effect& effect_ = effect[i];
            EffectMixTransition& mTransition = effect[i].mixTransition;
            switch (mTransition.actionDry) {
                case EF_ACTION_UP:
                    if (effect_.dryFloat >= mTransition.targetDry) {
                        mTransition.actionDry = EF_ACTION_NONE;
                    } else {
                        effect_.dryFloat += kChangeVal;
                    }
                    break;

                case EF_ACTION_DOWN:
                    if (effect_.dryFloat <= mTransition.targetDry) {
                        mTransition.actionDry = EF_ACTION_NONE;
                    } else {
                        effect_.dryFloat -= kChangeVal;
                    }
                    break;
            }
            switch (mTransition.actionWet) {
                case EF_ACTION_UP:
                    if (effect_.wetFloat >= mTransition.targetWet) {
                        mTransition.actionWet = EF_ACTION_NONE;
                    } else {
                        effect_.wetFloat += kChangeVal;
                    }
                    break;

                case EF_ACTION_DOWN:
                    if (effect_.wetFloat <= mTransition.targetWet) {
                        mTransition.actionWet = EF_ACTION_NONE;
                    } else {
                        effect_.wetFloat -= kChangeVal;
                    }
                    break;
            }
            if ((mTransition.actionDry == EF_ACTION_NONE) && (mTransition.actionWet == EF_ACTION_NONE)) {
                mTransition.active = false;
            }
        }
    }

    // 7. REVERB TRANSITIONS
    // Process reverb generation/mix transitions and mode phase updates.
    if (reverb.genTransition.active) {
        ReverbGenTransition& gTransition = reverb.genTransition;
        switch (gTransition.actionDry) {
            case REV_ACTION_UP:
                if (gTransition.activeDry >= gTransition.targetDry) {
                    gTransition.actionDry = REV_ACTION_NONE;
                } else {
                    gTransition.activeDry += kChangeVal;
                }
                break;

            case REV_ACTION_DOWN:
                if (gTransition.activeDry <= gTransition.targetDry) {
                    gTransition.actionDry = REV_ACTION_NONE;
                } else {
                    gTransition.activeDry -= kChangeVal;
                }
                break;
        }
        switch (gTransition.actionWet) {
            case REV_ACTION_UP:
                if (gTransition.activeWet >= gTransition.targetWet) {
                    gTransition.actionWet = REV_ACTION_NONE;
                } else {
                    gTransition.activeWet += kChangeVal;
                }
                break;

            case REV_ACTION_DOWN:
                if (gTransition.activeWet <= gTransition.targetWet) {
                    gTransition.actionWet = REV_ACTION_NONE;
                } else {
                    gTransition.activeWet -= kChangeVal;
                }
                break;
        }
        if ((gTransition.actionDry == REV_ACTION_NONE) && (gTransition.actionWet == REV_ACTION_NONE)) {
            switch (gTransition.mode) {
                case REV_MODE_NONE:
                    gTransition.active = false;
                    gTransition.mode = REV_MODE_NONE;
                    gTransition.phase = REV_PHASE_NONE;
                    transitionClearFlag = true;
                    break;

                case REV_MODE_ACTIVE:
                    gTransition.active = false;
                    gTransition.mode = REV_MODE_NONE;
                    gTransition.phase = REV_PHASE_NONE;
                    transitionClearFlag = true;
                    checkMenuTransition(REVERB_MENU, false);
                    checkEncoderTransition(ENC_REVERB_ACTIVE, false);
                    break;

                case REV_MODE_PREDELAY:
                    switch (gTransition.phase) {
                        case REV_PHASE_NONE:
                            gTransition.active = false;
                            gTransition.mode = REV_MODE_NONE;
                            gTransition.phase = REV_PHASE_NONE;
                            transitionClearFlag = true;
                            break;

                        case REV_PHASE_A:
                            reverb.setPreDelay(kReverbPreDelayDataLibrary[reverb.preDelay].data);
                            gTransition.phase = REV_PHASE_B;
                            gTransition.activeDry = 1.0;
                            gTransition.targetDry = 0.0;
                            gTransition.activeWet = 0.0;
                            gTransition.targetWet = 1.0;
                            reverb_calculateGenTransition();
                            transitionShowFlag = 2;
                            checkMenuTransition(REVERB_MENU, true);
                            checkEncoderTransition(ENC_REVERB_PREDELAY, true);
                            break;

                        case REV_PHASE_B:
                            gTransition.active = false;
                            gTransition.mode = REV_MODE_NONE;
                            gTransition.phase = REV_PHASE_NONE;
                            transitionClearFlag = true;
                            checkMenuTransition(REVERB_MENU, false);
                            checkEncoderTransition(ENC_REVERB_PREDELAY, false);
                            break;
                    }
                    break;

                case REV_MODE_SURROUND:
                    switch (gTransition.phase) {
                        case REV_PHASE_NONE:
                            gTransition.active = false;
                            gTransition.mode = REV_MODE_NONE;
                            gTransition.phase = REV_PHASE_NONE;
                            transitionClearFlag = true;
                            break;

                        case REV_PHASE_A:
                            reverb.setSurround(kReverbSurroundDataLibrary[reverb.surround].data);
                            gTransition.phase = REV_PHASE_B;
                            gTransition.activeDry = 1.0;
                            gTransition.targetDry = 0.0;
                            gTransition.activeWet = 0.0;
                            gTransition.targetWet = 1.0;
                            reverb_calculateGenTransition();
                            transitionShowFlag = 2;
                            checkMenuTransition(REVERB_MENU, true);
                            checkEncoderTransition(ENC_REVERB_SURROUND, true);
                            break;

                        case REV_PHASE_B:
                            gTransition.active = false;
                            gTransition.mode = REV_MODE_NONE;
                            gTransition.phase = REV_PHASE_NONE;
                            transitionClearFlag = true;
                            checkMenuTransition(REVERB_MENU, false);
                            checkEncoderTransition(ENC_REVERB_SURROUND, false);
                            break;
                    }
                    break;
            }
        }
    }
    if (reverb.mixTransition.active) {
        ReverbMixTransition& mTransition = reverb.mixTransition;
        switch (mTransition.actionDry) {
            case REV_ACTION_UP:
                if (reverb.dryFloat >= mTransition.targetDry) {
                    mTransition.actionDry = REV_ACTION_NONE;
                } else {
                    reverb.dryFloat += kChangeVal;
                }
                break;

            case REV_ACTION_DOWN:
                if (reverb.dryFloat <= mTransition.targetDry) {
                    mTransition.actionDry = REV_ACTION_NONE;
                } else {
                    reverb.dryFloat -= kChangeVal;
                }
                break;
        }
        switch (mTransition.actionWet) {
            case REV_ACTION_UP:
                if (reverb.wetFloat >= mTransition.targetWet) {
                    mTransition.actionWet = REV_ACTION_NONE;
                } else {
                    reverb.wetFloat += kChangeVal;
                }
                break;

            case REV_ACTION_DOWN:
                if (reverb.wetFloat <= mTransition.targetWet) {
                    mTransition.actionWet = REV_ACTION_NONE;
                } else {
                    reverb.wetFloat -= kChangeVal;
                }
                break;
        }
        if ((mTransition.actionDry == REV_ACTION_NONE) && (mTransition.actionWet == REV_ACTION_NONE)) {
            mTransition.active = false;
        }
    }
}

void Controller::interruptSystemPreset() {
    /// @brief Finalizes the system-wide preset adjustment window by stopping the timeout timer and signaling a persistent storage write operation if changes were made.

    // 1. SYSTEM PRESET TIMER STOP
    // Stop the inactivity timer used during system preset edits.
    stopSystemPresetTimer();

    // 2. DEFERRED SAVE SIGNAL
    // Request a deferred NVM save when preset data has pending changes.
    if (systemPresetSaveActive) {
        systemPresetSaveFlag = true;
    }
}

void Controller::interruptEncoderPreset() {
    /// @brief Finalizes the encoder preset adjustment window by halting the preset timer and signaling a deferred save operation if the save state is active.

    // 1. ENCODER PRESET TIMER STOP
    // Stop the inactivity timer used during encoder preset edits.
    stopEncoderPresetTimer();

    // 2. DEFERRED SAVE SIGNAL
    // Request a deferred save when encoder preset data is marked dirty.
    if (encoderPresetSaveActive)
        encoderPresetSaveFlag = true;
}

void Controller::interruptLayerKeypadRead() {
    /// @brief Signals a hardware interrupt event from the layer keypad by setting a read request flag for the main processing loop.

    // 1. LAYER KEYPAD READ FLAG
    // Set read flag so keypad scanning runs in non-interrupt context.
    keyboard.keyLayer.readFlag = true;
}

void Controller::interruptLeftKeypadRead() {
    /// @brief Signals a hardware interrupt event from the left keypad by setting a read request flag for the main processing loop.

    // 1. LEFT KEYPAD READ FLAG
    // Set read flag so keypad scanning runs in non-interrupt context.
    keyboard.keyLeft.readFlag = true;
}

void Controller::interruptRightKeypadRead() {
    /// @brief Signals a hardware interrupt event from the right keypad by setting a read request flag for the main processing loop.

    // 1. RIGHT KEYPAD READ FLAG
    // Set read flag so keypad scanning runs in non-interrupt context.
    keyboard.keyRight.readFlag = true;
}

void Controller::interruptEncoderRead(uint8_t encNum_) {
    /// @brief Sets a read request flag for a specific rotary encoder to signal the main processing loop that a hardware interrupt has occurred.

    Encoder& enc = encoder[encNum_];

    // 1. CAPTURE AB
    // Read Phase A and B pins immediately
    uint8_t a = (enc.aPort->IDR & enc.aPin) ? 2 : 0;
    uint8_t b = (enc.bPort->IDR & enc.bPin) ? 1 : 0;
    enc.capturedAB = (a | b);  // Store the snapshot

    // 2. ENCODER READ FLAG
    // Set selected encoder read flag for deferred processing.
    enc.readFlag = true;
}

void Controller::interruptFuncButtonRead() {
    /// @brief Manages navigation through factory test modes by handling function button interrupts, coordinating UI updates, and initializing hardware peripherals for each test stage.

    static uint32_t lastReliableTick = 0;

    // 1. MENU & DEBOUNCE VALIDATION
    // Continue only in test menu and after debounce window expires.
    if ((menu == TEST_MENU) && ((HAL_GetTick() - lastReliableTick) > 10)) {
        // 2. TEST MODE STATE TRANSITION
        // Dispatch function-button action based on current test mode.
        switch (testMode) {
            case TEST_NONE:
                break;

            case TEST_SCREEN:
                // TRANSITION: SCREEN -> KEY TEST
                // Switch from display test to key/input test stage.
                testMode = TEST_KEY;

                // Clear all MCP23017 LED expanders before switching modes
                i2c4_txData[0] = 0x00;
                i2c4_txData[1] = 0x00;
                HAL_I2C_Mem_Write(&hi2c4, ledEqLeft.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);
                HAL_I2C_Mem_Write(&hi2c4, ledEqRight.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);
                HAL_I2C_Mem_Write(&hi2c4, ledLayer.address, MCP23017_GPIOA, I2C_MEMADD_SIZE_8BIT, i2c4_txData, 2, 1);

                // Re-initialize input peripherals for testing
                encoder_initialize();
                keyboard_initialize();
                lcd_test(testMode);
                break;

            case TEST_KEY:
                // TRANSITION: KEY -> AUDIO TEST
                // Switch to audio path verification stage.
                testMode = TEST_AUDIO;
                lcd_test(testMode);
                io_setSyncOut(2);         // Enable MODE-B sync output for test mode
                io.midi.txActive = true;  // Enable MIDI TX for test mode
                io.midi.txClock = true;   // Enable MIDI clock (0xF8) TX only
                io.midi.txTransport = false;
                record();                 // Start looped recording so the sequencer clock runs
                break;

            case TEST_AUDIO:
                // TRANSITION: AUDIO -> SYNC TEST
                // Switch to synchronization verification stage.
                // Sync output is already active from TEST_AUDIO; MIDI notes continue via interruptPlay.
                testMode = TEST_SYNC;
                lcd_test(testMode);
                break;

            case TEST_SYNC:
                // TRANSITION: SYNC -> END/RESET
                // Finalize test flow and trigger full system reset.
                testMode = TEST_END;
                lcd_test(testMode);
                reset();
                NVIC_SystemReset();  // Perform a full hardware reset after test completion
                break;

            case TEST_END:
                break;
        }

        // 3. DEBOUNCE TIMESTAMP UPDATE
        // Store the latest accepted button event tick.
        lastReliableTick = HAL_GetTick();
    }
}

void Controller::interruptUpKeyRead() {
    /// @brief Processes the up-key press event by managing a debounce counter and triggering the action flag under valid system conditions.

    // 1. PRESS COUNTER UPDATE
    // Increment debounce counter until stable press threshold is reached.
    if (keyboard.upKeyCounter < 2) {
        keyboard.upKeyCounter += 1;
    }

    // 2. ACTION FLAG VALIDATION
    // Trigger action only when no alert/conflicting key state is active.
    else if ((!alertFlag) && (keyboard.upKeyFlag) && (!keyboard.downKeyFlag)) {
        keyboard.upActionFlag = true;
    }
}

void Controller::interruptDownKeyRead() {
    /// @brief Processes the down-key press event by managing a debounce counter and triggering the action flag under valid system conditions.

    // 1. PRESS COUNTER UPDATE
    // Increment debounce counter until stable press threshold is reached.
    if (keyboard.downKeyCounter < 2) {
        keyboard.downKeyCounter += 1;
    }

    // 2. ACTION FLAG VALIDATION
    // Trigger action only when no alert/conflicting key state is active.
    else if ((!alertFlag) && (keyboard.downKeyFlag) && (!keyboard.upKeyFlag)) {
        keyboard.downActionFlag = true;
    }
}

void Controller::interruptLongKeyRead() {
    /// @brief Processes long-press keyboard events by managing a duration counter and triggering specific system actions like muting, copying, or resetting when the timing threshold is exceeded.

    // 1. LONG-PRESS COUNTER UPDATE
    // Increment counter until long-press threshold is reached.
    if (keyboard.longKeyCounter < kLongKeyCountHigh) {
        keyboard.longKeyCounter += 1;
    } else {
        // 2. LONG-PRESS ACTION DISPATCH
        // Execute action mapped to the currently active long-press flag.

        // RHYTHM LOCK/UNLOCK ACTIONS
        // Handle long-press actions related to rhythm lock state.
        if (rhythmUnlockKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            rhythmUnlockKeyFlag = false;
            rhythmUnlockActionFlag = true;
        } else if (rhythmLockKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            rhythmLockKeyFlag = false;
            rhythmLockActionFlag = true;
        }

        // MASTER/OUTPUT MUTE ACTIONS
        // Handle long-press actions related to mute/unmute controls.
        else if (masterUnmuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            masterUnmuteKeyFlag = false;
            masterUnmuteActionFlag = true;
        } else if (masterMuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            masterMuteKeyFlag = false;
            masterMuteActionFlag = true;
        } else if (hpUnmuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            hpUnmuteKeyFlag = false;
            hpUnmuteActionFlag = true;
        } else if (hpMuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            hpMuteKeyFlag = false;
            hpMuteActionFlag = true;
        } else if (loUnmuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            loUnmuteKeyFlag = false;
            loUnmuteActionFlag = true;
        } else if (loMuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            loMuteKeyFlag = false;
            loMuteActionFlag = true;
        } else if (liUnmuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            liUnmuteKeyFlag = false;
            liUnmuteActionFlag = true;
        } else if (liMuteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            liMuteKeyFlag = false;
            liMuteActionFlag = true;
        }

        // LFO CLIPBOARD/RESET ACTIONS
        // Handle long-press actions for LFO copy, paste, and reset.
        else if (lfoCopyKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            lfoCopyKeyFlag = false;
            if (selectedLfoNum != -1) {
                lfoCopyActionFlag = true;
                textCopyFlag = true;
            }
        } else if (lfoPasteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            lfoPasteKeyFlag = false;
            if ((lfoCopy.copyActive) && (selectedLfoNum != -1)) {
                lfoPasteActionFlag = true;
                textPasteFlag = true;
            }
        } else if (lfoResetKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            lfoResetKeyFlag = false;
            lfoResetActionFlag = true;
        }

        // LAYER INSTRUMENT ACTIONS
        // Handle long-press actions for layer instrument copy/paste/reset.
        else if (layerInstCopyKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            layerInstCopyKeyFlag = false;
            if (selectedLayerNum != -1) {
                layerInstCopyActionFlag = true;
                textCopyFlag = true;
            }
        } else if (layerInstPasteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            layerInstPasteKeyFlag = false;
            if ((layerInstCopy.copyActive) && (selectedLayerNum != -1)) {
                layerInstPasteActionFlag = true;
                textPasteFlag = true;
            }
        } else if (layerInstResetKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            layerInstResetKeyFlag = false;
            layerInstResetActionFlag = true;
        }

        // LAYER SONG ACTIONS
        // Handle long-press actions for layer song copy/paste/reset.
        else if (layerSongCopyKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            layerFillCopyKeyFlag = false;
            layerSongCopyKeyFlag = false;
            if (selectedLayerNum != -1) {
                layerSongCopyActionFlag = true;
                textCopyFlag = true;
            }
        } else if (layerSongPasteKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            layerFillPasteKeyFlag = false;
            layerSongPasteKeyFlag = false;
            if ((layerSongCopy.copyActive) && selectedLayerNum != -1) {
                layerSongPasteActionFlag = true;
                textPasteFlag = true;
            }
        } else if (layerSongResetKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            layerSongResetKeyFlag = false;
            if (selectedLayerNum != -1) {
                layerSongResetActionFlag = true;
            }
        }

        // NOTE PAGE: ADD held long enough -> a new melody when ADD is released, unless ADD was
        // used as a modifier meanwhile (ADD + pad = step entry, ADD + UP/DOWN = slide). Only a
        // flag here (this is an ISR); the release in the main loop decides.
        else if (noteMelodyKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            noteMelodyKeyFlag = false;
            noteMelodyLongFlag = true;
        }

        // SYNTH PAGES: ADD held long enough -> a random sound when ADD is released.
        else if (synthRandomKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            synthRandomKeyFlag = false;
            synthRandomLongFlag = true;
        }

        // MULTI-STAGE BEAT GENERATION
        // Cycle beat-generation presets on repeated long-press steps.
        else if (layerBeatKeyFlag) {
            keyboard.longKeyCounter = 0;
            layerSongAddPending = false;  // long press: generator, not a single-beat tap
            switch (layerBeatKeyStage) {
                case 0:
                    layerSong_generateBeat(0);
                    layerBeatKeyStage = 1;
                    break;

                case 1:
                    layerSong_generateBeat(1);
                    layerBeatKeyStage = 2;
                    break;

                case 2:
                    layerSong_generateBeat(2);
                    layerBeatKeyStage = 3;
                    break;

                case 3:
                    // Apply BASIC_4 fill to all beats (4 hits per beat).
                    layerSong_generateBeat(3);
                    layerBeatKeyStage = 4;
                    break;

                case 4:
                    // Apply BASIC_8 fill to all beats (8 hits per beat).
                    layerSong_generateBeat(4);
                    layerBeatKeyFlag = false;
                    stopLongKeyTimer();
                    keyboard.longKeyCounter = 0;
                    break;
            }
        } else {
            // Default: Reset and stop if no valid flag is identified
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
        }
    }
}

void Controller::interruptText() {
    /// @brief Toggles the text visibility state or triggers a system flag to clear the displayed text after the visibility timer expires.

    // 1. TEXT VISIBILITY STATE
    // Toggle text display state or complete text timeout handling.
    if (textShow) {
        // Disable visibility for the current display cycle
        textShow = false;
    } else {
        // 2. TEXT SESSION TERMINATION
        // Stop timer and request text clear operation.
        stopTextTimer();

        // Signal the UI controller to wipe the text from the display
        textClearFlag = true;
    }
}

void Controller::interruptLedEq() {
    /// @brief Triggers playback flags to synchronize LED visualization updates with the active EQ processing cycles.

    // 1. EQ LED READ FLAGS
    // Mark both EQ LED channels ready for visualization update.
    ledEqReadFlag[0] = true;
    ledEqReadFlag[1] = true;
}

void Controller::interruptSd() {
    /// @brief Monitors the physical presence of the SD card and handles emergency removal by closing active files, unmounting the file system, and notifying the UI.

    // 1. SD REMOVAL DETECTION
    // Detect unexpected SD card removal using hardware detect signal.
    if ((sd.detect) && (sd_detect() == SD_ERROR)) {
        // 2. FILE SYSTEM SHUTDOWN
        // Close active file handle and unmount SD interface safely.
        f_close(&sd.file);

        // Detach the SD card driver from the system
        sd_unmount();

        // 3. STATE & UI ALERT UPDATE
        // Reset SD flags and notify UI about removal condition.
        sd.detect = false;
        sd.ready = false;

        // Display a critical error message on the LCD for the user
        lcd_drawSdAlert(SD_ERROR_DETECT);
    }
}

void Controller::interruptBeatSync() {
    /// @brief Terminates the beat synchronization output signal and halts the dedicated hardware timer to stop clock pulse generation.

    // 1. SYNC OUTPUT STOP
    // Pull sync output line low to terminate active pulse.
    HAL_GPIO_WritePin(SYNC_OUT_PULSE_GPIO_Port, SYNC_OUT_PULSE_Pin, GPIO_PIN_RESET);

    // 2. SYNC TIMER STOP
    // Stop beat-sync timer to prevent further pulse interrupts.
    stopBeatSyncTimer();
}

void Controller::interruptLimitAlert() {
    /// @brief Disables the limit alert timer and triggers the flag to clear active limit notifications from the system.

    // 1. ALERT TIMER STOP
    // Stop limit-alert timer to end timed alert phase.
    stopLimitAlertTimer();

    // 2. ALERT CLEAR FLAG
    // Request UI/system to clear limit alert state on next cycle.
    limitAlertClearFlag = true;
}
