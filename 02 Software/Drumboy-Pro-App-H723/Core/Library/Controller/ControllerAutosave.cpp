// AUTOSAVE + resume, and crash-safe saving (v2.01).
// Part of class Controller (declared in Controller.h). Timing rules and the commit record are
// in Global/Autosave.h (host-tested by 02 Software/Tests/test_autosave.cpp).
//
// Every project / drumkit save writes complete temporary files first (kSaveTempMain /
// kSaveTempSide), then the commit record kSaveCommitFile, then replaces the real files. A power
// cut before the record is complete leaves the old files untouched (the temporary files are
// thrown away at the next save or power-on); a cut after it is finished at the next save or
// power-on.
//
// The session is auto-saved as an extra project (System/File/Autosave.rwf + .rwn) when it has
// changed and nobody touched the Drumboy for a while; at power-on it offers to resume it.

// Global.h's unused helpers would warn once more in this new file.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "Controller.h"
#pragma GCC diagnostic pop

namespace {

bool fileExists(const char* path_) {
    FILINFO info;
    return f_stat(path_, &info) == FR_OK;
}

// f_rename refuses to overwrite: remove the destination first (missing is fine).
bool replaceFile(const char* temp_, const char* dest_) {
    const FRESULT del = f_unlink(dest_);
    if ((del != FR_OK) && (del != FR_NO_FILE)) return false;
    return f_rename(temp_, dest_) == FR_OK;
}

}  // namespace

/* Crash-safe saving ----------------------------------------------------------*/

bool Controller::sd_applyCommit() {
    /// @brief Finishes (or discards) a save that the card shows to be in progress. Safe to run
    /// any number of times. Returns false only when a committed save could not be finished.

    uint8_t record[autosave::kRecordSize] = {};
    UINT got = 0;
    bool recordValid = false;
    char mainPath[autosave::kPathSize] = {};
    char sidePath[autosave::kPathSize] = {};
    uint8_t sideMode = autosave::kSideKeep;

    f_close(&sd.file);
    if (f_open(&sd.file, kSaveCommitFile, FA_READ) == FR_OK) {
        if (f_read(&sd.file, record, sizeof(record), &got) == FR_OK) {
            recordValid = autosave::decode(record, static_cast<uint16_t>(got), mainPath, sidePath, sideMode);
        }
        f_close(&sd.file);
    }

    const autosave::Plan plan = autosave::plan(recordValid, sideMode, fileExists(kSaveTempMain), fileExists(kSaveTempSide));
    bool ok = true;
    // Side file first, main file last (see Autosave.h).
    if (plan.replaceSide && !replaceFile(kSaveTempSide, sidePath)) ok = false;
    if (ok && plan.deleteSide) {
        const FRESULT del = f_unlink(sidePath);
        if ((del != FR_OK) && (del != FR_NO_FILE)) ok = false;
    }
    if (ok && plan.replaceMain && !replaceFile(kSaveTempMain, mainPath)) ok = false;

    // Done (or nothing to do): clear the record and anything left over. A commit that could
    // not be finished keeps everything for the next attempt.
    if (ok) {
        f_unlink(kSaveCommitFile);
        f_unlink(kSaveTempMain);
        f_unlink(kSaveTempSide);
    }
    return ok;
}

void Controller::sd_beginSafeSave() {
    /// @brief Before a save: finish an earlier committed save, or discard an unfinished one.

    sd_applyCommit();
}

SdResult Controller::sd_commitSafeSave(const char* main_, const char* side_, uint8_t sideMode_) {
    /// @brief The temporary files are complete: write the commit record, then replace the real
    /// files. From the moment the record is on the card the save counts as done.

    uint8_t record[autosave::kRecordSize];
    if (!autosave::encode(main_, side_, sideMode_, record)) return SD_ERROR;

    f_close(&sd.file);
    if (f_open(&sd.file, kSaveCommitFile, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) return SD_ERROR;
    bool written = (sd_write(record, sizeof(record)) == FR_OK);
    if (f_close(&sd.file) != FR_OK) written = false;  // f_close flushes: part of the write
    if (!written) {
        f_unlink(kSaveCommitFile);  // a torn record would be rejected anyway
        return SD_ERROR;
    }
    return sd_applyCommit() ? SD_OK : SD_ERROR;
}

void Controller::sd_recoverSave() {
    /// @brief Power-on: finish or discard a save that a power cut interrupted.

    if (!sd.ready) return;
    keyboard_disable();
    sd_applyCommit();
    keyboard_enable();
}

/* AUTOSAVE -------------------------------------------------------------------*/

void Controller::autosave_noteInput() {
    autosaveDirty = true;
    autosaveLastInput = HAL_GetTick();
}

void Controller::autosave_setEnabled(bool on_) {
    /// @brief AUTOSAVE on/off (File menu, knob 5). Turn right = ON, left = OFF.

    if (autosaveEnabled == on_) return;
    autosaveEnabled = on_;
    if (menu == FILE_MENU) lcd_drawFile_AutosaveData();

    if (systemPresetSaveActive) {
        stopSystemPresetTimer();
        startSystemPresetTimer();
    }
}

void Controller::autosave_writePending(bool pending_) {
    /// @brief Remembers in System.rwp whether an auto-save waits to be resumed (written by the
    /// main loop's preset persistence, like every other setting).

    if (autosavePending == pending_) return;
    autosavePending = pending_;
    if (systemPresetSaveActive) systemPresetSaveFlag = true;
}

void Controller::autosave_service() {
    /// @brief Main loop: auto-save the session when it has changed and the Drumboy has been
    /// left alone for a while (10 s stopped, 30 s playing; never while recording or while an
    /// alert or a file operation is on screen).

    const uint32_t now = HAL_GetTick();

    // Recording (pads or MIDI) changes beats without a key per change: wait until it stops.
    if (recordActive) {
        autosaveDirty = true;
        autosaveLastInput = now;
        return;
    }

    autosave::State s;
    s.enabled = autosaveEnabled;
    s.sdReady = sd.ready;
    s.dirty = autosaveDirty;
    s.busy = alertFlag || loadAction || saveAction || (menu == TEST_MENU) || (menu == INIT_MENU);
    s.playing = playActive;
    s.now = now;
    s.lastInput = autosaveLastInput;
    s.lastSave = autosaveLastSave;
    s.savedOnce = autosaveSavedOnce;
    if (!autosave::due(s)) return;

    autosaveLastSave = now;
    autosaveSavedOnce = true;

    // The File menu shows the status of the slot under the cursor; saving the auto-save
    // checks its own file, so keep the menu's value.
    const FileStatus keepStatus = fileStatus;
    const bool ok = (sd_saveFile(kAutosaveFileNum) == SD_OK);
    fileStatus = keepStatus;

    if (ok) {
        autosaveDirty = false;
        autosave_writePending(true);
    }
    // A failed auto-save stays dirty and is tried again after kMinGapMs.
}

void Controller::autosave_askResume() {
    /// @brief End of boot: offer to resume the auto-saved session (ADD = yes, CLEAR = no).

    if (!autosaveEnabled || !autosavePending || !sd.ready) return;

    const FileStatus keepStatus = fileStatus;
    sd_checkFile(kAutosaveFileNum);
    const bool available = (fileStatus == FILE_ACTIVE);
    fileStatus = keepStatus;
    if (!available) {
        autosave_writePending(false);
        return;
    }

    alertFlag = true;
    alertType = ALERT_RESUME;
    lcd_drawAlert();
}

void Controller::autosave_answerResume(bool yes_) {
    /// @brief The RESUME question was answered. Either way it is not asked again until the
    /// next auto-save. NO keeps the auto-save on the card (it is replaced by the next one).

    autosave_writePending(false);
    if (!yes_) return;

    lcd_clearAlert();
    reset();
    lcd_resetPlay();

    loadAction = true;
    const FileStatus keepStatus = fileStatus;
    const SdResult result = sd_loadFile(kAutosaveFileNum);  // also restores currentFileSlot
    fileStatus = keepStatus;
    loadAction = false;

    alertType = (result == SD_OK) ? ALERT_LOADSUCCESS : ALERT_LOADERROR;
    lcd_drawAlert();
    HAL_Delay(1000);
    lcd_clearAlert();

    // The session now equals the auto-save.
    autosaveDirty = false;
}
