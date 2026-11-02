# Changelog

Application firmware versions (`kVersionMajor.kVersionMinor` in the app's `Global.h`; the
SD-card package is named after it). Rolling back = installing an older package.

## v2.00

The biggest update so far, and the first release since v1.10: a new major version. It contains
the test builds v1.11 (SYNTH layers), v1.12 (synth sound design) and v1.13 (presets, envelope,
LFO, PAD HOLD), which were not released on their own and are described below, plus what was
built as v1.14: VISUALS, the Moffe Edition look, 66 alphabetical presets, synth names in the
track list and recorded PAD HOLD lengths (this section). `kVersionMajor` 2, `kVersionMinor`
0; the package is `Drumboy_Pro_Fw_Moffe_02_00.bin`. Nothing about the files changes with the
number: projects, drumkits and settings from every earlier version load.

**The Moffe Edition look ("Gold Rush").** `kAccentColor` (gold #FFC21A, `0xFE03`) replaces the
factory cyan everywhere in the UI (menu headers, play bar, active values), `kAccent2Color`
(ruby #FF3D5A, `0xF9EB`) replaces magenta, and `kLayerSelectedColor` is light gold #FFE27A. The KNOB /
SONG footer labels are drawn in ruby (`lcd_drawFooterLabel`). The SD card's panel images
still carry the factory cyan play bar and magenta footer labels; `lcd_drawPage` paints over
them, so no new images are needed. Boot screen: MOFFE in gold with a deep ruby (#8A1028) drop shadow, gold
shades rim, EDITION fading in to ruby with a gold line under it (`kEditionShadow*`,
`kEditionLabelColor`, `kEditionUnderline*`). The factory test screens are unchanged. VISUALS
gets a GOLD RUSH theme (now the default; seven themes). The selected beat in Layer Song is ruby (`kBeatSelectedColor`) and its marker 4 px wide instead of 2, so it stands out from the light gold beats of the selected layer. Also fixed: two `sprintf` overflows
into 3-byte buffers in the footer bank numbers.

**VISUALS mode: full-screen music visuals.** HOME (CENTER) on the home screen turns it on;
LEFT/RIGHT = style, UP/DOWN = colour theme, HOME = off, any menu-panel key leaves and opens its
menu. Pads, transport, MUTE/FILL/STYLE and the home-screen knobs keep working.

- Thirteen styles. Added later in v1.14: RAIN (digital rain, one column per band, hit
  bursts), FIREWORKS (a rocket per hit, bursting in the layer's colour), MANDALA (waveform and
  spectrum mirrored into eight turning wedges), DISCO (light-up floor: eighth-note patterns,
  bar checkerboard, tiles per hit), RIDGES (stacked spectrum ridge lines, painter's algorithm
  for hidden lines), LIFE (Game of Life per eighth note, gliders / R-pentominoes per hit).
  DTCM use 57 KB of 128 KB.
- The first seven: SHOWTIME (layer lights + VU + spectrum + triggered scope + beat-synced neon
  floor), ORBIT (layers as planets, hit ripples and lightning, bass core, one planet per bar),
  PHASE (phase portrait of the output with a three-generation trail), SPECTRUM (32 mirrored
  bars with caps), SPECTRO (scrolling spectrogram with beat ticks), TUNNEL (frames per eighth
  note, bold on the bar, starfield), METERS (two VU needle meters + ten layer ladders). Six
  themes: SYNTHWAVE, ACID, OCEAN, FIRE, ICE, RAINBOW.
- Drawing: the panel has no frame buffer, so each style erases exactly what it drew the frame
  before; ~30 fps from the main loop. While VISUALS is on the LCD driver is **muted**
  (`Lcd::setMuted`, every public draw call returns at once), so the normal UI keeps its state
  but draws nothing; leaving redraws the home screen in full (page, layer rows, songs, tabs,
  icons, play bar, home menu, a pending alert). An alert ends VISUALS.
- Data: while VISUALS is on, the audio ISR copies the output (1024-sample stereo ring) and peak
  levels (master and per layer) into `visTap`; `layer_playBeat` records hits. The main loop runs
  a 1024-point FFT (32/64 log bands, 40 Hz–16 kHz). Maths in `Global/Visuals.h`, host-tested by
  `test_visuals.cpp`.
- Memory: the visuals live in **DTCM** (new `.DTCM` NOLOAD section in the linker script, 30 KB of
  128 KB; it was unused). RAM_D1 is unchanged. Flash +25 KB.
- Also in this version: recording with PAD HOLD keeps the held note lengths (see v1.13 below;
  it was added after the v1.13 test build).

**SYNTH presets: 66, alphabetical.** 34 new factory sounds (ALIEN SPA, BEE SWARM, BIG FUZZ,
COWBELL!, DEEP SPACE, DISCO DUCK, FAIRY DUST, HYPERDRIVE, KAZOO KING, NINJA STAR, TOY PIANO,
UFO LANDED, VIKING, ZOMBIE, …); the PRESET knob lists them A–Z, then USER 01..16. EMPTY MIND is
still the default sound (found by `synth::defaultPresetIndex()`, no longer index 0). Presets
are not stored by index anywhere, so old projects are unaffected.

**Track list shows the synth sound.** A SYNTH layer's row on the main screen shows its sound
like a sample name: the preset name (`USER nn` for a saved sound), `NAME*` once edited, `SYNTH`
for a sound of its own. Found by comparing the sound with the presets (`synth::sameSound`,
PAD HOLD ignored), so it also works after loading a project or drumkit; the PRESET knob and the
SOUND page start from that preset too. Updated on preset/random/undo/knob changes, MODE
changes, paste, project and drumkit loading.

## v1.13 (test build, released in v2.00)

**SYNTH: presets, random sounds, AMP envelope, filter types, its own LFO page, synth sounds in
drumkits and the instrument copy (step 4d)**

- **Six synth pages** (Layer Instrument of a SYNTH layer; the layer key steps through them):
  SOUND → OSC → FILTER → AMP → LFO → routing. Two pages share each `LAYER_INST_x_MENU`
  (`kSynthPageMenu`, `Controller::synthPage`); the menu opens on SOUND.
- **SOUND page:** **PRESET** (knob 1) browses 32 factory sounds with epic and silly names
  (`ACID TRIP`, `WUBZILLA`, `DRAGONROAR`, `BOOMSTICK`, `TSS TSS`, …; basses, leads, plucks, keys,
  pads, strings, brass, drums and effects) and then the USER slots; every step plays a note.
  **SAVE TO** (knob 2) picks USER 01–16; **ADD** saves the sound there
  (`System/Preset/SynthUser.rws`, "RWSYNUSR" + 16 × [used, 20 values], loaded at start-up).
- **Random sounds:** on OSC / FILTER / AMP / LFO, **ADD hold** = a new random sound on release
  (a bass, lead, pluck, pad or effect with values in that family's useful ranges), **ADD tap** = a
  variation (three settings move 5–15 steps, sometimes the waveform). ADD used with a knob or
  UP/DOWN does nothing on release.
- **Undo:** a **CLEAR tap** on any synth page brings back the sound from when the Layer
  Instrument menu was opened. CLEAR hold still resets.
- **AMP page:** **ATTACK** (1 ms–3 s), **AMP DECAY** (5 ms–5 s) to **SUSTAIN**, **RELEASE**
  (5 ms–5 s), and **GATE** (moved here from FILTER). The defaults reproduce the old fixed 2 ms /
  full / 12 ms envelope, so existing sounds are unchanged. The display draws the envelope.
- **PAD HOLD** (AMP page knob 8; NOTE page knob 3 for TONE and SYNTH layers, shown as `HOLD` in
  the strip): ON = a pad note sounds while the pad is pressed and is released when you let go
  (SYNTH: gate held up to 60 s, then RELEASE; TONE: the sample fades out in ~1 ms). The newest
  pad owns the note. Stored with the sound (21st value) but kept when presets, random sounds or
  undo change the sound. Files: "SYN6", "SYK4", "RWSYNUS2" (older v1.13 test-build files load).
  **Recording with PAD HOLD keeps the note length**: the release stores how long the pad was
  held (per beat, in sequencer intervals), and playback sounds the note exactly that long
  instead of GATE % (SYNTH: gate = held length; TONE: fades out after it). A SLIDE into the next
  note still wins. While the pad is still down the sequencer does not retrigger the new beat.
  Saved in the `.rwn` file as "HLD1" after the slides. `Beat` grew from 8 to 10 bytes
  (RAM_D1 92.1% → 94.2%).
- **FILT TYPE** (FILTER page, knob 7): LP 24 (as before), LP 12, BAND, HIGH, tapped from the same
  ladder (classic mode mixing), so it costs a few multiplies per sample.

- **LFO page** for SYNTH layers: pressing the layer key in the Layer Instrument menu now goes OSC
  → FILTER → **LFO** → routing. Knobs: **1 LFO SHAPE** (SINE, TRIANGLE, SAW DOWN, SQUARE, RANDOM
  = sample & hold), **2 LFO RATE** (tempo-synced: 1/32, 1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1, 2, 4,
  8 bars; bars follow the Rhythm menu's MEASURE), **5 LFO DEPTH** (0 = OFF, the default),
  **6 LFO TARGET** (CUTOFF ±3 octaves, PITCH vibrato up to ±12 semitones with a cubic curve so
  low depths are a gentle vibrato, LEVEL tremolo down to silence), **7 LFO RETRIG** (FREE = runs
  on across notes, NOTE = restarts with every note except slides). The display shows the wave,
  as tall as DEPTH, with a marker that follows the voice's LFO.
  Everything is on the synth's own page, so no setup in the global LFO menu is needed (an early
  test build used ADD + CUTOFF to pick a global LFO, which needed switching on in the LFO menu
  and was hard to discover; that was replaced before release).
- The LFO runs per voice in the audio interrupt, once per 32-sample block, and keeps running
  between notes; SQUARE/RANDOM steps are smoothed over ~2 blocks and LEVEL is ramped per sample,
  so nothing clicks. PITCH never pushes the oscillator past Nyquist.
- **Drumkits** now include each layer's MODE (DRUM / TONE / SYNTH) and its whole SYNTH sound,
  LFO included. Saved as a "SYK2" section after the kit's "EOF": older firmware reads a fixed
  size and never sees it. Kits saved by older versions have no such section: loading one leaves
  the layers' MODE and synth sound as they are, so an old drum kit never wipes a synth line.
- **Instrument copy/paste** (Layer Instrument menu) carries MODE and the SYNTH sound too: a synth
  layer pastes as the same synth, and the page swaps between sample and synth pages if needed.
- **NOTE page pads no longer edit by accident** (from hardware testing: trying sounds after a
  recording changed the track). While stopped, pads only play. Step entry is now **ADD + pad**
  (the strip shows `ADD+PAD: STEP`). ADD hold (new melody) now fires when ADD is released, and
  not at all if ADD was used with a pad or UP/DOWN, so holding ADD for step entry can never
  overwrite the melody.
- **Wider note range** (from hardware testing: "range too small"): UP/DOWN moves the pad
  keyboard 4 octaves down and 3 up (was 2 and 1), and notes may lie five octaves of the scale
  either side of the root (was two), so TRANSPOSE and the NOTE knob reach as far. The beat's
  note byte now uses its spare bit 7: degrees -64..63 (host-tested packing; it is never stored on
  SD, the `.rwn` file keeps one signed byte per note as before).
- Projects: the `.rwn` sound section is now "SYN6" (21 values per layer: sound, LFO, AMP,
  filter type, pad hold), drumkits "SYK4". v1.12 ("SYN2") and v1.11 ("SYN1") projects still load, as do
  the v1.13 test builds' files.

Under the hood: `synth::pack/unpack/isDefault` are the single place for the sound's file layout
(project, drumkit), host-tested for round trips, old shorter records and junk. `SynthVoice.h`
gained the LFO (`lfoValue`, `lfoIncrement`, `advanceLfo`; `render(..., lfoIncrement)`), tested
for shape ranges, tempo lock (1/4 at 120 BPM = 2 Hz, bars in 3/4 and 4/4), audible effect on all
three targets, click-free tremolo, retrigger/free/slide behaviour and bad input.
`tone_setMode()` is now the one click-free way to switch a layer's MODE (NOTE page, drumkit
load, paste). The LFO page shares `LAYER_INST_2_MENU` with routing (`synthLfoPage`), with its own
knob row in `kEncoderSynthFunction`. RAM_D1 92.0 %, app +8.6 KB. No new compiler warnings.
## v1.12 (test build, released in v2.00)

**SYNTH: DETUNE, SUB, NOISE, SLIDE and sample waveforms (steps 4b + 4c)**

- **OSC page** (Layer Instrument page 1 of a SYNTH layer) now uses all its knobs: **1 WAVE**,
  **2 DETUNE** (two oscillators spread up to ±40 cents around the note, the second one starting
  at a random phase like a free-running analog oscillator: thick, chorus-like), **5 LEVEL**,
  **6 PITCH**, **7 SUB** (a square one octave down), **8 NOISE**.
- **WAVE = SAMPLE:** the layer's loaded sample is played as one cycle of the waveform, so any
  single-cycle WAV on the card becomes an oscillator (load it while the layer is in TONE or DRUM
  mode, then switch to SYNTH). DETUNE, filter and everything else apply. The display shows the
  cycle, or `NO SAMPLE LOADED`.
- **SLIDE per beat** (NOTE page, **ADD + UP** = on, **ADD + DOWN** = off for the selected beat):
  the note slides in from the previous one, 303 style. On SYNTH layers the previous note is
  held until it and the envelopes keep running (true legato); on TONE layers the sample
  retriggers but the pitch slides. Slide time = GLIDE, at least about 36 ms. The NOTE value shows
  `SLIDE`, the strip joins the two notes, CLEAR (tap) undoes slides too, and MIDI out overlaps
  the notes so external mono synths play legato.
- Saved with the project (`.rwn`: "SYN2" with the new values, "SLD1" with the slides); projects
  saved by v1.11 still load. Older versions ignore both.
- **Fix: GLIDE on SYNTH layers** (found in hardware testing). The voice only glided if the
  previous note was still sounding; with GATE below 100 it had usually faded out (gate plus
  release) before the next beat, so the next note jumped. The voice now remembers its last pitch
  and GLIDE slides from it, as on TONE layers; only the very first note after a mode change jumps.
- **Fix: DRIVE was hardly audible** (found in hardware testing). It only clipped before the
  filter, which smoothed the harmonics away, and the level makeup cancelled the rest. DRIVE now
  also saturates after the filter (1–30× gain, normalised, fading in over 0–25): from warm to
  loud and square-ish. DRIVE 0 stays clean; the default (25) is a little warmer than before.
- **Fix: DETUNE was too subtle** in its first take (25 cents in one direction); now ±40 cents
  around the note with a random start phase for the second oscillator.

Under the hood: SLIDE shares the beat's note byte (bits 0-5 note, bit 6 slide; host-tested
packing), so it costs no RAM; snapshots carry it through MOVE, SHIFT ALL and EUCLID MERGE. The
sequencer passes "slides in" and "the next beat slides in" with each trigger. `SynthVoice.h`:
detune/sub/noise mix, linear-interpolated table oscillator, legato start; `test_synth_voice.cpp`
now 2.96 million checks (everything at maximum stays bounded, table accuracy < 0.001, legato
keeps the envelopes and glides). RAM_D1 +368 B (91.9 %), `processAudioLayer` +0.6 KB. No new
compiler warnings.

## v1.11 (test build, released in v2.00)

**SYNTH layers — a built-in synth voice (step 4a)**

- **MODE on the NOTE page has a third value: DRUM → TONE → SYNTH.** A SYNTH layer plays its notes
  with a built-in voice instead of a sample; no sample needs to be loaded. Everything from the
  NOTE page works the same: scale, root, GLIDE, TRANSPOSE, the pad keyboard and the melody
  generator. The layer goes through EQ, filters, effects, reverb, mixer and LFOs like any other.
- **Layer Instrument menu (I + layer) of a SYNTH layer:**
  - **Page 1 OSC:** **1 WAVE** (saw, square, triangle, sine), **5 LEVEL**, **6 PITCH** (as for
    samples, LFOs included). The display shows the waveform.
  - **Page 2 FILTER:** **1 CUTOFF**, **2 RESONANCE**, **5 ENV AMT** (how far each note opens the
    filter), **6 DECAY** (how fast it closes, 20 ms – 2 s), **7 GATE** (how long a note is held,
    as % of the time until the next beat), **8 DRIVE** (saturation). The display shows the filter
    envelope and the gate.
  - **Page 3** routing (EQ / FILTER / EFFECT / REVERB) as before.
- **Accent for free:** a beat's VELOCITY decides the accent. FLAT beats play full and bright;
  GHOST beats softer and darker.
- **MIDI out:** a SYNTH layer sends its notes with the real note length (Note Off when the gate
  ends).
- **Saved with the project** in the `.rwn` file, after the part older firmware reads: v1.09/v1.10
  open the project with the layer as a DRUM layer and its notes intact. CLEAR in the Layer
  Instrument menu and NEW FILE reset the sound. Not part of drumkits or the instrument copy.
- Coming next (4b/4c): DETUNE, SUB, NOISE, per-beat SLIDE, single-cycle WAVs from the card.

**Fix: erasing beats on the EUCLID and NOTE pages**

- On these pages CLEAR only undid, so a layer could not be erased there, including TONE and
  SYNTH layers, which are edited on the NOTE page (reported from a hardware test). Now, as on
  the edit page, **holding CLEAR erases all of the layer's beats** in the bank; a **tap** still
  undoes. After an erase the page starts over from the empty layer, so the next EUCLID knob turn
  or CLEAR tap can't bring the erased beats back. The hint reads **CLEAR: UNDO  HOLD: ERASE**.

Under the hood: the voice is a pure header, `Global/SynthVoice.h`, host-tested in
`test_synth_voice.cpp` (2.19 million checks). The tests cover every waveform at maximum resonance,
drive and envelope, filter stability, gate and release, click-free retrigger, glide and accent.
It uses a PolyBLEP saw/square, a 4-pole low-pass with soft-clipped resonance feedback (all states
provably inside ±1), a 2 ms attack and a 12 ms release. Filter coefficients are computed once per
32-sample block. The sequencer prepares each note (pitch, level, velocity, gate from the beat
length) and hands it to the audio ISR under a critical section; drum and TONE layers run the
same code as before. RAM_D1 +856 B (91.8 %), flash +8.6 KB, `processAudioLayer` +1.9 KB (the
voice is inlined). No new compiler warnings.

## v1.10

**Play and generate melodies (NOTE page of a TONE layer)**

- **The 10 pads are a scale keyboard** for the layer: pad 1 is the root, pads 2–10 the next
  notes of the scale, so every pad is in key. **UP/DOWN** move the keyboard an octave (two
  octaves down, one up); the strip shows the range, e.g. `PADS C4-E5`.
- **Stopped: step entry.** A pad gives the selected beat its note and moves on to the next beat:
  tap in a bassline note by note.
- **Playing:** the pads play along. **With REC** they record beats with those notes (on the
  quantize grid, like drum hits); recording over an existing beat replaces its note. A hit
  during the precount lands on the first beat with its note.
- **Hold ADD: new melody.** A random line through the scale over the bank's beats: mostly
  steps, sometimes a leap, within an octave of the root, starting and ending on the root. Every
  hold gives a different one.
- **Tap ADD: variation.** About a quarter of the notes (at least one) move one or two scale
  steps; keep tapping to let the melody evolve.
- **CLEAR** undoes all of it, back to the notes the page opened with.
- The modifier keys still work on the pads (S + layer to change page or layer, MUTE, FILL…).
  On DRUM layers, and outside the NOTE page, the pads are unchanged.

Under the hood: the generator, the variation and the pad mapping are pure functions in
`Global/ToneScale.h` (deterministic xorshift32, seeded from the cycle counter on the device),
host-tested in `test_tone_scale.cpp`. The long press only sets a flag in the long-key ISR; the
main loop generates and draws. `layerSong_setBeat()` takes an optional note. RAM_D1 +16 B, flash
+2.9 KB. Audio path unchanged; no new compiler warnings.

Tested on a Drumboy Pro: pad keyboard and melody generator.

## v1.09

**Melody layers (TONE) — play your samples as notes**

- **New NOTE page in Layer Song.** **S + layer** cycles **edit → EUCLID → NOTE → edit**.
  Knobs: **1 MODE** (DRUM / TONE), **2 NOTE** (the selected beat's note, e.g. `D#4`),
  **5 ROOT** (C–B), **6 SCALE** (chromatic, major, minor, dorian, phrygian, lydian, mixolydian,
  harmonic minor, pentatonic major/minor, blues), **7 GLIDE** (0–100), **8 TRANSPOSE** (every
  note of the bank, in scale steps; the melody moves as a whole and stops when its highest or
  lowest note reaches the edge of the range, so the intervals are never lost).
- **DRUM** layers play exactly as before. A **TONE** layer plays each beat at its own note.
  Notes snap to the scale, so a wrong note is impossible.
- Notes are stored as steps of the scale. Changing ROOT or SCALE later keeps the melody in key
  instead of leaving notes outside it. Two octaves either side of the root.
- The sample is taken to be a **C** (shown as C4) at the layer's PITCH; use PITCH to tune it.
- The strip shows a small piano roll of the bank (time → right, pitch ↑), with the selected
  beat in cyan. **LEFT/RIGHT** select a beat; turning **NOTE** plays it (while stopped). The
  layer's pad plays the selected beat's note. **CLEAR** undoes the note edits made since the
  page opened.
- **GLIDE** slides each note from the previous pitch (5 ms to 0.5 s): from a quick 303-style
  slide to a slow swoop.
- Notes travel with their beats: MOVE, SHIFT ALL, EUCLID MERGE, quantize and song copy/paste
  keep them; an ADD tap gives the new beat the note of the beat before it.
- **MIDI out:** TONE layers send the beat's own note (the sample's pitch is MIDI 60 = C4) and
  release the previous note first, so external synths get a clean monophonic line. Drum layers
  send what they always sent.
- **Saved with the project** in a small `File_NNN.rwn` next to `File_NNN.rwf`. The `.rwf`
  format and size are unchanged; older firmware ignores the new file and plays the layer as a
  drum. Projects without notes get no `.rwn`. NEW FILE and CLEAR FILE reset it. Not part of
  drumkits.
- Limits: one voice per layer (a new note retriggers the sample with the usual short
  crossfade); as on any sampler, higher notes play the sample faster and therefore shorter.
  Pads outside the NOTE page and MIDI in play the root.

**Audio CPU meter**

- **IO menu, knob 5 CPU LOAD** (average over about 0.1 s) and **knob 6 CPU PEAK** (the busiest
  single audio block since start-up; turn the knob to reset it; red from 80 %). Above 100 % the
  audio interrupt misses its deadline and the output clicks. It measures the audio interrupt
  against the real time between two blocks, including other interrupts that delay it.

Under the hood: pitch, scales, note names and glide are pure functions in
`Global/ToneScale.h`, host-tested (`test_tone_scale.cpp`, 8200 checks). The note uses a padding
byte of `Beat` (guarded by `static_assert(sizeof(Beat) == 8)`), so 3200 notes cost no RAM;
RAM_D1 grows by 656 B (91.3 → 91.5 %), flash by 11.5 KB. Glide is applied once per audio block,
outside the per-sample loops (`processAudioLayer` +64 B); the meter adds two cycle-counter
reads per block (`processAudioBlock` +164 B). No new compiler warnings.

Tested on a Drumboy Pro: notes, GLIDE, TRANSPOSE (after the fix below, part of v1.09) and the CPU
meter; CPU PEAK about 42 % with all 10 layers playing (two TONE) and chorus, delay, reverb and both
filters on. Fixed during testing: TRANSPOSE clamped every note on its own, so turning it far
squashed the melody into one note; it now moves the melody as a whole and stops at the edge.
Saving and reloading notes: still to be confirmed.

## v1.08

- **EUCLID page:** the step strip now shows **CLEAR: UNDO** at the bottom right, so it is
  visible that the CLEAR key restores the beats the layer had when the page opened. (The
  device font has no `=`, hence the colon.)
- **Package name:** SD packages are now called `Drumboy_Pro_Fw_Moffe_MM_mm.bin`, so they can't be
  confused with official Randomwaves updates. The bootloader (factory or Moffe) accepts both;
  `make_fw_package.py` checks the name against the bootloader's search pattern and its 32-character
  long-file-name limit.
- **Repository prepared for public sharing:** clear "unofficial, use at your own risk" notices,
  credits and a `NOTICE` file for third-party licenses, a release workflow (tag → draft release
  with package, bootloader and SHA-256 checksums), issue templates and `CONTRIBUTING.md`. The
  Randomwaves user manual is no longer included (it is not MIT-licensed); it is linked instead.
- **Install guide:** the README has a step-by-step install guide (which file to download, where
  it goes on the card, what the screen shows, troubleshooting, roll back). Releases also include
  the factory package for rolling back.

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
