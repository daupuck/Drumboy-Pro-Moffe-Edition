# Drumboy Pro – Moffe Edition

Moffe Edition is **unofficial community firmware** for the Drumboy Pro, based on the open-source
[Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro) (factory firmware v1.01). It
is not affiliated with or endorsed by Randomwaves. This page collects everything that is new
compared with the factory firmware: what each feature does and how to use it on the device.

- **Current version:** v1.09 — download from the
  [latest release](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/releases/latest);
  how to install: [README → Install](README.md#install)
- **User manual:** published by Randomwaves at [randomwaves.io](https://www.randomwaves.io) (it
  describes the factory firmware); page numbers below refer to it.
- **Technical details per version:** [CHANGELOG.md](CHANGELOG.md)
- **Build, test and install:** [02 Software/DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>)

> [!WARNING]
> **Use at your own risk.** This firmware is provided "as is", without warranty of any kind (see
> [LICENSE](LICENSE)). Back up your whole SD card before installing, keep the factory firmware for
> rolling back, and only flash the bootloader over SWD if you know what you are doing. Modifying
> the firmware may affect your warranty with Randomwaves. Report problems in this repository's
> [Issues](../../issues), not to Randomwaves.

> **Status (1 November 2026):** every version is built, unit-tested and verified by CI. v1.07 has
> been run on a Drumboy Pro by the maintainer, with song mode confirmed working; the other features
> are in hardware testing. The melody layers and CPU meter of v1.09 are new and not yet tested on
> a device. Each release says what has been tested on hardware.

## Knob numbering

Knobs are numbered the way the screen shows them: two rows of four, left to right. Each
function's name is shown on screen directly above its value.

```
 Top:     [1]  [2]  [3]  [4]
 Bottom:  [5]  [6]  [7]  [8]
```

---

## 1. Place beats precisely – Layer Song menu (v1.03)

Open with **S + layer key** (manual pp. 37–38). Previously, beats could only be created with the
pattern generator or recorded live, and could not be moved freely. The two unused knobs at the
bottom right now move beats.

| Control | What it does |
|---|---|
| **Knob 7 – MOVE** | Moves the selected beat one step left or right. It passes other beats, keeps its fill and stops at the start and end of the song. The value shows the position as `bar.beat.sixteenth`, e.g. `2.3.4`; a trailing `+` means the beat sits between two sixteenths. |
| **Knob 8 – SHIFT ALL** | Moves *all* beats of the layer one step. Beats pushed past the end come back in at the start, so turning the other way restores the pattern. The value shows the step size, e.g. `1:16`. |
| **ADD (short press)** | Inserts one beat in the middle of the gap after the selected beat and selects it, ready to place with MOVE. On an empty layer the first beat goes to the start. |
| ADD (hold, empty layer) | Pattern generator, as before. |
| ADD + LEFT/RIGHT | Moves the selected beat, same as MOVE. |

**Step size** is the quantize setting in the Rhythm menu (1:64 to 1:1).

**Workflow:** select a beat with LEFT/RIGHT, tap **ADD**, then turn **MOVE** until the beat sits
where you want it.

## 2. Moffe Edition boot screen (v1.04)

At power-on the logo fades in and **pixel sunglasses** slide down over its eyes. Then **MOFFE**
appears in large magenta letters, one at a time, and **EDITION** fades in below.

- Drawn by the firmware, so the SD card and the logo file are unchanged and any card works.
- Boot time is unchanged: the animation runs inside the pause that already followed the logo.
- Name, colour and timing are constants (`kEdition*`) in the app's `Global.h`.

## 3. Sample preview – Layer Instrument menu (v1.05)

Open with **I + layer key** (manual, Layer Instrument). You can now hear a sample *before* you
load it.

| You do | What happens |
|---|---|
| Turn **SAMPLE** (or **INST**) and stop | After a short moment (0.15 s) the selected sample plays once. Turning again stops it, and the next one plays when you stop. |
| Press the **layer key** of the layer you are editing | Replays the preview, as long as the selected sample differs from the loaded one. |
| Press **ADD** | Loads the sample onto the layer, as before. |

- What is loaded on the layer is not touched until you press **ADD**.
- **PREVIEW** appears on screen where **LOADING** usually does.
- The preview is dry (no pitch, EQ or effects of the layer), but master volume, mute and limiter
  apply.
- Long samples start immediately, and the menus are never locked while the file is read.
- While recording, the layer key plays the loaded sample, as before.

**Settings in the Metronome menu:**

| Knob | Setting | Default |
|---|---|---|
| **5 – PREVIEW** | Sample preview on or off | ON |
| **6 – PREV LEVEL** | Preview volume, 0–100 | 70 |

Both are saved and kept across power cycles.

## 4. Euclidean rhythms – EUCLID page in Layer Song (v1.06)

A Euclidean pattern spreads a number of **hits** as evenly as possible over a number of
**steps**. It is a fast, musical starting point for drums. Examples: E(3,8) = `x..x..x.`
(tresillo), E(5,8) = `x.xx.xx.` (cinquillo) and E(4,16) = four even hits.

**Open the page:** press **S + layer key** as usual, then **S + the same layer key once more**.
Pressing it again moves on to the [NOTE page](#6-melody-layers--note-page-in-layer-song-v109),
and once more back to the edit page.

```
 Top:     [1 STEPS]   [2 HITS]   [ step strip: ■··■··■· ]
 Bottom:  [5 ROTATE]  [6 GRID]   [7 LENGTH]  [8 MODE]
```

| Control | What it does |
|---|---|
| **1 STEPS** | Number of steps in the pattern (1–32) |
| **2 HITS** | Number of hits (0 up to the number of steps) |
| **5 ROTATE** | Shifts the pattern one step at a time, wrapping around |
| **6 GRID** | Length of each step, from 1:32 to 1:4 |
| **7 LENGTH** | **REPEAT** repeats the pattern over the whole song, **ONCE** lays it down once |
| **8 MODE** | **REPLACE** replaces the layer's beats; **MERGE** adds the hits on top of the beats you already have, which keep their fills |
| **CLEAR** | Restores the beats the layer had when you opened the page (the page shows **CLEAR: UNDO** as a reminder) |

- **You hear changes immediately.** The pattern is written to the layer on every turn, so you can
  shape it while the sequencer plays.
- **The step strip** at the top right shows the pattern; the step being played is marked in cyan.
- **Polymeters:** STEPS 7 with GRID 1:16 gives a 7/16 pattern that drifts against the bar.
- **Maximum 64 beats** per layer and bank. If the pattern needs more, *MAX 64 BEATS* is shown and
  the rest is cut.
- **The pattern is saved as ordinary beats** in the project, and you can fine-tune it on the edit
  page with MOVE and fills. The EUCLID knob settings themselves are not saved.
- **On the EUCLID page, UP/DOWN and a short ADD press do nothing**, because the next turn would
  overwrite those edits.

## 5. Song mode – arrange parts into a song (v1.07)

Each of the 5 song banks (manual p. 39) can hold a part of a song: intro, verse, chorus, fill,
outro. Song mode plays them in an order you set, switching automatically at the end of each
loop.

**Rhythm menu, bottom row:**

```
 Top:     [1 TEMPO]  [2 MEASURE]  [3 BAR]   [4 QUANTIZE]     (unchanged)
 Bottom:  [5 SONG]   [6 STEP]     [7 PART]  [8 REPEAT]
```

| Knob | What it does |
|---|---|
| **5 SONG** | **OFF**: banks switch manually, as before. **LOOP**: the chain starts again after the last step. **ONCE**: the song stops at the end of the last step. |
| **6 STEP** | Which step of the chain you are editing (1–16), shown as `step/length` |
| **7 PART** | The part (song bank 1–5) that step plays, or **END** to end the chain there |
| **8 REPEAT** | How many loops that step plays (1–16) |

**Example:** step 1 = part 1 × 2 (intro), step 2 = part 2 × 4 (verse), step 3 = part 3 × 1
(fill), step 4 = part 4 × 2 (chorus), step 5 = END.

- **PLAY** from the top starts at step 1. **STOP** pauses, and PLAY continues where it was.
  **RESET** goes back to step 1.
- **ONCE** stops exactly at the end of the last loop of the last step, ready to play again from
  the top.
- While the song plays, SONG shows where it is, e.g. `LOOP 3/6`.
- Turning SONG on during playback starts the chain at the next loop end. You can still switch
  banks manually with SONG BANK + layer key.
- **Saved with the project.** Files from older firmware open with song mode OFF; drumkits are
  not affected. NEW FILE resets the chain.
- All parts share the song length (BAR × MEASURE). Use REPEAT for longer sections.

## 6. Melody layers – NOTE page in Layer Song (v1.09)

Any layer can now be a **DRUM** layer (as before) or a **TONE** layer that plays every beat at its
own note. Your kick becomes an 808 bassline, a vocal chop a melody, a pad sample a chord riff.

**Open the page:** **S + layer key**, then **S + the same layer key** twice more (edit → EUCLID →
NOTE). Once more goes back to the edit page.

```
 Top:     [1 MODE]   [2 NOTE]    [ note strip: a small piano roll ]
 Bottom:  [5 ROOT]   [6 SCALE]   [7 GLIDE]   [8 TRANSPOSE]
```

| Control | What it does |
|---|---|
| **1 MODE** | **DRUM**: the layer plays as before and ignores the notes. **TONE**: every beat plays at its note. |
| **2 NOTE** | The note of the selected beat, e.g. `D#4`. Only notes of the scale are offered, two octaves either side of the root. |
| **5 ROOT** | The key: C to B. |
| **6 SCALE** | Chromatic, major, minor, dorian, phrygian, lydian, mixolydian, harmonic minor, pentatonic major, pentatonic minor, blues. |
| **7 GLIDE** | 0 = off. Higher values slide from the previous note into the next one, from a quick 303-style slide up to half a second. |
| **8 TRANSPOSE** | Moves every note of the bank one scale step up or down. The value shows how far you have moved since the page opened. |
| **LEFT / RIGHT** | Select the beat whose note you edit (SHIFT + LEFT/RIGHT still moves it). |
| **Layer key** | Plays the selected beat's note. |
| **CLEAR** | Undoes the note edits made since the page opened (**CLEAR: UNDO**). |

**Workflow:** put beats on the layer (record, ADD, EUCLID…), open the NOTE page, set MODE to TONE,
pick ROOT and SCALE, then step through the beats with LEFT/RIGHT and turn NOTE. While the
sequencer is stopped, every turn plays the note so you hear what you set.

- **Always in key.** Notes are stored as steps of the scale, so changing ROOT or SCALE later
  re-harmonises the whole melody instead of leaving notes outside the key.
- **Tuning:** the sample is taken to be a **C** (shown as C4) at the layer's PITCH. If your
  sample is in another key, tune it with PITCH in the Layer Instrument menu.
- **The strip** shows every beat of the bank at its pitch (the line in the middle is the root);
  the selected beat is cyan.
- **Notes stay with their beats:** MOVE, SHIFT ALL, quantize, EUCLID MERGE and song copy/paste
  keep them. A short ADD press gives the new beat the note of the beat before it, so a line is
  quick to continue.
- **MIDI out:** a TONE layer sends each beat's own note (the sample's pitch is MIDI note 60) and
  releases the previous note first, so the Drumboy Pro can play a bassline on an external synth.
- **Saved with the project** in a small extra file (`File_NNN.rwn`) next to the project file.
  Projects from older firmware open with all layers as DRUM; older firmware ignores the new file.
  NEW FILE resets everything; drumkits don't store notes.
- **Good to know:** one note at a time per layer (use several layers for chords). As on any
  sampler, higher notes play the sample faster, so they are also shorter. Pads outside the NOTE
  page and MIDI in play the root.

---

## Improvements and fixes

### Audio CPU meter (v1.09)
- **IO menu, knob 5 CPU LOAD:** how busy the audio engine is, averaged over about 0.1 s.
- **Knob 6 CPU PEAK:** the busiest moment since start-up (turn the knob to reset). It turns red
  from 80 %; above 100 % the audio misses its deadline and you hear clicks.
- Useful to see how much room there is before adding effects, or, later, synth layers.

### Safer firmware updates (v1.02)
- The bootloader no longer starts a broken app after a failed update. It shows
  *NO VALID APPLICATION* and retries the package on the next power-on.
- Errors while writing flash show *FLASH-CHECK*; previously the misleading *FORMAT-CHECK* appeared.
- **Note:** the bootloader itself can only be updated with an ST-Link (SWD). An SD update only
  updates the app. Without an ST-Link the device keeps the factory bootloader, which is safe.

### Saving to the SD card (v1.02)
- If the card is full or a save fails, you get *save error* instead of a half-written file.
- System settings and knob mappings that failed to save are retried automatically.
- Instrument names longer than 25 characters work.
- Very short or corrupt WAV files no longer hang the device.

### Display and behaviour fixes
- Tempo and MIDI channel in the Global menu could be displayed incorrectly (v1.02).
- ADD + LEFT/RIGHT in Layer Song erased the fill of the beat and of the beat before it (v1.03).
- Clearing a layer from another menu, e.g. by pasting, could silently switch the menu state (v1.03).
- After quantizing, the last beat could end up out of order (v1.03).
- In the Global menu, REVERB DAMPING and PREDELAY showed each other's names (v1.03).

---

## Versions

| Version | Contents |
|---|---|
| v1.01 | Factory firmware from Randomwaves (`02 Software/Firmware/Drumboy_Pro_Fw_01_01.bin`) |
| v1.02 | Safer firmware updates, more reliable SD saving, fixes |
| v1.03 | Layer Song: MOVE, SHIFT ALL and single-beat ADD |
| v1.04 | Moffe Edition boot screen |
| v1.05 | Sample preview in the Layer Instrument menu |
| v1.06 | Layer Song EUCLID page: Euclidean rhythms |
| v1.07 | Song mode: chain the 5 song banks into a song |
| v1.08 | EUCLID page shows the CLEAR: UNDO hint |
| v1.09 | Melody layers (TONE) on the NOTE page; audio CPU meter in the IO menu |

## Install and roll back

The step-by-step guide (which file to download, where to put it, what the screen shows and what
to do if something goes wrong) is in the **[README → Install](README.md#install)**. In short:

1. Back up the whole SD card.
2. Download `Drumboy_Pro_Fw_Moffe_01_09.bin` from the
   [latest release](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/releases/latest).
3. Copy it, with its exact name, to the root of the card as the only `Drumboy_Pro_Fw…bin` there.
4. Power on: *UPDATING FIRMWARE V1.09* → *FIRMWARE UPDATED* → sunglasses.

**Roll back:** install `Drumboy_Pro_Fw_01_01.bin` (factory, in the release Assets and in
`02 Software/Firmware/`) the same way. The bootloader accepts any version, older or newer.

Moffe Edition packages have `Moffe` in their file name, so they can never be confused with an
official Randomwaves update (`Drumboy_Pro_Fw_MM_mm.bin`); the bootloader accepts both.

To build the package yourself: `cmake --build --preset firmware` (it ends up in
`build/firmware/`). See [DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>).

## For developers

- Command-line build, and CI that builds, tests and verifies every pull request.
- Host unit tests for the logic that needs no hardware (`02 Software/Tests`).
- `make_fw_package.py` builds and verifies SD packages with the same rules as the bootloader.
- `Controller.cpp` is split by subsystem; the real-time audio engine is in `ControllerAudio.cpp`.
- Rules and workflow for safe changes are in [DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>).
