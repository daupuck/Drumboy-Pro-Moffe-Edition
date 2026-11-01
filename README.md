# Drumboy Pro – Moffe Edition

**Unofficial community firmware for the Randomwaves Drumboy Pro**, based on the open-source
[Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro) project.

> [!IMPORTANT]
> This is **not an official Randomwaves release** and is **not affiliated with or endorsed by
> Randomwaves**. "Drumboy Pro" is Randomwaves' product name, used here only to say which device this
> firmware is for. Please report problems with this firmware in this repository's
> [Issues](../../issues), **not** to Randomwaves.

> [!WARNING]
> **Use at your own risk.** This firmware is provided "as is", without warranty of any kind (see
> [LICENSE](LICENSE)). Installing it is your own decision and responsibility. Before you install:
> **back up your whole SD card** — it holds your projects, drumkits and settings — and keep the
> factory firmware for rolling back (see [Install and roll back](MOFFE-EDITION.md#install-and-roll-back)).
> Flashing the **bootloader** over SWD is for experienced users only. Modifying the firmware may
> affect your warranty with Randomwaves.

✨ **[What the Moffe Edition adds](MOFFE-EDITION.md)** — Euclidean rhythms, song mode, precise beat
placement, sample preview, safer firmware updates and many fixes. Version history:
[CHANGELOG.md](CHANGELOG.md). Downloads: [Releases](../../releases).

📖 **[Developer Wiki](https://www.randomwaves.io/drumboy-pro/drumboy-pro-wiki.html)** (Randomwaves) — full hardware specs, schematics walkthrough, firmware architecture, DSP internals, and memory map.

📘 **User manual** — published by Randomwaves at [randomwaves.io](https://www.randomwaves.io). It describes the factory firmware; [MOFFE-EDITION.md](MOFFE-EDITION.md) describes what is different here.

---

*The rest of this page describes the Drumboy Pro hardware and firmware as published by Randomwaves.*

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
  🌐 [randomwaves.io](https://www.randomwaves.io) · 💬 [Discord](https://discord.gg/5atevNH6UU) ·
  ✉️ hello@randomwaves.io. Please don't contact Randomwaves about problems with this modified firmware.
