# Drumboy Pro – Moffe Edition

Moffe Edition is **unofficial community firmware** for the Drumboy Pro, based on the open-source
[Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro) (factory firmware v1.01). It
is not affiliated with or endorsed by Randomwaves. This page collects everything that is new
compared with the factory firmware: what each feature does and how to use it on the device.

- **Current version:** v2.08 — download from the
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

> **Status (3 November 2026):** every version is built, unit-tested and verified by CI. Tested on
> a Drumboy Pro by the maintainer: song mode (v1.07); the melody layers with notes, GLIDE and
> TRANSPOSE, and the CPU meter (v1.09: CPU PEAK about 42 % with all 10 layers playing, two of them
> TONE, and chorus, delay, reverb and both filters on); the pad keyboard and the melody generator
> (v1.10); from v2.00, in its test builds: SYNTH layers, erasing on the EUCLID/NOTE pages, DETUNE,
> SUB, NOISE, DRIVE, GLIDE, WAVE = SAMPLE and SLIDE; AUTOSAVE and resume (v2.01); names for
> projects and drumkits (v2.02); song transpose (v2.03). **Not yet tested on a device:** the rest of v2.00 (presets,
> random sounds, AMP and LFO pages, PAD HOLD, VISUALS, the new look), v2.04 (harmony layers), v2.05 (FM, pitch envelope, filter attack), v2.06 and v2.07 (the new visuals), v2.08 (MIDI keyboard, arpeggiator). Each release says what has been tested on hardware;
> please report anything odd in [Issues](../../issues).

## What's new in v2.08 – play it from a keyboard, arpeggiate it

| | New | What it means for you |
|---|---|---|
| 🎹 | **[MIDI keyboard plays the synth](#12-midi-keyboard-and-arpeggiator-v208)** | While the pads are a keyboard (NOTE or ARP page, or a SYNTH layer's sound pages), a MIDI keyboard plays that layer: in its scale, with velocity, held as long as you hold the key, recorded with REC. |
| 🔁 | **[ARP page](#12-midi-keyboard-and-arpeggiator-v208)** | A new Layer Song page: every note of a TONE / SYNTH layer becomes an arpeggio – UP, DOWN, UP/DOWN or RANDOM, 1/8 to 1/32 with triplets, 1–3 octaves, five chord shapes, GATE. Harmony layers follow every arp note. |
| 🩹 | **Better MIDI in** | Notes sent with running status, and Note On with velocity 0 (how most keyboards end a note), are now understood; fast chords and releases are no longer lost. |

## What's new in v2.07 – more light show

| | New | What it means for you |
|---|---|---|
| 🌀 | **[GEISS, MILK TUNNEL](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | A smearing feedback plasma and a warping texture tunnel, full screen in chunky retro pixels. |
| 🤖 | **[ROBOT DJ, DISCO CROWD](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | A scratching, nodding, laser-eyed robot at the decks; dancers, a mirror ball and a stage-diver. |
| 🎼 | **[PIANO ROLL, KEY WHEEL](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | Your melodies and harmonies as notes; the circle of fifths following your key and song transpose. 20 styles in all. |

## What's new in v2.06 – a new light show

| | New | What it means for you |
|---|---|---|
| 📼 | **[CLASSIC](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | The late-90s media-player window: spectrum bars with peak dots, scope, marquee, time and buttons. Plus a **WINAMP** colour theme. |
| 😬 | **[MOUTH, EYEBALLS, SPEAKER](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | Teeth that are level meters, eyes that stare at the loudest layer, a woofer pumping on the kick. |
| 🌴 | **[DEMO SCENE, SUNSET DRIVE, SUPERSCOPE](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | Copper bars and sine scrollers, an 80s neon sunset, a tumbling 3D waveform. |
| 🎛️ | **[STEP GRID](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | Your actual pattern lighting up as it plays. |
| 🔀 | **AUTO + INFO** | ADD = a new style every 16 bars; CLEAR = project, tempo and bar on screen. 16 styles, 9 themes. |

## What's new in v2.05 – FM and synth drums

| | New | What it means for you |
|---|---|---|
| 🔔 | **[FM voice](#7-synth-layers--a-built-in-synth-voice-v200)** | WAVE = **FM**: DX-style e-pianos, bells, gongs, rubbery basses and metallic hits. New **MOD** page with FM RATIO, FM AMOUNT and FM DECAY. |
| 🥁 | **[PITCH ENV](#7-synth-layers--a-built-in-synth-voice-v200)** | Every note can start higher and drop to its pitch: 808 kicks, toms, zaps and lasers from the synth. |
| 🌅 | **[FILTER ATTACK](#7-synth-layers--a-built-in-synth-voice-v200)** | The filter can open slowly instead of jumping: wah swells and blooming pads. |
| 🎁 | **13 new presets** | `808 BOOM`, `BELL TOWER`, `DX PIANO`, `FM BRUISER`, `GONG SHOW`, `ICE PALACE`, `KALIMBA`, `LOG DRUM`, `METAL GURU`, `SLOW BLOOM`, `TOM TOM`, `WAH WAH`, `ZAP ATTACK` – 79 in all. Random sounds use FM and pitch drops too. |

## What's new in v2.04 – harmony layers

| | New | What it means for you |
|---|---|---|
| 🎶 | **[Harmony layers](#harmony-layers-v204)** | A melody layer can FOLLOW another and play every note it plays a 3rd, 4th, 5th, 6th or octave higher or lower, always in key: pads over basslines, chords from two layers. NOTE page, knob 4. |

## What's new in v2.03 – song transpose

| | New | What it means for you |
|---|---|---|
| 🎼 | **[TRANSPOSE per song step](#5-song-mode--arrange-parts-into-a-song-v107)** | Each step of the song chain can move all melody layers up or down the scale (ADD + knob 7 in the Rhythm menu): verse, chorus and bridge from one pattern, always in key. |

## What's new in v2.02 – give it a name

| | New | What it means for you |
|---|---|---|
| 🏷️ | **[Project and drumkit names](#11-names-for-projects-and-drumkits-v202)** | LOAD, SAVE and CLEAR show your projects and drumkits by name (`NEON YAK`, `DJ BLOOP`) instead of FILE_007. |
| 🎲 | **[Random names](#11-names-for-projects-and-drumkits-v202)** | Every new project and drumkit gets a funny synth-flavoured name; turn knob 8 for another one. |
| ✏️ | **[Your own name](#11-names-for-projects-and-drumkits-v202)** | Knob 6 changes a letter, knob 7 picks which one: up to 10 letters, digits and `! - + . ? #`. |

## What's new in v2.01 – never lose a session

| | New | What it means for you |
|---|---|---|
| 🛟 | **[AUTOSAVE](#10-autosave-and-resume-v201)** | The Drumboy saves your session by itself whenever you have changed something and then leave it alone for a moment (10 s when stopped, 30 s while playing; never while recording). On by default; File menu, knob 5. |
| ⏪ | **[Resume](#10-autosave-and-resume-v201)** | After a power cut, a freeze or simply switching off: at the next power-on the Drumboy asks **RESUME LAST SESSION?** – ADD = yes, CLEAR = no. |
| 🔒 | **[Crash-safe saving](#10-autosave-and-resume-v201)** | Every save (projects, drumkits, auto-save) now writes a complete new copy first and only then replaces the old one. A power cut in the middle of a save can no longer destroy the project you had. |
| 📂 | **[File menu starts at your project](#10-autosave-and-resume-v201)** | LOAD / SAVE / CLEAR start at the slot of the project you are working on instead of always at FILE_001. |

## What's new in v2.00 – the big one

The biggest update yet: the Drumboy Pro gets **a real synthesizer, a light show and a new look**.

| | New | What it means for you |
|---|---|---|
| 🎛️ | **[SYNTH layers](#7-synth-layers--a-built-in-synth-voice-v200)** | Any layer can be a synth: saw, square, triangle, sine or your own single-cycle WAVs, detune, sub oscillator, noise, a resonant filter (4 types) with envelope, DRIVE, slides and glide. Acid basslines, pads and leads without a single sample. |
| 🎁 | **[66 presets](#7-synth-layers--a-built-in-synth-voice-v200)** | From `ACID TRIP` to `ZOMBIE`, alphabetical, plus 16 USER slots for your own sounds. The track list shows each synth layer's sound by name. |
| 🎲 | **[Random sounds](#7-synth-layers--a-built-in-synth-voice-v200)** | Hold ADD: a brand-new sound that makes musical sense. Tap ADD: a small variation. Tap CLEAR: undo. |
| 📈 | **[AMP envelope and LFO](#7-synth-layers--a-built-in-synth-voice-v200)** | ATTACK, DECAY, SUSTAIN, RELEASE, and a tempo-synced LFO for wobbles, vibrato and tremolo. |
| 🎹 | **[PAD HOLD](#7-synth-layers--a-built-in-synth-voice-v200)** | Notes sound as long as you hold the pad, like a keyboard, and recorded notes play back just as long. |
| 🌈 | **[VISUALS](#8-visuals--full-screen-music-visuals-v200-all-new-line-up-in-v206)** | 13 full-screen visuals that dance to your music: fireworks for every hit, a Game of Life seeded by your drums, a spectrum skyline, digital rain, a kaleidoscope and more. Press HOME on the home screen. |
| 🏆 | **[The Moffe Edition look](#9-the-moffe-edition-look-v200)** | A gold-and-orange "Gold Rush" style throughout, and a new golden boot screen. |

Coming from v1.10 or older? Also new since then: the [NOTE page](#6-melody-layers--note-page-in-layer-song-v109)
plays pads without touching your track (ADD + pad = step entry) and has a wider keyboard range,
per-beat **SLIDE**, and synth sounds travel with **drumkits** and the instrument **copy/paste**.
Every detail per version: [CHANGELOG.md](CHANGELOG.md).

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
appears letter by letter, and **EDITION** fades in below. Since v2.00 it wears the
[Moffe Edition look](#9-the-moffe-edition-look-v200): MOFFE in gold with a deep ruby drop shadow,
a gold rim on the sunglasses and a gold line under a ruby EDITION (before v2.00:
large magenta letters).

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
| **CLEAR** (tap) | Restores the beats the layer had when you opened the page |
| **CLEAR** (hold) | Erases all of the layer's beats in this bank, as on the edit page (v2.00). The page shows **CLEAR: UNDO  HOLD: ERASE** as a reminder. |

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
| **7 PART** | The part (song bank 1–5) that step plays, or **END** to end the chain there. **ADD + knob 7** = that step's **TRANSPOSE** (v2.03), shown as e.g. `2  KEY+3` |
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

**TRANSPOSE – a chord progression from one pattern (v2.03).** Hold **ADD** and turn **knob 7**:
the step's melody layers (TONE and SYNTH) move up or down by that many **notes of their scale**
(−7…+7) while the step plays. Drum layers are not touched, and your notes are not changed – the
move is applied as they play.

- **Example:** one bassline in C major, chain *part 1 ×2 (KEY+0) → part 1 ×2 (KEY+3) → part 1 ×2
  (KEY+4) → part 1 ×2 (KEY+0)*: the classic I–IV–V–I, all in key.
- Steps in scale notes keep everything in the scale, so it can't sound off-key; a 3rd up in a
  minor scale stays minor.
- It changes exactly at the loop end, with the part. A note still ringing keeps its pitch.
- Pads played along while the chain runs follow the step's transpose too, and what you record
  then moves along with the song.
- Only while song mode plays the chain: stopped, or with SONG OFF, everything is at its written
  pitch. MIDI out sends the transposed notes.
- Saved with the project; older projects have no transpose.

## 6. Melody layers – NOTE page in Layer Song (v1.09)

Any layer can now be a **DRUM** layer (as before) or a **TONE** layer that plays every beat at its
own note. Your kick becomes an 808 bassline, a vocal chop a melody, a pad sample a chord riff.

**Open the page:** **S + layer key**, then **S + the same layer key** twice more (edit → EUCLID →
NOTE). Once more opens the ARP page (v2.08, [section 12](#12-midi-keyboard-and-arpeggiator-v208)),
and once more goes back to the edit page.

```
 Top:     [1 MODE]   [2 NOTE]    [ note strip: a small piano roll ]
 Bottom:  [5 ROOT]   [6 SCALE]   [7 GLIDE]   [8 TRANSPOSE]
```

| Control | What it does |
|---|---|
| **1 MODE** | **DRUM**: the layer plays as before and ignores the notes. **TONE**: every beat plays its sample at its note. **SYNTH**: every beat plays the built-in synth at its note ([section 7](#7-synth-layers--a-built-in-synth-voice-v200), v2.00). |
| **2 NOTE** | The note of the selected beat, e.g. `D#4`. Only notes of the scale are offered, five octaves either side of the root (two before v2.00). |
| **4 FOLLOW** (no label) | **Harmony** (v2.04): turn to make this layer follow another one (OFF, L1…L10) – see *Harmony layers* below. **ADD + knob 4** picks the interval. The strip shows e.g. `FOLLOW L3  3RD UP`. |
| **5 ROOT** | The key: C to B. |
| **6 SCALE** | Chromatic, major, minor, dorian, phrygian, lydian, mixolydian, harmonic minor, pentatonic major, pentatonic minor, blues. |
| **7 GLIDE** | 0 = off. Higher values slide from the previous note into the next one, from a quick 303-style slide up to half a second. |
| **8 TRANSPOSE** | Moves every note of the bank one scale step up or down, keeping the melody intact. It stops when the highest or lowest note reaches the edge of the range. The value shows how far you have moved since the page opened. |
| **LEFT / RIGHT** | Select the beat whose note you edit (ADD + LEFT/RIGHT still moves it). |
| **The 10 pads** (TONE, SYNTH) | A scale keyboard: pad 1 is the root, the others the next notes of the scale. Stopped: the pads only **play**, so you can try sounds without changing the track. Playing: play along; with REC, record. (v1.10; v2.00: pads alone no longer edit while stopped) |
| **ADD + pad** (stopped) | Step entry: hold ADD and tap pads; each pad gives the selected beat its note and selects the next beat. The strip shows `ADD+PAD: STEP` as a reminder. (v2.00) |
| **UP / DOWN** (TONE, SYNTH) | Move the pad keyboard an octave down or up: 4 octaves down to 3 up (v2.00; was 2 down, 1 up). The strip shows the range, e.g. `PADS C4-E5`. (v1.10) |
| **ADD + UP / DOWN** | **SLIDE** on / off for the selected beat: the note slides in from the previous one (303 style). The NOTE value shows `SLIDE` and the strip joins the two notes. On SYNTH layers the notes join without a new attack; slide time is GLIDE (at least ~36 ms). (v2.00) |
| **ADD hold** | A new melody over the bank's beats, in the scale, starting and ending on the root. It comes when you **let go** of ADD (v2.00), and not at all if you used ADD with a pad or UP/DOWN meanwhile. (v1.10) |
| **ADD tap** | A variation: a few notes move a step or two. Tap again and again to let it evolve. (v1.10) |
| **CLEAR** (tap) | Undoes the note edits made since the page opened, including generated melodies. |
| **CLEAR** (hold) | Erases all of the layer's beats in this bank, as on the edit page (v2.00). To remove a single beat, use a CLEAR tap on the edit page. |

**Workflow:** put beats on the layer (record, ADD, EUCLID…), open the NOTE page, set MODE to TONE
and pick ROOT and SCALE. Then either:

- **tap the notes in:** with the sequencer stopped, hold **ADD** and press the pads one after
  another; each pad gives the selected beat its note and moves to the next beat (pads without
  ADD only play, for trying sounds);
- **let it write one:** hold **ADD** for a new melody, tap **ADD** to vary it, **CLEAR** to go back;
- **play it in:** press PLAY and REC and play the pads like a keyboard;
- **fine-tune:** select a beat with LEFT/RIGHT and turn NOTE (while stopped, you hear each step).

A quick bassline: EUCLID page E(5,16), then the NOTE page, MODE TONE, hold ADD until you like it.

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
- **Good to know:** one note at a time per layer (use several layers – or harmony layers – for chords). As on any
  sampler, higher notes play the sample faster, so they are also shorter. Outside the NOTE page
  the pads play the root; on the NOTE page of a TONE layer the pads only play that layer (go back
  to the edit page to play the drums). MIDI in plays the layer like the pads do from v2.08 – see
  [section 12](#12-midi-keyboard-and-arpeggiator-v208).

### Harmony layers (v2.04)

A **harmony layer** plays along with another melody layer – the **leader** – a chosen interval
higher or lower, always in key: a pad a 3rd above your bassline, a 5th for a power riff, or two
harmony layers for full chords. The leader stays exactly as it is.

#### Before you start

- **The leader** (e.g. layer 3): MODE **TONE** or **SYNTH** on its NOTE page, and beats with
  notes – record them, step them in with ADD + pads, or hold ADD on the NOTE page for a melody.
- **The harmony layer** (e.g. layer 4): MODE **TONE** (a sample with a clear pitch) or **SYNTH**,
  with its own sound. For SYNTH: Layer Instrument (**I + layer key**), SOUND page, knob 1
  **PRESET** – soft sounds like `WARM HUG` sit nicely over a bass. It needs no beats of its own.
  (A DRUM layer can't harmonise: its strip shows `DRUM: MODE TO TONE`.)
- The NOTE page: Layer Song (**S + layer key**), then press the layer key until the NOTE page
  shows (edit page → EUCLID → NOTE).

#### Make a layer follow

On the **harmony layer's** NOTE page:

```
 Top:     [1 MODE]  [2 NOTE]   [3 –]     [4 –]          knob 4 = FOLLOW (no label)
 Bottom:  [5 ROOT]  [6 SCALE]  [7 GLIDE] [8 TRANSPOSE]
```

1. **Turn knob 4** to the right: OFF → L1 → L2 → … → L10 (it skips the layer itself). Stop at the
   leader, e.g. **L3**. Turning left goes back towards OFF.
2. **Look at the note strip** (the graph area at the top right): it shows `FOLLOW L3  3RD UP` in
   gold, draws the leader's melody at the pitches this layer will play, and its title shows the
   leader's key (e.g. `NOTES  C MINOR`). The NOTE value turns grey: this layer's own notes are
   not used while it follows.
3. **Hold ADD and turn knob 4** to choose the interval (the list wraps around). Using ADD here
   doesn't make a new melody when you let go.

   | Interval | Sound |
   |---|---|
   | **3RD UP** (default) | The classic sweet harmony; major or minor, whichever the key gives |
   | **4TH UP** | Open, "suspended" |
   | **5TH UP** | Strong and powerful (power chord) |
   | **6TH UP** | Soft, a little nostalgic |
   | **OCT UP** | The same melody an octave higher: doubles the line |
   | **3RD DN … 6TH DN** | The same intervals below the melody |
   | **OCT DN** | The same melody an octave lower: a fat sub under a lead |

4. **Press PLAY.** The harmony layer plays every note of the leader, on exactly the same beats,
   at the interval.

To stop, turn knob 4 back to **OFF**: the layer plays its own beats again (they were kept, only
ignored).

#### How it behaves

- **Always in key.** The interval is counted in the leader's scale, and the harmony layer plays
  in the **leader's ROOT and SCALE**: a 3rd above C in C major is E (major), above D it is F
  (minor). Change the leader's ROOT or SCALE and the harmony follows. The harmony layer's own
  ROOT and SCALE are ignored while it follows (and kept for when FOLLOW is off).
- **Same rhythm.** Beats, fills, accents (FLAT / GHOST), slides and notes recorded with PAD HOLD
  are all followed.
- **Live.** Change a note, tap ADD for a variation, record new notes: the harmony follows
  immediately. Nothing is copied.
- **Pads.** On the **leader's** NOTE page the pads play the leader *and* every harmony layer:
  chords with one finger. With PAD HOLD on the leader the harmony holds too, and stops when you
  let go. On the harmony layer's own NOTE page the pads play only that layer (in the leader's
  key).
- **Its own sound and mix.** Level, PITCH, sound or sample, GLIDE, the EQ / filter / effect /
  reverb sends and probability are the harmony layer's own, so you can mix it separately.
- **Mute.** MUTE on the harmony layer silences just the harmony; MUTE on the leader silences both
  (the harmony is triggered by the leader).
- **Song TRANSPOSE** ([section 5](#5-song-mode--arrange-parts-into-a-song-v107)) moves the leader
  and its harmonies together along the scale.

#### Chords: several harmony layers

Any number of layers can follow the same leader.

- **Major / minor triads:** layer 4 FOLLOW L3 **3RD UP**, layer 5 FOLLOW L3 **5TH UP**: every
  bass note becomes a full chord in the key.
- **Thick lead:** a second synth on **OCT UP** with a different preset.
- **Wide pad:** **3RD UP** and **6TH UP** on two soft pads.

#### Recipes

1. **Bass + pad in two minutes:** layer 3 SYNTH `ACID TRIP`, EUCLID page E(5,16), NOTE page hold
   ADD for a melody. Layer 4 SYNTH `WARM HUG`, FOLLOW L3, 3RD UP; turn layer 4's level down a bit.
2. **Power riff:** a TONE layer with a guitar or synth stab, a second layer following it on
   5TH UP, a third on OCT DN.
3. **A chord progression:** bass + 3RD UP + 5TH UP as above, then song mode LOOP in the Rhythm
   menu with steps KEY+0 → KEY+3 → KEY+4 → KEY+0 (ADD + knob 7): a full I–IV–V–I from one
   bassline.

#### Good to know

- **Scales with 5 or 6 notes** (pentatonic, blues): the intervals are the scale's own next notes
  ("3RD UP" = two scale notes up), so they always fit the scale. In **CHROMATIC** the steps are
  semitones; choose a 7-note scale for classic harmony.
- **Range:** at the top or bottom of the note range the harmony note stops at the edge instead
  of jumping an octave.
- **One level only:** a harmony layer can't itself be a leader (a layer following it stays
  silent).
- **Saved with the project** (in the `.rwn` file) and restored on load and by AUTOSAVE resume;
  NEW FILE clears it. Drumkits and the instrument copy/paste don't store it. Older firmware plays
  the layer's own beats.
- Switching the harmony layer to DRUM keeps its FOLLOW setting but switches it off; it comes back
  with TONE or SYNTH.

#### No harmony to hear?

- Both layers on **TONE or SYNTH**, and the leader has beats?
- The harmony layer has a sound (a loaded sample for TONE, a sound for SYNTH) and its level isn't 0?
- Neither layer muted?
- The harmony layer's strip shows `FOLLOW Lx …` in gold, and Lx isn't itself following another
  layer?
- No sound at all? Check **MASTER MUTE** in the Mixer menu (select it and hold ADD to unmute).

## 7. SYNTH layers – a built-in synth voice (v2.00)

Set **MODE** on the NOTE page to **SYNTH** and the layer becomes a monophonic subtractive synth:
no sample needed. It plays its notes exactly like a TONE layer (scale, GLIDE, TRANSPOSE, pad
keyboard, melody generator) and goes through EQ, filters, effects, reverb and the mixer like any
other layer.

**The sound** is set in the Layer Instrument menu (**I + layer key**), which shows synth pages for
a SYNTH layer. Press the layer key again to go to the next page. On all synth pages the **10 pads
are a scale keyboard** for the synth, so you can play it while shaping the sound (v2.00; same
scale and octave as on the NOTE page, where UP/DOWN sets the octave):

```
 Page 1 SOUND:   [1 PRESET]    [2 SAVE TO]    [ preset name, hints ]           (v2.00)
 Page 2 OSC:     [1 WAVE]      [2 DETUNE]     [ waveform ]
                 [5 LEVEL]     [6 PITCH]      [7 SUB]        [8 NOISE]
 Page 3 MOD:     [1 FM RATIO]  [2 FM AMOUNT]  [ FM wave, pitch drop ]        (v2.05)
                 [5 FM DECAY]  [6 PITCH ENV]  [7 PITCH DEC]  [8 FILT ATK]
 Page 4 FILTER:  [1 CUTOFF]    [2 RESO]       [ filter envelope + type ]
                 [5 ENV AMT]   [6 DECAY]      [7 FILT TYPE]  [8 DRIVE]
 Page 5 AMP:     [1 ATTACK]    [2 AMP DECAY]  [ level envelope ]               (v2.00)
                 [5 SUSTAIN]   [6 RELEASE]    [7 GATE]       [8 PAD HOLD]
 Page 6 LFO:     [1 LFO SHAPE] [2 LFO RATE]   [ LFO wave + position ]          (v2.00)
                 [5 LFO DEPTH] [6 LFO TARGET] [7 LFO RETRIG]
 Page 7:         routing to EQ / FILTER / EFFECT / REVERB, as for samples
```

**Sounds in three ways** (v2.00):

| | What it does |
|---|---|
| **PRESET** (SOUND page, knob 1) | 79 factory sounds in **alphabetical order**, with names to match their egos: `808 BOOM` (kick), `ACID TRIP`, `ALIEN SPA`, `BAD PENNY`, `BANSHEE`, `BEE SWARM`, `BELL TOWER` (FM), `BIG FUZZ`, `BLIP BLOOP`, `BOOMSTICK` (kick), `BRASS GOD`, `BUBBLEGUM`, `CAT WALK`, `CHURCH BOI`, `CLOUD NINE`, `COCONUTS`, `COIN GET`, `COWBELL!`, `DAD STEP`, `DARK LORD`, `DEEP SPACE`, `DING DONG`, `DISCO DUCK`, `DIZZY BOT`, `DRAGONROAR`, `DX PIANO` (FM), `EARTHQUAKE`, `EMPTY MIND` (the default), `EPIC MOVIE`, `FAIRY DUST`, `FM BRUISER` (FM bass), `FOGHORN`, `GHOST TOWN`, `GLASS HARP`, `GOLD RUSH`, `GONG SHOW` (FM), `HONKY BONK`, `HYPERDRIVE`, `ICE PALACE` (FM), `JELLYFISH`, `KALIMBA` (FM), `KAZOO KING`, `LASER CAT`, `LOG DRUM`, `LOUNGE LIZ`, `METAL GURU` (FM), `MOON BASS`, `MORDOR`, `NINJA STAR`, `NOODLE ARM`, `OCTOPUS`, `PANIC BOT`, `PEW PEW`, `PIXEL HERO`, `R2 DEEDOO`, `ROBO CHOIR`, `RUBBERDUCK`, `SAWSOME`, `SEASICK`, `SLOW BLOOM`, `SNARE WARS`, `SPACE TAXI`, `SQUARE GUY`, `STARGATE`, `STORMBORN`, `SUNNY DAY`, `THICC BASS`, `TOM TOM`, `TOY PIANO`, `TSS TSS` (hi-hat), `TUMBLEWEED`, `UFO LANDED`, `VIKING`, `WAH WAH`, `WARM HUG`, `WUBZILLA`, `ZAP ATTACK`, `ZEN PLUCK`, `ZOMBIE` – then your own sounds, `USER 01`…`USER 16`. Turning plays a note so you hear each one. (79 since v2.05: 13 new FM, pitch-envelope and filter-attack sounds; 66 in v2.00, 32 before.) |
| **Track list name** | A SYNTH layer shows its sound in the layer list on the main screen, like a sample name: the preset name, `USER 03` for a saved sound, a `*` after the name once you have changed it, or `SYNTH` for a sound of its own (e.g. a random one). Also after loading a project or drumkit. (v2.00) |
| **ADD** on the SOUND page | Saves the current sound into the USER slot shown by **SAVE TO** (knob 2; `*` = slot already used, it is overwritten). Kept on the SD card (`System/Preset/SynthUser.rws`) for all projects. |
| **ADD hold** on OSC / FILTER / AMP / LFO | A **random sound** when you let go: a bass, lead, pluck, pad or effect, always in useful ranges. |
| **ADD tap** on those pages | A **variation**: three settings move a little, now and then the waveform changes. Tap again and again. |
| **CLEAR tap** on any synth page | **Undo**: back to the sound the layer had when you opened the Layer Instrument menu (presets, random sounds and edits alike). CLEAR hold still resets the layer. |

ADD used together with a knob (LFO on LEVEL/PITCH) or UP/DOWN never changes the sound.

| Control | What it does |
|---|---|
| **WAVE** | Saw (bright, the classic bass), square (hollow), triangle (soft), sine (pure, sub-bass), **SAMPLE** (v2.00): the layer's loaded sample as one cycle of the waveform – see below, or **FM** (v2.05): two-operator FM, set on the MOD page – e-pianos, bells, FM basses, metal. |
| **DETUNE** | 0 = one oscillator. Higher values use two, spread up to 40 cents below and above the note (it stays in tune): from a slight thickening to a wide, chorus-like beating. (v2.00) |
| **LEVEL / PITCH** | As for samples, including their LFOs. PITCH 1.00 = the notes as shown on the NOTE page. |
| **SUB** | Adds a square wave one octave below: more weight for basses. (v2.00) |
| **NOISE** | Adds white noise: breath, grit, hi-hat-like textures with a short GATE. (v2.00) |
| **CUTOFF** | How open the filter is when no envelope is applied. |
| **RESONANCE** | The peak at the cutoff: from smooth to squelchy. |
| **ENV AMT** | How far each note opens the filter. With a low CUTOFF this gives the "wow" of acid basslines. |
| **DECAY** | How fast the filter closes again after each note, from 20 ms (a pluck) to 2 s (a slow sweep). |
| **FILT TYPE** | **LP 24** (classic, the default), **LP 12** (gentler, brighter), **BAND** (nasal, vocal, telephone) or **HIGH** (thin, airy; good for hats and noise). (v2.00) |
| **DRIVE** | Saturation in and after the filter: 0 = clean, 25–50 warm and thick, 75–100 loud and square-ish. It also makes the sound louder, so lower LEVEL a bit at high values. |
| **ATTACK** | How the note fades in: 1 ms (a click-free hit) to 3 s (a slow swell). (v2.00) |
| **AMP DECAY** | How fast the level falls from the top to SUSTAIN while the note is held: 5 ms to 5 s. (v2.00) |
| **SUSTAIN** | The level while the note is held, after the decay. 100 = full (the old behaviour), 0 = a pluck that dies away. (v2.00) |
| **RELEASE** | How long the note rings on after it ends: 5 ms to 5 s. Long values give pads and bells their tail. (v2.00) |
| **GATE** | How long each note is held: a share of the time until the next beat. Low = short staccato, 100 = notes run into each other. (On the AMP page since v2.00.) |
| **PAD HOLD** | **OFF**: a pad plays a short note, as before. **ON**: a pad note sounds **as long as you hold the pad** and goes into its RELEASE when you let go, like a keyboard. Pressing another pad switches to that note. Also on the NOTE page (knob 3, shown as `HOLD` in the strip), and for TONE layers there: the sample plays while held and fades out on release (it still stops at the end of the sample). Presets and random sounds keep it as it is. **Recording** with PAD HOLD on stores how long you held each pad, so the song plays the notes back just as long (not cut by GATE). (v2.00) |
| **LFO SHAPE** | SINE, TRIANGLE, SAW DOWN, SQUARE or RANDOM (a new random value every cycle: the classic "sample & hold" bleeps). (v2.00) |
| **LFO RATE** | How long one LFO cycle lasts, locked to the tempo: 1/32 to 1/2 (with 1/8T and 1/4T triplets), or 1, 2, 4, 8 bars. (v2.00) |
| **LFO DEPTH** | How strongly it moves its target. **OFF (0) by default**: turn this up first. (v2.00) |
| **LFO TARGET** | **CUTOFF** (wobble/sweep, up to 3 octaves each way), **PITCH** (vibrato: gentle at low depth, up to an octave each way at 100) or **LEVEL** (tremolo, down to silence at 100). (v2.00) |
| **LFO RETRIG** | **FREE**: the LFO runs on by itself, across notes. **NOTE**: it starts again with every note (not on a SLIDE): with SAW DOWN on CUTOFF every note gets its own filter sweep. (v2.00) |
| **FM RATIO** (MOD page) | WAVE = **FM** only. The modulator's frequency as a multiple of the note: whole numbers (`1:1`, `2:1`, `3:1` …) sound harmonic – pianos, organs, basses; `1.41`, `2.76` and `3.5` are inharmonic – bells, gongs, metal. (v2.05) |
| **FM AMOUNT** | WAVE = FM only. How deep the modulation goes: 0 = a pure sine, higher = brighter and more complex, up to harsh and metallic. Accents (FLAT beats) are a little brighter than GHOST ones. (v2.05) |
| **FM DECAY** | How fast that brightness fades after the note starts: short = a percussive "ding" (e-piano, bell), long = it stays bright, **HOLD** (100) = it never fades. (v2.05) |
| **PITCH ENV** | Any wave: each note starts higher (up to 4 octaves) and drops to its pitch – synth kicks and toms, zaps and lasers. 0 = OFF. (v2.05) |
| **PITCH DEC** | How fast that drop is: 2 ms (a click of pitch, punch) to 1 s (a slow dive). (v2.05) |
| **FILT ATK** | The filter envelope **rises** instead of jumping open: slow "wah" swells and pads that bloom. 0 = OFF (opens at once, as before). (v2.05) |

- **Your own waveforms (WAVE = SAMPLE):** put single-cycle WAVs (one period of a waveform,
  typically 600 samples; free collections exist online) in an instrument folder on the card. Load
  one on the layer while it is in TONE or DRUM mode (Layer Instrument, INST/SAMPLE, ADD), switch
  MODE to SYNTH and set WAVE to SAMPLE. The whole sample is played as one cycle at the note's
  pitch, so a long sample sounds like a buzzy, time-squeezed texture: fun, but single cycles
  sound cleanest. Very bright cycles alias at high notes.
- **SLIDE** (v2.00): on the NOTE page, select a beat and press **ADD + UP**; it slides in from
  the beat before it. Two or three slides in a line give the classic acid "wow".
- **Accent:** the beat's **VELOCITY** on the edit page decides it. FLAT beats play full and
  bright, GHOST beats softer and with less filter: put a few FLAT beats between GHOST beats for a
  classic accented line.
- **MIDI out** sends the notes with their real length (Note Off when the gate ends).
- **A first acid bass:** EUCLID E(7,16) on a layer → NOTE page: MODE SYNTH, ROOT A, SCALE MINOR,
  hold ADD → Layer Instrument: PRESET `ACID TRIP`. Then turn CUTOFF and RESONANCE on the FILTER
  page while it plays.
- **A filter wobble** (v2.00): on the synth layer's LFO page (I + layer key, press the layer key
  four more times) set LFO SHAPE SINE, LFO RATE 1/4, LFO TARGET CUTOFF and turn **LFO DEPTH** up. The
  wave on screen moves with it. CUTOFF on the FILTER page sets the centre it swings around. For a
  dubstep-style wobble try 1/8 or 1/8T; for RANDOM bleeps, SHAPE RANDOM at 1/16.
- The global LFOs in the LFO menu still work on LEVEL and PITCH as before (hold ADD and turn the
  knob); the synth's own LFO is separate and needs no setup elsewhere.
- **Saved with the project** (in the `.rwn` file). Older Moffe versions open the project with the
  layer as a DRUM layer and its notes intact. CLEAR (hold) in the Layer Instrument menu resets the
  sound.
- **Drumkits and the instrument copy** (v2.00) include MODE and the whole synth sound, so a saved
  kit brings its synth basses back and a pasted synth layer is the same synth. Kits saved before
  v2.00 don't change the layers' MODE or synth sound when loaded.
- **CPU:** about 1–2 % per playing synth layer; check the IO menu's CPU meter.

---

## 8. VISUALS – full-screen music visuals (v2.00, all-new line-up in v2.06)

The whole screen becomes a light show that follows the music. You can keep playing: pads,
PLAY/STOP/REC, MUTE/FILL/STYLE + pad and the home-screen knobs all work while it runs.

| Key | What it does |
|---|---|
| **HOME** (centre key) on the home screen | VISUALS on. (HOME in any other menu still goes to the home screen, as before.) |
| **LEFT / RIGHT** | The previous / next style (20). |
| **UP / DOWN** | The colour theme: GOLD RUSH (the default), SYNTHWAVE, ACID, OCEAN, FIRE, ICE, RAINBOW, **WINAMP** (classic green-yellow-red) and **C64** (retro blue-purple). |
| **ADD** | **AUTO** on / off (v2.06): a new random style every 16 bars, exactly on the bar (every 20 s when stopped) – like the old media-player visualisers. |
| **CLEAR** | **INFO** on / off (v2.06): project name, tempo, bar.beat and song step in the bottom-right corner. |
| **HOME** | VISUALS off, back to the home screen. |
| **Any menu key** (right-hand panel) | Leaves VISUALS and opens that menu. |

MUTE / FILL / STYLE + ADD or CLEAR keep their normal meaning (e.g. MUTE + ADD mutes all). A line
at the top shows the style, the theme, `[AUTO]` and the keys for a few seconds after each change.

| Style | What you see |
|---|---|
| **1 CLASSIC** | A tribute to the 1998 media-player look: a bevelled grey window with a ribbed `MOFFE AMP` title bar, an LCD with the play time, a scrolling marquee (project, tempo, bank), L/R level bars and an oscilloscope, a big 32-bar spectrum analyser with falling peak dots, a position bar, transport buttons, SHUFFLE (= AUTO) and REPEAT (= song loop) lights and a volume slider. Try it with the WINAMP theme. (v2.06) |
| **2 SHOWTIME** | Everything at once: ten layer lights that flash on every hit, VU columns at both edges, a spectrum skyline with falling caps, an oscilloscope across the horizon and a neon floor rolling towards you on the beat. |
| **3 SUPERSCOPE** | The waveform as a glowing 3D ribbon that slowly tumbles; every bar it takes a new shape – ring, spiral, figure eight, wave carpet – and the bass pumps its size. (v2.06) |
| **4 GEISS** | A feedback plasma in the spirit of the classic visualiser plug-ins: each frame the last picture is zoomed, turned, blurred and faded, and the waveform, the hits and a ring on every bar are painted into it – everything smears into flowing, colour-cycling trails. Chunky 6 x 6 pixels, full screen. (v2.07) |
| **5 MILK TUNNEL** | Flying down a warping, colour-cycling texture tunnel: the kick pushes you forward, the hi-hats twist it, it breathes with the music and flashes on every bar. (v2.07; replaces TUNNEL) |
| **6 MOUTH** | A huge cartoon mouth: the jaw opens with the bass, the ten upper teeth are the layers' meters (they stretch with each layer and flash on its hits), the ten lower teeth are the spectrum, and the tongue wobbles with the music. (v2.06) |
| **7 EYEBALLS** | Ten eyes, one per layer: they pop wide on their hits, all stare at the loudest layer, the pupils widen with the bass, they blink in quiet parts and turn **bloodshot** while the limiter works. (v2.06) |
| **8 SPEAKER** | A big woofer pumping on the kick (the cone rings and dust cap push out), glitter flying off on accents, the tweeter fizzing with the highs and the bass port puffing rings. (v2.06) |
| **9 ROBOT DJ** | A pixel robot behind two turntables and a mixer: it nods on the beat, its chest light thumps with the bass, the mixer meters dance, it scratches on big hits, throws its arms up on a loud bar and fires laser eyes on accents. (v2.07) |
| **10 DISCO CROWD** | A mirror ball throwing sparkles, eight pixel dancers (one per layer) jumping on their hits and waving with the hi-hats, a stage-diver surfing over them every four bars, and the light-up floor flashing under their feet. (v2.07; replaces DISCO) |
| **11 DEMO SCENE** | A home-computer demo: copper bars rolling through the middle (wider with the bass), a sine-wave text scroller with your project and tempo, a parallax starfield and a chrome MOFFE logo bouncing on the beat. (v2.06) |
| **12 SUNSET DRIVE** | An 80s neon sunset: violet-to-orange sky with twinkling stars, a striped sun that breathes with the bass while its stripes slide down, mountain silhouettes, palm trees and a neon grid road rolling towards you on the beat. (v2.06) |
| **13 STEP GRID** | Your pattern itself: ten rows (layers) of steps across the bank, a play head sweeping along and every step lighting up as it plays (muted layers dimmed). (v2.06) |
| **14 PIANO ROLL** | Every note of the bank's melody layers (TONE / SYNTH) at its pitch and length across the loop in its layer's colour, harmony layers as outlined notes beside their leader. Notes light up as the play head passes; a song TRANSPOSE moves them all and shows the step. (v2.07) |
| **15 KEY WHEEL** | The circle of fifths – major keys outside, their minors inside – with the song's key lit, a needle that glides to the chord the song is on (song TRANSPOSE walks it round, e.g. I → IV → V), the chord named underneath, and a heart beating with the bass. The key comes from the first melody layer. (v2.07) |
| **16 SPECTRO** | A spectrogram painting itself across the screen: time left to right, pitch bottom to top, loudness as heat. Ticks underneath mark the beats (white = a new bar). |
| **17 KALEIDO** | A kaleidoscope: the waveform and the spectrum mirrored into eight wedges, slowly turning, with a heart that beats with the bass. (Called MANDALA before v2.06.) |
| **18 ORBIT** | The ten layers as planets circling a core that breathes with the bass. Every hit sends a ripple out of its planet and a flash of lightning to the core; the orbit turns one planet per bar. |
| **19 RIDGES** | The spectrum's recent past as stacked mountain ridges, like the famous album cover: the newest line in front. |
| **20 LIFE** | Conway's Game of Life, one generation per eighth note. Every hit drops a glider into its layer's part of the world (an accent: a wild R-pentomino); newborn cells flash white. |

v2.06/v2.07 retired PHASE (now SUPERSCOPE), SPECTRUM (now CLASSIC), METERS (now MOUTH), RAIN and
FIREWORKS (now DEMO SCENE and SUNSET DRIVE), TUNNEL (now MILK TUNNEL) and DISCO (now DISCO CROWD).

- The visuals follow the sequencer while it plays (floor, road, orbit, tunnel, logo bounce and beat
  ticks lock to the tempo) and the TEMPO setting when it is stopped.
- An alert (e.g. a save question) switches VISUALS off so you can answer it.
- The style, theme, AUTO and INFO are kept until power-off.
- **CPU:** the audio engine only copies its output for the visuals (well under 1 %); the drawing
  runs between the other work, about 30 frames per second.

---

## 9. The Moffe Edition look (v2.00)

The whole interface wears a warm **"Gold Rush"** palette instead of the factory cyan and
magenta:

- **Gold** (#FFC21A) for knob names, the play bar, active values and highlights.
- **Light gold** for the selected layer's row.
- **Orange** (#FF8A1F) where the factory used magenta: the **KNOB** and **SONG** labels at the
  bottom of the screen and indicators such as MUTE and the rhythm locks.
- **Gold play button**, and the play bar in gold.
- **Ruby** for the selected beat in Layer Song, with a wider marker, so you can always see
  where you are on the line (the layer's other beats are light gold).
- **Boot screen:** MOFFE in gold with a deep ruby drop shadow, a gold rim on the sunglasses, and
  a gold line under a ruby EDITION (see [the boot screen](#2-moffe-edition-boot-screen-v104)).
- **VISUALS** starts in the matching GOLD RUSH colour theme.

Nothing to install for it: the firmware repaints the parts of the SD card's panel images that
were coloured, so any card works. The factory test screens keep their original colours. For
developers: the colours are `kAccentColor`, `kAccent2Color` and `kLayerSelectedColor` in the
app's `Global.h`.

---

## 10. AUTOSAVE and resume (v2.01)

Pull the plug, trip over the cable, or have the Drumboy freeze: your session is still there.

**How it works**
- Whenever you have **changed something** (any key or knob) and then **leave the Drumboy alone**
  for a moment, it saves the whole session to the SD card by itself:
  - **stopped:** after 10 seconds without touching anything;
  - **playing:** after 30 seconds without touching anything (a save pauses the screen and the
    buttons for a split second, the music keeps playing);
  - **never while recording**, never while a question is on the screen, and at most every 30
    seconds.
- The auto-save is its own file, `System/File/Autosave.rwf` (and `.rwn` for notes and synth
  sounds). It **never overwrites your projects** FILE_001…FILE_100 – those change only when you
  press SAVE yourself.
- At the next **power-on** the Drumboy asks **RESUME LAST SESSION?**
  - **ADD = yes:** the auto-saved session is loaded, exactly as it was (it does not start
    playing). SAVE in the File menu then offers the slot the project came from.
  - **CLEAR = no:** you start fresh. The question is not asked again until something new has been
    auto-saved.
- It asks instead of loading by itself on purpose: if a project ever made the Drumboy freeze,
  it would otherwise freeze again at every start.

**Turning it off:** File menu, **knob 5 AUTOSAVE**: right = ON (the default), left = OFF. The
setting is remembered.

**Crash-safe saving.** Every save – SAVE and CLEAR in the File and Drumkit menus and the
auto-save – first writes a complete new copy (`System/Save.tmf`, `Save.tmn`), then a small note
saying where it goes (`System/Save.cmt`), and only then replaces the old file. If the power goes
during a save, the Drumboy finishes it or throws the half-written copy away at the next power-on:
you always get either the old version or the new one, never a broken file. (Before v2.01 a save
wrote straight over the old file.) These three files can appear on the card for a moment; you
can ignore them.

**File menu starts at your project.** After LOAD or SAVE (or resuming), the LOAD, SAVE and CLEAR
tabs start at that project's slot instead of FILE_001, so saving again is one press.

**Good to know**
- The auto-save holds what the project files hold: beats, notes, sounds, effects, tempo, song
  mode. Global settings (mixer volumes, metronome, MIDI) are saved separately, as before.
- After a freeze you get the last auto-save: what you did in the last seconds before it, or a
  recording that was running, may be missing.
- The factory firmware and older Moffe versions ignore the new files.

---

## 11. Names for projects and drumkits (v2.02)

Your projects and drumkits have names now – yours, or a random one like `TURBO DUCK`,
`DJ BLOOP`, `YAK 303` or `SPICY WIZARD`.

**In the File menu and the Drumkit menu**

```
 Top:     [1 NEW]       [2 LOAD   007]  [3 SAVE]       [4 CLEAR]
 Bottom:  [5 AUTOSAVE]  [6 NAME]        [7 CURSOR]     [8 RANDOM]      (File menu)
 Bottom:  [5 –]         [6 NAME]        [7 CURSOR]     [8 RANDOM]      (Drumkit menu)
```

- **LOAD / SAVE / CLEAR** show the slot's **name**, and the slot number moves up into the
  header (`LOAD   007`). A slot without a name (empty, or saved by older firmware) shows
  `FILE_007` / `DRUM_007` as before.
- **Knob 6 NAME** shows the name of what you are working on: the project in the File menu, the
  drumkit in the Drumkit menu. Turning it changes the character under the cursor, which is shown
  in **gold** (a gold `_` on a space): A–Z, 0–9, space and `! - + . ? #`.
- **Knob 7 CURSOR** moves the cursor over the 10 characters.
- **Knob 8 RANDOM**: every click rolls a new random name. Keep turning until one makes you smile.
- Then **SAVE** as usual: the name is saved with the project or drumkit, and comes back when you
  load it (and when you resume an auto-save).

**Where names come from**
- At power-on and after **NEW**, the project and the drumkit each get a random name.
- **LOAD** brings the saved name. A project from before v2.02 has no name: its NAME is empty
  (shown as a gold `_`). Give it one, or SAVE it as it is to keep `FILE_007`.
- To remove a name, set every character to a space (the first choice of knob 6).

**Good to know**
- The name is stored inside the file, in bytes no firmware used. The files keep their names on
  the card (`File_007.rwf`, `Drumkit_007.rwd`), so the factory firmware and older Moffe versions
  still load them; they just don't show the name, and saving with them drops it.

---

## 12. MIDI keyboard and arpeggiator (v2.08)

### Play a melody layer from a MIDI keyboard

1. Connect the keyboard's MIDI OUT to the Drumboy Pro's MIDI IN.
2. **IO menu, knob 3 MIDI-IN:** set it to **NOTE** (or **NOTE+SYNC**); **ADD + knob 3** sets the
   channel, which must match the keyboard's.
3. Open a page where the **pads are a keyboard**: the **NOTE** or **ARP** page of a TONE / SYNTH
   layer (S + layer key, S + the same key until it shows), or any **synth page** of a SYNTH layer
   (I + layer key).

Now the keyboard plays that layer:

- **In key:** every key is snapped to the layer's ROOT and SCALE (a key outside the scale plays the
  nearest scale note, the lower one when two are equally near). Middle C (MIDI note 60) is C4, the
  note the pads call C4. A harmony layer plays in the key of the layer it follows.
- **Velocity:** soft keys play softer (and, on SYNTH layers, darker: the filter and FM respond to
  it, as for GHOST beats).
- **Held:** the note sounds as long as you hold the key and goes into its RELEASE when you let go –
  always, whatever PAD HOLD is set to.
- **One note at a time** per layer, like the pads: a new key takes over, and only the newest key's
  release ends the note.
- **Recording:** PLAY + REC and play: the notes are written like pad notes, with how long you held
  each key, so the song plays them back just as long.
- **Step entry:** stopped, hold **ADD** and press keys: each key gives the selected beat its note
  and moves on, as with ADD + pad.
- **Elsewhere** (home screen, other menus) MIDI notes trigger the drum layers as before: C = layer
  1, C# = layer 2 … A = layer 10.
- **Fixed in v2.08:** notes sent with *running status*, and Note On with velocity 0 (how many
  keyboards end a note), used to be missed or played as hits; up to 32 notes are now buffered, so
  chords and fast releases are not lost.

### ARP page – the arpeggiator

Every note of a TONE / SYNTH layer can become an **arpeggio**: the notes of a chord in the layer's
scale, played one after another in time with the tempo.

**Open the page:** **S + layer key**, then **S + the same layer key** until **ARP** shows (edit →
EUCLID → NOTE → ARP).

```
 Top:     [1 ARP]      [2 RATE]    [ pattern preview ]
 Bottom:  [5 OCTAVES]  [6 CHORD]   [7 GATE]   [8 PAD HOLD]
```

| Control | What it does |
|---|---|
| **1 ARP** | **OFF** (the default: notes play as before), **UP**, **DOWN**, **UP/DOWN** (up and back down without repeating the top and bottom note) or **RANDOM**. |
| **2 RATE** | How fast: **1/8**, **1/8T**, **1/16** (the default), **1/16T** or **1/32**, locked to the tempo. |
| **5 OCTAVES** | 1, 2 or 3 octaves: the chord is repeated higher up. |
| **6 CHORD** | Which notes, in scale steps above the played note: **TRIAD** (1-3-5), **7TH** (1-3-5-7), **POWER** (1-5), **SUS4** (1-4-5) or **OCTAVE** (just the note, in octaves – great with 2 or 3 OCTAVES). Always in key: in a minor scale TRIAD is a minor chord, in major a major one. |
| **7 GATE** | How long each arp note sounds, as a share of one step: 10 % (staccato blips) to 100 % (legato). |
| **8 PAD HOLD** | The same setting as on the NOTE page: **ON** = an arpeggio runs as long as you hold the pad. |
| **The pads** | A scale keyboard, as on the NOTE page: press a pad and hear its arpeggio. |
| **UP / DOWN** | The pad keyboard's octave. |
| **CLEAR** (tap) | ARP OFF (the other settings are kept). |
| **CLEAR** (hold) | Erases the layer's beats in this bank, as on the other pages. |

**How long an arpeggio lasts:**

- **A beat of the song:** from the beat until the next beat (or as long as a note recorded with
  PAD HOLD was held). So a long note in the pattern gives a long arpeggio, a short one a few notes.
- **A pad or MIDI key held:** as long as you hold it (pads need PAD HOLD on; MIDI keys always hold).
- **A pad tap** (PAD HOLD off): one quarter note.

It works with the transport stopped too: hold a key and play.

- **The preview** shows one pattern cycle at its pitches, as wide as GATE. Grey means the ARP is OFF
  or the layer can't arpeggiate; the title says why (`TONE / SYNTH LAYERS ONLY`, `FOLLOWS L3`).
- **Harmony layers follow every arp note**, at their interval: one arpeggiating layer plus a
  harmony layer a 3rd up gives arpeggiated chords. The ARP of a harmony layer itself is not used
  (it plays its leader's notes).
- **Recording** writes the note you played, not the arp notes: the arpeggio is played from it every
  time, so you can change ARP, RATE or CHORD afterwards and the whole part follows.
- **MIDI out** sends every arp note, so the Drumboy Pro can arpeggiate an external synth.
- **Song transpose** (v2.03) moves the arpeggios along with the melody.
- **Saved with the project** (in the `.rwn` file) and restored by AUTOSAVE resume; NEW FILE resets
  it. Older firmware plays the plain notes.
- **A first arpeggio:** a SYNTH layer with preset `ZEN PLUCK` or `DX PIANO`, ROOT A, SCALE MINOR.
  ARP page: ARP UP, RATE 1/16, OCTAVES 2, GATE 50. Hold PAD HOLD on, hold a pad, then try another.
  Put a few long beats on the layer and press PLAY: a chord progression becomes a running arpeggio.

#### No arpeggio to hear?

- **MODE** on the NOTE page is TONE or SYNTH, and the layer isn't FOLLOWing another (see the title)?
- **ARP** isn't OFF?
- On a TONE layer, a very short sample ends before the next arp note: try a longer sample or a
  SYNTH layer.
- No sound at all? Check **MASTER MUTE** in the Mixer menu (select it and hold ADD to unmute).

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
| v1.10 | Pads as a scale keyboard (step entry, live recording); melody generator and variations |
| **v2.00** | **SYNTH layers**: a built-in synth voice (saw, square, triangle, sine or single-cycle WAVs; DETUNE, SUB, NOISE, DRIVE, four filter types, AMP envelope, LFO page, SLIDE and GLIDE), 66 presets in alphabetical order + 16 USER slots, random sounds and variations, PAD HOLD (also when recording), the synth sound's name in the track list, synth sounds in drumkits and the instrument copy. **VISUALS**: 13 full-screen music visuals in seven colour themes. **The Moffe Edition look**: gold and orange, new boot screen. **NOTE page**: pads only play while stopped (ADD + pad = step entry), wider octave range. (Test builds v1.11–v1.14 led up to it; they were not released.) |
| **v2.01** | **AUTOSAVE** (File menu, knob 5) and **RESUME LAST SESSION?** at power-on; **crash-safe saving** for projects and drumkits; the File menu starts at the current project's slot. |
| **v2.02** | **Names** for projects and drumkits: shown in LOAD / SAVE / CLEAR, edited with knobs 6–7, or rolled at random with knob 8. |
| **v2.03** | Song mode **TRANSPOSE** per chain step (ADD + knob 7 in the Rhythm menu): melody layers move along their scale, drums stay. |
| **v2.08** | **MIDI keyboard** plays the selected TONE / SYNTH layer (in key, velocity, held, recorded); **ARP page** in Layer Song: arpeggiator with 5 modes, 5 rates, 1–3 octaves, 5 chords, GATE; MIDI in understands running status and velocity-0 note offs. |
| **v2.07** | **VISUALS** batch 2: GEISS (feedback plasma), MILK TUNNEL, ROBOT DJ, DISCO CROWD, PIANO ROLL, KEY WHEEL – 20 styles. |
| **v2.06** | **VISUALS** line-up: CLASSIC, SUPERSCOPE, MOUTH, EYEBALLS, SPEAKER, DEMO SCENE, SUNSET DRIVE, STEP GRID (16 styles); themes WINAMP and C64 (9); AUTO (ADD) and INFO (CLEAR). |
| **v2.05** | **FM voice** (WAVE = FM) and a **MOD** page: FM RATIO / AMOUNT / DECAY, PITCH ENV + PITCH DEC (synth drums, zaps), FILT ATK (filter envelope attack); 13 new presets (79); random sounds use them. |
| **v2.04** | **Harmony layers**: a TONE/SYNTH layer FOLLOWs another and plays its notes at an interval, in its key (NOTE page knob 4, ADD + knob 4 = interval). |

## Coming next

Plans, not promises; each step is released only after it has been tried on a device.

- Ideas welcome in [Issues](../../issues). Song transpose and harmony (the last plan listed here)
  came in v2.03 and v2.04; the MIDI keyboard and the arpeggiator in v2.08.

## Install and roll back

The step-by-step guide (which file to download, where to put it, what the screen shows and what
to do if something goes wrong) is in the **[README → Install](README.md#install)**. In short:

1. Back up the whole SD card.
2. Download `Drumboy_Pro_Fw_Moffe_02_08.bin` from the
   [latest release](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/releases/latest).
3. Copy it, with its exact name, to the root of the card as the only `Drumboy_Pro_Fw…bin` there.
4. Power on: *UPDATING FIRMWARE V2.08* → *FIRMWARE UPDATED* → sunglasses.

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
