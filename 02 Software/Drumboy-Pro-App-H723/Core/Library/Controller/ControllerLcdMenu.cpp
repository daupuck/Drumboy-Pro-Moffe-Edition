// LCD drawing for each menu page.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

/* Lcd Main Menu Functions ---------------------------------------------------*/

void Controller::lcd_drawGlobalMenu() {
    /// @brief Draws global menu headers and values for all encoder slots.

    // 1. MENU ICON DRAW
    // Draw global menu icon area.
    lcd_drawMenuIcon(GLOBAL_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. SLOT CONTENT DRAW
    // Draw function header and value for each encoder slot.
    for (uint8_t i = 0; i < kEncoderSize; i++) {
        lcd_drawGlobalMenuFunction(i);
        lcd_drawGlobalMenuData(i);
    }
}

void Controller::lcd_drawGlobalMenuFunction(uint8_t encoderNum_) {
    /// @brief Renders function/header/sign labels for one global encoder slot.

    // 1. SLOT CONTEXT LOAD
    // Read current encoder assignment for the target slot.
    Encoder enc = encoder[encoderNum_];

    // 2. GLOBAL MODE GUARD
    // This panel is valid only for global-mode mappings.
    if (enc.mode == ENC_MODE_GLOBAL) {
        // 3. FUNCTION HEADER DRAW
        // Draw the main function title (10-char field).
        RGB16Color color = (enc.function == ENC_FNC_OFF) ? kEncoderGlobalInactiveColor : kGlobalMenuHeaderColor;
        lcd_setMenuHeaderState(color);
        lcd.drawText(kEncoderFuncDataLibrary[enc.function].funcHeader, kMenuHeaderTextSize, kMenuHeaderX[encoderNum_], kMenuHeaderY[encoderNum_]);

        // 4. MENU GROUP DRAW
        // Draw short menu group label under the function title.
        lcd_setMenuTextState(WHITE);
        lcd.setAlignment(LEFT);
        lcd.drawText(kEncoderFuncDataLibrary[enc.function].menuHeader, 6, kMenuHeaderX[encoderNum_], kMenuSignBoxY[encoderNum_]);

        // 5. UNIT/SIGN DRAW
        // Draw right-side unit/sign text.
        lcd_setMenuSignState(kAccentColor);
        lcd.drawText(kEncoderFuncDataLibrary[enc.function].sign, 3, kMenuSignBoxX[encoderNum_], kMenuSignBoxY[encoderNum_]);
    }
}

void Controller::lcd_drawGlobalMenuData(uint8_t encoderNum_) {
    /// @brief Draws global-menu data value for selected encoder slot.

    // 1. ENCODER CONTEXT LOAD
    // Load encoder data and process only global-mode slots.
    Encoder enc = encoder[encoderNum_];
    if (enc.mode == ENC_MODE_GLOBAL) {
        // 2. VALUE STYLE SETUP
        // Configure menu data style before text resolution.
        RGB16Color color = (enc.function == ENC_FNC_OFF) ? kEncoderGlobalInactiveColor : kGlobalMenuDataColor;
        lcd_setMenuDataState(color);
        const char* textPtr;
        // Formatted values must outlive the switch: textPtr is drawn after it, so a buffer
        // declared inside a case would already be out of scope (dangling pointer).
        char valueText[kMenuDataTextSize + 1];

        // 3. VALUE TEXT RESOLUTION
        // Resolve value text from encoder function and module state.
        switch (enc.function) {
            case ENC_FNC_NONE:
                textPtr = kDataDashR;
                break;

                // Mixer functions

            case ENC_MIXER_MASTERVOLUME:
                textPtr = kNumberDataLibrary[mixer.masterVolume].nameLongR;
                break;

            case ENC_MIXER_HPVOLUME:
                textPtr = kMixerHpVolumeDataLibrary[mixer.hpVolume].nameLongR;
                break;

            case ENC_MIXER_LOVOLUME:
                textPtr = kMixerLoVolumeDataLibrary[mixer.loVolume].nameLongR;
                break;

            case ENC_MIXER_LIVOLUME:
                textPtr = kMixerLiVolumeDataLibrary[mixer.liVolume].nameLongR;
                break;

            case ENC_MIXER_LIMIXERIN:
                textPtr = kMixerLiMixerInDataLibrary[mixer.liMixerIn].nameLongR;
                break;

            case ENC_MIXER_PAN:
                textPtr = kMixerPanDataLibrary[mixer.pan].nameLongR;
                break;

            case ENC_MIXER_LIMITER:
                textPtr = kMixerLimiterDataLibrary[mixer.limiter].nameLongR;
                break;

            case ENC_MIXER_NORMALIZE:
                (mixer.normalize) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

                // Rhythm functions

            case ENC_RHYTHM_TEMPO:
                snprintf(valueText, sizeof(valueText), "       %03d", rhythm.tempo);
                textPtr = valueText;
                break;

            case ENC_RHYTHM_MEASURE:
                textPtr = kNumberDataLibrary[rhythm.measure].nameLongR;
                break;

            case ENC_RHYTHM_BAR:
                textPtr = kNumberDataLibrary[rhythm.bar].nameLongR;
                break;

            case ENC_RHYTHM_QUANTIZE:
                textPtr = kQuantizeDataLibrary[rhythm.quantize].nameLongR;
                break;

                // Metronome functions

            case ENC_METRO_ACTIVE:
                (metronome.active) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_METRO_PRECOUNT:
                (metronome.precount) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_METRO_SAMPLE:
                textPtr = kMetroSampleDataLibrary[metronome.sample].nameLongR;
                break;

            case ENC_METRO_VOLUME:
                textPtr = kNumberDataLibrary[metronome.volume].nameLongR;
                break;

                // Io functions

            case ENC_IO_SYNCIN: {
                if (keyboard.shiftKeyPress) {
                    const uint8_t syncInPpqnIdx = (io.syncInPpqn <= kMaxIoSyncPpqn) ? io.syncInPpqn : kInitialIoSyncInPpqn;
                    textPtr = (io.syncIn > 0) ? kIoSyncPpqnDataLibrary[syncInPpqnIdx].nameLongR : kIoSyncInDataLibrary[io.syncIn].nameLongR;
                } else {
                    textPtr = kIoSyncInDataLibrary[io.syncIn].nameLongR;
                }
                break;
            }

            case ENC_IO_SYNCOUT: {
                if (keyboard.shiftKeyPress) {
                    const uint8_t syncOutPpqnIdx = (io.syncOutPpqn <= kMaxIoSyncPpqn) ? io.syncOutPpqn : kInitialIoSyncOutPpqn;
                    textPtr = (io.syncOut > 0) ? kIoSyncPpqnDataLibrary[syncOutPpqnIdx].nameLongR : kIoSyncOutDataLibrary[io.syncOut].nameLongR;
                } else {
                    textPtr = kIoSyncOutDataLibrary[io.syncOut].nameLongR;
                }
                break;
            }

            case ENC_IO_MIDIIN: {
                if (keyboard.shiftKeyPress) {
                    snprintf(valueText, sizeof(valueText), "      CH%02d", io.midiInChannel + 1);
                    textPtr = valueText;
                } else {
                    textPtr = kIoMidiModeDataLibrary[io.midiIn].nameLongR;
                }
                break;
            }

            case ENC_IO_MIDIOUT: {
                if (keyboard.shiftKeyPress) {
                    snprintf(valueText, sizeof(valueText), "      CH%02d", io.midiOutChannel + 1);
                    textPtr = valueText;
                } else {
                    textPtr = kIoMidiModeDataLibrary[io.midiOut].nameLongR;
                }
                break;
            }

                // Eq functions

            case ENC_EQ_ACTIVE:
                (eq.active) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_EQ_Q:
                textPtr = kEqQDataLibrary[eq.q].nameLongR;
                break;

            case ENC_EQ_LOWSHELF:
                textPtr = kEqGainDataLibrary[eq.gainLowShelf].nameLongR;
                break;

            case ENC_EQ_HIGHSHELF:
                textPtr = kEqGainDataLibrary[eq.gainHighShelf].nameLongR;
                break;

            case ENC_EQ_PEAK0:
                textPtr = kEqGainDataLibrary[eq.gainPeak[0]].nameLongR;
                break;

            case ENC_EQ_PEAK1:
                textPtr = kEqGainDataLibrary[eq.gainPeak[1]].nameLongR;
                break;

            case ENC_EQ_PEAK2:
                textPtr = kEqGainDataLibrary[eq.gainPeak[2]].nameLongR;
                break;

            case ENC_EQ_PEAK3:
                textPtr = kEqGainDataLibrary[eq.gainPeak[3]].nameLongR;
                break;

                // Filter 0 functions

            case ENC_FILTER_0_ACTIVE:
                (filter[0].active) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_FILTER_0_TYPE:
                textPtr = kFilterTypeDataLibrary[filter[0].type].nameLongR;
                break;

            case ENC_FILTER_0_FREQ:
                textPtr = kFilterFrequencyDataLibrary[filter[0].frequency].nameLongR;
                break;

            case ENC_FILTER_0_RESON:
                textPtr = kNumberDataLibrary[filter[0].resonance].nameLongR;
                break;

            case ENC_FILTER_0_SLOPE:
                textPtr = kFilterSlopeDataLibrary[filter[0].slope].nameLongR;
                break;

            case ENC_FILTER_0_DRIVE:
                textPtr = kNumberDataLibrary[filter[0].drive].nameLongR;
                break;

            case ENC_FILTER_0_DRY:
                textPtr = kNumberDataLibrary[filter[0].dry].nameLongR;
                break;

            case ENC_FILTER_0_WET:
                textPtr = kNumberDataLibrary[filter[0].wet].nameLongR;
                break;

                // Fİlter 1 functions

            case ENC_FILTER_1_ACTIVE:
                (filter[1].active) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_FILTER_1_TYPE:
                textPtr = kFilterTypeDataLibrary[filter[1].type].nameLongR;
                break;

            case ENC_FILTER_1_FREQ:
                textPtr = kFilterFrequencyDataLibrary[filter[1].frequency].nameLongR;
                break;

            case ENC_FILTER_1_RESON:
                textPtr = kNumberDataLibrary[filter[1].resonance].nameLongR;
                break;

            case ENC_FILTER_1_SLOPE:
                textPtr = kFilterSlopeDataLibrary[filter[1].slope].nameLongR;
                break;

            case ENC_FILTER_1_DRIVE:
                textPtr = kNumberDataLibrary[filter[1].drive].nameLongR;
                break;

            case ENC_FILTER_1_DRY:
                textPtr = kNumberDataLibrary[filter[1].dry].nameLongR;
                break;

            case ENC_FILTER_1_WET:
                textPtr = kNumberDataLibrary[filter[1].wet].nameLongR;
                break;

                // Effect 0 functions

            case ENC_EFFECT_0_ACTIVE:
                (effect[0].active) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_EFFECT_0_TYPE:
                textPtr = kEffectTypeDataLibrary[effect[0].type].nameLongR;
                break;

            case ENC_EFFECT_0_PARAM_A:
                switch (effect[0].type) {
                    case EF_DELAY:
                        textPtr = kDelayTimeDataLibrary[effect[0].delay.aTime].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kChorusTimeDataLibrary[effect[0].chorus.aTime].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kFlangerTimeDataLibrary[effect[0].flanger.aTime].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kPhaserFreqDataLibrary[effect[0].phaser.aStartFreq].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorThresholdDataLibrary[effect[0].compressor.aThreshold].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderThresholdDataLibrary[effect[0].expander.aThreshold].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kOverdriveGainDataLibrary[effect[0].overdrive.aGain].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kDistortionGainDataLibrary[effect[0].distortion.aGain].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kBitcrusherResolutionDataLibrary[effect[0].bitcrusher.aResolution].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_0_PARAM_B:
                switch (effect[0].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[0].delay.bLevel].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kChorusFeedbackDataLibrary[effect[0].chorus.bFeedback].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kFlangerFeedbackDataLibrary[effect[0].flanger.bFeedback].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kPhaserFreqDataLibrary[effect[0].phaser.bEndFreq].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorRateDataLibrary[effect[0].compressor.bRate].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderRateDataLibrary[effect[0].expander.bRate].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kOverdriveThresholdDataLibrary[effect[0].overdrive.bThreshold].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kDistortionThresholdDataLibrary[effect[0].distortion.bThreshold].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kBitcrusherSampleRateDataLibrary[effect[0].bitcrusher.bSampleRate].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_0_PARAM_C:
                switch (effect[0].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[0].delay.cFeedback].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kChorusRateDataLibrary[effect[0].chorus.cRate].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kFlangerRateDataLibrary[effect[0].flanger.cRate].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kPhaserRateDataLibrary[effect[0].phaser.cRate].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorAttackTimeDataLibrary[effect[0].compressor.cAttackTime].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderAttackTimeDataLibrary[effect[0].expander.cAttackTime].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kOverdriveToneDataLibrary[effect[0].overdrive.cTone].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kDistortionToneDataLibrary[effect[0].distortion.cTone].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kBitcrusherThresholdDataLibrary[effect[0].bitcrusher.cThreshold].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_0_PARAM_D:
                switch (effect[0].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[0].delay.dDamping].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kNumberDataLibrary[effect[0].chorus.dDepth].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kNumberDataLibrary[effect[0].flanger.dWidth].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kNumberDataLibrary[effect[0].phaser.dFeedback].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorReleaseTimeDataLibrary[effect[0].compressor.dReleaseTime].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderReleaseTimeDataLibrary[effect[0].expander.dReleaseTime].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kNumberDataLibrary[effect[0].overdrive.dDriveColor].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kNumberDataLibrary[effect[0].distortion.dAsym].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kNumberDataLibrary[effect[0].bitcrusher.dSmooth].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_0_PARAM_E:
                switch (effect[0].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[0].delay.eDry].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kNumberDataLibrary[effect[0].chorus.eDry].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kNumberDataLibrary[effect[0].flanger.eDry].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kNumberDataLibrary[effect[0].phaser.eDry].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kNumberDataLibrary[effect[0].compressor.eDry].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kNumberDataLibrary[effect[0].expander.eDry].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kNumberDataLibrary[effect[0].overdrive.eDry].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kNumberDataLibrary[effect[0].distortion.eDry].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kNumberDataLibrary[effect[0].bitcrusher.eDry].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_0_PARAM_F:
                switch (effect[0].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[0].delay.fWet].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kNumberDataLibrary[effect[0].chorus.fWet].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kNumberDataLibrary[effect[0].flanger.fWet].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kNumberDataLibrary[effect[0].phaser.fWet].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kNumberDataLibrary[effect[0].compressor.fWet].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kNumberDataLibrary[effect[0].expander.fWet].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kNumberDataLibrary[effect[0].overdrive.fWet].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kNumberDataLibrary[effect[0].distortion.fWet].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kNumberDataLibrary[effect[0].bitcrusher.fWet].nameLongR;
                        break;
                }
                break;

                // Effect 1 functions

            case ENC_EFFECT_1_ACTIVE:
                (effect[1].active) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_EFFECT_1_TYPE:
                textPtr = kEffectTypeDataLibrary[effect[1].type].nameLongR;
                break;

            case ENC_EFFECT_1_PARAM_A:
                switch (effect[1].type) {
                    case EF_DELAY:
                        textPtr = kDelayTimeDataLibrary[effect[1].delay.aTime].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kChorusTimeDataLibrary[effect[1].chorus.aTime].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kFlangerTimeDataLibrary[effect[1].flanger.aTime].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kPhaserFreqDataLibrary[effect[1].phaser.aStartFreq].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorThresholdDataLibrary[effect[1].compressor.aThreshold].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderThresholdDataLibrary[effect[1].expander.aThreshold].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kOverdriveGainDataLibrary[effect[1].overdrive.aGain].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kDistortionGainDataLibrary[effect[1].distortion.aGain].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kBitcrusherResolutionDataLibrary[effect[1].bitcrusher.aResolution].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_1_PARAM_B:
                switch (effect[1].type) {
                    case EF_DELAY:

                        textPtr = kNumberDataLibrary[effect[1].delay.bLevel].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kChorusFeedbackDataLibrary[effect[1].chorus.bFeedback].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kFlangerFeedbackDataLibrary[effect[1].flanger.bFeedback].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kPhaserFreqDataLibrary[effect[1].phaser.bEndFreq].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorRateDataLibrary[effect[1].compressor.bRate].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderRateDataLibrary[effect[1].expander.bRate].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kOverdriveThresholdDataLibrary[effect[1].overdrive.bThreshold].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kDistortionThresholdDataLibrary[effect[1].distortion.bThreshold].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kBitcrusherSampleRateDataLibrary[effect[1].bitcrusher.bSampleRate].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_1_PARAM_C:
                switch (effect[1].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[1].delay.cFeedback].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kChorusRateDataLibrary[effect[1].chorus.cRate].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kFlangerRateDataLibrary[effect[1].flanger.cRate].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kPhaserRateDataLibrary[effect[1].phaser.cRate].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorAttackTimeDataLibrary[effect[1].compressor.cAttackTime].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderAttackTimeDataLibrary[effect[1].expander.cAttackTime].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kOverdriveToneDataLibrary[effect[1].overdrive.cTone].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kDistortionToneDataLibrary[effect[1].distortion.cTone].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kBitcrusherThresholdDataLibrary[effect[1].bitcrusher.cThreshold].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_1_PARAM_D:
                switch (effect[1].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[1].delay.dDamping].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kNumberDataLibrary[effect[1].chorus.dDepth].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kNumberDataLibrary[effect[1].flanger.dWidth].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kNumberDataLibrary[effect[1].phaser.dFeedback].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kCompressorReleaseTimeDataLibrary[effect[1].compressor.dReleaseTime].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kExpanderReleaseTimeDataLibrary[effect[1].expander.dReleaseTime].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kNumberDataLibrary[effect[1].overdrive.dDriveColor].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kNumberDataLibrary[effect[1].distortion.dAsym].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kNumberDataLibrary[effect[1].bitcrusher.dSmooth].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_1_PARAM_E:
                switch (effect[1].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[1].delay.eDry].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kNumberDataLibrary[effect[1].chorus.eDry].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kNumberDataLibrary[effect[1].flanger.eDry].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kNumberDataLibrary[effect[1].phaser.eDry].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kNumberDataLibrary[effect[1].compressor.eDry].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kNumberDataLibrary[effect[1].expander.eDry].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kNumberDataLibrary[effect[1].overdrive.eDry].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kNumberDataLibrary[effect[1].distortion.eDry].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kNumberDataLibrary[effect[1].bitcrusher.eDry].nameLongR;
                        break;
                }
                break;

            case ENC_EFFECT_1_PARAM_F:
                switch (effect[1].type) {
                    case EF_DELAY:
                        textPtr = kNumberDataLibrary[effect[1].delay.fWet].nameLongR;
                        break;

                    case EF_CHORUS:
                        textPtr = kNumberDataLibrary[effect[1].chorus.fWet].nameLongR;
                        break;

                    case EF_FLANGER:
                        textPtr = kNumberDataLibrary[effect[1].flanger.fWet].nameLongR;
                        break;

                    case EF_PHASER:
                        textPtr = kNumberDataLibrary[effect[1].phaser.fWet].nameLongR;
                        break;

                    case EF_COMPRESSOR:
                        textPtr = kNumberDataLibrary[effect[1].compressor.fWet].nameLongR;
                        break;

                    case EF_EXPANDER:
                        textPtr = kNumberDataLibrary[effect[1].expander.fWet].nameLongR;
                        break;

                    case EF_OVERDRIVE:
                        textPtr = kNumberDataLibrary[effect[1].overdrive.fWet].nameLongR;
                        break;

                    case EF_DISTORTION:
                        textPtr = kNumberDataLibrary[effect[1].distortion.fWet].nameLongR;
                        break;

                    case EF_BITCRUSHER:
                        textPtr = kNumberDataLibrary[effect[1].bitcrusher.fWet].nameLongR;
                        break;
                }
                break;

                // Reverb functions

            case ENC_REVERB_ACTIVE:
                (reverb.active) ? textPtr = kDataOn : textPtr = kDataOff;
                break;

            case ENC_REVERB_SIZE:
                textPtr = kNumberDataLibrary[reverb.size].nameLongR;
                break;

            case ENC_REVERB_DECAY:
                textPtr = kNumberDataLibrary[reverb.decay].nameLongR;
                break;

            case ENC_REVERB_DAMPING:
                textPtr = kNumberDataLibrary[reverb.damping].nameLongR;
                break;

            case ENC_REVERB_PREDELAY:
                textPtr = kReverbPreDelayDataLibrary[reverb.preDelay].nameLongR;
                break;

            case ENC_REVERB_SURROUND:
                textPtr = kReverbSurroundDataLibrary[reverb.surround].nameLongR;
                break;

            case ENC_REVERB_DRY:
                textPtr = kNumberDataLibrary[reverb.dry].nameLongR;
                break;

            case ENC_REVERB_WET:
                textPtr = kNumberDataLibrary[reverb.wet].nameLongR;
                break;

            default:
                textPtr = kDataDashR;
                break;
        }
        // 4. VALUE DRAW
        // Draw resolved value text into slot.
        lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[encoderNum_], kMenuDataY[encoderNum_]);
    }
}

void Controller::lcd_updateGlobalMenuFunction(EncoderFunc function_) {
    /// @brief Refreshes global-menu headers for encoders using the given function.

    // 1. ENCODER SCAN
    // Redraw function headers for matching encoder bindings.
    for (uint8_t i = 0; i < kEncoderSize; i++) {
        if (encoder[i].function == function_) lcd_drawGlobalMenuFunction(i);
    }
}

void Controller::lcd_updateGlobalMenuData(EncoderFunc function_) {
    /// @brief Refreshes global-menu values for encoders using the given function.

    // 1. ENCODER SCAN
    // Redraw data fields for matching encoder bindings.
    for (uint8_t i = 0; i < kEncoderSize; i++) {
        if (encoder[i].function == function_) lcd_drawGlobalMenuData(i);
    }
}

/* Lcd File Menu Functions ---------------------------------------------------*/

void Controller::lcd_drawFileMenu() {
    /// @brief Draws File menu layout and operation rows.

    // 1. MENU ICON DRAW
    // Draw file menu icon area.
    lcd_drawMenuIcon(FILE_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw headers for New/Load/Save/Clear operations.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderNew, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderLoad, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderSave, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderClear, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText(kHeaderAutosave, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);

    // 4. SIGN/UNIT DRAW
    // Draw unit/sign text for each parameter slot.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);

    // 5. DATA DRAW
    // Draw tab-dependent data for all operation rows.
    lcd_drawFile_NewData();
    lcd_drawFile_LoadData();
    lcd_drawFile_SaveData();
    lcd_drawFile_ClearData();
    lcd_drawFile_AutosaveData();
    lcd_drawName_Row();
}

void Controller::lcd_drawFile_AutosaveData() {
    /// @brief Draws AUTOSAVE on/off (File menu, knob 5; v2.01).

    lcd_setMenuDataState(WHITE);
    lcd.drawText(autosaveEnabled ? kDataOn : kDataOff, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawFile_NewData() {
    /// @brief Draws File->New row value for current tab state.

    // 1. STYLE SETUP
    // Configure data text style.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw select marker when tab is active, dash otherwise.
    const char* text;
    (menuTab == 0) ? text = kDataSelect : text = kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawFile_LoadData() {
    /// @brief Draws File->Load row with selected file index.

    // v2.02: the slot's name (or FILE_007 / DRUM_007), slot number in the header.
    lcd_drawSlotLabel(1, false);
}

void Controller::lcd_drawFile_SaveData() {
    /// @brief Draws File->Save row with selected file index.

    // v2.02: the slot's name (or FILE_007 / DRUM_007), slot number in the header.
    lcd_drawSlotLabel(2, false);
}

void Controller::lcd_drawFile_ClearData() {
    /// @brief Draws File->Clear row with selected file index.

    // v2.02: the slot's name (or FILE_007 / DRUM_007), slot number in the header.
    lcd_drawSlotLabel(3, false);
}

/* Lcd Drumkit Menu Functions ------------------------------------------------*/

void Controller::lcd_drawDrumkitMenu() {
    /// @brief Draws Drumkit menu layout and operation rows.

    // 1. MENU ICON DRAW
    // Draw drumkit menu icon area.
    lcd_drawMenuIcon(DRUMKIT_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw headers for New/Load/Save/Clear operations.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderNew, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderLoad, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderSave, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderClear, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);

    // 4. SIGN/UNIT DRAW
    // Draw unit/sign text for each parameter slot.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);

    // 5. DATA DRAW
    // Draw tab-dependent data for all operation rows.
    lcd_drawDrumkit_NewData();
    lcd_drawDrumkit_LoadData();
    lcd_drawDrumkit_SaveData();
    lcd_drawDrumkit_ClearData();
    lcd_drawName_Row();
}

void Controller::lcd_drawDrumkit_NewData() {
    /// @brief Draws Drumkit->New row value for current tab state.

    // 1. STYLE SETUP
    // Configure data text style.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw select marker when tab is active, dash otherwise.
    const char* text;
    (menuTab == 0) ? text = kDataSelect : text = kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawDrumkit_LoadData() {
    /// @brief Draws Drumkit->Load row with selected drumkit index.

    // v2.02: the slot's name (or FILE_007 / DRUM_007), slot number in the header.
    lcd_drawSlotLabel(1, true);
}

void Controller::lcd_drawDrumkit_SaveData() {
    /// @brief Draws Drumkit->Save row with selected drumkit index.

    // v2.02: the slot's name (or FILE_007 / DRUM_007), slot number in the header.
    lcd_drawSlotLabel(2, true);
}

void Controller::lcd_drawDrumkit_ClearData() {
    /// @brief Draws Drumkit->Clear row with selected drumkit index.

    // v2.02: the slot's name (or FILE_007 / DRUM_007), slot number in the header.
    lcd_drawSlotLabel(3, true);
}

/* Lcd Mixer Menu Functions --------------------------------------------------*/

void Controller::lcd_drawMixerMenu() {
    /// @brief Draws Mixer menu layout with level, routing, and utility fields.

    // 1. MENU ICON DRAW
    // Draw mixer menu icon area.
    lcd_drawMenuIcon(MIXER_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw headers for all mixer parameters.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText("MAS-VOL   ", kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText("HP-VOL    ", kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText("LO-VOL    ", kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText("LI-VOL    ", kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText("LI-IN     ", kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderPan, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderLimiter, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderNormalize, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 4. SIGN/UNIT DRAW
    // Draw unit/sign text for each parameter slot.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText(" DB", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText(" DB", kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    lcd.drawText(" DB", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText(" LR", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
    lcd.drawText(" DB", kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    // 5. DATA DRAW
    // Draw value rows for all mixer controls.
    lcd_drawMixer_MasterVolumeData();
    lcd_drawMixer_HpVolumeData();
    lcd_drawMixer_LoVolumeData();
    lcd_drawMixer_LiVolumeData();
    lcd_drawMixer_LiInData();
    lcd_drawMixer_PanData();
    lcd_drawMixer_LimiterData();
    lcd_drawMixer_NormalizeData();
}

void Controller::lcd_drawMixer_MasterVolumeData() {
    /// @brief Draws master output volume value and mute indicator.

    // 1. STYLE SETUP
    // Configure data style for master volume field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current master volume value.
    lcd.drawText(kNumberDataLibrary[mixer.masterVolume].nameLongR, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);

    // 3. MUTE INDICATOR DRAW
    // Draw mute status with active/inactive color.
    (mixer.masterMute) ? lcd_setMenuNumState(kAccent2Color) : lcd_setMenuNumState(BLACK);
    lcd.drawText("    MUTE", kMenuTextBoxTextSize, kMenuTextBoxX[0], kMenuTextBoxY[0]);
}

void Controller::lcd_drawMixer_HpVolumeData() {
    /// @brief Draws headphone output volume value and mute indicator.

    // 1. STYLE SETUP
    // Configure data style for headphone volume field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current headphone volume value.
    lcd.drawText(kMixerHpVolumeDataLibrary[mixer.hpVolume].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);

    // 3. MUTE INDICATOR DRAW
    // Draw mute status with active/inactive color.
    (mixer.hpMute) ? lcd_setMenuNumState(kAccent2Color) : lcd_setMenuNumState(BLACK);
    lcd.drawText("    MUTE", kMenuTextBoxTextSize, kMenuTextBoxX[1], kMenuTextBoxY[1]);
}

void Controller::lcd_drawMixer_LoVolumeData() {
    /// @brief Draws line output volume value and mute indicator.

    // 1. STYLE SETUP
    // Configure data style for line output volume field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current line output volume value.
    lcd.drawText(kMixerLoVolumeDataLibrary[mixer.loVolume].nameLongR, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[2]);

    // 3. MUTE INDICATOR DRAW
    // Draw mute status with active/inactive color.
    (mixer.loMute) ? lcd_setMenuNumState(kAccent2Color) : lcd_setMenuNumState(BLACK);
    lcd.drawText("    MUTE", kMenuTextBoxTextSize, kMenuTextBoxX[2], kMenuTextBoxY[2]);
}

void Controller::lcd_drawMixer_LiVolumeData() {
    /// @brief Draws line input volume value and mute indicator.

    // 1. STYLE SETUP
    // Configure data style for line input volume field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current line input volume value.
    lcd.drawText(kMixerLiVolumeDataLibrary[mixer.liVolume].nameLongR, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[3]);

    // 3. MUTE INDICATOR DRAW
    // Draw mute status with active/inactive color.
    (mixer.liMute) ? lcd_setMenuNumState(kAccent2Color) : lcd_setMenuNumState(BLACK);
    lcd.drawText("    MUTE", kMenuTextBoxTextSize, kMenuTextBoxX[3], kMenuTextBoxY[3]);
}

void Controller::lcd_drawMixer_LiInData() {
    /// @brief Draws line-input routing mode (Direct/Process).

    // 1. STYLE SETUP
    // Configure data style for line-input mode field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current line-input mode value.
    lcd.drawText(kMixerLiMixerInDataLibrary[mixer.liMixerIn].nameLongR, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawMixer_PanData() {
    /// @brief Draws stereo pan position value.

    // 1. STYLE SETUP
    // Configure data style for pan field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current pan value.
    lcd.drawText(kMixerPanDataLibrary[mixer.pan].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawMixer_LimiterData() {
    /// @brief Draws limiter value field.

    // 1. STYLE SETUP
    // Configure data style for limiter field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current limiter value.
    lcd.drawText(kMixerLimiterDataLibrary[mixer.limiter].nameLongR, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawMixer_NormalizeData() {
    /// @brief Draws normalization enable/disable state.

    // 1. STYLE SETUP
    // Configure data style for normalize field.
    lcd_setMenuDataState(WHITE);

    // 2. STATE DRAW
    // Draw normalized ON/OFF text.
    const char* textPtr;
    (mixer.normalize) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, kMenuDataX[7], kMenuDataY[7]);
}

/* Lcd Rhythm Menu Functions -------------------------------------------------*/

void Controller::lcd_drawRhythmMenu() {
    /// @brief Draws Rhythm menu layout and timing parameter fields.

    // 1. MENU ICON DRAW
    // Draw rhythm menu icon area.
    lcd_drawMenuIcon(RHYTHM_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw headers for tempo/measure/bar/quantize.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderTempo, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderMeasure, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderBar, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderQuantize, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText(kHeaderSong, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderSongStep, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderSongPart, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderSongRepeat, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 4. SIGN/UNIT DRAW
    // Draw unit/sign text for each parameter slot.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
    lcd.drawText("  X", kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    // 5. DATA DRAW
    // Draw rhythm parameter values.
    lcd_drawRhythm_TempoData();
    lcd_drawRhythm_MeasureData();
    lcd_drawRhythm_BarData();
    lcd_drawRhythm_QuantizeData();
    lcd_drawRhythm_SongData();
    lcd_drawRhythm_SongStepData();
    lcd_drawRhythm_SongPartData();
    lcd_drawRhythm_SongRepeatData();
}

/* Song mode fields (knobs 5-8) */

void Controller::lcd_drawRhythm_SongData() {
    /// @brief SONG: the mode, plus the playing step while the chain runs (e.g. "LOOP 3/6").

    char text[kMenuDataTextSize + 1];
    char value[24];
    const uint8_t mode = (songChain.mode <= songchain::ONCE) ? songChain.mode : songchain::OFF;
    if (songChainRunning) {
        snprintf(value, sizeof(value), "%s %u/%u", kSongModeText[mode], songPosition.step + 1, songchain::length(songChain));
    } else {
        snprintf(value, sizeof(value), "%s", kSongModeText[mode]);
    }
    snprintf(text, sizeof(text), "%10.10s", value);
    lcd_setMenuDataState(WHITE);
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawRhythm_SongStepData() {
    /// @brief STEP: the chain step being edited and the chain length (e.g. "05/06").

    char text[kMenuDataTextSize + 1];
    char value[24];
    const uint8_t length = songchain::length(songChain);
    if (length > 0) {
        snprintf(value, sizeof(value), "%02u/%02u", songEditStep + 1, length);
    } else {
        snprintf(value, sizeof(value), "%02u/--", songEditStep + 1);
    }
    snprintf(text, sizeof(text), "%10.10s", value);
    lcd_setMenuDataState(WHITE);
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawRhythm_SongPartData() {
    /// @brief PART: the part (song bank 1-5) of the edited step, or END; with a TRANSPOSE
    /// (v2.03, ADD + this knob) also its scale steps, e.g. "  2  KEY+3".

    const songchain::Step& step = songChain.steps[songEditStep];
    lcd_setMenuDataState(WHITE);
    if (step.part >= songchain::kParts) {
        lcd.drawText("       END", kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
        return;
    }
    char text[kMenuDataTextSize + 1] = "          ";
    text[kMenuDataTextSize - 1] = static_cast<char>('1' + step.part);
    if (step.transpose != 0) {
        // "  2  KEY+3": part 2, three scale steps up
        const int t = (step.transpose < 0) ? -step.transpose : step.transpose;
        text[kMenuDataTextSize - 1] = ' ';
        text[2] = static_cast<char>('1' + step.part);
        memcpy(text + 3, "  KEY", 5);
        text[8] = (step.transpose < 0) ? '-' : '+';
        text[9] = static_cast<char>('0' + t);
    }
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawRhythm_SongRepeatData() {
    /// @brief REPEAT: how many loops the edited step plays ("--" for END).

    const songchain::Step& step = songChain.steps[songEditStep];
    lcd_setMenuDataState(WHITE);
    lcd.drawText((step.part < songchain::kParts) ? kNumberDataLibrary[step.repeats].nameLongR : kDataDashR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);
}

void Controller::lcd_drawRhythm_TempoData() {
    /// @brief Draws tempo value and sync role indicator.

    // 1. STYLE SETUP
    // Configure data style for tempo field.
    lcd_setMenuDataState(WHITE);

    // 2. TEMPO DRAW
    // Format and draw current tempo value.
    char kData[kMenuDataTextSize];
    sprintf(kData, "       %03d", rhythm.tempo);
    lcd.drawText(kData, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);

    // 3. SYNC INDICATOR DRAW
    // Priority: any input clock source -> SLAVE, otherwise any output clock source -> MASTER.
    bool slaveModeActive = (io.syncIn != 0) || (io.midiIn != 0);
    bool masterModeActive = !slaveModeActive && ((io.syncOut != 0) || (io.midiOut != 0));

    if (slaveModeActive) {
        lcd_setMenuTextState(YELLOW);
        lcd.drawText("   SLAVE", kMenuTextBoxTextSize, kMenuTextBoxX[0], kMenuTextBoxY[0]);
    } else if (masterModeActive) {
        lcd_setMenuTextState(YELLOW);
        lcd.drawText("  MASTER", kMenuTextBoxTextSize, kMenuTextBoxX[0], kMenuTextBoxY[0]);
    } else {
        // Clear stale text when all sync/midi routes are off.
        lcd_setMenuTextState(BLACK);
        lcd.drawText("        ", kMenuTextBoxTextSize, kMenuTextBoxX[0], kMenuTextBoxY[0]);
    }
}

void Controller::lcd_drawRhythm_MeasureData() {
    /// @brief Draws measure value and lock indicator.

    // 1. STYLE SETUP
    // Configure data style for measure field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current measure value.
    lcd.drawText(kNumberDataLibrary[rhythm.measure].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);

    // 3. LOCK INDICATOR DRAW
    // Draw LOCK status with active/inactive color.
    (rhythm.measureLock) ? lcd_setMenuTextState(kAccent2Color) : lcd_setMenuTextState(BLACK);
    lcd.drawText("    LOCK", kMenuTextBoxTextSize, kMenuTextBoxX[1], kMenuTextBoxY[1]);
}

void Controller::lcd_drawRhythm_BarData() {
    /// @brief Draws bar value and lock indicator.

    // 1. STYLE SETUP
    // Configure data style for bar field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current bar value.
    lcd.drawText(kNumberDataLibrary[rhythm.bar].nameLongR, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[2]);

    // 3. LOCK INDICATOR DRAW
    // Draw LOCK status with active/inactive color.
    (rhythm.barLock) ? lcd_setMenuTextState(kAccent2Color) : lcd_setMenuTextState(BLACK);
    lcd.drawText("    LOCK", kMenuTextBoxTextSize, kMenuTextBoxX[2], kMenuTextBoxY[2]);
}

void Controller::lcd_drawRhythm_QuantizeData() {
    /// @brief Draws quantize value and lock indicator.

    // 1. STYLE SETUP
    // Configure data style for quantize field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current quantize value.
    lcd.drawText(kQuantizeDataLibrary[rhythm.quantize].nameLongR, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[3]);

    // 3. LOCK INDICATOR DRAW
    // Draw LOCK status with active/inactive color.
    (rhythm.quantizeLock) ? lcd_setMenuTextState(kAccent2Color) : lcd_setMenuTextState(BLACK);
    lcd.drawText("    LOCK", kMenuTextBoxTextSize, kMenuTextBoxX[3], kMenuTextBoxY[3]);
}

/* Lcd Metronome Menu Functions ----------------------------------------------*/

void Controller::lcd_drawMetroMenu() {
    /// @brief Draws metronome menu layout and control values.

    // 1. MENU ICON DRAW
    // Draw metronome menu icon area.
    lcd_drawMenuIcon(METRO_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw headers for active/precount/sample/volume fields.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderPrecount, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderSample, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderVolume, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText(kHeaderPreview, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderPreviewLevel, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);

    // 4. SIGN/UNIT DRAW
    // Draw unit/sign text for each parameter slot.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);

    // 5. DATA DRAW
    // Draw metronome parameter values.
    lcd_drawMetro_ActiveData();
    lcd_drawMetro_PrecountData();
    lcd_drawMetro_SampleData();
    lcd_drawMetro_VolumeData();
    lcd_drawMetro_PreviewData();
    lcd_drawMetro_PreviewLevelData();
}

void Controller::lcd_drawMetro_PreviewData() {
    /// @brief Draws the sample preview on/off value (knob 5).

    lcd_setMenuDataState(WHITE);
    lcd.drawText(previewActive ? kDataOn : kDataOff, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawMetro_PreviewLevelData() {
    /// @brief Draws the sample preview level (knob 6).

    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[previewLevel].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawMetro_ActiveData() {
    /// @brief Draws metronome active state value.

    // 1. STYLE SETUP
    // Configure data style for active field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state.
    const char* textPtr;
    (metronome.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawMetro_PrecountData() {
    /// @brief Draws metronome precount state value.

    // 1. STYLE SETUP
    // Configure data style for precount field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state.
    const char* textPtr;
    (metronome.precount) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);
}

void Controller::lcd_drawMetro_SampleData() {
    /// @brief Draws selected metronome sample value.

    // 1. STYLE SETUP
    // Configure data style for sample field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw selected metronome sample text.
    lcd.drawText(kMetroSampleDataLibrary[metronome.sample].nameLongR, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[2]);
}

void Controller::lcd_drawMetro_VolumeData() {
    /// @brief Draws metronome volume value.

    // 1. STYLE SETUP
    // Configure data style for volume field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw current metronome volume text.
    lcd.drawText(kNumberDataLibrary[metronome.volume].nameLongR, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[3]);
}

/* Lcd Io Menu Functions -----------------------------------------------------*/

void Controller::lcd_drawIoMenu() {
    /// @brief Draws I/O menu layout and sync/MIDI values.

    // 1. MENU ICON DRAW
    // Draw I/O menu icon area.
    lcd_drawMenuIcon(IO_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw headers for sync/MIDI fields.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderSyncIn, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderSyncOut, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderMidiIn, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderMidiOut, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText(kHeaderCpuLoad, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderCpuPeak, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);

    // 4. DATA DRAW
    // Draw current sync/MIDI values. Sign boxes 0 and 1 are managed by SyncInPpqnData/SyncOutPpqnData.
    lcd_drawIo_SyncInData();
    lcd_drawIo_SyncOutData();
    lcd_drawIo_MidiInData();
    lcd_drawIo_MidiOutData();
    cpuShownLoad = 0xFFFF;  // force the CPU values to be drawn now
    cpuShownPeak = 0xFFFF;
    lcd_drawIo_CpuData();
    lcd_drawIo_CpuPeakData();
}

namespace {

// Per mille -> right-aligned "23.4" in a 10-character data field.
void formatPermille(uint16_t permille, char* text, size_t size) {
    if (permille > 9999) permille = 9999;
    char value[12];
    snprintf(value, sizeof(value), "%u.%u", static_cast<unsigned>(permille / 10), static_cast<unsigned>(permille % 10));
    snprintf(text, size, "%10.10s", value);
}

}  // namespace

void Controller::lcd_drawIo_CpuData() {
    /// @brief Draws the average audio CPU load (knob 5, read-only).

    const uint16_t value = cpuLoadPermille;
    if (value == cpuShownLoad) return;
    cpuShownLoad = value;
    char text[kMenuDataTextSize + 1];
    formatPermille(value, text, sizeof(text));
    lcd_setMenuDataState(WHITE);
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawIo_CpuPeakData() {
    /// @brief Draws the highest audio CPU load of a single block (knob 6; turning it resets).
    /// Red from 80 %: above 100 % the audio ISR misses its deadline and the output clicks.

    const uint16_t value = cpuPeakPermille;
    if (value == cpuShownPeak) return;
    cpuShownPeak = value;
    char text[kMenuDataTextSize + 1];
    formatPermille(value, text, sizeof(text));
    lcd_setMenuDataState((value >= 800) ? RED : WHITE);
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawIo_SyncInData() {
    /// @brief Draws sync input mode name and PPQN (when in MODE-A).

    // 1. MODE NAME DRAW
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kIoSyncInDataLibrary[io.syncIn].nameLongR, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[2]);

    // 2. PPQN DRAW (sign field, only in MODE-A)
    lcd_drawIo_SyncInPpqnData();
}

void Controller::lcd_drawIo_SyncOutData() {
    /// @brief Draws sync output mode name and PPQN (when in MODE-A/MODE-B).

    // 1. MODE NAME DRAW
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kIoSyncOutDataLibrary[io.syncOut].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[3]);

    // 2. PPQN DRAW (sign field, only in MODE-A/MODE-B)
    lcd_drawIo_SyncOutPpqnData();
}

void Controller::lcd_drawIo_SyncInPpqnData() {
    /// @brief Draws the Sync-In PPQN value in the sign field (MODE-A only).
    if (io.syncIn > 0) {
        char buf[5];
        const uint8_t syncInPpqnIdx = (io.syncInPpqn <= kMaxIoSyncPpqn) ? io.syncInPpqn : kInitialIoSyncInPpqn;
        sprintf(buf, " %02d", kIoSyncPpqnDataLibrary[syncInPpqnIdx].ppqn);
        lcd_setMenuSignState(kAccentColor);
        lcd.drawText(buf, kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    } else {
        lcd_clearSignBox(0);
    }
}

void Controller::lcd_drawIo_SyncOutPpqnData() {
    /// @brief Draws the Sync-Out PPQN value in the sign field (MODE-A/MODE-B only).
    if (io.syncOut > 0) {
        char buf[5];
        const uint8_t syncOutPpqnIdx = (io.syncOutPpqn <= kMaxIoSyncPpqn) ? io.syncOutPpqn : kInitialIoSyncOutPpqn;
        sprintf(buf, " %02d", kIoSyncPpqnDataLibrary[syncOutPpqnIdx].ppqn);
        lcd_setMenuSignState(kAccentColor);
        lcd.drawText(buf, kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    } else {
        lcd_clearSignBox(1);
    }
}

void Controller::lcd_drawIo_MidiInData() {
    /// @brief Draws MIDI input mode name and channel number.

    // 1. MODE NAME DRAW
    // Draw current MIDI-IN mode text in the data field.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kIoMidiModeDataLibrary[io.midiIn].nameLongR, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[0]);

    // 2. CHANNEL NUMBER DRAW
    // Show channel number in sign field only when MIDI-In is active (not OFF).
    if (io.midiIn != 0) {
        char chanBuf[4];
        sprintf(chanBuf, " %02d", io.midiInChannel + 1);
        lcd_setMenuSignState(kAccentColor);
        lcd.drawText(chanBuf, kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    } else {
        lcd_clearSignBox(2);
    }
}

void Controller::lcd_drawIo_MidiOutData() {
    /// @brief Draws MIDI output mode name and channel number.

    // 1. MODE NAME DRAW
    // Draw current MIDI-OUT mode text in the data field.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kIoMidiModeDataLibrary[io.midiOut].nameLongR, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[1]);

    // 2. CHANNEL NUMBER DRAW
    // Show channel number in sign field only when MIDI-Out is active (not OFF).
    if (io.midiOut != 0) {
        char chanBuf[4];
        sprintf(chanBuf, " %02d", io.midiOutChannel + 1);
        lcd_setMenuSignState(kAccentColor);
        lcd.drawText(chanBuf, kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    } else {
        lcd_clearSignBox(3);
    }
}

/* Lcd Filter Menu Functions -------------------------------------------------*/

void Controller::lcd_drawFilterMenu(uint8_t filterNum_) {
    /// @brief Draws filter menu layout and parameter values for selected filter.

    // 1. MENU ICON DRAW
    // Draw filter menu icon for selected filter index.
    (filterNum_ == 0) ? lcd_drawMenuIcon(FILTER_0_MENU) : lcd_drawMenuIcon(FILTER_1_MENU);

    // 2. FILTER NUMBER DRAW
    // Draw selected filter number marker.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + filterNum_, 1, kMenuNumX, kMenuNumY);

    // 3. HEADER DRAW
    // Draw headers for filter parameters.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderType, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderSlope, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderFrequency, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText(kHeaderResonance, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderDrive, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderDry, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderWet, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 4. SIGN DRAW
    // Draw unit/sign labels for each parameter field.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText(" DB", kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    lcd.drawText(" HZ", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    // 5. DATA DRAW
    // Draw current filter parameter values.
    lcd_drawFilter_ActiveData(filterNum_);
    lcd_drawFilter_TypeData(filterNum_);
    lcd_drawFilter_SlopeData(filterNum_);
    lcd_drawFilter_FrequencyData(filterNum_);
    lcd_drawFilter_ResonanceData(filterNum_);
    lcd_drawFilter_DriveData(filterNum_);
    lcd_drawFilter_DryData(filterNum_);
    lcd_drawFilter_WetData(filterNum_);
}

void Controller::lcd_drawFilter_ActiveData(uint8_t filterNum_) {
    /// @brief Draws filter active state value.

    // 1. STYLE SETUP
    // Configure data style for active field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state.
    const char* textPtr;
    (filter[filterNum_].active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawFilter_TypeData(uint8_t filterNum_) {
    /// @brief Draws filter type value.

    // 1. STYLE SETUP
    // Configure data style for type field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw filter type text.
    lcd.drawText(kFilterTypeDataLibrary[filter[filterNum_].type].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);
}

void Controller::lcd_drawFilter_SlopeData(uint8_t filterNum_) {
    /// @brief Draws filter slope value.

    // 1. STYLE SETUP
    // Configure data style for slope field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw filter slope text.
    lcd.drawText(kFilterSlopeDataLibrary[filter[filterNum_].slope].nameLongR, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[2]);
}

void Controller::lcd_drawFilter_FrequencyData(uint8_t filterNum_) {
    /// @brief Draws filter cutoff frequency value and dynamic unit label.

    // 1. UNIT DRAW
    // Draw HZ/KHZ sign according to current frequency magnitude.
    lcd_setMenuSignState(kAccentColor);

    // Draw frequency unit (HZ or KHZ)
    (kFilterFrequencyDataLibrary[filter[filterNum_].frequency].data < 1000) ? lcd.drawText(" HZ", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]) : lcd.drawText("KHZ", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    // 2. VALUE DRAW
    // Draw filter frequency text.
    lcd_setMenuDataState(WHITE);

    // Draw frequency value
    lcd.drawText(kFilterFrequencyDataLibrary[filter[filterNum_].frequency].nameLongR, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[3]);
}

void Controller::lcd_drawFilter_ResonanceData(uint8_t filterNum_) {
    /// @brief Draws filter resonance value.

    // 1. STYLE SETUP
    // Configure data style for resonance field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw resonance text.
    lcd.drawText(kNumberDataLibrary[filter[filterNum_].resonance].nameLongR, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawFilter_DriveData(uint8_t filterNum_) {
    /// @brief Draws filter drive value.

    // 1. STYLE SETUP
    // Configure data style for drive field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw drive text.
    lcd.drawText(kNumberDataLibrary[filter[filterNum_].drive].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawFilter_DryData(uint8_t filterNum_) {
    /// @brief Draws dry-mix value.

    // 1. STYLE SETUP
    // Configure data style for dry field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw dry-mix text.
    lcd.drawText(kNumberDataLibrary[filter[filterNum_].dry].nameLongR, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawFilter_WetData(uint8_t filterNum_) {
    /// @brief Draws wet-mix value.

    // 1. STYLE SETUP
    // Configure data style for wet field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw wet-mix text.
    lcd.drawText(kNumberDataLibrary[filter[filterNum_].wet].nameLongR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);
}

/* Lcd Eq Menu Functions -----------------------------------------------------*/

void Controller::lcd_drawEqMenu() {
    /// @brief Draws EQ menu layout, values, and shelf/peak mini graphics.

    // 1. MENU ICON DRAW
    // Draw EQ menu icon area.
    lcd_drawMenuIcon(EQ_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw headers for EQ bands and controls.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderQfactor, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderLowshelf, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderHighshelf, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText(kHeaderPeak1, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderPeak2, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderPeak3, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderPeak4, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 4. SIGN/UNIT DRAW
    // Draw unit/sign text for each parameter slot.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);

    // 5. DATA DRAW
    // Draw EQ parameter values.
    lcd_drawEq_ActiveData();
    lcd_drawEq_QData();
    lcd_drawEq_LowShelfData();
    lcd_drawEq_HighShelfData();
    for (uint8_t i = 0; i < 4; i++) {
        lcd_drawEq_PeakData(i);
    }

    // 6. ICON/GRAPH DRAW
    // Draw shelf/peak symbols beside EQ value fields.
    lcd.setForeColor(kAccentColor);
    uint16_t xPos;
    uint16_t yPos;

    // low shelf icon
    xPos = kMenuTextBoxX[2] - 20;
    yPos = kMenuTextBoxY[2];
    lcd.drawHLine(xPos, yPos, 6);
    lcd.drawLine(xPos + 5, yPos, xPos + 10, yPos + 5);
    lcd.drawHLine(xPos + 10, yPos + 5, 6);

    // high shelf icon
    xPos = kMenuTextBoxX[3] - 20;
    yPos = kMenuTextBoxY[3];
    lcd.drawHLine(xPos, yPos + 5, 6);
    lcd.drawLine(xPos + 5, yPos + 5, xPos + 10, yPos);
    lcd.drawHLine(xPos + 10, yPos, 6);

    // peak filter icon
    for (uint8_t i = 4; i < 8; i++) {
        xPos = kMenuTextBoxX[i] - 24;
        yPos = kMenuTextBoxY[i];
        lcd.drawHLine(xPos, yPos + 5, 5);
        lcd.drawLine(xPos + 4, yPos + 5, xPos + 9, yPos);
        lcd.drawLine(xPos + 9, yPos, xPos + 14, yPos + 5);
        lcd.drawHLine(xPos + 14, yPos + 5, 5);
    }
}

void Controller::lcd_drawEq_ActiveData() {
    /// @brief Draws EQ active state value.

    // 1. STYLE SETUP
    // Configure data style for active field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state.
    const char* textPtr;
    (eq.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawEq_QData() {
    /// @brief Draws EQ Q-factor value.

    // 1. STYLE SETUP
    // Configure data style for Q field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw Q-factor text.
    lcd.drawText(kEqQDataLibrary[eq.q].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);
}

void Controller::lcd_drawEq_LowShelfData() {
    /// @brief Draws low-shelf frequency sign and gain value.

    // 1. FREQUENCY SIGN DRAW
    // Draw low-shelf frequency label.
    lcd_setMenuSignState(GRAY_75);
    lcd.drawText(kEqFreqDataLibrary[eq.freqLowShelf].nameLongR, kMenuDataTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);

    // 2. GAIN VALUE DRAW
    // Draw low-shelf gain text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kEqGainDataLibrary[eq.gainLowShelf].nameLongR, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[2]);
}

void Controller::lcd_drawEq_HighShelfData() {
    /// @brief Draws high-shelf frequency sign and gain value.

    // 1. FREQUENCY SIGN DRAW
    // Draw high-shelf frequency label.
    lcd_setMenuSignState(GRAY_75);
    lcd.drawText(kEqFreqDataLibrary[eq.freqHighShelf].nameLongR, kMenuDataTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);

    // 2. GAIN VALUE DRAW
    // Draw high-shelf gain text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kEqGainDataLibrary[eq.gainHighShelf].nameLongR, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[3]);
}

void Controller::lcd_drawEq_PeakData(uint8_t peakNum_) {
    /// @brief Draws frequency sign and gain value for selected peak band.

    // 1. FREQUENCY SIGN DRAW
    // Draw peak-band frequency label.
    lcd_setMenuSignState(GRAY_75);
    lcd.drawText(kEqFreqDataLibrary[eq.freqPeak[peakNum_]].nameLongR, kMenuDataTextSize, kMenuSignBoxX[peakNum_ + 4], kMenuSignBoxY[peakNum_ + 4]);

    // 2. GAIN VALUE DRAW
    // Draw peak-band gain text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kEqGainDataLibrary[eq.gainPeak[peakNum_]].nameLongR, kMenuDataTextSize, kMenuDataX[peakNum_ + 4], kMenuDataY[peakNum_ + 4]);
}

/* Lcd Effect Menu Functions -------------------------------------------------*/

void Controller::lcd_drawEffectMenu(uint8_t effectNum_) {
    /// @brief Draws Effect menu layout and parameter values for selected effect unit.

    // 1. MENU ICON DRAW
    // Draw effect menu icon for selected effect index.
    (effectNum_ == 0) ? lcd_drawMenuIcon(EFFECT_0_MENU) : lcd_drawMenuIcon(EFFECT_1_MENU);

    // 2. EFFECT NUMBER DRAW
    // Draw selected effect number marker.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + effectNum_, 1, kMenuNumX, kMenuNumY);

    // 3. HEADER DRAW
    // Draw active header and parameter labels.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);

    // 4. SIGN/UNIT DRAW
    // Draw unit/sign text for each parameter slot.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);

    // 5. DATA DRAW
    // Draw effect active/type and A-F parameter values.
    lcd_drawEffect_ActiveData(effectNum_);
    lcd_drawEffect_TypeData(effectNum_);
    lcd_drawEffect_AData(effectNum_);
    lcd_drawEffect_BData(effectNum_);
    lcd_drawEffect_CData(effectNum_);
    lcd_drawEffect_DData(effectNum_);
    lcd_drawEffect_EData(effectNum_);
    lcd_drawEffect_FData(effectNum_);
}

void Controller::lcd_drawEffect_ActiveData(uint8_t effectNum_) {
    /// @brief Draws effect active state value.

    // 1. STYLE SETUP
    // Configure data style for active field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state.
    const char* textPtr;
    (effect[effectNum_].active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawEffect_TypeData(uint8_t effectNum_) {
    /// @brief Draws effect type header/sign and value.

    // 1. HEADER/SIGN DRAW
    // Draw TYPE header and sign field.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText("TYPE      ", kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuDataX[1], kMenuSignBoxY[1]);
    // 2. VALUE DRAW
    // Draw selected effect type value.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kEffectTypeDataLibrary[effect[effectNum_].type].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);
}

void Controller::lcd_drawEffect_AData(uint8_t effectNum_) {
    /// @brief Draws effect parameter A header/sign and value.

    Effect& effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    // 1. HEADER/SIGN DRAW
    // Draw parameter-A header and sign labels.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(aHeader[type_], kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(aSign[type_], kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);

    // 2. VALUE RESOLUTION
    // Resolve parameter-A value text based on effect type.
    lcd_setMenuDataState(WHITE);
    const char* textPtr;
    switch (type_) {
        case EF_DELAY:
            textPtr = kDelayTimeDataLibrary[effect_.delay.aTime].nameLongR;
            break;

        case EF_CHORUS:
            textPtr = kChorusTimeDataLibrary[effect_.chorus.aTime].nameLongR;
            break;

        case EF_FLANGER:
            textPtr = kFlangerTimeDataLibrary[effect_.flanger.aTime].nameLongR;
            break;

        case EF_PHASER:
            textPtr = kPhaserFreqDataLibrary[effect_.phaser.aStartFreq].nameLongR;
            break;

        case EF_COMPRESSOR:
            textPtr = kCompressorThresholdDataLibrary[effect_.compressor.aThreshold].nameLongR;
            break;

        case EF_EXPANDER:
            textPtr = kExpanderThresholdDataLibrary[effect_.expander.aThreshold].nameLongR;
            break;

        case EF_OVERDRIVE:
            textPtr = kOverdriveGainDataLibrary[effect_.overdrive.aGain].nameLongR;
            break;

        case EF_DISTORTION:
            textPtr = kDistortionGainDataLibrary[effect_.distortion.aGain].nameLongR;
            break;

        case EF_BITCRUSHER:
            textPtr = kBitcrusherResolutionDataLibrary[effect_.bitcrusher.aResolution].nameLongR;
            break;

        default:
            textPtr = "----------";
            break;
    }
    // 3. VALUE DRAW
    // Draw resolved parameter-A value.
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[2]);
}

void Controller::lcd_drawEffect_BData(uint8_t effectNum_) {
    /// @brief Draws effect parameter B header/sign and value.

    Effect& effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    // 1. HEADER/SIGN DRAW
    // Draw parameter-B header and sign labels.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(bHeader[type_], kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(bSign[type_], kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);

    // 2. VALUE RESOLUTION
    // Resolve parameter-B value text based on effect type.
    lcd_setMenuDataState(WHITE);
    const char* textPtr;
    switch (type_) {
        case EF_DELAY:
            textPtr = kNumberDataLibrary[effect_.delay.bLevel].nameLongR;
            break;

        case EF_CHORUS:
            textPtr = kChorusFeedbackDataLibrary[effect_.chorus.bFeedback].nameLongR;
            break;

        case EF_FLANGER:
            textPtr = kFlangerFeedbackDataLibrary[effect_.flanger.bFeedback].nameLongR;
            break;

        case EF_PHASER:
            textPtr = kPhaserFreqDataLibrary[effect_.phaser.bEndFreq].nameLongR;
            break;

        case EF_COMPRESSOR:
            textPtr = kCompressorRateDataLibrary[effect_.compressor.bRate].nameLongR;
            break;

        case EF_EXPANDER:
            textPtr = kExpanderRateDataLibrary[effect_.expander.bRate].nameLongR;
            break;

        case EF_OVERDRIVE:
            textPtr = kOverdriveThresholdDataLibrary[effect_.overdrive.bThreshold].nameLongR;
            break;

        case EF_DISTORTION:
            textPtr = kDistortionThresholdDataLibrary[effect_.distortion.bThreshold].nameLongR;
            break;

        case EF_BITCRUSHER:
            textPtr = kBitcrusherSampleRateDataLibrary[effect_.bitcrusher.bSampleRate].nameLongR;
            break;
    }
    // 3. VALUE DRAW
    // Draw resolved parameter-B value.
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[3]);
}

void Controller::lcd_drawEffect_CData(uint8_t effectNum_) {
    /// @brief Draws effect parameter C header/sign and value.

    Effect& effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    // 1. HEADER/SIGN DRAW
    // Draw parameter-C header and sign labels.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(cHeader[type_], kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(cSign[type_], kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);

    // 2. VALUE RESOLUTION
    // Resolve parameter-C value text based on effect type.
    lcd_setMenuDataState(WHITE);
    const char* textPtr;
    switch (type_) {
        case EF_DELAY:
            textPtr = kNumberDataLibrary[effect_.delay.cFeedback].nameLongR;
            break;

        case EF_CHORUS:
            textPtr = kChorusRateDataLibrary[effect_.chorus.cRate].nameLongR;
            break;

        case EF_FLANGER:
            textPtr = kFlangerRateDataLibrary[effect_.flanger.cRate].nameLongR;
            break;

        case EF_PHASER:
            textPtr = kPhaserRateDataLibrary[effect_.phaser.cRate].nameLongR;
            break;

        case EF_COMPRESSOR:
            textPtr = kCompressorAttackTimeDataLibrary[effect_.compressor.cAttackTime].nameLongR;
            break;

        case EF_EXPANDER:
            textPtr = kExpanderAttackTimeDataLibrary[effect_.expander.cAttackTime].nameLongR;
            break;

        case EF_OVERDRIVE:
            textPtr = kOverdriveToneDataLibrary[effect_.overdrive.cTone].nameLongR;
            break;

        case EF_DISTORTION:
            textPtr = kDistortionToneDataLibrary[effect_.distortion.cTone].nameLongR;
            break;

        case EF_BITCRUSHER:
            textPtr = kBitcrusherThresholdDataLibrary[effect_.bitcrusher.cThreshold].nameLongR;
            break;
    }
    // 3. VALUE DRAW
    // Draw resolved parameter-C value.
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawEffect_DData(uint8_t effectNum_) {
    /// @brief Draws effect parameter D header/sign and value.

    Effect& effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    // 1. HEADER/SIGN DRAW
    // Draw parameter-D header and sign labels.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(dHeader[type_], kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(dSign[type_], kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);

    // 2. VALUE RESOLUTION
    // Resolve parameter-D value text based on effect type.
    lcd_setMenuDataState(WHITE);
    const char* textPtr;
    switch (type_) {
        case EF_DELAY:
            textPtr = kNumberDataLibrary[effect_.delay.dDamping].nameLongR;
            break;

        case EF_CHORUS:
            textPtr = kNumberDataLibrary[effect_.chorus.dDepth].nameLongR;
            break;

        case EF_FLANGER:
            textPtr = kNumberDataLibrary[effect_.flanger.dWidth].nameLongR;
            break;

        case EF_PHASER:
            textPtr = kNumberDataLibrary[effect_.phaser.dFeedback].nameLongR;
            break;

        case EF_COMPRESSOR:
            textPtr = kCompressorReleaseTimeDataLibrary[effect_.compressor.dReleaseTime].nameLongR;
            break;

        case EF_EXPANDER:
            textPtr = kExpanderReleaseTimeDataLibrary[effect_.expander.dReleaseTime].nameLongR;
            break;

        case EF_OVERDRIVE:
            textPtr = kNumberDataLibrary[effect_.overdrive.dDriveColor].nameLongR;
            break;

        case EF_DISTORTION:
            textPtr = kNumberDataLibrary[effect_.distortion.dAsym].nameLongR;
            break;

        case EF_BITCRUSHER:
            textPtr = kNumberDataLibrary[effect_.bitcrusher.dSmooth].nameLongR;
            break;
    }
    // 3. VALUE DRAW
    // Draw resolved parameter-D value.
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawEffect_EData(uint8_t effectNum_) {
    /// @brief Draws effect parameter E header/sign and value.

    Effect& effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    // 1. HEADER/SIGN DRAW
    // Draw parameter-E header and sign labels.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(eHeader[type_], kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(eSign[type_], kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);

    // 2. VALUE RESOLUTION
    // Resolve parameter-E value text based on effect type.
    lcd_setMenuDataState(WHITE);
    const char* textPtr;
    switch (type_) {
        case EF_DELAY:
            textPtr = kNumberDataLibrary[effect_.delay.eDry].nameLongR;
            break;

        case EF_CHORUS:
            textPtr = kNumberDataLibrary[effect_.chorus.eDry].nameLongR;
            break;

        case EF_FLANGER:
            textPtr = kNumberDataLibrary[effect_.flanger.eDry].nameLongR;
            break;

        case EF_PHASER:
            textPtr = kNumberDataLibrary[effect_.phaser.eDry].nameLongR;
            break;

        case EF_COMPRESSOR:
            textPtr = kNumberDataLibrary[effect_.compressor.eDry].nameLongR;
            break;

        case EF_EXPANDER:
            textPtr = kNumberDataLibrary[effect_.expander.eDry].nameLongR;
            break;

        case EF_OVERDRIVE:
            textPtr = kNumberDataLibrary[effect_.overdrive.eDry].nameLongR;
            break;

        case EF_DISTORTION:
            textPtr = kNumberDataLibrary[effect_.distortion.eDry].nameLongR;
            break;

        case EF_BITCRUSHER:
            textPtr = kNumberDataLibrary[effect_.bitcrusher.eDry].nameLongR;
            break;
    }
    // 3. VALUE DRAW
    // Draw resolved parameter-E value.
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawEffect_FData(uint8_t effectNum_) {
    /// @brief Draws effect parameter F header/sign and value.

    Effect& effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    // 1. HEADER/SIGN DRAW
    // Draw parameter-F header and sign labels.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(fHeader[type_], kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(fSign[type_], kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    // 2. VALUE RESOLUTION
    // Resolve parameter-F value text based on effect type.
    lcd_setMenuDataState(WHITE);
    const char* textPtr;
    switch (type_) {
        case EF_DELAY:
            textPtr = kNumberDataLibrary[effect_.delay.fWet].nameLongR;
            break;

        case EF_CHORUS:
            textPtr = kNumberDataLibrary[effect_.chorus.fWet].nameLongR;
            break;

        case EF_FLANGER:
            textPtr = kNumberDataLibrary[effect_.flanger.fWet].nameLongR;
            break;

        case EF_PHASER:
            textPtr = kNumberDataLibrary[effect_.phaser.fWet].nameLongR;
            break;

        case EF_COMPRESSOR:
            textPtr = kNumberDataLibrary[effect_.compressor.fWet].nameLongR;
            break;

        case EF_EXPANDER:
            textPtr = kNumberDataLibrary[effect_.expander.fWet].nameLongR;
            break;

        case EF_OVERDRIVE:
            textPtr = kNumberDataLibrary[effect_.overdrive.fWet].nameLongR;
            break;

        case EF_DISTORTION:
            textPtr = kNumberDataLibrary[effect_.distortion.fWet].nameLongR;
            break;

        case EF_BITCRUSHER:
            textPtr = kNumberDataLibrary[effect_.bitcrusher.fWet].nameLongR;
            break;
    }
    // 3. VALUE DRAW
    // Draw resolved parameter-F value.
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);
}

/* Lcd Reverb Menu Functions -------------------------------------------------*/

void Controller::lcd_drawReverbMenu() {
    /// @brief Draws Reverb menu layout and parameter values.

    // 1. MENU ICON DRAW
    // Draw reverb menu icon area.
    lcd_drawMenuIcon(REVERB_MENU);

    // 2. MENU NUMBER CLEAR
    // Clear menu-number area for this menu.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.clearRect(kMenuNumX, kMenuNumY, 7, 9);

    // 3. HEADER DRAW
    // Draw active and reverb parameter headers.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderSize, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderDecay, kMenuHeaderTextSize, kMenuHeaderX[2], kMenuHeaderY[2]);
    lcd.drawText(kHeaderDamping, kMenuHeaderTextSize, kMenuHeaderX[3], kMenuHeaderY[3]);
    lcd.drawText(kHeaderPredelay, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderSurround, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderDry, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderWet, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 4. SIGN DRAW
    // Draw unit/sign labels for reverb fields.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[2], kMenuSignBoxY[2]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[3], kMenuSignBoxY[3]);
    lcd.drawText(" MS", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
    lcd.drawText("  %", kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    // 5. DATA DRAW
    // Draw current reverb parameter values.
    lcd_drawReverb_ActiveData();
    lcd_drawReverb_SizeData();
    lcd_drawReverb_DecayData();
    lcd_drawReverb_DampingData();
    lcd_drawReverb_PreDelayData();
    lcd_drawReverb_SurroundData();
    lcd_drawReverb_DryData();
    lcd_drawReverb_WetData();
}

void Controller::lcd_drawReverb_ActiveData() {
    /// @brief Draws reverb active state value.

    // 1. STYLE SETUP
    // Configure data style for active field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state.
    const char* textPtr;
    (reverb.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawReverb_SizeData() {
    /// @brief Draws reverb size value.

    // 1. STYLE SETUP
    // Configure data style for size field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw size text.
    lcd.drawText(kNumberDataLibrary[reverb.size].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);
}

void Controller::lcd_drawReverb_DecayData() {
    /// @brief Draws reverb decay value.

    // 1. STYLE SETUP
    // Configure data style for decay field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw decay text.
    lcd.drawText(kNumberDataLibrary[reverb.decay].nameLongR, kMenuDataTextSize, kMenuDataX[2], kMenuDataY[2]);
}

void Controller::lcd_drawReverb_DampingData() {
    /// @brief Draws reverb damping value.

    // 1. STYLE SETUP
    // Configure data style for damping field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw damping text.
    lcd.drawText(kNumberDataLibrary[reverb.damping].nameLongR, kMenuDataTextSize, kMenuDataX[3], kMenuDataY[3]);
}

void Controller::lcd_drawReverb_PreDelayData() {
    /// @brief Draws reverb pre-delay value.

    // 1. STYLE SETUP
    // Configure data style for pre-delay field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw pre-delay text.
    lcd.drawText(kReverbPreDelayDataLibrary[reverb.preDelay].nameLongR, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawReverb_SurroundData() {
    /// @brief Draws reverb surround value.

    // 1. STYLE SETUP
    // Configure data style for surround field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw surround text.
    lcd.drawText(kReverbSurroundDataLibrary[reverb.surround].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawReverb_DryData() {
    /// @brief Draws reverb dry-mix value.

    // 1. STYLE SETUP
    // Configure data style for dry field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw dry-mix text.
    lcd.drawText(kNumberDataLibrary[reverb.dry].nameLongR, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawReverb_WetData() {
    /// @brief Draws reverb wet-mix value.

    // 1. STYLE SETUP
    // Configure data style for wet field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw wet-mix text.
    lcd.drawText(kNumberDataLibrary[reverb.wet].nameLongR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);
}

/* Lcd Lfo Menu Functions ----------------------------------------------------*/

void Controller::lcd_drawLfoMenu(uint8_t lfoNum_) {
    /// @brief Draws LFO menu layout, values, and waveform graph for selected LFO.

    // 1. MENU ICON DRAW
    // Draw LFO menu icon area.
    lcd_drawMenuIcon(LFO_MENU);

    // 2. LFO NUMBER DRAW
    // Draw selected LFO number marker.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + lfoNum_, 1, kMenuNumX, kMenuNumY);

    // 3. HEADER DRAW
    // Draw headers for LFO parameter columns.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);  // Active
    lcd.drawText(kHeaderType, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);    // Type
    lcd.drawText(kHeaderRate, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);    // Rate
    lcd.drawText(kHeaderPhase, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);   // Phase
    lcd.drawText(kHeaderInvert, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);  // Inversion
    lcd.drawText(kHeaderDepth, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);   // Depth

    // 4. SIGN DRAW
    // Draw unit/sign labels for parameter fields.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuDataX[0], kMenuSignBoxY[0]);  // Active
    lcd.drawText("   ", kMenuSignTextSize, kMenuDataX[1], kMenuSignBoxY[1]);  // Type
    lcd.drawText("BAR", kMenuSignTextSize, kMenuDataX[4], kMenuSignBoxY[4]);  // Rate
    lcd.drawText("DEG", kMenuSignTextSize, kMenuDataX[5], kMenuSignBoxY[5]);  // Phase
    lcd.drawText("ACT", kMenuSignTextSize, kMenuDataX[6], kMenuSignBoxY[6]);  // Inversion
    lcd.drawText("  %", kMenuSignTextSize, kMenuDataX[7], kMenuSignBoxY[7]);  // Depth

    // 5. DATA/GRAPH DRAW
    // Draw LFO parameter values and waveform graph.
    lcd_drawLfo_ActiveData(lfoNum_);  // Active
    lcd_drawLfo_TypeData(lfoNum_);    // Type
    lcd_drawLfo_RateData(lfoNum_);    // Rate
    lcd_drawLfo_PhaseData(lfoNum_);   // Phase
    lcd_drawLfo_InvertData(lfoNum_);  // Inversion
    lcd_drawLfo_DepthData(lfoNum_);   // Depth
    lcd_drawLfo_GraphData(lfoNum_);   // Waveform

    // 6. GRAPH BORDER DRAW
    // Draw side/baseline markers around graph area.
    lcd.setForeColor(WHITE);
    lcd.drawVLine(kGraphLfoX - 10, kGraphLfoY + 15, 30);
    lcd.drawVLine(kGraphLfoX + 180 + 10, kGraphLfoY + 15, 30);
    for (uint8_t i = 0; i < 20; i++) {
        lcd.drawPixel(kGraphLfoX - 70 + (i * 3), kGraphLfoY + 29);
        lcd.drawPixel(kGraphLfoX + 190 + (i * 3), kGraphLfoY + 29);
    }
}

void Controller::lcd_drawLfo_ActiveData(uint8_t lfoNum_) {
    /// @brief Draws active state value for selected LFO.

    // 1. STYLE SETUP
    // Configure data style for active field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state text.
    const char* textPtr;
    (lfoLibrary[lfoNum_].active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);
}

void Controller::lcd_drawLfo_TypeData(uint8_t lfoNum_) {
    /// @brief Draws waveform type value for selected LFO.

    // 1. STYLE SETUP
    // Configure data style for type field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw selected waveform type text.
    lcd.drawText(kLfoTypeDataLibrary[lfoLibrary[lfoNum_].type].nameLongR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);
}

void Controller::lcd_drawLfo_GraphData(uint8_t lfoNum_) {
    /// @brief Draw LFO waveform and display its period or inactive status
    Lfo& lfo = lfoLibrary[lfoNum_];

    // Select graph: 0 = inactive (flat), otherwise type+1
    uint8_t lfoGraph = (lfo.active) ? (lfo.type + 1) : 0;

    uint8_t phaseOffset = kLfoPhaseDataLibrary[lfo.phase].graphStart;
    uint16_t centerY = kGraphLfoY + 29;

    // 1. Draw bipolar graph
    lcd.setBackColor(BLACK);
    lcd.setForeColor(kAccentColor);
    for (uint8_t i = 0; i < 60; i++) {
        uint8_t dataIdx = (i + phaseOffset) % 60;
        int8_t rawValue = kLfoGraphDataLibrary[lfoGraph][dataIdx];

        // vertical flip implementation
        int16_t scaledValue = lfo.invert ? -rawValue : rawValue;

        // ensure visibility for zero-crossings
        if (scaledValue == 0) scaledValue = (lfo.invert) ? -1 : 1;

        uint16_t xPos = kGraphLfoX + 1 + (i * 3);
        lcd.clearVLine(xPos, kGraphLfoY, 61);
        lcd.drawVLine(xPos, centerY, scaledValue);
    }

    lcd.setFont(FONT_05x07);
    char timeBuf[8];
    const char* rateStr;

    // 2. Determine display data based on active state
    if (lfo.active) {
        // calculate period in seconds using current rhythm parameters
        float secondsPerMeasure = (60.0f * rhythm.measure) / rhythm.tempo;
        float lfoPeriod = secondsPerMeasure / kLfoRateDataLibrary[lfo.rate].rate;

        int32_t integral = static_cast<int32_t>(lfoPeriod);
        int32_t fractional = static_cast<int32_t>((lfoPeriod - integral) * 100.0f);
        if (fractional < 0) fractional = 0;

        snprintf(timeBuf, sizeof(timeBuf), "%ld.%02ld", integral, fractional);
        rateStr = kLfoRateDataLibrary[lfo.rate].nameShortR;
    } else {
        // inactive state placeholder
        strncpy(timeBuf, " ---", sizeof(timeBuf));
        rateStr = "--- ";
    }

    // 3. Render Rate metadata (left side)
    lcd.setForeColor(WHITE);
    lcd.setAlignment(LEFT);
    lcd.drawText(rateStr, 3, kMenuHeaderX[2] + 5, kMenuHeaderY[2] + 10);
    lcd.setForeColor(GRAY_75);
    lcd.drawText("BAR", 3, kMenuHeaderX[2] + 5, kMenuHeaderY[2] + 35);

    // 4. Render Duration metadata (right side)
    lcd.setForeColor(WHITE);
    lcd.setAlignment(RIGHT);
    lcd.drawText(timeBuf, strlen(timeBuf), kMenuDataX[3] - 5, kMenuHeaderY[3] + 10);
    lcd.setForeColor(GRAY_75);
    lcd.drawText("SEC", 3, kMenuDataX[3] - 5, kMenuHeaderY[3] + 35);
}

void Controller::lcd_drawLfo_RateData(uint8_t lfoNum_) {
    /// @brief Draws rate value for selected LFO.

    // 1. STYLE SETUP
    // Configure data style for rate field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw selected LFO rate text.
    lcd.drawText(kLfoRateDataLibrary[lfoLibrary[lfoNum_].rate].nameLongR, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawLfo_PhaseData(uint8_t lfoNum_) {
    /// @brief Draws phase value for selected LFO.

    // 1. STYLE SETUP
    // Configure data style for phase field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw selected LFO phase text.
    lcd.drawText(kLfoPhaseDataLibrary[lfoLibrary[lfoNum_].phase].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawLfo_InvertData(uint8_t lfoNum_) {
    /// @brief Draws invert state value for selected LFO.

    // 1. STYLE SETUP
    // Configure data style for invert field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw ON/OFF state text.
    const char* textPtr;
    (lfoLibrary[lfoNum_].invert) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawLfo_DepthData(uint8_t lfoNum_) {
    /// @brief Draws modulation depth value for selected LFO.

    // 1. STYLE SETUP
    // Configure data style for depth field.
    lcd_setMenuDataState(WHITE);

    // 2. VALUE DRAW
    // Draw selected LFO depth text.
    lcd.drawText(kNumberDataLibrary[lfoLibrary[lfoNum_].depth].nameLongR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);
}

/* Lcd Layer Inst Menu Functions ---------------------------------------------*/

void Controller::lcd_drawLayerInst0Menu(uint8_t layerNum_, bool graph_) {
    /// @brief Draws Layer Instrument page-0 layout and values.

    // SYNTH layer: the OSC page instead of the sample page.
    if (synthInstPageActive() && (layerNum_ == selectedLayerNum)) {
        lcd_drawSynthPage(layerNum_);
        return;
    }

    // 1. MENU ICON DRAW
    // Draw layer instrument menu icon area.
    lcd_drawMenuIcon(LAYER_INST_0_MENU);

    // 2. LAYER NUMBER DRAW
    // Draw selected layer number marker.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + layerNum_, 1, kMenuNumX, kMenuNumY);

    // 3. HEADER DRAW
    // Draw headers for instrument and playback fields.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderInst, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderSample, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderLevel, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderPitch, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderDirection, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderResolution, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 4. SIGN DRAW
    // Draw sign placeholders for first two fields.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);

    // 5. DATA DRAW
    // Draw page-0 instrument parameter values.
    lcd_drawLayerInst_InstData(layerNum_);
    lcd_drawLayerInst_SampleData(layerNum_);
    lcd_drawLayerInst_LevelData(layerNum_);
    lcd_drawLayerInst_PitchData(layerNum_);
    lcd_drawLayerInst_DirectionData(layerNum_);
    lcd_drawLayerInst_ResolutionData(layerNum_);
    if (graph_) lcd_drawLayerInst_GraphData(layerNum_);

    // 6. GRAPH BORDER DRAW
    // Draw side/baseline markers around graph area.
    lcd.setForeColor(WHITE);
    lcd.drawVLine(kGraphLfoX - 10, kGraphLfoY + 15, 30);
    lcd.drawVLine(kGraphLfoX + 180 + 10, kGraphLfoY + 15, 30);
    for (uint8_t i = 0; i < 20; i++) {
        lcd.drawPixel(kGraphLfoX - 70 + (i * 3), kGraphLfoY + 29);
        lcd.drawPixel(kGraphLfoX + 190 + (i * 3), kGraphLfoY + 29);
    }
}

void Controller::lcd_drawLayerInst1Menu(uint8_t layerNum_) {
    /// @brief Draws Layer Instrument page-1 timing/probability fields.

    // SYNTH layer: the FILTER page instead of start/end/timing/probability.
    if (synthInstPageActive() && (layerNum_ == selectedLayerNum)) {
        lcd_drawSynthPage(layerNum_);
        return;
    }

    // 1. HEADER DRAW
    // Draw headers for start/end/timing/probability fields.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderStart, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderEnd, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderTiming, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderProbability, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 2. DATA DRAW
    // Draw page-1 parameter values.
    lcd_drawLayerInst_StartData(layerNum_);
    lcd_drawLayerInst_EndData(layerNum_);
    lcd_drawLayerInst_TimingData(layerNum_);
    lcd_drawLayerInst_ProbabilityData(layerNum_);
}

void Controller::lcd_drawLayerInst2Menu(uint8_t layerNum_) {
    /// @brief Draws Layer Instrument page-2 routing fields.

    // SYNTH layer: the LFO page comes first (same menu); the routing page has no sample on knobs 1-2.
    if (synthLfoPageActive() && (layerNum_ == selectedLayerNum)) {
        lcd_drawSynthPage(layerNum_);
        return;
    }
    if (synthInstPageActive() && (layerNum_ == selectedLayerNum)) {
        lcd.setBackColor(BLACK);
        lcd_clearMenuTab(0);
        lcd_clearMenuTab(1);
        lcd.clearRect(kMenuSignBoxX[1] - 70, kMenuSignBoxY[1], 70, kMenuSignBoxHeight);
        lcd.setForeColor(GRAY_50);
        lcd.clearRect(493, 30, 325, 60);
        lcd.drawVLine(kMenuLineX[3], kMenuLineY[3], kMenuLineHeight);
        lcd.setFont(FONT_05x07);
        lcd.setAlignment(LEFT);
        lcd.setForeColor(GRAY_75);
        lcd.drawText("SYNTH  ROUTING", 14, kBeatGraphStartX, kBeatGraphStartY + 2);
        if (menuTab < 2) {  // the focused tab's marker was in a cleared slot
            lcd.drawRGB16Image((const RGB16Color*)(RAM_ICON_SELECT_PALETTE_ADDRESS), (const uint8_t*)(RAM_ICON_SELECT_ON_DATA_ADDRESS), 64,
                               kMenuSelectX[menuTab], kMenuSelectY[menuTab], kIconSelectWidth, kIconSelectHeight);
        }
    }

    // 1. HEADER DRAW
    // Draw headers for EQ/Filter/Effect/Reverb routing.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderEq, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderFilter, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderEffect, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderReverb, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 2. SIGN DRAW
    // Draw routing order labels.
    lcd_setMenuSignState(GRAY_75);
    lcd.drawText("  A", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText("  B", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
    lcd.drawText("  C", kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
    lcd.drawText("  D", kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    // 3. DATA DRAW
    // Draw routing selection values.
    lcd_drawLayerInst_FilterData(layerNum_);
    lcd_drawLayerInst_EqData(layerNum_);
    lcd_drawLayerInst_EffectData(layerNum_);
    lcd_drawLayerInst_ReverbData(layerNum_);
}

void Controller::lcd_drawLayerInst_InstData(uint8_t layerNum_) {
    /// @brief Draws selected instrument name with load-state coloring.
    if (synthInstPageActive()) return;  // SYNTH pages use this slot

    // 1. CONTEXT LOAD
    // Resolve layer reference for selected and loaded instrument data.
    Layer& layer = layerLibrary[layerNum_];

    // 2. NAME DRAW
    // Draw instrument name using loaded/selected match color.
    lcd_setMenuDataState(WHITE);
    (strcmp(layer.instSelectedData.fileName, layer.instLoadedData.fileName) == 0) ? lcd.setForeColor(WHITE) : lcd.setForeColor(GRAY_60);
    lcd.drawText(layer.instSelectedData.nameShortR, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);

    // 3. OPTIONAL NUMBER STYLE
    // Keep number style setup for optional instrument number rendering.
    lcd_setMenuTextState(GRAY_75);

    // lcd.drawText(layer.instSelectedData.num, 4, kMenuTextBoxX[0], kMenuTextBoxY[0]);
}

void Controller::lcd_drawLayerInst_SampleData(uint8_t layerNum_) {
    /// @brief Draws selected sample name and related status details.
    if (synthInstPageActive()) return;  // SYNTH pages use this slot

    // 1. CONTEXT LOAD
    // Resolve layer reference for selected and loaded sample data.
    Layer& layer = layerLibrary[layerNum_];

    // 2. SAMPLE NAME DRAW
    // Draw sample name using loaded/selected match color.
    lcd_setMenuDataState(WHITE);
    ((strcmp(layer.instSelectedData.fileName, layer.instLoadedData.fileName) == 0) && (strcmp(layer.sampleSelectedData.fileName, layer.sampleLoadedData.fileName) == 0)) ? lcd.setForeColor(WHITE) : lcd.setForeColor(GRAY_60);
    lcd.drawText(layer.sampleSelectedData.nameShortR, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);

    // 3. DETAILS STYLE SETUP
    // Configure style for auxiliary sample detail area.
    lcd_setMenuSignState(GRAY_75);

    // 4. DETAILS/ERROR DRAW
    // Draw read/type errors or sample metadata based on selection state.
    uint16_t xPos = kMenuSignBoxX[1];
    uint16_t yPos = kMenuSignBoxY[1];
    if (layer.sampleSelected != -1) {
        // Show error if sample read failed
        if (layer.sampleSelectedReadError) {
            lcd.setForeColor(kAccent2Color);
            lcd.drawText(" READ ERROR", 11, xPos, yPos);

            // Show error if sample type is wrong
        } else if (layer.sampleSelectedTypeError) {
            lcd.setForeColor(kAccent2Color);
            lcd.drawText(" TYPE ERROR", 11, xPos, yPos);

            // Show sample info
        } else {
            lcd.setSpacing(2);
            lcd.setForeColor(GRAY_60);
            lcd.drawText(kFrequencyDataLibrary[layer.sampleSelectedData.frequency].name, 6, xPos - 35, yPos);
            lcd.drawText(kBitdepthDataLibrary[layer.sampleSelectedData.bitdepth].name, 2, xPos - 14, yPos);
            lcd.drawText(kChannelDataLibrary[layer.sampleSelectedData.channel].name, 1, xPos, yPos);
            lcd.drawText("-", 1, xPos - 28, yPos);
            lcd.drawText("-", 1, xPos - 7, yPos);
        }
    } else {
        // Clear details if no sample selected
        lcd.clearRect(xPos - 80, yPos, 80, 8);
    }
}

void Controller::lcd_drawLayerInst_GraphData(uint8_t layerNum_) {
    /// @brief Draws sample waveform graph plus bit-depth and duration metadata.
    if (synthInstPageActive()) return;  // SYNTH pages draw their own graph

    // 1. CONTEXT LOAD
    // Resolve layer and loaded sample data references.
    if (layerNum_ >= kLayerLibrarySize) return;

    Layer& layer = layerLibrary[layerNum_];
    SampleData& sData = layer.sampleLoadedData;
    uint16_t centerY = kGraphLfoY + 29;

    // 2. GRAPH RANGE SETUP
    // Convert start/end selection values to graph index space.
    uint8_t startIdx = (static_cast<uint16_t>(layer.start) * kLayerInstGraphPointCount) / 100;
    uint8_t endIdx = (static_cast<uint16_t>(layer.end) * kLayerInstGraphPointCount) / 100;
    if (startIdx > kLayerInstGraphPointCount) startIdx = kLayerInstGraphPointCount;
    if (endIdx > kLayerInstGraphPointCount) endIdx = kLayerInstGraphPointCount;

    // 3. DIRECTION MODE
    // Resolve whether waveform should be horizontally reversed.
    bool isReverse = (layer.direction == 1 || layer.direction == 3);

    bool firstDraw = !gLayerInstGraphCacheInit[layerNum_];
    bool directionChanged = (!firstDraw) && (gLayerInstGraphPrevDirection[layerNum_] != layer.direction);
    bool selectionChanged = firstDraw || directionChanged || (gLayerInstGraphPrevStartIdx[layerNum_] != startIdx) || (gLayerInstGraphPrevEndIdx[layerNum_] != endIdx);

    // 4. WAVEFORM DRAW
    // Redraw geometry only when required; on start/end edits, recolor only changed columns.
    lcd.setBackColor(BLACK);
    for (uint8_t i = 0; i < kLayerInstGraphPointCount; i++) {
        // Scale x-axis (spaced by 3 pixels per point)
        uint16_t xPos = kGraphLfoX + 1 + (i * 3);

        // Calculate data index based on direction
        uint8_t dataIdx = isReverse ? ((kLayerInstGraphPointCount - 1) - i) : i;
        uint8_t val = sData.graphData[dataIdx];
        bool activeZone = (dataIdx >= startIdx) && (dataIdx < endIdx);

        bool amplitudeChanged = (gLayerInstGraphPrevAmplitude[layerNum_][i] != val);
        bool zoneChanged = (gLayerInstGraphPrevActiveZone[layerNum_][i] != activeZone);

        bool requiresGeometryRedraw = firstDraw || directionChanged || amplitudeChanged;
        bool requiresColorOnlyRedraw = (!requiresGeometryRedraw) && zoneChanged;
        if ((!requiresGeometryRedraw) && (!requiresColorOnlyRedraw)) continue;

        // Color based on audio data index range
        lcd.setForeColor(activeZone ? kAccentColor : GRAY_50);

        if (requiresGeometryRedraw) {
            // Shape changed: clear old column geometry first.
            lcd.clearVLine(xPos, kGraphLfoY, kLayerInstGraphColumnHeight);
            if (val > 0) {
                // Draw bipolar: centerY - val to centerY + val
                lcd.drawVLine(xPos, centerY - val, val * 2);
            } else {
                // Center dot for zero amplitude
                lcd.drawPixel(xPos, centerY);
            }
        } else {
            // Color-only update (typically start/end edits): repaint without clear.
            if (val > 0) {
                lcd.drawVLine(xPos, centerY - val, val * 2);
            } else {
                lcd.drawPixel(xPos, centerY);
            }
        }

        gLayerInstGraphPrevAmplitude[layerNum_][i] = val;
        gLayerInstGraphPrevActiveZone[layerNum_][i] = activeZone;
    }

    // 5. EFFECTIVE BIT DEPTH CALC
    // Derive effective bit depth from sample and layer resolution settings.
    uint8_t fileBits = 0;
    if (sData.bitdepth == BD_08)
        fileBits = 8;
    else if (sData.bitdepth == BD_16)
        fileBits = 16;
    else if (sData.bitdepth == BD_24)
        fileBits = 24;
    else if (sData.bitdepth == BD_32)
        fileBits = 32;

    // layer.resolution index 0 corresponds to 4-bit
    uint8_t layerBits = layer.resolution + 4;

    // Use layer resolution if it is lower than the sample bit depth
    uint8_t effectiveBits = (fileBits > 0 && layerBits < fileBits) ? layerBits : fileBits;

    char bitBuf[4];
    if (effectiveBits > 0) {
        snprintf(bitBuf, sizeof(bitBuf), "%02d", effectiveBits);
    } else {
        snprintf(bitBuf, sizeof(bitBuf), "00");
    }

    bool bitMetaChanged = firstDraw || (gLayerInstGraphPrevBitDepth[layerNum_] != effectiveBits) || (gLayerInstGraphPrevResolution[layerNum_] != layer.resolution);
    if (bitMetaChanged) {
        // 6. BIT DEPTH META DRAW
        // Draw effective bit-depth metadata labels only when changed.
        lcd.setFont(FONT_05x07);
        lcd.setAlignment(LEFT);
        lcd.clearRect(kMenuHeaderX[2] + 5, kMenuHeaderY[2] + 10, 16, kLayerInstGraphMetaTextHeight);
        lcd.clearRect(kMenuHeaderX[2] + 5, kMenuHeaderY[2] + 35, 16, kLayerInstGraphMetaTextHeight);
        lcd.setForeColor(WHITE);
        lcd.drawText(bitBuf, strlen(bitBuf), kMenuHeaderX[2] + 5, kMenuHeaderY[2] + 10);
        lcd.setForeColor(GRAY_75);
        lcd.drawText("BIT", 3, kMenuHeaderX[2] + 5, kMenuHeaderY[2] + 35);
    }

    // 7. DURATION CALCULATION
    // Compute playback duration from selection window and pitch factor.
    float pitchFactor = kLayerPitchDataLibrary[layer.pitch].data;
    float selectionRatio = static_cast<float>(layer.end - layer.start) / 100.0f;
    if (selectionRatio < 0.0f) selectionRatio = 0.0f;

    // Total duration adjusted for selection window and playback speed
    float effectiveTime = (sData.timeData * selectionRatio) / pitchFactor;

    char timeBuf[8];
    int32_t integral = static_cast<int32_t>(effectiveTime);
    int32_t fractional = static_cast<int32_t>((effectiveTime - integral) * 100.0f);
    if (fractional < 0) fractional = 0;
    snprintf(timeBuf, sizeof(timeBuf), "%ld.%02ld", integral, fractional);

    bool durationChanged = firstDraw || (strcmp(gLayerInstGraphPrevTimeText[layerNum_], timeBuf) != 0) || (gLayerInstGraphPrevPitch[layerNum_] != layer.pitch) || selectionChanged;
    if (durationChanged) {
        // 8. DURATION META DRAW
        // Draw calculated duration metadata labels only when changed.
        lcd.setFont(FONT_05x07);
        lcd.setAlignment(RIGHT);
        lcd.clearRect(kMenuDataX[3] - 40, kMenuHeaderY[3] + 10, 40, kLayerInstGraphMetaTextHeight);
        lcd.clearRect(kMenuDataX[3] - 22, kMenuHeaderY[3] + 35, 22, kLayerInstGraphMetaTextHeight);
        lcd.setForeColor(WHITE);
        lcd.drawText(timeBuf, strlen(timeBuf), kMenuDataX[3] - 5, kMenuHeaderY[3] + 10);
        lcd.setForeColor(GRAY_75);
        lcd.drawText("SEC", 3, kMenuDataX[3] - 5, kMenuHeaderY[3] + 35);
    }

    gLayerInstGraphPrevPitch[layerNum_] = layer.pitch;
    gLayerInstGraphPrevResolution[layerNum_] = layer.resolution;
    gLayerInstGraphPrevDirection[layerNum_] = layer.direction;
    gLayerInstGraphPrevStartIdx[layerNum_] = startIdx;
    gLayerInstGraphPrevEndIdx[layerNum_] = endIdx;
    gLayerInstGraphPrevBitDepth[layerNum_] = effectiveBits;
    snprintf(gLayerInstGraphPrevTimeText[layerNum_], sizeof(gLayerInstGraphPrevTimeText[layerNum_]), "%s", timeBuf);
    gLayerInstGraphCacheInit[layerNum_] = true;
}

void Controller::lcd_drawLayerInst_LevelData(uint8_t layerNum_) {
    /// @brief Draws layer level value and assigned LFO label.
    if (synthInstPageActive() && !synthPageIs(SYNTH_PAGE_OSC)) return;  // only on the SYNTH OSC page

    // 1. CONTEXT LOAD
    // Resolve layer reference for level and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. LEVEL VALUE DRAW
    // Draw level value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[layer.level].nameLongR, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoLevel].nameShortR, kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
}

void Controller::lcd_drawLayerInst_PitchData(uint8_t layerNum_) {
    /// @brief Draws layer pitch value and assigned LFO label.
    if (synthInstPageActive() && !synthPageIs(SYNTH_PAGE_OSC)) return;  // only on the SYNTH OSC page

    // 1. CONTEXT LOAD
    // Resolve layer reference for pitch and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. PITCH VALUE DRAW
    // Draw pitch value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLayerPitchDataLibrary[layerLibrary[layerNum_].pitch].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoPitch].nameShortR, kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
}

void Controller::lcd_drawLayerInst_DirectionData(uint8_t layerNum_) {
    /// @brief Draws layer direction value and assigned LFO label.
    if (synthInstPageActive()) return;  // not a SYNTH setting

    // 1. CONTEXT LOAD
    // Resolve layer reference for direction and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. DIRECTION VALUE DRAW
    // Draw direction value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLayerDirectionDataLibrary[layerLibrary[layerNum_].direction].nameLongR, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoDirection].nameShortR, kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
}

void Controller::lcd_drawLayerInst_ResolutionData(uint8_t layerNum_) {
    /// @brief Draws layer resolution value and assigned LFO label.
    if (synthInstPageActive()) return;  // not a SYNTH setting

    // 1. CONTEXT LOAD
    // Resolve layer reference for resolution and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. RESOLUTION VALUE DRAW
    // Draw resolution value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLayerResolutionDataLibrary[layerLibrary[layerNum_].resolution].nameLongR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoResolution].nameShortR, kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);
}

void Controller::lcd_drawLayerInst_StartData(uint8_t layerNum_) {
    /// @brief Draws start-position value and assigned LFO label.
    if (synthInstPageActive()) return;  // not a SYNTH setting

    // 1. CONTEXT LOAD
    // Resolve layer reference for start position and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. START VALUE DRAW
    // Draw start-position value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[layer.start].nameLongR, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoStart].nameShortR, kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
}

void Controller::lcd_drawLayerInst_EndData(uint8_t layerNum_) {
    /// @brief Draws end-position value and assigned LFO label.
    if (synthInstPageActive()) return;  // not a SYNTH setting

    // 1. CONTEXT LOAD
    // Resolve layer reference for end position and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. END VALUE DRAW
    // Draw end-position value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[layer.end].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoEnd].nameShortR, kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
}

void Controller::lcd_drawLayerInst_TimingData(uint8_t layerNum_) {
    /// @brief Draws timing-offset value and assigned LFO label.
    if (synthInstPageActive()) return;  // not a SYNTH setting

    // 1. CONTEXT LOAD
    // Resolve layer reference for timing and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. TIMING VALUE DRAW
    // Draw timing-offset value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[layer.timing].nameLongR, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoTiming].nameShortR, kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
}

void Controller::lcd_drawLayerInst_ProbabilityData(uint8_t layerNum_) {
    /// @brief Draws probability value and assigned LFO label.
    if (synthInstPageActive()) return;  // not a SYNTH setting

    // 1. CONTEXT LOAD
    // Resolve layer reference for probability and LFO assignment.
    Layer& layer = layerLibrary[layerNum_];

    // 2. PROBABILITY VALUE DRAW
    // Draw probability value text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[layer.probability].nameLongR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);

    // 3. LFO LABEL DRAW
    // Draw assigned LFO short label.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText(kLfoNameDataLibrary[layer.lfoProbability].nameShortR, kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);
}

void Controller::lcd_drawLayerInst_EqData(uint8_t layerNum_) {
    if (synthLfoPageActive()) return;  // the SYNTH LFO page uses these slots
    /// @brief Draws EQ routing assignment value.

    // 1. VALUE DRAW
    // Draw EQ routing status text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLayerEqDataLibrary[layerLibrary[layerNum_].eq].nameLongR, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawLayerInst_FilterData(uint8_t layerNum_) {
    if (synthLfoPageActive()) return;  // the SYNTH LFO page uses these slots
    /// @brief Draws Filter routing assignment value.

    // 1. VALUE DRAW
    // Draw Filter routing status text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLayerFilterDataLibrary[layerLibrary[layerNum_].filter].nameLongR, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawLayerInst_EffectData(uint8_t layerNum_) {
    if (synthLfoPageActive()) return;  // the SYNTH LFO page uses these slots
    /// @brief Draws Effect routing assignment value.

    // 1. VALUE DRAW
    // Draw Effect routing status text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLayerEffectDataLibrary[layerLibrary[layerNum_].effect].nameLongR, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawLayerInst_ReverbData(uint8_t layerNum_) {
    if (synthLfoPageActive()) return;  // the SYNTH LFO page uses these slots
    /// @brief Draws Reverb routing assignment value.

    // 1. VALUE DRAW
    // Draw Reverb routing status text.
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLayerReverbDataLibrary[layerLibrary[layerNum_].reverb].nameLongR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);
}

/* Lcd Layer Song Menu Functions ---------------------------------------------*/

void Controller::lcd_drawLayerSongMenu(uint8_t layerNum_) {
    /// @brief Draws Layer Song menu layout and beat-fill controls.

    // 1. MENU ICON DRAW
    // Draw layer-song menu icon area.
    lcd_drawMenuIcon(LAYER_SONG_MENU);

    // 2. LAYER NUMBER DRAW
    // Draw selected layer number marker.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + layerNum_, 1, kMenuNumX, kMenuNumY);

    // 3. HEADER DRAW
    // Draw headers for type/pattern/timing/velocity fields.
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderType, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderPattern, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderTiming, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderVelocity, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderMove, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderShiftAll, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 4. DATA DRAW
    // Draw beat-fill values and graph.
    lcd_drawLayerSong_BeatFillTypeData(layerNum_);
    lcd_drawLayerSong_BeatFillPatternData(layerNum_);
    lcd_drawLayerSong_BeatFillTimeData(layerNum_);
    lcd_drawLayerSong_BeatFillLevelData(layerNum_);
    lcd_drawLayerSong_BeatFillGraphData(layerNum_);
    lcd_drawLayerSong_MoveData(layerNum_);
    lcd_drawLayerSong_ShiftData(layerNum_);

    // 5. SIGN DRAW
    // Draw sign placeholders for visible fields.
    lcd_setMenuSignState(kAccentColor);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[0], kMenuSignBoxY[0]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[1], kMenuSignBoxY[1]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[4], kMenuSignBoxY[4]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[5], kMenuSignBoxY[5]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[6], kMenuSignBoxY[6]);
    lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[7], kMenuSignBoxY[7]);

    // 6. GRAPH BACKGROUND DRAW
    // Draw graph axes and boundary markers.
    lcd_setMenuHeaderState(WHITE);
    lcd.setFont(FONT_05x07);
    lcd.drawText("S", 1, kBeatGraphStartX - 15, kBeatGraphStartY);
    lcd.drawText("E", 1, kBeatGraphEndX + 11, kBeatGraphStartY);
    lcd.drawVLine(kBeatGraphEndX, kBeatGraphStartY, kBeatGraphHeight);
    lcd.drawHLine(kBeatGraphStartX, kBeatGraphEndY, kBeatGraphWidth);

    // 7. ACTIVE BEAT INDICATOR
    // Draw active beat cursor if a beat exists.
    if (layerLibrary[layerNum_].bankLibrary[activeSongBank].lastActiveBeatNum != -1)
        lcd_drawBeat(layerNum_, activeSongBank, 0, true);
}

void Controller::lcd_drawLayerSong_BeatFillTypeData(uint8_t layerNum_) {
    if (layerSongAltPageActive()) return;  // the EUCLID and NOTE pages use these slots

    /// @brief Draws beat-fill type value for selected beat.

    // 1. CONTEXT LOAD
    // Resolve layer and active bank references.
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[activeSongBank];

    // 2. VALUE DRAW
    // Draw fill-type text or dash when no beat is selected.
    lcd_setMenuDataState(WHITE);
    const char* text;
    (selectedBeatNum != -1) ? text = kFillTypeDataLibrary[bank.beatLibrary[selectedBeatNum].getFillType()].nameLong : text = kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[0], kMenuDataY[0]);

    // This field is redrawn on every selection change (beat left/right, insert, delete,
    // move, shift, fill edit), so the MOVE position readout is refreshed from here too.
    if ((menu == LAYER_SONG_MENU) && (layerNum_ == selectedLayerNum)) lcd_drawLayerSong_MoveData(layerNum_);

    // 3. OPTIONAL INDEX OVERLAY
    // Optional beat index overlay block kept disabled.
    /*
    lcd_setMenuNumState(GRAY_80);
    char numText[6];
    if (bank.lastActiveBeatNum != -1) {

        // Show selected and last active beat numbers
        char num0[3];
        char num1[3];
        sprintf(num0, "%02d", selectedBeatNum + 1);
        sprintf(num1, "%02d", bank.lastActiveBeatNum + 1);
        strcpy(numText, num0);
        strcat(numText, "|");
        strcat(numText, num1);
    } else {

        // Show dashes if not available
        strcpy(numText, "--|--");
    }
    lcd.drawText(numText, 5, kMenuDataX[0] - 16, kMenuHeaderY[0]);
    */
}

void Controller::lcd_drawLayerSong_BeatFillPatternData(uint8_t layerNum_) {
    if (layerSongAltPageActive()) return;  // the EUCLID and NOTE pages use these slots

    /// @brief Draws beat-fill pattern value for selected beat.

    // 1. CONTEXT LOAD
    // Resolve layer and active bank references.
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[activeSongBank];

    // 2. VALUE DRAW
    // Draw fill-pattern text or dash when no beat is selected.
    lcd_setMenuDataState(WHITE);
    const char* text;
    (selectedBeatNum != -1) ? text = kFillPatternDataLibrary[bank.beatLibrary[selectedBeatNum].getFillType()][bank.beatLibrary[selectedBeatNum].getFillPattern()].nameLong : text = kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[1], kMenuDataY[1]);
}

void Controller::lcd_drawLayerSong_BeatFillTimeData(uint8_t layerNum_) {
    if (layerSongAltPageActive()) return;  // the EUCLID and NOTE pages use these slots

    /// @brief Draws beat-fill time-mode value for selected beat.

    // 1. CONTEXT LOAD
    // Resolve layer and active bank references.
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[activeSongBank];

    // 2. STYLE SETUP
    // Configure data style for time field.
    lcd_setMenuDataState(WHITE);

    // 3. VALUE RESOLUTION/DRAW
    // Resolve time-mode text and draw it for selected beat state.
    const char* text;
    if (selectedBeatNum != -1) {
        // Show time mode for complex fills
        ((bank.beatLibrary[selectedBeatNum].getFillType() == 0) && (bank.beatLibrary[selectedBeatNum].getFillPattern() <= 2)) ? text = kDataDashR : text = kFillTimeDataLibrary[bank.beatLibrary[selectedBeatNum].getFillTime()].nameLongR;
    } else {
        // Show dash if not available
        text = kDataDashR;
    }
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[4], kMenuDataY[4]);
}

void Controller::lcd_drawLayerSong_BeatFillLevelData(uint8_t layerNum_) {
    if (layerSongAltPageActive()) return;  // the EUCLID and NOTE pages use these slots

    /// @brief Draws beat-fill level mode for selected beat.

    // 1. CONTEXT LOAD
    // Resolve layer and active bank references.
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[activeSongBank];
    lcd_setMenuDataState(WHITE);

    // 2. VALUE RESOLUTION/DRAW
    // Resolve level text and draw it for selected beat state.
    const char* text;
    if (selectedBeatNum != -1) {
        // Show level mode for complex fills
        ((bank.beatLibrary[selectedBeatNum].getFillType() == 0) && (bank.beatLibrary[selectedBeatNum].getFillPattern() <= 2)) ? text = kDataDashR : text = kFillLevelDataLibrary[bank.beatLibrary[selectedBeatNum].getFillLevel()].nameLongR;
    } else {
        // Show dash if not available
        text = kDataDashR;
    }
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[5], kMenuDataY[5]);
}

void Controller::lcd_drawLayerSong_MoveData(uint8_t layerNum_) {
    if (layerSongAltPageActive()) return;  // the EUCLID and NOTE pages use these slots

    /// @brief Draws the selected beat's position under MOVE as bar.beat.sixteenth.
    /// A trailing "+" marks a beat that sits between sixteenths (e.g. recorded live or
    /// placed with a 1:32 / 1:64 grid).

    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[activeSongBank];
    lcd_setMenuDataState(WHITE);

    char text[kMenuDataTextSize + 1];
    const uint16_t sixteenth = measureInterval / 4;
    if ((selectedBeatNum >= 0) && (selectedBeatNum <= bank.lastActiveBeatNum) && (barInterval > 0) && (sixteenth > 0)) {
        const uint16_t start = bank.beatLibrary[selectedBeatNum].getStartInterval();
        const unsigned bar = (start / barInterval) + 1;
        const unsigned inBar = start % barInterval;
        const unsigned beat = (inBar / measureInterval) + 1;
        const unsigned inBeat = inBar % measureInterval;
        const unsigned step = (inBeat / sixteenth) + 1;
        const bool offGrid = (inBeat % sixteenth) != 0;

        char position[40];  // sized for the compiler's worst case; real text is e.g. "12.4.3+"
        snprintf(position, sizeof(position), "%u.%u.%u%s", bar, beat, step, offGrid ? "+" : "");
        snprintf(text, sizeof(text), "%10.10s", position);
    } else {
        snprintf(text, sizeof(text), "%s", kDataDashR);
    }
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[6], kMenuDataY[6]);
}

void Controller::lcd_drawLayerSong_ShiftData(uint8_t layerNum_) {
    if (layerSongAltPageActive()) return;  // the EUCLID and NOTE pages use these slots

    /// @brief Draws the step size used by MOVE and SHIFT ALL (the Rhythm menu quantize).

    (void)layerNum_;
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kQuantizeDataLibrary[rhythm.quantize].nameLongR, kMenuDataTextSize, kMenuDataX[7], kMenuDataY[7]);
}

/* Lcd Layer Song EUCLID Page Functions --------------------------------------*/

void Controller::lcd_drawLayerSongEuclidMenu(uint8_t layerNum_) {
    /// @brief Draws the EUCLID page of the Layer Song menu: headers, values and step strip.

    // 1. ICON + LAYER NUMBER (same as the edit page)
    lcd_drawMenuIcon(LAYER_SONG_MENU);
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + layerNum_, 1, kMenuNumX, kMenuNumY);

    // 2. HEADERS
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderEuclidSteps, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderEuclidHits, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderEuclidRotate, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderEuclidGrid, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderEuclidLength, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderEuclidMode, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 3. SIGNS
    lcd_setMenuSignState(kAccentColor);
    const uint8_t slots[6] = {0, 1, 4, 5, 6, 7};
    for (uint8_t i = 0; i < 6; i++) lcd.drawText("   ", kMenuSignTextSize, kMenuSignBoxX[slots[i]], kMenuSignBoxY[slots[i]]);

    // 4. VALUES + STRIP
    for (uint8_t i = 0; i < 6; i++) lcd_drawEuclidData(slots[i]);
    lcd_drawEuclidStrip();
}

void Controller::lcd_drawEuclidData(uint8_t slot_) {
    /// @brief Draws one EUCLID page value (slot = knob position 0..7).

    if (!euclidPageActive()) return;
    const EuclidSettings& s = euclidSettings;
    const char* text;
    switch (slot_) {
        case 0:
            text = kNumberDataLibrary[s.steps].nameLongR;
            break;
        case 1:
            text = kNumberDataLibrary[s.hits].nameLongR;
            break;
        case 4:
            text = kNumberDataLibrary[s.rotate].nameLongR;
            break;
        case 5:
            text = kQuantizeDataLibrary[s.grid].nameLongR;
            break;
        case 6:
            text = kEuclidLengthText[s.repeat ? 1 : 0];
            break;
        case 7:
            text = kEuclidModeText[s.merge ? 1 : 0];
            break;
        default:
            return;
    }
    lcd_setMenuDataState(WHITE);
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[slot_], kMenuDataY[slot_]);
}

namespace {

// Step strip geometry inside the fill-graph area (x 516..796, y 32..87).
constexpr uint16_t kEuclidCellY = kBeatGraphStartY + 18;
constexpr uint16_t kEuclidCellHeight = 24;
constexpr uint16_t kEuclidMaxPitch = 35;

uint16_t euclidPitch(uint8_t steps) {
    const uint16_t pitch = (steps > 0) ? (kBeatGraphWidth / steps) : kEuclidMaxPitch;
    return (pitch > kEuclidMaxPitch) ? kEuclidMaxPitch : pitch;
}

uint16_t euclidStripX(uint8_t steps) {
    return kBeatGraphStartX + ((kBeatGraphWidth - (euclidPitch(steps) * steps)) / 2);
}

}  // namespace

void Controller::lcd_drawEuclidCell(uint8_t step_, bool playing_) {
    /// @brief Draws one step of the strip: filled = hit, outline = rest, cyan = now playing.

    if (!euclidPageActive() || (step_ >= euclidSettings.steps)) return;
    const uint16_t pitch = euclidPitch(euclidSettings.steps);
    const uint16_t width = (pitch > 3) ? (pitch - 2) : 1;
    const uint16_t x = euclidStripX(euclidSettings.steps) + (step_ * pitch);

    lcd.setBackColor(BLACK);
    lcd.clearRect(x, kEuclidCellY, width, kEuclidCellHeight);
    lcd.setForeColor(playing_ ? kAccentColor : (euclidPattern[step_] ? WHITE : GRAY_50));
    if (euclidPattern[step_]) {
        lcd.fillRect(x, kEuclidCellY, width, kEuclidCellHeight);
    } else {
        lcd.drawRect(x, kEuclidCellY, width, kEuclidCellHeight);
    }
}

void Controller::lcd_drawEuclidStrip() {
    /// @brief Draws the whole step strip with its "E(hits,steps)" title and beat ticks.

    if (!euclidPageActive()) return;
    const EuclidSettings& s = euclidSettings;
    euclidPlayStep = -1;  // cells are redrawn without the playhead; the next update re-marks it

    lcd.setBackColor(BLACK);
    lcd.clearRect(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphWidth + 1, kBeatGraphHeight + 1);

    // Title, and a warning when the bank's 64-beat limit cut the pattern.
    char title[24];
    snprintf(title, sizeof(title), "EUCLID E(%u,%u)", s.hits, s.steps);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.setForeColor(GRAY_75);
    lcd.drawText(title, strlen(title), kBeatGraphStartX, kBeatGraphStartY + 2);
    if (euclidTruncated) {
        lcd.setAlignment(RIGHT);
        lcd.setForeColor(RED);
        lcd.drawText("MAX 64 BEATS", 12, kBeatGraphEndX, kBeatGraphStartY + 2);
    }

    // Cells, plus a tick under every 4th step for orientation.
    const uint16_t pitch = euclidPitch(s.steps);
    const uint16_t x0 = euclidStripX(s.steps);
    for (uint8_t i = 0; i < s.steps; i++) {
        lcd_drawEuclidCell(i, false);
        if ((i % 4) == 0) {
            lcd.setForeColor(GRAY_50);
            lcd.fillRect(x0 + (i * pitch), kEuclidCellY + kEuclidCellHeight + 3, 2, 3);
        }
    }

    // Hint: a CLEAR tap restores the beats from before the page opened, holding it erases the
    // layer (the keys are not shown anywhere else on this page). Bottom right; no '=' in the font.
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(RIGHT);
    lcd.setForeColor(GRAY_50);
    lcd.drawText("CLEAR: UNDO  HOLD: ERASE", 24, kBeatGraphEndX, kBeatGraphEndY - 6);
}

void Controller::lcd_updateEuclidPlayhead() {
    /// @brief Main loop: marks the step that is playing now (cyan) while the EUCLID page is
    /// open and the transport runs. Redraws only the two cells that change.

    if (!euclidPageActive() || !playActive) {
        if (euclidPlayStep >= 0) {
            const int16_t previous = euclidPlayStep;
            euclidPlayStep = -1;
            lcd_drawEuclidCell(static_cast<uint8_t>(previous), false);
        }
        return;
    }

    const uint32_t grid = static_cast<uint32_t>(kQuantizeInterval[euclidSettings.grid]);
    const uint32_t cycle = grid * euclidSettings.steps;
    if (cycle == 0) return;
    const uint32_t position = playInterval;
    const int16_t step = ((!euclidSettings.repeat) && (position >= cycle)) ? -1 : static_cast<int16_t>((position % cycle) / grid);

    if (step != euclidPlayStep) {
        if (euclidPlayStep >= 0) lcd_drawEuclidCell(static_cast<uint8_t>(euclidPlayStep), false);
        if (step >= 0) lcd_drawEuclidCell(static_cast<uint8_t>(step), true);
        euclidPlayStep = step;
    }
}

/* Lcd Layer Song NOTE Page Functions ----------------------------------------*/

void Controller::lcd_drawLayerSongNoteMenu(uint8_t layerNum_) {
    /// @brief Draws the NOTE page of the Layer Song menu: headers, values and note strip.

    // 1. ICON + LAYER NUMBER (same as the edit page)
    lcd_drawMenuIcon(LAYER_SONG_MENU);
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + layerNum_, 1, kMenuNumX, kMenuNumY);

    // 2. HEADERS
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kHeaderNoteMode, kMenuHeaderTextSize, kMenuHeaderX[0], kMenuHeaderY[0]);
    lcd.drawText(kHeaderNoteNote, kMenuHeaderTextSize, kMenuHeaderX[1], kMenuHeaderY[1]);
    lcd.drawText(kHeaderNoteRoot, kMenuHeaderTextSize, kMenuHeaderX[4], kMenuHeaderY[4]);
    lcd.drawText(kHeaderNoteScale, kMenuHeaderTextSize, kMenuHeaderX[5], kMenuHeaderY[5]);
    lcd.drawText(kHeaderNoteGlide, kMenuHeaderTextSize, kMenuHeaderX[6], kMenuHeaderY[6]);
    lcd.drawText(kHeaderNoteTranspose, kMenuHeaderTextSize, kMenuHeaderX[7], kMenuHeaderY[7]);

    // 3. SIGNS
    lcd_setMenuSignState(kAccentColor);
    const uint8_t slots[6] = {0, 1, 4, 5, 6, 7};
    for (uint8_t i = 0; i < 6; i++) lcd.drawText((slots[i] == 6) ? "  %" : "   ", kMenuSignTextSize, kMenuSignBoxX[slots[i]], kMenuSignBoxY[slots[i]]);

    // 4. VALUES + STRIP (knobs 3 PAD HOLD and 4 FOLLOW have no label: the strip shows them)
    for (uint8_t i = 0; i < 6; i++) lcd_drawNoteData(slots[i]);
    lcd_drawNoteStrip();
}

void Controller::lcd_drawNoteData(uint8_t slot_) {
    /// @brief Draws one NOTE page value (slot = knob position 0..7).

    if (!notePageActive() || (selectedLayerNum == -1)) return;
    const Layer& layer = layerLibrary[selectedLayerNum];
    const Bank& bank = layer.bankLibrary[activeSongBank];
    char text[kMenuDataTextSize + 1];
    char value[20];
    RGB16Color color = WHITE;

    switch (slot_) {
        case 0:
            snprintf(text, sizeof(text), "%10.10s", kToneModeText[(layer.toneMode < tone::kModeCount) ? layer.toneMode : 0]);
            break;
        case 1:
            if ((selectedBeatNum >= 0) && (selectedBeatNum <= bank.lastActiveBeatNum)) {
                const Beat& beat = bank.beatLibrary[selectedBeatNum];
                char name[8];
                tone::noteName(tone::semitones(beat.getNote(), layer.toneScale, layer.toneRoot), name, sizeof(name));
                snprintf(value, sizeof(value), beat.getSlide() ? "%s SLIDE" : "%s", name);  // ADD + UP / DOWN
                snprintf(text, sizeof(text), "%10.10s", value);
            } else {
                snprintf(text, sizeof(text), "%s", kDataDashR);
            }
            if (!tone::playsNotes(layer.toneMode)) color = GRAY_50;  // edited, but not heard as a DRUM layer
            if (layer.followLayer >= 0) color = GRAY_50;            // v2.04: it plays the followed layer's notes
            break;
        case 4:
            snprintf(text, sizeof(text), "%10.10s", tone::kNoteNames[layer.toneRoot % tone::kRootCount]);
            break;
        case 5:
            snprintf(text, sizeof(text), "%10.10s", tone::kScales[tone::safeScale(layer.toneScale)].name);
            break;
        case 6:
            snprintf(text, sizeof(text), "%10.10s", kNumberDataLibrary[layer.toneGlide].nameLongR);
            break;
        case 7:
            snprintf(value, sizeof(value), (noteTransposeTotal > 0) ? "+%d" : "%d", noteTransposeTotal);
            snprintf(text, sizeof(text), "%10.10s", value);
            break;
        default:
            return;
    }
    lcd_setMenuDataState(color);
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[slot_], kMenuDataY[slot_]);
}

namespace {

// Note strip geometry inside the fill-graph area (x 516..796, y 32..87): a small piano roll,
// time left to right, pitch bottom to top, the root on the middle line.
constexpr uint16_t kNoteRollTopY = kBeatGraphStartY + 13;
constexpr uint16_t kNoteRollBottomY = kBeatGraphEndY - 11;
constexpr uint16_t kNoteRollMidY = (kNoteRollTopY + kNoteRollBottomY) / 2;
constexpr uint16_t kNoteMarkWidth = 4;

}  // namespace

void Controller::lcd_drawNoteStrip() {
    /// @brief Draws the note strip: title, root line and one mark per beat at its pitch.

    if (!notePageActive() || (selectedLayerNum == -1)) return;
    const Layer& layer = layerLibrary[selectedLayerNum];
    const bool toneOn = tone::playsNotes(layer.toneMode);
    // v2.04 harmony: a layer that FOLLOWs another (knob 4) shows the notes it will actually
    // play: the followed layer's beats at the interval, in that layer's key.
    const bool following = toneOn && (layer.followLayer >= 0);
    const Layer& src = following ? layerLibrary[layer.followLayer] : layer;
    const Bank& bank = src.bankLibrary[activeSongBank];
    const Layer& key = layerLibrary[layer_keyLayer(static_cast<uint8_t>(selectedLayerNum))];

    lcd.setBackColor(BLACK);
    lcd.clearRect(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphWidth + 1, kBeatGraphHeight + 1);

    // Title: key and scale, or a reminder that a DRUM layer ignores the notes.
    char title[32];
    snprintf(title, sizeof(title), "NOTES  %s %s", tone::kNoteNames[key.toneRoot % tone::kRootCount], tone::kScales[tone::safeScale(key.toneScale)].name);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.setForeColor(GRAY_75);
    lcd.drawText(title, strlen(title), kBeatGraphStartX, kBeatGraphStartY + 2);
    lcd.setAlignment(RIGHT);
    lcd.setForeColor(GRAY_50);
    if (!toneOn) {
        lcd.drawText("DRUM: MODE TO TONE", 18, kBeatGraphEndX, kBeatGraphStartY + 2);
    } else if (following) {
        // Knob 4 FOLLOW, ADD + knob 4 the interval (the knob has no label: the strip shows it).
        char follow[11];
        harmony::label(layer.followLayer, layer.harmonyInterval, follow);
        char text[24];
        snprintf(text, sizeof(text), "FOLLOW %s", follow);
        lcd.setForeColor(kAccentColor);
        lcd.drawText(text, strlen(text), kBeatGraphEndX, kBeatGraphStartY + 2);
    } else {
        lcd.drawText("ADD+PAD: STEP", 13, kBeatGraphEndX, kBeatGraphStartY + 2);  // pads alone only play
    }

    // Root line (the sample's pitch in this key) for orientation.
    lcd.setForeColor(GRAY_25);
    lcd.drawHLine(kBeatGraphStartX, kNoteRollMidY, kBeatGraphWidth);

    // One mark per beat; half a pixel per semitone keeps +-30 semitones inside the strip. A beat
    // with SLIDE is joined to the beat before it by a line.
    if (songInterval > 0) {
        int previousX = -1, previousY = -1;
        for (int16_t i = 0; i <= bank.lastActiveBeatNum; i++) {
            const Beat& beat = bank.beatLibrary[i];
            if (!beat.getActive()) continue;
            const int8_t degree = following ? harmony::note(beat.getNote(), layer.harmonyInterval, src.toneScale) : beat.getNote();
            const int semis = tone::semitones(degree, key.toneScale, key.toneRoot);
            int y = static_cast<int>(kNoteRollMidY) - (semis / 2);
            if (y < static_cast<int>(kNoteRollTopY)) y = kNoteRollTopY;
            if (y > static_cast<int>(kNoteRollBottomY) - 2) y = kNoteRollBottomY - 2;
            const uint32_t x = kBeatGraphStartX + ((static_cast<uint32_t>(beat.getStartInterval()) * (kBeatGraphWidth - kNoteMarkWidth)) / songInterval);
            if (beat.getSlide() && (previousX >= 0) && (static_cast<int>(x) > previousX + static_cast<int>(kNoteMarkWidth))) {
                lcd.setForeColor(GRAY_75);
                lcd.drawHLine(static_cast<uint16_t>(previousX + kNoteMarkWidth), static_cast<uint16_t>(previousY), static_cast<uint16_t>(static_cast<int>(x) - previousX - kNoteMarkWidth));
                const int y0 = (y < previousY) ? y : previousY;
                const int y1 = (y < previousY) ? previousY : y;
                lcd.drawVLine(static_cast<uint16_t>(x), static_cast<uint16_t>(y0), static_cast<uint16_t>(y1 - y0 + 1));
            }
            const bool selected = !following && (i == selectedBeatNum);
            lcd.setForeColor(selected ? kAccentColor : (toneOn ? WHITE : GRAY_50));
            lcd.fillRect(static_cast<uint16_t>(x), static_cast<uint16_t>(y), kNoteMarkWidth, selected ? 3 : 2);
            previousX = static_cast<int>(x);
            previousY = y;
        }
    }

    // The pads' note range (they are a scale keyboard on a TONE layer; UP/DOWN moves it).
    lcd.setFont(FONT_05x07);
    if (toneOn) {
        char low[8], high[8], keys[32];
        tone::noteName(tone::semitones(tone::keyDegree(0, noteKeysOctave, key.toneScale), key.toneScale, key.toneRoot), low, sizeof(low));
        tone::noteName(tone::semitones(tone::keyDegree(tone::kKeyCount - 1, noteKeysOctave, key.toneScale), key.toneScale, key.toneRoot), high, sizeof(high));
        // PAD HOLD (knob 3): pad notes sound while the pad is pressed.
        snprintf(keys, sizeof(keys), "PADS %s-%s%s", low, high, layer.synth.padHold ? "  HOLD" : "");
        lcd.setAlignment(LEFT);
        lcd.setForeColor(GRAY_50);
        lcd.drawText(keys, strlen(keys), kBeatGraphStartX, kBeatGraphEndY - 6);
    }

    // A CLEAR tap undoes the note edits made since the page opened; holding it erases the layer
    // (as on the EUCLID page).
    lcd.setAlignment(RIGHT);
    lcd.setForeColor(GRAY_50);
    lcd.drawText("CLEAR: UNDO  HOLD: ERASE", 24, kBeatGraphEndX, kBeatGraphEndY - 6);
}

/* Lcd SYNTH Pages (Layer Inst menu of a SYNTH layer) -------------------------*/

void Controller::lcd_drawSynthPage(uint8_t layerNum_) {
    /// @brief Draws the shown page of a SYNTH layer's Layer Instrument menu (SOUND, OSC, FILTER,
    /// AMP or LFO; ROUTING is the normal routing page). Headers and signs come from the knob map
    /// (kEncoderSynthFunction + kEncoderFuncDataLibrary). Clears what a sample page left first.

    if (!synthInstPageActive()) return;
    const uint8_t page = synthPageNow();
    if (page == SYNTH_PAGE_ROUTING) return;

    // 1. CLEAR SLOTS, SAMPLE DETAILS AND THE DISPLAY AREA
    lcd.setBackColor(BLACK);
    const uint8_t slots[6] = {0, 1, 4, 5, 6, 7};
    for (uint8_t i = 0; i < 6; i++) lcd_clearMenuTab(slots[i]);
    lcd.clearRect(kMenuSignBoxX[1] - 70, kMenuSignBoxY[1], 70, kMenuSignBoxHeight);
    lcd.setForeColor(GRAY_50);
    lcd.clearRect(493, 30, 325, 60);
    lcd.drawVLine(kMenuLineX[3], kMenuLineY[3], kMenuLineHeight);

    // 2. ICON + LAYER NUMBER
    lcd_drawMenuIcon(menu);
    lcd_setMenuHeaderState(kAccentColor);
    lcd.drawText(kDataMenuNum + layerNum_, 1, kMenuNumX, kMenuNumY);

    // 3. HEADERS + SIGNS (LEVEL/PITCH draw their own sign: the LFO assigned with ADD + knob)
    for (uint8_t i = 0; i < 6; i++) {
        const EncoderFunc f = kEncoderSynthFunction[page][slots[i]];
        if (f == ENC_FNC_NONE) continue;
        lcd_setMenuHeaderState(kAccentColor);
        lcd.drawText(kEncoderFuncDataLibrary[f].funcHeader, kMenuHeaderTextSize, kMenuHeaderX[slots[i]], kMenuHeaderY[slots[i]]);
        if ((f == ENC_INST_0_LEVEL) || (f == ENC_INST_0_PITCH)) continue;
        lcd_setMenuSignState(kAccentColor);
        lcd.drawText(kEncoderFuncDataLibrary[f].sign, kMenuSignTextSize, kMenuSignBoxX[slots[i]], kMenuSignBoxY[slots[i]]);
    }

    // 4. VALUES + DISPLAY
    for (uint8_t i = 0; i < 6; i++) {
        const EncoderFunc f = kEncoderSynthFunction[page][slots[i]];
        if (f == ENC_INST_0_LEVEL) {
            lcd_drawLayerInst_LevelData(layerNum_);
        } else if (f == ENC_INST_0_PITCH) {
            lcd_drawLayerInst_PitchData(layerNum_);
        } else if (f != ENC_FNC_NONE) {
            lcd_drawSynthData(slots[i]);
        }
    }
    lcd_drawSynthGraph();

    // 5. The slot clear above also erased the tab-selection marker: put it back.
    if (menuTab < 8) {
        lcd.drawRGB16Image((const RGB16Color*)(RAM_ICON_SELECT_PALETTE_ADDRESS), (const uint8_t*)(RAM_ICON_SELECT_ON_DATA_ADDRESS), 64,
                           kMenuSelectX[menuTab], kMenuSelectY[menuTab], kIconSelectWidth, kIconSelectHeight);
    }
}

void Controller::lcd_drawSynthData(uint8_t slot_) {
    /// @brief Draws one SYNTH value of the shown page (slot = knob position 0..7).

    if (!synthInstPageActive() || (slot_ >= 8)) return;
    const uint8_t page = synthPageNow();
    if (page == SYNTH_PAGE_ROUTING) return;
    const EncoderFunc f = kEncoderSynthFunction[page][slot_];
    const synth::Params p = synth::sanitize(layerLibrary[selectedLayerNum].synth);
    const char* name = nullptr;  // a word, or nullptr for a number
    uint8_t value = 0;
    char buffer[12];
    switch (f) {
        case ENC_SYNTH_PRESET: {
            int16_t index = synthPresetIndex[selectedLayerNum];
            if (index < 0) index = synth_matchPreset(static_cast<uint8_t>(selectedLayerNum));  // e.g. after loading
            if (index < 0) {
                name = "OWN SOUND";
            } else if (index < synth::kFactoryPresetCount) {
                name = synth::kFactoryPresets[index].name;
            } else {
                snprintf(buffer, sizeof(buffer), "USER %02d", index - synth::kFactoryPresetCount + 1);
                name = buffer;
            }
            break;
        }
        case ENC_SYNTH_SAVE_SLOT:
            snprintf(buffer, sizeof(buffer), "USER %02u%s", static_cast<unsigned>(synthSaveSlot + 1), synthUserUsed[synthSaveSlot] ? " *" : "");
            name = buffer;
            break;
        case ENC_SYNTH_WAVE: name = synth::kWaveNames[p.wave]; break;
        case ENC_SYNTH_FILTER_TYPE: name = synth::kFilterTypeNames[p.filterType]; break;
        case ENC_SYNTH_LFO_SHAPE: name = synth::kLfoShapeNames[p.lfoShape]; break;
        case ENC_SYNTH_LFO_RATE: name = synth::kLfoRates[p.lfoRate].name; break;
        case ENC_SYNTH_LFO_TARGET: name = synth::kLfoTargetNames[p.lfoTarget]; break;
        case ENC_SYNTH_LFO_RETRIG: name = p.lfoRetrig ? "NOTE" : "FREE"; break;
        case ENC_SYNTH_PAD_HOLD: name = p.padHold ? "ON" : "OFF"; break;
        case ENC_SYNTH_LFO_DEPTH:
            if (p.lfoDepth == 0) name = "OFF";
            value = p.lfoDepth;
            break;
        case ENC_SYNTH_DETUNE: value = p.detune; break;
        case ENC_SYNTH_SUB: value = p.sub; break;
        case ENC_SYNTH_NOISE: value = p.noise; break;
        case ENC_SYNTH_CUTOFF: value = p.cutoff; break;
        case ENC_SYNTH_RESO: value = p.resonance; break;
        case ENC_SYNTH_ENVAMT: value = p.envAmount; break;
        case ENC_SYNTH_DECAY: value = p.decay; break;
        case ENC_SYNTH_GATE: value = p.gate; break;
        case ENC_SYNTH_DRIVE: value = p.drive; break;
        case ENC_SYNTH_ATTACK: value = p.attack; break;
        case ENC_SYNTH_AMP_DECAY: value = p.ampDecay; break;
        case ENC_SYNTH_SUSTAIN: value = p.sustain; break;
        case ENC_SYNTH_RELEASE: value = p.release; break;
        // MOD page (v2.05)
        case ENC_SYNTH_FM_RATIO:
            snprintf(buffer, sizeof(buffer), "%s:1", synth::kFmRatioNames[p.fmRatio]);
            name = buffer;
            break;
        case ENC_SYNTH_FM_AMOUNT: value = p.fmAmount; break;
        case ENC_SYNTH_FM_DECAY:
            if (p.fmDecay >= synth::kMaxParam) name = "HOLD";
            value = p.fmDecay;
            break;
        case ENC_SYNTH_PITCH_ENV:
            if (p.pitchEnv == 0) name = "OFF";
            value = p.pitchEnv;
            break;
        case ENC_SYNTH_PITCH_DECAY: value = p.pitchDecay; break;
        case ENC_SYNTH_FILT_ATTACK:
            if (p.filterAttack == 0) name = "OFF";
            value = p.filterAttack;
            break;
        default: return;  // empty slot, or LEVEL/PITCH (drawn by their own functions)
    }
    char text[kMenuDataTextSize + 1];
    snprintf(text, sizeof(text), "%10.10s", (name != nullptr) ? name : kNumberDataLibrary[value].nameLongR);
    lcd_setMenuDataState(WHITE);
    lcd.drawText(text, kMenuDataTextSize, kMenuDataX[slot_], kMenuDataY[slot_]);
}

void Controller::lcd_drawSynthSoundGraph() {
    /// @brief SOUND page: the preset name large, where it comes from, the save slot and the
    /// last message (saved / random / undone), plus what ADD and CLEAR do here.

    const synth::Params p = synth::sanitize(layerLibrary[selectedLayerNum].synth);
    int16_t index = synthPresetIndex[selectedLayerNum];
    if (index < 0) index = synth_matchPreset(static_cast<uint8_t>(selectedLayerNum));  // e.g. after loading
    char line[40];
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.setForeColor(GRAY_75);
    if (index < 0) {
        snprintf(line, sizeof(line), "SYNTH  SOUND");
    } else if (index < synth::kFactoryPresetCount) {
        snprintf(line, sizeof(line), "SYNTH  SOUND %d/%d", index + 1, synth::kFactoryPresetCount);
    } else {
        snprintf(line, sizeof(line), "SYNTH  SOUND  USER");
    }
    lcd.drawText(line, strlen(line), kBeatGraphStartX, kBeatGraphStartY + 2);

    // The name, big.
    char name[16];
    if (index < 0) {
        snprintf(name, sizeof(name), "OWN SOUND");
    } else if (index < synth::kFactoryPresetCount) {
        snprintf(name, sizeof(name), "%s", synth::kFactoryPresets[index].name);
    } else {
        snprintf(name, sizeof(name), "USER %02d", index - synth::kFactoryPresetCount + 1);
    }
    lcd.setFont(FONT_10x14);
    lcd.setForeColor(kAccentColor);
    lcd.drawText(name, strlen(name), kBeatGraphStartX, kBeatGraphStartY + 14);

    // A one-line summary of what it is made of.
    snprintf(line, sizeof(line), "%s %s  CUT %d  RES %d", synth::kWaveNames[p.wave], synth::kFilterTypeNames[p.filterType], p.cutoff, p.resonance);
    lcd.setFont(FONT_05x07);
    lcd.setForeColor(GRAY_50);
    lcd.drawText(line, strlen(line), kBeatGraphStartX, kBeatGraphStartY + 32);

    // Message (saved, random...) or the hints.
    if (synthStatus[0] != '\0') {
        lcd.setForeColor(kAccent2Color);
        lcd.drawText(synthStatus, strlen(synthStatus), kBeatGraphStartX, kBeatGraphEndY - 6);
    } else {
        snprintf(line, sizeof(line), "ADD: SAVE TO USER %02u", static_cast<unsigned>(synthSaveSlot + 1));
        lcd.setForeColor(GRAY_50);
        lcd.drawText(line, strlen(line), kBeatGraphStartX, kBeatGraphEndY - 6);
    }
    lcd.setAlignment(RIGHT);
    lcd.setForeColor(GRAY_50);
    lcd.drawText("CLEAR: UNDO", 11, kBeatGraphEndX, kBeatGraphEndY - 6);
    lcd.setAlignment(LEFT);
}

void Controller::lcd_drawSynthAmpGraph() {
    /// @brief AMP page: the shape of the level over a held note and its release (ATTACK, DECAY
    /// to SUSTAIN, RELEASE), on a square-root time axis so short and long times both show.

    const synth::Params p = synth::sanitize(layerLibrary[selectedLayerNum].synth);
    const uint16_t top = kBeatGraphStartY + 13;
    const uint16_t bottom = kBeatGraphEndY - 10;
    const uint16_t height = bottom - top;
    const float a = sqrtf(synth::attackSeconds(p.attack));
    const float d = sqrtf(3.0f * synth::decaySeconds(p.ampDecay));
    const float hold = 0.5f;  // a fixed "held" part, so the sustain level is visible
    const float r = sqrtf(3.0f * synth::decaySeconds(p.release));
    const float total = a + d + hold + r;
    const float sustain = p.sustain / 100.0f;
    lcd.setForeColor(GRAY_25);
    lcd.drawHLine(kBeatGraphStartX, bottom, kBeatGraphWidth);
    lcd.setForeColor(kAccentColor);
    int previous = -1;
    for (uint16_t x = 0; x < kBeatGraphWidth; x++) {
        const float t = (static_cast<float>(x) / kBeatGraphWidth) * total;
        float level;
        if (t < a) {
            level = t / a;
        } else if (t < a + d) {
            const float k = (t - a) / d;  // 0..1 over 3 time constants
            level = sustain + ((1.0f - sustain) * expf(-3.0f * k));
        } else if (t < a + d + hold) {
            level = sustain + ((1.0f - sustain) * expf(-3.0f));
        } else {
            const float k = (t - a - d - hold) / r;
            level = (sustain + ((1.0f - sustain) * expf(-3.0f))) * expf(-3.0f * k);
        }
        const int y = bottom - static_cast<int>(level * height);
        if (previous < 0) previous = y;
        const int y0 = (y < previous) ? y : previous;
        const int y1 = (y < previous) ? previous : y;
        lcd.drawVLine(kBeatGraphStartX + x, static_cast<uint16_t>(y0), static_cast<uint16_t>(y1 - y0 + 1));
        previous = y;
    }
    // Where the note is let go.
    const uint16_t releaseX = static_cast<uint16_t>(((a + d + hold) / total) * kBeatGraphWidth);
    lcd.setForeColor(GRAY_50);
    lcd.drawVLine(kBeatGraphStartX + releaseX, top, height);
    char line[40];
    snprintf(line, sizeof(line), "GATE %d%%  ADD: RANDOM  TAP: VARY", p.gate);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.setForeColor(GRAY_50);
    lcd.drawText(line, strlen(line), kBeatGraphStartX, kBeatGraphEndY - 6);
}

void Controller::lcd_drawSynthLfoGraph() {
    /// @brief LFO page: two cycles of the shape, as tall as DEPTH, with what it moves and how
    /// fast; the moving position is drawn by lcd_updateSynthLfoCursor().

    if (!synthLfoPageActive()) return;
    const synth::Params p = synth::sanitize(layerLibrary[selectedLayerNum].synth);
    char info[24];
    if (p.lfoDepth == 0) {
        snprintf(info, sizeof(info), "OFF: TURN DEPTH");
    } else {
        snprintf(info, sizeof(info), "%s %s", synth::kLfoTargetNames[p.lfoTarget], synth::kLfoRates[p.lfoRate].name);
    }
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(RIGHT);
    lcd.setForeColor((p.lfoDepth == 0) ? kAccent2Color : GRAY_75);
    lcd.drawText(info, static_cast<uint8_t>(strlen(info)), kBeatGraphEndX, kBeatGraphStartY + 2);
    lcd.setAlignment(LEFT);

    const uint16_t top = kBeatGraphStartY + 13;
    const uint16_t bottom = kBeatGraphEndY - 4;
    const uint16_t mid = top + ((bottom - top) / 2);
    const float amplitude = ((bottom - top) / 2 - 1) * ((p.lfoDepth == 0) ? 0.0f : (0.25f + (0.75f * (p.lfoDepth / 100.0f))));
    lcd.setForeColor(GRAY_25);
    lcd.drawHLine(kBeatGraphStartX, mid, kBeatGraphWidth);
    lcd.setForeColor(kAccentColor);
    int previous = -1;
    for (uint16_t x = 0; x < kBeatGraphWidth; x++) {
        float t = (2.0f * x) / kBeatGraphWidth;
        t -= static_cast<float>(static_cast<int>(t));
        // RANDOM is drawn as steps of a fixed pattern (the real values change every cycle).
        const float hold = (static_cast<int>((4.0f * x) / kBeatGraphWidth) % 2) ? -0.6f : 0.8f;
        const float s = synth::lfoValue(p.lfoShape, t, hold);
        const int y = mid - static_cast<int>(s * amplitude);
        if (previous < 0) previous = y;
        const int y0 = (y < previous) ? y : previous;
        const int y1 = (y < previous) ? previous : y;
        lcd.drawVLine(kBeatGraphStartX + x, static_cast<uint16_t>(y0), static_cast<uint16_t>(y1 - y0 + 1));
        previous = y;
    }
    synthLfoCursorX = -1;  // the graph was redrawn: no old cursor to erase
}

void Controller::lcd_drawSynthModGraph() {
    /// @brief MOD page (v2.05): on the left one cycle of the FM wave at its full depth (or a
    /// hint that FM needs WAVE = FM), on the right the pitch drop (gold) and the filter
    /// envelope's rise (grey) over the first half second.

    const synth::Params p = synth::sanitize(layerLibrary[selectedLayerNum].synth);
    const bool fm = (p.wave == synth::kFm);
    char info[32];
    if (fm) {
        snprintf(info, sizeof(info), "FM %s:1  DEPTH %d", synth::kFmRatioNames[p.fmRatio], p.fmAmount);
    } else {
        snprintf(info, sizeof(info), "FM: SET WAVE TO FM (OSC)");
    }
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(RIGHT);
    lcd.setForeColor(fm ? GRAY_75 : kAccent2Color);
    lcd.drawText(info, static_cast<uint8_t>(strlen(info)), kBeatGraphEndX, kBeatGraphStartY + 2);
    lcd.setAlignment(LEFT);

    const uint16_t top = kBeatGraphStartY + 13;
    const uint16_t bottom = kBeatGraphEndY - 4;
    const uint16_t height = bottom - top;
    const uint16_t half = kBeatGraphWidth / 2;

    // Left: the FM wave.
    if (fm) {
        const uint16_t mid = top + (height / 2);
        const float index = synth::fmIndexFor(p.fmAmount);
        lcd.setForeColor(GRAY_25);
        lcd.drawHLine(kBeatGraphStartX, mid, half - 6);
        lcd.setForeColor(kAccentColor);
        int previous = -1;
        for (uint16_t x = 0; x < half - 6; x++) {
            const float t = static_cast<float>(x) / static_cast<float>(half - 6);
            const int y = mid - static_cast<int>(synth::fmShape(t, p.fmRatio, index) * (height / 2 - 1));
            if (previous < 0) previous = y;
            const int y0 = (y < previous) ? y : previous;
            const int y1 = (y < previous) ? previous : y;
            lcd.drawVLine(kBeatGraphStartX + x, static_cast<uint16_t>(y0), static_cast<uint16_t>(y1 - y0 + 1));
            previous = y;
        }
    }

    // Right: PITCH ENV (gold, how far above the note) and FILTER ATTACK (grey, 0 -> open).
    const uint16_t x0 = kBeatGraphStartX + half + 6;
    const uint16_t w = kBeatGraphWidth - half - 6;
    lcd.setForeColor(GRAY_25);
    lcd.drawHLine(x0, bottom, w);
    const float pitchTau = synth::pitchDecaySeconds(p.pitchDecay);
    const float attack = (p.filterAttack > 0) ? synth::attackSeconds(p.filterAttack) : 0.0f;
    for (uint16_t x = 0; x < w; x += 2) {
        const float t = 0.5f * static_cast<float>(x) / static_cast<float>(w);  // 0 .. 0.5 s
        if (p.filterAttack > 0) {
            const float rise = (t < attack) ? (t / attack) : 1.0f;
            const uint16_t h = static_cast<uint16_t>(rise * height * 0.9f);
            lcd.setForeColor(GRAY_50);
            if (h > 0) lcd.drawVLine(x0 + x, bottom - h, 1);
        }
        if (p.pitchEnv > 0) {
            const float drop = (static_cast<float>(p.pitchEnv) / 100.0f) * expf(-t / pitchTau);
            const uint16_t h = static_cast<uint16_t>(drop * height);
            lcd.setForeColor(kAccentColor);
            if (h > 0) lcd.drawVLine(x0 + x, bottom - h, h);
        }
    }
    if ((p.pitchEnv == 0) && (p.filterAttack == 0)) {
        lcd.setForeColor(GRAY_50);
        lcd.drawText("PITCH ENV / FILT ATK OFF", 24, x0, top + (height / 2) - 3);
    }
}

void Controller::lcd_updateSynthLfoCursor() {
    /// @brief Main loop: a small marker under the LFO graph at the voice's current LFO position
    /// (first of the two cycles). Only redraws when it moved.

    if (!synthLfoPageActive()) return;
    const synth::Params& p = layerLibrary[selectedLayerNum].synth;
    if (p.lfoDepth == 0) return;
    const float phase = synthVoice[selectedLayerNum].lfoPhase;  // a float read: atomic on the M7
    if (!(phase >= 0.0f) || (phase >= 1.0f)) return;
    const int16_t x = static_cast<int16_t>(phase * (kBeatGraphWidth / 2));
    if (x == synthLfoCursorX) return;
    const uint16_t y = kBeatGraphEndY - 2;
    if (synthLfoCursorX >= 0) {
        lcd.setForeColor(BLACK);
        lcd.fillRect(kBeatGraphStartX + synthLfoCursorX, y, 3, 3);
    }
    lcd.setForeColor(WHITE);
    lcd.fillRect(kBeatGraphStartX + x, y, 3, 3);
    synthLfoCursorX = x;
}

void Controller::lcd_drawSynthGraph() {
    /// @brief The display area of the shown synth page: SOUND = preset, OSC = one cycle of the
    /// waveform, FILTER = the filter envelope over 1 s, AMP = the level envelope, LFO = the wave.

    if (!synthInstPageActive()) return;
    const uint8_t page = synthPageNow();
    if (page == SYNTH_PAGE_ROUTING) return;
    const synth::Params p = synth::sanitize(layerLibrary[selectedLayerNum].synth);

    lcd.setBackColor(BLACK);
    lcd.clearRect(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphWidth + 1, kBeatGraphHeight + 1);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.setForeColor(GRAY_75);
    if (page != SYNTH_PAGE_SOUND) {
        char title[24];
        snprintf(title, sizeof(title), "SYNTH  %s", kSynthPageNames[page]);
        lcd.drawText(title, strlen(title), kBeatGraphStartX, kBeatGraphStartY + 2);
    }
    switch (page) {
        case SYNTH_PAGE_SOUND:
            lcd_drawSynthSoundGraph();
            return;
        case SYNTH_PAGE_AMP:
            lcd_drawSynthAmpGraph();
            return;
        case SYNTH_PAGE_LFO:
            lcd_drawSynthLfoGraph();
            return;
        case SYNTH_PAGE_MOD:
            lcd_drawSynthModGraph();
            return;
        default:
            break;
    }

    const uint16_t top = kBeatGraphStartY + 13;
    const uint16_t bottom = kBeatGraphEndY - 4;
    const uint16_t height = bottom - top;

    if (page == SYNTH_PAGE_OSC) {
        // Waveform, 2 cycles across the strip. WAVE = SAMPLE shows the layer's loaded sample as
        // the cycle (or a hint if there is none).
        const Layer& layer = layerLibrary[selectedLayerNum];
        synth::Voice shape;
        if ((p.wave == synth::kSample) && (layer.playSampleSector != -1) && !layer.sampleLoading &&
            sampleSectorLibrary[layer.playSampleSector].active && (sampleSectorLibrary[layer.playSampleSector].size >= 2)) {
            shape.table = reinterpret_cast<const float*>(RAM_SAMPLE_ADDRESS + (layer.playSampleSector * kSampleByteSize));
            shape.tableLength = sampleSectorLibrary[layer.playSampleSector].size;
        } else if (p.wave == synth::kSample) {
            lcd.setAlignment(RIGHT);
            lcd.setForeColor(kAccent2Color);
            lcd.drawText("NO SAMPLE LOADED", 16, kBeatGraphEndX, kBeatGraphStartY + 2);  // load one in TONE/DRUM mode
            lcd.setAlignment(LEFT);
        }
        const uint16_t mid = top + (height / 2);
        lcd.setForeColor(GRAY_25);
        lcd.drawHLine(kBeatGraphStartX, mid, kBeatGraphWidth);
        lcd.setForeColor(kAccentColor);
        int previous = -1;
        for (uint16_t x = 0; x < kBeatGraphWidth; x++) {
            float t = (2.0f * x) / kBeatGraphWidth;
            t -= static_cast<float>(static_cast<int>(t));
            // FM (v2.05): the shape at its full depth (how bright the attack is).
            float s = (p.wave == synth::kFm) ? synth::fmShape(t, p.fmRatio, synth::fmIndexFor(p.fmAmount)) : synth::voiceOscillator(shape, p.wave, t, 0.0f);
            if (s > 1.0f) s = 1.0f;
            if (s < -1.0f) s = -1.0f;
            const int y = mid - static_cast<int>(s * (height / 2 - 1));
            if (previous < 0) previous = y;
            const int y0 = (y < previous) ? y : previous;
            const int y1 = (y < previous) ? previous : y;
            lcd.drawVLine(kBeatGraphStartX + x, static_cast<uint16_t>(y0), static_cast<uint16_t>(y1 - y0 + 1));
            previous = y;
        }
    } else {
        // FILTER: the filter envelope over 1 s (starts at ENV AMT, decays with DECAY), with the
        // filter type named.
        lcd.setAlignment(RIGHT);
        lcd.setForeColor(GRAY_75);
        lcd.drawText(synth::kFilterTypeNames[p.filterType], strlen(synth::kFilterTypeNames[p.filterType]), kBeatGraphEndX, kBeatGraphStartY + 2);
        lcd.setAlignment(LEFT);
        const float seconds = 0.02f * exp2f((p.decay / 100.0f) * 6.64f);
        const float amount = p.envAmount / 100.0f;
        lcd.setForeColor(kAccentColor);
        for (uint16_t x = 0; x < kBeatGraphWidth; x += 2) {
            const float t = static_cast<float>(x) / kBeatGraphWidth;
            const float e = amount * expf(-t / seconds);
            const uint16_t h = static_cast<uint16_t>(e * height);
            if (h > 0) lcd.drawVLine(kBeatGraphStartX + x, bottom - h, h);
        }
    }
}

void Controller::lcd_redrawNotePage() {
    /// @brief After a selection or note change: refreshes the NOTE value and the strip.

    if (!notePageActive()) return;
    lcd_drawNoteData(1);
    lcd_drawNoteStrip();
}

void Controller::lcd_drawLayerSong_BeatFillGraphData(uint8_t layerNum_) {
    if (layerSongAltPageActive()) return;  // the EUCLID and NOTE pages use these slots

    /// @brief Draws beat-fill graph for selected beat and handles clear/reset states.

    // 1. CONTEXT LOAD
    // Resolve layer and active bank references.
    Layer& layer = layerLibrary[layerNum_];
    Bank& bank = layer.bankLibrary[activeSongBank];
    if (selectedBeatNum != -1) {
        // Get beat and its parameters
        Beat& beat = bank.beatLibrary[selectedBeatNum];
        uint8_t fillType = beat.getFillType();
        uint8_t fillPattern = beat.getFillPattern();
        uint8_t fillTime = beat.getFillTime();
        uint8_t fillLevel = beat.getFillLevel();

        // Redraw graph if parameters changed
        if ((fillType != preBeatFillType) || (fillPattern != preBeatFillPattern) || (fillTime != preBeatFillTime) || (fillLevel != preBeatFillLevel)) {
            const uint16_t graphDataStartX = kBeatGraphStartX + 1;
            const uint16_t graphDataEndX = kBeatGraphStartX + kBeatGraphWidth - 1;  // exclusive
            uint16_t drawCursorX = graphDataStartX;

            const uint16_t gridX0 = kBeatGraphStartX;
            const uint16_t gridX1 = kBeatGraphStartX + (kBeatGraphWidth / 4);
            const uint16_t gridX2 = kBeatGraphStartX + (kBeatGraphWidth / 2);
            const uint16_t gridX3 = kBeatGraphStartX + (3 * kBeatGraphWidth / 4);

            auto clearGapTo = [&](uint16_t nextX) {
                if (nextX > drawCursorX) {
                    lcd.setForeColor(BLACK);
                    lcd.fillRect(drawCursorX, kBeatGraphStartY, nextX - drawCursorX, kBeatGraphHeight);
                }
            };

            auto drawBar = [&](uint16_t xGraph, uint16_t yGraph, uint8_t beatWidth, uint8_t yGraphSize) {
                uint16_t clampedX = xGraph;
                if (clampedX < graphDataStartX) clampedX = graphDataStartX;
                if (clampedX >= graphDataEndX) return;

                uint16_t barWidth = beatWidth;
                if ((clampedX + barWidth) > graphDataEndX) barWidth = graphDataEndX - clampedX;
                if (barWidth == 0) return;

                // Clear previous frame content before drawing the new bar shape.
                lcd.setForeColor(BLACK);
                lcd.fillRect(clampedX, kBeatGraphStartY, barWidth, kBeatGraphHeight);

                lcd.setForeColor(WHITE);
                if (yGraphSize > 0) lcd.fillRect(clampedX, yGraph, barWidth, yGraphSize);

                uint16_t barEndX = clampedX + barWidth;
                drawCursorX = (barEndX > drawCursorX) ? barEndX : drawCursorX;
            };

            // Draw complex fill data
            if ((fillType > 0) || ((fillType == 0) && (fillPattern > 2))) {
                BeatFill& beatFillRef = bank.beatFillLibrary[beat.getFillTarget()];
                uint8_t stepSize = beatFillRef.getStepSize();
                for (uint8_t i = 0; i < stepSize; i++) {
                    // Calculate graph position and size for each micro-beat
                    uint16_t startInterval = beat.getStartInterval();
                    uint16_t endInterval = beat.getEndInterval();
                    uint16_t fillInterval = beatFillRef.beatMicroLibrary[i].getInterval();
                    float multiplier = (float)(fillInterval - startInterval) / (endInterval - startInterval);
                    float level = beatFillRef.beatMicroLibrary[i].getLevel();
                    uint16_t xGraph = kBeatGraphStartX + (multiplier * kBeatGraphWidth);
                    uint8_t yGraphSize = (level / 100) * kBeatGraphBeatHeight;
                    uint16_t yGraph = kBeatGraphStartY + (kBeatGraphHeight - yGraphSize);

                    // Calculate width for this beat
                    uint16_t nextFillInterval;
                    float nextMultiplier;
                    uint16_t nextXGraph;
                    uint16_t diffX;
                    uint8_t beatWidth;
                    if (i < (stepSize - 1)) {
                        nextFillInterval = beatFillRef.beatMicroLibrary[i + 1].getInterval();
                        nextMultiplier = (float)(nextFillInterval - startInterval) / (endInterval - startInterval);
                        nextXGraph = kBeatGraphStartX + (nextMultiplier * kBeatGraphWidth);
                    } else {
                        nextFillInterval = beat.getEndInterval();
                        nextMultiplier = (float)(nextFillInterval - startInterval) / (endInterval - startInterval);
                        nextXGraph = kBeatGraphStartX + (nextMultiplier * kBeatGraphWidth);
                    }
                    diffX = nextXGraph - xGraph;
                    if (diffX >= 40) {
                        beatWidth = kBeatGraphBeatWidth[4];
                    } else if (diffX >= 30) {
                        beatWidth = kBeatGraphBeatWidth[3];
                    } else if (diffX >= 20) {
                        beatWidth = kBeatGraphBeatWidth[2];
                    } else if (diffX >= 10) {
                        beatWidth = kBeatGraphBeatWidth[1];
                    } else {
                        beatWidth = kBeatGraphBeatWidth[0];
                    }

                    // Draw from left to right and paint gaps black between beats.
                    clearGapTo(xGraph);
                    drawBar(xGraph, yGraph, beatWidth, yGraphSize);
                }

                // Clear remaining tail after last micro-beat.
                clearGapTo(graphDataEndX);
            } else {
                // Draw simple fill data
                uint16_t xGraph = kBeatGraphStartX;
                uint8_t beatWidth = kBeatGraphBeatWidth[3];
                switch (fillPattern) {
                    case 0:  // Flat
                        clearGapTo(xGraph);
                        drawBar(xGraph, kBeatGraphStartY, beatWidth, kBeatGraphBeatHeight);
                        clearGapTo(graphDataEndX);
                        break;

                    case 1:  // Half
                        clearGapTo(xGraph);
                        drawBar(xGraph, kBeatGraphStartY + (kBeatGraphBeatHeight / 2), beatWidth, (kBeatGraphBeatHeight / 2) + 1);
                        clearGapTo(graphDataEndX);
                        break;

                    case 2:  // Muted
                        clearGapTo(graphDataEndX);
                        break;
                }
            }

            // Draw graph grid after bars in all cases.
            lcd.setForeColor(WHITE);
            lcd.drawVLine(gridX0, kBeatGraphStartY, kBeatGraphHeight);
            for (uint8_t i = 0; i < 18; i++) {
                uint16_t y = kBeatGraphStartY + (i * 3);
                lcd.drawPixel(gridX1, y);
                lcd.drawPixel(gridX2, y);
                lcd.drawPixel(gridX3, y);
            }
        }

        // Save current parameters
        preBeatFillType = fillType;
        preBeatFillPattern = fillPattern;
        preBeatFillTime = fillTime;
        preBeatFillLevel = fillLevel;
    } else if ((preMenu != LAYER_SONG_MENU) || (preBeatFillType != -1)) {
        // Clear and reset graph on menu change or deselection
        lcd.clearRect(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphWidth - 2, kBeatGraphHeight);
        lcd.drawVLine(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphHeight);
        for (uint8_t i = 0; i < 18; i++) {
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 2), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (3 * kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
        }
        preBeatFillType = -1;
        preBeatFillPattern = -1;
        preBeatFillTime = -1;
        preBeatFillLevel = -1;
    } else {
        // Default clear state
        lcd.clearRect(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphWidth - 2, kBeatGraphHeight - 1);
        lcd.drawVLine(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphHeight);
        for (uint8_t i = 0; i < 18; i++) {
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 2), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (3 * kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
        }
        preBeatFillType = -1;
        preBeatFillPattern = -1;
        preBeatFillTime = -1;
        preBeatFillLevel = -1;
    }
}

