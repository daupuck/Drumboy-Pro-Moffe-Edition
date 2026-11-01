# Changelog

Application firmware versions (`kVersionMajor.kVersionMinor` in the app's `Global.h`; the
SD-card package is named after it). Rolling back = installing an older package.

## v1.08

- **EUCLID page:** the step strip now shows **CLEAR: UNDO** at the bottom right, so it is
  visible that the CLEAR key restores the beats the layer had when the page opened. (The
  device font has no `=`, hence the colon.)

## v1.07

**Song mode — chain the 5 song banks into a whole song**

- **Rhythm menu, bottom row:** **5 SONG** (OFF / LOOP / ONCE), **6 STEP** (which chain step
  you edit, shown as `step/length`), **7 PART** (song bank 1–5 for that step, or END),
  **8 REPEAT** (how many loops that step plays, 1–16).
- Up to 16 steps, e.g. intro ×2 → verse ×4 → fill ×1 → chorus ×2 … The chain ends at the first
  END (or after step 16).
- With SONG on, **PLAY from the top starts at step 1** and switches to its part at once; parts
  then change automatically at the end of each loop, using the same mechanism as the manual
  SONG BANK trigger. **STOP** pauses and PLAY resumes in place; **RESET** returns to step 1.
- **LOOP** starts the chain again after the last step; **ONCE** stops (and returns to the top)
  exactly at the end of the last loop of the last step.
- While the chain runs, SONG shows the mode and the playing step, e.g. `LOOP 3/6`.
- Turning SONG on during playback starts the chain at the next loop end; turning it off keeps
  the current bank playing. Manual SONG BANK switching still works.
- **Saved with the project** (.rwf) in bytes of the file that nothing else uses — file size and
  format are unchanged, older firmware ignores them, and files from older firmware load with
  song mode OFF. Drumkit files are not affected. NEW FILE resets the chain.
- All parts share the song length set by BAR × MEASURE; use REPEAT for longer sections.

Under the hood: chain logic (start, advance, LOOP/ONCE, encode/decode) is a pure function set in
`Global/SongChain.h`, host-tested (`test_song_chain.cpp`). The sequencer ISR calls
`song_onLoopEnd()` once per loop, just before the existing bank shift; it never draws. The
audio engine is unchanged; `interruptPlay` grew by one call (+32 B).

## v1.06

**Layer Song EUCLID page — generate Euclidean rhythms**

- **S + layer** opens Layer Song as before; pressing **S + the same layer again** switches to the
  **EUCLID** page (and back).
- Knobs: **1 STEPS** (1–32), **2 HITS** (0–STEPS), **5 ROTATE** (wraps), **6 GRID** (1:32–1:4),
  **7 LENGTH** (REPEAT over the song / ONCE), **8 MODE** (REPLACE the layer's beats / MERGE with
  them, keeping their fills).
- Every turn rewrites the layer live, so you hear it while the sequencer plays. The pattern is
  always rebuilt from the beats the layer had when the page opened, so nothing accumulates, and
  **CLEAR** puts those beats back.
- The fill-graph area shows the pattern as a step strip, titled `EUCLID E(hits,steps)`; the step
  being played is marked in cyan. `MAX 64 BEATS` appears if the bank's 64-beat limit cut it.
- STEPS × GRID that does not fill a bar gives polymeters (e.g. 7 steps of 1:16).
- The result is ordinary beats, saved with the project; fine-tune them on the edit page with
  MOVE and fills. The knob settings are not saved (they are a tool, not song data).
- On the EUCLID page, UP/DOWN and the ADD tap are inactive (they edit things the page
  regenerates); everything else in Layer Song works as before.

Under the hood: Bjorklund's algorithm, rotation, expansion to song positions and REPLACE/MERGE
are pure functions in `Global/EuclidPattern.h`, host-tested (1607 checks, including the
patterns from Toussaint's paper and maximal evenness for every steps/hits up to 32). Writes go
through the same `layerSong_rebuildBank` as MOVE/SHIFT ALL (sequencer tick masked). The page is
a flag on `LAYER_SONG_MENU`, not a new menu, so all existing Layer Song handling is unchanged.
Audio path and sequencer ISR unchanged.

## v1.05

**Sample preview in the Layer Instrument menu — hear a sample before you load it**

- When the **INST** or **SAMPLE** knob stops for 150 ms, the selected sample plays once. Turning
  again stops it and starts the next. The loaded sample and the layer are not touched until you
  press **ADD** as before.
- The selected layer's **pad** replays the preview while the selected sample differs from the
  loaded one (outside recording; otherwise the pad plays the layer as before).
- **PREVIEW** appears where **LOADING** does while it plays.
- **Metronome menu, knob 5 PREVIEW** (on/off) and **knob 6 PREV LEVEL** (0–100). Saved in the
  system preset; defaults ON / 70.
- The preview is dry (no pitch/EQ/effects of the layer), mono, and goes through master
  mute/volume/limiter. It streams from the SD card, so even long samples start at once and the
  menus stay responsive.

Under the hood: own 960 KB SDRAM buffer in a previously unused gap (the 15 sample sectors are
untouched); the SD file is opened per chunk so FatFs' two-file limit is never held; the WAV
conversion is now shared by the loader and the preview (`Global/SampleFormat.h`, host-tested).
`System.rwp` grows from 21 to 23 data bytes inside the existing 48-byte file; older firmware
still reads it, and files from older firmware load with the preview defaults.

## v1.04 — Moffe Edition

- **Boot screen:** pixel sunglasses slide down onto the logo's eyes, then **MOFFE** appears in
  large magenta letters, letter by letter, with **EDITION** fading in below. Drawn by the
  firmware on top of the SD-card logo, so no SD files change and any card works. Boot time is
  unchanged (the animation runs inside the existing pause after the logo).
- The logo moved up 35 px and the SD statistics line down 20 px to make room.
- The name, colour and timing are constants in the app's `Global.h` (`kEdition*`).
- New `Lcd::drawTextScaled()` for large text built from the device fonts.

## v1.03

**Layer Song menu — place beats where you want them**

- **MOVE (knob 7, bottom row, 3rd from left)** — moves the selected beat one grid step left or
  right. The step is the Rhythm menu *quantize* value (1:64 … 1:1). Beats can pass their
  neighbours, keep their fill, and stop at the start/end of the song. A beat that was recorded
  between grid lines snaps to the grid on the first move. The value shows the position as
  `bar.beat.sixteenth` (a trailing `+` means between sixteenths).
- **SHIFT ALL (knob 8, bottom right)** — moves every beat of the layer one grid step. Beats that
  leave the end come back in at the start, so the pattern is never lost; turn the other way to
  undo. Fills and off-grid timing are kept. The value shows the current step size.
- **ADD tap** — a short press of ADD inserts one beat in the middle of the gap after the selected
  beat (on the grid) and selects it, ready to place with MOVE. On an empty layer it puts a beat
  at the start. Holding ADD on an empty layer still runs the pattern generator, and
  ADD + LEFT/RIGHT still moves the selected beat (now with the MOVE behaviour).

**Fixes**

- ADD + LEFT/RIGHT no longer erases the fill of the moved beat and the beat before it, and no
  longer stops at the neighbouring beat.
- Clearing a layer from another menu (e.g. pasting a song) no longer silently switches the
  internal menu state to Layer Song.
- Beat sorting after quantize/clear skipped the last beat, which could leave a bank out of order.
- Global menu: REVERB DAMPING and PREDELAY showed each other's names.
- Unused knobs read past the end of the encoder function table.

## v1.02

Firmware update and SD-card robustness fixes, `Controller.cpp` split into subsystems, command-line
build + CI. See pull requests #1–#3.

## v1.01

Factory firmware from Randomwaves (`02 Software/Firmware/Drumboy_Pro_Fw_01_01.bin`).
