# Contributing to Drumboy Pro – Moffe Edition

Thanks for your interest! This is unofficial community firmware based on
[Randomwaves/Drumboy-Pro](https://github.com/Randomwaves/Drumboy-Pro). Contributions are welcome
under the same [MIT license](LICENSE).

## Reporting problems and ideas

Use this repository's [Issues](../../issues) and the templates there. Please include the firmware
version and whether you use the factory or the Moffe Edition bootloader. Problems with the original
hardware or factory firmware belong with Randomwaves.

## Making changes

1. Fork the repository and create a branch from `main` (`feature/...` or `fix/...`).
2. Read [02 Software/DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>) first — it explains how the
   firmware runs (audio ISR budget, sequencer ISR, LCD, SD) and the rules that keep changes safe.
3. Build with `cmake --preset firmware && cmake --build --preset firmware` and make sure there are
   **no errors and no new compiler warnings**.
4. Put logic that needs no hardware in a small header under `Core/Library/Global/` and add a host
   test in `02 Software/Tests/test_*.cpp` (CI compiles and runs every one of them).
5. Do not change the real-time audio path (`ControllerAudio.cpp`) or SD file formats without
   explaining why in the pull request; mention the size of the `processAudio*` functions before and
   after if you touch them.
6. Update `CHANGELOG.md` and, for user-visible features, `MOFFE-EDITION.md`.
7. Open a pull request. CI must be green. Say what you tested **on a real Drumboy Pro** — untested
   changes can still be merged, but releases state what was hardware-tested.

## Releases

Maintainers bump `kVersionMinor` (and `kVersionMajor` if needed) in
`Drumboy-Pro-App-H723/Core/Library/Global/Global.h`, add a `CHANGELOG.md` section, then push a tag
`vX.YY` matching that version. The [Release](.github/workflows/release.yml) workflow builds that
commit and creates a **draft** release with the SD package, the bootloader binary and SHA-256
checksums; the maintainer adds the hardware-testing notes and publishes it.
