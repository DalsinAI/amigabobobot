#!/usr/bin/env python3
"""BoboBot's Moon music, mods/zak.mod, is a Scream Tracker 3 module (S3M)
under a .mod name; acsdl12 plays ProTracker MODs only, and BoboBot will
not start without all its tunes. This writes it again as a 4-channel MOD:

    python3 amiga/s3m2mod.py mods/zak.mod amiga/mods/zak.mod

The song uses two channels. The samples are kept as they are (made
signed); each note gets the period that plays its sample at its own C4
speed (any period works in acsdl12's replayer, not only ProTracker's
three octaves). The effects acsdl12 plays are carried over: speed and
tempo (F), pattern break (D), volume slides (A, a D00 repeating the last),
set volume (C, from the volume column, or 0 for a note cut). The others
(portamento, vibrato, sample offset, fine slides) are dropped, as the
replayer would ignore them. Channel 4 starts the song at its speed.
The AmiCradle Team. GPL 2, like BoboBot.
"""
import struct
import sys

PAULA = 3546895


def bcd(v):
    return ((v // 10) << 4) | (v % 10) if v < 100 else 0


def main(src, dst):
    d = open(src, "rb").read()
    assert d[0x2c:0x30] == b"SCRM", "not an S3M"
    ordnum, insnum, patnum, flags, cwt, ffi = struct.unpack("<6H", d[0x20:0x2c])
    speed, tempo = d[0x31], d[0x32]
    o = 0x60
    orders = [x for x in d[o:o + ordnum] if x < 254]
    o += ordnum
    ipp = struct.unpack("<%dH" % insnum, d[o:o + 2 * insnum])
    o += 2 * insnum
    ppp = struct.unpack("<%dH" % patnum, d[o:o + 2 * patnum])

    samples = []
    for i in range(min(insnum, 31)):
        b = ipp[i] * 16
        if d[b] != 1:
            samples.append(None)
            continue
        memseg = (d[b + 13] << 16) | struct.unpack("<H", d[b + 14:b + 16])[0]
        ln, ls, le = struct.unpack("<3I", d[b + 16:b + 28])
        vol, sflags = d[b + 28], d[b + 31]
        c2 = struct.unpack("<I", d[b + 32:b + 36])[0] or 8363
        name = d[b + 48:b + 70].split(b"\0")[0]
        raw = d[memseg * 16:memseg * 16 + ln]
        if ffi == 2:
            raw = bytes(x ^ 0x80 for x in raw)
        if len(raw) & 1:
            raw += b"\0"
        loop = (ls, le) if sflags & 1 and le > ls + 2 else None
        samples.append(dict(data=raw, vol=min(vol, 64), c2=c2, loop=loop, name=name))
    for i in range(min(insnum, 31), insnum):
        assert d[ipp[i] * 16] != 1, "more than 31 samples"

    def period(note, ins):
        octave, semi = note >> 4, note & 15
        s = samples[ins - 1] if 0 < ins <= len(samples) else None
        c2 = s["c2"] if s else 8363
        freq = c2 * 2.0 ** ((octave - 4) + (semi / 12.0))
        return max(1, min(4095, int(round(PAULA / freq))))

    last_ins = [0] * 32
    last_d = [0] * 32
    mod_pats = []
    for pn in range(patnum):
        rows = [[(0, 0, 0, 0) for _ in range(4)] for _ in range(64)]
        if ppp[pn]:
            q = ppp[pn] * 16 + 2
            row = 0
            while row < 64:
                w = d[q]
                q += 1
                if w == 0:
                    row += 1
                    continue
                ch = w & 31
                note = ins = vol = cmd = info = None
                if w & 32:
                    note, ins = d[q], d[q + 1]
                    q += 2
                if w & 64:
                    vol = d[q]
                    q += 1
                if w & 128:
                    cmd, info = d[q], d[q + 1]
                    q += 2
                if ch >= 2:
                    continue
                per, eff, par = 0, 0, 0
                if ins:
                    last_ins[ch] = ins
                    assert ins <= 31
                if note is not None and note < 254:
                    per = period(note, ins or last_ins[ch])
                elif note == 254:
                    eff, par = 0xC, 0
                if cmd is not None:
                    c = chr(64 + cmd)
                    if c == "A" and info:
                        eff, par = 0xF, min(info, 31)
                    elif c == "T" and info >= 32:
                        eff, par = 0xF, info
                    elif c == "C":
                        eff, par = 0xD, bcd(info)
                    elif c == "B":
                        eff, par = 0xB, info
                    elif c == "D":
                        if info == 0:
                            info = last_d[ch]
                        last_d[ch] = info
                        hi, lo = info >> 4, info & 15
                        if lo == 0 and hi:
                            eff, par = 0xA, hi << 4
                        elif hi == 0 and lo:
                            eff, par = 0xA, lo
                if vol is not None and (eff in (0, 0xA)) and note != 254:
                    eff, par = 0xC, min(vol, 64)
                rows[row][ch] = (per, ins or 0, eff, par)
        mod_pats.append(rows)

    # start at the S3M's speed and tempo: channel 4 of the first pattern
    first = orders[0]
    mod_pats[first][0][3] = (0, 0, 0xF, speed)
    if tempo != 125:
        mod_pats[first][1][3] = (0, 0, 0xF, tempo)  # a row late; it is 125 here

    out = bytearray(d[0x00:0x14].split(b"\0")[0].ljust(20, b"\0")[:20])
    for i in range(31):
        s = samples[i] if i < len(samples) else None
        if not s:
            out += bytes(30)
            continue
        ls, ll = (s["loop"][0] // 2, (s["loop"][1] - s["loop"][0]) // 2) if s["loop"] else (0, 1)
        out += s["name"][:22].ljust(22, b"\0")
        out += struct.pack(">HBBHH", len(s["data"]) // 2, 0, s["vol"], ls, ll)
    out += bytes([len(orders), 127])
    out += bytes(orders).ljust(128, b"\0")
    out += b"M.K."
    for rows in mod_pats:
        for r in rows:
            for per, ins, eff, par in r:
                out += bytes([(ins & 0xF0) | (per >> 8), per & 0xFF, ((ins & 15) << 4) | eff, par])
    for s in samples:
        if s:
            out += s["data"]
    open(dst, "wb").write(bytes(out))
    print(f"{len(orders)} orders, {patnum} patterns, {sum(1 for s in samples if s)} samples, "
          f"speed {speed} tempo {tempo}, {len(out)} bytes")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
