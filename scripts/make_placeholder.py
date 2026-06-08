#!/usr/bin/env python3
"""Write a solid-color 400x300 PNG card placeholder using only the stdlib."""
import struct
import sys
import zlib
from pathlib import Path

WIDTH, HEIGHT = 400, 300
RGB = (42, 42, 42)  # #2a2a2a, matches the dark theme card background


def _chunk(tag: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def main() -> int:
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("placeholder.png")
    row = b"\x00" + bytes(RGB) * WIDTH  # filter byte 0 + RGB pixels
    raw = row * HEIGHT
    png = (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0))
        + _chunk(b"IDAT", zlib.compress(raw, 9))
        + _chunk(b"IEND", b"")
    )
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(png)
    print(f"wrote {out} ({len(png)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
