#!/usr/bin/env python3
"""Build or verify a Drumboy Pro SD-card firmware update package.

Package layout (little-endian), as read by the bootloader (Drumboy-Pro-Boot-H723,
Controller::updateFirmware):

    offset  size  field
    0       24    "RW_DRUMBOY_PRO_FIRMWARE "  (ASCII, no NUL)
    24      2     version major  (u16, < 10)
    26      2     version minor  (u16, < 100)
    28      4     payload size   (u32, 1 .. 917504)
    32      4     CRC-32 of payload (zlib / ISO-3309)
    36      n     payload = raw application image linked at 0x08020000

Copy the resulting Drumboy_Pro_Fw_MM_mm.bin to the root of the SD card and power on.

Examples:
    make_fw_package.py Drumboy-Pro-App-H723.bin --version 1.02
    make_fw_package.py Drumboy-Pro-App-H723.bin --version-header .../Global/Global.h --out-dir build/firmware
    make_fw_package.py --verify Drumboy_Pro_Fw_01_02.bin

This replaces nothing: Drumboy-Pro-Firmware-Builder.cpp still works and produces the
same bytes. This script adds the safety checks the bootloader performs, so a package
that would be rejected (or brick the update) is refused up front.
"""

import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

HEADER_TITLE = b"RW_DRUMBOY_PRO_FIRMWARE "
HEADER_SIZE = len(HEADER_TITLE) + 12  # 36

APP_FLASH_START = 0x08020000
APP_FLASH_END = 0x08100000  # exclusive
MAX_PAYLOAD = APP_FLASH_END - APP_FLASH_START  # 917504 bytes (896 KB)

# RAM regions a valid initial stack pointer may point into (end address inclusive),
# matching Controller::isApplicationValid() in the bootloader.
STACK_REGIONS = [(0x20000000, 0x20020000), (0x24000000, 0x24050000)]


class PackageError(Exception):
    pass


def check_payload(payload: bytes) -> None:
    """Raise PackageError if the bootloader would reject or mis-handle this image."""
    if not 0 < len(payload) <= MAX_PAYLOAD:
        raise PackageError(f"payload is {len(payload)} bytes; must be 1..{MAX_PAYLOAD}")
    if len(payload) < 8:
        raise PackageError("payload too small to contain a vector table")

    sp, reset = struct.unpack_from("<II", payload, 0)
    if sp & 0x7 or not any(lo <= sp <= hi for lo, hi in STACK_REGIONS):
        raise PackageError(
            f"initial stack pointer 0x{sp:08X} is not in DTCM/AXI-SRAM; "
            "is this really the application image?")
    reset_addr = reset & ~1
    if not reset & 1 or not APP_FLASH_START + 8 <= reset_addr < APP_FLASH_END:
        raise PackageError(
            f"reset handler 0x{reset:08X} is not a Thumb address inside "
            f"0x{APP_FLASH_START:08X}..0x{APP_FLASH_END:08X}; the image must be linked "
            "for the application region (STM32H723ZGTX_FLASH.ld of the App project), "
            "not the bootloader")


def parse_version(text: str) -> tuple[int, int]:
    m = re.fullmatch(r"(\d+)\.(\d{1,2})", text.strip())
    if not m:
        raise PackageError(f"version must look like 1.02, got {text!r}")
    major, minor = int(m.group(1)), int(m.group(2))
    return major, minor


def version_from_header(path: Path) -> tuple[int, int]:
    src = path.read_text(encoding="utf-8", errors="replace")

    def grab(name: str) -> int:
        m = re.search(rf"\b{name}\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;", src)
        if not m:
            raise PackageError(f"{name} not found in {path}")
        return int(m.group(1), 0)

    return grab("kVersionMajor"), grab("kVersionMinor")


def check_version(major: int, minor: int) -> None:
    if not (0 <= major < 10 and 0 <= minor < 100):
        raise PackageError(f"version {major}.{minor:02d} out of range (major < 10, minor < 100)")


def build(payload: bytes, major: int, minor: int) -> bytes:
    check_version(major, minor)
    check_payload(payload)
    crc = zlib.crc32(payload) & 0xFFFFFFFF
    return HEADER_TITLE + struct.pack("<HHII", major, minor, len(payload), crc) + payload


def verify(package: bytes) -> tuple[int, int, int, int]:
    """Validate a package exactly like the bootloader does. Returns (major, minor, size, crc)."""
    if len(package) < HEADER_SIZE or package[:len(HEADER_TITLE)] != HEADER_TITLE:
        raise PackageError("not a Drumboy Pro firmware package (bad header)")
    major, minor, size, crc = struct.unpack_from("<HHII", package, len(HEADER_TITLE))
    check_version(major, minor)
    if size + HEADER_SIZE != len(package):
        raise PackageError(f"size field says {size} bytes but file holds {len(package) - HEADER_SIZE}")
    payload = package[HEADER_SIZE:]
    actual = zlib.crc32(payload) & 0xFFFFFFFF
    if actual != crc:
        raise PackageError(f"CRC mismatch: header 0x{crc:08X}, payload 0x{actual:08X}")
    check_payload(payload)
    return major, minor, size, crc


def package_name(major: int, minor: int) -> str:
    # Must match the bootloader's search pattern "Drumboy_Pro_Fw?*.bin".
    return f"Drumboy_Pro_Fw_{major:02d}_{minor:02d}.bin"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="application .bin to package, or package to check with --verify")
    ap.add_argument("--verify", action="store_true", help="check an existing package instead of building one")
    src = ap.add_mutually_exclusive_group()
    src.add_argument("--version", help="package version, e.g. 1.02")
    src.add_argument("--version-header", type=Path, help="read kVersionMajor/kVersionMinor from this Global.h")
    ap.add_argument("--out-dir", type=Path, default=Path("."), help="output folder (default: current)")
    args = ap.parse_args(argv)

    data = Path(args.input).read_bytes()
    try:
        if args.verify:
            major, minor, size, crc = verify(data)
            print(f"OK  {args.input}: v{major}.{minor:02d}, {size} bytes, CRC 0x{crc:08X}")
            return 0

        if args.version:
            major, minor = parse_version(args.version)
        elif args.version_header:
            major, minor = version_from_header(args.version_header)
        else:
            ap.error("give --version or --version-header")

        pkg = build(data, major, minor)
        verify(pkg)  # round-trip through the same checks the bootloader runs
        args.out_dir.mkdir(parents=True, exist_ok=True)
        out = args.out_dir / package_name(major, minor)
        out.write_bytes(pkg)
        print(f"Wrote {out} (v{major}.{minor:02d}, {len(data)} bytes payload, "
              f"CRC 0x{zlib.crc32(data) & 0xFFFFFFFF:08X})")
        return 0
    except PackageError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
