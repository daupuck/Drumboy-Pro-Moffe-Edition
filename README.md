# Drumboy Pro – Moffe Edition

[![Firmware CI](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/actions/workflows/firmware.yml/badge.svg)](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/actions/workflows/firmware.yml)
[![Latest release](https://img.shields.io/github/v/release/daupuck/Drumboy-Pro-Moffe-Edition?label=release)](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/releases/latest)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**Unofficial community firmware for the Randomwaves Drumboy Pro.** Same hardware, more groove —
and the boot screen now wears sunglasses. 😎

Built on the open-source [Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro)
project. Installs from the SD card in about five minutes, keeps all your projects, and can be
rolled back to the factory firmware the same way.

> [!IMPORTANT]
> This is **not an official Randomwaves release** and is **not affiliated with or endorsed by
> Randomwaves**. "Drumboy Pro" is Randomwaves' product name, used here only to say which device this
> firmware is for. Please report problems with this firmware in this repository's
> [Issues](../../issues), **not** to Randomwaves.

> [!WARNING]
> **Use at your own risk.** This firmware is provided "as is", without warranty of any kind (see
> [LICENSE](LICENSE)). Installing it is your own decision and responsibility. **Back up your whole
> SD card first** and keep the factory firmware for rolling back. Modifying the firmware may affect
> your warranty with Randomwaves.

## What you get

| | Feature | In one sentence |
|---|---|---|
| 🥁 | [**Euclidean rhythms**](MOFFE-EDITION.md#4-euclidean-rhythms--euclid-page-in-layer-song-v106) | Pick steps and hits, turn a knob, hear the pattern change live. |
| 🎹 | [**Melody layers**](MOFFE-EDITION.md#6-melody-layers--note-page-in-layer-song-v109) | Turn any layer into an instrument: give each beat a note in a key and scale, add glide, play the pads as a keyboard, or hold ADD and let it write a melody. Your kick just became an 808 bassline. |
| 🎛️ | [**SYNTH layers**](MOFFE-EDITION.md#7-synth-layers--a-built-in-synth-voice-v111) | A built-in synth on any layer: saw, square, triangle, sine or your own single-cycle WAVs, with detune, sub, noise, a resonant filter with envelope, and 303-style slides. Acid basslines without a single sample. |
| 🎼 | [**Song mode**](MOFFE-EDITION.md#5-song-mode--arrange-parts-into-a-song-v107) | Chain the 5 song banks into intro → verse → chorus, with repeats, looped or played once. |
| 🎯 | [**Precise beat placement**](MOFFE-EDITION.md#1-place-beats-precisely--layer-song-menu-v103) | Move a single beat, shift a whole layer, or add one beat exactly where you want it. |
| 🎧 | [**Sample preview**](MOFFE-EDITION.md#3-sample-preview--layer-instrument-menu-v105) | Hear a sample *before* you load it. Your ears will thank you. |
| 📊 | [**CPU meter**](MOFFE-EDITION.md#audio-cpu-meter-v109) | See how busy the audio engine is (IO menu), so you know how much room is left. |
| 🛟 | [**Safer saving and fixes**](MOFFE-EDITION.md#improvements-and-fixes) | A full card or a failed save no longer leaves half-written files, plus a stack of bug fixes. |
| 😎 | [**Boot screen**](MOFFE-EDITION.md#2-moffe-edition-boot-screen-v104) | Purely cosmetic. Deeply important. |

Full guide to every feature: **[MOFFE-EDITION.md](MOFFE-EDITION.md)** ·
Version history: **[CHANGELOG.md](CHANGELOG.md)**

## Install

### What you need

- A Drumboy Pro (factory firmware v1.01 or any Moffe Edition version).
- Its microSD card and a computer that can read it.
- About five minutes. No ST-Link, no soldering, no programming.

### Step 1 — Back up your SD card

Power off the Drumboy Pro, take out the microSD card, put it in your computer and copy **the whole
card** to a folder on your computer. Your projects (`System/File`), drumkits (`System/Drumkit`)
and settings (`System/Preset`) live there. Updating does not touch them, but a backup costs
a minute, and future you will be very grateful in the unlikely event it's needed.

### Step 2 — Download the right file

1. Open **[Releases → latest](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/releases/latest)**.
2. Scroll down to **Assets** and click **`Drumboy_Pro_Fw_Moffe_01_13.bin`** (the numbers are the
   version: `01_13` = v1.13).

What the files in Assets are:

| File | Who needs it |
|---|---|
| `Drumboy_Pro_Fw_Moffe_XX_YY.bin` | **Everyone.** This is the firmware update. |
| `Drumboy_Pro_Fw_01_01.bin` | Only if you want to go back to the factory firmware. |
| `*.sha256` | Optional, to check that the download is intact (see below). |
| `Drumboy-Pro-Moffe-Bootloader-vX.YY.bin` | **Not needed, and not for the SD card.** Advanced users with an ST-Link only. |
| Source code (zip / tar.gz) | Added automatically by GitHub. Not needed to install. |

<details>
<summary>Optional: check the download</summary>

Open the matching `.sha256` file in a text editor and compare its long number with the output of:

- **Windows:** `certutil -hashfile Drumboy_Pro_Fw_Moffe_01_13.bin SHA256`
- **macOS:** `shasum -a 256 Drumboy_Pro_Fw_Moffe_01_13.bin`
- **Linux:** `sha256sum -c Drumboy_Pro_Fw_Moffe_01_13.bin.sha256`

If they differ, download it again. The bootloader also checks a CRC before it writes anything, so
a damaged file is refused rather than installed.
</details>

### Step 3 — Copy it to the card

Copy the file to the **root** of the card: the top level, next to the `Sample` and `System`
folders, **not** inside them.

```
SD card
├── Drumboy_Pro_Fw_Moffe_01_13.bin   ← here
├── Sample/
└── System/
```

- **Keep the exact file name.** The bootloader only looks for names that start with
  `Drumboy_Pro_Fw` and are at most 32 characters long. It is very literal-minded. If your
  browser saved it as `Drumboy_Pro_Fw_Moffe_01_13 (1).bin`, rename it back.
- **Make it the only `Drumboy_Pro_Fw…bin` file in the root.** The bootloader installs the first
  one it finds.
- Don't change anything else on the card. Eject the card safely before you pull it out.

### Step 4 — Power on

Put the card back in the Drumboy Pro and switch it on. You should see:

1. **UPDATING FIRMWARE** with the version (e.g. **V1.13**) and a line that fills up.
2. **FIRMWARE UPDATED** for a few seconds.
3. The normal start-up, now with **sunglasses and MOFFE EDITION** on the logo. If you see the
   sunglasses, it worked. The sunglasses don't lie.

The bootloader then moves the package to `System/Firmware/` on the card, so the next start-ups are
normal and quick. That's it, go make some noise.

### If something goes wrong

| What you see | What it means | What to do |
|---|---|---|
| Normal start-up, no update screen, no sunglasses | The bootloader didn't find the file. | Check step 3: in the root, exact name, nothing in front of it. |
| **FIRMWARE ERROR** + **FORMAT-CHECK**, **SIZE-CHECK** or **CRC-CHECK** | The file is damaged, incomplete or not a firmware package (for example the bootloader file). Nothing was written, and your old firmware still runs. | Download the package again and repeat steps 3–4. |
| **FIRMWARE ERROR** + **VERIFY-CHECK** or **FLASH-CHECK**, or the screen stays dark after a failed update | Writing to the chip failed. | Copy the package to the root again and switch on. The bootloader always starts first and simply tries again. |

> [!NOTE]
> The factory bootloader **deletes** a package from the card when an update fails. If the file is
> gone after an error, that's why: copy it again.

Still stuck? Open an [issue](../../issues) with the firmware version and what the screen showed.

### Roll back to the factory firmware

Download **`Drumboy_Pro_Fw_01_01.bin`** from the release Assets (it is also in
[`02 Software/Firmware/`](<02 Software/Firmware>)) and install it exactly like steps 3–4, with the
Moffe package removed from the root. Your projects keep working; the factory firmware simply
ignores the new song-mode and preview settings. No hard feelings.

## Where to find things

| You want… | Go to |
|---|---|
| The firmware | [Releases → latest](https://github.com/daupuck/Drumboy-Pro-Moffe-Edition/releases/latest) → Assets |
| How to use the new features | [MOFFE-EDITION.md](MOFFE-EDITION.md) |
| What changed in each version | [CHANGELOG.md](CHANGELOG.md) |
| The original user manual | [randomwaves.io](https://www.randomwaves.io) (Randomwaves; describes the factory firmware) |
| Hardware details, schematics, memory map | [Developer Wiki](https://www.randomwaves.io/drumboy-pro/drumboy-pro-wiki.html) (Randomwaves) |
| To report a bug or suggest a feature | [Issues](../../issues) |
| To build it yourself or contribute | [DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>) · [CONTRIBUTING.md](CONTRIBUTING.md) |

---

## About the Drumboy Pro

*This section describes the Drumboy Pro hardware and firmware as published by Randomwaves.*

**An open-source hardware groovebox** — sequencer, sampler, and DSP effects engine built around an STM32H723 (Cortex-M7 @ 550 MHz), with a 540×960 touch-free color display, 42 keys, 8 rotary encoders, and full MIDI/Sync I/O.

This repository holds the complete hardware design (KiCad) and firmware (STM32CubeIDE / C++) for Drumboy Pro, released by [Randomwaves](https://randomwaves.io) as an open hardware/software project, together with the Moffe Edition firmware changes.

![Drumboy Pro](<01 Hardware/Drawing/Drumboy Pro.png>)

---

## Overview

The MCU sits at the center of six subsystems: an NT35510 display driven over a 16-bit parallel GPIO bus, 16 MB of external SDRAM for sample streaming, an SGTL5000 audio codec over I2S/I2C, a microSD card for storage, a control surface fanned out over six MCP23017 I/O expanders, and external connectivity (MIDI, Sync, USB-C power).

| | |
|---|---|
| **MCU** | STM32H723ZGT6 — Cortex-M7 @ 550 MHz (VOS0), LQFP-144 |
| **Display** | NT35510, 540×960, 16-bit parallel |
| **SDRAM** | 16 MB (IS42S16800J-7TL-TR), FMC bus |
| **Audio codec** | SGTL5000 — I2S3 audio / I2C3 control |
| **Storage** | microSD (TF-01A), SDMMC2, FAT filesystem |
| **Control surface** | 42 keys, 42 LEDs, 8 rotary encoders — 6× MCP23017 over I2C |
| **Connectivity** | MIDI In/Out, Sync In/Out, USB-C power |
| **Flash** | 1024 KB, split between bootloader and application |

## Features

- **10 sample layers**, each independently modulated by a 10-slot LFO matrix (sample-and-hold at note-trigger time)
- **Per-layer routing** into a shared DSP chain: parametric EQ → 2× multimode filter → 2× send effect → stereo reverb
- **9 effect types** per send slot: Delay, Chorus, Flanger, Phaser, Compressor, Expander, Overdrive, Distortion, Bitcrusher
- **Freeverb/Schroeder-Moorer style reverb** with pre-delay and stereo surround derivation
- **Sequencer** with 5 song banks per layer, 64 beats per bank, and algorithmic complex-fill generation (timing alignment × level curve templates)
- **MIDI In/Out** (opto-isolated input) and **Sync In/Out** for external clock/gate sync
- **SD-card firmware updates** via a dedicated bootloader with CRC-32 verified staged writes
- Fully open hardware (KiCad) and firmware (C/C++ against the STM32 HAL) — no CMSIS-DSP or external DSP library, all hand-rolled biquad/delay-line DSP on the Cortex-M7 FPU

## Repository structure

```
01 Hardware/
  Main/       KiCad project — main board (Drumboy_Main)
  Top/        KiCad project — top/control-surface board (Drumboy_Top)
  Drawing/    Mechanical drawings, enclosure models, graphics, fonts

02 Software/
  Drumboy-Pro-Boot-H723/   Bootloader — STM32CubeIDE project
  Drumboy-Pro-App-H723/    Application firmware — STM32CubeIDE project
  Firmware/                 Prebuilt firmware binaries + SD-package tools (make_fw_package.py)
  Sd Card/                  Reference SD card contents (samples, presets, drumkits, system data)
  Tests/                    Host unit tests (run in CI)
  DEVELOPMENT.md            Build, test and SD-card update workflow

MOFFE-EDITION.md, CHANGELOG.md              What the Moffe Edition adds, version history
CONTRIBUTING.md, NOTICE                     How to contribute; third-party licenses

CMakeLists.txt, CMakePresets.json, cmake/   Command-line build (same settings as the IDE)
.github/workflows/firmware.yml              CI: builds and verifies every push and pull request
.github/workflows/release.yml               Builds and publishes a release for every version tag
```

### Firmware projects

Firmware is split into two independently-built projects that share one flash device — a bad application build can never brick the update path. Both live in this repository, as in the upstream [Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro):

| Project | Responsibility |
|---|---|
| `02 Software/Drumboy-Pro-Boot-H723` | Checks the SD card for a firmware package, validates and programs it, then jumps to the application (only if the application image is valid) |
| `02 Software/Drumboy-Pro-App-H723` | The sequencer, DSP engine, UI, and all runtime behavior. The `Controller` class is split by subsystem across `Core/Library/Controller/Controller*.cpp`; the real-time audio path is in `ControllerAudio.cpp` |

## Hardware

Designed in **KiCad 7**. Open `01 Hardware/Main/Drumboy_Main.kicad_pro` for the main board, or `01 Hardware/Top/Drumboy_Top.kicad_pro` for the control-surface board. All third-party symbols, footprints, and 3D models are vendored under each project's `Library/` folder.

Mechanical design (enclosure and 3D model) lives in `01 Hardware/Drawing/` as **Rhinoceros 8** files — `Drumboy Pro 2D.3dm` and `Drumboy Pro 3D.zip` (zipped to keep it under GitHub's file size limit; unzip to get the `.3dm`).

A full BOM is generated from KiCad at `01 Hardware/Main/Production/bom.csv`. See the [wiki](https://www.randomwaves.io/drumboy-pro/drumboy-pro-wiki.html) for the annotated schematic walkthrough, clock tree, power-domain isolation scheme, and key component reference.

## Firmware

Built with **STM32CubeIDE**, written in C/C++ against the STM32 HAL.

- Bootloader owns flash sector 0 (`0x08000000`–`0x0801FFFF`)
- Application owns sectors 1–7 (`0x08020000`–`0x080FFFFF`), with the vector table relocated at startup

To flash over SWD, wire an ST-Link V2 to the board's `SWDIO` (PA13), `SWCLK` (PA14), and `GND` pins, then build and flash from STM32CubeIDE. See the wiki's [Build, Flash & Debug](https://www.randomwaves.io/drumboy-pro/drumboy-pro-wiki.html) section for details.

### Building from the command line

The same sources build without the IDE (this is what CI runs):

```sh
# needs Arm GNU Toolchain 13.3.rel1, CMake >= 3.22, Ninja, Python 3
export ARM_TOOLCHAIN_DIR=/path/to/arm-gnu-toolchain-13.3.rel1   # or put arm-none-eabi-gcc on PATH
cmake --preset firmware
cmake --build --preset firmware
```

This produces `build/Drumboy-Pro-Boot-H723.bin`, `build/Drumboy-Pro-App-H723.bin` and the SD-card update package `build/firmware/Drumboy_Pro_Fw_Moffe_MM_mm.bin` (version taken from `kVersionMajor`/`kVersionMinor` in the app's `Global.h`; the `Moffe` in the name keeps it apart from official Randomwaves packages).

See **[02 Software/DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>)** for the full workflow: making changes safely, testing a build from the SD card, and rolling back. Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md).

## License and credits

- Released under the [MIT License](LICENSE). The original Drumboy Pro hardware and firmware are
  © Randomwaves; the Moffe Edition changes are © daupuck. Both are covered by the same MIT license.
- Third-party components (STM32 HAL, CMSIS, FatFs, KiCad libraries) keep their own licenses — see
  [NOTICE](NOTICE).
- Sample library, fonts, drawings and the factory firmware binaries are as published by Randomwaves in
  [Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro).
- Thanks to Randomwaves for designing the Drumboy Pro and releasing it as open hardware and software.

## Community and support

- **Moffe Edition** (this firmware): use this repository's [Issues](../../issues).
- **Original Drumboy Pro** (hardware, factory firmware): Randomwaves —
  🌐 [randomwaves.io](https://www.randomwaves.io) · 💬 [Discord](https://discord.gg/5atevNH6UU).
  Please don't contact Randomwaves about problems with this modified firmware; they didn't make it,
  and it isn't fair to make them fix it.
