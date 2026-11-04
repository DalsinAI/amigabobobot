#!/usr/bin/env python3
"""amiga/xsdl/font6x13.h from X's own "fixed" font (misc-fixed 6x13,
public domain), the font BoboBot asks X for:

    python3 amiga/make_font.py /usr/share/fonts/X11/misc/6x13-ISO8859-1.pcf.gz \
        > amiga/xsdl/font6x13.h

Characters 32 to 126, 13 rows each, one byte a row, the leftmost of the
6 pixels in bit 7. Needs Pillow. The AmiCradle Team. GPL 2, like BoboBot.
"""
import gzip
import sys
from PIL import PcfFontFile

f = PcfFontFile.PcfFontFile(gzip.open(sys.argv[1]))
print("/* misc-fixed 6x13 (\"fixed\"), public domain; made by amiga/make_font.py */")
print("#define XSDL_FONT_W 6\n#define XSDL_FONT_H 13\n#define XSDL_FONT_ASCENT 11\n#define XSDL_FONT_DESCENT 2")
print("static const unsigned char xsdl_font[95][13] = {")
for ch in range(32, 127):
    g = f.glyph[ch]
    rows = [0] * 13
    if g:
        (_, _), (x0, y0, x1, y1), _, im = g
        # dst box is relative to the baseline: row r of the image is at y0+r
        for y in range(im.size[1]):
            for x in range(im.size[0]):
                if im.getpixel((x, y)):
                    row = 11 + y0 + y
                    col = x0 + x
                    if 0 <= row < 13 and 0 <= col < 8:
                        rows[row] |= 0x80 >> col
    print("    {" + ",".join("0x%02x" % r for r in rows) + "}, /* %r */" % chr(ch))
print("};")
