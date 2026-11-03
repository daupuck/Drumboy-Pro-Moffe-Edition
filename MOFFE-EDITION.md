# Drumboy Pro – Moffe Edition

Moffe Edition is **unofficial community firmware** for the Drumboy Pro, based on the open-source
[Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro) (factory firmware v1.01). It
is not affiliated with or endorsed by Randomwaves. This page collects everything that is new
compared with the factory firmware: what each feature does and how to use it on the device.

- **Current version:** v2.03 — download from the
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
> projects and drumkits (v2.02). **Not yet tested on a device:** the rest of v2.00 (presets,
> random sounds, AMP and LFO pages, PAD HOLD, VISUALS, the new look) and v2.03 (song transpose). Each release says what has been tested on hardware;
> please report anything odd in [Issues](../../issues).

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
| 🌈 | **[VISUALS](#8-visuals--full-screen-music-visuals-v200)** | 13 full-screen visuals that dance to your music: fireworks for every hit, a Game of Life seeded by your drums, a spectrum skyline, digital rain, a kaleidoscope and more. Press HOME on the home screen. |
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
NOTE). Once more goes back to the edit page.

```
 Top:     [1 MODE]   [2 NOTE]    [ note strip: a small piano roll ]
 Bottom:  [5 ROOT]   [6 SCALE]   [7 GLIDE]   [8 TRANSPOSE]
```

| Control | What it does |
|---|---|
| **1 MODE** | **DRUM**: the layer plays as before and ignores the notes. **TONE**: every beat plays its sample at its note. **SYNTH**: every beat plays the built-in synth at its note ([section 7](#7-synth-layers--a-built-in-synth-voice-v200), v2.00). |
| **2 NOTE** | The note of the selected beat, e.g. `D#4`. Only notes of the scale are offered, five octaves either side of the root (two before v2.00). |
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
- **Good to know:** one note at a time per layer (use several layers for chords). As on any
  sampler, higher notes play the sample faster, so they are also shorter. Outside the NOTE page
  the pads, and MIDI in, play the root; on the NOTE page of a TONE layer the pads only play that
  layer (go back to the edit page to play the drums).

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
 Page 3 FILTER:  [1 CUTOFF]    [2 RESO]       [ filter envelope + type ]
                 [5 ENV AMT]   [6 DECAY]      [7 FILT TYPE]  [8 DRIVE]
 Page 4 AMP:     [1 ATTACK]    [2 AMP DECAY]  [ level envelope ]               (v2.00)
                 [5 SUSTAIN]   [6 RELEASE]    [7 GATE]       [8 PAD HOLD]
 Page 5 LFO:     [1 LFO SHAPE] [2 LFO RATE]   [ LFO wave + position ]          (v2.00)
                 [5 LFO DEPTH] [6 LFO TARGET] [7 LFO RETRIG]
 Page 6:         routing to EQ / FILTER / EFFECT / REVERB, as for samples
```

**Sounds in three ways** (v2.00):

| | What it does |
|---|---|
| **PRESET** (SOUND page, knob 1) | 66 factory sounds in **alphabetical order**, with names to match their egos: `ACID TRIP`, `ALIEN SPA`, `BAD PENNY`, `BANSHEE`, `BEE SWARM`, `BIG FUZZ`, `BLIP BLOOP`, `BOOMSTICK` (kick), `BRASS GOD`, `BUBBLEGUM`, `CAT WALK`, `CHURCH BOI`, `CLOUD NINE`, `COCONUTS`, `COIN GET`, `COWBELL!`, `DAD STEP`, `DARK LORD`, `DEEP SPACE`, `DING DONG`, `DISCO DUCK`, `DIZZY BOT`, `DRAGONROAR`, `EARTHQUAKE`, `EMPTY MIND` (the default), `EPIC MOVIE`, `FAIRY DUST`, `FOGHORN`, `GHOST TOWN`, `GLASS HARP`, `GOLD RUSH`, `HONKY BONK`, `HYPERDRIVE`, `JELLYFISH`, `KAZOO KING`, `LASER CAT`, `LOUNGE LIZ`, `MOON BASS`, `MORDOR`, `NINJA STAR`, `NOODLE ARM`, `OCTOPUS`, `PANIC BOT`, `PEW PEW`, `PIXEL HERO`, `R2 DEEDOO`, `ROBO CHOIR`, `RUBBERDUCK`, `SAWSOME`, `SEASICK`, `SNARE WARS`, `SPACE TAXI`, `SQUARE GUY`, `STARGATE`, `STORMBORN`, `SUNNY DAY`, `THICC BASS`, `TOY PIANO`, `TSS TSS` (hi-hat), `TUMBLEWEED`, `UFO LANDED`, `VIKING`, `WARM HUG`, `WUBZILLA`, `ZEN PLUCK`, `ZOMBIE` – then your own sounds, `USER 01`…`USER 16`. Turning plays a note so you hear each one. (66 and alphabetical since v2.00; 32 before.) |
| **Track list name** | A SYNTH layer shows its sound in the layer list on the main screen, like a sample name: the preset name, `USER 03` for a saved sound, a `*` after the name once you have changed it, or `SYNTH` for a sound of its own (e.g. a random one). Also after loading a project or drumkit. (v2.00) |
| **ADD** on the SOUND page | Saves the current sound into the USER slot shown by **SAVE TO** (knob 2; `*` = slot already used, it is overwritten). Kept on the SD card (`System/Preset/SynthUser.rws`) for all projects. |
| **ADD hold** on OSC / FILTER / AMP / LFO | A **random sound** when you let go: a bass, lead, pluck, pad or effect, always in useful ranges. |
| **ADD tap** on those pages | A **variation**: three settings move a little, now and then the waveform changes. Tap again and again. |
| **CLEAR tap** on any synth page | **Undo**: back to the sound the layer had when you opened the Layer Instrument menu (presets, random sounds and edits alike). CLEAR hold still resets the layer. |

ADD used together with a knob (LFO on LEVEL/PITCH) or UP/DOWN never changes the sound.

| Control | What it does |
|---|---|
| **WAVE** | Saw (bright, the classic bass), square (hollow), triangle (soft), sine (pure, sub-bass), or **SAMPLE** (v2.00): the layer's loaded sample as one cycle of the waveform – see below. |
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

## 8. VISUALS – full-screen music visuals (v2.00)

The whole screen becomes a light show that follows the music. You can keep playing: pads,
PLAY/STOP/REC, MUTE/FILL/STYLE + pad and the home-screen knobs all work while it runs.

| Key | What it does |
|---|---|
| **HOME** (centre key) on the home screen | VISUALS on. (HOME in any other menu still goes to the home screen, as before.) |
| **LEFT / RIGHT** | The previous / next style (13). |
| **UP / DOWN** | The colour theme: GOLD RUSH (the default), SYNTHWAVE, ACID, OCEAN, FIRE, ICE, RAINBOW. |
| **HOME** | VISUALS off, back to the home screen. |
| **Any menu key** (right-hand panel) | Leaves VISUALS and opens that menu. |

A line at the top shows the style, the theme and the keys for a few seconds after each change.

| Style | What you see |
|---|---|
| **1 SHOWTIME** | Everything at once: ten layer lights that flash on every hit, VU columns at both edges, a spectrum skyline with falling caps, an oscilloscope across the horizon and a neon floor rolling towards you on the beat. |
| **2 ORBIT** | The ten layers as planets circling a core that breathes with the bass. Every hit sends a ripple out of its planet and a flash of lightning to the core; the orbit turns one planet per bar. |
| **3 PHASE** | The sound drawn against itself a moment later: a pure tone is a clean loop, chords become knots, noise becomes a cloud, with a glowing trail, slowly turning. Silence shows a gentle figure. |
| **4 SPECTRUM** | 32 rainbow towers mirrored in a reflecting floor, with falling white caps. |
| **5 SPECTRO** | A spectrogram painting itself across the screen: time left to right, pitch bottom to top, loudness as heat. Ticks underneath mark the beats (white = a new bar). |
| **6 TUNNEL** | Flying through a tunnel of frames, one per eighth note (a bold white one on each bar), through stars that speed up with the music. |
| **7 METERS** | Two big analogue needle meters (left, right) over ten LED ladders, one per layer. |
| **8 RAIN** | Digital rain: 64 columns of falling characters, one per frequency band. Loud bands fall fast with long trails; a hit sends a burst down its layer's columns. |
| **9 FIREWORKS** | Every hit launches a rocket from its layer's place on the ground that bursts into sparks in the layer's colour; accents fly higher and burst bigger. |
| **10 MANDALA** | A kaleidoscope: the waveform and the spectrum mirrored into eight wedges, slowly turning, with a heart that beats with the bass. |
| **11 DISCO** | A light-up dance floor: patterns on every eighth note, a checkerboard on each bar, and every hit lights a tile in its layer's part of the floor. |
| **12 RIDGES** | The spectrum's recent past as stacked mountain ridges, like the famous album cover: the newest line in front. |
| **13 LIFE** | Conway's Game of Life, one generation per eighth note. Every hit drops a glider into its layer's part of the world (an accent: a wild R-pentomino); newborn cells flash white. |

- The visuals follow the sequencer while it plays (floor, orbit, tunnel and beat ticks lock to the
  tempo) and the TEMPO setting when it is stopped.
- An alert (e.g. a save question) switches VISUALS off so you can answer it.
- The style and theme are kept until power-off.
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

## Coming next

Plans, not promises; each step is released only after it has been tried on a device.

- **Song transpose and harmony:** each song-mode step can change key, and a layer can follow
  another a third or fifth higher.

## Install and roll back

The step-by-step guide (which file to download, where to put it, what the screen shows and what
to do if something goes wrong) is in the **[README → Install](README.md#install)**. In short:

1. Back up the whole SD card.
2. Download `Drumboy_Pro_Fw_Moffe_02_03.bin` from the
   [latest release](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/releases/latest).
3. Copy it, with its exact name, to the root of the card as the only `Drumboy_Pro_Fw…bin` there.
4. Power on: *UPDATING FIRMWARE V2.03* → *FIRMWARE UPDATED* → sunglasses.

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
