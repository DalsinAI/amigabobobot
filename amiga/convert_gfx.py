#!/usr/bin/env python3
"""BoboBot's pictures for a 256-colour screen.

    python3 amiga/convert_gfx.py images amiga/images

BoboBot reads its 32x32 PPMs itself, rounds each colour down to a multiple
of 16 and asks X for it; pure white is see-through. Together the pictures
use 914 such colours, and the Amiga screen has 256. This writes the same
PPMs again, every colour one of 240 chosen for the whole set (a median cut
of all the pictures' pixels, rounded to multiples of 16 as the game rounds
them, so the game finds exactly these), white left white and black left
black (the title's 3D shapes treat black as empty). The other 16 colours
are the game's own: black, white and the nine of its meters.
Needs Pillow. The AmiCradle Team. GPL 2, like BoboBot.
"""
import sys
from pathlib import Path
from PIL import Image

COLOURS = 240
W = H = 32


def read_ppm(path):
    """As bobobot.c's loadobject: skip four whitespace characters (and
    comments), then 32x32 RGB triples."""
    d = path.read_bytes()
    i = ws = 0
    while ws < 4 and i < len(d):
        c = d[i:i + 1]
        if c == b"#":
            while i < len(d) and d[i:i + 1] not in (b"\n", b"\r"):
                i += 1
        elif c.isspace():
            ws += 1
        i += 1
    px = d[i:i + W * H * 3]
    if len(px) != W * H * 3:
        raise ValueError(f"{path}: short picture")
    return [tuple(px[k:k + 3]) for k in range(0, len(px), 3)]


def q16(c):
    return tuple(v // 16 * 16 for v in c)


def snap(v):
    return min(240, int(round(v / 16.0)) * 16)


def main(src, dst):
    src, dst = Path(src), Path(dst)
    pics = {p.name: read_ppm(p) for p in sorted(src.glob("*.ppm"))}
    seen = [q16(c) for px in pics.values() for c in px if c != (255, 255, 255)]
    strip = Image.new("RGB", (len(seen), 1))
    strip.putdata(seen)
    pal = strip.quantize(colors=COLOURS - 1, method=Image.Quantize.MEDIANCUT,
                         kmeans=8, dither=Image.Dither.NONE).getpalette()[:3 * (COLOURS - 1)]
    chosen = {tuple(snap(v) for v in pal[k:k + 3]) for k in range(0, len(pal), 3)}
    chosen.discard((0, 0, 0))
    chosen = sorted(chosen)
    assert len(chosen) + 1 <= COLOURS

    cache = {}

    def nearest(c):
        if c == (0, 0, 0):
            return c
        if c not in cache:
            cache[c] = min(chosen, key=lambda p: 2 * (p[0] - c[0]) ** 2 + 4 * (p[1] - c[1]) ** 2
                           + 3 * (p[2] - c[2]) ** 2)
        return cache[c]

    dst.mkdir(parents=True, exist_ok=True)
    used = set()
    for name, px in pics.items():
        out = bytearray(b"P6\n32 32\n255\n")
        for c in px:
            if c != (255, 255, 255):
                c = nearest(q16(c))
                used.add(c)
            out += bytes(c)
        (dst / name).write_bytes(bytes(out))
    print(f"{len(pics)} pictures, {len(set(seen))} colours made {len(used)}")


if __name__ == "__main__":
    main(*sys.argv[1:3])
