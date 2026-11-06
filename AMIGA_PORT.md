# BoboBot for AmigaOS 3.x

BoboBot is New Breed Software's platform game starring a robot monkey
(Bill Kendrick, Marianne Waage and Melissa Hardenbrook, 1998-2000, GPL 2),
from http://newbreedsoftware.com/bobobot/. This is its last release,
Preview 3, on AmigaOS 3.x, 68040 + FPU, built on acsdl12 (SDL 1.2 and
SDL_mixer on ACGame and AHI).

## Build

    make -f amiga/Makefile.amigaos \
        STOVE=/path/to/openamigagcc-16.2.0-open4-x86_64-linux \
        NDK=/path/to/gfopensdk/OpenNDK/Include_H \
        ACSDL=/path/to/amicradle-guest/gameports/acsdl12 \
        ACSDLLIB=/path/to/acsdl12/build \
        ACGAME=/path/to/acgame ACGAMELIB=/path/to/acgame/build-amiga

acsdl12 is built first with its own `Makefile.amigaos`, without its own
copy of ACGame (`ACG=`): the game links acgame.library's stubs
(`libacgame.a`). `package/build-amiga.sh` (STOVE and OUT set) makes the
release archive with its installer, fetching and building acgame,
acsdl12 and zlib at pinned commits. The Makefile gives
`build/amigaos/BoboBot/` (and `BoboBot.info` beside it): the program
`bobobot`, its `images`, `sounds`, `levels` and `mods` drawers, the HTML
manual in `docs`, this ReadMe and the licence, with icons.

## Installing

Unpack the archive anywhere (on AmiCradle: `SYS:Utilities/OpenCompress x
bobobot-...lha RAM:`), then double-click `Install-BoboBot` beside the
`BoboBot` drawer: it asks where to put the game (`Work:Games` by default)
and updates an earlier install in place (BoboBot saves nothing). The
drawer's `Libs` holds the acgame.library the game was built against, and
the game opens that copy first.

## Playing

Run `bobobot` from its drawer, from Workbench or a Shell; it finds its
files in PROGDIR:. 320x256 in 256 colours, AGA or RTG
(`ENV:ACGame/Display`). `bobobot --nomusic` or `--nosound` as upstream.

Cursor keys move (Up jumps too), Space jumps, Z fires, Tab, Esc or Help
pause and choose a weapon, Q quits (Y to confirm on the map). On the map,
the cursor keys choose a level and Space, Return or Z starts it; B goes
straight to its boss. A joystick or CD32 pad: the stick moves, fire fires
(and starts, on the title and map), the second button jumps, any other
button is Tab. How to play: `docs/index.html`.

## Changes for the Amiga

BoboBot draws with Xlib (X11) and uses SDL only for its sound. The
drawing is left as it is: `amiga/xsdl` is the part of Xlib it calls,
written on acsdl12's 8-bit screen:

- `X11/Xlib.h`, `Xutil.h`, `keysym.h`, `cursorfont.h` with X11's names
  and values, and `xsdl.c`: the window is the SDL screen, pixmaps are
  bytes in memory, one 256-colour map (black, white, the colours asked
  for while there is room, the nearest after), GCs with foreground,
  background, line width and a clip mask at a clip origin (the sprites'
  masks and the torchlight of the dark rooms), lines, thick lines, arcs,
  rectangles, filled polygons (the title's 3D shapes), XBM bitmaps, and
  X's "fixed" font (misc-fixed 6x13, public domain; `font6x13.h`, made by
  `amiga/make_font.py`). The window shows when the game looks for
  events, syncs or flushes, or sleeps out its frame.
- Keys come as X key events. Help is Tab; the joystick comes as keys
  too, in place of upstream's Linux `/dev/js0` code (not built).
- The frame clock (`gettimeofday` and `usleep`, 22.5 ms a frame) counts
  acsdl12's milliseconds: libnix's clock moves in 20 ms steps
  (`amiga/xsdl/xsdl_time.h`, included by `bobobot.c`).

Data:

- The 32x32 PPMs use 914 colours (after the game's own rounding to
  multiples of 16) and the screen has 256. `amiga/convert_gfx.py`
  (Pillow; run by hand, its output kept in `amiga/images`) writes them
  again in 237 colours chosen for the whole set, already rounded as the
  game rounds them; see-through white stays white and black stays black.
  They are still PPMs, read by the game as upstream.
- `mods/zak.mod` (the Moon level's tune) is really a Scream Tracker 3
  S3M, and acsdl12 plays MODs only; BoboBot will not start without all
  its tunes. `amiga/s3m2mod.py` made `amiga/mods/zak.mod` from it, a
  4-channel MOD with the S3M's two channels and samples as they were, at
  the periods that keep each sample's pitch; it carries the effects
  acsdl12's replayer plays (speed, tempo, break, volume and volume
  slides) and drops the rest (portamento, vibrato, offset). The other
  seven tunes are MODs (one 8-channel) and play as they are.

In `src/bobobot.c`, under `#ifdef __amigaos__`: DATA is `PROGDIR:` and
the paths after it take no `/` (`DATA_SEP`); there is no X display to
ask for; SDL_Quit runs at exit (BoboBot exits on any missing file with
AHI open). `amiga/stack.c`: a 128 KB stack.

Byte order: BoboBot reads its PPMs a byte at a time, and its levels and
the XBM are text; it reads nothing of more than a byte. The WAVs and MODs
are read by acsdl12, which swaps the WAVs' little-endian fields.

Sound is as upstream: 11025 Hz, mono on the Amiga.

State (4 November 2026): builds and links; not yet run in a lab or on a
real Amiga.
