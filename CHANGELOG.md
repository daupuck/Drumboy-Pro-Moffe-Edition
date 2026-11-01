# Changelog

Application firmware versions (`kVersionMajor.kVersionMinor` in the app's `Global.h`; the
SD-card package is named after it). Rolling back = installing an older package.

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
