#!/usr/bin/env python3
"""Convert an RP2040 flash binary to UF2 without a native picotool dependency.

Format: https://github.com/microsoft/uf2 . This tool is RP2040-only.
The final two sectors of a standard 2 MiB Pico are reserved for the counter.
"""
import argparse
import pathlib
import struct


def convert(data: bytes) -> bytes:
    if not data or len(data) > 2 * 1024 * 1024 - 8192:
        raise ValueError("binary must fit the Pico application flash region")
    count = (len(data) + 255) // 256
    blocks = []
    for i in range(count):
        header = struct.pack("<8I", 0x0A324655, 0x9E5D5157, 0x2000,
                             0x10000000 + i * 256, 256, i, count, 0xE48BFF56)
        payload = data[i * 256:(i + 1) * 256].ljust(256, b"\xff")
        blocks.append(header + payload + bytes(220) + struct.pack("<I", 0x0AB16F30))
    return b"".join(blocks)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output.write_bytes(convert(args.input.read_bytes()))
