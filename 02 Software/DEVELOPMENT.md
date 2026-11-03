# Developing Drumboy Pro firmware safely

This is the workflow for changing the firmware and trying it on a real device without
risking the device or the user's data on the SD card.

## How updates work (and why the device is hard to brick)

| | Flash | How it is updated |
|---|---|---|
| **Bootloader** (`Drumboy-Pro-Boot-H723`) | sector 0, `0x08000000`–`0x0801FFFF` | **Only over SWD** (ST-Link). Never by SD card. |
| **Application** (`Drumboy-Pro-App-H723`) | sectors 1–7, `0x08020000`–`0x080FFFFF` | SD card package, or SWD |

On every power-on the bootloader runs first. If the SD card root contains a
`Drumboy_Pro_Fw*.bin` package it validates it (header, version range, size, CRC-32),
erases the application sectors, programs them, re-reads the flash and checks the CRC again,
then starts the application.

Because the bootloader never overwrites itself, **a bad application can always be replaced
from the SD card**: put a known-good package on the card and power-cycle. Treat the
bootloader with much more care — only reflash it over SWD, and only when you have to.

Since the `fix/bootloader-safety` changes, the bootloader also refuses to jump into an
application whose vector table is invalid, shows *NO VALID APPLICATION*, and keeps a package
that failed while programming so the next boot retries it. A device still running the
**original factory bootloader** (v1.01) does not do this: if an update fails it deletes the
package and may hang on a black screen. Recovery is the same — copy the package to the card
again and power-cycle.

## How the firmware runs (what matters when adding features)

Summarised from the [developer wiki](https://www.randomwaves.io/drumboy-pro/drumboy-pro-wiki.html)
and checked against the code in this repo.

**Execution contexts**

| Context | Runs | Budget / constraint |
|---|---|---|
| I2S3 DMA half/complete callbacks (`main.cpp`) → `processAudioBlock()` | every 32 stereo samples at 44.1 kHz | **≈ 0.73 ms per call** (≈ 400 000 CPU cycles at 550 MHz) for the whole chain: receive → metronome → 10 layers → EQ → 2 filters → 2 effects → reverb → send. Never block here. Measured with the IO-menu CPU meter (v1.09): peak ≈ 32 % with 10 layers playing, ≈ 42 % with chorus, delay, reverb and both filters on. |
| TIM15 → `interruptPlay()` | sequencer tick (BPM-derived) | triggers beats, samples LFOs (sample-and-hold at trigger), sends MIDI/sync |
| Other timers | TIM4 MIDI clock, TIM5 UI transitions, TIM6/TIM7 preset autosave debounce, TIM8 LED meters, TIM12–14 long-press, TIM16 text overlay, TIM17 SD hot-plug, TIM23 sync pulse, TIM24 limit alert | keep handlers short; most just raise flags |
| GPIO EXTI | keypads (MCP23017 INT), 8 encoders, function button, sync-in pulse | |
| Main loop `controller.update()` | `keyboard_action → encoder_action → led_action → lcd_update`, plus deferred SD saves | the only place for slow work (SD, large redraws) |

**Practical consequences**

- The LCD is a **bit-banged 16-bit GPIO bus**: drawing is slow and CPU-bound. Encoder `xxx_set*()`
  methods update the model *and* redraw in the same call, so a new parameter that redraws a lot
  makes the knob feel sluggish. Draw only the region that changed.
- SD access (FatFs over SDMMC2) belongs in the main loop. Interrupts only set flags
  (see the preset autosave: TIM6/TIM7 → dirty flag → `saveSystemPreset()` in the loop).
- **D-cache is off**, I-cache is on. DMA buffers need no cache maintenance today; if you enable
  the D-cache, every DMA buffer (I2S, I2C, SD) needs it.
- BDMA (I2C3/I2C4) can only reach **RAM_D3**: declare such buffers with
  `__attribute__((section(".RAM_D3")))`.
- **FTZ + default-NaN** are set at startup; DSP code must not rely on denormals.
- Audio is `float` internally; int ↔ float conversion happens only in `processAudioReceive` /
  `processAudioSend`. Hot helpers in `Global.h` are `ALWAYSINLINE`.
- Sample data streams from the 16 MB SDRAM at `0xC0000000` (15 sample sectors); Delay/Chorus use
  dedicated SDRAM ring buffers, and the Layer Inst sample preview has its own buffer at
  `RAM_PREVIEW_ADDRESS` (`0xC0EA6000`, the former gap before the images). `RAM_D1` (320 KB) is
  ~91 % used — large new buffers should go to SDRAM.
- FatFs allows **two open files at once** (`_FS_LOCK = 2`). Background work (like the sample
  preview) must not hold a file open between main-loop steps; open, read, close per step.
- The I2S clock is PLL3 in fractional mode (44.099 kHz); don't touch PLL3 or the I2S config
  without measuring pitch.

**SD-card package limits.** The factory bootloader (still on most devices) rejects payloads of
917 504 bytes or more and drops the tail of a payload whose size is not a multiple of 4.
`make_fw_package.py` enforces both, so a package it accepts installs on old and new bootloaders.

## Tools

| Tool | Version | Notes |
|---|---|---|
| Arm GNU Toolchain | 13.3.rel1 | [developer.arm.com](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). Same version as CI. Set `ARM_TOOLCHAIN_DIR` to its folder. |
| CMake | ≥ 3.22 | |
| Ninja | any | |
| Python | ≥ 3.9 | for `make_fw_package.py` |
| STM32CubeIDE | optional | the IDE projects still work unchanged; new `.cpp` files under `Core/` are picked up automatically |
| ST-Link + STM32CubeProgrammer | optional | only needed for the bootloader, debugging, or a device that will not boot |

Windows example (PowerShell):

```powershell
$env:ARM_TOOLCHAIN_DIR = "C:\path\to\arm-gnu-toolchain-13.3.rel1-mingw-w64-i686-arm-none-eabi"
cmake --preset firmware
cmake --build --preset firmware
```

## The change workflow

1. **Branch.** `git switch -c feature/<name>` from an up-to-date `main`. Never develop on `main`.
2. **Change the code.** Keep each change small enough to test on the device in one sitting.
3. **Build locally** with `cmake --build --preset firmware`. It must build with no errors; look
   at new warnings in code you touched (`build/build.log`). Run the host unit tests (below).
4. **Check memory.** The build prints memory usage. `RAM_D1` is already around 90 %;
   watch it. Flash for the application must stay below 896 KB (the packager refuses larger
   images).
5. **Bump the version** in `Drumboy-Pro-App-H723/Core/Library/Global/Global.h`
   (`kVersionMinor`, `kVersionMajor`). The package name and the version shown by the
   bootloader during the update come from here, so you can see on the device which build is
   being installed.
6. **Open a pull request.** CI builds both projects and verifies the package; download the
   package from the workflow run's artifacts if you want the exact CI build.
7. **Test on the device** (below). Merge only after it passes.

### Host unit tests

Logic that doesn't need hardware lives in small headers without HAL dependencies, so the
**same code** that runs on the device can be tested on a PC. Example:
`Drumboy-Pro-App-H723/Core/Library/Global/LayerSongEdit.h` (Layer Song MOVE / SHIFT ALL / ADD),
tested by `02 Software/Tests/test_layer_song_edit.cpp`, and `Global/SampleFormat.h` (WAV PCM
conversion used by the sample loader and the preview), tested by `test_sample_format.cpp`, and
`Global/EuclidPattern.h` (Euclidean rhythms for the Layer Song EUCLID page), tested by
`test_euclid.cpp`, `Global/SongChain.h` (song mode chain), tested by `test_song_chain.cpp`, and
`Global/ToneScale.h` (TONE layers: scales, pitch, note names, glide, TRANSPOSE, the melody
generator and the pad keyboard), tested by `test_tone_scale.cpp`, and `Global/SynthVoice.h`
(the SYNTH voice that runs in the audio ISR: oscillators, filter, envelopes, gate, glide), tested
by `test_synth_voice.cpp`. CI compiles and runs every
`02 Software/Tests/test_*.cpp` with `-Wall -Wextra -Werror`.

```sh
g++ -std=c++17 -Wall -Wextra -Werror -I"02 Software/Drumboy-Pro-App-H723/Core/Library/Global" \
    "02 Software/Tests/test_layer_song_edit.cpp" -o test_layer_song_edit && ./test_layer_song_edit
```

On Windows without g++, `zig c++` (winget `zig.zig`) works with the same flags. When adding a
feature, put the decision logic (positions, ranges, parsing) in such a header and test it;
keep the Controller method as thin glue (snapshot → pure function → write back → redraw).

### Rules that keep changes safe

- **Real-time audio code** lives in `ControllerAudio.cpp` (`processAudioBlock` and
  `processAudio*`). It runs in the I2S DMA interrupt. Do not call SD, LCD, `HAL_Delay` or
  anything that can block from there, and keep the functions it calls in the same file so
  `-Ofast` can inline them. After touching it, compare function sizes with
  `arm-none-eabi-nm -C -S --size-sort build/Drumboy-Pro-App-H723.elf | grep processAudio`.
- **No `static` variables in headers.** Every `.cpp` that includes the header gets its own
  copy, so state silently stops being shared. Use `inline` (C++17) for header-defined
  globals, or put them in a `.cpp`.
- **SD file formats are fixed-size binary layouts** (`kSystemPresetDataSize`,
  `kDrumkitByteSize`, `kSongByteSize`, …). Changing a layout makes existing files on users'
  cards fail to load. If you must change one, change the file's reference header string as
  well and handle the old format. Prefer unused bytes (song chain, v1.07) or a small file next
  to the project (notes in `File_NNN.rwn`, v1.09) so older firmware keeps working.
- **Long-press and other timer ISRs must not draw or touch the SD card.** Raise a flag and do
  the work in the main loop (e.g. ADD hold on the NOTE page: `noteMelodyKeyFlag` in the
  long-key ISR → `noteMelodyActionFlag` → `note_melody()` in `update()`).
- **Check the CPU meter** (IO menu, knobs 5–6) after any change that adds work per audio block.
- **Writes to SD** go through `sd_write()` / `sd_read()`, which treat short transfers (card
  full) as errors, and check `f_close()` — it flushes the last sector.
- **Do not lower the optimization level** of the application (`-Ofast`) without testing audio
  under load; the DSP engine depends on it.

## Testing a build from the SD card

1. **Back up the card first.** Copy the whole card to your computer. `System/File/`
   (projects), `System/Drumkit/` and `System/Preset/` hold the user's own data.
2. **Verify the package** you are about to install:
   ```sh
   python3 "02 Software/Firmware/make_fw_package.py" --verify build/firmware/Drumboy_Pro_Fw_Moffe_02_03.bin
   ```
3. **Copy only the package** to the **root** of the card. Do not touch anything else.
   Make sure there is only one `Drumboy_Pro_Fw*.bin` in the root.
4. **Eject** the card safely, insert it in the Drumboy Pro and power on.
5. You should see *UPDATING FIRMWARE*, the version (e.g. *V1.02*) and a progress bar, then
   *FIRMWARE UPDATED*. The bootloader moves the package to `System/Firmware/`.
6. **Smoke test** after the update:
   - [ ] Device boots to the main screen; audio plays
   - [ ] Load a project and a drumkit
   - [ ] Save a project and a drumkit (to an empty slot), power-cycle, load them again
   - [ ] Change a system setting (e.g. master volume) and an encoder mapping, power-cycle,
         confirm they were kept
   - [ ] Select an instrument with a long name (> 25 characters)
   - [ ] MIDI/Sync in and out, if you use them
   - [ ] Whatever your change is about

If the screen shows *FIRMWARE ERROR*, the second line says which check failed
(FORMAT / VERSION / SIZE / CRC / FLASH / VERIFY).

## Rolling back

The bootloader accepts any version — there is no downgrade protection — so rolling back is
just installing an older package:

- Factory firmware: `02 Software/Firmware/Drumboy_Pro_Fw_01_01.bin`
- Any earlier build: the package archived on the card in `System/Firmware/`, or the CI
  artifact of an earlier commit.

Copy it to the card root and power-cycle. Keep the last known-good package somewhere safe
before you install a new one.
