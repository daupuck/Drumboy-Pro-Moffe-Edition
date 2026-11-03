// Layers, instruments/samples and sequencer song data.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

#include "SampleFormat.h"

namespace {

// Raw SD read buffer (RAM_D1), shared by the sample loader and the sample preview. Both run
// only from the main loop and the loader stops the preview first, so they never overlap.
// 24576 = 4 x 6144; every chunk size used is a multiple of 6144, which divides evenly by every
// possible frame size (1..32 bytes), so a chunk never ends mid-frame.
constexpr uint32_t kSdRawBufferSize = 24576;
alignas(32) uint8_t sdRawBuffer[kSdRawBufferSize];

}  // namespace

////////////////////////////////////////////////////////////////////////////////
/* Song Functions ------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::calculateSongInterval() {
    /// @brief Recalculates all timing-related intervals for the sequencer, metronome, and LFOs.
    /// Converts musical structure (Bars/Measures) into precise sample-based durations.

    // 1. SEQUENCER GRID CALCULATION
    // Compute total samples in the full loop from measure and bar settings.
    songInterval = kMeasureInterval * rhythm.measure * rhythm.bar;

    // Compute total samples in a single bar.
    barInterval = kMeasureInterval * rhythm.measure;

    // Set the base grid unit used by beat-level timing.
    measureInterval = kMeasureInterval;

    // 2. METADATA UPDATE
    // Update total measure count used by the sequencer cursor.
    rhythm.measureTotal = rhythm.measure * rhythm.bar;

    // Align metronome precounter with one full bar.
    metronome.precounterMax = barInterval;

    // 3. LFO PHASE SYNCHRONIZATION
    // Recalculate per-sample phase increment for each LFO.
    for (uint8_t i = 0; i < kLfoLibrarySize; i++) {
        // Keep each LFO cycle locked to the current bar duration.
        lfoLibrary[i].sampleInc = (kLfoSampleSize * kLfoRateDataLibrary[lfoLibrary[i].rate].rate) / barInterval;
    }
}

uint16_t Controller::calculateTriggerInterval() {
    /// @brief Calculates the target sample index for the next synchronized trigger event.
    /// This ensures that transport changes (like Stop or Reset) occur exactly on a musical boundary.

    uint16_t interval;

    // 1. BOUNDARY CALCULATION DISPATCHER
    // Select snap boundary according to the active trigger mode.
    switch (triggerMode) {
        case TRIG_MEASURE:

            // Snap trigger to the end of the current measure.
            interval = ((playInterval / measureInterval + 1) * measureInterval) - 1;
            break;

        case TRIG_BAR:

            // Snap trigger to the end of the current bar.
            interval = ((playInterval / barInterval + 1) * barInterval) - 1;
            break;

        case TRIG_SONG:

            // Snap trigger to the end of the full song loop.
            interval = songInterval - 1;
            break;
    }

    return interval;
}

void Controller::adjustMeasureBarTiming() {
    /// @brief Synchronizes the sequencer grid with new timing settings and prunes invalid data.
    /// Recalculates all internal timing intervals and removes any beats that fall outside
    /// the newly defined song boundaries.

    // 1. REFRESH TIMING CONSTANTS
    // Rebuild timing intervals using current rhythm settings.
    calculateSongInterval();

    // 2. OUT-OF-BOUNDS MEMORY CLEANUP
    // Remove beat data that no longer fits inside the new loop length.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        for (uint8_t j = 0; j < kBankLibrarySize; j++) {
            // 3. SELECTIVE TRUNCATION
            // Delete beats at or beyond the new song interval.
            layerSong_resetBeats(i, j, songInterval);
        }
    }
}

void Controller::updatePlayTimerPeriod() {
    /// @brief Recalculates and applies hardware timer periods based on the current BPM.
    /// Converts musical Tempo (BPM) into precise microsecond intervals for the sequencer engine.

    // 1. SEQUENCER CLOCK CALCULATION
    // Convert BPM to the playback timer period.
    playTimerPeriod = kMicroSecondsinOneMinute / (rhythm.tempo * kMeasureInterval);

    // 2. METRONOME/MEASURE TIMING
    // Derive measure pulse period from current tempo based on 44.1kHz audio rate.
    measureTimerPeriod = 2646000 / rhythm.tempo;

    // 3. MIDI CLOCK CALCULATION
    // Calculate 24 PPQN pulse period (60,000,000us / 24 pulses = 2,500,000).
    midiClockPeriod = 2500000 / rhythm.tempo;

    // 4. HARDWARE REGISTER UPDATES
    // Write new period to TIM15 auto-reload register for the sequencer.
    __HAL_TIM_SET_AUTORELOAD(&htim15, playTimerPeriod - 1);

    // Write new period to TIM4 auto-reload register for external MIDI sync.
    __HAL_TIM_SET_AUTORELOAD(&htim4, midiClockPeriod - 1);
}

////////////////////////////////////////////////////////////////////////////////
/* Layer Functions -----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::layer_playBeat(uint8_t layerNum_, float fillVolMultiplier_, int8_t note_, uint16_t lengthIntervals_, uint8_t beatFlags_) {
    /// @brief Plays a beat (sequencer, pads, auditions) and, v2.04, the same note on every layer
    /// that FOLLOWs this one, at its interval in this layer's key.
    /// v2.08: with ARP on, the note becomes an arpeggio instead; arp_tick() plays its notes
    /// through here again (kPlayArp), so harmony layers follow every arp note.

    if (!(beatFlags_ & (kPlayHarmony | kPlayArp)) && arp_wants(layerNum_)) {
        arp_start(layerNum_, fillVolMultiplier_, note_, lengthIntervals_, beatFlags_);
        return;
    }

    layer_playVoice(layerNum_, fillVolMultiplier_, note_, lengthIntervals_, beatFlags_);
    if (beatFlags_ & kPlayHarmony) return;  // a harmony note never starts another one

    const Layer& source = layerLibrary[layerNum_];
    if (!tone::playsNotes(source.toneMode)) return;
    for (uint8_t h = 0; h < kLayerLibrarySize; h++) {
        if (h == layerNum_) continue;
        const Layer& follower = layerLibrary[h];
        if ((follower.followLayer != static_cast<int8_t>(layerNum_)) || !tone::playsNotes(follower.toneMode)) continue;
        if (follower.mute || !layerSounds(h)) continue;
        const int8_t note = harmony::note(note_, follower.harmonyInterval, source.toneScale);
        layer_playVoice(h, fillVolMultiplier_, note, lengthIntervals_, static_cast<uint8_t>(beatFlags_ | kPlayHarmony));
    }
}

uint8_t Controller::layer_keyLayer(uint8_t layerNum_) const {
    /// @brief The layer whose ROOT / SCALE a layer's notes are played in: the followed layer for
    /// a harmony layer (v2.04), otherwise the layer itself.

    const int8_t f = layerLibrary[layerNum_].followLayer;
    return ((f >= 0) && (f < static_cast<int8_t>(kLayerLibrarySize)) && (f != static_cast<int8_t>(layerNum_))) ? static_cast<uint8_t>(f) : layerNum_;
}

void Controller::layer_playVoice(uint8_t layerNum_, float fillVolMultiplier_, int8_t note_, uint16_t lengthIntervals_, uint8_t beatFlags_) {
    /// @brief Prepares and triggers layer beat playback with LFO/style-dependent modulation.

    // VISUALS mode: every trigger flashes its layer (velocity; the main loop takes it).
    if (visualActive && (layerNum_ < kLayerLibrarySize) && (fillVolMultiplier_ > visTap.hit[layerNum_])) visTap.hit[layerNum_] = fillVolMultiplier_;

    // 0. SYNTH LAYERS have their own voice and need no sample.
    if (layerLibrary[layerNum_].toneMode == tone::kModeSynth) {
        synth_playBeat(layerNum_, fillVolMultiplier_, note_, lengthIntervals_, beatFlags_);
        return;
    }

    // 1. CONTEXT LOAD
    // Resolve layer, play-state buffers, and temporary beat structures.
    toneReleasePending[layerNum_] = false;  // PAD HOLD: an old release must not cut this new note
    // A note recorded with PAD HOLD fades out after its length (counted down by the sequencer).
    toneHoldLeft[layerNum_] = (beatFlags_ & kPlayHeld) ? lengthIntervals_ : 0;
    Layer& layer = layerLibrary[layerNum_];
    LayerPlayData& lD = layerPlayData[layerNum_];
    BeatPlayData& tBD = lD.tempBeatPlayData;
    BeatPlayData& aBD = lD.activeBeatPlayData;

    // 2. PLAYBACK GATE
    // Continue only when a playable sample sector is selected and active.
    if ((layer.samplePlay) && (!layer.sampleLoading) && (layer.playSampleSector != -1)) {
        uint8_t targetSector = layer.playSampleSector;
        if ((sampleSectorLibrary[layer.playSampleSector].active) && (sampleSectorLibrary[layer.playSampleSector].playMode) && (!(sampleSectorLibrary[layer.playSampleSector].writeMode))) {
            int8_t nDirection, nResolution;
            int16_t nPitch, nLevel, nStart, nEnd, nTiming, nProbability;
            float normMultiplier;
            ((mixer.normalize) && (layer.normalize)) ? normMultiplier = sampleSectorLibrary[targetSector].normMultiplier : normMultiplier = 1.0f;

            // calculate trigger probability
            bool probability = true;
            int random = rand() % 10000;
            float threshold;
            if (layer.style) {
                if ((layer.lfoProbability) && (lfoLibrary[layer.lfoProbability - 1].active)) {
                    Lfo& lfo = lfoLibrary[layer.lfoProbability - 1];
                    float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                    float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                    nProbability = layer.probability + round(100 * (lfoData - 0.5) * lfoDepth);
                    nProbability = clamp(nProbability, (int16_t)kMinLayerProbability, (int16_t)kMaxLayerProbability);
                } else {
                    nProbability = layer.probability;
                }
                threshold = kNumberDataLibrary[nProbability].stdMultiplier * 10000.0f;
                if (random > threshold) probability = false;
            }
            if (probability) {
                // initialize temp beat data
                tBD.active = true;
                tBD.ramAddress = RAM_SAMPLE_ADDRESS + (targetSector * kSampleByteSize);
                tBD.counterMax = sampleSectorLibrary[targetSector].size;
                tBD.mixerInMode = layer.mixerInMode;
                lD.beatStartFlag = true;
                lD.beatEndFlag = false;
                if (layer.style) {
                    // calculate level with lfo
                    if ((layer.lfoLevel) && (lfoLibrary[(layer.lfoLevel - 1)].active)) {
                        Lfo& lfo = lfoLibrary[layer.lfoLevel - 1];
                        float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                        float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                        nLevel = layer.level + round(50 * (lfoData - 0.5) * lfoDepth);
                        nLevel = clamp(nLevel, (int16_t)kMinLayerLevel, (int16_t)kMaxLayerLevel);
                    } else {
                        nLevel = layer.level;
                    }
                    tBD.volumeMultiplier = kLayerVolumeCoef * kNumberDataLibrary[nLevel].powMultiplier * fillVolMultiplier_ * normMultiplier * mixer.masterVolumeFloat;

                    // calculate pitch with lfo
                    if ((layer.lfoPitch) && (lfoLibrary[(layer.lfoPitch - 1)].active)) {
                        Lfo& lfo = lfoLibrary[(layer.lfoPitch - 1)];
                        float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                        float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                        nPitch = layer.pitch + round(100 * (lfoData - 0.5) * lfoDepth);
                        nPitch = clamp(nPitch, (int16_t)kMinLayerPitch, (int16_t)kMaxLayerPitch);
                    } else {
                        nPitch = layer.pitch;
                    }
                    tBD.increment = kLayerPitchDataLibrary[nPitch].data;

                    // calculate resolution with lfo
                    if ((layer.lfoResolution) && (lfoLibrary[(layer.lfoResolution - 1)].active)) {
                        Lfo& lfo = lfoLibrary[(layer.lfoResolution - 1)];
                        float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                        float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                        nResolution = layer.resolution + round(24 * (lfoData - 0.5) * lfoDepth);
                        nResolution = clamp(nResolution, (int8_t)kMinLayerResolution, (int8_t)kMaxLayerResolution);
                    } else {
                        nResolution = layer.resolution;
                    }
                    tBD.resolutionCoef = kLayerResolutionDataLibrary[nResolution].data;

                    // calculate start point with lfo
                    if ((layer.lfoStart) && (lfoLibrary[(layer.lfoStart - 1)].active)) {
                        Lfo& lfo = lfoLibrary[(layer.lfoStart - 1)];
                        float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                        float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                        nStart = layer.start + round(100 * (lfoData - 0.5) * lfoDepth);
                        nStart = clamp(nStart, (int16_t)0, (int16_t)100);
                    } else {
                        nStart = layer.start;
                    }

                    // calculate end point with lfo
                    if ((layer.lfoEnd) && (lfoLibrary[(layer.lfoEnd - 1)].active)) {
                        Lfo& lfo = lfoLibrary[(layer.lfoEnd - 1)];
                        float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                        float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                        nEnd = layer.end + round(100 * (lfoData - 0.5) * lfoDepth);
                        nEnd = clamp(nEnd, (int16_t)0, (int16_t)100);
                    } else {
                        nEnd = layer.end;
                    }
                    if (nStart >= nEnd) {
                        if (nStart > 0)
                            nStart = nEnd - 1;
                        else
                            nEnd = nStart + 1;
                    }
                    tBD.counterStart = kNumberDataLibrary[nStart].stdMultiplier * tBD.counterMax;
                    tBD.counterEnd = kNumberDataLibrary[nEnd].stdMultiplier * tBD.counterMax;

                    // calculate direction with lfo
                    if ((layer.lfoDirection) && (lfoLibrary[(layer.lfoDirection - 1)].active)) {
                        Lfo& lfo = lfoLibrary[(layer.lfoDirection - 1)];
                        float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                        float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                        uint8_t data = round(4 * (lfoData - 0.5) * lfoDepth) + 2;
                        nDirection = kLayerDirectionProbability[layer.direction][data];
                    } else {
                        nDirection = layer.direction;
                    }
                    tBD.directionMode = nDirection;
                    tBD.directionPhase = 0;
                    ((tBD.directionMode == 0) || (tBD.directionMode == 2)) ? tBD.counter = tBD.counterStart : tBD.counter = tBD.counterEnd - 1;

                    // calculate timing delay with lfo
                    if ((layer.lfoTiming) && (lfoLibrary[(layer.lfoTiming - 1)].active)) {
                        Lfo& lfo = lfoLibrary[(layer.lfoTiming - 1)];
                        float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
                        float lfoDepth = kNumberDataLibrary[lfo.depth].stdMultiplier;
                        nTiming = layer.timing + round(100 * (lfoData - 0.5) * lfoDepth);
                        nTiming = clamp(nTiming, (int16_t)kMinLayerTiming, (int16_t)kMaxLayerTiming);
                    } else {
                        nTiming = layer.timing;
                    }
                    if (nTiming == 0) {
                        lD.tempBeatTimingFlag = false;
                        lD.tempBeatTimingCounter = 0;
                        lD.tempBeatTimingCounterMax = 0;
                    } else {
                        lD.tempBeatTimingFlag = true;
                        lD.tempBeatTimingCounter = 0;
                        lD.tempBeatTimingCounterMax = kNumberDataLibrary[nTiming].stdMultiplier * 800;
                    }
                } else {
                    // set default playback parameters
                    tBD.volumeMultiplier = kLayerVolumeCoef * kNumberDataLibrary[layer.level].powMultiplier * fillVolMultiplier_ * normMultiplier * mixer.masterVolumeFloat;
                    tBD.increment = 1;
                    tBD.directionMode = 0;
                    tBD.directionPhase = 0;
                    tBD.counter = 0;
                    tBD.resolutionCoef = kLayerResolutionDataLibrary[kMaxLayerResolution].data;
                    tBD.counterStart = 0;
                    tBD.counterEnd = tBD.counterMax;
                }

                // TONE layers: play the beat's note in the layer's root/scale on top of PITCH
                // (and its LFO). With GLIDE, start at the previous note's speed and let the
                // audio ISR slide to the new one. Drum layers are untouched.
                tBD.glideTarget = 0.0f;
                tBD.glideCoef = 0.0f;
                if (layer.toneMode == tone::kModeTone) {
                    // v2.03: song mode TRANSPOSE moves the note along the scale. v2.04: a
                    // harmony layer plays in the key of the layer it follows.
                    const Layer& key = layerLibrary[layer_keyLayer(layerNum_)];
                    const int semis = tone::semitones(note_ + song_transposeNow(), key.toneScale, key.toneRoot);
                    const float target = tone::clampIncrement(tBD.increment * tone::ratio(semis));
                    // SLIDE on this beat glides even with GLIDE 0 (sample is retriggered, as always on TONE).
                    const bool slideIn = (beatFlags_ & kPlaySlideIn) != 0;
                    if (((layer.toneGlide > 0) || slideIn) && (lD.toneLastIncrement > 0.0f) && (lD.toneLastIncrement != target)) {
                        tBD.increment = lD.toneLastIncrement;
                        tBD.glideTarget = target;
                        tBD.glideCoef = (slideIn && (layer.toneGlide < kSlideGlideMin)) ? tone::glideBlockCoef(kSlideGlideMin, kAudioBlocksPerSecond) : layer.toneGlideCoef;
                    } else {
                        tBD.increment = target;
                    }
                    lD.toneLastIncrement = target;
                    toneMidiNote[layerNum_] = tone::midiNote(semis);
                    toneMidiLegato[layerNum_] = slideIn;
                }

                // configure fade logic based on direction
                switch (tBD.directionMode) {
                    case 0:
                        tBD.fadeFlag = true;
                        tBD.fadeAction = false;
                        tBD.counterFadeStart = tBD.counterEnd - (50 * tBD.increment);
                        tBD.counterFade = 0;
                        break;

                    case 1:
                        tBD.fadeFlag = true;
                        tBD.fadeAction = false;
                        tBD.counterFadeStart = tBD.counterStart + 50;
                        tBD.counterFade = 0;
                        break;

                    case 2:
                        tBD.fadeFlag = false;
                        tBD.fadeAction = false;
                        tBD.counterFadeStart = tBD.counterStart + 50;
                        tBD.counterFade = 0;
                        break;

                    case 3:
                        tBD.fadeFlag = false;
                        tBD.fadeAction = false;
                        tBD.counterFadeStart = tBD.counterEnd - (50 * tBD.increment);
                        tBD.counterFade = 0;
                        break;
                }
                tBD.newBeatFlag = false;

                // activate beat if no delay
                if (!lD.tempBeatTimingFlag) {
                    // aBD is read every audio block by processAudioLayer(), which runs from the
                    // I2S DMA ISR, while this function can itself run from a timer ISR
                    // (interruptPlay). Publish under a critical section so the audio ISR never
                    // observes 'active' alongside a stale/partially-copied ramAddress/counterMax,
                    // or a torn fadeAction/counterFade pair.
                    uint32_t primaskBeat = __get_PRIMASK();
                    __disable_irq();
                    if (aBD.active) {
                        aBD.fadeAction = true;
                        aBD.counterFade = 0;
                        aBD.newBeatFlag = true;
                    } else {
                        memcpy(&aBD, &tBD, sizeof(BeatPlayData));
                        memset(&tBD, 0x00, sizeof(BeatPlayData));
                    }
                    __set_PRIMASK(primaskBeat);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Layer Inst Functions ------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::layerInst_select(uint8_t layerNum_) {
    /// @brief Enters Layer Inst menu or cycles Layer Inst sub-pages for selected layer.
    /// Synchronizes menu context, selection data, UI transitions, and encoder mapping.

    // 1. MENU ENTRY / LAYER SWITCH
    // Enter Layer Inst flow when current menu differs or selected layer changes.
    if (((menu != LAYER_INST_0_MENU) && (menu != LAYER_INST_1_MENU) && (menu != LAYER_INST_2_MENU)) || ((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) && (selectedLayerNum != layerNum_)) {
        // 2. MENU CONTEXT PREPARATION
        // Store previous menu/tab state and open Layer Inst page-0.
        preMenu = menu;
        preMenuTab = menuTab;
        menuTab = 0;
        menu = LAYER_INST_0_MENU;
        synthPage = SYNTH_PAGE_SOUND;  // a SYNTH layer opens on its SOUND (preset) page
        synthStatus[0] = '\0';

        // 3. TARGET LAYER SNAPSHOT
        // Mirror loaded inst/sample fields into selected-edit buffers.
        Layer& layer = layerLibrary[layerNum_];
        layer.instSelected = layer.instLoaded;
        layer.sampleSelected = layer.sampleLoaded;
        layer.instSelectedData = layer.instLoadedData;
        layer.sampleSelectedData = layer.sampleLoadedData;
        layer.wavSelectedData = layer.wavLoadedData;
        uint8_t targetSector = layer.playSampleSector;

        // 4. UI TRANSITION & RENDER
        // Clear previous selection context, then animate and draw layer menu.
        preMenuLayerClear();
        selectedLayerNum = layerNum_;
        synth_captureUndo();  // CLEAR tap on a synth page goes back to this sound
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawLayerInst0Menu(layerNum_, true);
        lcd_drawLayerTab(selectedLayerNum);

        // 5. ENCODER/LED CONTEXT UPDATE
        // Rebind local encoders and update layer-select LED state.
        encoder_update();
        ledLayerPlayMode = false;
        ledLayerSelectFlag = true;
        ledLayerSelectData = 0x01 << selectedLayerNum;

        /* Debug
        if (targetSector >= 0) {
            check(targetSector, 1, 0);
            check(sampleSectorLibrary[targetSector].active, 1, 1);
            check(sampleSectorLibrary[targetSector].playMode, 1, 2);
            check(sampleSectorLibrary[targetSector].writeMode, 1, 3);
            check(sampleSectorLibrary[targetSector].size, 1, 4);
        } else {
            check(-1, 1, 0);
        }
        */
    } else if ((selectedLayerNum == layerNum_) && synthLayerSelected()) {
        // 6a. SYNTH LAYER: SOUND -> OSC -> FILTER -> AMP -> LFO -> ROUTING -> SOUND. Two pages
        // share each LAYER_INST_x_MENU; the knobs and the display follow synthPage.
        synthPage = static_cast<uint8_t>((synthPageNow() + 1) % SYNTH_PAGE_COUNT);
        preMenu = menu;
        menu = static_cast<Menu>(LAYER_INST_0_MENU + kSynthPageMenu[synthPage]);
        lcd_transitionMenu();
        encoder_update();
        if (menu == LAYER_INST_0_MENU) lcd_drawLayerInst0Menu(layerNum_, false);
        if (menu == LAYER_INST_1_MENU) lcd_drawLayerInst1Menu(layerNum_);
        if (menu == LAYER_INST_2_MENU) lcd_drawLayerInst2Menu(layerNum_);
    } else if (selectedLayerNum == layerNum_) {
        // 6. SUB-MENU CYCLING
        // Repeated layer press rotates Layer Inst pages 0 -> 1 -> 2 -> 0.
        if (menu == LAYER_INST_0_MENU) {
            preMenu = menu;
            menu = LAYER_INST_1_MENU;

            // Animate transition and render page-1.
            lcd_transitionMenu();
            lcd_drawLayerInst1Menu(layerNum_);

            // Refresh encoder bindings for new page.
            encoder_update();
        } else if (menu == LAYER_INST_1_MENU) {
            preMenu = menu;
            menu = LAYER_INST_2_MENU;

            // Animate transition and render page-2.
            lcd_transitionMenu();

            // Refresh encoder bindings for new page.
            encoder_update();
            lcd_drawLayerInst2Menu(layerNum_);
        } else if (menu == LAYER_INST_2_MENU) {
            preMenu = menu;
            menu = LAYER_INST_0_MENU;

            // Animate transition and render page-0.
            lcd_transitionMenu();
            lcd_drawLayerInst0Menu(layerNum_, false);

            // Refresh encoder bindings for new page.
            encoder_update();
        }
    }
}
void Controller::layerInst_reset(uint8_t layerNum_) {
    /// @brief Resets Layer Inst parameters to initial defaults.
    /// Restores selection, playback-shape, modulation, routing, and toggle states.

    // 1. DEFAULT PARAMETER RESTORE
    // Apply defaults through setter path to preserve side effects/UI updates.
    layerInst_setInstSelected(layerNum_, -1);
    layerInst_setInstLoaded(layerNum_);
    layerInst_setSampleSelected(layerNum_, -1);
    layerInst_setSampleLoaded(layerNum_);
    layerInst_setLevel(layerNum_, kInitialLayerLevel);
    layerInst_setPitch(layerNum_, kInitialLayerPitch);
    layerInst_setDirection(layerNum_, kInitialLayerDirection);
    layerInst_setResolution(layerNum_, kInitialLayerResolution);
    layerInst_setStart(layerNum_, kInitialLayerStart);
    layerInst_setEnd(layerNum_, kInitialLayerEnd);
    layerInst_setTiming(layerNum_, kInitialLayerTiming);
    layerInst_setProbability(layerNum_, kInitialLayerProbability);
    layerInst_setLfoLevel(layerNum_, kInitialLayerLfo);
    layerInst_setLfoPitch(layerNum_, kInitialLayerLfo);
    layerInst_setLfoDirection(layerNum_, kInitialLayerLfo);
    layerInst_setLfoResolution(layerNum_, kInitialLayerLfo);
    layerInst_setLfoStart(layerNum_, kInitialLayerLfo);
    layerInst_setLfoEnd(layerNum_, kInitialLayerLfo);
    layerInst_setLfoTiming(layerNum_, kInitialLayerLfo);
    layerInst_setLfoProbability(layerNum_, kInitialLayerLfo);
    layerInst_setEq(layerNum_, kInitialLayerEq);
    layerInst_setFilter(layerNum_, kInitialLayerFilter);
    layerInst_setEffect(layerNum_, kInitialLayerEffect);
    layerInst_setReverb(layerNum_, kInitialLayerReverb);
    layerInst_setMute(layerNum_, false);
    layerInst_setFill(layerNum_, true);
    layerInst_setStyle(layerNum_, true);
    layerLibrary[layerNum_].synth = synth::Params{};  // the SYNTH sound belongs to the instrument too
    if (synthInstPageActive() && (selectedLayerNum == layerNum_)) lcd_drawSynthPage(layerNum_);
}

void Controller::layerInst_menuRight() {
    /// @brief Advances Layer Inst tab focus to the right with wrap-around and skip logic.

    // 1. TAB STATE UPDATE
    // Save previous tab, increment tab, and skip hidden range 1 -> 4.
    preMenuTab = menuTab;
    if (menuTab < kMaxMenu8Tab)
        (menuTab == 1) ? menuTab = 4 : menuTab += 1;
    else
        menuTab = kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection-frame transition for tab movement.
    lcd_transitionSelect();
}

void Controller::layerInst_menuLeft() {
    /// @brief Moves Layer Inst tab focus to the left with wrap-around and skip logic.

    // 1. TAB STATE UPDATE
    // Save previous tab, decrement tab, and skip hidden range 4 -> 1.
    preMenuTab = menuTab;
    if (menuTab > kMinMenu8Tab)
        (menuTab == 4) ? menuTab = 1 : menuTab -= 1;
    else
        menuTab = kMaxMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection-frame transition for tab movement.
    lcd_transitionSelect();
}

void Controller::layerInst_menuUp() {
    Layer& layer = layerLibrary[selectedLayerNum];

    /// @brief Increments or toggles parameter mapped to active Layer Inst tab/page.

    // SYNTH layer: tabs whose knob is a synth setting (or empty) edit that instead; LEVEL, PITCH
    // and the routing page keep their normal handling below.
    if (synthInstPageActive() && (menuTab < 8)) {
        const EncoderFunc f = kEncoderSynthFunction[layerInstPage()][menuTab];
        if (f != encoderLocalFunction[menu][menuTab]) {
            synth_change(f, true);
            return;
        }
    }

    // 1. TAB/PAGE-BASED DISPATCH
    // Route encoder-up action to the correct parameter set for active page.
    switch (menuTab) {
        case 0:
            if (instLibrarySize > 0) {
                (layer.instSelected < (instLibrarySize - 1)) ? layerInst_setInstSelected(selectedLayerNum, layer.instSelected + 1) : layerInst_setInstSelected(selectedLayerNum, -1);
            }
            break;

        case 1:
            if ((layer.instSelected != -1) && (sampleLibrarySize[layer.instSelected] > 0)) {
                (layer.sampleSelected < (sampleLibrarySize[layer.instSelected] - 1)) ? layerInst_setSampleSelected(selectedLayerNum, layer.sampleSelected + 1) : layerInst_setSampleSelected(selectedLayerNum, 0);
            }
            break;

        case 4:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoLevel < kMaxLfo) ? layerInst_setLfoLevel(selectedLayerNum, layer.lfoLevel + 1) : layerInst_setLfoLevel(selectedLayerNum, kMinLfo);
                    } else if (layer.level < kMaxLayerLevel)
                        layerInst_setLevel(selectedLayerNum, layer.level + 1);
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoStart < kMaxLfo) ? layerInst_setLfoStart(selectedLayerNum, layer.lfoStart + 1) : layerInst_setLfoStart(selectedLayerNum, kMinLfo);
                    } else if (layer.start < kMaxLayerPosition)
                        layerInst_setStart(selectedLayerNum, layer.start + 1);
                    break;

                case LAYER_INST_2_MENU:
                    (layer.eq < kMaxLayerEq) ? layerInst_setEq(selectedLayerNum, layer.eq + 1) : layerInst_setEq(selectedLayerNum, kMinLayerEq);
                    break;
            }
            break;

        case 5:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoPitch < kMaxLfo) ? layerInst_setLfoPitch(selectedLayerNum, layer.lfoPitch + 1) : layerInst_setLfoPitch(selectedLayerNum, kMinLfo);
                    } else if (layer.pitch < kMaxLayerPitch)
                        layerInst_setPitch(selectedLayerNum, layer.pitch + 1);
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoEnd < kMaxLfo) ? layerInst_setLfoEnd(selectedLayerNum, layer.lfoEnd + 1) : layerInst_setLfoEnd(selectedLayerNum, kMinLfo);
                    } else if (layer.end < kMaxLayerPosition) {
                        layerInst_setEnd(selectedLayerNum, layer.end + 1);
                    }
                    break;

                case LAYER_INST_2_MENU:
                    (layer.filter < kMaxLayerFilter) ? layerInst_setFilter(selectedLayerNum, layer.filter + 1) : layerInst_setFilter(selectedLayerNum, kMinLayerFilter);
                    break;
            }
            break;

        case 6:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoDirection < kMaxLfo) ? layerInst_setLfoDirection(selectedLayerNum, layer.lfoDirection + 1) : layerInst_setLfoDirection(selectedLayerNum, kMinLfo);
                    } else {
                        (layer.direction < kMaxLayerDirection) ? layerInst_setDirection(selectedLayerNum, layer.direction + 1) : layerInst_setDirection(selectedLayerNum, kMinLayerDirection);
                    }
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoTiming < kMaxLfo) ? layerInst_setLfoTiming(selectedLayerNum, layer.lfoTiming + 1) : layerInst_setLfoTiming(selectedLayerNum, kMinLfo);
                    } else if (layer.timing < kMaxLayerTiming)
                        layerInst_setTiming(selectedLayerNum, layer.timing + 1);
                    break;

                case LAYER_INST_2_MENU:
                    (layer.effect < kMaxLayerEffect) ? layerInst_setEffect(selectedLayerNum, layer.effect + 1) : layerInst_setEffect(selectedLayerNum, kMinLayerEffect);
                    break;
            }
            break;

        case 7:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoResolution < kMaxLfo) ? layerInst_setLfoResolution(selectedLayerNum, layer.lfoResolution + 1) : layerInst_setLfoResolution(selectedLayerNum, kMinLfo);
                    } else if (layer.resolution < kMaxLayerResolution)
                        layerInst_setResolution(selectedLayerNum, layer.resolution + 1);
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoProbability < kMaxLfo) ? layerInst_setLfoProbability(selectedLayerNum, layer.lfoProbability + 1) : layerInst_setLfoProbability(selectedLayerNum, kMinLfo);
                    } else if (layer.probability < kMaxLayerProbability)
                        layerInst_setProbability(selectedLayerNum, layer.probability + 1);
                    break;

                case LAYER_INST_2_MENU:
                    (layer.reverb < kMaxLayerReverb) ? layerInst_setReverb(selectedLayerNum, layer.reverb + 1) : layerInst_setReverb(selectedLayerNum, kMinLayerReverb);
                    break;
            }
    }
}

void Controller::layerInst_menuDown() {
    Layer& layer = layerLibrary[selectedLayerNum];

    /// @brief Decrements or toggles parameter mapped to active Layer Inst tab/page.

    // SYNTH layer: see layerInst_menuUp().
    if (synthInstPageActive() && (menuTab < 8)) {
        const EncoderFunc f = kEncoderSynthFunction[layerInstPage()][menuTab];
        if (f != encoderLocalFunction[menu][menuTab]) {
            synth_change(f, false);
            return;
        }
    }

    // 1. TAB/PAGE-BASED DISPATCH
    // Route encoder-down action to the correct parameter set for active page.
    switch (menuTab) {
        case 0:
            if (instLibrarySize > 0) {
                (layer.instSelected > -1) ? layerInst_setInstSelected(selectedLayerNum, layer.instSelected - 1) : layerInst_setInstSelected(selectedLayerNum, instLibrarySize - 1);
            }
            break;

        case 1:
            if ((layer.instSelected != -1) && (sampleLibrarySize[layer.instSelected] > 0)) {
                (layer.sampleSelected > 0) ? layerInst_setSampleSelected(selectedLayerNum, layer.sampleSelected - 1) : layerInst_setSampleSelected(selectedLayerNum, sampleLibrarySize[layer.instSelected] - 1);
            }
            break;

        case 4:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoLevel > kMinLfo) ? layerInst_setLfoLevel(selectedLayerNum, layer.lfoLevel - 1) : layerInst_setLfoLevel(selectedLayerNum, kMaxLfo);
                    } else if (layer.level > kMinLayerLevel)
                        layerInst_setLevel(selectedLayerNum, layer.level - 1);
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoStart > kMinLfo) ? layerInst_setLfoStart(selectedLayerNum, layer.lfoStart - 1) : layerInst_setLfoStart(selectedLayerNum, kMaxLfo);
                    } else if (layer.start > kMinLayerPosition)
                        layerInst_setStart(selectedLayerNum, layer.start - 1);
                    break;

                case LAYER_INST_2_MENU:
                    (layer.eq > kMinLayerEq) ? layerInst_setEq(selectedLayerNum, layer.eq - 1) : layerInst_setEq(selectedLayerNum, kMaxLayerEq);
                    break;
            }
            break;

        case 5:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoPitch > kMinLfo) ? layerInst_setLfoPitch(selectedLayerNum, layer.lfoPitch - 1) : layerInst_setLfoPitch(selectedLayerNum, kMaxLfo);
                    } else if (layer.pitch > kMinLayerPitch)
                        layerInst_setPitch(selectedLayerNum, layer.pitch - 1);
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoEnd > kMinLfo) ? layerInst_setLfoEnd(selectedLayerNum, layer.lfoEnd - 1) : layerInst_setLfoEnd(selectedLayerNum, kMaxLfo);
                    } else if (layer.end > kMinLayerPosition)
                        layerInst_setEnd(selectedLayerNum, layer.end - 1);
                    break;

                case LAYER_INST_2_MENU:
                    (layer.filter > kMinLayerFilter) ? layerInst_setFilter(selectedLayerNum, layer.filter - 1) : layerInst_setFilter(selectedLayerNum, kMaxLayerFilter);
                    break;
            }
            break;

        case 6:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoDirection > kMinLfo) ? layerInst_setLfoDirection(selectedLayerNum, layer.lfoDirection - 1) : layerInst_setLfoDirection(selectedLayerNum, kMaxLfo);
                    } else {
                        (layer.direction > kMinLayerDirection) ? layerInst_setDirection(selectedLayerNum, layer.direction - 1) : layerInst_setDirection(selectedLayerNum, kMaxLayerDirection);
                    }
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoTiming > kMinLfo) ? layerInst_setLfoTiming(selectedLayerNum, layer.lfoTiming - 1) : layerInst_setLfoTiming(selectedLayerNum, kMaxLfo);
                    } else if (layer.timing > kMinLayerTiming)
                        layerInst_setTiming(selectedLayerNum, layer.timing - 1);
                    break;

                case LAYER_INST_2_MENU:
                    (layer.effect > kMinLayerEffect) ? layerInst_setEffect(selectedLayerNum, layer.effect - 1) : layerInst_setEffect(selectedLayerNum, kMaxLayerEffect);
                    break;
            }
            break;

        case 7:
            switch (menu) {
                case LAYER_INST_0_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoResolution > kMinLfo) ? layerInst_setLfoResolution(selectedLayerNum, layer.lfoResolution - 1) : layerInst_setLfoResolution(selectedLayerNum, kMaxLfo);
                    } else if (layer.resolution > kMinLayerResolution)
                        layerInst_setResolution(selectedLayerNum, layer.resolution - 1);
                    break;

                case LAYER_INST_1_MENU:
                    if (keyboard.shiftKeyPress) {
                        (layer.lfoProbability > kMinLfo) ? layerInst_setLfoProbability(selectedLayerNum, layer.lfoProbability - 1) : layerInst_setLfoProbability(selectedLayerNum, kMaxLfo);
                    } else if (layer.probability > kMinLayerProbability)
                        layerInst_setProbability(selectedLayerNum, layer.probability - 1);
                    break;

                case LAYER_INST_2_MENU:
                    (layer.reverb > kMinLayerReverb) ? layerInst_setReverb(selectedLayerNum, layer.reverb - 1) : layerInst_setReverb(selectedLayerNum, kMaxLayerReverb);
                    break;
            }
            break;
    }
}

void Controller::layerInst_setInstSelected(uint8_t layerNum_, int16_t inst_) {
    /// @brief Selects instrument entry for a layer and rebuilds display metadata.
    /// Reads instrument library record, formats labels, and updates related sample selection.

    Layer& layer = layerLibrary[layerNum_];
    if ((inst_ >= -1) && (inst_ < instLibrarySize)) {
        // 1. INPUT GUARD & PREPARE
        // Disable keyboard interrupts during SD/FatFs and metadata operations.
        keyboard_disable();

        // 2. INSTRUMENT INDEX UPDATE
        // Store selected index and clear previous instrument metadata cache.
        layer.instSelected = inst_;
        memset(&(layer.instSelectedData), 0x00, sizeof(InstData));
        if (inst_ != -1) {
            // 3. INSTRUMENT LIBRARY READ
            // Resolve instrument labels and derive file/folder paths.
            sprintf(layer.instSelectedData.num, "%04d", layer.instSelected + 1);
            char temp[kFileNameSize + 1] = {};  // zero-filled: guarantees NUL even for a full 32-byte record
            f_close(&sd.file);
            if ((f_open(&sd.file, "System/Data/Inst.lib", FA_READ) == FR_OK) && (f_lseek(&sd.file, 25 + (layer.instSelected * 32)) == FR_OK) && (sd_read(temp, kFileNameSize) == FR_OK)) {
                f_close(&sd.file);

                // Construct file path
                strcpy(layer.instSelectedData.fileName, "System/Data/");
                strncat(layer.instSelectedData.fileName, temp, kFileNameSize);
                strncat(layer.instSelectedData.fileName, ".ins", 4);

                // Construct folder path
                strcpy(layer.instSelectedData.folderName, "Sample/");
                strncat(layer.instSelectedData.folderName, temp, kFileNameSize);
                strncat(layer.instSelectedData.folderName, "/", 1);

                // Copy full name
                strncpy(layer.instSelectedData.nameLong, temp, kFileNameSize);

                // Format short names for display
                uint8_t length = strlen(temp);

                // Left-aligned short name
                if (length > 12) {
                    strncpy(layer.instSelectedData.nameShortL, temp, 11);
                    strcat(layer.instSelectedData.nameShortL, "_");
                } else if (length == 12) {
                    strncpy(layer.instSelectedData.nameShortL, temp, length);
                } else {
                    char bText[] = "            ";
                    uint8_t bLength = 12 - length;
                    strncpy(layer.instSelectedData.nameShortL, temp, length);
                    strncat(layer.instSelectedData.nameShortL, bText, bLength);
                }

                // Right-aligned short name
                if (length > 10) {
                    strncpy(layer.instSelectedData.nameShortR, temp, 9);
                    strcat(layer.instSelectedData.nameShortR, "_");
                } else if (length == 10) {
                    strncpy(layer.instSelectedData.nameShortR, temp, length);
                } else {
                    char bText[] = "            ";
                    uint8_t bLength = 10 - length;
                    strncpy(layer.instSelectedData.nameShortR, bText, bLength);
                    strncat(layer.instSelectedData.nameShortR, temp, length);
                }
            }
        } else {
            // 3. EMPTY-SELECTION FALLBACK
            // Populate dash placeholders when no instrument is selected.
            memset(layer.instSelectedData.num, '-', 4);
            memcpy(layer.instSelectedData.nameShortL, kDataDashL, 13);
            memcpy(layer.instSelectedData.nameShortR, kDataDashR, 11);
        }

        // 4. SAMPLE SELECTION COUPLING
        // Auto-select first sample when instrument has sample entries.
        ((inst_ != -1) && (sampleLibrarySize[inst_])) ? layerInst_setSampleSelected(layerNum_, 0) : layerInst_setSampleSelected(layerNum_, -1);
        f_close(&sd.file);

        // 5. FINALIZE & UI REFRESH
        // Re-enable keyboard interrupts and refresh visible menu fields.
        keyboard_enable();
        if (((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) && (layerNum_ == selectedLayerNum)) lcd_drawLayerInst_InstData(layerNum_);
    }
}

void Controller::layerInst_setInstLoaded(uint8_t layerNum_) {
    /// @brief Commits selected instrument as loaded instrument for target layer.
    /// Updates cached loaded metadata and refreshes tab/menu visuals.

    Layer& layer = layerLibrary[layerNum_];

    // 1. CHANGE DETECTION
    // Continue only when selected instrument differs from loaded one.
    if (strcmp(layer.instSelectedData.fileName, layer.instLoadedData.fileName) != 0) {
        // 2. LOADED STATE UPDATE
        // Clear previous loaded cache and copy selected state.
        memset(&(layer.instLoadedData), 0x00, sizeof(InstData));
        layer.instLoaded = layer.instSelected;
        layer.instLoadedData = layer.instSelectedData;

        // 3. UI REFRESH
        // Update layer tab and visible Layer Inst menu fields.
        lcd_drawLayerTab_InstData(layerNum_);
        if (((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) && (layerNum_ == selectedLayerNum)) lcd_drawLayerInst_InstData(layerNum_);
    }
}

void Controller::layerInst_setSampleSelected(uint8_t layerNum_, int16_t sample_) {
    /// @brief Selects sample entry for target layer and parses WAV metadata.
    /// Validates RIFF/FMT/DATA chunks and computes playback-size coefficients.

    Layer& layer = layerLibrary[layerNum_];
    if ((sample_ >= -1) && (sample_ < sampleLibrarySize[layerLibrary[layerNum_].instSelected])) {
        // 1. INPUT GUARD & PREPARE
        // Disable keyboard interrupts during SD/FatFs and parser operations.
        keyboard_disable();

        // 2. SAMPLE INDEX UPDATE
        // Store index and clear previous sample/wav metadata cache.
        layer.sampleSelected = sample_;
        layer.sampleSelectedReadError = false;
        layer.sampleSelectedTypeError = false;
        memset(&(layer.sampleSelectedData), 0x00, sizeof(SampleData));
        memset(&(layer.wavSelectedData), 0x00, sizeof(WavData));
        if (sample_ != -1) {
            // 3. SAMPLE LIBRARY LOOKUP
            // Resolve sample name/path from instrument index file.
            sprintf(layer.sampleSelectedData.num, "%04d", layer.sampleSelected + 1);
            char temp[kFileNameSize + 1] = {};
            f_close(&sd.file);
            if ((f_open(&sd.file, layer.instSelectedData.fileName, FA_READ) == FR_OK) && (f_lseek(&sd.file, 25 + (layer.sampleSelected * kFileNameSize)) == FR_OK) && (sd_read(temp, kFileNameSize) == FR_OK)) {
                f_close(&sd.file);

                // Construct full file path
                strcpy(layer.sampleSelectedData.fileName, layer.instSelectedData.folderName);
                strncat(layer.sampleSelectedData.fileName, temp, kFileNameSize);

                // Copy full name
                strncpy(layer.sampleSelectedData.nameLong, temp, kFileNameSize);

                // Format short name
                // strtok returns NULL for an empty record or a name made only of dots.
                char* temp2 = strtok(temp, ".");
                if (temp2 == NULL) temp2 = temp;
                uint8_t length = strlen(temp2);
                if (length > 10) {
                    strncpy(layer.sampleSelectedData.nameShortR, temp2, 9);
                    strcat(layer.sampleSelectedData.nameShortR, "_");
                } else if (length == 10) {
                    strncpy(layer.sampleSelectedData.nameShortR, temp2, length);
                } else {
                    char bText[] = "          ";
                    uint8_t bLength = 10 - length;
                    strncpy(layer.sampleSelectedData.nameShortR, bText, bLength);
                    strncat(layer.sampleSelectedData.nameShortR, temp2, length);
                }

                // 4. WAV HEADER PARSING
                // Parse RIFF/FMT/DATA and compute size/channel/bitrate descriptors.
                bool sampleReadError = false;
                bool sampleTypeError = false;
                if (f_open(&sd.file, layer.sampleSelectedData.fileName, FA_READ) == FR_OK) {
                    if (sd_read(&layer.wavSelectedData.riff_chunk, 12) == FR_OK) {
                        // Check riff header
                        if ((layer.wavSelectedData.riff_chunk.chunkId == 0x46464952) && (layer.wavSelectedData.riff_chunk.fileFormat == 0x45564157)) {
                            // Cap chunkSize to actual file size to guard against corrupt RIFF headers
                            // that report an oversized value, which would cause the search loops to
                            // iterate far past EOF and read garbage or stall for a very long time.
                            uint32_t chunkSize = layer.wavSelectedData.riff_chunk.chunkSize + 8;
                            uint32_t fileSize = f_size(&sd.file);
                            if (chunkSize > fileSize) chunkSize = fileSize;
                            // The searches below compute (chunkSize - 24) / (chunkSize - 8) in
                            // unsigned arithmetic; a tiny or truncated file would wrap that to ~4e9
                            // and stall. Clamp so such files fall through to the "invalid" path.
                            if (chunkSize < 24) chunkSize = 24;

                                // Search for fmt chunk
                            for (uint32_t i = 12; i < (chunkSize - 24); i++) {
                                    f_lseek(&sd.file, i);
                                    sd_read(&layer.wavSelectedData.fmt_chunk, 24);
                                if (layer.wavSelectedData.fmt_chunk.chunkId == 0x20746D66) {
                                    layer.wavSelectedData.fmt_chunk.chunkStartByte = i;
                                    break;
                                }
                            }

                            // Validate fmt chunk parameters
                            if ((layer.wavSelectedData.fmt_chunk.chunkId == 0x20746D66) &&
                                ((layer.wavSelectedData.fmt_chunk.chunkSize == 16) || (layer.wavSelectedData.fmt_chunk.chunkSize == 18) || (layer.wavSelectedData.fmt_chunk.chunkSize == 40)) &&
                                ((layer.wavSelectedData.fmt_chunk.audioFormat == 0x01) || (layer.wavSelectedData.fmt_chunk.audioFormat == 0x03)) &&
                                ((layer.wavSelectedData.fmt_chunk.nbrChannels == 0x01) || (layer.wavSelectedData.fmt_chunk.nbrChannels == 0x02)) &&
                                ((layer.wavSelectedData.fmt_chunk.sampleRate == 8000) || (layer.wavSelectedData.fmt_chunk.sampleRate == 16000) || (layer.wavSelectedData.fmt_chunk.sampleRate == 48000) || (layer.wavSelectedData.fmt_chunk.sampleRate == 11025) || (layer.wavSelectedData.fmt_chunk.sampleRate == 22050) || (layer.wavSelectedData.fmt_chunk.sampleRate == 24000) || (layer.wavSelectedData.fmt_chunk.sampleRate == 44100) ||
                                 (layer.wavSelectedData.fmt_chunk.sampleRate == 48000) || (layer.wavSelectedData.fmt_chunk.sampleRate == 88200) || (layer.wavSelectedData.fmt_chunk.sampleRate == 96000) || (layer.wavSelectedData.fmt_chunk.sampleRate == 176400) || (layer.wavSelectedData.fmt_chunk.sampleRate == 192000)) &&
                                ((layer.wavSelectedData.fmt_chunk.bitPerSample == 8) || (layer.wavSelectedData.fmt_chunk.bitPerSample == 16) || (layer.wavSelectedData.fmt_chunk.bitPerSample == 24) || (layer.wavSelectedData.fmt_chunk.bitPerSample == 32))) {
                                // Search for data chunk
                                for (uint32_t j = 12; j < (chunkSize - 8); j++) {
                                    f_lseek(&sd.file, j);
                                    sd_read(&layer.wavSelectedData.data_chunk, 8);
                                    if (layer.wavSelectedData.data_chunk.chunkId == 0x61746164) {
                                        layer.wavSelectedData.data_chunk.chunkStartByte = j;
                                        break;
                                    }
                                }

                                // Analyze data chunk
                                if (layer.wavSelectedData.data_chunk.chunkId == 0x61746164) {
                                    layer.sampleSelectedData.coefSampleSize = 1.0f;

                                    // Calculate coefficients based on format
                                    switch (layer.wavSelectedData.fmt_chunk.nbrChannels) {
                                        case 0x01:
                                            layer.sampleSelectedData.coefSampleSize *= 1.0f;
                                            layer.sampleSelectedData.channel = CH_MONO;
                                            break;

                                        case 0x02:
                                            layer.sampleSelectedData.coefSampleSize *= 0.5f;
                                            layer.sampleSelectedData.channel = CH_STEREO;
                                            break;
                                    }
                                    switch (layer.wavSelectedData.fmt_chunk.bitPerSample) {
                                        case 8:
                                            layer.sampleSelectedData.coefSampleSize *= 1.0f;
                                            layer.sampleSelectedData.bitdepth = BD_08;
                                            break;

                                        case 16:
                                            layer.sampleSelectedData.coefSampleSize *= 0.5f;
                                            layer.sampleSelectedData.bitdepth = BD_16;
                                            break;

                                        case 24:
                                            layer.sampleSelectedData.coefSampleSize *= (1.0f / 3.0f);
                                            layer.sampleSelectedData.bitdepth = BD_24;
                                            break;

                                        case 32:
                                            layer.sampleSelectedData.coefSampleSize *= 0.25f;
                                            layer.sampleSelectedData.bitdepth = BD_32;
                                            break;
                                    }
                                    switch (layer.wavSelectedData.fmt_chunk.sampleRate) {
                                        case 8000:
                                            layer.sampleSelectedData.coefSampleSize *= 6;
                                            layer.sampleSelectedData.frequency = FR_008kHz;
                                            break;

                                        case 11025:
                                            layer.sampleSelectedData.coefSampleSize *= 4;
                                            layer.sampleSelectedData.frequency = FR_011kHz;
                                            break;

                                        case 16000:
                                            layer.sampleSelectedData.coefSampleSize *= 3;
                                            layer.sampleSelectedData.frequency = FR_016kHz;
                                            break;

                                        case 22050:
                                            layer.sampleSelectedData.coefSampleSize *= 2;
                                            layer.sampleSelectedData.frequency = FR_022kHz;
                                            break;

                                        case 24000:
                                            layer.sampleSelectedData.coefSampleSize *= 2;
                                            layer.sampleSelectedData.frequency = FR_024kHz;
                                            break;

                                        case 44100:
                                            layer.sampleSelectedData.coefSampleSize *= 1;
                                            layer.sampleSelectedData.frequency = FR_044kHz;
                                            break;

                                        case 48000:
                                            layer.sampleSelectedData.coefSampleSize *= 1;
                                            layer.sampleSelectedData.frequency = FR_048kHz;
                                            break;

                                        case 88200:
                                            layer.sampleSelectedData.coefSampleSize *= 0.50;
                                            layer.sampleSelectedData.frequency = FR_088kHz;
                                            break;

                                        case 96000:
                                            layer.sampleSelectedData.coefSampleSize *= 0.50;
                                            layer.sampleSelectedData.frequency = FR_096kHz;
                                            break;

                                        case 176400:
                                            layer.sampleSelectedData.coefSampleSize *= 0.25;
                                            layer.sampleSelectedData.frequency = FR_176kHz;
                                            break;

                                        case 192000:
                                            layer.sampleSelectedData.coefSampleSize *= 0.25;
                                            layer.sampleSelectedData.frequency = FR_192kHz;
                                            break;
                                    }
                                    // Validate data chunk size against actual file size.
                                    // Some WAV files report an incorrect chunkSize in the header
                                    // (e.g. truncated files or buggy encoders). Seek to EOF and
                                    // compute the real available byte count; cap the reported size
                                    // if it exceeds what is actually on disk.
                                    uint32_t actualDataByteSize = f_size(&sd.file) - (layer.wavSelectedData.data_chunk.chunkStartByte + 8);
                                    uint32_t reportedDataByteSize = layer.wavSelectedData.data_chunk.chunkSize;
                                    layer.sampleSelectedData.sampleByteSize = (reportedDataByteSize <= actualDataByteSize) ? reportedDataByteSize : actualDataByteSize;
                                    layer.sampleSelectedData.sampleSize = layer.sampleSelectedData.sampleByteSize * layer.sampleSelectedData.coefSampleSize;

                                    // Cap sample size
                                    if (layer.sampleSelectedData.sampleSize > kSampleSize) {
                                        layer.sampleSelectedData.sampleSize = kSampleSize;
                                        layer.sampleSelectedData.sampleByteSize = layer.sampleSelectedData.sampleSize / layer.sampleSelectedData.coefSampleSize;
                                    }
                                } else {
                                    sampleReadError = true;
                                }
                            } else {
                                sampleTypeError = true;
                            }
                        } else {
                            sampleReadError = true;
                        }
                    } else {
                        sampleReadError = true;
                    }
                } else {
                    sampleReadError = true;
                }
                f_close(&sd.file);

                // 5. ERROR FLAG FINALIZATION
                // Persist parser/type errors and mark NA fields when needed.
                if ((sampleReadError) || (sampleTypeError)) {
                    /* action */
                    layer.sampleSelectedReadError = sampleReadError;
                    layer.sampleSelectedTypeError = sampleTypeError;
                    layer.sampleSelectedData.channel = CH_NA;
                    layer.sampleSelectedData.bitdepth = BD_NA;
                    layer.sampleSelectedData.frequency = FR_NA;
                }
            }
        } else {
            // 3. EMPTY-SELECTION FALLBACK
            // Reset text/format/sample-size fields to neutral defaults.
            memset(layer.sampleSelectedData.num, '-', 4);
            memcpy(layer.sampleSelectedData.nameShortR, kDataDashR, 11);
            layer.sampleSelectedData.channel = CH_NA;
            layer.sampleSelectedData.bitdepth = BD_NA;
            layer.sampleSelectedData.frequency = FR_NA;
            layer.sampleSelectedData.coefSampleSize = 0;
            layer.sampleSelectedData.sampleByteSize = 0;
            layer.sampleSelectedData.sampleSize = 0;
        }

        // 6. FINALIZE & UI REFRESH
        // Close file handle, restore keyboard interrupts, and refresh visible sample fields.
        f_close(&sd.file);
        keyboard_enable();
        if (((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) && (layerNum_ == selectedLayerNum)) lcd_drawLayerInst_SampleData(layerNum_);
    }
}

void Controller::layerInst_setSampleLoaded(uint8_t layerNum_) {
    /// @brief Loads selected sample into SDRAM sector and updates active playback binding.
    /// Handles metadata copy, sector allocation, PCM conversion, normalization, and UI refresh.

    // 1. FATFS PRE-CLEANUP
    // Stop any sample preview first: it shares the SD read buffer with this loader.
    // Then force-close any previous file handle to start from clean FatFs state.
    preview_stop();
    f_close(&sd.file);
    Layer& layer = layerLibrary[layerNum_];
    const int16_t prevSampleLoaded = layer.sampleLoaded;
    char prevSampleLoadedFileName[sizeof(layer.sampleLoadedData.fileName)] = {0};
    memcpy(prevSampleLoadedFileName, layer.sampleLoadedData.fileName, sizeof(prevSampleLoadedFileName));

    // 2. LOAD CONDITION VALIDATION
    // Continue only when selected sample is valid and instrument context still matches.
    if ((!layer.sampleSelectedReadError) && (!layer.sampleSelectedTypeError) && (layer.instSelected == layer.instLoaded)) {
        // Disable keyboard while long-running SD -> SDRAM transfer is in progress.
        keyboard_disable();
        layer.sampleLoading = true;

        // Step 1: Metadata Preparation
        memset(&(layer.sampleLoadedData), 0x00, sizeof(SampleData));
        memset(&(layer.wavLoadedData), 0x00, sizeof(WavData));
        if (layer.sampleSelected != -1) {
            // Copy metadata from selection to active load slot
            layer.sampleLoaded = layer.sampleSelected;
            layer.sampleLoadedData = layer.sampleSelectedData;
            layer.wavLoadedData = layer.wavSelectedData;

            // UI Feedback: Show Magenta Loading indicator
            if (((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) && (layerNum_ == selectedLayerNum)) {
                lcd_setMenuNumState(kAccent2Color);
                lcd.drawText("    LOADING", 11, kMenuSignBoxX[1], kMenuSignBoxY[1]);
            }

            //  HARDWARE SAFETY ---
            // Abort any hanging SDMMC operations to prevent the 20s freeze
            if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_READY) {
                HAL_SD_Abort(&hsd2);
            }

            // Static buffer (24KB), not stack-allocated: RAM_D1 has ~80KB of headroom above
            // the linker's guaranteed heap+stack reservation (8KB heap + 16KB stack), so this
            // comfortably fits without risking a stack collision during a deep call chain.
            //
            // Size is 24576 (6144 x 4): 6144 = 6KB, the original stack-buffer size, chosen
            // because it divides evenly by every possible offsetSize below (1,2,3,4,6,8,12,16,
            // 24,32 -- the product of coefSamplerate {1,2,4} * coefChannel {1,2} * coefBps
            // {1,2,3,4}), so a chunk boundary never lands mid-sample. Any multiple of 6144
            // inherits that same guarantee, so 4x keeps the property while quadrupling read
            // throughput.
            const uint32_t rawArraySize = kSdRawBufferSize;
            uint8_t* rawArray = sdRawBuffer;  // shared with the sample preview (never used concurrently)

            // Determine Format Coefficients and Upsampling Factor (targeting 44.1kHz).
            // Shared with the sample preview; see Global/SampleFormat.h.
            const sampleformat::Plan formatPlan = sampleformat::plan(layer.wavLoadedData.fmt_chunk.sampleRate,
                                                                     layer.wavLoadedData.fmt_chunk.nbrChannels,
                                                                     layer.wavLoadedData.fmt_chunk.bitPerSample);
            const uint32_t offsetSize = formatPlan.offsetSize;
            const uint16_t writeCount = formatPlan.writeCount;

            // Normalization & Tracking Vars
            uint32_t sdramDataCounter = 0;
            float minFloat = 0.0f, maxFloat = 0.0f;

            // Calculate samples per pixel based on final playback size.
            // Guard against division by zero when sampleSize < 60 (very short samples).
            const uint32_t samplesPerPixel = (layer.sampleLoadedData.sampleSize >= 60) ? (layer.sampleLoadedData.sampleSize / 60) : 1;

            // Step 2: SDRAM Sector Selection Algorithm (Referenced from your code)
            bool sectorAvailable = false;
            uint8_t targetSector = 0;

            // Option 1: Find an inactive (passive) sector
            for (uint8_t i = 0; i < kSampleSectorLibrarySize; i++) {
                if (!sampleSectorLibrary[i].active) {
                    sectorAvailable = true;
                    targetSector = i;
                    break;
                }
            }

            // Option 2: Reuse current layer's existing sector if none are free
            if (!sectorAvailable) {
                for (uint8_t i = 0; i < kSampleSectorLibrarySize; i++) {
                    if (sampleSectorLibrary[i].layer == layer.number) {
                        sectorAvailable = true;
                        targetSector = i;
                        break;
                    }
                }
            }

            // Step 3: Open File and Sequential Polling Read
            if (sectorAvailable) {
                // Stop every in-flight beat reading this sector before writeMode goes up, so
                // nothing reads a sample mid-overwrite.
                uint32_t primaskSector = __get_PRIMASK();
                __disable_irq();
                uint32_t targetSectorAddress = RAM_SAMPLE_ADDRESS + (targetSector * kSampleByteSize);
                for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                    BeatPlayData& sectorABD = layerPlayData[i].activeBeatPlayData;
                    BeatPlayData& sectorTBD = layerPlayData[i].tempBeatPlayData;
                    if (sectorABD.active && sectorABD.ramAddress == targetSectorAddress) {
                        sectorABD.active = false;
                        sectorABD.counter = 0;
                    }
                    if (sectorTBD.active && sectorTBD.ramAddress == targetSectorAddress) {
                        sectorTBD.active = false;
                        sectorTBD.counter = 0;
                    }
                }
                layer.writeSampleSector = targetSector;
                sampleSectorLibrary[targetSector].active = true;
                sampleSectorLibrary[targetSector].layer = layer.number;
                sampleSectorLibrary[targetSector].size = layer.sampleLoadedData.sampleSize - 1;
                sampleSectorLibrary[targetSector].playMode = false;
                sampleSectorLibrary[targetSector].writeMode = true;
                sampleSectorLibrary[targetSector].normMultiplier = 1.0f;
                __set_PRIMASK(primaskSector);
                if ((f_open(&sd.file, layer.sampleLoadedData.fileName, FA_READ) == FR_OK) &&
                    (f_lseek(&sd.file, layer.wavLoadedData.data_chunk.chunkStartByte + 8) == FR_OK)) {
                    uint32_t totalBytes = layer.sampleLoadedData.sampleByteSize;
                    uint32_t bytesReadTotal = 0;
                    bool sampleReadFailed = false;

                    // Main Polling Loop
                    while (bytesReadTotal < totalBytes) {
                        uint32_t toRead = (totalBytes - bytesReadTotal > rawArraySize) ? rawArraySize : (totalBytes - bytesReadTotal);

                        // Hardware Guard: Check status register for errors during the loop
                        if (hsd2.Instance->STA & (SDMMC_STA_DTIMEOUT | SDMMC_STA_DCRCFAIL)) {
                            HAL_SD_Abort(&hsd2);
                            sampleReadFailed = true;
                            break;
                        }
                        if (f_read(&sd.file, rawArray, toRead, &sd.bytesread) == FR_OK) {
                            if (sd.bytesread == 0) {
                                // Premature EOF: fewer bytes on disk than the header promised.
                                sampleReadFailed = true;
                                break;
                            }
                            uint32_t readSamples = sd.bytesread / offsetSize;
                            for (uint32_t j = 0; j < readSamples; j++) {
                                // Multiformat PCM to Float Conversion (Global/SampleFormat.h)
                                const float val = sampleformat::toFloat(&rawArray[j * offsetSize], formatPlan.bitsPerSample);
                                if (val < minFloat) minFloat = val;
                                if (val > maxFloat) maxFloat = val;

                                // Update visual peaks for the 60-bin graph array
                                uint8_t graphIdx = sdramDataCounter / samplesPerPixel;
                                if (graphIdx < 60) {
                                    uint8_t currentPeak = static_cast<uint8_t>(fabsf(val) * 30.0f);
                                    if (currentPeak > layer.sampleLoadedData.graphData[graphIdx]) {
                                        layer.sampleLoadedData.graphData[graphIdx] = currentPeak;
                                    }
                                }

                                // Sequential Write to SDRAM
                                uint32_t sdramBase = RAM_SAMPLE_ADDRESS + (layer.writeSampleSector * kSampleByteSize);
                                for (int k = 0; k < writeCount; k++) {
                                    if (sdramDataCounter >= kSampleSize)
                                        break;
                                    *(volatile float*)(sdramBase + (sdramDataCounter * 4)) = val;
                                    sdramDataCounter++;
                                }
                            }
                            bytesReadTotal += sd.bytesread;
                        } else {
                            sampleReadFailed = true;
                            break;  // f_read error recovery
                        }
                        lcd_drawPlay();
                    }

                    if (!sampleReadFailed) {
                        // Finalize Normalization Factor
                        float peak = (fabsf(minFloat) > fabsf(maxFloat)) ? fabsf(minFloat) : fabsf(maxFloat);
                        float normValue = 1.0f / (peak < 0.00001f ? 1.0f : peak);

                        // Switch sector to PlayMode
                        uint32_t primaskPublish = __get_PRIMASK();
                        __disable_irq();
                        sampleSectorLibrary[targetSector].normMultiplier = normValue;
                        __DMB();
                        sampleSectorLibrary[targetSector].playMode = true;
                        sampleSectorLibrary[targetSector].writeMode = false;
                        layer.playSampleSector = layer.writeSampleSector;
                        layer.samplePlay = true;
                        __set_PRIMASK(primaskPublish);

                        // Store duration in seconds (post-upsampling total samples)
                        layer.sampleLoadedData.timeData = static_cast<float>(layer.sampleLoadedData.sampleSize) / AUDIO_SAMPLE_RATE;

                        // Clean up old redundant sectors for this specific layer
                        for (uint8_t s = 0; s < kSampleSectorLibrarySize; s++) {
                            if (sampleSectorLibrary[s].active && sampleSectorLibrary[s].layer == layer.number && s != layer.playSampleSector) {
                                layerInst_clearSampleSector(s);
                            }
                        }
                        sdReadFailCount = 0;
                    } else {
                        // Sector holds partial/corrupt PCM data: release it instead of marking
                        // it playable. Only stop playback if this was the layer's active sector
                        // (Option 2 reuse) -- a freshly allocated sector (Option 1) leaves the
                        // previous, still-valid sample sector untouched.
                        uint32_t primaskRelease = __get_PRIMASK();
                        __disable_irq();
                        bool wasActiveSector = (targetSector == layer.playSampleSector);
                        layerInst_clearSampleSector(targetSector);
                        layer.writeSampleSector = -1;
                        if (wasActiveSector) {
                            layer.samplePlay = false;
                            layer.playSampleSector = -1;
                        }
                        __set_PRIMASK(primaskRelease);

                        if (++sdReadFailCount >= 3) {
                            sdReadFailCount = 0;
                            sd_unmount();
                            sd_mount();
                        }
                    }
                } else {
                    // f_open or f_lseek failed: release the sector that was pre-marked as active
                    // so it does not appear occupied to future load calls.
                    uint32_t primaskOpenFail = __get_PRIMASK();
                    __disable_irq();
                    bool wasPlayingSector = (targetSector == layer.playSampleSector);
                    layerInst_clearSampleSector(targetSector);
                    layer.writeSampleSector = -1;
                    if (wasPlayingSector) {
                        layer.samplePlay = false;
                        layer.playSampleSector = -1;
                    }
                    __set_PRIMASK(primaskOpenFail);
                }
            }
        } else {
            // Handle Empty Selection: Reset layer parameters and clear sectors
            layer.sampleLoaded = layer.sampleSelected;
            layer.sampleLoadedData = layer.sampleSelectedData;
            layer.wavLoadedData = layer.wavSelectedData;
            uint32_t primaskEmpty = __get_PRIMASK();
            __disable_irq();
            layer.samplePlay = false;
            for (uint8_t i = 0; i < kSampleSectorLibrarySize; i++) {
                if (sampleSectorLibrary[i].layer == layer.number) {
                    layerInst_clearSampleSector(i);
                }
            }
            layer.playSampleSector = -1;
            layer.writeSampleSector = -1;
            __set_PRIMASK(primaskEmpty);
        }
        f_close(&sd.file);
        layer.sampleLoading = false;
        keyboard_enable();

        // 4. GRAPH CACHE RESET ON SAMPLE CHANGE
        // Force full graph redraw when loaded sample reference changes.
        bool sampleChanged = (prevSampleLoaded != layer.sampleLoaded) || (memcmp(prevSampleLoadedFileName, layer.sampleLoadedData.fileName, sizeof(prevSampleLoadedFileName)) != 0);
        if (sampleChanged) resetLayerInstGraphCache(layerNum_);

        // 5. UI REFRESH & DEBUG VIEW
        // Update visible sample/graph widgets when Layer Inst menu is active.
        if (((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) && (layerNum_ == selectedLayerNum)) {
            lcd_drawLayerInst_SampleData(layerNum_);
            lcd_drawLayerInst_GraphData(layerNum_);
        }

        /* Debug
        lcd.setFont(FONT_05x07);
        lcd.setForeColor(WHITE);
        lcd.setBackColor(BLACK);
        lcd.setAlignment(LEFT);
        for (uint8_t t = 0; t < 15; t++) {
            lcd.drawNumber(sampleSectorLibrary[t].active, 2, 20, 10 * t);
            lcd.drawNumber(sampleSectorLibrary[t].layer, 2, 40, 10 * t);
            lcd.drawNumber(sampleSectorLibrary[t].playMode, 1, 60, 10 * t);
            lcd.drawNumber(sampleSectorLibrary[t].writeMode, 1, 80, 10 * t);
        }
        */
    }
}

void Controller::layerInst_clearSampleSector(uint8_t sectorNum_) {
    /// @brief Clears one sample sector descriptor and marks it reusable.

    // 1. SECTOR STATE RESET
    // Clear ownership, size, mode flags, and normalization coefficient.
    sampleSectorLibrary[sectorNum_].active = false;
    sampleSectorLibrary[sectorNum_].layer = 0;
    sampleSectorLibrary[sectorNum_].size = 0;
    sampleSectorLibrary[sectorNum_].playMode = false;
    sampleSectorLibrary[sectorNum_].writeMode = false;
    sampleSectorLibrary[sectorNum_].normMultiplier = 1.0f;
}

void Controller::layerInst_setLevel(uint8_t layerNum_, uint8_t level_) {
    /// @brief Sets layer playback level and refreshes related Layer Inst UI.

    // 1. INPUT VALIDATION
    // Accept values only inside configured layer-level range.
    if ((level_ >= kMinLayerLevel) && (level_ <= kMaxLayerLevel)) {
        // 2. DATA UPDATE
        // Apply level to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.level = level_;

        // 3. UI REFRESH
        // Refresh data field when corresponding menu page is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_LevelData(layerNum_);
    }
}

void Controller::layerInst_setPitch(uint8_t layerNum_, uint8_t pitch_) {
    /// @brief Sets layer pitch and refreshes pitch/graph widgets when visible.

    // 1. INPUT VALIDATION
    // Accept values only inside configured pitch range.
    if ((pitch_ >= kMinLayerPitch) && (pitch_ <= kMaxLayerPitch)) {
        // 2. DATA UPDATE
        // Apply pitch to target layer.
        Layer& layer = layerLibrary[layerNum_];
        if (layer.pitch == pitch_) return;
        layer.pitch = pitch_;

        // 3. UI REFRESH
        // Update pitch label and waveform graph when menu is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) {
            lcd_drawLayerInst_PitchData(layerNum_);
            lcd_drawLayerInst_GraphData(layerNum_);
        }
    }
}

void Controller::layerInst_setDirection(uint8_t layerNum_, uint8_t direction_) {
    /// @brief Sets layer playback direction mode and refreshes graph context.

    // 1. INPUT VALIDATION
    // Accept values only inside configured direction range.
    if ((direction_ >= kMinLayerDirection) && (direction_ <= kMaxLayerDirection)) {
        // 2. DATA UPDATE
        // Apply direction mode to target layer.
        Layer& layer = layerLibrary[layerNum_];
        if (layer.direction == direction_) return;
        layer.direction = direction_;

        // 3. UI REFRESH
        // Update direction label and graph when menu is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) {
            lcd_drawLayerInst_DirectionData(layerNum_);
            lcd_drawLayerInst_GraphData(layerNum_);
        }
    }
}

void Controller::layerInst_setResolution(uint8_t layerNum_, uint8_t resolution_) {
    /// @brief Sets layer resolution factor and refreshes graph context.

    // 1. INPUT VALIDATION
    // Accept values only inside configured resolution range.
    if ((resolution_ >= kMinLayerResolution) && (resolution_ <= kMaxLayerResolution)) {
        // 2. DATA UPDATE
        // Apply resolution to target layer.
        Layer& layer = layerLibrary[layerNum_];
        if (layer.resolution == resolution_) return;
        layer.resolution = resolution_;

        // 3. UI REFRESH
        // Update resolution label and graph when menu is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) {
            lcd_drawLayerInst_ResolutionData(layerNum_);
            lcd_drawLayerInst_GraphData(layerNum_);
        }
    }
}

void Controller::layerInst_setStart(uint8_t layerNum_, uint8_t start_) {
    /// @brief Sets layer sample start point under start < end constraint.

    // 1. INPUT VALIDATION
    // Accept valid range only when start remains below current end.
    if ((start_ >= kMinLayerPosition) && (start_ <= kMaxLayerPosition) && (start_ < layerLibrary[layerNum_].end)) {
        // 2. DATA UPDATE
        // Apply start position to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.start = start_;

        // 3. UI REFRESH
        // Update start label and graph when menu is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) {
            lcd_drawLayerInst_StartData(layerNum_);
            lcd_drawLayerInst_GraphData(layerNum_);
        }
    }
}

void Controller::layerInst_setEnd(uint8_t layerNum_, uint8_t end_) {
    /// @brief Sets layer sample end point under end > start constraint.

    // 1. INPUT VALIDATION
    // Accept valid range only when end remains above current start.
    if ((end_ >= kMinLayerPosition) && (end_ <= kMaxLayerPosition) && (end_ > layerLibrary[layerNum_].start)) {
        // 2. DATA UPDATE
        // Apply end position to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.end = end_;

        // 3. UI REFRESH
        // Update end label and graph when menu is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) {
            lcd_drawLayerInst_EndData(layerNum_);
            lcd_drawLayerInst_GraphData(layerNum_);
        }
    }
}

void Controller::layerInst_setTiming(uint8_t layerNum_, uint8_t timing_) {
    /// @brief Sets layer trigger timing offset.

    // 1. INPUT VALIDATION
    // Accept values only inside configured timing range.
    if ((timing_ >= kMinLayerTiming) && (timing_ <= kMaxLayerTiming)) {
        // 2. DATA UPDATE
        // Apply timing value to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.timing = timing_;

        // 3. UI REFRESH
        // Refresh timing label when page-1 is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_TimingData(layerNum_);
    }
}

void Controller::layerInst_setProbability(uint8_t layerNum_, uint8_t probability_) {
    /// @brief Sets layer trigger probability.

    // 1. INPUT VALIDATION
    // Accept values only inside configured probability range.
    if ((probability_ >= kMinLayerProbability) && (probability_ <= kMaxLayerProbability)) {
        // 2. DATA UPDATE
        // Apply probability value to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.probability = probability_;

        // 3. UI REFRESH
        // Refresh probability label when page-1 is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_ProbabilityData(layerNum_);
    }
}

void Controller::layerInst_setLfoLevel(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for level modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to level target.
        Layer& layer = layerLibrary[layerNum_];
        layer.lfoLevel = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-0 is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_LevelData(layerNum_);
    }
}

void Controller::layerInst_setLfoPitch(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for pitch modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to pitch target.
        Layer& layer = layerLibrary[layerNum_];
        layer.lfoPitch = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-0 is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_PitchData(layerNum_);
    }
}

void Controller::layerInst_setLfoDirection(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for direction modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to direction target.
        Layer& layer = layerLibrary[layerNum_];
        layer.lfoDirection = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-0 is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_DirectionData(layerNum_);
    }
}

void Controller::layerInst_setLfoResolution(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for resolution modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to resolution target.
        Layer& layer = layerLibrary[layerNum_];
        layer.lfoResolution = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-0 is visible.
        if ((menu == LAYER_INST_0_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_ResolutionData(layerNum_);
    }
}

void Controller::layerInst_setLfoStart(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for start-position modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to start target.
        Layer& layer = layerLibrary[layerNum_];
        if (layer.lfoStart == lfo_) return;
        layer.lfoStart = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-1 is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_StartData(layerNum_);
    }
}

void Controller::layerInst_setLfoEnd(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for end-position modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to end target.
        Layer& layer = layerLibrary[layerNum_];
        if (layer.lfoEnd == lfo_) return;
        layer.lfoEnd = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-1 is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_EndData(layerNum_);
    }
}

void Controller::layerInst_setLfoTiming(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for timing modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to timing target.
        Layer& layer = layerLibrary[layerNum_];
        layer.lfoTiming = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-1 is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_TimingData(layerNum_);
    }
}

void Controller::layerInst_setLfoProbability(uint8_t layerNum_, uint8_t lfo_) {
    /// @brief Assigns LFO source for probability modulation.

    // 1. INPUT VALIDATION
    // Accept LFO index only inside configured LFO routing range.
    if ((lfo_ >= kMinLfo) && (lfo_ <= kMaxLfo)) {
        // 2. DATA UPDATE
        // Apply LFO routing to probability target.
        Layer& layer = layerLibrary[layerNum_];
        layer.lfoProbability = lfo_;

        // 3. UI REFRESH
        // Refresh related data field when page-1 is visible.
        if ((menu == LAYER_INST_1_MENU) && (selectedLayerNum == layer.number)) lcd_drawLayerInst_ProbabilityData(layerNum_);
    }
}

void Controller::layerInst_setEq(uint8_t layerNum_, uint8_t eq_) {
    /// @brief Sets layer EQ routing option and recalculates mixer input mode.

    // 1. INPUT VALIDATION
    // Accept values only inside configured layer-EQ range.
    if ((eq_ >= kMinLayerEq) && (eq_ <= kMaxLayerEq)) {
        // 2. DATA UPDATE
        // Apply EQ mode to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.eq = eq_;

        // 3. EXCLUSIVE ROUTING RULES
        // Enforce mutually exclusive processing-chain settings.
        switch (layer.eq) {
            case 0:  // Off
                break;

            case 1:  // On
                layerInst_setFilter(layerNum_, 2);
                layerInst_setEffect(layerNum_, 2);
                layerInst_setReverb(layerNum_, 1);
                break;
        }

        // 4. MIX MODE RECOMPUTE
        // Recalculate mixer input route based on active modules.
        layerInst_calculateMixMode(layerNum_);

        // 5. UI REFRESH
        // Refresh page-2 field when Layer Inst page-2 is visible.
        if (menu == LAYER_INST_2_MENU) lcd_drawLayerInst_EqData(layerNum_);
    }
}

void Controller::layerInst_setFilter(uint8_t layerNum_, uint8_t filter_) {
    /// @brief Sets layer filter routing option and recalculates mixer input mode.

    // 1. INPUT VALIDATION
    // Accept values only inside configured layer-filter range.
    if ((filter_ >= kMinLayerFilter) && (filter_ <= kMaxLayerFilter)) {
        // 2. DATA UPDATE
        // Apply filter mode to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.filter = filter_;

        // 3. EXCLUSIVE ROUTING RULES
        // Enforce mutually exclusive processing-chain settings.
        switch (layer.filter) {
            case 0:  // Off
                layerInst_setEq(layerNum_, 0);
                break;

            case 1:  // Filter 1

            case 2:  // Filter 2
                layerInst_setEffect(layerNum_, 2);
                layerInst_setReverb(layerNum_, 1);
                break;
        }

        // 4. MIX MODE RECOMPUTE
        // Recalculate mixer input route based on active modules.
        layerInst_calculateMixMode(layerNum_);

        // 5. UI REFRESH
        // Refresh page-2 field when Layer Inst page-2 is visible.
        if (menu == LAYER_INST_2_MENU) lcd_drawLayerInst_FilterData(layerNum_);
    }
}

void Controller::layerInst_setEffect(uint8_t layerNum_, uint8_t effect_) {
    /// @brief Sets layer effect routing option and recalculates mixer input mode.

    // 1. INPUT VALIDATION
    // Accept values only inside configured layer-effect range.
    if ((effect_ >= kMinLayerEffect) && (effect_ <= kMaxLayerEffect)) {
        // 2. DATA UPDATE
        // Apply effect mode to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.effect = effect_;

        // 3. EXCLUSIVE ROUTING RULES
        // Enforce mutually exclusive processing-chain settings.
        switch (layer.effect) {
            case 0:  // Off
                layerInst_setEq(layerNum_, 0);
                layerInst_setFilter(layerNum_, 0);
                break;

            case 1:  // Effect 1

            case 2:  // Effect 2
                layerInst_setReverb(layerNum_, 1);
                break;
        }

        // 4. MIX MODE RECOMPUTE
        // Recalculate mixer input route based on active modules.
        layerInst_calculateMixMode(layerNum_);

        // 5. UI REFRESH
        // Refresh page-2 field when Layer Inst page-2 is visible.
        if (menu == LAYER_INST_2_MENU) lcd_drawLayerInst_EffectData(layerNum_);
    }
}

void Controller::layerInst_setReverb(uint8_t layerNum_, uint8_t reverb_) {
    /// @brief Sets layer reverb routing option and recalculates mixer input mode.

    // 1. INPUT VALIDATION
    // Accept values only inside configured layer-reverb range.
    if ((reverb_ >= kMinLayerReverb) && (reverb_ <= kMaxLayerReverb)) {
        // 2. DATA UPDATE
        // Apply reverb mode to target layer.
        Layer& layer = layerLibrary[layerNum_];
        layer.reverb = reverb_;

        // 3. EXCLUSIVE ROUTING RULES
        // Enforce mutually exclusive processing-chain settings.
        switch (layer.reverb) {
            case 0:  // Off
                layerInst_setEq(layerNum_, 0);
                layerInst_setFilter(layerNum_, 0);
                layerInst_setEffect(layerNum_, 0);
                break;

            case 1:  // On
                break;
        }

        // 4. MIX MODE RECOMPUTE
        // Recalculate mixer input route based on active modules.
        layerInst_calculateMixMode(layerNum_);

        // 5. UI REFRESH
        // Refresh page-2 field when Layer Inst page-2 is visible.
        if (menu == LAYER_INST_2_MENU) lcd_drawLayerInst_ReverbData(layerNum_);
    }
}

void Controller::layerInst_calculateMixMode(uint8_t layerNum_) {
    /// @brief Determines layer mixer input mode from active processing modules.

    // 1. ROUTING PRIORITY RESOLUTION
    // Apply fixed priority chain to choose single mixer input source.
    Layer& layer = layerLibrary[layerNum_];
    if (layer.eq) {
        layer.mixerInMode = MIXER_IN_EQ;
    } else if (layer.filter == 2) {
        layer.mixerInMode = MIXER_IN_FILTER_0;
    } else if (layer.filter == 1) {
        layer.mixerInMode = MIXER_IN_FILTER_1;
    } else if (layer.effect == 2) {
        layer.mixerInMode = MIXER_IN_EFFECT_0;
    } else if (layer.effect == 1) {
        layer.mixerInMode = MIXER_IN_EFFECT_1;
    } else if (layer.reverb) {
        layer.mixerInMode = MIXER_IN_REVERB;
    } else {
        layer.mixerInMode = MIXER_IN_SEND;
    }
}

void Controller::layerInst_setMute(uint8_t layerNum_, bool mute_) {
    /// @brief Sets layer mute state and refreshes layer-tab indicator.

    // 1. DATA UPDATE
    // Apply mute state to target layer.
    layerLibrary[layerNum_].mute = mute_;

    // 2. UI REFRESH
    // Redraw mute icon on layer tab.
    lcd_drawLayerTab_Mute(layerNum_);
}

void Controller::layerInst_setFill(uint8_t layerNum_, bool fill_) {
    /// @brief Sets layer fill-enable state and refreshes layer-tab indicator.

    // 1. DATA UPDATE
    // Apply fill state to target layer.
    layerLibrary[layerNum_].fill = fill_;

    // 2. UI REFRESH
    // Redraw fill icon on layer tab.
    lcd_drawLayerTab_Fill(layerNum_);
}

void Controller::layerInst_setStyle(uint8_t layerNum_, bool style_) {
    /// @brief Sets layer style-enable state and refreshes layer-tab indicator.

    // 1. DATA UPDATE
    // Apply style state to target layer.
    layerLibrary[layerNum_].style = style_;

    // 2. UI REFRESH
    // Redraw style icon on layer tab.
    lcd_drawLayerTab_Style(layerNum_);
}

void Controller::layerInst_copy() {
    /// @brief Copies current layer-instrument settings to clipboard buffer.

    // 1. SELECTION VALIDATION
    // Continue only when an active layer is selected.
    if (selectedLayerNum != -1) {
        // 2. CLIPBOARD SNAPSHOT
        // Store loaded/sample routing, modulation, FX, and flags.
        layerInstCopy.copyActive = true;
        Layer& layer = layerLibrary[selectedLayerNum];
        layerInstCopy.inst = layer.instLoaded;
        layerInstCopy.sample = layer.sampleLoaded;
        layerInstCopy.level = layer.level;
        layerInstCopy.pitch = layer.pitch;
        layerInstCopy.direction = layer.direction;
        layerInstCopy.resolution = layer.resolution;
        layerInstCopy.start = layer.start;
        layerInstCopy.end = layer.end;
        layerInstCopy.timing = layer.timing;
        layerInstCopy.probability = layer.probability;
        layerInstCopy.lfoLevel = layer.lfoLevel;
        layerInstCopy.lfoPitch = layer.lfoPitch;
        layerInstCopy.lfoDirection = layer.lfoDirection;
        layerInstCopy.lfoResolution = layer.lfoResolution;
        layerInstCopy.lfoStart = layer.lfoStart;
        layerInstCopy.lfoEnd = layer.lfoEnd;
        layerInstCopy.lfoTiming = layer.lfoTiming;
        layerInstCopy.lfoProbability = layer.lfoProbability;
        layerInstCopy.eq = layer.eq;
        layerInstCopy.filter = layer.filter;
        layerInstCopy.effect = layer.effect;
        layerInstCopy.reverb = layer.reverb;
        layerInstCopy.mute = layer.mute;
        layerInstCopy.fill = layer.fill;
        layerInstCopy.style = layer.style;
        layerInstCopy.toneMode = layer.toneMode;
        layerInstCopy.synth = layer.synth;
    }
}

void Controller::layerInst_paste() {
    /// @brief Pastes clipboard Layer Inst settings into selected layer.

    // 1. CLIPBOARD & TARGET VALIDATION
    // Continue only when clipboard is active and layer selection is valid.
    if ((layerInstCopy.copyActive) && (selectedLayerNum != -1)) {
        // 2. PARAMETER RESTORE
        // Re-apply clipboard values through setter path.
        layerInst_setInstSelected(selectedLayerNum, layerInstCopy.inst);
        layerInst_setInstLoaded(selectedLayerNum);
        layerInst_setSampleSelected(selectedLayerNum, layerInstCopy.sample);
        layerInst_setSampleLoaded(selectedLayerNum);
        layerInst_setLevel(selectedLayerNum, layerInstCopy.level);
        layerInst_setPitch(selectedLayerNum, layerInstCopy.pitch);
        layerInst_setDirection(selectedLayerNum, layerInstCopy.direction);
        layerInst_setResolution(selectedLayerNum, layerInstCopy.resolution);
        layerInst_setStart(selectedLayerNum, layerInstCopy.start);
        layerInst_setEnd(selectedLayerNum, layerInstCopy.end);
        layerInst_setTiming(selectedLayerNum, layerInstCopy.timing);
        layerInst_setProbability(selectedLayerNum, layerInstCopy.probability);
        layerInst_setLfoLevel(selectedLayerNum, layerInstCopy.lfoLevel);
        layerInst_setLfoPitch(selectedLayerNum, layerInstCopy.lfoPitch);
        layerInst_setLfoDirection(selectedLayerNum, layerInstCopy.lfoDirection);
        layerInst_setLfoResolution(selectedLayerNum, layerInstCopy.lfoResolution);
        layerInst_setLfoStart(selectedLayerNum, layerInstCopy.lfoStart);
        layerInst_setLfoEnd(selectedLayerNum, layerInstCopy.lfoEnd);
        layerInst_setLfoTiming(selectedLayerNum, layerInstCopy.lfoTiming);
        layerInst_setLfoProbability(selectedLayerNum, layerInstCopy.lfoProbability);
        layerInst_setEq(selectedLayerNum, layerInstCopy.eq);
        layerInst_setFilter(selectedLayerNum, layerInstCopy.filter);
        layerInst_setEffect(selectedLayerNum, layerInstCopy.effect);
        layerInst_setReverb(selectedLayerNum, layerInstCopy.reverb);
        layerInst_setMute(selectedLayerNum, layerInstCopy.mute);
        layerInst_setFill(selectedLayerNum, layerInstCopy.fill);
        layerInst_setStyle(selectedLayerNum, layerInstCopy.style);
        // 3. MODE + SYNTH SOUND (v1.13): a synth layer pastes as the same synth.
        const uint8_t layerNum = static_cast<uint8_t>(selectedLayerNum);
        const bool wasSynth = synthLayerSelected();
        if (wasSynth) synth_silence(layerNum);
        layerLibrary[layerNum].synth = synth::sanitize(layerInstCopy.synth);
        synthPresetIndex[layerNum] = -1;  // named by the pasted sound itself
        tone_setMode(layerNum, layerInstCopy.toneMode);
        lcd_drawLayerTab_InstData(layerNum);

        // 4. REDRAW: a change between sample and SYNTH layer swaps the whole page and its knobs.
        const bool inLayerInst = (menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU);
        if (inLayerInst && (wasSynth != synthLayerSelected())) {
            synthPage = static_cast<uint8_t>((menu == LAYER_INST_0_MENU) ? SYNTH_PAGE_OSC : ((menu == LAYER_INST_1_MENU) ? SYNTH_PAGE_FILTER : SYNTH_PAGE_ROUTING));
            lcd_transitionMenu();
            if (menu == LAYER_INST_0_MENU) lcd_drawLayerInst0Menu(layerNum, true);
            if (menu == LAYER_INST_1_MENU) lcd_drawLayerInst1Menu(layerNum);
            if (menu == LAYER_INST_2_MENU) lcd_drawLayerInst2Menu(layerNum);
            encoder_update();
        } else if (synthInstPageActive()) {
            lcd_drawSynthPage(layerNum);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Layer Song Functions ------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::layerSong_select(uint8_t layerNum_, uint8_t bankNum_) {
    /// @brief Enters Layer Song menu and focuses requested layer/bank context.

    // 1. MENU ENTRY VALIDATION
    // Continue when menu context changes or selected layer differs.
    if ((menu != LAYER_SONG_MENU) || ((menu == LAYER_SONG_MENU) && (selectedLayerNum != layerNum_))) {
        // 2. MENU STATE TRANSITION
        // Store previous menu state and reset local tab navigation.
        preMenu = menu;
        preMenuTab = menuTab;
        menuTab = 0;
        menu = LAYER_SONG_MENU;

        // 3. LAYER CONTEXT PREPARATION
        // Clear previous selection and set current layer/beat defaults.
        preMenuLayerClear();
        selectedLayerNum = layerNum_;

        // Default to first beat when bank has content; otherwise keep no selection.
        (layerLibrary[selectedLayerNum].bankLibrary[bankNum_].lastActiveBeatNum != -1) ? selectedBeatNum = 0 : selectedBeatNum = -1;

        // Always open on the edit page (page 0); S + the same layer again opens EUCLID.
        layerSongPage = 0;
        euclidPlayStep = -1;

        // 4. UI TRANSITION & RENDER
        // Animate and redraw Layer Song menu widgets.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawLayerSongMenu(layerNum_);
        lcd_drawLayerTab(selectedLayerNum);

        // 5. ENCODER & LED CONTEXT UPDATE
        // Rebind encoder map and update layer select LED indicators.
        encoder_update();
        ledLayerPlayMode = false;
        ledLayerSelectFlag = true;
        ledLayerSelectData = 0x01 << selectedLayerNum;
    } else {
        // 6. PAGE CYCLE: S + the layer already shown goes edit -> EUCLID -> NOTE -> edit.
        layerSong_togglePage();
    }
}

void Controller::layerSong_togglePage() {
    /// @brief Cycles the Layer Song menu: edit page -> EUCLID page -> NOTE page -> edit page.
    /// Opening EUCLID captures the layer's beats as the base (for MERGE and CLEAR = restore);
    /// opening NOTE captures the notes (for CLEAR = undo).

    if (selectedLayerNum == -1) return;
    layerSongPage = (layerSongPage + 1) % 4;  // v2.08: ... -> NOTE -> ARP -> edit
    euclidPlayStep = -1;

    // 1. CLEAR THE SLOTS AND THE GRAPH AREA (as a menu transition does).
    lcd.setBackColor(BLACK);
    const uint8_t slots[6] = {0, 1, 4, 5, 6, 7};
    for (uint8_t i = 0; i < 6; i++) lcd_clearMenuTab(slots[i]);
    lcd.setForeColor(GRAY_50);
    lcd.clearRect(493, 30, 325, 60);
    lcd.drawVLine(kMenuLineX[3], kMenuLineY[3], kMenuLineHeight);
    preBeatFillType = -1;
    preBeatFillPattern = -1;
    preBeatFillTime = -1;
    preBeatFillLevel = -1;

    // 2. DRAW THE NEW PAGE
    if (layerSongPage == 1) {
        euclid_captureBase();
        euclid_computePattern();
        euclidTruncated = false;
        lcd_drawLayerSongEuclidMenu(selectedLayerNum);
    } else if (layerSongPage == 2) {
        // EUCLID may have rebuilt the bank, so pick the first beat again.
        Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
        if ((selectedBeatNum < 0) || (selectedBeatNum > bank.lastActiveBeatNum)) selectedBeatNum = (bank.lastActiveBeatNum != -1) ? 0 : -1;
        if (selectedBeatNum >= 0) lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, true);
        note_captureBase();
        noteTransposeTotal = 0;
        lcd_drawLayerSongNoteMenu(selectedLayerNum);
    } else if (layerSongPage == 3) {
        lcd_drawLayerSongArpMenu(selectedLayerNum);
    } else {
        Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
        selectedBeatNum = (bank.lastActiveBeatNum != -1) ? 0 : -1;
        lcd_drawLayerSongMenu(selectedLayerNum);
        lcd_drawSong(selectedLayerNum, activeSongBank);
    }
    encoder_update();
}

void Controller::euclid_captureBase() {
    /// @brief Remembers the selected layer's current beats (active bank) as the EUCLID base.

    if (selectedLayerNum == -1) return;
    euclidBaseCount = layerSong_snapshotBank(selectedLayerNum, activeSongBank, euclidBase);
    euclidBaseLayer = selectedLayerNum;
    euclidBaseBank = activeSongBank;
}

void Controller::euclid_computePattern() {
    /// @brief Recomputes euclidPattern from the current settings (hits, steps, rotation).

    for (uint8_t i = 0; i < kMaxEuclidSteps; i++) euclidPattern[i] = false;
    euclid::pattern(euclidSettings.hits, euclidSettings.steps, euclidPattern);
    euclid::rotate(euclidPattern, euclidSettings.steps, euclidSettings.rotate);
}

void Controller::euclid_apply() {
    /// @brief Writes the Euclid pattern to the selected layer (active bank), live.
    /// Always rebuilt from the base captured when the page opened, so turning knobs back and
    /// forth never accumulates beats. MERGE keeps the base beats and their fills.

    if (selectedLayerNum == -1) return;
    // The layer or bank changed since the page opened: that becomes the new base.
    if ((euclidBaseLayer != selectedLayerNum) || (euclidBaseBank != activeSongBank)) euclid_captureBase();

    euclid_computePattern();

    uint16_t positions[kBeatLibrarySize];
    bool truncatedHits = false;
    const uint8_t hitCount = euclid::positions(euclidPattern, euclidSettings.steps,
                                               static_cast<uint16_t>(kQuantizeInterval[euclidSettings.grid]),
                                               songInterval, euclidSettings.repeat, positions, kBeatLibrarySize, truncatedHits);

    BeatSnapshot beats[kBeatLibrarySize];
    bool truncatedMerge = false;
    const uint8_t count = euclid::compose(euclidBase, euclidBaseCount, positions, hitCount, euclidSettings.merge,
                                          beats, kBeatLibrarySize, truncatedMerge);
    euclidTruncated = truncatedHits || truncatedMerge;

    layerSong_rebuildBank(selectedLayerNum, activeSongBank, beats, count, -1);
    selectedBeatNum = (count > 0) ? 0 : -1;

    lcd_drawSong(selectedLayerNum, activeSongBank);
    lcd_drawEuclidStrip();
}

void Controller::euclid_restore() {
    /// @brief CLEAR on the EUCLID page: puts back the beats the layer had when the page opened.

    if ((selectedLayerNum == -1) || (euclidBaseLayer != selectedLayerNum) || (euclidBaseBank != activeSongBank)) return;
    BeatSnapshot beats[kBeatLibrarySize];
    for (uint8_t i = 0; i < euclidBaseCount; i++) beats[i] = euclidBase[i];
    layerSong_rebuildBank(selectedLayerNum, activeSongBank, beats, euclidBaseCount, -1);
    selectedBeatNum = (euclidBaseCount > 0) ? 0 : -1;
    euclidTruncated = false;
    lcd_drawSong(selectedLayerNum, activeSongBank);
    lcd_drawEuclidStrip();
}

void Controller::euclid_change(EncoderFunc function_, bool up_) {
    /// @brief Handles an EUCLID page knob: updates the setting, redraws it and applies live.

    EuclidSettings& s = euclidSettings;
    uint8_t slot = 0;
    switch (function_) {
        case ENC_EUCLID_STEPS:
            if (up_ && (s.steps < kMaxEuclidSteps)) s.steps++;
            if (!up_ && (s.steps > kMinEuclidSteps)) s.steps--;
            if (s.hits > s.steps) s.hits = s.steps;
            if (s.rotate >= s.steps) s.rotate = s.steps - 1;
            slot = 0;
            break;
        case ENC_EUCLID_HITS:
            if (up_ && (s.hits < s.steps)) s.hits++;
            if (!up_ && (s.hits > 0)) s.hits--;
            slot = 1;
            break;
        case ENC_EUCLID_ROTATE:  // wraps around, like turning a circle
            s.rotate = up_ ? static_cast<uint8_t>((s.rotate + 1) % s.steps) : static_cast<uint8_t>((s.rotate + s.steps - 1) % s.steps);
            slot = 4;
            break;
        case ENC_EUCLID_GRID:
            if (up_ && (s.grid < kMaxEuclidGrid)) s.grid++;
            if (!up_ && (s.grid > kMinEuclidGrid)) s.grid--;
            slot = 5;
            break;
        case ENC_EUCLID_LENGTH:
            s.repeat = !s.repeat;
            slot = 6;
            break;
        case ENC_EUCLID_MODE:
            s.merge = !s.merge;
            slot = 7;
            break;
        default:
            return;
    }

    // STEPS also bounds HITS and ROTATE, so redraw those too.
    lcd_drawEuclidData(slot);
    if (function_ == ENC_EUCLID_STEPS) {
        lcd_drawEuclidData(1);
        lcd_drawEuclidData(4);
    }
    euclid_apply();
}

/* Layer Song NOTE Page / TONE Layers ------------------------------------------*/

// A beat's note is a scale degree (ToneScale.h). Notes are edited here and played by
// layer_playBeat(); they never touch the audio ISR except through the playback speed.

void Controller::note_captureBase() {
    /// @brief Remembers the notes of the selected layer's active bank (CLEAR on the NOTE page
    /// puts them back).

    if (selectedLayerNum == -1) return;
    const Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
    noteBaseCount = 0;
    for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
        const Beat& beat = bank.beatLibrary[i];
        if (!beat.getActive()) continue;
        noteBaseStart[noteBaseCount] = beat.getStartInterval();
        noteBase[noteBaseCount] = static_cast<int8_t>(tone::packNote(beat.getNote(), beat.getSlide()));  // note + SLIDE
        noteBaseCount++;
    }
    noteBaseLayer = selectedLayerNum;
    noteBaseBank = activeSongBank;
}

void Controller::note_restore() {
    /// @brief CLEAR on the NOTE page: every beat that still exists gets its note from when the
    /// page opened back (matched by position, so moved or new beats keep theirs).

    if ((selectedLayerNum == -1) || (noteBaseLayer != selectedLayerNum) || (noteBaseBank != activeSongBank)) return;
    Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
    for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
        Beat& beat = bank.beatLibrary[i];
        if (!beat.getActive()) continue;
        for (uint8_t k = 0; k < noteBaseCount; k++) {
            if (noteBaseStart[k] == beat.getStartInterval()) {
                const uint8_t packed = static_cast<uint8_t>(noteBase[k]);
                beat.setNote(tone::unpackDegree(packed));
                beat.setSlide(tone::unpackSlide(packed));
                break;
            }
        }
    }
    noteTransposeTotal = 0;
    lcd_drawNoteData(7);
    lcd_redrawNotePage();
}

int8_t Controller::note_auditionNote(uint8_t layerNum_) {
    /// @brief Note a manual pad press plays: the selected beat's note while that layer is open
    /// on the NOTE page, otherwise the root.

    if (!notePageActive() || (layerNum_ != selectedLayerNum)) return 0;
    const Bank& bank = layerLibrary[layerNum_].bankLibrary[activeSongBank];
    if ((selectedBeatNum < 0) || (selectedBeatNum > bank.lastActiveBeatNum)) return 0;
    return bank.beatLibrary[selectedBeatNum].getNote();
}

void Controller::note_audition() {
    /// @brief Plays the selected beat's note once after an edit, so you hear what you set.
    /// Only while the transport is stopped (the sequencer is playing the layer otherwise).

    if ((selectedLayerNum == -1) || playActive) return;
    Layer& layer = layerLibrary[selectedLayerNum];
    if (!tone::playsNotes(layer.toneMode) || layer.mute || !layerSounds(selectedLayerNum)) return;
    layer_playBeat(selectedLayerNum, 1.0f, note_auditionNote(selectedLayerNum));
}

bool Controller::noteKeysActive() const {
    // NOTE page of a TONE/SYNTH layer, and (v1.13) the Layer Instrument pages of a SYNTH layer,
    // so the sound can be played across the keyboard while it is being shaped.
    // v2.08: also the ARP page, to play arpeggios from the pads.
    if (synthInstPageActive()) return true;
    return (notePageActive() || arpPageActive()) && (selectedLayerNum != -1) && tone::playsNotes(layerLibrary[selectedLayerNum].toneMode);
}

/* SYNTH Layers ----------------------------------------------------------------*/

bool Controller::layerSounds(uint8_t layerNum_) const {
    const Layer& layer = layerLibrary[layerNum_];
    return layer.samplePlay || (layer.toneMode == tone::kModeSynth);
}

bool Controller::synthLayerSelected() const {
    return (selectedLayerNum != -1) && (layerLibrary[selectedLayerNum].toneMode == tone::kModeSynth);
}

uint8_t Controller::layerInstPage() const {
    // SYNTH layer: the shown SynthPage = row of kEncoderSynthFunction. Otherwise 0..2 per menu.
    if (synthInstPageActive()) return synthPageNow();
    return (menu == LAYER_INST_1_MENU) ? 1 : ((menu == LAYER_INST_2_MENU) ? 2 : 0);
}

uint8_t Controller::synthPageNow() const {
    // synthPage must belong to the current LAYER_INST_x_MENU; if some other path changed the menu
    // (e.g. returning to it from another menu), show that menu's first synth page.
    const uint8_t menuIndex = (menu == LAYER_INST_1_MENU) ? 1 : ((menu == LAYER_INST_2_MENU) ? 2 : 0);
    if ((synthPage < SYNTH_PAGE_COUNT) && (kSynthPageMenu[synthPage] == menuIndex)) return synthPage;
    static const uint8_t kFirstPage[3] = {SYNTH_PAGE_SOUND, SYNTH_PAGE_FILTER, SYNTH_PAGE_LFO};
    return kFirstPage[menuIndex];
}

uint8_t Controller::synthSlotOf(EncoderFunc function_) const {
    const uint8_t page = synthPageNow();
    for (uint8_t i = 0; i < 8; i++) {
        if (kEncoderSynthFunction[page][i] == function_) return i;
    }
    return 0xFF;
}

bool Controller::synthInstPageActive() const {
    return ((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) && synthLayerSelected();
}

void Controller::synth_playBeat(uint8_t layerNum_, float fillVolMultiplier_, int8_t note_, uint16_t lengthIntervals_, uint8_t beatFlags_) {
    /// @brief Prepares a SYNTH note (pitch from note/root/scale/PITCH, level, velocity, gate) and
    /// hands it to the audio ISR. Called from the sequencer ISR and from the main loop.
    /// LEVEL, PITCH and PROB follow the layer's settings and LFOs like samples (with STYLE on).

    Layer& layer = layerLibrary[layerNum_];
    LayerPlayData& lD = layerPlayData[layerNum_];

    int16_t nLevel = layer.level;
    int16_t nPitch = layer.pitch;
    if (layer.style) {
        // Probability (PROB on Layer Inst page 2 of a sample layer, kept for SYNTH layers too).
        int16_t nProbability = layer.probability;
        if ((layer.lfoProbability) && (lfoLibrary[layer.lfoProbability - 1].active)) {
            Lfo& lfo = lfoLibrary[layer.lfoProbability - 1];
            const float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
            nProbability = clamp((int16_t)(layer.probability + round(100 * (lfoData - 0.5) * kNumberDataLibrary[lfo.depth].stdMultiplier)), (int16_t)kMinLayerProbability, (int16_t)kMaxLayerProbability);
        }
        if ((rand() % 10000) > (kNumberDataLibrary[nProbability].stdMultiplier * 10000.0f)) return;

        if ((layer.lfoLevel) && (lfoLibrary[layer.lfoLevel - 1].active)) {
            Lfo& lfo = lfoLibrary[layer.lfoLevel - 1];
            const float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
            nLevel = clamp((int16_t)(layer.level + round(50 * (lfoData - 0.5) * kNumberDataLibrary[lfo.depth].stdMultiplier)), (int16_t)kMinLayerLevel, (int16_t)kMaxLayerLevel);
        }
        if ((layer.lfoPitch) && (lfoLibrary[layer.lfoPitch - 1].active)) {
            Lfo& lfo = lfoLibrary[layer.lfoPitch - 1];
            const float lfoData = kLfoDataLibrary[lfo.type][(uint16_t)floor(lfo.sampleCounter)];
            nPitch = clamp((int16_t)(layer.pitch + round(100 * (lfoData - 0.5) * kNumberDataLibrary[lfo.depth].stdMultiplier)), (int16_t)kMinLayerPitch, (int16_t)kMaxLayerPitch);
        }
    }

    // Pitch: C4 at PITCH 1.00, moved by the note in ROOT/SCALE (and by the song step's
    // TRANSPOSE, v2.03). A harmony layer plays in the key of the layer it follows (v2.04).
    const Layer& key = layerLibrary[layer_keyLayer(layerNum_)];
    const int semis = tone::semitones(note_ + song_transposeNow(), key.toneScale, key.toneRoot);
    const float speed = tone::clampIncrement(kLayerPitchDataLibrary[nPitch].data * tone::ratio(semis));

    // Gate: GATE % of the time until the next beat (an eighth note for pads and auditions). If the
    // next beat slides in, the note is held until it (plus 3 ms) so the two notes join.
    const uint32_t length = (lengthIntervals_ > 0) ? lengthIntervals_ : (kMeasureInterval / 2);
    const float samplesPerInterval = static_cast<float>(playTimerPeriod) * (kAudioSampleRate / 1000000.0f);
    // A note recorded with PAD HOLD (kPlayHeld) sounds exactly as long as the pad was held.
    const bool tieOut = (beatFlags_ & kPlayTieOut) != 0;
    const bool held = (beatFlags_ & kPlayHeld) != 0;
    const float gateShare = (tieOut || held) ? 1.0f : (static_cast<float>(layer.synth.gate) / 100.0f);
    uint32_t gateSamples = static_cast<uint32_t>(gateShare * static_cast<float>(length) * samplesPerInterval);
    if (tieOut) gateSamples += static_cast<uint32_t>(0.003f * kAudioSampleRate);
    if (gateSamples < 64) gateSamples = 64;  // even GATE 0 is a short blip, not silence
    // PAD HOLD: held until the pad is released (the audio ISR then shortens the gate); 60 s at
    // most, so a lost release can never hang a note forever.
    if (beatFlags_ & kPlayHold) gateSamples = static_cast<uint32_t>(60.0f * kAudioSampleRate);

    const bool slideIn = (beatFlags_ & kPlaySlideIn) != 0;
    synth::NoteOn n;
    n.increment = speed * (kSynthC4Hz / kAudioSampleRate);
    n.gain = kLayerVolumeCoef * kNumberDataLibrary[nLevel].powMultiplier * fillVolMultiplier_ * mixer.masterVolumeFloat;
    n.velocity = (fillVolMultiplier_ >= 0.99f) ? 1.0f : 0.5f;  // FLAT = accent, GHOST = softer and darker
    // v2.08 MIDI keys (gain = velocity^2): brightness from 0.5 (softest) to 1.0 (hardest).
    if (beatFlags_ & kPlayVelocity) n.velocity = 0.5f + (0.5f * sqrtf((fillVolMultiplier_ > 0.0f) ? ((fillVolMultiplier_ < 1.0f) ? fillVolMultiplier_ : 1.0f) : 0.0f));
    n.gateSamples = gateSamples;
    n.glideCoef = (layer.toneGlide > 0) ? layer.toneGlideCoef : 0.0f;
    n.lfoRetrig = (layer.synth.lfoRetrig != 0);  // a slide keeps the LFO going (start() ignores it then)
    if (slideIn) {
        n.legato = true;  // envelopes keep going, the pitch glides (GLIDE time, at least ~36 ms)
        n.glideCoef = (layer.toneGlide >= kSlideGlideMin) ? layer.toneGlideCoef : tone::glideBlockCoef(kSlideGlideMin, kAudioBlocksPerSecond);
    }

    // WAVE = SAMPLE: the layer's loaded sample (in SDRAM) is one cycle of the waveform.
    if ((layer.synth.wave == synth::kSample) && (layer.playSampleSector != -1) && !layer.sampleLoading &&
        sampleSectorLibrary[layer.playSampleSector].active && (sampleSectorLibrary[layer.playSampleSector].size >= 2)) {
        n.table = reinterpret_cast<const float*>(RAM_SAMPLE_ADDRESS + (layer.playSampleSector * kSampleByteSize));
        n.tableLength = sampleSectorLibrary[layer.playSampleSector].size;
    }

    toneMidiNote[layerNum_] = tone::midiNote(semis);
    toneMidiLegato[layerNum_] = slideIn;
    lD.beatStartFlag = true;
    lD.beatEndFlag = false;

    // Publish under a critical section: the audio ISR must never see a half-written note.
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    synthPending[layerNum_] = n;
    synthPendingFlag[layerNum_] = true;
    synthReleasePending[layerNum_] = false;  // an old release must not cut this new note
    __set_PRIMASK(primask);
}

float Controller::synth_lfoIncrement(uint8_t layerNum_) const {
    /// @brief Phase step per sample of the layer's own LFO: RATE locked to the current tempo
    /// (bars follow the Rhythm menu's MEASURE). Called by the audio ISR once per block.

    return synth::lfoIncrement(layerLibrary[layerNum_].synth.lfoRate, static_cast<float>(rhythm.tempo), static_cast<float>(rhythm.measure), kAudioSampleRate);
}

void Controller::synth_silence(uint8_t layerNum_) {
    /// @brief Stops a SYNTH voice at once (mode change, new file) and releases its MIDI note.

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    synthPendingFlag[layerNum_] = false;
    synth::stop(synthVoice[layerNum_]);
    __set_PRIMASK(primask);
    if (toneMidiSounding[layerNum_] != 0xFF) {
        uint8_t off[3] = {(uint8_t)(0x80 + io.midi.txChannel), toneMidiSounding[layerNum_], 0x64};
        enqueueMidiTx(off, 3);
        toneMidiSounding[layerNum_] = 0xFF;
    }
}

void Controller::synth_change(EncoderFunc function_, bool up_) {
    /// @brief Handles a SYNTH knob on the Layer Inst pages (also UP/DOWN on the focused tab).

    if (selectedLayerNum == -1) return;
    synth::Params& p = layerLibrary[selectedLayerNum].synth;

    // 1. SOUND PAGE: presets and the save slot.
    if (function_ == ENC_SYNTH_PRESET) {
        const int16_t total = synth::kFactoryPresetCount + synth::kUserPresetCount;
        int16_t index = synthPresetIndex[selectedLayerNum];
        // Not loaded with the knob (new layer, loaded project): start from the preset the sound is.
        if (index < 0) index = synth_matchPreset(static_cast<uint8_t>(selectedLayerNum));
        // Step to the next preset that exists (empty USER slots are skipped).
        for (int16_t tries = 0; tries < total; tries++) {
            index = static_cast<int16_t>(up_ ? index + 1 : index - 1);
            if ((index < 0) || (index >= total)) return;  // ends of the list: no wrap
            if ((index < synth::kFactoryPresetCount) || synthUserUsed[index - synth::kFactoryPresetCount]) break;
        }
        synth_loadPreset(index);
        return;
    }
    if (function_ == ENC_SYNTH_SAVE_SLOT) {
        if (up_ && (synthSaveSlot < synth::kUserPresetCount - 1)) {
            synthSaveSlot++;
        } else if (!up_ && (synthSaveSlot > 0)) {
            synthSaveSlot--;
        } else {
            return;
        }
        synthStatus[0] = '\0';
        lcd_drawSynthData(1);
        lcd_drawSynthGraph();
        return;
    }

    // 2. CHOICES (step through a list, no wrap).
    uint8_t* choice = nullptr;
    uint8_t count = 0;
    switch (function_) {
        case ENC_SYNTH_WAVE: choice = &p.wave; count = synth::kWaveCount; break;
        case ENC_SYNTH_LFO_SHAPE: choice = &p.lfoShape; count = synth::kLfoShapeCount; break;
        case ENC_SYNTH_LFO_RATE: choice = &p.lfoRate; count = synth::kLfoRateCount; break;
        case ENC_SYNTH_LFO_TARGET: choice = &p.lfoTarget; count = synth::kLfoTargetCount; break;
        case ENC_SYNTH_LFO_RETRIG: choice = &p.lfoRetrig; count = 2; break;
        case ENC_SYNTH_FILTER_TYPE: choice = &p.filterType; count = synth::kFilterTypeCount; break;
        case ENC_SYNTH_PAD_HOLD: choice = &p.padHold; count = 2; break;
        case ENC_SYNTH_FM_RATIO: choice = &p.fmRatio; count = synth::kFmRatioCount; break;  // v2.05
        default: break;
    }
    if (choice != nullptr) {
        if (*choice >= count) *choice = 0;  // never index past a table
        if (up_ && (*choice < count - 1)) {
            (*choice)++;
        } else if (!up_ && (*choice > 0)) {
            (*choice)--;
        } else {
            return;
        }
        if (function_ == ENC_SYNTH_PAD_HOLD) {
            note_releaseHeld();  // switching HOLD off must not leave a note hanging
            if (notePageActive()) lcd_drawNoteStrip();  // NOTE page knob 3: shown in the strip
            if (arpPageActive()) lcd_drawArpData(7);    // v2.08: ARP page knob 8
        }
        const uint8_t slot = synthSlotOf(function_);
        if ((slot != 0xFF) && synthInstPageActive()) lcd_drawSynthData(slot);
        if ((function_ != ENC_SYNTH_LFO_RETRIG) && (function_ != ENC_SYNTH_PAD_HOLD)) lcd_drawSynthGraph();
        if (function_ != ENC_SYNTH_PAD_HOLD) lcd_drawLayerTab_InstData(static_cast<uint8_t>(selectedLayerNum));  // "*" = edited
        if ((function_ == ENC_SYNTH_WAVE) || (function_ == ENC_SYNTH_FILTER_TYPE) || (function_ == ENC_SYNTH_FM_RATIO)) note_audition();
        return;
    }

    // 3. VALUES 0..100.
    uint8_t* value = nullptr;
    switch (function_) {
        case ENC_SYNTH_CUTOFF: value = &p.cutoff; break;
        case ENC_SYNTH_RESO: value = &p.resonance; break;
        case ENC_SYNTH_ENVAMT: value = &p.envAmount; break;
        case ENC_SYNTH_DECAY: value = &p.decay; break;
        case ENC_SYNTH_GATE: value = &p.gate; break;
        case ENC_SYNTH_DRIVE: value = &p.drive; break;
        case ENC_SYNTH_DETUNE: value = &p.detune; break;
        case ENC_SYNTH_SUB: value = &p.sub; break;
        case ENC_SYNTH_NOISE: value = &p.noise; break;
        case ENC_SYNTH_LFO_DEPTH: value = &p.lfoDepth; break;
        case ENC_SYNTH_ATTACK: value = &p.attack; break;
        case ENC_SYNTH_AMP_DECAY: value = &p.ampDecay; break;
        case ENC_SYNTH_SUSTAIN: value = &p.sustain; break;
        case ENC_SYNTH_RELEASE: value = &p.release; break;
        // MOD page (v2.05)
        case ENC_SYNTH_FM_AMOUNT: value = &p.fmAmount; break;
        case ENC_SYNTH_FM_DECAY: value = &p.fmDecay; break;
        case ENC_SYNTH_PITCH_ENV: value = &p.pitchEnv; break;
        case ENC_SYNTH_PITCH_DECAY: value = &p.pitchDecay; break;
        case ENC_SYNTH_FILT_ATTACK: value = &p.filterAttack; break;
        default: return;  // ENC_FNC_NONE: empty slot
    }
    if (up_ && (*value < synth::kMaxParam)) {
        (*value)++;
    } else if (!up_ && (*value > 0)) {
        (*value)--;
    } else {
        return;
    }
    const uint8_t slot = synthSlotOf(function_);
    if (slot != 0xFF) lcd_drawSynthData(slot);
    lcd_drawLayerTab_InstData(static_cast<uint8_t>(selectedLayerNum));  // track list: "*" once edited
    // Graphs that show this value; redrawn every few steps so fast turning stays smooth.
    const bool graphValue = (function_ == ENC_SYNTH_DECAY) || (function_ == ENC_SYNTH_GATE) || (function_ == ENC_SYNTH_ENVAMT) ||
                            (function_ == ENC_SYNTH_ATTACK) || (function_ == ENC_SYNTH_AMP_DECAY) || (function_ == ENC_SYNTH_SUSTAIN) ||
                            (function_ == ENC_SYNTH_RELEASE) || (function_ == ENC_SYNTH_LFO_DEPTH) || (function_ == ENC_SYNTH_FM_AMOUNT) ||
                            (function_ == ENC_SYNTH_FM_DECAY) || (function_ == ENC_SYNTH_PITCH_ENV) || (function_ == ENC_SYNTH_PITCH_DECAY) ||
                            (function_ == ENC_SYNTH_FILT_ATTACK);
    if (graphValue && ((*value <= 1) || (*value >= 99) || ((*value % 5) == 0))) lcd_drawSynthGraph();
}

/* SYNTH Sounds: presets, user slots, random, undo ----------------------------*/

void Controller::synth_captureUndo() {
    /// @brief Remembers the selected layer's sound (when the Layer Instrument menu opens), so a
    /// CLEAR tap on a synth page can bring it back after presets, random sounds and edits.

    if (selectedLayerNum == -1) return;
    synthUndoParams = layerLibrary[selectedLayerNum].synth;
    synthUndoLayer = selectedLayerNum;
}

void Controller::synth_undo() {
    if ((selectedLayerNum == -1) || (synthUndoLayer != selectedLayerNum)) return;
    const uint8_t hold = layerLibrary[selectedLayerNum].synth.padHold;  // a playing setting: kept
    layerLibrary[selectedLayerNum].synth = synth::sanitize(synthUndoParams);
    layerLibrary[selectedLayerNum].synth.padHold = hold;
    synthPresetIndex[selectedLayerNum] = -1;
    snprintf(synthStatus, sizeof(synthStatus), "UNDONE");
    synth_soundChanged();
}

void Controller::synth_soundChanged() {
    /// @brief After the whole sound changed (preset, random, undo): redraw the visible synth page
    /// and the layer's name in the track list, and play a note so you hear it.

    if (synthInstPageActive()) lcd_drawSynthPage(static_cast<uint8_t>(selectedLayerNum));
    if (selectedLayerNum != -1) lcd_drawLayerTab_InstData(static_cast<uint8_t>(selectedLayerNum));
    note_audition();
}

int16_t Controller::synth_matchPreset(uint8_t layerNum_) const {
    /// @brief The preset (factory index, or kFactoryPresetCount + USER slot) whose sound the layer
    /// has exactly (PAD HOLD aside), or -1. Works after loading a project too, where the PRESET
    /// knob position is not stored.

    const synth::Params& p = layerLibrary[layerNum_].synth;
    const int16_t factory = synth::findFactoryPreset(p);
    if (factory >= 0) return factory;
    for (uint8_t u = 0; u < synth::kUserPresetCount; u++) {
        if (synthUserUsed[u] && synth::sameSound(p, synthUserPresets[u])) return static_cast<int16_t>(synth::kFactoryPresetCount + u);
    }
    return -1;
}

void Controller::synth_soundName(uint8_t layerNum_, char* name_, size_t size_) const {
    /// @brief Name of a SYNTH layer's sound for the track list: the preset name ("USER 03" for a
    /// user slot); with a "*" when it was edited after loading that preset; "SYNTH" for a sound
    /// of its own (random, or edited and then reloaded).

    const int16_t match = synth_matchPreset(layerNum_);
    const int16_t loaded = synthPresetIndex[layerNum_];
    const int16_t index = (match >= 0) ? match : loaded;
    const char* star = (match >= 0) ? "" : "*";
    if ((index >= 0) && (index < synth::kFactoryPresetCount)) {
        snprintf(name_, size_, "%s%s", synth::kFactoryPresets[index].name, star);
    } else if ((index >= synth::kFactoryPresetCount) && (index < synth::kFactoryPresetCount + synth::kUserPresetCount)) {
        snprintf(name_, size_, "USER %02d%s", index - synth::kFactoryPresetCount + 1, star);
    } else {
        snprintf(name_, size_, "SYNTH");
    }
}

void Controller::synth_loadPreset(int16_t index_) {
    /// @brief SOUND page PRESET knob: factory sound or USER slot onto the selected layer.

    if (selectedLayerNum == -1) return;
    const int16_t total = synth::kFactoryPresetCount + synth::kUserPresetCount;
    if ((index_ < 0) || (index_ >= total)) return;
    const uint8_t hold = layerLibrary[selectedLayerNum].synth.padHold;  // PAD HOLD is how you play, not the sound
    if (index_ < synth::kFactoryPresetCount) {
        layerLibrary[selectedLayerNum].synth = synth::kFactoryPresets[index_].params;
    } else {
        const uint8_t user = static_cast<uint8_t>(index_ - synth::kFactoryPresetCount);
        if (!synthUserUsed[user]) return;
        layerLibrary[selectedLayerNum].synth = synth::sanitize(synthUserPresets[user]);
    }
    layerLibrary[selectedLayerNum].synth.padHold = hold;
    synthPresetIndex[selectedLayerNum] = index_;
    synthStatus[0] = '\0';
    synth_soundChanged();
}

void Controller::synth_saveUser() {
    /// @brief SOUND page, ADD: the selected layer's sound into USER slot SAVE TO, on the SD card.

    if ((selectedLayerNum == -1) || (synthSaveSlot >= synth::kUserPresetCount)) return;
    synthUserPresets[synthSaveSlot] = synth::sanitize(layerLibrary[selectedLayerNum].synth);
    synthUserUsed[synthSaveSlot] = true;
    if (sd_saveSynthUser() == SD_OK) {
        synthPresetIndex[selectedLayerNum] = static_cast<int16_t>(synth::kFactoryPresetCount + synthSaveSlot);
        snprintf(synthStatus, sizeof(synthStatus), "SAVED TO USER %02u", static_cast<unsigned>(synthSaveSlot + 1));
    } else {
        snprintf(synthStatus, sizeof(synthStatus), "SAVE ERROR");
    }
    if (synthInstPageActive()) lcd_drawSynthPage(static_cast<uint8_t>(selectedLayerNum));
    lcd_drawLayerTab_InstData(static_cast<uint8_t>(selectedLayerNum));  // now "USER nn"
}

void Controller::synth_randomize(bool fresh_) {
    /// @brief OSC/FILTER/AMP/LFO page, ADD: hold = a new random sound, tap = a small variation.

    if (selectedLayerNum == -1) return;
    uint32_t seed = DWT_CYCCNT ^ (HAL_GetTick() << 16) ^ 0x5EED5EEDu;  // a different result every time
    synth::Params& p = layerLibrary[selectedLayerNum].synth;
    const uint8_t hold = p.padHold;
    if (fresh_) {
        p = synth::randomSound(seed);
        p.padHold = hold;
        snprintf(synthStatus, sizeof(synthStatus), "RANDOM SOUND");
    } else {
        synth::mutateSound(p, seed);
        snprintf(synthStatus, sizeof(synthStatus), "VARIATION");
    }
    synthPresetIndex[selectedLayerNum] = -1;
    synth_soundChanged();
}

void Controller::synth_addPressed() {
    /// @brief ADD pressed on a synth page. SOUND page: save at once. OSC/FILTER/AMP/LFO: wait for
    /// the release (tap = variation, held long = random sound), so ADD + knob (LFO assignment on
    /// LEVEL/PITCH) or ADD + UP/DOWN never change the sound by accident.

    synthAddPending = false;
    synthRandomLongFlag = false;
    const uint8_t page = synthPageNow();
    if (page == SYNTH_PAGE_SOUND) {
        synth_saveUser();
        return;
    }
    if (page == SYNTH_PAGE_ROUTING) return;
    synthAddPending = true;
    synthRandomKeyFlag = true;
    startLongKeyTimer();
}

void Controller::synth_addReleased() {
    if (synthRandomKeyFlag) {
        stopLongKeyTimer();
        keyboard.longKeyCounter = 0;
        synthRandomKeyFlag = false;
    }
    const bool pending = synthAddPending && synthInstPageActive();
    const bool fresh = synthRandomLongFlag;
    synthAddPending = false;
    synthRandomLongFlag = false;
    if (pending) synth_randomize(fresh);
}

void Controller::note_padPressed(uint8_t pad_) {
    /// @brief A pad on the NOTE page of a TONE layer: plays that scale note on the layer.
    /// Recording: writes a beat with the note (an existing beat there takes the note).
    /// Stopped: just plays (to try sounds without touching the track). ADD + pad = step entry:
    /// the selected beat gets the note and the selection moves on (v1.13; before, every pad
    /// press while stopped edited the track).
    /// Playing without REC: just plays along.

    if ((selectedLayerNum == -1) || (pad_ >= tone::kKeyCount)) return;
    const uint8_t layerNum = static_cast<uint8_t>(selectedLayerNum);
    // v2.04: a harmony layer's pads play in the key of the layer it follows.
    const int8_t note = tone::keyDegree(pad_, noteKeysOctave, layerLibrary[layer_keyLayer(layerNum)].toneScale);
    note_keyDown(note, 1.0f, layerLibrary[layerNum].synth.padHold != 0, static_cast<int16_t>(pad_));
}

void Controller::note_midiIn(const midiin::Event& event_) {
    /// @brief v2.08 MIDI IN: while the pads are a keyboard (NOTE / ARP page of a TONE or SYNTH
    /// layer, or a SYNTH layer's sound pages), a MIDI keyboard plays that layer: snapped to its
    /// scale, with velocity, held until the key is let go, recorded like a pad. Otherwise a note
    /// triggers a drum layer (key % 12), as before.

    const int16_t keyId = static_cast<int16_t>(1000 + event_.key);
    // A key let go always ends its held note, even if the page changed while it was down.
    if (!event_.on && (padHoldLayer >= 0) && (padHoldKey == keyId)) {
        note_releaseHeld();  // monophonic: only the newest key's release ends the note
        return;
    }
    if (noteKeysActive() && !alertFlag) {
        if (event_.on) {
            const uint8_t layerNum = static_cast<uint8_t>(selectedLayerNum);
            const Layer& key = layerLibrary[layer_keyLayer(layerNum)];
            const int8_t degree = tone::degreeForSemitone(static_cast<int>(event_.key) - 60, key.toneScale, key.toneRoot);  // 60 = C4 = PITCH 1.00
            // Velocity: square law (64 = -12 dB), the usual feel of a keyboard.
            const float x = static_cast<float>((event_.velocity > 127) ? 127 : event_.velocity) / 127.0f;
            note_keyDown(degree, x * x, true, keyId);
        }
        return;
    }
    if (!event_.on) return;

    const uint8_t layerNum = event_.key % 12;  // Map MIDI key to layer (0-11)
    if (layerNum >= kLayerLibrarySize) return;
    Layer& layer = layerLibrary[layerNum];
    // Trigger audio if layer is active and unmuted
    if ((!layer.mute) && layerSounds(layerNum)) layer_playBeat(layerNum, 1.0f);
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

void Controller::note_keyDown(int8_t note, float velocity_, bool hold_, int16_t keyId_) {
    /// @brief A key of the pad keyboard (note_padPressed) or of a MIDI keyboard (note_midiIn)
    /// plays `note` on the selected layer: held until the key is let go when `hold_` (PAD HOLD,
    /// always for MIDI), recorded while REC is on, step entry with ADD while stopped.

    if (selectedLayerNum == -1) return;
    const uint8_t layerNum = static_cast<uint8_t>(selectedLayerNum);
    Layer& layer = layerLibrary[layerNum];
    Bank& bank = layer.bankLibrary[activeSongBank];

    note_finishHoldRec();  // monophonic: a held note being recorded ends where the next one starts

    if ((!layer.mute) && layerSounds(layerNum)) {
        // MIDI keys: the synth's brightness follows the velocity smoothly (kPlayVelocity).
        const uint8_t velocityFlag = (keyId_ >= 1000) ? kPlayVelocity : 0;
        if (hold_) {
            // Held: the note lasts until this key is released (note_padReleased / note_midiIn).
            if ((padHoldLayer >= 0) && (padHoldLayer != static_cast<int8_t>(layerNum))) note_releaseHeld();
            layer_playBeat(layerNum, velocity_, note, 0, static_cast<uint8_t>(kPlayHold | velocityFlag));
            padHoldLayer = static_cast<int8_t>(layerNum);
            padHoldKey = keyId_;
        } else {
            layer_playBeat(layerNum, velocity_, note, 0, velocityFlag);
        }
    }

    if (recordActive) {
        if (!metronome.precountState) {
            const uint16_t press = playInterval;
            const uint16_t start = layerSong_quantizeStart(press);
            // With PAD HOLD the release records how long the pad was held (note_finishHoldRec),
            // so playback sounds the same. Set up before the beat exists, so the sequencer never
            // retriggers it while the pad is still down.
            if (hold_ && (padHoldLayer == static_cast<int8_t>(layerNum))) {
                holdRecPress = press;
                holdRecStart = start;
                holdRecBank = activeSongBank;
                holdRecLayer = static_cast<int8_t>(layerNum);  // last: the sequencer ISR reads it
            }
            layerSong_setBeat(layerNum, activeSongBank, press, 0, 0, 0, 0, true, true, note);
            // Until then the beat has the GATE length (also when it replaced an old held note).
            for (int16_t k = 0; k <= bank.lastActiveBeatNum; k++) {
                if (bank.beatLibrary[k].getActive() && (bank.beatLibrary[k].getStartInterval() == start)) {
                    bank.beatLibrary[k].setHold(0);
                    break;
                }
            }
        } else {
            // Hit during precount: committed to position 0 (with its note) once playback starts.
            precountPendingBeat[layerNum] = true;
            precountPendingHasNote[layerNum] = true;
            precountPendingNote[layerNum] = note;
        }
        lcd_redrawNotePage();
        return;
    }

    if (!playActive && keyboard.shiftKeyPress && notePageActive() && (selectedBeatNum >= 0) && (selectedBeatNum <= bank.lastActiveBeatNum)) {
        // ADD is a modifier here: no new melody / variation when it is released.
        if (noteMelodyKeyFlag) {
            stopLongKeyTimer();
            keyboard.longKeyCounter = 0;
            noteMelodyKeyFlag = false;
        }
        noteMelodyLongFlag = false;
        layerSongAddPending = false;
        bank.beatLibrary[selectedBeatNum].setNote(note);
        if (bank.lastActiveBeatNum > 0) {
            layerSong_beatRight();  // next beat (wraps); redraws the NOTE page
        } else {
            lcd_redrawNotePage();
        }
    }
}

void Controller::note_padReleased(uint8_t pad_) {
    /// @brief A pad was let go: with PAD HOLD, the note it started ends now (release / fade).
    /// Another pad pressed since then owns the note, so its release is the one that counts.

    if ((padHoldLayer < 0) || (padHoldKey != static_cast<int16_t>(pad_))) return;
    note_releaseHeld();
}

void Controller::note_releaseHeld() {
    /// @brief Ends the held pad note, if any (pad released, HOLD switched off).

    note_finishHoldRec();
    if (padHoldLayer < 0) return;
    const uint8_t layerNum = static_cast<uint8_t>(padHoldLayer);
    padHoldLayer = -1;
    padHoldKey = -1;
    if (arpRun[layerNum].held) arp_stop(layerNum);  // v2.08: a held arpeggio ends with its key
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        // The held layer, and (v2.04) every harmony layer holding its note along with it.
        if ((l != layerNum) && (layerLibrary[l].followLayer != static_cast<int8_t>(layerNum))) continue;
        if (layerLibrary[l].toneMode == tone::kModeSynth) {
            synthReleasePending[l] = true;
        } else if ((l == layerNum) || (layerLibrary[l].toneMode == tone::kModeTone)) {
            toneReleasePending[l] = true;
        }
    }
}

void Controller::note_finishHoldRec() {
    /// @brief Recording with PAD HOLD: stores how long the pad of the recorded note was held
    /// (in sequencer intervals, across the loop end), so playback sounds it just as long.
    /// Stopped in the meantime: the length is unknown and the note keeps its GATE length.

    if (holdRecLayer < 0) return;
    const uint8_t layerNum = static_cast<uint8_t>(holdRecLayer);
    holdRecLayer = -1;
    if (!playActive || (songInterval == 0)) return;

    const uint16_t now = playInterval;
    uint32_t length = (now >= holdRecPress) ? (now - holdRecPress) : (static_cast<uint32_t>(now) + songInterval - holdRecPress);
    if (length == 0) length = 1;
    if (length > songInterval) length = songInterval;

    Bank& bank = layerLibrary[layerNum].bankLibrary[holdRecBank];
    for (int16_t k = 0; k <= bank.lastActiveBeatNum; k++) {
        Beat& beat = bank.beatLibrary[k];
        if (beat.getActive() && (beat.getStartInterval() == holdRecStart)) {
            beat.setHold(static_cast<uint16_t>(length));
            break;
        }
    }
}

/* Arpeggiator (v2.08) ---------------------------------------------------------*/

bool Controller::arp_wants(uint8_t layerNum_) const {
    /// @brief This layer's notes become arpeggios: ARP is on and it plays notes (TONE / SYNTH).
    /// A FOLLOW layer plays its leader's notes (and so its leader's arpeggio) instead.
    const Layer& layer = layerLibrary[layerNum_];
    return (layer.arp.mode != arp::OFF) && (layer.arp.mode < arp::kModeCount) && tone::playsNotes(layer.toneMode) && !layer_follows(layerNum_);
}

void Controller::arp_start(uint8_t layerNum_, float velocity_, int8_t note_, uint16_t lengthIntervals_, uint8_t beatFlags_) {
    /// @brief Starts an arpeggio on `note_` (from layer_playBeat). It lasts as long as the note
    /// would: until the key is released (kPlayHold), the beat's length (sequencer), or a quarter
    /// note (a plain pad tap or an audition). The first arp note plays on the next sequencer
    /// tick (at most ~2 ms later), so every arp note comes from one place: arp_tick().

    ArpRun r;
    r.active = true;
    r.held = (beatFlags_ & kPlayHold) != 0;
    r.fromSequencer = !r.held && (lengthIntervals_ > 0);
    r.base = note_;
    r.velocity = velocity_;
    r.flags = static_cast<uint8_t>(beatFlags_ & kPlayVelocity);
    r.step = 0;
    r.countdown = 0;
    // Held: until the key is let go, but 60 s at most (like a held synth note), so a lost
    // release can never leave an arpeggio running forever.
    const uint32_t heldCap = (playTimerPeriod > 0) ? (60000000u / playTimerPeriod) : 60000u;
    r.remaining = r.held ? heldCap : ((lengthIntervals_ > 0) ? lengthIntervals_ : static_cast<uint32_t>(kMeasureInterval));
    r.gateLeft = 0;

    const uint32_t primask = __get_PRIMASK();  // pads call this from the main loop, the sequencer from its ISR
    __disable_irq();
    arpRun[layerNum_] = r;
    __set_PRIMASK(primask);
}

void Controller::arp_releaseVoices(uint8_t layerNum_) {
    /// @brief Ends the sounding note of a layer and of every layer that FOLLOWs it.
    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        if ((l != layerNum_) && (layerLibrary[l].followLayer != static_cast<int8_t>(layerNum_))) continue;
        if (layerLibrary[l].toneMode == tone::kModeSynth) {
            synthReleasePending[l] = true;
        } else if (layerLibrary[l].toneMode == tone::kModeTone) {
            toneReleasePending[l] = true;
        }
    }
}

void Controller::arp_stop(uint8_t layerNum_) {
    /// @brief Ends a running arpeggio; its last note fades out.

    if (layerNum_ >= kLayerLibrarySize) return;
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    const bool wasActive = arpRun[layerNum_].active;
    arpRun[layerNum_].active = false;
    arpRun[layerNum_].held = false;
    __set_PRIMASK(primask);
    if (wasActive) arp_releaseVoices(layerNum_);
}

void Controller::arp_tick() {
    /// @brief Sequencer timer ISR, every interval (800 per quarter note), also while stopped:
    /// plays the next note of every running arpeggio, through layer_playBeat so harmony layers
    /// follow it. A note lasts GATE % of a step (TONE layers are faded out here).

    for (uint8_t l = 0; l < kLayerLibrarySize; l++) {
        ArpRun& r = arpRun[l];
        if (!r.active) continue;
        const Layer& layer = layerLibrary[l];

        // ARP switched off, the layer changed mode, or the transport stopped a sequencer note.
        if (!arp_wants(l) || (r.fromSequencer && !playActive)) {
            r.active = false;
            r.held = false;
            arp_releaseVoices(l);
            continue;
        }

        if ((r.gateLeft > 0) && (--r.gateLeft == 0)) arp_releaseVoices(l);

        if (r.remaining == 0) {
            r.active = false;
            r.held = false;
            if (r.gateLeft > 0) arp_releaseVoices(l);
            continue;
        }
        r.remaining--;

        if (r.countdown > 0) {
            r.countdown--;
            continue;
        }

        const arp::Settings s = arp::sanitize(layer.arp);
        const uint8_t scale = tone::safeScale(layerLibrary[layer_keyLayer(l)].toneScale);
        const uint8_t size = tone::kScales[scale].size;
        arpRandom = (arpRandom * 1664525u) + 1013904223u;
        int note = r.base + arp::noteAt(s, r.step, size, arpRandom >> 8);
        r.step++;
        while ((size > 0) && (note > tone::maxDegree(scale))) note -= size;  // fold back into range
        const uint16_t length = arp::noteLength(s);
        if (!layer.mute) layer_playBeat(l, r.velocity, static_cast<int8_t>(note), length, static_cast<uint8_t>(kPlayArp | kPlayHeld | r.flags));
        r.countdown = static_cast<uint16_t>(arp::kRateIntervals[s.rate] - 1);
        r.gateLeft = (layer.toneMode == tone::kModeTone) ? length : 0;
    }
}

void Controller::arp_change(EncoderFunc function_, bool up_) {
    /// @brief Handles an ARP page knob.

    if (selectedLayerNum == -1) return;
    arp::Settings& s = layerLibrary[selectedLayerNum].arp;
    uint8_t* value = nullptr;
    uint8_t lo = 0, hi = 0, step = 1;
    uint8_t slot = 0;
    switch (function_) {
        case ENC_ARP_MODE:
            value = &s.mode, lo = 0, hi = arp::kModeCount - 1, slot = 0;
            break;
        case ENC_ARP_RATE:
            value = &s.rate, lo = 0, hi = arp::kRateCount - 1, slot = 1;
            break;
        case ENC_ARP_OCTAVES:
            value = &s.octaves, lo = 1, hi = arp::kMaxOctaves, slot = 4;
            break;
        case ENC_ARP_CHORD:
            value = &s.chord, lo = 0, hi = arp::kChordCount - 1, slot = 5;
            break;
        case ENC_ARP_GATE:
            value = &s.gate, lo = arp::kMinGate, hi = arp::kMaxGate, step = 5, slot = 6;
            break;
        default:
            return;
    }
    s = arp::sanitize(s);  // one byte each: read atomically by the sequencer ISR
    if (up_) {
        if (*value >= hi) return;
        *value = static_cast<uint8_t>(((*value + step) > hi) ? hi : (*value + step));
    } else {
        if (*value <= lo) return;
        *value = static_cast<uint8_t>(((*value - step) < lo) ? lo : (*value - step));
    }
    lcd_drawArpData(slot);
    lcd_drawArpStrip();
}

void Controller::arp_off() {
    /// @brief CLEAR tap on the ARP page: the arpeggiator off (the other settings are kept).

    if (selectedLayerNum == -1) return;
    layerLibrary[selectedLayerNum].arp.mode = arp::OFF;
    arp_stop(static_cast<uint8_t>(selectedLayerNum));
    lcd_drawArpData(0);
    lcd_drawArpStrip();
}

void Controller::note_setSlide(bool slide_) {
    /// @brief ADD + UP / DOWN on the NOTE page: SLIDE on / off for the selected beat (the note
    /// slides in from the previous one; on SYNTH layers without restarting the envelopes).

    // ADD is being used as shift here, so its pending tap and hold (melody) are cancelled.
    if (noteMelodyKeyFlag) {
        stopLongKeyTimer();
        keyboard.longKeyCounter = 0;
        noteMelodyKeyFlag = false;
    }
    noteMelodyLongFlag = false;
    layerSongAddPending = false;

    if (selectedLayerNum == -1) return;
    Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
    if ((selectedBeatNum < 0) || (selectedBeatNum > bank.lastActiveBeatNum)) return;
    Beat& beat = bank.beatLibrary[selectedBeatNum];
    if (beat.getSlide() == slide_) return;
    beat.setSlide(slide_);
    lcd_redrawNotePage();
}

void Controller::note_shiftKeys(bool up_) {
    /// @brief UP/DOWN on the NOTE page: moves the pad keyboard one octave of the scale.

    if (up_ && (noteKeysOctave < tone::kMaxKeysOctave)) {
        noteKeysOctave++;
    } else if (!up_ && (noteKeysOctave > tone::kMinKeysOctave)) {
        noteKeysOctave--;
    } else {
        return;
    }
    lcd_drawNoteStrip();  // shows the keyboard range
}

void Controller::note_melody(bool fresh_) {
    /// @brief ADD on the NOTE page: hold = a new melody over the bank's beats, tap = a small
    /// variation of the current one. CLEAR undoes both (back to the notes the page opened with).

    if (selectedLayerNum == -1) return;
    Layer& layer = layerLibrary[selectedLayerNum];
    Bank& bank = layer.bankLibrary[activeSongBank];

    int8_t notes[kBeatLibrarySize];
    uint8_t count = 0;
    for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
        if (bank.beatLibrary[i].getActive()) notes[count++] = bank.beatLibrary[i].getNote();
    }
    if (count == 0) return;

    uint32_t seed = DWT_CYCCNT ^ (HAL_GetTick() << 16);  // a different result on every press
    if (fresh_) {
        tone::generateMelody(notes, count, layer.toneScale, seed);
    } else {
        tone::mutateMelody(notes, count, layer.toneScale, seed);
    }

    count = 0;
    for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
        if (bank.beatLibrary[i].getActive()) bank.beatLibrary[i].setNote(notes[count++]);
    }
    lcd_redrawNotePage();
    note_audition();
}

void Controller::tone_setGlide(uint8_t layerNum_, uint8_t glide_) {
    /// @brief Sets GLIDE (0..100) and its precomputed per-block coefficient.

    Layer& layer = layerLibrary[layerNum_];
    layer.toneGlide = (glide_ > tone::kMaxGlide) ? tone::kMaxGlide : glide_;
    layer.toneGlideCoef = tone::glideBlockCoef(layer.toneGlide, kAudioBlocksPerSecond);
}

void Controller::tone_resetLayer(uint8_t layerNum_) {
    /// @brief Back to a plain DRUM layer with default ROOT/SCALE/GLIDE (notes live in the beats
    /// and are reset with them). A note still held on the MIDI output is released.

    Layer& layer = layerLibrary[layerNum_];
    if (layer.toneMode == tone::kModeSynth) synth_silence(layerNum_);
    layer.toneMode = tone::kModeDrum;
    layer.toneRoot = 0;
    layer.toneScale = tone::kDefaultScale;
    layer.synth = synth::Params{};
    layer.followLayer = harmony::kNone;  // v2.04
    layer.harmonyInterval = harmony::THIRD_UP;
    arp_stop(layerNum_);  // v2.08
    layer.arp = arp::Settings{};
    tone_setGlide(layerNum_, 0);
    layerPlayData[layerNum_].toneLastIncrement = 0.0f;
    if (toneMidiSounding[layerNum_] != 0xFF) {
        uint8_t off[3] = {(uint8_t)(0x80 + io.midi.txChannel), toneMidiSounding[layerNum_], 0x64};
        enqueueMidiTx(off, 3);
        toneMidiSounding[layerNum_] = 0xFF;
    }
}

void Controller::tone_setMode(uint8_t layerNum_, uint8_t mode_) {
    /// @brief Switches a layer between DRUM / TONE / SYNTH without a click or a hanging note.
    /// ROOT, SCALE, GLIDE and the SYNTH sound are kept for the next time the mode comes back.

    if ((layerNum_ >= kLayerLibrarySize) || (mode_ >= tone::kModeCount)) return;
    Layer& layer = layerLibrary[layerNum_];
    if (layer.toneMode == mode_) return;
    if (layer.toneMode == tone::kModeSynth) synth_silence(layerNum_);  // leaving SYNTH
    if (mode_ == tone::kModeSynth) {
        // Entering SYNTH: a sample that is still sounding stops (the voice replaces it).
        const uint32_t primask = __get_PRIMASK();
        __disable_irq();
        layerPlayData[layerNum_].activeBeatPlayData.active = false;
        layerPlayData[layerNum_].activeBeatPlayData.newBeatFlag = false;
        __set_PRIMASK(primask);
    }
    if (mode_ == tone::kModeDrum) {
        const uint8_t root = layer.toneRoot, scale = layer.toneScale, glide = layer.toneGlide;
        const synth::Params sound = layer.synth;
        const int8_t follow = layer.followLayer;
        const uint8_t interval = layer.harmonyInterval;
        const arp::Settings arpSettings = layer.arp;  // v2.08: kept for TONE / SYNTH, like FOLLOW
        tone_resetLayer(layerNum_);  // releases a held MIDI note
        layer.arp = arpSettings;
        layer.toneRoot = root;       // ...but keeps the settings for next time
        layer.toneScale = scale;
        layer.synth = sound;
        layer.followLayer = follow;  // v2.04: inactive on a DRUM layer, back with TONE / SYNTH
        layer.harmonyInterval = interval;
        tone_setGlide(layerNum_, glide);
    }
    layer.toneMode = mode_;
    layerPlayData[layerNum_].toneLastIncrement = 0.0f;
    lcd_drawLayerTab_InstData(layerNum_);  // track list: sample name <-> synth sound name
}

void Controller::tone_resetAll() {
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) tone_resetLayer(i);
}

void Controller::note_change(EncoderFunc function_, bool up_) {
    /// @brief Handles a NOTE page knob.

    if (selectedLayerNum == -1) return;
    Layer& layer = layerLibrary[selectedLayerNum];
    Bank& bank = layer.bankLibrary[activeSongBank];
    const bool beatValid = (selectedBeatNum >= 0) && (selectedBeatNum <= bank.lastActiveBeatNum);

    switch (function_) {
        case ENC_NOTE_MODE: {
            // DRUM -> TONE -> SYNTH (and back).
            if (up_ && (layer.toneMode >= tone::kModeCount - 1)) return;
            if (!up_ && (layer.toneMode == tone::kModeDrum)) return;
            tone_setMode(static_cast<uint8_t>(selectedLayerNum), static_cast<uint8_t>(up_ ? layer.toneMode + 1 : layer.toneMode - 1));
            lcd_drawNoteData(0);
            lcd_redrawNotePage();
            note_audition();
            return;
        }

        case ENC_NOTE_NOTE: {
            if (!beatValid) return;
            Beat& beat = bank.beatLibrary[selectedBeatNum];
            const int8_t current = tone::clampDegree(beat.getNote(), layer.toneScale);
            const int8_t next = tone::clampDegree(current + (up_ ? 1 : -1), layer.toneScale);
            if (next == beat.getNote()) return;
            beat.setNote(next);  // one byte: read atomically by the sequencer ISR
            break;
        }

        case ENC_NOTE_ROOT:
            layer.toneRoot = up_ ? static_cast<uint8_t>((layer.toneRoot + 1) % tone::kRootCount)
                                 : static_cast<uint8_t>((layer.toneRoot + tone::kRootCount - 1) % tone::kRootCount);
            lcd_drawNoteData(4);
            break;

        case ENC_NOTE_SCALE:
            // Notes are kept as they are; a degree outside a smaller scale's range is clamped
            // when played, so browsing scales never loses a melody.
            if (up_ && (layer.toneScale < tone::kScaleCount - 1)) {
                layer.toneScale++;
            } else if (!up_ && (layer.toneScale > 0)) {
                layer.toneScale--;
            } else {
                return;
            }
            lcd_drawNoteData(5);
            break;

        case ENC_NOTE_GLIDE:
            if (up_ && (layer.toneGlide < tone::kMaxGlide)) {
                tone_setGlide(selectedLayerNum, layer.toneGlide + 1);
            } else if (!up_ && (layer.toneGlide > 0)) {
                tone_setGlide(selectedLayerNum, layer.toneGlide - 1);
            } else {
                return;
            }
            lcd_drawNoteData(6);
            return;

        case ENC_NOTE_FOLLOW: {
            // v2.04 harmony: knob 4 = which layer to follow (OFF, L1..L10 but not itself);
            // ADD + knob 4 = the interval. ADD is a modifier here: no melody on release.
            if (keyboard.shiftKeyPress) {
                if (noteMelodyKeyFlag) {
                    stopLongKeyTimer();
                    keyboard.longKeyCounter = 0;
                    noteMelodyKeyFlag = false;
                }
                noteMelodyLongFlag = false;
                layerSongAddPending = false;
                const uint8_t i = harmony::safeInterval(layer.harmonyInterval);
                layer.harmonyInterval = up_ ? static_cast<uint8_t>((i + 1) % harmony::kIntervalCount)
                                            : static_cast<uint8_t>((i + harmony::kIntervalCount - 1) % harmony::kIntervalCount);
            } else {
                const int8_t next = harmony::stepFollow(layer.followLayer, up_, static_cast<uint8_t>(selectedLayerNum), kLayerLibrarySize);
                if (next == layer.followLayer) return;
                if (layer.followLayer < 0) layer.harmonyInterval = harmony::safeInterval(layer.harmonyInterval);
                layer.followLayer = next;  // one byte: read atomically by the sequencer ISR
                if (layer.toneMode == tone::kModeSynth) synth_silence(static_cast<uint8_t>(selectedLayerNum));  // no hanging note from its own beats
            }
            lcd_drawNoteData(1);  // the beat's own note is greyed while following
            lcd_drawNoteStrip();  // shows FOLLOW and the notes it will play
            return;
        }

        case ENC_NOTE_TRANSPOSE: {
            if (bank.lastActiveBeatNum == -1) return;
            const int delta = up_ ? 1 : -1;

            // Move the melody as a whole (ToneScale.h): at the edge of the range it stops
            // instead of squashing notes together.
            int8_t notes[kBeatLibrarySize];
            uint8_t count = 0;
            for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                if (bank.beatLibrary[i].getActive()) notes[count++] = bank.beatLibrary[i].getNote();
            }
            if (!tone::transposeAll(notes, count, delta, layer.toneScale)) return;
            count = 0;
            for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                if (bank.beatLibrary[i].getActive()) bank.beatLibrary[i].setNote(notes[count++]);
            }

            if ((noteTransposeTotal + delta >= -99) && (noteTransposeTotal + delta <= 99)) noteTransposeTotal += delta;
            lcd_drawNoteData(7);
            break;
        }

        default:
            return;
    }

    // NOTE, ROOT, SCALE and TRANSPOSE change the note names and the strip.
    lcd_redrawNotePage();
    note_audition();
}

void Controller::layerSong_reset(uint8_t layerNum_, uint8_t bankNum_) {
    /// @brief Clears all beats for the target layer song bank.

    // 1. BANK RESET DISPATCH
    // Forward request to full beat-reset helper.
    layerSong_resetAllBeats(layerNum_, bankNum_);
}

void Controller::layerSong_menuRight() {
    /// @brief Advances Layer Song tab focus to the right with wrap-around/skip logic.

    // 1. TAB STATE UPDATE
    // Save previous tab and advance with hidden-tab skip.
    preMenuTab = menuTab;
    if (menuTab < 5)
        (menuTab == 1) ? menuTab = 4 : menuTab += 1;
    else
        menuTab = kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation.
    lcd_transitionSelect();
}

void Controller::layerSong_menuLeft() {
    /// @brief Moves Layer Song tab focus to the left with wrap-around/skip logic.

    // 1. TAB STATE UPDATE
    // Save previous tab and decrement with hidden-tab skip.
    preMenuTab = menuTab;
    if (menuTab > kMinMenu8Tab)
        (menuTab == 4) ? menuTab = 1 : menuTab -= 1;
    else
        menuTab = 5;

    // 2. UI TRANSITION
    // Trigger selection transition animation.
    lcd_transitionSelect();
}

void Controller::layerSong_menuUp(uint8_t tab_) {
    /// @brief Increments Layer Song fill parameters based on active tab mapping.

    Layer& layer = layerLibrary[selectedLayerNum];

    // 1. ACTIVE BEAT VALIDATION
    // Continue only when current bank contains active beats.
    if (layer.bankLibrary[activeSongBank].lastActiveBeatNum != -1) {
        Beat& beat = layer.bankLibrary[activeSongBank].beatLibrary[selectedBeatNum];
        uint8_t fillType = beat.getFillType();
        uint8_t fillPattern = beat.getFillPattern();
        uint8_t fillTime = beat.getFillTime();
        uint8_t fillLevel = beat.getFillLevel();

        // 2. TAB-BASED PARAMETER DISPATCH
        // Route encoder-up action to fill type/pattern/time/level target.
        switch (tab_) {
            case 0:
                (fillType < kMaxLayerFillType) ? fillType += 1 : fillType = kMinLayerFillType;
                layerSong_setSelectedBeatFill(fillType, 0, fillTime, fillLevel);
                break;

            case 1:
                (fillPattern < kMaxLayerPatternType) ? fillPattern += 1 : fillPattern = kMinLayerPatternType;
                layerSong_setSelectedBeatFill(fillType, fillPattern, fillTime, fillLevel);
                break;

            case 4:
                // Update time mode for complex fills.
                if ((fillType > 0) || ((fillType == 0) && (fillPattern > 2))) {
                    (fillTime < kMaxFillTime) ? fillTime += 1 : fillTime = kMinFillTime;
                    layerSong_setSelectedBeatFillTime(fillTime);
                }
                break;

            case 5:
                // Update level mode for complex fills.
                if ((fillType > 0) || ((fillType == 0) && (fillPattern > 2))) {
                    (fillLevel < kMaxFillLevel) ? fillLevel += 1 : fillLevel = kMinFillLevel;
                    layerSong_setSelectedBeatFillLevel(fillLevel);
                }
                break;
        }
    }
}

void Controller::layerSong_menuDown(uint8_t tab_) {
    /// @brief Decrements Layer Song fill parameters based on active tab mapping.

    Layer& layer = layerLibrary[selectedLayerNum];

    // 1. ACTIVE BEAT VALIDATION
    // Continue only when current bank contains active beats.
    if (layer.bankLibrary[activeSongBank].lastActiveBeatNum != -1) {
        Beat& beat = layer.bankLibrary[activeSongBank].beatLibrary[selectedBeatNum];
        uint8_t fillType = beat.getFillType();
        uint8_t fillPattern = beat.getFillPattern();
        uint8_t fillTime = beat.getFillTime();
        uint8_t fillLevel = beat.getFillLevel();

        // 2. TAB-BASED PARAMETER DISPATCH
        // Route encoder-down action to fill type/time/level target.
        switch (tab_) {
            case 0:
                (fillType > kMinLayerFillType) ? fillType -= 1 : fillType = kMaxLayerFillType;
                layerSong_setSelectedBeatFill(fillType, 0, fillTime, fillLevel);
                break;

            case 1:
                (fillPattern > kMinLayerPatternType) ? fillPattern -= 1 : fillPattern = kMaxLayerPatternType;
                layerSong_setSelectedBeatFill(fillType, fillPattern, fillTime, fillLevel);
                break;

            case 4:
                // Update time mode for complex fills.
                if ((fillType > 0) || ((fillType == 0) && (fillPattern > 2))) {
                    (fillTime > kMinFillTime) ? fillTime -= 1 : fillTime = kMaxFillTime;
                    layerSong_setSelectedBeatFillTime(fillTime);
                }
                break;

            case 5:
                // Update level mode for complex fills.
                if ((fillType > 0) || ((fillType == 0) && (fillPattern > 2))) {
                    (fillLevel > kMinFillLevel) ? fillLevel -= 1 : fillLevel = kMaxFillLevel;
                    layerSong_setSelectedBeatFillLevel(fillLevel);
                }
                break;
        }
    }
}

void Controller::layerSong_beatRight() {
    /// @brief Moves beat selection right and refreshes related Layer Song UI fields.

    Layer& layer = layerLibrary[selectedLayerNum];

    // 1. BEAT COUNT VALIDATION
    // Continue only when more than one active beat exists.
    if (layer.bankLibrary[activeSongBank].lastActiveBeatNum > 0) {
        // 2. SELECTION UPDATE
        // Deselect current beat, move index right with wrap, select new beat.
        lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, false);
        (selectedBeatNum < layer.bankLibrary[activeSongBank].lastActiveBeatNum) ? selectedBeatNum += 1 : selectedBeatNum = 0;
        lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, true);

        // 3. UI DATA REFRESH
        // Redraw beat fill parameter widgets for selected beat (or the NOTE page).
        lcd_redrawNotePage();
        lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);

        /* Debug
        Beat &beatSelected = layer.bankLibrary[activeBankTab].beatLibrary[selectedBeatNum];
        check(selectedBeatNum, 0, 0);
        check(beatSelected.getStartInterval(), 0, 1);
        check(beatSelected.getEndInterval(), 0, 2);
        check(beatSelected.getFillType(), 0, 3);
        check(beatSelected.getFillTarget(), 0, 4);
        if (beatSelected.getFillType() >= 3) {
            BeatMicro &beatMicroX = layer.bankLibrary[activeBankTab].beatFillLibrary[beatSelected.getFillTarget()].beatMicroLibrary[0];
            check(beatMicroX.getInterval(), 0, 5);
        }
        */
    }
}

void Controller::layerSong_beatLeft() {
    /// @brief Moves beat selection left and refreshes related Layer Song UI fields.

    Layer& layer = layerLibrary[selectedLayerNum];

    // 1. BEAT COUNT VALIDATION
    // Continue only when more than one active beat exists.
    if (layer.bankLibrary[activeSongBank].lastActiveBeatNum > 0) {
        // 2. SELECTION UPDATE
        // Deselect current beat, move index left with wrap, select new beat.
        lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, false);
        (selectedBeatNum > 0) ? selectedBeatNum -= 1 : selectedBeatNum = layer.bankLibrary[activeSongBank].lastActiveBeatNum;
        lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, true);

        // 3. UI DATA REFRESH
        // Redraw beat fill parameter widgets for selected beat (or the NOTE page).
        lcd_redrawNotePage();
        lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);

        /* Debug
        Beat &beatSelected = layer.bankLibrary[activeBankTab].beatLibrary[selectedBeatNum];
        check(selectedBeatNum, 0, 0);
        check(beatSelected.getStartInterval(), 0, 1);
        check(beatSelected.getEndInterval(), 0, 2);
        check(beatSelected.getFillType(), 0, 3);
        check(beatSelected.getFillTarget(), 0, 4);
        if (beatSelected.getFillType() >= 3) {
            BeatMicro &beatMicroX = layer.bankLibrary[activeBankTab].beatFillLibrary[beatSelected.getFillTarget()].beatMicroLibrary[0];
            check(beatMicroX.getInterval(), 0, 5);
        }
        */
    }
}

uint16_t Controller::layerSong_quantizeStart(uint16_t interval_) const {
    /// @brief Where a beat recorded at interval_ lands: the nearest grid line (Rhythm QUANTIZE);
    /// the end of the song wraps to its start.

    const uint16_t quantizeInterval = kQuantizeInterval[rhythm.quantize];
    uint16_t startInterval = ((interval_ / quantizeInterval) + ((interval_ % quantizeInterval) / (quantizeInterval / 2))) * quantizeInterval;
    if (startInterval == songInterval) startInterval = 0;
    return startInterval;
}

void Controller::layerSong_setBeat(uint8_t layerNum_, uint8_t bankTab_, uint16_t interval_,uint8_t fillType_, uint8_t fillPattern_, uint8_t fillTime_, uint8_t fillLevel_, bool selected_, bool hasNote_, int8_t note_) {
    /// @brief Inserts a beat into target bank with quantization, ordering, and UI sync.

    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];

    // 1. CAPACITY & QUANTIZE PREPARATION
    // Validate beat capacity and quantize incoming interval.
    if (bank.lastActiveBeatNum < kBeatLibrarySize) {
        // Quantize incoming interval using nearest-grid rounding; the song end wraps to 0.
        const uint16_t startInterval = layerSong_quantizeStart(interval_);
        uint16_t endInterval;

        // 3. INSERTION INDEX RESOLUTION
        // Detect duplicates and compute sorted insertion index.
        bool duplicate = false;
        uint8_t beatNum;
        if (bank.lastActiveBeatNum == -1) {
            beatNum = 0;
            endInterval = songInterval;
        } else {
            beatNum = 0;
            for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                // Check for duplicate start interval.
                if (bank.beatLibrary[i].getStartInterval() == startInterval) {
                    duplicate = true;
                    // Recording a note over an existing beat replaces its note (overdub).
                    if (hasNote_) bank.beatLibrary[i].setNote(note_);
                    break;
                }

                // Find insertion point.
                else if (bank.beatLibrary[i].getStartInterval() < startInterval) {
                    beatNum = i + 1;
                }

                // Stop searching once upper bound is found.
                else {
                    beatNum = i;
                    break;
                }
            }
            if (!duplicate) {
                // Shift beats to make room for insertion.
                if (beatNum <= bank.lastActiveBeatNum) {
                    for (int8_t j = bank.lastActiveBeatNum; j >= beatNum; j--) {
                        bank.beatLibrary[j + 1] = bank.beatLibrary[j];
                    }
                    bank.beatLibrary[beatNum].setFillTarget(NULL_FILL_TARGET);
                    bank.beatLibrary[beatNum].setFillType(0);
                    bank.beatLibrary[beatNum].setFillPattern(0);
                }

                // Adjust previous beat duration and redraw if needed.
                if (beatNum > 0) {
                    bank.beatLibrary[beatNum - 1].setEndInterval(startInterval);
                    bank.setBeatFill(beatNum - 1, 0, 0, 0, 0);
                    bool sel;

                    // Keep previous beat highlight only if it was selected in active menu.
                    ((menu == LAYER_SONG_MENU) && (selectedLayerNum == layerNum_) && (beatNum - 1 == selectedBeatNum)) ? sel = true : sel = false;
                    lcd_drawBeat(layerNum_, bankTab_, beatNum - 1, sel);
                }

                // Resolve end interval for inserted beat.
                if (beatNum == (bank.lastActiveBeatNum + 1)) {
                    endInterval = songInterval;
                } else {
                    endInterval = bank.beatLibrary[beatNum + 1].getStartInterval();
                }
            }
        }

        // 4. BEAT WRITE & PLAYBACK UPDATE
        // Commit beat, recalculate active/play indices, and refresh draw state.
        if (!duplicate) {
            bank.setBeat(beatNum, startInterval, endInterval);
            bank.setBeatFill(beatNum, fillType_, fillPattern_, fillTime_, fillLevel_);
            if (hasNote_) bank.beatLibrary[beatNum].setNote(note_);
            layerSong_calculateLastActiveBeatNum(layerNum_, bankTab_);
            layerSong_calculatePlayBeatNum(layerNum_, bankTab_, playInterval);
            if ((menu == LAYER_SONG_MENU) && (selectedLayerNum == layerNum_) && (selected_)) {
                // Shift current selection right if insertion occurred before it.
                if (selectedBeatNum >= beatNum) selectedBeatNum += 1;
                if (selectedBeatNum != -1) lcd_drawBeat(layerNum_, bankTab_, selectedBeatNum, false);
                selectedBeatNum = beatNum;
                lcd_drawBeat(layerNum_, bankTab_, selectedBeatNum, true);
                lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
            } else {
                lcd_drawBeat(layerNum_, bankTab_, beatNum, false);
            }

            // Prevent double-trigger when play pointer equals inserted beat.
            int8_t playBeatNum = bank.playBeatNum;
            if (playBeatNum == beatNum) {
                (playBeatNum < bank.lastActiveBeatNum) ? bank.playBeatNum += 1 : bank.playBeatNum = 0;
            }
        }
    }
}

void Controller::layerSong_setSelectedBeatFill(uint8_t fillType_, uint8_t fillPattern_, uint8_t fillTime_, uint8_t fillLevel_) {
    /// @brief Applies fill type/pattern/time/level to selected beat and refreshes UI.

    // 1. SIMPLE FILL NORMALIZATION
    // Reset time/level modifiers for simple fill modes.
    /*
    if (fillType_ == 0) {
        fillTime_ = 0;
        fillLevel_ = 0;
    }
    */

    // 2. BEAT FILL RE-CALCULATION
    // Recompute selected beat fill data and playback cursor.
    layerSong_calculateBeatFill(selectedLayerNum, activeSongBank, selectedBeatNum, fillType_, fillPattern_, fillTime_, fillLevel_);
    layerSong_calculatePlayBeatNum(selectedLayerNum, activeSongBank, playInterval);

    // 3. UI REFRESH
    // Redraw beat fill widgets and selected beat marker.
    lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
    lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, true);
}

void Controller::layerSong_setSelectedBeatFillTime(uint8_t fillTime_) {
    /// @brief Updates selected beat fill-time mode and refreshes dependent UI.

    // 1. TIME MODE RE-CALCULATION
    // Recompute selected beat fill-time profile and play cursor.
    layerSong_calculateBeatFillTime(selectedLayerNum, activeSongBank, selectedBeatNum, fillTime_);
    layerSong_calculatePlayBeatNum(selectedLayerNum, activeSongBank, playInterval);

    // 2. UI REFRESH
    // Redraw fill-time and graph widgets.
    lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
}

void Controller::layerSong_setSelectedBeatFillLevel(uint8_t fillLevel_) {
    /// @brief Updates selected beat fill-level mode and refreshes dependent UI.

    // 1. LEVEL MODE RE-CALCULATION
    // Recompute selected beat fill-level profile and play cursor.
    layerSong_calculateBeatFillLevel(selectedLayerNum, activeSongBank, selectedBeatNum, fillLevel_);
    layerSong_calculatePlayBeatNum(selectedLayerNum, activeSongBank, playInterval);

    // 2. UI REFRESH
    // Redraw fill-level and graph widgets.
    lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
}

void Controller::layerSong_generateBeat(uint8_t type) {
    /// @brief Generates predefined beat layouts for the active layer song bank.

    uint16_t interval;

    // 1. GENERATION TYPE DISPATCH
    // Build beat sequence according to selected generation preset.
    switch (type) {
        case 0:

            // Generate one beat at each bar boundary.
            for (int8_t i = (rhythm.bar - 1); i >= 0; i--) {
                interval = i * barInterval;
                layerSong_setBeat(selectedLayerNum, activeSongBank, interval, 0, 0, 0, 0, true);
            }
            break;

        case 1:

            // Generate off-beat pattern type-1.
            if (rhythm.measure > 1) {
                if (rhythm.measure % 2) {
                    for (int8_t i = (rhythm.measureTotal - 1); i >= 0; i--) {
                        interval = i * measureInterval;
                        if (interval % barInterval) layerSong_setBeat(selectedLayerNum, activeSongBank, i * measureInterval, 0, 0, 0, 0, false);
                    }
                } else {
                    for (int8_t i = (rhythm.bar - 1); i >= 0; i--) {
                        interval = i * barInterval + (barInterval / 2);
                        if (interval % barInterval) {
                            layerSong_setBeat(selectedLayerNum, activeSongBank, interval, 0, 0, 0, 0, false);
                        }
                    }
                }
            }
            break;

        case 2:

            // Generate off-beat pattern type-2.
            if (rhythm.measure > 1) {
                if (rhythm.measure % 2 == 0) {
                    for (int8_t i = (rhythm.measureTotal - 1); i >= 0; i--) {
                        interval = i * measureInterval;
                        if (interval % (barInterval / 2) != 0) {
                            layerSong_setBeat(selectedLayerNum, activeSongBank, i * measureInterval, 0, 0, 0, 0, false);
                        }
                    }
                }
            }
            break;

        case 3: {
            // Apply BASIC_4 fill to all existing beats (4 evenly-spaced hits per beat).
            Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
            if (bank.lastActiveBeatNum != -1) {
                for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                    uint16_t endInterval = (i < bank.lastActiveBeatNum) ? bank.beatLibrary[i + 1].getStartInterval() : songInterval;
                    bank.beatLibrary[i].setEndInterval(endInterval);
                    bank.setBeatFill(i, 0, 5, 0, 0);  // fillType=0 BASIC, fillPattern=5 BASIC_4
                }
            }
        } break;

        case 4: {
            // Apply BASIC_8 fill to all existing beats (8 evenly-spaced hits per beat).
            Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
            if (bank.lastActiveBeatNum != -1) {
                for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                    uint16_t endInterval = (i < bank.lastActiveBeatNum) ? bank.beatLibrary[i + 1].getStartInterval() : songInterval;
                    bank.beatLibrary[i].setEndInterval(endInterval);
                    bank.setBeatFill(i, 0, 7, 0, 0);  // fillType=0 BASIC, fillPattern=7 BASIC_8
                }
            }
        } break;
    }

    // 2. UI REFRESH
    // Redraw fill widgets when Layer Song menu is visible.
    if (menu == LAYER_SONG_MENU) {
        lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
        lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
    }

    // 3. PLAYBACK CURSOR UPDATE
    // Recompute play beat index after generation changes.
    layerSong_calculatePlayBeatNum(selectedLayerNum, activeSongBank, playInterval);
}

void Controller::layerSong_shiftSelectedBeat(bool direction_) {
    /// @brief Shifts selected beat by one quantize step (ADD + LEFT/RIGHT).
    /// Kept for compatibility; uses the same logic as the MOVE encoder, which keeps fills
    /// and can pass neighbouring beats (the old version stopped at neighbours and cleared fills).

    layerSong_moveSelectedBeat(direction_);
}

////////////////////////////////////////////////////////////////////////////////
/* Layer Song Edit Functions (MOVE / SHIFT ALL / single-beat ADD) ------------*/
////////////////////////////////////////////////////////////////////////////////

namespace {

// Masks the sequencer tick (TIM15 -> interruptPlay) while a bank is rewritten. That ISR
// reads the beat list to trigger notes and also writes to it while recording, so a
// half-rebuilt bank must never be visible to it. The previous enable state is restored,
// so this is safe whether or not the transport is running. A tick that arrives while
// masked stays pending and runs right after (a few microseconds of jitter).
struct SequencerIrqGuard {
    const bool wasEnabled;
    SequencerIrqGuard() : wasEnabled(NVIC_GetEnableIRQ(TIM15_IRQn) != 0) {
        NVIC_DisableIRQ(TIM15_IRQn);
        __DSB();
        __ISB();
    }
    ~SequencerIrqGuard() {
        if (wasEnabled) NVIC_EnableIRQ(TIM15_IRQn);
    }
    SequencerIrqGuard(const SequencerIrqGuard&) = delete;
    SequencerIrqGuard& operator=(const SequencerIrqGuard&) = delete;
};

uint16_t quantizeStep(uint8_t quantize_) {
    return static_cast<uint16_t>(kQuantizeInterval[quantize_]);
}

}  // namespace

uint8_t Controller::layerSong_snapshotBank(uint8_t layerNum_, uint8_t bankTab_, BeatSnapshot* beats_) {
    /// @brief Copies the active beats of a bank (position + fill settings) into beats_.
    /// Returns the number of beats copied. beats_ must hold kBeatLibrarySize entries.

    Bank& bank = layerLibrary[layerNum_].bankLibrary[bankTab_];
    uint8_t count = 0;
    for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
        const Beat& beat = bank.beatLibrary[i];
        if (!beat.getActive()) continue;
        beats_[count].startInterval = beat.getStartInterval();
        beats_[count].fillType = beat.getFillType();
        beats_[count].fillPattern = beat.getFillPattern();
        beats_[count].fillTime = beat.getFillTime();
        beats_[count].fillLevel = beat.getFillLevel();
        beats_[count].note = beat.getNote();
        beats_[count].slide = beat.getSlide();
        beats_[count].hold = beat.getHold();
        count++;
    }
    return count;
}

int8_t Controller::layerSong_rebuildBank(uint8_t layerNum_, uint8_t bankTab_, BeatSnapshot* beats_, uint8_t count_, int8_t track_) {
    /// @brief Rewrites a bank from a list of beats in any order.
    /// Sorts by start interval, drops duplicate positions, recomputes every end interval and
    /// every fill (fills are stored with absolute positions, so they must be regenerated after
    /// a move), and updates the play pointer. Returns the new index of the entry that was at
    /// position track_ in beats_, or -1 if it was dropped / the bank is empty.

    Bank& bank = layerLibrary[layerNum_].bankLibrary[bankTab_];
    int8_t tracked = track_;

    // 1-2. SORT BY POSITION, DROP DUPLICATES, follow the tracked entry (LayerSongEdit.h).
    count_ = layersong::sortUnique(beats_, count_, tracked);
    if (count_ > kBeatLibrarySize) count_ = kBeatLibrarySize;

    // 3. REWRITE BANK with the sequencer tick masked.
    {
        SequencerIrqGuard guard;

        for (uint8_t i = 0; i < kBeatLibrarySize; i++) bank.resetBeat(i);
        for (uint8_t i = 0; i < kBeatFillLibrarySize; i++) bank.beatFillLibrary[i].reset();

        for (uint8_t i = 0; i < count_; i++) {
            const uint16_t endInterval = (i + 1 < count_) ? beats_[i + 1].startInterval : songInterval;
            bank.setBeat(i, beats_[i].startInterval, endInterval);
            bank.beatLibrary[i].setNote(beats_[i].note);
            bank.beatLibrary[i].setSlide(beats_[i].slide);
            bank.beatLibrary[i].setHold(beats_[i].hold);
        }
        for (uint8_t i = 0; i < count_; i++) {
            bank.setBeatFill(i, beats_[i].fillType, beats_[i].fillPattern, beats_[i].fillTime, beats_[i].fillLevel);
        }

        bank.lastActiveBeatNum = (count_ > 0) ? static_cast<int8_t>(count_ - 1) : -1;
        layerSong_calculatePlayBeatNum(layerNum_, bankTab_, playInterval);
    }

    return ((count_ > 0) && (tracked >= 0) && (tracked < count_)) ? tracked : -1;
}

void Controller::layerSong_redrawEdited() {
    /// @brief Redraws the selected layer's lane and the Layer Song fields after an edit.

    if ((menu != LAYER_SONG_MENU) || (selectedLayerNum == -1)) return;
    lcd_drawSong(selectedLayerNum, activeSongBank);
    lcd_redrawNotePage();
    lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);  // also refreshes MOVE position
    lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
    lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
}

void Controller::layerSong_moveSelectedBeat(bool direction_) {
    /// @brief MOVE: moves the selected beat to the next/previous quantize grid line.
    /// Off-grid beats (e.g. recorded live) snap to the grid on the first move. Positions that
    /// already hold a beat are skipped, so a beat can pass its neighbours. Fill settings are
    /// kept. The move stops at the start and end of the song (no wrap-around).

    if (selectedLayerNum == -1) return;
    Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
    if ((bank.lastActiveBeatNum == -1) || (selectedBeatNum < 0) || (selectedBeatNum > bank.lastActiveBeatNum)) return;

    BeatSnapshot beats[kBeatLibrarySize];
    const uint8_t count = layerSong_snapshotBank(selectedLayerNum, activeSongBank, beats);
    if (selectedBeatNum >= count) return;

    // 1. TARGET: next free grid line in the requested direction (LayerSongEdit.h).
    const int32_t target = layersong::moveTarget(beats, count, static_cast<uint8_t>(selectedBeatNum), direction_,
                                                 quantizeStep(rhythm.quantize), songInterval);
    if (target < 0) return;  // reached the song edge

    // 2. APPLY + REDRAW
    beats[selectedBeatNum].startInterval = static_cast<uint16_t>(target);
    selectedBeatNum = layerSong_rebuildBank(selectedLayerNum, activeSongBank, beats, count, selectedBeatNum);
    layerSong_redrawEdited();
}

void Controller::layerSong_shiftAllBeats(bool direction_) {
    /// @brief SHIFT ALL: rotates every beat of the active bank by one quantize step.
    /// Beats pushed past the end re-enter at the start (and vice versa), so the pattern is
    /// never lost and turning the other way restores it. Relative timing, including
    /// off-grid beats, and all fills are kept. The selection follows the selected beat.

    if (selectedLayerNum == -1) return;
    Bank& bank = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank];
    if ((bank.lastActiveBeatNum == -1) || (songInterval == 0)) return;

    BeatSnapshot beats[kBeatLibrarySize];
    const uint8_t count = layerSong_snapshotBank(selectedLayerNum, activeSongBank, beats);
    if (count == 0) return;

    if (!layersong::shiftAll(beats, count, direction_, quantizeStep(rhythm.quantize), songInterval)) return;

    const int8_t track = ((selectedBeatNum >= 0) && (selectedBeatNum < count)) ? selectedBeatNum : 0;
    selectedBeatNum = layerSong_rebuildBank(selectedLayerNum, activeSongBank, beats, count, track);
    layerSong_redrawEdited();
}

void Controller::layerSong_insertBeat() {
    /// @brief ADD tap: inserts one plain beat in the middle of the gap after the selected beat.
    /// The position is the quantize grid line closest to the middle that lies strictly inside
    /// the gap (after the last beat the gap wraps to the first beat). On an empty layer the
    /// beat goes to the start of the song. The new beat becomes the selection so it can be
    /// placed precisely with MOVE. Does nothing when the gap has no free grid line or the
    /// bank is full (64 beats).

    if (selectedLayerNum == -1) return;

    BeatSnapshot beats[kBeatLibrarySize];
    const uint8_t count = layerSong_snapshotBank(selectedLayerNum, activeSongBank, beats);
    if (count >= kBeatLibrarySize) return;

    // Sort defensively (banks written by older code could have the last beat out of order),
    // keeping track of the selected beat, then find the gap after it.
    int8_t sel = selectedBeatNum;
    const uint8_t sorted = layersong::sortUnique(beats, count, sel);
    const int32_t position = layersong::insertPosition(beats, sorted, sel, quantizeStep(rhythm.quantize), songInterval);
    if (position < 0) return;  // no free grid line in this gap

    beats[sorted].startInterval = static_cast<uint16_t>(position);
    beats[sorted].fillType = 0;
    beats[sorted].fillPattern = 0;
    beats[sorted].fillTime = 0;
    beats[sorted].fillLevel = 0;
    // The new beat continues the line: it takes the note of the beat it was inserted after.
    beats[sorted].note = ((sel >= 0) && (sel < static_cast<int8_t>(sorted))) ? beats[sel].note : 0;
    beats[sorted].slide = false;
    beats[sorted].hold = 0;

    selectedBeatNum = layerSong_rebuildBank(selectedLayerNum, activeSongBank, beats, sorted + 1, sorted);
    layerSong_redrawEdited();
}

void Controller::layerSong_resetSelectedBeat() {
    /// @brief Removes selected beat, compacts beat list, and refreshes selection/UI.

    // 1. SELECTION & BANK VALIDATION
    // Continue only when selected layer and bank contain active beats.
    if (selectedLayerNum != -1) {
        Layer& layer = layerLibrary[selectedLayerNum];
        Bank& bank = layer.bankLibrary[activeSongBank];
        if (bank.lastActiveBeatNum != -1) {
            // 2. PREVIOUS BEAT REPAIR
            // Adjust previous beat end interval after removing current beat.
            if (selectedBeatNum > 0) {
                if (selectedBeatNum != bank.lastActiveBeatNum) {
                    bank.beatLibrary[selectedBeatNum - 1].setEndInterval(bank.beatLibrary[selectedBeatNum + 1].getStartInterval());
                } else {
                    bank.beatLibrary[selectedBeatNum - 1].setEndInterval(songInterval);
                }
                bank.setBeatFill(selectedBeatNum - 1, 0, 0, 0, 0);
                lcd_drawBeat(selectedLayerNum, activeSongBank, (selectedBeatNum - 1), false);
            }

            // 3. BEAT CLEAR & REORDER
            // Clear target beat, reset slot, and compact active list.
            lcd_clearBeat(selectedLayerNum, activeSongBank, selectedBeatNum);
            bank.resetBeat(selectedBeatNum);
            layerSong_arrangeActiveBeats(selectedLayerNum, activeSongBank, false, true, true, true);

            // 4. SELECTION RECALCULATION
            // Select previous valid beat or set selection to none.
            if (bank.lastActiveBeatNum != -1) {
                if (selectedBeatNum == 0) {
                    selectedBeatNum = bank.lastActiveBeatNum;
                } else {
                    selectedBeatNum -= 1;
                }
                lcd_drawBeat(selectedLayerNum, activeSongBank, selectedBeatNum, true);
            } else {
                selectedBeatNum = -1;
            }

            // 5. UI REFRESH
            // Redraw fill widgets for updated beat state.
            lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
        }
    }
}

void Controller::layerSong_resetBeats(uint8_t layerNum_, uint8_t bankTab_, uint16_t startInterval_) {
    /// @brief Resets beats whose start interval is at/after given position.

    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];

    // 1. ACTIVE BANK VALIDATION
    // Continue only when target bank contains active beats.
    if (bank.lastActiveBeatNum != -1) {
        bool reorder = false;
        for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
            Beat& beat = bank.beatLibrary[i];
            if (beat.getStartInterval() >= startInterval_) {
                // 2. TARGET BEAT RESET
                // Extend previous beat and reset current beat slot.
                if (i > 0) {
                    bank.beatLibrary[i - 1].setEndInterval(songInterval);
                    bank.setBeatFill(i - 1, 0, 0, 0, 0);
                }
                bank.resetBeat(i);
                reorder = true;
            }
        }

        // 3. ACTIVE LIST ARRANGEMENT
        // Rebuild active beat list and playback state after resets.
        layerSong_arrangeActiveBeats(layerNum_, bankTab_, reorder, reorder, reorder, true);
    }
}

void Controller::layerSong_resetAllBeats(uint8_t layerNum_, uint8_t bankTab_) {
    /// @brief Resets every beat slot in the target layer song bank.

    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];

    // 1. ACTIVE BANK VALIDATION
    // Continue only when target bank contains active beats.
    if (bank.lastActiveBeatNum != -1) {
        // 2. FULL BANK RESET
        // Clear all beat slots and reset runtime beat pointers.
        for (uint16_t i = 0; i < kBeatLibrarySize; i++) {
            bank.resetBeat(i);
        }
        bank.lastActiveBeatNum = -1;
        bank.playBeatNum = -1;
        lcd_clearSong(layerNum_, bankTab_);

        // 3. CONDITIONAL UI REFRESH
        // Refresh beat widgets when target bank is currently visible.
        // The selection always becomes invalid for an emptied bank; the fields are only redrawn
        // when the Layer Song menu is visible. (This used to read "menu = LAYER_SONG_MENU", an
        // assignment that silently switched the menu state when a layer was cleared elsewhere.)
        if ((selectedLayerNum == layerNum_) && (activeSongBank == bankTab_)) selectedBeatNum = -1;
        if ((selectedLayerNum == layerNum_) && (activeSongBank == bankTab_) && (menu == LAYER_SONG_MENU)) {
            lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
            lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
        }
    }
}

void Controller::layerSong_quantizeActiveBeats(uint8_t layerNum_, uint8_t bankTab_) {
    /// @brief Quantizes active beats to current rhythm grid and rebuilds beat ordering.

    uint16_t quantizeInterval = kQuantizeInterval[rhythm.quantize];

    // 1. ACTIVE BEAT QUANTIZATION
    // Snap start/end intervals to quantize grid for active beats.
    for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
        Layer& layer = layerLibrary[i];
        Bank& bank = layer.bankLibrary[bankTab_];
        if (bank.lastActiveBeatNum != -1) {
            for (uint8_t j = 0; j < kBeatLibrarySize; j++) {
                Beat& beat = bank.beatLibrary[j];
                if (beat.getActive()) {
                    uint16_t startInterval = beat.getStartInterval();
                    uint16_t endInterval = beat.getEndInterval();

                    // Compensate interval for better quantization.
                    if (startInterval != 0) startInterval -= 1;
                    if (endInterval != 0) endInterval -= 1;
                    startInterval = ((startInterval / quantizeInterval) + ((startInterval % quantizeInterval) / (quantizeInterval / 2))) * quantizeInterval;
                    endInterval = ((endInterval / quantizeInterval) + ((endInterval % quantizeInterval) / (quantizeInterval / 2))) * quantizeInterval;
                    if (endInterval > songInterval) {
                        endInterval = songInterval;
                    }
                    if (startInterval != songInterval) {
                        beat.setStartInterval(startInterval);
                        beat.setEndInterval(endInterval);
                        bank.setBeatFill(j, 0, 0, 0, 0);
                    } else {
                        bank.resetBeat(j);
                    }
                }
            }

            // 2. ACTIVE LIST ARRANGEMENT
            // Reorder and normalize beat list after quantization pass.
            layerSong_arrangeActiveBeats(layerNum_, bankTab_, true, true, true, true);
        }
    }
}

void Controller::layerSong_arrangeActiveBeats(uint8_t layerNum_, uint8_t bankTab_, bool duplicate_, bool collect_, bool sort_, bool lastFill_) {
    /// @brief Normalizes active beat list via duplicate removal, collect, sort, and fill rules.

    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];

    // 1. ACTIVE BANK VALIDATION
    // Continue only when target bank contains active beats.
    if (bank.lastActiveBeatNum != -1) {
        // 2. DUPLICATE REMOVAL PHASE
        // Remove beats that share the same start interval.
        if (duplicate_) {
            for (uint8_t i = 0; i < kBeatLibrarySize; i++) {
                Beat& beat = bank.beatLibrary[i];
                if (beat.getActive()) {
                    uint16_t beatStartInterval = beat.getStartInterval();
                    for (uint8_t j = i + 1; j < kBeatLibrarySize; j++) {
                        Beat& nextBeat = bank.beatLibrary[j];
                        if (nextBeat.getActive()) {
                            uint16_t nextStartInterval = bank.beatLibrary[j].getStartInterval();
                            if (nextStartInterval == beatStartInterval) {
                                bank.resetBeat(j);
                            }
                        }
                    }
                }
            }
        }

        // 3. COMPACT PHASE
        // Collect active beats contiguously and update last-active index.
        if (collect_) {
            Beat tempLibrary[kBeatLibrarySize];
            int8_t tempCounter = -1;
            for (uint8_t i = 0; i < kBeatLibrarySize; i++) {
                if (bank.beatLibrary[i].getActive()) {
                    tempCounter += 1;
                    tempLibrary[tempCounter] = bank.beatLibrary[i];
                }
            }
            for (uint8_t j = 0; j < kBeatLibrarySize; j++) {
                bank.beatLibrary[j] = tempLibrary[j];
            }
            bank.lastActiveBeatNum = tempCounter;
        }

        // 4. SORT PHASE
        // Sort active beats by start interval and rebuild end intervals.
        if ((sort_) && (bank.lastActiveBeatNum > 0)) {
            // Sort beats (selection sort over indices 0..lastActiveBeatNum inclusive; the
            // bounds used to stop one short, leaving the last beat unsorted).
            uint8_t min;
            Beat temp;
            for (uint8_t i = 0; i < bank.lastActiveBeatNum; i++) {
                min = i;
                for (uint8_t j = i + 1; j <= bank.lastActiveBeatNum; j++) {
                    if (bank.beatLibrary[j].getStartInterval() < bank.beatLibrary[min].getStartInterval()) {
                        min = j;
                    }
                }
                temp = bank.beatLibrary[i];
                bank.beatLibrary[i] = bank.beatLibrary[min];
                bank.beatLibrary[min] = temp;
            }

            // Update end intervals.
            for (uint8_t k = 0; k < bank.lastActiveBeatNum; k++) {
                bank.beatLibrary[k].setEndInterval(bank.beatLibrary[k + 1].getStartInterval());
            }
            bank.beatLibrary[bank.lastActiveBeatNum].setEndInterval(songInterval);

            // Update fill parameters.
            for (uint8_t l = 0; l < bank.lastActiveBeatNum; l++) {
                uint8_t fillType = bank.beatLibrary[l].getFillType();
                uint8_t fillPattern = bank.beatLibrary[l].getFillPattern();
                uint8_t fillTime = bank.beatLibrary[l].getFillTime();
                uint8_t fillLevel = bank.beatLibrary[l].getFillLevel();
                bank.setBeatFill(l, fillType, fillPattern, fillTime, fillLevel);
            }
        }

        // 5. LAST-BEAT FILL RULE
        // Clear invalid fill on last beat when end interval is not song end.
        if (lastFill_) {
            if (bank.lastActiveBeatNum != -1) {
                Beat& beat = bank.beatLibrary[bank.lastActiveBeatNum];
                if ((beat.getFillType() != 0) && (beat.getEndInterval() != songInterval)) {
                    bank.setBeatFill(bank.lastActiveBeatNum, 0, 0, 0, 0);
                }
            }
        }

        // 6. PLAYBACK POINTER UPDATE
        // Recalculate play beat pointer after arrangement.
        layerSong_calculatePlayBeatNum(layerNum_, bankTab_, playInterval);
    }
}

void Controller::layerSong_calculateLastActiveBeatNum(uint8_t layerNum_, uint8_t bankTab_) {
    /// @brief Calculates and stores the last active beat index for target bank.

    int8_t lastActiveBeatNum = -1;
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];

    // 1. LAST ACTIVE BEAT SCAN
    // Walk beat list until first inactive slot and keep last active index.
    for (uint16_t i = 0; i < kBeatLibrarySize; i++) {
        if (bank.beatLibrary[i].getActive()) {
            lastActiveBeatNum = i;
        } else {
            break;
        }
    }
    bank.lastActiveBeatNum = lastActiveBeatNum;
}

void Controller::layerSong_calculatePlayBeatNum(uint8_t layerNum_, uint8_t bankTab_, uint16_t playInterval_) {
    /// @brief Resolves next playable beat/micro-beat index for given play interval.

    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];

    // 1. ACTIVE BEAT VALIDATION
    // Continue only when bank contains at least one active beat.
    if (bank.lastActiveBeatNum != -1) {
        // 2. FILL-ENABLED SEARCH
        // Resolve next beat/micro-step when fill mode is enabled.
        if (layer.fill) {
            bool findFlag = false;
            for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                Beat& beat = bank.beatLibrary[i];
                uint8_t fillType = beat.getFillType();
                uint8_t fillPattern = beat.getFillPattern();

                // Simple beat processing.
                if ((fillType == 0) && (fillPattern <= 2)) {
                    if ((beat.getActive()) && (beat.getStartInterval() >= playInterval)) {
                        bank.playBeatNum = i;
                        bank.playBeatMicroNum = -1;
                        findFlag = true;
                        break;
                    }
                    if (findFlag) {
                        break;
                    }
                }

                // Complex fill processing.
                else {
                    uint8_t fillTarget = beat.getFillTarget();
                    if (fillTarget != NULL_FILL_TARGET) {
                        BeatFill& beatFillRef = bank.beatFillLibrary[fillTarget];
                        for (uint8_t j = 0; j < beatFillRef.getStepSize(); j++) {
                            BeatMicro& beatMicro = beatFillRef.beatMicroLibrary[j];
                            if ((beat.getActive()) && (beatMicro.getActive()) && (beatMicro.getInterval() >= playInterval)) {
                                bank.playBeatNum = i;
                                bank.playBeatMicroNum = j;
                                findFlag = true;
                                break;
                            }
                        }
                    }
                    if (findFlag) {
                        break;
                    }
                }
            }

            if (!findFlag) {
                Beat& beat = bank.beatLibrary[0];
                bank.playBeatNum = 0;
                bank.playBeatMicroNum = ((beat.getFillType() == 0) && (beat.getFillPattern() <= 2)) ? -1 : 0;
            }
        }

        // 3. FILL-DISABLED SEARCH
        // Resolve next base beat when fill mode is disabled.
        else {
            bool findFlag = false;
            for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                Beat& beat = bank.beatLibrary[i];
                if ((beat.getActive()) && (beat.getStartInterval() >= playInterval)) {
                    bank.playBeatNum = i;
                    bank.playBeatMicroNum = -1;
                    findFlag = true;
                    break;
                }
            }

            if (!findFlag) {
                bank.playBeatNum = 0;
                bank.playBeatMicroNum = -1;
            }
        }
    } else {
        // 4. EMPTY BANK FALLBACK
        // Mark play pointers as invalid when no active beats exist.
        bank.playBeatNum = -1;
        bank.playBeatMicroNum = -1;
    }
}

void Controller::layerSong_calculateBeatFill(uint8_t layerNum_, uint8_t bankTab_, uint8_t beatNum_, uint8_t fillType_, uint8_t fillPattern_, uint8_t fillTime_, uint8_t fillLevel_) {
    /// @brief Recomputes selected beat end interval and applies fill parameters.

    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];
    Beat& beat = bank.beatLibrary[beatNum_];
    uint16_t endInterval;

    // 1. END INTERVAL RESOLUTION
    // Resolve end interval from next beat or song boundary.
    (beatNum_ < bank.lastActiveBeatNum) ? endInterval = bank.beatLibrary[beatNum_ + 1].getStartInterval() : endInterval = songInterval;

    // 2. FILL PARAMETER APPLY
    // Store resolved end interval and update fill parameters.
    beat.setEndInterval(endInterval);
    bank.setBeatFill(beatNum_, fillType_, fillPattern_, fillTime_, fillLevel_);
}

void Controller::layerSong_calculateBeatFillTime(uint8_t layerNum_, uint8_t bankTab_, uint8_t beatNum_, uint8_t fillTime_) {
    /// @brief Updates fill-time value for target beat while preserving other fill fields.

    // 1. FILL TIME UPDATE
    // Apply new fill-time while keeping existing type/pattern/level.
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];
    Beat& beat = bank.beatLibrary[beatNum_];
    bank.setBeatFill(beatNum_, beat.getFillType(), beat.getFillPattern(), fillTime_, beat.getFillLevel());
}

void Controller::layerSong_calculateBeatFillLevel(uint8_t layerNum_, uint8_t bankTab_, uint8_t beatNum_, uint8_t fillLevel_) {
    /// @brief Updates fill-level value for target beat while preserving other fill fields.

    // 1. FILL LEVEL UPDATE
    // Apply new fill-level while keeping existing type/pattern/time.
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[bankTab_];
    Beat& beat = bank.beatLibrary[beatNum_];
    bank.setBeatFill(beatNum_, beat.getFillType(), beat.getFillPattern(), beat.getFillTime(), fillLevel_);
}

void Controller::layerSong_copy() {
    /// @brief Copies active bank beat data of selected layer to clipboard buffer.

    // 1. SELECTION VALIDATION
    // Continue only when selected layer index is valid.
    if (selectedLayerNum != -1) {
        // 2. CLIPBOARD SNAPSHOT
        // Mark clipboard active and copy beat library data.
        layerSongCopy.copyActive = true;
        for (uint8_t i = 0; i < kBeatLibrarySize; i++) {
            layerSongCopy.lastActiveBeatNum = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum;
            layerSongCopy.beatLibrary[i] = layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].beatLibrary[i];
        }
    }
}

void Controller::layerSong_paste() {
    /// @brief Pastes clipboard beat data into selected layer active song bank.

    // 1. CLIPBOARD & TARGET VALIDATION
    // Continue only when clipboard is active and selection is valid.
    if ((layerSongCopy.copyActive) && (selectedLayerNum != -1)) {
        Layer& layer = layerLibrary[selectedLayerNum];

        // 2. EXISTING DATA CLEANUP
        // Clear current bank beats before paste operation.
        if (layer.bankLibrary[activeSongBank].lastActiveBeatNum != -1) layerSong_resetAllBeats(selectedLayerNum, activeSongBank);

        // 3. CLIPBOARD APPLY
        // Recreate beats from clipboard entries within song range.
        if (layerSongCopy.lastActiveBeatNum != -1) {
            for (int8_t i = layerSongCopy.lastActiveBeatNum; i >= 0; i--) {
                uint16_t startInterval = layerSongCopy.beatLibrary[i].getStartInterval();
                uint8_t fillType = layerSongCopy.beatLibrary[i].getFillType();
                uint8_t fillPattern = layerSongCopy.beatLibrary[i].getFillPattern();
                uint8_t fillTime = layerSongCopy.beatLibrary[i].getFillTime();
                uint8_t fillLevel = layerSongCopy.beatLibrary[i].getFillLevel();
                if (startInterval < songInterval) layerSong_setBeat(selectedLayerNum, activeSongBank, startInterval, fillType, fillPattern, fillTime, fillLevel, true);
            }

            // 4. SELECTION & UI REFRESH
            // Recompute selection/play pointers and redraw menu widgets.
            if (layer.bankLibrary[activeSongBank].lastActiveBeatNum != -1) {
                selectedBeatNum = 0;
                layerSong_calculateLastActiveBeatNum(selectedLayerNum, activeSongBank);
                layerSong_calculatePlayBeatNum(selectedLayerNum, activeSongBank, playInterval);
                lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillPatternData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
                lcd_drawSong(selectedLayerNum, activeSongBank);
            }
        }
    }
}

void Controller::layerSong_selectBank(uint8_t bankNum_) {
    /// @brief Selects active song bank and refreshes all layer song visuals/play pointers.

    // 1. INPUT VALIDATION
    // Accept only values inside configured song-bank limits.
    if ((bankNum_ >= kMinSongBank) && (bankNum_ <= kMaxSongBank)) {
        // 2. BANK SWITCH CONDITION
        // Continue only when requested bank differs from active bank.
        if (bankNum_ != activeSongBank) {
            uint8_t preBankNum = activeSongBank;
            activeSongBank = bankNum_;
            lcd_drawSongBankTab(bankNum_, false);

            // 3. ACTIVE MENU REFRESH
            // Refresh selected beat and visible Layer Song widgets.
            if (menu == LAYER_SONG_MENU) {
                (layerLibrary[selectedLayerNum].bankLibrary[activeSongBank].lastActiveBeatNum != -1) ? selectedBeatNum = 0 : selectedBeatNum = -1;
                lcd_drawLayerSong_BeatFillTypeData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillTimeData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillLevelData(selectedLayerNum);
                lcd_drawLayerSong_BeatFillGraphData(selectedLayerNum);
            }

            // 4. GLOBAL LAYER REFRESH
            // Redraw all layer song lanes and update play pointers.
            for (uint8_t i = 0; i < kLayerLibrarySize; i++) {
                lcd_clearSong(i, preBankNum);
                lcd_drawSong(i, activeSongBank);
                layerSong_calculatePlayBeatNum(i, activeSongBank, playInterval);
            }
            lcd_redrawNotePage();
        }
    }
}

void Controller::layerSong_triggerBank(uint8_t bankNum_) {
    /// @brief Arms/cancels bank shift trigger for next song loop boundary.

    // 1. INPUT VALIDATION
    // Accept only values inside configured song-bank limits.
    if ((bankNum_ >= kMinSongBank) && (bankNum_ <= kMaxSongBank)) {
        // 2. SHIFT ARM/CANCEL
        // Arm bank shift when target differs, cancel when same bank is retriggered.
        if (bankNum_ != activeSongBank) {
            bankShiftFlag = true;
            targetSongBank = bankNum_;
            lcd_drawSongBankTab(bankNum_, true);
        } else if ((bankNum_ == activeSongBank) && (bankShiftFlag) && (targetSongBank != activeSongBank)) {
            bankShiftFlag = false;
            targetSongBank = bankNum_;
            lcd_drawSongBankTab(bankNum_, false);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Sample Preview Functions --------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

// Lets the user hear the *selected* sample in the Layer Inst menu before loading it with ADD.
// The file is streamed from SD into RAM_PREVIEW_ADDRESS in kPreviewChunkBytes steps from the
// main loop (preview_service), and the audio ISR (processAudioSend) plays whatever has been
// written so far. Nothing here touches the layers' sample sectors or loaded samples.

namespace {

// Opened and closed for every chunk: FatFs is configured with _FS_LOCK = 2 (two open files at
// once), and other SD operations may need both, so the preview never holds a file open between
// main-loop steps. Static (not on the stack) because a FIL carries a 4 KB sector buffer.
FIL previewChunkFile;

}  // namespace

void Controller::preview_request(uint8_t layerNum_) {
    /// @brief Debounced preview: stop the current one now, start this layer's selected sample
    /// once the knob has rested for kPreviewDebounceMs (handled in preview_service).

    if (!previewActive) return;
    preview_stop();
    previewRequestPending = true;
    previewRequestTick = HAL_GetTick();
    previewRequestLayer = static_cast<int8_t>(layerNum_);
}

void Controller::preview_stop() {
    /// @brief Stops any preview: cancels a pending request and the streaming, and lets the
    /// audio ISR fade the sound out (one 32-sample block) before the buffer can be reused.

    previewRequestPending = false;
    if (previewPlay.streaming) preview_finishStreaming();

    if (previewPlay.playing) {
        previewPlay.stopRequest = true;
        const uint32_t start = HAL_GetTick();
        while (previewPlay.playing && ((HAL_GetTick() - start) <= kPreviewStopTimeoutMs)) {
        }
        // If the audio stream is not running, nothing would clear the flags.
        previewPlay.playing = false;
        previewPlay.stopRequest = false;
    }
    preview_showIndicator(false);
}

void Controller::preview_finishStreaming() {
    /// @brief Marks the stream complete; the ISR plays to the last written sample and stops.

    previewPlay.streaming = false;
}

bool Controller::preview_start(uint8_t layerNum_) {
    /// @brief Starts previewing the selected (not necessarily loaded) sample of a layer.
    /// Reads the first chunk right away so sound starts within one main-loop step, then keeps
    /// streaming from preview_service(). Returns false if there is nothing valid to play.

    preview_stop();
    if ((!previewActive) || (layerNum_ >= kLayerLibrarySize)) return false;

    // 1. VALIDATE SELECTION (the WAV header was parsed by layerInst_setSampleSelected)
    Layer& layer = layerLibrary[layerNum_];
    if ((layer.sampleSelected == -1) || layer.sampleSelectedReadError || layer.sampleSelectedTypeError) return false;
    if (layer.sampleSelectedData.sampleByteSize == 0) return false;

    const WavData& wav = layer.wavSelectedData;
    previewPlan = sampleformat::plan(wav.fmt_chunk.sampleRate, wav.fmt_chunk.nbrChannels, wav.fmt_chunk.bitPerSample);
    if ((previewPlan.offsetSize == 0) || ((kPreviewChunkBytes % previewPlan.offsetSize) != 0)) return false;

    // 2. STREAM STATE
    static_assert(sizeof(previewFileName) == sizeof(layer.sampleSelectedData.fileName), "file name buffer size");
    memcpy(previewFileName, layer.sampleSelectedData.fileName, sizeof(previewFileName));
    previewFileName[sizeof(previewFileName) - 1] = '\0';
    previewDataStart = wav.data_chunk.chunkStartByte + 8;
    previewBytesTotal = layer.sampleSelectedData.sampleByteSize;
    previewBytesRead = 0;
    previewOutCount = 0;
    previewLayer = static_cast<int8_t>(layerNum_);

    previewPlay.counter = 0;
    previewPlay.filled = 0;
    previewPlay.gain = kNumberDataLibrary[previewLevel].powMultiplier;
    previewPlay.stopRequest = false;
    previewPlay.streaming = true;

    // 3. FIRST CHUNK, THEN START PLAYBACK
    if (!preview_streamChunk()) {
        preview_finishStreaming();
        return false;
    }
    __DMB();
    previewPlay.playing = true;
    preview_showIndicator(true);
    return true;
}

bool Controller::preview_streamChunk() {
    /// @brief Reads up to kPreviewChunkBytes of PCM, converts it like the loader does and
    /// appends it to the preview buffer. Returns false on a read error.

    if (!previewPlay.streaming) return false;

    const uint32_t remaining = previewBytesTotal - previewBytesRead;
    uint32_t toRead = (remaining < kPreviewChunkBytes) ? remaining : kPreviewChunkBytes;
    toRead -= toRead % previewPlan.offsetSize;  // whole frames only

    bool ok = false;
    UINT bytesRead = 0;
    if ((toRead > 0) && (previewOutCount < kSampleSize)) {
        if (f_open(&previewChunkFile, previewFileName, FA_READ) == FR_OK) {
            ok = (f_lseek(&previewChunkFile, previewDataStart + previewBytesRead) == FR_OK) &&
                 (f_read(&previewChunkFile, sdRawBuffer, toRead, &bytesRead) == FR_OK) && (bytesRead > 0);
            f_close(&previewChunkFile);
        }
    }

    if (ok) {
        // Convert and write the data first, then publish the new fill level to the ISR.
        float* out = reinterpret_cast<float*>(RAM_PREVIEW_ADDRESS);
        const uint32_t frames = bytesRead / previewPlan.offsetSize;
        for (uint32_t i = 0; i < frames; i++) {
            const float val = sampleformat::toFloat(&sdRawBuffer[i * previewPlan.offsetSize], previewPlan.bitsPerSample);
            for (uint8_t k = 0; (k < previewPlan.writeCount) && (previewOutCount < kSampleSize); k++) {
                out[previewOutCount++] = val;
            }
        }
        previewBytesRead += bytesRead;
        __DMB();
        previewPlay.filled = previewOutCount;
    }

    if ((!ok) || (previewBytesRead >= previewBytesTotal) || (previewOutCount >= kSampleSize)) preview_finishStreaming();
    return ok;
}

void Controller::preview_service() {
    /// @brief Main-loop step: start a debounced preview when due, stream one chunk, and clear
    /// the on-screen indicator when playback has ended.

    if (previewRequestPending && ((HAL_GetTick() - previewRequestTick) >= kPreviewDebounceMs)) {
        previewRequestPending = false;
        if (previewRequestLayer >= 0) preview_start(static_cast<uint8_t>(previewRequestLayer));
    }
    if (previewPlay.streaming) preview_streamChunk();
    if (previewIndicator && (!previewPlay.playing) && (!previewPlay.streaming)) preview_showIndicator(false);
}

void Controller::preview_showIndicator(bool show_) {
    /// @brief Shows/hides "PREVIEW" where "LOADING" appears in the Layer Inst menu.

    const bool visible = ((menu == LAYER_INST_0_MENU) || (menu == LAYER_INST_1_MENU) || (menu == LAYER_INST_2_MENU)) &&
                         (previewLayer == selectedLayerNum);
    if (show_) {
        if (visible) {
            lcd_setMenuNumState(kAccentColor);
            lcd.drawText("    PREVIEW", 11, kMenuSignBoxX[1], kMenuSignBoxY[1]);
            previewIndicator = true;
        }
    } else if (previewIndicator) {
        previewIndicator = false;
        if (visible) {
            lcd.setBackColor(BLACK);
            lcd.clearRect(kMenuSignBoxX[1] - 70, kMenuSignBoxY[1], 70, kMenuSignBoxHeight);
        }
    }
}

