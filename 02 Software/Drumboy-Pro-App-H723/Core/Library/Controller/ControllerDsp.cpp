// EQ, filter, effect, reverb and LFO parameter handling.
// Part of class Controller (declared in Controller.h); split out of Controller.cpp.

#include "Controller.h"

////////////////////////////////////////////////////////////////////////////////
/* Eq Functions --------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::eq_select() {
    /// @brief Switches to EQ menu and prepares UI/encoder context for equalizer operations.
    if (menu != EQ_MENU) {
        // 1. MENU STATE TRANSITION
        // Store previous menu context and activate EQ_MENU tab root.
        preMenu = menu;
        menu = EQ_MENU;
        preMenuTab = menuTab;
        menuTab = 0;

        // 2. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 3. UI TRANSITION & RENDER
        // Animate menu change and render EQ screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawEqMenu();

        // 4. ENCODER CONTEXT UPDATE
        // Rebind encoders to EQ menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::eq_reset() {
    /// @brief Restores EQ state parameters to default values.

    // 1. CORE PARAMETER RESET
    // Re-apply active, Q, shelf frequency/gain defaults.
    eq_setActive(kInitialEqActive);
    eq_setQ(kInitialEqQ);
    eq_setFreqLowShelf(kInitialEqFreqLowShelf);
    eq_setGainLowShelf(kInitialEqGainLowShelf);
    eq_setFreqHighShelf(kInitialEqFreqHighShelf);
    eq_setGainHighShelf(kInitialEqGainHighShelf);

    // 2. PEAK BAND RESET
    // Re-apply frequency/gain defaults to all peak bands.
    for (uint8_t i = 0; i < 4; i++) {
        eq_setFreqPeak(i, kInitialEqFreqPeak[i]);
        eq_setGainPeak(i, kInitialEqGainPeak[i]);
    }
}

void Controller::eq_menuRight() {
    /// @brief Advances EQ menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab < kMaxMenu8Tab) ? (menuTab + 1) : kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::eq_menuLeft() {
    /// @brief Moves EQ menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab > kMinMenu8Tab) ? (menuTab - 1) : kMaxMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::eq_menuUp() {
    /// @brief Increments or toggles the parameter mapped to active EQ tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific EQ setting.
    switch (menuTab) {
        case 0:
            eq_setActive(!eq.active);
            break;

        case 1:
            if (eq.q < kMaxEqQ) eq_setQ(eq.q + 1);
            break;

        case 2:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqLowShelf < kMaxEqFreq)
                    eq_setFreqLowShelf(eq.freqLowShelf + 1);
            } else if (eq.gainLowShelf < kMaxEqGain) {
                eq_setGainLowShelf(eq.gainLowShelf + 1);
            }
            break;

        case 3:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqHighShelf < kMaxEqFreq)
                    eq_setFreqHighShelf(eq.freqHighShelf + 1);
            } else if (eq.gainHighShelf < kMaxEqGain)
                eq_setGainHighShelf(eq.gainHighShelf + 1);
            break;

        case 4:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[0] < kMaxEqFreq)
                    eq_setFreqPeak(0, eq.freqPeak[0] + 1);
            } else if (eq.gainPeak[0] < kMaxEqGain)
                eq_setGainPeak(0, eq.gainPeak[0] + 1);
            break;

        case 5:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[1] < kMaxEqFreq)
                    eq_setFreqPeak(1, eq.freqPeak[1] + 1);
            } else if (eq.gainPeak[1] < kMaxEqGain)
                eq_setGainPeak(1, eq.gainPeak[1] + 1);
            break;

        case 6:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[2] < kMaxEqFreq)
                    eq_setFreqPeak(2, eq.freqPeak[2] + 1);
            } else if (eq.gainPeak[2] < kMaxEqGain)
                eq_setGainPeak(2, eq.gainPeak[2] + 1);
            break;

        case 7:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[3] < kMaxEqFreq)
                    eq_setFreqPeak(3, eq.freqPeak[3] + 1);
            } else if (eq.gainPeak[3] < kMaxEqGain)
                eq_setGainPeak(3, eq.gainPeak[3] + 1);
            break;
    }
}

void Controller::eq_menuDown() {
    /// @brief Decrements or toggles the parameter mapped to active EQ tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific EQ setting.
    switch (menuTab) {
        case 0:
            eq_setActive(!eq.active);
            break;

        case 1:
            if (eq.q > kMinEqQ) eq_setQ(eq.q - 1);
            break;

        case 2:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqLowShelf > kMinEqFreq)
                    eq_setFreqLowShelf(eq.freqLowShelf - 1);
            } else if (eq.gainLowShelf > kMinEqGain)
                eq_setGainLowShelf(eq.gainLowShelf - 1);
            break;

        case 3:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqHighShelf > kMinEqFreq)
                    eq_setFreqHighShelf(eq.freqHighShelf - 1);
            } else if (eq.gainHighShelf > kMinEqGain)
                eq_setGainHighShelf(eq.gainHighShelf - 1);
            break;

        case 4:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[0] > kMinEqFreq)
                    eq_setFreqPeak(0, eq.freqPeak[0] - 1);
            } else if (eq.gainPeak[0] > kMinEqGain)
                eq_setGainPeak(0, eq.gainPeak[0] - 1);
            break;

        case 5:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[1] > kMinEqFreq)
                    eq_setFreqPeak(1, eq.freqPeak[1] - 1);
            } else if (eq.gainPeak[1] > kMinEqGain)
                eq_setGainPeak(1, eq.gainPeak[1] - 1);
            break;

        case 6:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[2] > kMinEqFreq)
                    eq_setFreqPeak(2, eq.freqPeak[2] - 1);
            } else if (eq.gainPeak[2] > kMinEqGain)
                eq_setGainPeak(2, eq.gainPeak[2] - 1);
            break;

        case 7:

            // Shift key switches control from gain to frequency field.
            if (keyboard.shiftKeyPress) {
                if (eq.freqPeak[3] > kMinEqFreq)
                    eq_setFreqPeak(3, eq.freqPeak[3] - 1);
            } else if (eq.gainPeak[3] > kMinEqGain)
                eq_setGainPeak(3, eq.gainPeak[3] - 1);
            break;
    }
}

void Controller::eq_setActive(bool active_) {
    /// @brief Sets EQ active state using transition flow when runtime conditions allow.

    // 1. TRANSITION GUARD
    // Ignore requests while a previous EQ transition is still active.
    if ((!eq.genTransition.active)) {
        if (loadAction) {
            // 2. LOAD-TIME DIRECT APPLY
            // During load workflow, apply state immediately without transition.
            eq.active = active_;
        } else {
            // 3. RUNTIME TRANSITION APPLY
            // Start active/inactive crossfade transition for smooth output change.
            // if (active_) eq.cleanMemory();
            eq_genTransition(EQ_MODE_ACTIVE, eq.active, active_);
            eq.active = active_;

            // 4. UI & GLOBAL REFRESH
            // Refresh EQ tab data (if visible) and global summary field.
            if (menu == EQ_MENU) lcd_drawEq_ActiveData();
            lcd_updateGlobalMenuData(ENC_EQ_ACTIVE);
        }
    }
}

void Controller::eq_setQ(uint8_t q_) {
    /// @brief Sets EQ Q factor and recomputes all peak-band coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured Q limits.
    if ((q_ >= kMinEqQ) && (q_ <= kMaxEqQ)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store Q value and regenerate all peak filter coefficients.
        eq.q = q_;
        for (uint8_t i = 0; i < 4; i++) {
            eq.calculateCoefPeak(i);
        }

        // 3. UI REFRESH
        // Refresh EQ tab field when EQ menu is visible.
        if (menu == EQ_MENU) lcd_drawEq_QData();
    }
}

void Controller::eq_setFreqLowShelf(uint8_t freq_) {
    /// @brief Sets low-shelf frequency index and recalculates low-shelf coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured EQ frequency limits.
    if ((freq_ >= kMinEqFreq) && (freq_ <= kMaxEqFreq)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store frequency value and regenerate low-shelf coefficients.
        eq.freqLowShelf = freq_;
        eq.calculateCoefLowShelf();

        // 3. UI REFRESH
        // Refresh EQ tab field when EQ menu is visible.
        if (menu == EQ_MENU) lcd_drawEq_LowShelfData();
    }
}

void Controller::eq_setGainLowShelf(uint8_t gain_) {
    /// @brief Sets low-shelf gain index and recalculates low-shelf coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured EQ gain limits.
    if ((gain_ >= kMinEqGain) && (gain_ <= kMaxEqGain)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store gain value and regenerate low-shelf coefficients.
        eq.gainLowShelf = gain_;
        eq.calculateCoefLowShelf();

        // 3. UI & GLOBAL REFRESH
        // Refresh EQ tab field (if visible) and global summary field.
        if (menu == EQ_MENU) lcd_drawEq_LowShelfData();
        lcd_updateGlobalMenuData(ENC_EQ_LOWSHELF);
    }
}

void Controller::eq_setFreqHighShelf(uint8_t freq_) {
    /// @brief Sets high-shelf frequency index and recalculates high-shelf coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured EQ frequency limits.
    if ((freq_ >= kMinEqFreq) && (freq_ <= kMaxEqFreq)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store frequency value and regenerate high-shelf coefficients.
        eq.freqHighShelf = freq_;
        eq.calculateCoefHighShelf();

        // 3. UI REFRESH
        // Refresh EQ tab field when EQ menu is visible.
        if (menu == EQ_MENU) lcd_drawEq_HighShelfData();
    }
}

void Controller::eq_setGainHighShelf(uint8_t gain_) {
    /// @brief Sets high-shelf gain index and recalculates high-shelf coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured EQ gain limits.
    if ((gain_ >= kMinEqGain) && (gain_ <= kMaxEqGain)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store gain value and regenerate high-shelf coefficients.
        eq.gainHighShelf = gain_;
        eq.calculateCoefHighShelf();

        // 3. UI & GLOBAL REFRESH
        // Refresh EQ tab field (if visible) and global summary field.
        if (menu == EQ_MENU) lcd_drawEq_HighShelfData();
        lcd_updateGlobalMenuData(ENC_EQ_HIGHSHELF);
    }
}

void Controller::eq_setFreqPeak(uint8_t peakNum_, uint8_t freq_) {
    /// @brief Sets selected peak-band frequency index and recalculates that band's coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured EQ frequency limits.
    if ((freq_ >= kMinEqFreq) && (freq_ <= kMaxEqFreq)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store frequency value and regenerate selected peak coefficients.
        eq.freqPeak[peakNum_] = freq_;
        eq.calculateCoefPeak(peakNum_);

        // 3. UI REFRESH
        // Refresh peak-band field when EQ menu is visible.
        if (menu == EQ_MENU) lcd_drawEq_PeakData(peakNum_);
    }
}

void Controller::eq_setGainPeak(uint8_t peakNum_, uint8_t gain_) {
    /// @brief Sets selected peak-band gain index and recalculates that band's coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured EQ gain limits.
    if ((gain_ >= kMinEqGain) && (gain_ <= kMaxEqGain)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store gain value and regenerate selected peak coefficients.
        eq.gainPeak[peakNum_] = gain_;
        eq.calculateCoefPeak(peakNum_);

        // 3. UI & GLOBAL REFRESH
        // Refresh peak-band field (if visible) and global summary field.
        if (menu == EQ_MENU) lcd_drawEq_PeakData(peakNum_);
        lcd_updateGlobalMenuData((EncoderFunc)(ENC_EQ_PEAK0 + peakNum_));
    }
}

void Controller::eq_genTransition(EqTransitionMode mode_, bool activeActive_, bool targetActive_) {
    /// @brief Configures and starts EQ dry/wet transition for active-state changes.

    EqGenTransition& gTransition = eq.genTransition;

    // 1. TRANSITION CONTEXT INITIALIZATION
    // Store mode/phase/state parameters for transition engine.
    gTransition.mode = mode_;
    gTransition.phase = EQ_PHASE_A;
    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;

    // 2. MODE-SPECIFIC TARGET MAPPING
    // Map requested mode to dry/wet start/target pairs.
    switch (mode_) {
        case EQ_MODE_NONE:
            break;

        case EQ_MODE_ACTIVE:
            switch (targetActive_) {
                case true:
                    gTransition.activeDry = 1.0;
                    gTransition.targetDry = 0.0;
                    gTransition.activeWet = 0.0;
                    gTransition.targetWet = 1.0;
                    transitionShowFlag = 2;
                    checkMenuTransition(EQ_MENU, true);
                    checkEncoderTransition(ENC_EQ_ACTIVE, true);
                    break;

                case false:
                    gTransition.activeDry = 0.0;
                    gTransition.targetDry = 1.0;
                    gTransition.activeWet = 1.0;
                    gTransition.targetWet = 0.0;
                    transitionShowFlag = 1;
                    checkMenuTransition(EQ_MENU, true);
                    checkEncoderTransition(ENC_EQ_ACTIVE, true);
                    break;
            }
            break;
    }

    // 3. TRANSITION PRE-CALC & ARM
    // Calculate action directions and mark transition as active.
    eq_calculateActiveTransition();
    gTransition.active = true;
}

void Controller::eq_calculateActiveTransition() {
    /// @brief Determines dry/wet transition directions against EQ active targets.

    EqGenTransition& gTransition = eq.genTransition;

    // 1. DRY PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for dry path.
    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = EQ_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = EQ_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = EQ_ACTION_DOWN;
    }

    // 2. WET PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for wet path.
    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = EQ_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = EQ_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = EQ_ACTION_DOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Filter Functions ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::filter_select(uint8_t filterNum_) {
    /// @brief Switches to Filter menu (0/1) and prepares UI/encoder context for filter operations.

    // 1. MENU SWITCH CONDITION
    // Enter filter menu when not already in filter context or when switching filter index.
    if (((menu == FILTER_0_MENU) && (filterNum_ == 1)) || ((menu == FILTER_1_MENU) && (filterNum_ == 0)) || (menu != FILTER_0_MENU) && (menu != FILTER_1_MENU)) {
        // 2. MENU STATE TRANSITION
        // Store previous menu context and activate target filter menu.
        preMenu = menu;
        (filterNum_) ? menu = FILTER_1_MENU : menu = FILTER_0_MENU;
        preMenuTab = menuTab;
        menuTab = 0;

        // 3. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 4. UI TRANSITION & RENDER
        // Animate menu change and render selected filter screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawFilterMenu(filterNum_);

        // 5. ENCODER CONTEXT UPDATE
        // Rebind encoders to Filter menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::filter_reset(uint8_t filterNum_) {
    /// @brief Restores selected filter block parameters to default values.

    // 1. PARAMETER RESET
    // Re-apply default active/type/slope/frequency/resonance/drive/dry/wet values.
    filter_setActive(filterNum_, kInitialFilterActive);
    filter_setType(filterNum_, kInitialFilterType[filterNum_]);
    filter_setSlope(filterNum_, kInitialFilterSlope);
    filter_setFrequency(filterNum_, kInitialFilterFrequency);
    filter_setResonance(filterNum_, kInitialFilterResonance);
    filter_setDrive(filterNum_, kInitialFilterDrive);
    filter_setDry(filterNum_, kInitialFilterDry);
    filter_setWet(filterNum_, kInitialFilterWet);

    // 2. INTERNAL STATE CLEANUP
    // Flush filter memory and rebuild coefficients after reset.
    Filter& filter_ = filter[filterNum_];
    filter_.cleanMemory();
    filter_.calculateCoef();
}

void Controller::filter_menuRight() {
    /// @brief Advances Filter menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab < kMaxMenu8Tab) ? (menuTab + 1) : kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::filter_menuLeft() {
    /// @brief Moves Filter menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab > kMinMenu8Tab) ? (menuTab - 1) : kMaxMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::filter_menuUp() {
    /// @brief Increments or toggles parameter mapped to active Filter tab.

    // 1. ACTIVE FILTER RESOLUTION
    // Resolve runtime filter index from currently active filter menu.
    uint8_t filterNum;
    (menu == FILTER_0_MENU) ? filterNum = 0 : filterNum = 1;

    // 2. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific filter setting.
    switch (menuTab) {
        case 0:
            filter_setActive(filterNum, !filter[filterNum].active);
            break;

        case 1:
            (filter[filterNum].type < kMaxFilterType) ? filter_setType(filterNum, filter[filterNum].type + 1) : filter_setType(filterNum, kMinFilterType);
            break;

        case 2:
            (filter[filterNum].slope < kMaxFilterSlope) ? filter_setSlope(filterNum, filter[filterNum].slope + 1) : filter_setSlope(filterNum, kMinFilterSlope);
            break;

        case 3:
            if (filter[filterNum].frequency < kMaxFilterFrequency)
                filter_setFrequency(filterNum, filter[filterNum].frequency + 1);
            break;

        case 4:
            if (filter[filterNum].resonance < kMaxFilterResonance)
                filter_setResonance(filterNum, filter[filterNum].resonance + 1);
            break;

        case 5:
            if (filter[filterNum].drive < kMaxFilterDrive)
                filter_setDrive(filterNum, filter[filterNum].drive + 1);
            break;

        case 6:

            // Keep dry/wet within mix constraints by compensating opposite side.
            if (filter[filterNum].dry < kMaxFilterDry) {
                filter_setDry(filterNum, filter[filterNum].dry + 1);
            } else if ((filter[filterNum].dry == kMaxFilterDry) && (filter[filterNum].wet > kMinFilterWet)) {
                filter_setWet(filterNum, filter[filterNum].wet - 1);
            }
            break;

        case 7:

            // Keep dry/wet within mix constraints by compensating opposite side.
            if (filter[filterNum].wet < kMaxFilterWet) {
                filter_setWet(filterNum, filter[filterNum].wet + 1);
            } else if ((filter[filterNum].wet == kMaxFilterWet) && (filter[filterNum].dry > kMinFilterDry)) {
                filter_setDry(filterNum, filter[filterNum].dry - 1);
            }
            break;
    }
}

void Controller::filter_menuDown() {
    /// @brief Decrements or toggles parameter mapped to active Filter tab.

    // 1. ACTIVE FILTER RESOLUTION
    // Resolve runtime filter index from currently active filter menu.
    uint8_t filterNum;
    (menu == FILTER_0_MENU) ? filterNum = 0 : filterNum = 1;

    // 2. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific filter setting.
    switch (menuTab) {
        case 0:
            filter_setActive(filterNum, !filter[filterNum].active);
            break;

        case 1:
            (filter[filterNum].type > kMinFilterType) ? filter_setType(filterNum, filter[filterNum].type - 1) : filter_setType(filterNum, kMaxFilterType);
            break;

        case 2:
            (filter[filterNum].slope > kMinFilterSlope) ? filter_setSlope(filterNum, filter[filterNum].slope - 1) : filter_setSlope(filterNum, kMaxFilterSlope);
            break;

        case 3:
            if (filter[filterNum].frequency > kMinFilterFrequency)
                filter_setFrequency(filterNum, filter[filterNum].frequency - 1);
            break;

        case 4:
            if (filter[filterNum].resonance > kMinFilterResonance)
                filter_setResonance(filterNum, filter[filterNum].resonance - 1);
            break;

        case 5:
            if (filter[filterNum].drive > kMinFilterDrive)
                filter_setDrive(filterNum, filter[filterNum].drive - 1);
            break;

        case 6:
            if (filter[filterNum].dry > kMinFilterDry)
                filter_setDry(filterNum, filter[filterNum].dry - 1);
            break;

        case 7:
            if (filter[filterNum].wet > kMinFilterWet)
                filter_setWet(filterNum, filter[filterNum].wet - 1);
            break;
    }
}

void Controller::filter_setActive(uint8_t filterNum_, bool active_) {
    /// @brief Sets filter active state using transition flow when runtime conditions allow.

    Filter& filter_ = filter[filterNum_];

    // 1. TRANSITION GUARD
    // Ignore requests while a previous filter transition is still active.
    if (!filter[filterNum_].genTransition.active) {
        if (loadAction) {
            // 2. LOAD-TIME DIRECT APPLY
            // During load workflow, apply state immediately without transition.
            filter_.active = active_;
        } else {
            // 3. RUNTIME TRANSITION APPLY
            // Start active/inactive crossfade transition for smooth output change.
            // if (active_) filter_.cleanMemory();
            filter_genTransition(filterNum_, FIL_MODE_ACTIVE, filter_.active, active_, filter_.type, filter_.type);
            filter_.active = active_;

            // 4. UI & GLOBAL REFRESH
            // Refresh Filter tab data (if visible) and global summary field.
            if ((((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))))
                lcd_drawFilter_ActiveData(filterNum_);
            EncoderFunc function;
            (filterNum_ == 0) ? function = ENC_FILTER_0_ACTIVE : function = ENC_FILTER_1_ACTIVE;
            lcd_updateGlobalMenuData(function);
        }
    }
}

void Controller::filter_setType(uint8_t filterNum_, uint8_t type_) {
    /// @brief Sets filter type and applies transition path when filter is active.

    Filter& filter_ = filter[filterNum_];

    // 1. TRANSITION GUARD
    // Ignore requests while a previous filter transition is still active.
    if (!filter[filterNum_].genTransition.active) {
        if (loadAction) {
            // 2. LOAD-TIME DIRECT APPLY
            // During load workflow, apply type immediately without transition.
            filter_.type = type_;
            filter_.calculateCoef();
        } else {
            // 3. RUNTIME TYPE APPLY
            // Active filter uses type transition; inactive filter updates coefficients directly.
            if (filter_.active) {
                filter_genTransition(filterNum_, FIL_MODE_TYPE, filter_.active, filter_.active, filter_.type, type_);
                filter_.type = type_;
            } else {
                filter_.type = type_;
                filter_.calculateCoef();
            }

            // 4. UI & GLOBAL REFRESH
            // Refresh Filter tab data (if visible) and global summary field.
            if ((((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))))
                lcd_drawFilter_TypeData(filterNum_);
            EncoderFunc function;
            (filterNum_ == 0) ? function = ENC_FILTER_0_TYPE : function = ENC_FILTER_1_TYPE;
            lcd_updateGlobalMenuData(function);
        }
    }
}

void Controller::filter_setSlope(uint8_t filterNum_, uint8_t slope_) {
    /// @brief Sets filter slope index and recalculates filter coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured slope limits.
    if ((slope_ >= kMinFilterSlope) && (slope_ <= kMaxFilterSlope)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store slope and regenerate coefficients.
        filter[filterNum_].slope = slope_;
        filter[filterNum_].calculateCoef();

        // 3. UI & GLOBAL REFRESH
        // Refresh Filter tab data (if visible) and global summary field.
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1)))
            lcd_drawFilter_SlopeData(filterNum_);
        EncoderFunc function;
        (filterNum_ == 0) ? function = ENC_FILTER_0_RESON : function = ENC_FILTER_1_RESON;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::filter_setFrequency(uint8_t filterNum_, uint8_t freq_) {
    /// @brief Sets filter frequency index and recalculates filter coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured frequency limits.
    if ((freq_ >= kMinFilterFrequency) && (freq_ <= kMaxFilterFrequency)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store frequency and regenerate coefficients.
        filter[filterNum_].frequency = freq_;
        filter[filterNum_].calculateCoef();

        // 3. UI & GLOBAL REFRESH
        // Refresh Filter tab data (if visible) and global summary field.
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1)))
            lcd_drawFilter_FrequencyData(filterNum_);
        EncoderFunc function;
        (filterNum_ == 0) ? function = ENC_FILTER_0_FREQ : function = ENC_FILTER_1_FREQ;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::filter_setResonance(uint8_t filterNum_, uint8_t res_) {
    /// @brief Sets filter resonance index and recalculates filter coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured resonance limits.
    if ((res_ >= kMinFilterResonance) && (res_ <= kMaxFilterResonance)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store resonance and regenerate coefficients.
        filter[filterNum_].resonance = res_;
        filter[filterNum_].calculateCoef();

        // 3. UI & GLOBAL REFRESH
        // Refresh Filter tab data (if visible) and global summary field.
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1)))
            lcd_drawFilter_ResonanceData(filterNum_);
        EncoderFunc function;
        (filterNum_ == 0) ? function = ENC_FILTER_0_RESON : function = ENC_FILTER_1_RESON;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::filter_setDrive(uint8_t filterNum_, uint8_t drive_) {
    /// @brief Sets filter drive index and recalculates filter coefficients.

    // 1. INPUT VALIDATION
    // Accept only values inside configured drive limits.
    if ((drive_ >= kMinFilterResonance) && (drive_ <= kMaxFilterResonance)) {
        // 2. DATA & COEFFICIENT UPDATE
        // Store drive and regenerate coefficients.
        filter[filterNum_].drive = drive_;
        filter[filterNum_].calculateCoef();

        // 3. UI & GLOBAL REFRESH
        // Refresh Filter tab data (if visible) and global summary field.
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1)))
            lcd_drawFilter_DriveData(filterNum_);
        EncoderFunc function;
        (filterNum_ == 0) ? function = ENC_FILTER_0_DRIVE : function = ENC_FILTER_1_DRIVE;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::filter_setDry(uint8_t filterNum_, uint8_t dry_) {
    /// @brief Sets filter dry mix component with optional wet compensation and transition.

    Filter& filter_ = filter[filterNum_];

    // 1. INPUT VALIDATION & TRANSITION GUARD
    // Accept valid range only when mix transition engine is idle.
    if ((dry_ >= kMinFilterDry) && (dry_ <= kMaxFilterDry) && (!filter_.mixTransition.active)) {
        // 2. TARGET MIX RESOLUTION
        // Resolve dry/wet targets and enforce optional sum limit.
        uint8_t targetDry = dry_;
        uint8_t targetWet = filter_.wet;
        if ((filter_.limitMix) && ((targetDry + targetWet) > filter_.limitMixData)) {
            targetWet = filter_.limitMixData - targetDry;
        }
        filter_.dry = targetDry;
        filter_.wet = targetWet;
        float targetDryFloat = kNumberDataLibrary[filter_.dry].stdMultiplier;
        float targetWetFloat = kNumberDataLibrary[filter_.wet].stdMultiplier;

        // 3. MIX APPLY MODE
        // Active filter uses transition; inactive filter updates values directly.
        if (filter_.active) {
            filter_mixTransition(filterNum_, targetDryFloat, targetWetFloat);
        } else {
            filter_.dryFloat = kNumberDataLibrary[filter_.dry].stdMultiplier;
            filter_.wetFloat = kNumberDataLibrary[filter_.wet].stdMultiplier;
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh Filter tab data (if visible) and global summary fields.
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))) {
            lcd_drawFilter_DryData(filterNum_);
            lcd_drawFilter_WetData(filterNum_);
        }
        EncoderFunc functionA;
        EncoderFunc functionB;
        (filterNum_ == 0) ? functionA = ENC_FILTER_0_DRY : functionA = ENC_FILTER_1_DRY;
        (filterNum_ == 0) ? functionB = ENC_FILTER_0_WET : functionB = ENC_FILTER_1_WET;
        lcd_updateGlobalMenuData(functionA);
        lcd_updateGlobalMenuData(functionB);
    }
}

void Controller::filter_setWet(uint8_t filterNum_, uint8_t wet_) {
    /// @brief Sets filter wet mix component with optional dry compensation and transition.

    Filter& filter_ = filter[filterNum_];

    // 1. INPUT VALIDATION & TRANSITION GUARD
    // Accept valid range only when mix transition engine is idle.
    if ((wet_ >= kMinFilterWet) && (wet_ <= kMaxFilterWet) && (!filter_.mixTransition.active)) {
        // 2. TARGET MIX RESOLUTION
        // Resolve dry/wet targets and enforce optional sum limit.
        uint8_t targetDry = filter_.dry;
        uint8_t targetWet = wet_;
        if ((filter_.limitMix) && ((targetDry + targetWet) > filter_.limitMixData)) {
            targetDry = filter_.limitMixData - targetWet;
        }
        filter_.dry = targetDry;
        filter_.wet = targetWet;
        float targetDryFloat = kNumberDataLibrary[filter_.dry].stdMultiplier;
        float targetWetFloat = kNumberDataLibrary[filter_.wet].stdMultiplier;

        // 3. MIX APPLY MODE
        // Active filter uses transition; inactive filter updates values directly.
        if (filter_.active) {
            filter_mixTransition(filterNum_, targetDryFloat, targetWetFloat);
        } else {
            filter_.dryFloat = kNumberDataLibrary[filter_.dry].stdMultiplier;
            filter_.wetFloat = kNumberDataLibrary[filter_.wet].stdMultiplier;
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh Filter tab data (if visible) and global summary fields.
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))) {
            lcd_drawFilter_DryData(filterNum_);
            lcd_drawFilter_WetData(filterNum_);
        }
        EncoderFunc functionA;
        EncoderFunc functionB;
        (filterNum_ == 0) ? functionA = ENC_FILTER_0_DRY : functionA = ENC_FILTER_1_DRY;
        (filterNum_ == 0) ? functionB = ENC_FILTER_0_WET : functionB = ENC_FILTER_1_WET;
        lcd_updateGlobalMenuData(functionA);
        lcd_updateGlobalMenuData(functionB);
    }
}

void Controller::filter_genTransition(uint8_t filterNum_, FilterTransitionMode mode_, bool activeActive_, bool targetActive_, uint8_t activeType_, uint8_t targetType_) {
    /// @brief Configures and starts filter dry/wet transition for active/type changes.

    Filter& filter_ = filter[filterNum_];
    FilterGenTransition& gTransition = filter_.genTransition;

    // 1. TRANSITION CONTEXT INITIALIZATION
    // Store mode/phase/state/type parameters for transition engine.
    gTransition.mode = mode_;
    gTransition.phase = FIL_PHASE_A;
    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;
    gTransition.activeType = activeType_;
    gTransition.targetType = targetType_;

    // 2. MODE-SPECIFIC TARGET MAPPING
    // Map requested mode to dry/wet start/target pairs.
    switch (mode_) {
        case FIL_MODE_NONE:
            break;

        case FIL_MODE_ACTIVE:
            switch (targetActive_) {
                case true:
                    gTransition.activeDry = 1.0;
                    gTransition.targetDry = 0.0;
                    gTransition.activeWet = 0.0;
                    gTransition.targetWet = 1.0;
                    filter_.calculateCoef();
                    transitionShowFlag = 2;
                    (filterNum_ == 0) ? checkMenuTransition(FILTER_0_MENU, true) : checkMenuTransition(FILTER_1_MENU, true);
                    (filterNum_ == 0) ? checkEncoderTransition(ENC_FILTER_0_ACTIVE, true) : checkEncoderTransition(ENC_FILTER_1_ACTIVE, true);
                    break;

                case false:
                    gTransition.activeDry = 0.0;
                    gTransition.targetDry = 1.0;
                    gTransition.activeWet = 1.0;
                    gTransition.targetWet = 0.0;
                    transitionShowFlag = 1;
                    (filterNum_ == 0) ? checkMenuTransition(FILTER_0_MENU, true) : checkMenuTransition(FILTER_1_MENU, true);
                    (filterNum_ == 0) ? checkEncoderTransition(ENC_FILTER_0_ACTIVE, true) : checkEncoderTransition(ENC_FILTER_1_ACTIVE, true);
                    break;
            }
            break;

        case FIL_MODE_TYPE:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;
            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;
            transitionShowFlag = 1;
            (filterNum_ == 0) ? checkMenuTransition(FILTER_0_MENU, true) : checkMenuTransition(FILTER_1_MENU, true);
            (filterNum_ == 0) ? checkEncoderTransition(ENC_FILTER_0_TYPE, true) : checkEncoderTransition(ENC_FILTER_1_TYPE, true);
            break;
    }

    // 3. TRANSITION PRE-CALC & ARM
    // Calculate action directions and mark transition as active.
    filter_calculateGenTransition(filterNum_);
    gTransition.active = true;
}

void Controller::filter_mixTransition(uint8_t filterNum_, float dryFloat_, float wetFloat_) {
    /// @brief Queues filter dry/wet mix transition targets and activates transition engine.

    Filter& filter_ = filter[filterNum_];
    FilterMixTransition& mTransition = filter_.mixTransition;

    // 1. TARGET UPDATE
    // Store requested dry/wet target multipliers.
    mTransition.targetDry = dryFloat_;
    mTransition.targetWet = wetFloat_;

    // 2. TRANSITION PRE-CALC & ARM
    // Calculate direction/step and mark transition as active.
    filter_calculateMixTransition(filterNum_);
    mTransition.active = true;
}

void Controller::filter_calculateGenTransition(uint8_t filterNum_) {
    /// @brief Determines filter generator transition directions against active targets.

    Filter& filter_ = filter[filterNum_];
    FilterGenTransition& gTransition = filter_.genTransition;

    // 1. DRY PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for dry path.
    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = FIL_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = FIL_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = FIL_ACTION_DOWN;
    }

    // 2. WET PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for wet path.
    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = FIL_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = FIL_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = FIL_ACTION_DOWN;
    }
}

void Controller::filter_calculateMixTransition(uint8_t filterNum_) {
    /// @brief Determines filter mix transition directions against dry/wet float targets.

    Filter& filter_ = filter[filterNum_];
    FilterMixTransition& mTransition = filter_.mixTransition;

    // 1. DRY MIX DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for dry mix.
    if (filter_.dryFloat == mTransition.targetDry) {
        mTransition.actionDry = FIL_ACTION_NONE;
    } else if (filter_.dryFloat < mTransition.targetDry) {
        mTransition.actionDry = FIL_ACTION_UP;
    } else if (filter_.dryFloat > mTransition.targetDry) {
        mTransition.actionDry = FIL_ACTION_DOWN;
    }

    // 2. WET MIX DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for wet mix.
    if (filter_.wetFloat == mTransition.targetWet) {
        mTransition.actionWet = FIL_ACTION_NONE;
    } else if (filter_.wetFloat < mTransition.targetWet) {
        mTransition.actionWet = FIL_ACTION_UP;
    } else if (filter_.wetFloat > mTransition.targetWet) {
        mTransition.actionWet = FIL_ACTION_DOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Effect Functions ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::effect_select(uint8_t effectNum_) {
    /// @brief Switches to Effect menu (0/1) and prepares UI/encoder context for effect operations.

    // 1. MENU SWITCH CONDITION
    // Enter effect menu when not already in effect context or when switching effect index.
    if (((menu == EFFECT_0_MENU) && (effectNum_ == 1)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 0)) || (menu != EFFECT_0_MENU) && (menu != EFFECT_1_MENU)) {
        // 2. MENU STATE TRANSITION
        // Store previous menu context and activate target effect menu.
        preMenu = menu;
        (effectNum_) ? menu = EFFECT_1_MENU : menu = EFFECT_0_MENU;
        preMenuTab = menuTab;
        menuTab = 0;

        // 3. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 4. UI TRANSITION & RENDER
        // Animate menu change and render selected effect screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawEffectMenu(effectNum_);

        // 5. ENCODER CONTEXT UPDATE
        // Rebind encoders to Effect menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::effect_reset(uint8_t effectNum_) {
    /// @brief Restores selected effect block parameters to default values.

    // 1. PARAMETER RESET
    // Re-apply default active/type settings for selected effect.
    effect_setActive(effectNum_, kInitialEffectActive);
    effect_setType(effectNum_, kInitialEffectType[effectNum_]);

    // 2. INTERNAL STATE RESET
    // Reset effect runtime buffers/state machine internals.
    effect[effectNum_].reset();
}

void Controller::effect_menuRight() {
    /// @brief Advances Effect menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab < kMaxMenu8Tab) ? (menuTab + 1) : kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::effect_menuLeft() {
    /// @brief Moves Effect menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab > kMinMenu8Tab) ? (menuTab - 1) : kMaxMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::effect_menuUp() {
    /// @brief Increments or toggles parameter mapped to active Effect tab.

    // 1. ACTIVE EFFECT CONTEXT RESOLUTION
    // Resolve runtime effect index, active type, and selected sub-effect data block.
    const uint8_t effectNum_ = (menu == EFFECT_0_MENU) ? 0 : 1;
    Effect& effect_ = effect[effectNum_];
    const uint8_t type_ = effect_.type;
    SubEffect& subEffect_ = effect_.subEffect[type_];

    // 2. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific effect parameter.
    switch (menuTab) {
        case 0:
            effect_setActive(effectNum_, !effect_.active);
            break;

        case 1:
            (effect_.type < kMaxEffectType) ? effect_setType(effectNum_, effect_.type + 1) : effect_setType(effectNum_, kMinEffectType);
            break;

        case 2:
            if (subEffect_.aData < subEffect_.kMaxAData) effect_setAData(effectNum_, type_, subEffect_.aData + 1);
            break;

        case 3:
            if (subEffect_.bData < subEffect_.kMaxBData) effect_setBData(effectNum_, type_, subEffect_.bData + 1);
            break;

        case 4:
            if (subEffect_.cData < subEffect_.kMaxCData) effect_setCData(effectNum_, type_, subEffect_.cData + 1);
            break;

        case 5:
            if (subEffect_.dData < subEffect_.kMaxDData) effect_setDData(effectNum_, type_, subEffect_.dData + 1);
            break;

        case 6:

            // Keep E/F mix pair within effect-specific limits by compensating opposite side.
            if (subEffect_.eData < subEffect_.kMaxEData) {
                effect_setEData(effectNum_, type_, subEffect_.eData + 1);
            } else if ((subEffect_.eData == subEffect_.kMaxEData) && (subEffect_.fData > subEffect_.kMinFData)) {
                effect_setFData(effectNum_, type_, subEffect_.fData - 1);
            }
            break;

        case 7:

            // Keep E/F mix pair within effect-specific limits by compensating opposite side.
            if (subEffect_.fData < subEffect_.kMaxFData) {
                effect_setFData(effectNum_, type_, subEffect_.fData + 1);
            } else if ((subEffect_.fData == subEffect_.kMaxFData) && (subEffect_.eData > subEffect_.kMinEData)) {
                effect_setEData(effectNum_, type_, subEffect_.eData - 1);
            }
            break;
    }
}

void Controller::effect_menuDown() {
    /// @brief Decrements or toggles parameter mapped to active Effect tab.

    // 1. ACTIVE EFFECT CONTEXT RESOLUTION
    // Resolve runtime effect index, active type, and selected sub-effect data block.
    const uint8_t effectNum_ = (menu == EFFECT_0_MENU) ? 0 : 1;
    Effect& effect_ = effect[effectNum_];
    const uint8_t type_ = effect_.type;
    SubEffect& subEffect_ = effect_.subEffect[type_];

    // 2. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific effect parameter.
    switch (menuTab) {
        case 0:
            effect_setActive(effectNum_, !effect[effectNum_].active);
            break;

        case 1:
            (effect_.type > kMinEffectType) ? effect_setType(effectNum_, effect_.type - 1) : effect_setType(effectNum_, kMaxEffectType);
            break;

        case 2:
            if (subEffect_.aData > subEffect_.kMinAData) effect_setAData(effectNum_, type_, subEffect_.aData - 1);
            break;

        case 3:
            if (subEffect_.bData > subEffect_.kMinBData) effect_setBData(effectNum_, type_, subEffect_.bData - 1);
            break;

        case 4:
            if (subEffect_.cData > subEffect_.kMinCData) effect_setCData(effectNum_, type_, subEffect_.cData - 1);
            break;

        case 5:
            if (subEffect_.dData > subEffect_.kMinDData) effect_setDData(effectNum_, type_, subEffect_.dData - 1);
            break;

        case 6:
            if (subEffect_.eData > subEffect_.kMinEData) {
                effect_setEData(effectNum_, type_, subEffect_.eData - 1);
            } else if (subEffect_.fData < subEffect_.kMaxFData) {
                effect_setFData(effectNum_, type_, subEffect_.fData + 1);
            }
            break;

        case 7:
            if (subEffect_.fData > subEffect_.kMinFData) {
                effect_setFData(effectNum_, type_, subEffect_.fData - 1);
            } else if (subEffect_.eData < subEffect_.kMaxEData) {
                effect_setEData(effectNum_, type_, subEffect_.eData + 1);
            }
            break;
    }
}

void Controller::effect_setActive(uint8_t effectNum_, bool active_) {
    /// @brief Sets effect active state using transition flow when runtime conditions allow.

    Effect& effect_ = effect[effectNum_];

    // 1. TRANSITION GUARD
    // Ignore requests while a previous effect transition is still active.
    if (!effect_.genTransition.active) {
        if (loadAction) {
            // 2. LOAD-TIME DIRECT APPLY
            // During load workflow, apply state immediately without transition.
            effect_.active = active_;
        } else {
            // 3. RUNTIME TRANSITION APPLY
            // Start active/inactive crossfade transition for smooth output change.
            if (active_) effect_cleanMemory(effectNum_, effect_.type);
            effect_genTransition(effectNum_, EF_MODE_ACTIVE, effect_.active, active_, effect_.type, effect_.type);
            effect_.active = active_;

            // 4. UI & GLOBAL REFRESH
            // Refresh Effect tab data (if visible) and global summary field.
            if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))))
                lcd_drawEffect_ActiveData(effectNum_);
            EncoderFunc function;
            (effectNum_ == 0) ? function = ENC_EFFECT_0_ACTIVE : function = ENC_EFFECT_1_ACTIVE;
            lcd_updateGlobalMenuData(function);
        }
    }
}

void Controller::effect_setType(uint8_t effectNum_, uint8_t type_) {
    /// @brief Sets effect type and applies transition path when effect is active.

    Effect& effect_ = effect[effectNum_];

    // 1. TRANSITION GUARD
    // Ignore requests while a previous effect transition is still active.
    if (!effect_.genTransition.active) {
        if (loadAction) {
            // 2. LOAD-TIME DIRECT APPLY
            // During load workflow, apply type immediately without transition.
            effect_cleanMemory(effectNum_, type_);
            effect_.type = type_;
            effect_.compMode = kEffectTypeDataLibrary[type_].compMode;
            effect_updateMixFloats(effectNum_);
        } else {
            // 3. RUNTIME TYPE APPLY
            // Active effect uses type transition; inactive effect updates directly.
            if (effect_.active) {
                effect_cleanMemory(effectNum_, type_);
                effect_genTransition(effectNum_, EF_MODE_TYPE, effect_.active, effect_.active, effect_.type, type_);
                effect_.type = type_;
                effect_.compMode = kEffectTypeDataLibrary[type_].compMode;
            } else {
                effect_.type = type_;
                effect_.compMode = kEffectTypeDataLibrary[type_].compMode;
                effect_updateMixFloats(effectNum_);
            }

            // 4. UI REFRESH
            // Refresh all parameter fields since active sub-effect schema can change.
            if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1)))) {
                lcd_drawEffect_TypeData(effectNum_);
                lcd_drawEffect_AData(effectNum_);
                lcd_drawEffect_BData(effectNum_);
                lcd_drawEffect_CData(effectNum_);
                lcd_drawEffect_DData(effectNum_);
                lcd_drawEffect_EData(effectNum_);
                lcd_drawEffect_FData(effectNum_);
            }

            // Encoder/global rebind block intentionally kept disabled as in original.
            EncoderFunc function;
            (effectNum_ == 0) ? function = ENC_EFFECT_0_TYPE : function = ENC_EFFECT_1_TYPE;
            lcd_updateGlobalMenuData(function);
            switch (effectNum_) {
                case 0:
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_0_PARAM_A);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_0_PARAM_B);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_0_PARAM_C);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_0_PARAM_D);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_0_PARAM_E);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_0_PARAM_F);
                    lcd_updateGlobalMenuData(ENC_EFFECT_0_PARAM_A);
                    lcd_updateGlobalMenuData(ENC_EFFECT_0_PARAM_B);
                    lcd_updateGlobalMenuData(ENC_EFFECT_0_PARAM_C);
                    lcd_updateGlobalMenuData(ENC_EFFECT_0_PARAM_D);
                    lcd_updateGlobalMenuData(ENC_EFFECT_0_PARAM_E);
                    lcd_updateGlobalMenuData(ENC_EFFECT_0_PARAM_F);
                    break;

                case 1:
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_1_PARAM_A);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_1_PARAM_B);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_1_PARAM_C);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_1_PARAM_D);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_1_PARAM_E);
                    lcd_updateGlobalMenuFunction(ENC_EFFECT_1_PARAM_F);
                    lcd_updateGlobalMenuData(ENC_EFFECT_1_PARAM_A);
                    lcd_updateGlobalMenuData(ENC_EFFECT_1_PARAM_B);
                    lcd_updateGlobalMenuData(ENC_EFFECT_1_PARAM_C);
                    lcd_updateGlobalMenuData(ENC_EFFECT_1_PARAM_D);
                    lcd_updateGlobalMenuData(ENC_EFFECT_1_PARAM_E);
                    lcd_updateGlobalMenuData(ENC_EFFECT_1_PARAM_F);
                    break;
            }
        }
    }
}

void Controller::effect_setAData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t aData_) {
    /// @brief Sets A parameter for selected sub-effect and propagates DSP/UI updates.

    Effect& effect_ = effect[effectNum_];
    SubEffect& subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    // 1. INPUT VALIDATION
    // Accept only values inside sub-effect specific A-range limits.
    if ((aData_ >= subEffect_.kMinAData) && (aData_ <= subEffect_.kMaxAData)) {
        // 2. SUB-EFFECT SPECIFIC APPLY
        // Route A parameter to selected effect engine and apply transition/direct update rules.
        switch (subEffectNum_) {
            case EF_DELAY: {  // time
                Delay& delay = effect_.delay;
                if (!effect_.genTransition.active) {
                    if ((!loadAction) && (effect_.active)) {
                        effect_genTransition(effectNum_, EF_MODE_TIME, effect_.active, effect_.active, effect_.type, effect_.type);
                        subEffect_.aData = aData_;
                        delay.aTime = aData_;
                    } else {
                        subEffect_.aData = aData_;
                        delay.aTime = aData_;
                        delay.time = kDelayTimeDataLibrary[aData_].data;
                        delay.update(rhythm.tempo);
                    }
                }
            } break;

            case EF_CHORUS: {  // time
                Chorus& chorus = effect_.chorus;
                if (!effect_.genTransition.active) {
                    if ((!loadAction) && (effect_.active)) {
                        effect_genTransition(effectNum_, EF_MODE_TIME, effect_.active, effect_.active, effect_.type, effect_.type);
                        subEffect_.aData = aData_;
                        chorus.aTime = aData_;
                    } else {
                        subEffect_.aData = aData_;
                        chorus.aTime = aData_;
                        chorus.time = kChorusTimeDataLibrary[aData_].data;
                        chorus.update();
                    }
                }
            } break;

            case EF_FLANGER: {  // time
                Flanger& flanger = effect_.flanger;
                if (!effect_.genTransition.active) {
                    if ((!loadAction) && (effect_.active)) {
                        effect_genTransition(effectNum_, EF_MODE_TIME, effect_.active, effect_.active, effect_.type, effect_.type);
                        subEffect_.aData = aData_;
                        flanger.aTime = aData_;
                    } else {
                        subEffect_.aData = aData_;
                        flanger.aTime = aData_;
                        flanger.time = kFlangerTimeDataLibrary[aData_].data;
                        flanger.update();
                    }
                }
            } break;

            case EF_PHASER: {  // startFreq
                Phaser& phaser = effect_.phaser;
                if (aData_ <= phaser.bEndFreq) {
                    subEffect_.aData = aData_;
                    phaser.aStartFreq = aData_;
                    phaser.startFreq = kPhaserFreqDataLibrary[aData_].data;
                    phaser.update();
                }
            } break;

            case EF_COMPRESSOR: {  // threshold
                Compressor& compressor = effect_.compressor;
                subEffect_.aData = aData_;
                compressor.aThreshold = aData_;
                compressor.threshold = kCompressorThresholdDataLibrary[aData_].data;
                compressor.update();
            } break;

            case EF_EXPANDER: {  // threshold
                Expander& expander = effect_.expander;
                subEffect_.aData = aData_;
                expander.aThreshold = aData_;
                expander.threshold = kExpanderThresholdDataLibrary[aData_].data;
                expander.update();
            } break;

            case EF_OVERDRIVE: {  // gain
                Overdrive& overdrive = effect_.overdrive;
                subEffect_.aData = aData_;
                overdrive.aGain = aData_;
                overdrive.gaindB = kOverdriveGainDataLibrary[aData_].data;
                overdrive.update();
            } break;

            case EF_DISTORTION: {  // gain
                Distortion& distortion = effect_.distortion;
                subEffect_.aData = aData_;
                distortion.aGain = aData_;
                distortion.gaindB = kDistortionGainDataLibrary[aData_].data;
                distortion.update();
            } break;

            case EF_BITCRUSHER: {  // resolution
                Bitcrusher& bitcrusher = effect_.bitcrusher;
                subEffect_.aData = aData_;
                bitcrusher.aResolution = aData_;
                bitcrusher.resolution = kBitcrusherResolutionDataLibrary[aData_].data;
                bitcrusher.update();
            } break;
        }

        // 3. UI & GLOBAL REFRESH
        // Refresh local parameter field (if visible) and global summary field.
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_AData(effectNum_);
        }
        const EncoderFunc function = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_A : ENC_EFFECT_1_PARAM_A;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::effect_setBData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t bData_) {
    /// @brief Sets B parameter for selected sub-effect and propagates DSP/UI updates.

    Effect& effect_ = effect[effectNum_];
    SubEffect& subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    // 1. INPUT VALIDATION
    // Accept only values inside sub-effect specific B-range limits.
    if ((bData_ >= subEffect_.kMinBData) && (bData_ <= subEffect_.kMaxBData)) {
        // 2. SUB-EFFECT SPECIFIC APPLY
        // Route B parameter to selected effect engine and apply transition/direct update rules.
        switch (subEffectNum_) {
            case EF_DELAY: {  // level
                Delay& delay = effect_.delay;
                subEffect_.bData = bData_;
                delay.bLevel = bData_;
                delay.level = kNumberDataLibrary[bData_].stdMultiplier;
            } break;

            case EF_CHORUS: {  // feedback
                Chorus& chorus = effect_.chorus;
                if (!effect_.genTransition.active) {
                    if ((!loadAction) && (effect_.active)) {
                        effect_genTransition(effectNum_, EF_MODE_FEEDBACK, effect_.active, effect_.active, effect_.type, effect_.type);
                        subEffect_.bData = bData_;
                        chorus.bFeedback = bData_;
                    } else {
                        subEffect_.bData = bData_;
                        chorus.bFeedback = bData_;
                        chorus.feedback = kChorusFeedbackDataLibrary[bData_].data;
                        chorus.update();
                    }
                }
            } break;

            case EF_FLANGER: {  // feedback
                Flanger& flanger = effect_.flanger;
                if (!effect_.genTransition.active) {
                    if ((!loadAction) && (effect_.active)) {
                        effect_genTransition(effectNum_, EF_MODE_FEEDBACK, effect_.active, effect_.active, effect_.type, effect_.type);
                        subEffect_.bData = bData_;
                        flanger.bFeedback = bData_;
                    } else {
                        subEffect_.bData = bData_;
                        flanger.bFeedback = bData_;
                        flanger.feedback = kFlangerFeedbackDataLibrary[bData_].data;
                        flanger.update();
                    }
                }
            } break;

            case EF_PHASER: {  // endFreq
                Phaser& phaser = effect_.phaser;
                if (bData_ >= phaser.aStartFreq) {
                    subEffect_.bData = bData_;
                    phaser.bEndFreq = bData_;
                    phaser.endFreq = kPhaserFreqDataLibrary[bData_].data;
                    phaser.update();
                }
            } break;

            case EF_COMPRESSOR: {  // rate
                Compressor& compressor = effect_.compressor;
                subEffect_.bData = bData_;
                compressor.bRate = bData_;
                compressor.rate = kCompressorRateDataLibrary[bData_].data;
                compressor.update();
            } break;

            case EF_EXPANDER: {  // rate
                Expander& expander = effect_.expander;
                subEffect_.bData = bData_;
                expander.bRate = bData_;
                expander.rate = kExpanderRateDataLibrary[bData_].data;
                expander.update();
            } break;

            case EF_OVERDRIVE: {  // threshold
                Overdrive& overdrive = effect_.overdrive;
                subEffect_.bData = bData_;
                overdrive.bThreshold = bData_;
                overdrive.thresholddB = kOverdriveThresholdDataLibrary[bData_].data;
                overdrive.update();
            } break;

            case EF_DISTORTION: {  // threshold
                Distortion& distortion = effect_.distortion;
                subEffect_.bData = bData_;
                distortion.bThreshold = bData_;
                distortion.thresholddB = kDistortionThresholdDataLibrary[bData_].data;
                distortion.update();
            } break;

            case EF_BITCRUSHER: {  // sampleRate
                Bitcrusher& bitcrusher = effect_.bitcrusher;
                subEffect_.bData = bData_;
                bitcrusher.bSampleRate = bData_;
                bitcrusher.sampleRate = kBitcrusherSampleRateDataLibrary[bData_].data;
                bitcrusher.update();
            } break;
        }

        // 3. UI & GLOBAL REFRESH
        // Refresh local parameter field (if visible) and global summary field.
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_BData(effectNum_);
        }
        const EncoderFunc function = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_B : ENC_EFFECT_1_PARAM_B;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::effect_setCData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t cData_) {
    /// @brief Sets C parameter for selected sub-effect and propagates DSP/UI updates.

    Effect& effect_ = effect[effectNum_];
    SubEffect& subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    // 1. INPUT VALIDATION
    // Accept only values inside sub-effect specific C-range limits.
    if ((cData_ >= subEffect_.kMinCData) && (cData_ <= subEffect_.kMaxCData)) {
        // 2. SUB-EFFECT SPECIFIC APPLY
        // Route C parameter to selected effect engine and update coefficients/state.
        switch (subEffectNum_) {
            case EF_DELAY: {  // feedback
                Delay& delay = effect_.delay;
                subEffect_.cData = cData_;
                delay.cFeedback = cData_;
                delay.feedback = kNumberDataLibrary[cData_].stdMultiplier;
            } break;

            case EF_CHORUS: {  // rate
                Chorus& chorus = effect_.chorus;
                subEffect_.cData = cData_;
                chorus.cRate = cData_;
                chorus.rate = kChorusRateDataLibrary[cData_].data;
                chorus.update();
            } break;

            case EF_FLANGER: {  // rate
                Flanger& flanger = effect_.flanger;
                subEffect_.cData = cData_;
                flanger.cRate = cData_;
                flanger.rate = kFlangerRateDataLibrary[cData_].data;
                flanger.update();
            } break;

            case EF_PHASER: {  // rate
                Phaser& phaser = effect_.phaser;
                subEffect_.cData = cData_;
                phaser.cRate = cData_;
                phaser.rate = kPhaserRateDataLibrary[cData_].data;
                phaser.update();
            } break;

            case EF_COMPRESSOR: {  // time
                Compressor& compressor = effect_.compressor;
                subEffect_.cData = cData_;
                compressor.cAttackTime = cData_;
                compressor.attackTime = kCompressorAttackTimeDataLibrary[cData_].data;
                compressor.update();
            } break;

            case EF_EXPANDER: {  // time
                Expander& expander = effect_.expander;
                subEffect_.cData = cData_;
                expander.cAttackTime = cData_;
                expander.attackTime = kExpanderAttackTimeDataLibrary[cData_].data;
                expander.update();
            } break;

            case EF_OVERDRIVE: {  // tone
                Overdrive& overdrive = effect_.overdrive;
                subEffect_.cData = cData_;
                overdrive.cTone = cData_;
                overdrive.tone = kOverdriveToneDataLibrary[cData_].data;
                overdrive.update();
            } break;

            case EF_DISTORTION: {  // tone
                Distortion& distortion = effect_.distortion;
                subEffect_.cData = cData_;
                distortion.cTone = cData_;
                distortion.tone = kDistortionToneDataLibrary[cData_].data;
                distortion.update();
            } break;

            case EF_BITCRUSHER: {  // clipThreshold
                Bitcrusher& bitcrusher = effect_.bitcrusher;
                subEffect_.cData = cData_;
                bitcrusher.cThreshold = cData_;
                bitcrusher.threshold = kBitcrusherThresholdDataLibrary[cData_].data;
                bitcrusher.update();
            } break;
        }

        // 3. UI & GLOBAL REFRESH
        // Refresh local parameter field (if visible) and global summary field.
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_CData(effectNum_);
        }
        const EncoderFunc function = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_C : ENC_EFFECT_1_PARAM_C;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::effect_setDData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t dData_) {
    /// @brief Sets D parameter for selected sub-effect and propagates DSP/UI updates.

    Effect& effect_ = effect[effectNum_];
    SubEffect& subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    // 1. INPUT VALIDATION
    // Check against sub-effect specific D-range limits (usually 0-100).
    if ((dData_ >= subEffect_.kMinDData) && (dData_ <= subEffect_.kMaxDData)) {
        // 2. SUB-EFFECT SPECIFIC APPLY
        switch (subEffectNum_) {
            case EF_DELAY: {  // damping
                Delay& delay = effect_.delay;
                subEffect_.dData = dData_;
                delay.dDamping = dData_;
                delay.update(rhythm.tempo);
            } break;

            case EF_CHORUS: {  // depth
                Chorus& chorus = effect_.chorus;
                subEffect_.dData = dData_;
                chorus.dDepth = dData_;
                chorus.update();
            } break;

            case EF_FLANGER: {  // width
                Flanger& flanger = effect_.flanger;
                subEffect_.dData = dData_;
                flanger.dWidth = dData_;
                flanger.update();
            } break;

            case EF_PHASER: {  // feedback
                Phaser& phaser = effect_.phaser;
                subEffect_.dData = dData_;
                phaser.dFeedback = dData_;
                phaser.update();
            } break;

            case EF_COMPRESSOR: {  // releaseTime
                Compressor& compressor = effect_.compressor;
                subEffect_.dData = dData_;
                compressor.dReleaseTime = dData_;
                compressor.releaseTime = kCompressorReleaseTimeDataLibrary[dData_].data;
                compressor.update();
            } break;

            case EF_EXPANDER: {  // releaseTime
                Expander& expander = effect_.expander;
                subEffect_.dData = dData_;
                expander.dReleaseTime = dData_;
                expander.releaseTime = kExpanderReleaseTimeDataLibrary[dData_].data;
                expander.update();
            } break;

            case EF_OVERDRIVE: {  // driveColor
                Overdrive& overdrive = effect_.overdrive;
                subEffect_.dData = dData_;
                overdrive.dDriveColor = dData_;
                overdrive.update();
            } break;

            case EF_DISTORTION: {  // asymmetry
                Distortion& distortion = effect_.distortion;
                subEffect_.dData = dData_;
                distortion.dAsym = dData_;
                distortion.update();
            } break;

            case EF_BITCRUSHER: {  // smoothing
                Bitcrusher& bitcrusher = effect_.bitcrusher;
                subEffect_.dData = dData_;
                bitcrusher.dSmooth = dData_;
                bitcrusher.update();
            } break;
        }

        // 3. UI & GLOBAL REFRESH
        // Refresh local parameter field and global bottom bar.
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_DData(effectNum_);
        }

        const EncoderFunc function = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_D : ENC_EFFECT_1_PARAM_D;
        lcd_updateGlobalMenuData(function);
    }
}

void Controller::effect_setEData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t eData_) {
    /// @brief Sets E mix parameter and rebalances F when mix-limit constraints require it.

    Effect& effect_ = effect[effectNum_];
    SubEffect& subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    // 1. INPUT VALIDATION & TRANSITION GUARD
    // Accept valid range only when mix transition engine is idle.
    if ((eData_ >= subEffect_.kMinEData) && (eData_ <= subEffect_.kMaxEData) && (!effect_.mixTransition.active)) {
        // 2. TARGET MIX RESOLUTION
        // Resolve dry/wet targets per sub-effect and enforce optional sum limit.
        uint8_t targetDry;
        uint8_t targetWet;
        float targetDryFloat;
        float targetWetFloat;
        switch (subEffectNum_) {
            case EF_DELAY: {
                Delay& delay = effect_.delay;
                targetDry = eData_;
                targetWet = delay.fWet;
                if ((delay.limitMix) && ((targetDry + targetWet) > delay.limitMixData)) {
                    targetWet = delay.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                delay.eDry = targetDry;
                delay.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[delay.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[delay.fWet].stdMultiplier;
            } break;

            case EF_CHORUS: {
                Chorus& chorus = effect_.chorus;
                targetDry = eData_;
                targetWet = chorus.fWet;
                if ((chorus.limitMix) && ((targetDry + targetWet) > chorus.limitMixData)) {
                    targetWet = chorus.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                chorus.eDry = targetDry;
                chorus.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[chorus.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[chorus.fWet].stdMultiplier;
            } break;

            case EF_FLANGER: {
                Flanger& flanger = effect_.flanger;
                targetDry = eData_;
                targetWet = flanger.fWet;
                if ((flanger.limitMix) && ((targetDry + targetWet) > flanger.limitMixData)) {
                    targetWet = flanger.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                flanger.eDry = targetDry;
                flanger.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[flanger.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[flanger.fWet].stdMultiplier;
            } break;

            case EF_PHASER: {
                Phaser& phaser = effect_.phaser;
                targetDry = eData_;
                targetWet = phaser.fWet;
                if ((phaser.limitMix) && ((targetDry + targetWet) > phaser.limitMixData)) {
                    targetWet = phaser.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                phaser.eDry = targetDry;
                phaser.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[phaser.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[phaser.fWet].stdMultiplier;
            } break;

            case EF_COMPRESSOR: {
                Compressor& compressor = effect_.compressor;
                targetDry = eData_;
                targetWet = compressor.fWet;
                if ((compressor.limitMix) && ((targetDry + targetWet) > compressor.limitMixData)) {
                    targetWet = compressor.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                compressor.eDry = targetDry;
                compressor.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[compressor.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[compressor.fWet].stdMultiplier;
            } break;

            case EF_EXPANDER: {
                Expander& expander = effect_.expander;
                targetDry = eData_;
                targetWet = expander.fWet;
                if ((expander.limitMix) && ((targetDry + targetWet) > expander.limitMixData)) {
                    targetWet = expander.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                expander.eDry = targetDry;
                expander.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[expander.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[expander.fWet].stdMultiplier;
            } break;

            case EF_OVERDRIVE: {
                Overdrive& overdrive = effect_.overdrive;
                targetDry = eData_;
                targetWet = overdrive.fWet;
                if ((overdrive.limitMix) && ((targetDry + targetWet) > overdrive.limitMixData)) {
                    targetWet = overdrive.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                overdrive.eDry = targetDry;
                overdrive.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[overdrive.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[overdrive.fWet].stdMultiplier;
            } break;

            case EF_DISTORTION: {
                Distortion& distortion = effect_.distortion;
                targetDry = eData_;
                targetWet = distortion.fWet;
                if ((distortion.limitMix) && ((targetDry + targetWet) > distortion.limitMixData)) {
                    targetWet = distortion.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                distortion.eDry = targetDry;
                distortion.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[distortion.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[distortion.fWet].stdMultiplier;
            } break;

            case EF_BITCRUSHER: {
                Bitcrusher& bitcrusher = effect_.bitcrusher;
                targetDry = eData_;
                targetWet = bitcrusher.fWet;
                if ((bitcrusher.limitMix) && ((targetDry + targetWet) > bitcrusher.limitMixData)) {
                    targetWet = bitcrusher.limitMixData - targetDry;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                bitcrusher.eDry = targetDry;
                bitcrusher.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[bitcrusher.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[bitcrusher.fWet].stdMultiplier;
            } break;
        }

        // 3. MIX APPLY MODE
        // Active effect uses transition; inactive effect updates values directly.
        if (effect_.active) {
            effect_mixTransition(effectNum_, targetDryFloat, targetWetFloat);
        } else {
            effect_.dryFloat = targetDryFloat;
            effect_.wetFloat = targetWetFloat;
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh local E/F fields (if visible) and global summary fields.
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_EData(effectNum_);
            lcd_drawEffect_FData(effectNum_);
        }
        const EncoderFunc functionA = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_E : ENC_EFFECT_1_PARAM_E;
        const EncoderFunc functionB = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_F : ENC_EFFECT_1_PARAM_F;
        lcd_updateGlobalMenuData(functionA);
        lcd_updateGlobalMenuData(functionB);
    }
}

void Controller::effect_setFData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t fData_) {
    /// @brief Sets F mix parameter and rebalances E when mix-limit constraints require it.

    Effect& effect_ = effect[effectNum_];
    SubEffect& subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    // 1. INPUT VALIDATION & TRANSITION GUARD
    // Accept valid range only when mix transition engine is idle.
    if ((fData_ >= subEffect_.kMinEData) && (fData_ <= subEffect_.kMaxEData) && (!effect_.mixTransition.active)) {
        // 2. TARGET MIX RESOLUTION
        // Resolve dry/wet targets per sub-effect and enforce optional sum limit.
        uint8_t targetDry;
        uint8_t targetWet;
        float targetDryFloat;
        float targetWetFloat;
        switch (subEffectNum_) {
            case EF_DELAY: {
                Delay& delay = effect_.delay;
                targetDry = delay.eDry;
                targetWet = fData_;
                if ((delay.limitMix) && ((targetDry + targetWet) > delay.limitMixData)) {
                    targetDry = delay.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                delay.eDry = targetDry;
                delay.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[delay.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[delay.fWet].stdMultiplier;
            } break;

            case EF_CHORUS: {
                Chorus& chorus = effect_.chorus;
                targetDry = chorus.eDry;
                targetWet = fData_;
                if ((chorus.limitMix) && ((targetDry + targetWet) > chorus.limitMixData)) {
                    targetDry = chorus.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                chorus.eDry = targetDry;
                chorus.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[chorus.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[chorus.fWet].stdMultiplier;
            } break;

            case EF_FLANGER: {
                Flanger& flanger = effect_.flanger;
                targetDry = flanger.eDry;
                targetWet = fData_;
                if ((flanger.limitMix) && ((targetDry + targetWet) > flanger.limitMixData)) {
                    targetDry = flanger.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                flanger.eDry = targetDry;
                flanger.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[flanger.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[flanger.fWet].stdMultiplier;
            } break;

            case EF_PHASER: {
                Phaser& phaser = effect_.phaser;
                targetDry = phaser.eDry;
                targetWet = fData_;
                if ((phaser.limitMix) && ((targetDry + targetWet) > phaser.limitMixData)) {
                    targetDry = phaser.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                phaser.eDry = targetDry;
                phaser.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[phaser.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[phaser.fWet].stdMultiplier;
            } break;

            case EF_COMPRESSOR: {
                Compressor& compressor = effect_.compressor;
                targetDry = compressor.eDry;
                targetWet = fData_;
                if ((compressor.limitMix) && ((targetDry + targetWet) > compressor.limitMixData)) {
                    targetDry = compressor.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                compressor.eDry = targetDry;
                compressor.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[compressor.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[compressor.fWet].stdMultiplier;
            } break;

            case EF_EXPANDER: {
                Expander& expander = effect_.expander;
                targetDry = expander.eDry;
                targetWet = fData_;
                if ((expander.limitMix) && ((targetDry + targetWet) > expander.limitMixData)) {
                    targetDry = expander.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                expander.eDry = targetDry;
                expander.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[expander.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[expander.fWet].stdMultiplier;
            } break;

            case EF_OVERDRIVE: {
                Overdrive& overdrive = effect_.overdrive;
                targetDry = overdrive.eDry;
                targetWet = fData_;
                if ((overdrive.limitMix) && ((targetDry + targetWet) > overdrive.limitMixData)) {
                    targetDry = overdrive.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                overdrive.eDry = targetDry;
                overdrive.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[overdrive.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[overdrive.fWet].stdMultiplier;
            } break;

            case EF_DISTORTION: {
                Distortion& distortion = effect_.distortion;
                targetDry = distortion.eDry;
                targetWet = fData_;
                if ((distortion.limitMix) && ((targetDry + targetWet) > distortion.limitMixData)) {
                    targetDry = distortion.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                distortion.eDry = targetDry;
                distortion.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[distortion.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[distortion.fWet].stdMultiplier;
            } break;

            case EF_BITCRUSHER: {
                Bitcrusher& bitcrusher = effect_.bitcrusher;
                targetDry = bitcrusher.eDry;
                targetWet = fData_;
                if ((bitcrusher.limitMix) && ((targetDry + targetWet) > bitcrusher.limitMixData)) {
                    targetDry = bitcrusher.limitMixData - targetWet;
                }
                subEffect_.eData = targetDry;
                subEffect_.fData = targetWet;
                bitcrusher.eDry = targetDry;
                bitcrusher.fWet = targetWet;
                targetDryFloat = kNumberDataLibrary[bitcrusher.eDry].stdMultiplier;
                targetWetFloat = kNumberDataLibrary[bitcrusher.fWet].stdMultiplier;
            } break;
        }

        // 3. MIX APPLY MODE
        // Active effect uses transition; inactive effect updates values directly.
        if (effect_.active) {
            effect_mixTransition(effectNum_, targetDryFloat, targetWetFloat);
        } else {
            effect_.dryFloat = targetDryFloat;
            effect_.wetFloat = targetWetFloat;
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh local E/F fields (if visible) and global summary fields.
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_EData(effectNum_);
            lcd_drawEffect_FData(effectNum_);
        }
        const EncoderFunc functionA = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_E : ENC_EFFECT_1_PARAM_E;
        const EncoderFunc functionB = (effectNum_ == 0) ? ENC_EFFECT_0_PARAM_F : ENC_EFFECT_1_PARAM_F;
        lcd_updateGlobalMenuData(functionA);
        lcd_updateGlobalMenuData(functionB);
    }
}

void Controller::effect_genTransition(uint8_t effectNum_, EffectTransitionMode mode_, bool activeActive_, bool targetActive_, uint8_t activeType_, uint8_t targetType_) {
    /// @brief Configures and starts effect dry/wet transition for active/type/time/feedback changes.

    Effect& effect_ = effect[effectNum_];
    EffectGenTransition& gTransition = effect_.genTransition;

    // 1. TRANSITION CONTEXT INITIALIZATION
    // Store mode/phase/state/type parameters for transition engine.
    gTransition.mode = mode_;
    gTransition.phase = EF_PHASE_A;
    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;
    gTransition.activeType = activeType_;
    gTransition.targetType = targetType_;
    Menu targetMenu;
    EncoderFunc targetFunc;

    // 2. MODE-SPECIFIC TARGET MAPPING
    // Map requested mode to dry/wet (and record-wet where applicable) start/target pairs.
    switch (mode_) {
        case EF_MODE_NONE:
            break;

        case EF_MODE_ACTIVE:
            switch (targetActive_) {
                case true:
                    gTransition.activeDry = 1.0;
                    gTransition.targetDry = 0.0;
                    gTransition.activeWet = 0.0;
                    gTransition.targetWet = 1.0;
                    if ((activeType_ == EF_DELAY) || (activeType_ == EF_CHORUS) || (activeType_ == EF_FLANGER)) {
                        gTransition.activeRecordWet = 0.0;
                        gTransition.targetRecordWet = 1.0;
                    }
                    transitionShowFlag = 2;
                    (effectNum_ == 0) ? checkMenuTransition(EFFECT_0_MENU, true) : checkMenuTransition(EFFECT_1_MENU, true);
                    (effectNum_ == 0) ? checkEncoderTransition(ENC_EFFECT_0_ACTIVE, true) : checkEncoderTransition(ENC_EFFECT_1_ACTIVE, true);
                    break;

                case false:
                    gTransition.activeDry = 0.0;
                    gTransition.targetDry = 1.0;
                    gTransition.activeWet = 1.0;
                    gTransition.targetWet = 0.0;
                    if ((activeType_ == EF_DELAY) || (activeType_ == EF_CHORUS) || (activeType_ == EF_FLANGER)) {
                        gTransition.activeRecordWet = 1.0;
                        gTransition.targetRecordWet = 0.0;
                    }
                    transitionShowFlag = 1;
                    (effectNum_ == 0) ? checkMenuTransition(EFFECT_0_MENU, true) : checkMenuTransition(EFFECT_1_MENU, true);
                    (effectNum_ == 0) ? checkEncoderTransition(ENC_EFFECT_0_ACTIVE, true) : checkEncoderTransition(ENC_EFFECT_1_ACTIVE, true);
                    break;
            }
            break;

        case EF_MODE_TYPE:
        case EF_MODE_TIME:
        case EF_MODE_FEEDBACK:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;
            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;
            if ((activeType_ == EF_DELAY) || (activeType_ == EF_CHORUS) || (activeType_ == EF_FLANGER)) {
                gTransition.activeRecordWet = 1.0;
                gTransition.targetRecordWet = 0.0;
            }
            transitionShowFlag = 1;
            if (effectNum_ == 0) {
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
    }

    // 3. TRANSITION PRE-CALC & ARM
    // Calculate action directions and mark transition as active.
    effect_calculateGenTransition(effectNum_);
    gTransition.active = true;
}

void Controller::effect_mixTransition(uint8_t effectNum_, float dryFloat_, float wetFloat_) {
    /// @brief Queues effect dry/wet mix transition targets and activates transition engine.

    Effect& effect_ = effect[effectNum_];
    EffectMixTransition& mTransition = effect_.mixTransition;

    // 1. TARGET UPDATE
    // Store requested dry/wet target multipliers.
    mTransition.targetDry = dryFloat_;
    mTransition.targetWet = wetFloat_;

    // 2. TRANSITION PRE-CALC & ARM
    // Calculate direction/step and mark transition as active.
    effect_calculateMixTransition(effectNum_);
    mTransition.active = true;
}

void Controller::effect_calculateGenTransition(uint8_t effectNum_) {
    /// @brief Determines effect generator transition directions against active targets.

    Effect& effect_ = effect[effectNum_];
    EffectGenTransition& gTransition = effect_.genTransition;

    // 1. DRY PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for dry path.
    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = EF_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = EF_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = EF_ACTION_DOWN;
    }

    // 2. WET PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for wet path.
    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = EF_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = EF_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = EF_ACTION_DOWN;
    }

    // 3. RECORD-WET PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for record-wet path.
    if (gTransition.activeRecordWet == gTransition.targetRecordWet) {
        gTransition.actionRecordWet = EF_ACTION_NONE;
    } else if (gTransition.activeRecordWet < gTransition.targetRecordWet) {
        gTransition.actionRecordWet = EF_ACTION_UP;
    } else if (gTransition.activeRecordWet > gTransition.targetRecordWet) {
        gTransition.actionRecordWet = EF_ACTION_DOWN;
    }
}

void Controller::effect_calculateMixTransition(uint8_t effectNum_) {
    /// @brief Determines effect mix transition directions against dry/wet float targets.

    Effect& effect_ = effect[effectNum_];
    EffectMixTransition& mTransition = effect_.mixTransition;

    // 1. DRY MIX DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for dry mix.
    if (effect_.dryFloat == mTransition.targetDry) {
        mTransition.actionDry = EF_ACTION_NONE;
    } else if (effect_.dryFloat < mTransition.targetDry) {
        mTransition.actionDry = EF_ACTION_UP;
    } else if (effect_.dryFloat > mTransition.targetDry) {
        mTransition.actionDry = EF_ACTION_DOWN;
    }

    // 2. WET MIX DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for wet mix.
    if (effect_.wetFloat == mTransition.targetWet) {
        mTransition.actionWet = EF_ACTION_NONE;
    } else if (effect_.wetFloat < mTransition.targetWet) {
        mTransition.actionWet = EF_ACTION_UP;
    } else if (effect_.wetFloat > mTransition.targetWet) {
        mTransition.actionWet = EF_ACTION_DOWN;
    }
}

void Controller::effect_updateMixFloats(uint8_t effectNum_) {
    Effect& effect_ = effect[effectNum_];

    switch (effect_.type) {
        case EF_DELAY:
            effect_.dryFloat = kNumberDataLibrary[effect_.delay.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.delay.fWet].stdMultiplier;
            break;

        case EF_CHORUS:
            effect_.dryFloat = kNumberDataLibrary[effect_.chorus.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.chorus.fWet].stdMultiplier;
            break;

        case EF_FLANGER:
            effect_.dryFloat = kNumberDataLibrary[effect_.flanger.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.flanger.fWet].stdMultiplier;
            break;

        case EF_PHASER:
            effect_.dryFloat = kNumberDataLibrary[effect_.phaser.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.phaser.fWet].stdMultiplier;
            break;

        case EF_COMPRESSOR:
            effect_.dryFloat = kNumberDataLibrary[effect_.compressor.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.compressor.fWet].stdMultiplier;
            break;

        case EF_EXPANDER:
            effect_.dryFloat = kNumberDataLibrary[effect_.expander.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.expander.fWet].stdMultiplier;
            break;

        case EF_OVERDRIVE:
            effect_.dryFloat = kNumberDataLibrary[effect_.overdrive.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.overdrive.fWet].stdMultiplier;
            break;

        case EF_DISTORTION:
            effect_.dryFloat = kNumberDataLibrary[effect_.distortion.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.distortion.fWet].stdMultiplier;
            break;

        case EF_BITCRUSHER:
            effect_.dryFloat = kNumberDataLibrary[effect_.bitcrusher.eDry].stdMultiplier;
            effect_.wetFloat = kNumberDataLibrary[effect_.bitcrusher.fWet].stdMultiplier;
            break;
    }
}

void Controller::effect_cleanMemory(uint8_t effectNum_, uint8_t type_) {
    /// @brief Clears internal memory buffers/state for selected effect type.

    Effect& effect_ = effect[effectNum_];

    // 1. TYPE-SPECIFIC MEMORY CLEAR
    // Route cleanup to the active effect engine and clear associated buffers.
    switch (type_) {
        case EF_DELAY:
            effect_.delay.cleanMemory();
            memset((uint8_t*)effect_.delayAddress, 0x00, kDelayByteSize);
            break;

        case EF_CHORUS:
            effect_.chorus.cleanMemory();
            memset((uint8_t*)effect_.chorusAddress, 0x00, kChorusByteSize);
            break;

        case EF_FLANGER:
            effect_.flanger.cleanMemory();
            memset(effect_.flangerBuffer, 0x00, sizeof(effect_.flangerBuffer));
            break;

        case EF_PHASER:
            memset(effect_.phaser.ff, 0x00, sizeof(effect_.phaser.ff));
            memset(effect_.phaser.fb, 0x00, sizeof(effect_.phaser.fb));
            break;

        case EF_COMPRESSOR:
            effect_.compressor.cleanMemory();
            break;

        case EF_EXPANDER:
            effect_.expander.cleanMemory();
            break;

        case EF_OVERDRIVE:
            effect_.overdrive.cleanMemory();
            break;

        case EF_DISTORTION:
            effect_.distortion.cleanMemory();
            break;

        case EF_BITCRUSHER:
            effect_.bitcrusher.cleanMemory();
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Reverb Functions ----------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::reverb_select() {
    /// @brief Switches to Reverb menu and prepares UI/encoder context for reverb operations.

    // 1. MENU STATE TRANSITION
    // Enter reverb menu only when not already active.
    if (menu != REVERB_MENU) {
        // Store previous menu context and activate REVERB_MENU tab root.
        preMenu = menu;
        menu = REVERB_MENU;
        preMenuTab = menuTab;
        menuTab = 0;

        // 2. CONTEXT CLEANUP
        // Clear layer/LFO selection artifacts from previously active menu.
        preMenuLayerClear();

        // 3. UI TRANSITION & RENDER
        // Animate menu change and render reverb screen components.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawReverbMenu();

        // 4. ENCODER CONTEXT UPDATE
        // Rebind encoders to Reverb menu local/global parameter mapping.
        encoder_update();
    }
}

void Controller::reverb_reset() {
    /// @brief Restores reverb state parameters to default values.

    // 1. PARAMETER RESET
    // Re-apply default active, time-domain, modulation, and mix values.
    reverb_setActive(kInitialReverbActive);
    reverb_setSize(kInitialReverbSize);
    reverb_setDecay(kInitialReverbDecay);
    reverb_setDamping(kInitialReverbDamping);
    reverb_setPreDelay(kInitialReverbPreDelay);
    reverb_setSurround(kInitialReverbSurround);
    reverb_setDry(kInitialReverbDry);
    reverb_setWet(kInitialReverbWet);
}

void Controller::reverb_menuRight() {
    /// @brief Advances Reverb menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Advance tab index and wrap to first tab at upper boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab < kMaxMenu8Tab) ? (menuTab + 1) : kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::reverb_menuLeft() {
    /// @brief Moves Reverb menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Decrement tab index and wrap to last tab at lower boundary.
    preMenuTab = menuTab;
    menuTab = (menuTab > kMinMenu8Tab) ? (menuTab - 1) : kMaxMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::reverb_menuUp() {
    /// @brief Increments or toggles parameter mapped to active Reverb tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific reverb setting.
    switch (menuTab) {
        case 0:
            reverb_setActive(!reverb.active);
            break;

        case 1:
            if (reverb.size < kMaxReverbSize) reverb_setSize(reverb.size + 1);
            break;

        case 2:
            if (reverb.decay < kMaxReverbDecay) reverb_setDecay(reverb.decay + 1);
            break;

        case 3:
            if (reverb.damping < kMaxReverbDamping) reverb_setDamping(reverb.damping + 1);
            break;

        case 4:
            if (reverb.preDelay < kMaxReverbPreDelay) reverb_setPreDelay(reverb.preDelay + 1);
            break;

        case 5:
            if (reverb.surround < kMaxReverbSurround) reverb_setSurround(reverb.surround + 1);
            break;

        case 6:

            // Keep dry/wet within mix constraints by compensating opposite side.
            if (reverb.dry < kMaxReverbDry) {
                reverb_setDry(reverb.dry + 1);
            } else if ((reverb.dry == kMaxReverbDry) && (reverb.wet > kMinReverbWet)) {
                reverb_setWet(reverb.wet - 1);
            }
            break;

        case 7:

            // Keep dry/wet within mix constraints by compensating opposite side.
            if (reverb.wet < kMaxReverbWet) {
                reverb_setWet(reverb.wet + 1);
            } else if ((reverb.wet == kMaxReverbWet) && (reverb.dry > kMinReverbDry)) {
                reverb_setDry(reverb.dry - 1);
            }
            break;
    }
}

void Controller::reverb_menuDown() {
    /// @brief Decrements or toggles parameter mapped to active Reverb tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific reverb setting.
    switch (menuTab) {
        case 0:
            reverb_setActive(!reverb.active);
            break;

        case 1:
            if (reverb.size > kMinReverbSize) reverb_setSize(reverb.size - 1);
            break;

        case 2:
            if (reverb.decay > kMinReverbDecay) reverb_setDecay(reverb.decay - 1);
            break;

        case 3:
            if (reverb.damping > kMinReverbDamping) reverb_setDamping(reverb.damping - 1);
            break;

        case 4:
            if (reverb.preDelay > kMinReverbPreDelay) reverb_setPreDelay(reverb.preDelay - 1);
            break;

        case 5:
            if (reverb.surround > kMinReverbSurround) reverb_setSurround(reverb.surround - 1);
            break;

        case 6:
            if (reverb.dry > kMinReverbDry) reverb_setDry(reverb.dry - 1);
            break;

        case 7:
            if (reverb.wet > kMinReverbWet) reverb_setWet(reverb.wet - 1);
            break;
    }
}

void Controller::reverb_setActive(bool active_) {
    /// @brief Sets reverb active state using transition flow when runtime conditions allow.

    // 1. TRANSITION GUARD
    // Ignore requests while a previous reverb transition is still active.
    if (!reverb.genTransition.active) {
        if (loadAction) {
            // 2. LOAD-TIME DIRECT APPLY
            // During load workflow, apply state immediately without transition.
            reverb.active = active_;
        } else {
            // 3. RUNTIME TRANSITION APPLY
            // Start active/inactive crossfade transition for smooth output change.
            // if (active_) reverb.cleanMemory();
            reverb_genTransition(REV_MODE_ACTIVE, reverb.active, active_);
            reverb.active = active_;

            // 4. UI & GLOBAL REFRESH
            // Refresh Reverb tab data (if visible) and global summary field.
            if (menu == REVERB_MENU) lcd_drawReverb_ActiveData();
            lcd_updateGlobalMenuData(ENC_REVERB_ACTIVE);
        }
    }
}

void Controller::reverb_setSize(uint8_t size_) {
    /// @brief Sets reverb room-size index and refreshes dependent UI/global state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured size limits.
    if ((size_ >= kMinReverbSize) && (size_ <= kMaxReverbSize)) {
        // 2. DATA UPDATE
        // Apply size update through reverb parameter API.
        reverb.setSize(size_);

        // 3. UI & GLOBAL REFRESH
        // Refresh Reverb tab data (if visible) and global summary field.
        if (menu == REVERB_MENU) lcd_drawReverb_SizeData();
        lcd_updateGlobalMenuData(ENC_REVERB_SIZE);
    }
}

void Controller::reverb_setDecay(uint8_t decay_) {
    /// @brief Sets reverb decay index and refreshes dependent UI/global state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured decay limits.
    if ((decay_ >= kMinReverbDecay) && (decay_ <= kMaxReverbDecay)) {
        // 2. DATA UPDATE
        // Apply decay update through reverb parameter API.
        reverb.setDecay(decay_);

        // 3. UI & GLOBAL REFRESH
        // Refresh Reverb tab data (if visible) and global summary field.
        if (menu == REVERB_MENU) lcd_drawReverb_DecayData();
        lcd_updateGlobalMenuData(ENC_REVERB_DECAY);
    }
}

void Controller::reverb_setDamping(uint8_t damping_) {
    /// @brief Sets reverb damping index and refreshes dependent UI/global state.

    // 1. INPUT VALIDATION
    // Accept only values inside configured damping limits.
    if ((damping_ >= kMinReverbDamping) && (damping_ <= kMaxReverbDamping)) {
        // 2. DATA UPDATE
        // Apply damping update through reverb parameter API.
        reverb.setDamping(damping_);

        // 3. UI & GLOBAL REFRESH
        // Refresh Reverb tab data (if visible) and global summary field.
        if (menu == REVERB_MENU) lcd_drawReverb_DampingData();
        lcd_updateGlobalMenuData(ENC_REVERB_DAMPING);
    }
}

void Controller::reverb_setPreDelay(uint8_t preDelay_) {
    /// @brief Sets reverb pre-delay and applies transition path when reverb is active.

    // 1. INPUT VALIDATION
    // Accept only values inside configured pre-delay limits.
    if ((preDelay_ >= kMinReverbPreDelay) && (preDelay_ <= kMaxReverbPreDelay)) {
        // 2. TRANSITION GUARD
        // Ignore requests while a previous reverb transition is still active.
        if ((!reverb.genTransition.active)) {
            if (loadAction) {
                // 3. LOAD-TIME DIRECT APPLY
                // During load workflow, apply value immediately without transition.
                reverb.preDelay = preDelay_;
                reverb.setPreDelay(kReverbPreDelayDataLibrary[preDelay_].data);
            } else {
                // 4. RUNTIME APPLY MODE
                // Active reverb uses transition; inactive reverb updates directly.
                if (reverb.active) {
                    reverb.preDelay = preDelay_;
                    reverb_genTransition(REV_MODE_PREDELAY, reverb.active, reverb.active);
                } else {
                    reverb.preDelay = preDelay_;
                    reverb.setPreDelay(kReverbPreDelayDataLibrary[preDelay_].data);
                }

                // 5. UI & GLOBAL REFRESH
                // Refresh Reverb tab data (if visible) and global summary field.
                if (menu == REVERB_MENU) lcd_drawReverb_PreDelayData();
                lcd_updateGlobalMenuData(ENC_REVERB_PREDELAY);
            }
        }
    }
}

void Controller::reverb_setSurround(uint8_t surround_) {
    /// @brief Sets reverb surround amount and applies transition path when reverb is active.

    // 1. INPUT VALIDATION
    // Accept only values inside configured surround limits.
    if ((surround_ >= kMinReverbSurround) && (surround_ <= kMaxReverbSurround)) {
        // 2. TRANSITION GUARD
        // Ignore requests while a previous reverb transition is still active.
        if ((!reverb.genTransition.active)) {
            if (loadAction) {
                // 3. LOAD-TIME DIRECT APPLY
                // During load workflow, apply value immediately without transition.
                reverb.surround = surround_;
                reverb.setSurround(kReverbSurroundDataLibrary[surround_].data);
            } else {
                // 4. RUNTIME APPLY MODE
                // Active reverb uses transition; inactive reverb updates directly.
                if (reverb.active) {
                    reverb.surround = surround_;
                    reverb_genTransition(REV_MODE_SURROUND, reverb.active, reverb.active);
                } else {
                    reverb.surround = surround_;
                    reverb.setSurround(kReverbSurroundDataLibrary[surround_].data);
                }

                // 5. UI & GLOBAL REFRESH
                // Refresh Reverb tab data (if visible) and global summary field.
                if (menu == REVERB_MENU) lcd_drawReverb_SurroundData();
                lcd_updateGlobalMenuData(ENC_REVERB_SURROUND);
            }
        }
    }
}

void Controller::reverb_setDry(uint8_t dry_) {
    /// @brief Sets reverb dry mix component with optional wet compensation and transition.

    // 1. INPUT VALIDATION & TRANSITION GUARD
    // Accept valid range only when mix transition engine is idle.
    if ((dry_ >= kMinReverbDry) && (dry_ <= kMaxReverbDry) && (!reverb.mixTransition.active)) {
        // 2. TARGET MIX RESOLUTION
        // Resolve dry/wet targets and enforce optional sum limit.
        uint8_t targetDry = dry_;
        uint8_t targetWet = reverb.wet;
        if ((reverb.limitMix) && ((targetDry + targetWet) > reverb.limitMixData)) {
            targetWet = reverb.limitMixData - targetDry;
        }
        reverb.dry = targetDry;
        reverb.wet = targetWet;
        float targetDryFloat = kNumberDataLibrary[reverb.dry].stdMultiplier;
        float targetWetFloat = kNumberDataLibrary[reverb.wet].stdMultiplier;

        // 3. MIX APPLY MODE
        // Active reverb uses transition; inactive reverb updates values directly.
        if (reverb.active) {
            reverb_mixTransition(targetDryFloat, targetWetFloat);
        } else {
            reverb.dryFloat = targetDryFloat;
            reverb.wetFloat = targetWetFloat;
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh Reverb tab data (if visible) and global summary fields.
        if (menu == REVERB_MENU) {
            lcd_drawReverb_DryData();
            lcd_drawReverb_WetData();
        }
        lcd_updateGlobalMenuData(ENC_REVERB_DRY);
        lcd_updateGlobalMenuData(ENC_REVERB_WET);
    }
}

void Controller::reverb_setWet(uint8_t wet_) {
    /// @brief Sets reverb wet mix component with optional dry compensation and transition.

    // 1. INPUT VALIDATION & TRANSITION GUARD
    // Accept valid range only when mix transition engine is idle.
    if ((wet_ >= kMinReverbWet) && (wet_ <= kMaxReverbWet) && (!reverb.mixTransition.active)) {
        // 2. TARGET MIX RESOLUTION
        // Resolve dry/wet targets and enforce optional sum limit.
        uint8_t targetDry = reverb.dry;
        uint8_t targetWet = wet_;
        if ((reverb.limitMix) && ((targetDry + targetWet) > reverb.limitMixData)) {
            targetDry = reverb.limitMixData - targetWet;
        }
        reverb.dry = targetDry;
        reverb.wet = targetWet;
        float targetDryFloat = kNumberDataLibrary[reverb.dry].stdMultiplier;
        float targetWetFloat = kNumberDataLibrary[reverb.wet].stdMultiplier;

        // 3. MIX APPLY MODE
        // Active reverb uses transition; inactive reverb updates values directly.
        if (reverb.active) {
            reverb_mixTransition(targetDryFloat, targetWetFloat);
        } else {
            reverb.dryFloat = targetDryFloat;
            reverb.wetFloat = targetWetFloat;
        }

        // 4. UI & GLOBAL REFRESH
        // Refresh Reverb tab data (if visible) and global summary fields.
        if (menu == REVERB_MENU) {
            lcd_drawReverb_DryData();
            lcd_drawReverb_WetData();
        }
        lcd_updateGlobalMenuData(ENC_REVERB_DRY);
        lcd_updateGlobalMenuData(ENC_REVERB_WET);
    }
}

void Controller::reverb_genTransition(ReverbTransitionMode mode_, bool activeActive_, bool targetActive_) {
    /// @brief Configures and starts reverb dry/wet transition for active/predelay/surround changes.

    ReverbGenTransition& gTransition = reverb.genTransition;

    // 1. TRANSITION CONTEXT INITIALIZATION
    // Store mode/phase/state parameters for transition engine.
    gTransition.mode = mode_;
    gTransition.phase = REV_PHASE_A;
    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;

    // 2. MODE-SPECIFIC TARGET MAPPING
    // Map requested mode to dry/wet start/target pairs.
    switch (mode_) {
        case REV_MODE_NONE:
            break;

        case REV_MODE_ACTIVE:
            switch (targetActive_) {
                case true:
                    gTransition.activeDry = 1.0;
                    gTransition.targetDry = 0.0;
                    gTransition.activeWet = 0.0;
                    gTransition.targetWet = 1.0;
                    transitionShowFlag = 2;
                    checkMenuTransition(REVERB_MENU, true);
                    checkEncoderTransition(ENC_REVERB_ACTIVE, true);
                    break;

                case false:
                    gTransition.activeDry = 0.0;
                    gTransition.targetDry = 1.0;
                    gTransition.activeWet = 1.0;
                    gTransition.targetWet = 0.0;
                    transitionShowFlag = 1;
                    checkMenuTransition(REVERB_MENU, true);
                    checkEncoderTransition(ENC_REVERB_ACTIVE, true);
                    break;
            }
            break;

        case REV_MODE_PREDELAY:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;
            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;
            transitionShowFlag = 1;
            checkMenuTransition(REVERB_MENU, true);
            checkEncoderTransition(ENC_REVERB_PREDELAY, true);
            break;

        case REV_MODE_SURROUND:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;
            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;
            transitionShowFlag = 1;
            checkMenuTransition(REVERB_MENU, true);
            checkEncoderTransition(ENC_REVERB_SURROUND, true);
            break;
    }

    // 3. TRANSITION PRE-CALC & ARM
    // Calculate action directions and mark transition as active.
    reverb_calculateGenTransition();
    gTransition.active = true;
}

void Controller::reverb_mixTransition(float dryFloat_, float wetFloat_) {
    /// @brief Queues reverb dry/wet mix transition targets and activates transition engine.

    ReverbMixTransition& mTransition = reverb.mixTransition;

    // 1. TARGET UPDATE
    // Store requested dry/wet target multipliers.
    mTransition.targetDry = dryFloat_;
    mTransition.targetWet = wetFloat_;

    // 2. TRANSITION PRE-CALC & ARM
    // Calculate direction/step and mark transition as active.
    reverb_calculateMixTransition();
    mTransition.active = true;
}

void Controller::reverb_calculateGenTransition() {
    /// @brief Determines reverb generator transition directions against active targets.

    ReverbGenTransition& gTransition = reverb.genTransition;

    // 1. DRY PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for dry path.
    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = REV_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = REV_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = REV_ACTION_DOWN;
    }

    // 2. WET PATH DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for wet path.
    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = REV_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = REV_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = REV_ACTION_DOWN;
    }
}

void Controller::reverb_calculateMixTransition() {
    /// @brief Determines reverb mix transition directions against dry/wet float targets.

    ReverbMixTransition& mTransition = reverb.mixTransition;

    // 1. DRY MIX DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for dry mix.
    if (reverb.dryFloat == mTransition.targetDry) {
        mTransition.actionDry = REV_ACTION_NONE;
    } else if (reverb.dryFloat < mTransition.targetDry) {
        mTransition.actionDry = REV_ACTION_UP;
    } else if (reverb.dryFloat > mTransition.targetDry) {
        mTransition.actionDry = REV_ACTION_DOWN;
    }

    // 2. WET MIX DIRECTION RESOLUTION
    // Map current-to-target relation into NONE/UP/DOWN for wet mix.
    if (reverb.wetFloat == mTransition.targetWet) {
        mTransition.actionWet = REV_ACTION_NONE;
    } else if (reverb.wetFloat < mTransition.targetWet) {
        mTransition.actionWet = REV_ACTION_UP;
    } else if (reverb.wetFloat > mTransition.targetWet) {
        mTransition.actionWet = REV_ACTION_DOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////
/* Lfo Functions -------------------------------------------------------------*/
////////////////////////////////////////////////////////////////////////////////

void Controller::lfo_select(uint8_t lfoNum_) {
    /// @brief Switches to LFO menu and focuses requested LFO slot.
    /// Prepares UI, encoder context, and LED feedback for LFO editing operations.

    // 1. MENU ENTRY VALIDATION
    // Continue only when either menu changes or focused LFO slot is different.
    if ((menu != LFO_MENU) || ((menu == LFO_MENU) && (lfoNum_ != selectedLfoNum))) {
        // 2. MENU/TAB CONTEXT PREPARATION
        // Store previous tab and reset navigation to LFO root tab.
        preMenu = menu;
        preMenuTab = menuTab;
        menuTab = 0;
        menu = LFO_MENU;

        // 3. TARGET SELECTION UPDATE
        // Clear previous layer selection context and set target LFO index.
        preMenuLayerClear();
        selectedLfoNum = lfoNum_;

        // 4. UI TRANSITION & RENDER
        // Animate transitions and render selected LFO menu view.
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawLfoMenu(lfoNum_);

        // 5. ENCODER CONTEXT UPDATE
        // Rebind encoders to LFO menu local parameter mapping.
        encoder_update();

        // 6. LED FEEDBACK UPDATE
        // Route selected LFO index to layer-select LED bit mask.
        ledLayerPlayMode = false;
        ledLayerSelectFlag = true;
        ledLayerSelectData = 0x01 << selectedLfoNum;
    }
}

void Controller::lfo_reset(uint8_t lfoNum_) {
    /// @brief Restores LFO parameters to default values.
    /// Applies defaults through setter path to keep UI/runtime state synchronized.

    // 1. PARAMETER RESET
    // Re-apply default active, waveform, timing, and depth values.
    lfo_setActive(lfoNum_, kInitialLfoActive);
    lfo_setType(lfoNum_, kInitialLfoType);
    lfo_setRate(lfoNum_, kInitialLfoRate);
    lfo_setPhase(lfoNum_, kInitialLfoPhase);
    lfo_setInvert(lfoNum_, kInitialLfoInvert);
    lfo_setDepth(lfoNum_, kInitialLfoDepth);
}

void Controller::lfo_menuRight() {
    /// @brief Advances LFO menu tab to the right with wrap-around.

    // 1. TAB STATE UPDATE
    // Save previous tab and advance index with hidden-tab skip.
    preMenuTab = menuTab;

    // Hidden-tab skip: jump from tab 1 to 4 before continuing.
    if (menuTab < kMaxMenu8Tab)
        (menuTab == 1) ? menuTab = 4 : menuTab += 1;
    else
        menuTab = kMinMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::lfo_menuLeft() {
    /// @brief Moves LFO menu tab to the left with wrap-around.

    // 1. TAB STATE UPDATE
    // Save previous tab and decrement index with hidden-tab skip.
    preMenuTab = menuTab;

    // Hidden-tab skip: jump from tab 4 to 1 before continuing.
    if (menuTab > kMinMenu8Tab)
        (menuTab == 4) ? menuTab = 1 : menuTab -= 1;
    else
        menuTab = kMaxMenu8Tab;

    // 2. UI TRANSITION
    // Trigger selection transition animation for tab change.
    lcd_transitionSelect();
}

void Controller::lfo_menuUp() {
    /// @brief Increments or toggles parameter mapped to active LFO tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-up action to tab-specific LFO setting.
    switch (menuTab) {
        case 0:

            // ACTIVE: toggle modulation enable state.
            lfo_setActive(selectedLfoNum, !lfoLibrary[selectedLfoNum].active);
            break;

        case 1:

            // TYPE: increment with wraparound at upper boundary.
            if (lfoLibrary[selectedLfoNum].type < kMaxLfoType)
                lfo_setType(selectedLfoNum, lfoLibrary[selectedLfoNum].type + 1);
            else
                lfo_setType(selectedLfoNum, kMinLfoType);
            break;

        case 4:

            // RATE: increment until max boundary.
            if (lfoLibrary[selectedLfoNum].rate < kMaxLfoRate)
                lfo_setRate(selectedLfoNum, lfoLibrary[selectedLfoNum].rate + 1);
            break;

        case 5:

            // PHASE: increment with wraparound at upper boundary.
            if (lfoLibrary[selectedLfoNum].phase < kMaxLfoPhase)
                lfo_setPhase(selectedLfoNum, lfoLibrary[selectedLfoNum].phase + 1);
            else
                lfo_setPhase(selectedLfoNum, kMinLfoPhase);

            break;

        case 6:

            // INVERT: toggle waveform polarity.
            lfo_setInvert(selectedLfoNum, !lfoLibrary[selectedLfoNum].invert);
            break;

        case 7:

            // DEPTH: increment until max boundary.
            if (lfoLibrary[selectedLfoNum].depth < kMaxLfoDepth)
                lfo_setDepth(selectedLfoNum, lfoLibrary[selectedLfoNum].depth + 1);
            break;
    }
}

void Controller::lfo_menuDown() {
    /// @brief Decrements or toggles parameter mapped to active LFO tab.

    // 1. TAB-BASED PARAMETER DISPATCH
    // Route encoder-down action to tab-specific LFO setting.
    switch (menuTab) {
        case 0:

            // ACTIVE: toggle modulation enable state.
            lfo_setActive(selectedLfoNum, !lfoLibrary[selectedLfoNum].active);
            break;

        case 1:

            // TYPE: decrement with wraparound at lower boundary.
            if (lfoLibrary[selectedLfoNum].type > kMinLfoType)
                lfo_setType(selectedLfoNum, lfoLibrary[selectedLfoNum].type - 1);
            else
                lfo_setType(selectedLfoNum, kMaxLfoType);
            break;

        case 4:

            // RATE: decrement until min boundary.
            if (lfoLibrary[selectedLfoNum].rate > kMinLfoRate)
                lfo_setRate(selectedLfoNum, lfoLibrary[selectedLfoNum].rate - 1);
            break;

        case 5:

            // PHASE: decrement with wraparound at lower boundary.
            if (lfoLibrary[selectedLfoNum].phase > kMinLfoPhase)
                lfo_setPhase(selectedLfoNum, lfoLibrary[selectedLfoNum].phase - 1);
            else
                lfo_setPhase(selectedLfoNum, kMaxLfoPhase);
            break;

        case 6:

            // INVERT: toggle waveform polarity.
            lfo_setInvert(selectedLfoNum, !lfoLibrary[selectedLfoNum].invert);
            break;

        case 7:

            // DEPTH: decrement until min boundary.
            if (lfoLibrary[selectedLfoNum].depth > kMinLfoDepth)
                lfo_setDepth(selectedLfoNum, lfoLibrary[selectedLfoNum].depth - 1);
            break;
    }
}

void Controller::lfo_setActive(uint8_t lfoNum_, uint8_t active_) {
    /// @brief Sets LFO active state and refreshes dependent UI/global state.

    // 1. DATA UPDATE
    // Apply active state to target LFO slot.
    lfoLibrary[lfoNum_].active = active_;

    // 2. UI REFRESH
    // Refresh LFO tab data only when edited slot is currently selected.
    if ((menu == LFO_MENU) && (lfoNum_ == selectedLfoNum)) {
        lcd_drawLfo_ActiveData(lfoNum_);
        lcd_drawLfo_GraphData(lfoNum_);
    }
}

void Controller::lfo_setType(uint8_t lfoNum_, uint8_t type_) {
    /// @brief Sets LFO waveform type and refreshes dependent UI/global state.

    // 1. DATA UPDATE
    // Apply waveform type to target LFO slot.
    lfoLibrary[lfoNum_].type = type_;

    // 2. UI REFRESH
    // Refresh LFO tab data only when edited slot is currently selected.
    if ((menu == LFO_MENU) && (lfoNum_ == selectedLfoNum)) {
        lcd_drawLfo_TypeData(lfoNum_);
        lcd_drawLfo_GraphData(lfoNum_);
    }
}

void Controller::lfo_setRate(uint8_t lfoNum_, uint8_t rate_) {
    /// @brief Sets LFO rate and recalculates sample increment.

    // 1. DATA UPDATE
    // Store selected rate index for target LFO slot.
    lfoLibrary[lfoNum_].rate = rate_;

    // 2. DERIVED PARAMETER CALCULATION
    // Convert rate data to wavetable sample increment using bar interval.
    lfoLibrary[lfoNum_].sampleInc = (kLfoSampleSize * kLfoRateDataLibrary[lfoLibrary[lfoNum_].rate].rate) / barInterval;

    // 3. UI REFRESH
    // Refresh LFO tab data only when edited slot is currently selected.
    if ((menu == LFO_MENU) && (lfoNum_ == selectedLfoNum)) {
        lcd_drawLfo_RateData(lfoNum_);
        lcd_drawLfo_GraphData(lfoNum_);
    }
}

void Controller::lfo_setPhase(uint8_t lfoNum_, uint8_t phase_) {
    /// @brief Sets LFO phase and compensates sample cursor continuity.

    // 1. DATA UPDATE
    // Store phase index and calculate phase-based sample offset.
    uint8_t prePhase = lfoLibrary[lfoNum_].phase;
    lfoLibrary[lfoNum_].phase = phase_;
    int32_t prePhaseStart = kLfoPhaseDataLibrary[prePhase].sampleStart;
    int32_t postPhaseStart = kLfoPhaseDataLibrary[phase_].sampleStart;
    int32_t phaseShift = postPhaseStart - prePhaseStart;
    lfoLibrary[lfoNum_].sampleStart = postPhaseStart;
    lfoLibrary[lfoNum_].sampleCounter += phaseShift;

    // 2. COUNTER NORMALIZATION
    // Keep sample counter inside valid wavetable bounds.
    if (lfoLibrary[lfoNum_].sampleCounter < 0) lfoLibrary[lfoNum_].sampleCounter = kLfoSampleSize - lfoLibrary[lfoNum_].sampleCounter;
    if (lfoLibrary[lfoNum_].sampleCounter > (kLfoSampleSize - 1)) lfoLibrary[lfoNum_].sampleCounter = lfoLibrary[lfoNum_].sampleCounter - kLfoSampleSize;

    // 3. UI REFRESH
    // Refresh LFO tab data only when edited slot is currently selected.
    if ((menu == LFO_MENU) && (lfoNum_ == selectedLfoNum)) {
        lcd_drawLfo_PhaseData(lfoNum_);
        lcd_drawLfo_GraphData(lfoNum_);
    }
}

void Controller::lfo_setInvert(uint8_t lfoNum_, bool invert_) {
    /// @brief Sets LFO invert mode and refreshes dependent UI/global state.

    // 1. DATA UPDATE
    // Apply invert mode to target LFO slot.
    lfoLibrary[lfoNum_].invert = invert_;

    // 2. UI REFRESH
    // Refresh LFO tab data only when edited slot is currently selected.
    if ((menu == LFO_MENU) && (lfoNum_ == selectedLfoNum)) {
        lcd_drawLfo_InvertData(lfoNum_);
        lcd_drawLfo_GraphData(lfoNum_);
    }
}

void Controller::lfo_setDepth(uint8_t lfoNum_, uint8_t depth_) {
    /// @brief Sets LFO depth and refreshes dependent UI/global state.

    // 1. DATA UPDATE
    // Apply depth value to target LFO slot.
    lfoLibrary[lfoNum_].depth = depth_;

    // 2. UI REFRESH
    // Refresh LFO tab data only when edited slot is currently selected.
    if ((menu == LFO_MENU) && (lfoNum_ == selectedLfoNum)) {
        lcd_drawLfo_DepthData(lfoNum_);
        lcd_drawLfo_GraphData(lfoNum_);
    }
}

void Controller::lfo_copy() {
    /// @brief Copies selected LFO parameters to clipboard buffer.

    // 1. SELECTION VALIDATION
    // Continue only when target LFO selection is valid.
    if (selectedLfoNum != -1) {
        // 2. CLIPBOARD SNAPSHOT
        // Mark clipboard active and store editable LFO parameters.
        lfoCopy.copyActive = true;
        Lfo& lfo = lfoLibrary[selectedLfoNum];
        lfoCopy.active = lfo.active;
        lfoCopy.type = lfo.type;
        lfoCopy.rate = lfo.rate;
        lfoCopy.phase = lfo.phase;
        lfoCopy.depth = lfo.depth;
    }
}

void Controller::lfo_paste() {
    /// @brief Pastes clipboard LFO parameters to selected LFO slot.

    // 1. CLIPBOARD & TARGET VALIDATION
    // Continue only when clipboard is active and target selection is valid.
    if ((lfoCopy.copyActive) && (selectedLfoNum != -1)) {
        // 2. PARAMETER RESTORE
        // Apply clipboard values through setters for consistent UI/runtime updates.
        lfo_setActive(selectedLfoNum, lfoCopy.active);
        lfo_setType(selectedLfoNum, lfoCopy.type);
        lfo_setRate(selectedLfoNum, lfoCopy.rate);
        lfo_setPhase(selectedLfoNum, lfoCopy.phase);
        lfo_setDepth(selectedLfoNum, lfoCopy.depth);
    }
}

