# Downwell — Amiga

An unofficial native Amiga port of **Downwell**, the 2015 action game by Moppin
(Ojiro Fumoto), published by Devolver Digital. Fall down a well, stomp and shoot
your way through its monsters with the gun boots, collect gems, and spend them in
the shops on the way down.

The port is written from the Windows Steam release's own program and data: its
GameMaker bytecode and resources were studied and reimplemented in C and C++ for
the Amiga, and the port is tested against them. It keeps the original's well,
art and palettes, and plays the sound effects on Paula.

Current Amiga release: **0.1 (26.11.2026), an early alpha**.

## What to expect from 0.1

0.1 is the **world 1 slice**: the game from its start to the end of level 1-3.
It is for people who want to try the port early and report what they find.

- **In this version:** the menus and the garden, the style select with all five
  styles, gameplay with the world's enemies, guns, combos and gem high, the shops,
  the side rooms with their time-stop, the upgrade choice between levels, death,
  results and Retry, hard mode, the pause menu with its palettes and options, and
  the saved game and records.
- **Not yet:** music; world 2 onwards. After level 1-3 the run ends at an
  end-of-slice screen.
- **Speed:** partly optimized. On the target machine gameplay averages about 30
  of its 50 updates a second: quiet stretches run at full speed, and the game
  slows down when the screen gets busy and in the side rooms. A faster processor
  helps. Full speed on a PAL Amiga is 50 updates a second, against the
  original's 60.
- **Known problems:**
  - the first level appears with a hard cut instead of the original's dithered
    reveal;
  - gems about to vanish can show as white outlines;
  - with many shotgun pellets in the air at once, the game can stop: it has
    room for 32 bullets.
- **Testing:** on an emulated A1200 with 4 MB Fast RAM, and on a real A1200 with a
  68040 at 40 MHz ([video](https://www.youtube.com/watch?v=4q3sZ8uSJLg)).

Please [report problems](https://github.com/Vesuri/downwell/issues) with what you
were doing when they happened.

## Requirements and installation

- Any Amiga with a 68020 or better, and Kickstart and Workbench 2.04 or newer.
  Its target machine, the one it is optimized and tested on, is an **A1200 with
  4 MB Fast RAM**.
- About 1 MB of free Chip memory and about 2.5 MB of further free memory (Fast or
  Chip). The game says so and does not start when it is not there.
- Installer 43 or newer and about 4 MB free on the hard disk.
- Your own copy of **Downwell for Windows from Steam** (app 360740), version
  1.0.5. Copy its `Downwell` drawer (`Steam\steamapps\common\Downwell`) to the
  Amiga or to a medium the Amiga can read.

Extract `Downwell-0.1.lha`, open its **Downwell Install** drawer and run
**Install**. Choose where the `Downwell` drawer goes, then select your Steam
Downwell drawer. Every file is checked, and other versions are refused. The
installer converts the game's graphics, rooms and sounds from your files into the
port's own data files: about an hour on an A1200, about eight minutes on a 50 MHz
68030. Updating keeps your saved game.

Start the installed **Downwell** icon, or `Downwell` from a Shell in its drawer.

No part of the original game is distributed: the archive holds only the port's
own files. See [the release ReadMe](release/ReadMe) for the full instructions.

## Controls

Joystick in port 2: left and right move, fire jumps and shoots (hold it to keep
shooting while you fall), and the second button cancels in the menus. On the
keyboard, X cancels, Escape or P pauses during play, and Escape quits elsewhere.

## Building

The Amiga build uses `m68k-amiga-elf-gcc`, `elf2hunk` and vasm, from AmigaXDev's
`make setup` (`~/.local/share/amiga/env.sh`). The tests and the data conversion
read your own Steam copy of the game, read-only, in `data/`; Git ignores it.

```sh
make extract           # the game's resources from data/, into extracted/ and generated/
make amiga             # the Amiga program: amiga/out/Downwell.exe
make release           # dist/Downwell-$(cat VERSION).lha, audited; clean tree only
make                   # the SDL host reference, then make run
```

How the port is built and tested is in the [development guide](docs/development.md),
the [release notes](docs/release.md) and the [project plan](PROJECT.md). Remaining
work is in [open-work.md](docs/open-work.md).

## Credits and licensing

Downwell and its original assets belong to Moppin and Devolver Digital. This is an
unofficial fan port, not affiliated with or endorsed by them. Amiga port by Vesa
Halttunen (Vesuri).

The installer's converter decodes the game's Ogg Vorbis sounds with Xiph.Org's
Tremor and libogg (BSD licences) and uses the AmigaXDev installer helpers
(LGPL-2.1-or-later); see [tools/install-data](tools/install-data). Those licences
apply to the converter only, not to the original game.
