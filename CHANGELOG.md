# Changelog

Application firmware versions (`kVersionMajor.kVersionMinor` in the app's `Global.h`; the
SD-card package is named after it). Rolling back = installing an older package.

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
