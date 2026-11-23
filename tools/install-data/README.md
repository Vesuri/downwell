# DownwellInstallData

The native converter the Installer runs (WI.4): it builds every installed
data file from the player's own Windows Steam copy of Downwell 1.0.5, checks
each input by SHA-256 first, and writes nothing it was not able to finish.
The same C sources build on the host, where `make test` compares each output
with the file the Python generators wrote in `generated/`, and on the 68020
(`make amiga`, after `. amiga/env.sh`).

Vendored, unmodified, pinned in `manifest/install-data.sha256`
(`make pinned` checks them before each build):

- `io.h`, `io_unix.c`, `io_amiga.c`, `sha256.c`, `sha256.h`, `start.s`,
  `COPYING.LIB`: AmigaXDev's shared install-data helpers
  (`~/.local/share/amiga/install-data`), as Prince of Persia 2 uses them.
- `tremor/`: Xiph.Org's Tremor, the integer-only Vorbis decoder, from
  `https://gitlab.xiph.org/xiph/tremor` commit `820fb3237ea8` (2025-04-03),
  BSD (`tremor/COPYING`). Only the decoder sources and headers are kept.
- `ogg/`: libogg 1.3.6 (2025-06-16), BSD (`ogg/COPYING`): `src/bitwise.c`,
  `src/framing.c`, `src/crctable.h` and `include/ogg/{ogg,os_types}.h`.
  `config/ogg/config_types.h` is ours (the configure step's output).

## Inputs it decodes

- The texture pages in `data.win` are not PNG. TXTR points at `2zoq`
  blobs: u16 width, u16 height, u32 decoded length, then a `BZh9` bzip2
  stream of a `fioq` image (GameMaker's early-draft QOI, with two's
  complement DIFF fields rather than QOI's bias). Such a decoder matches
  UTMT's exported PNGs on every pixel of both pages (587 KB and 930 KB
  decoded). A 900 KB bzip2 block takes about 2.25 MB in small-memory mode.
- Song and effect lengths need no decoder: the last Ogg page's granule
  position equals ffprobe's `duration_ts` for all 11 Ogg effects.
- The 11 Ogg effects decode with Tremor; the resampler is ours and its
  samples are the reference (the user's choice, 2026-11-23, in place of
  ffmpeg's `swr`). stb_vorbis with soft float took 83 minutes on the
  target for the effects alone, and its hashes differed.

## On the target

The converter is built with `-m68020-60 -msoft-float`: it needs a 68020 or
better (the user accepted that on 2026-11-23), and a 68060 runs it without
68060.library. The CMOT search (`exe.cpp`) runs src/core's binary64 multiply
and add through `binary64_amiga.s` (`DOWNWELL_MULTIPLY_BINARY64_HOOK`,
`DOWNWELL_ADD_BINARY64_HOOK`). The portable versions are the oracles:
`make amiga-multiply-test` with `time_amiga.py --multiply-test` finds no
difference in 258,564 cases each on the 68020 and the 68060. The multiply
uses `mulu.l` 32x32->64 only when exec reports a 68020 or 68030; a 68040 or
68060 (which Kickstart 3.1 reports as a 68040 without 68060.library) takes
the portable path.

`time_amiga.py SETUP` runs the `amiga-timing` build from the Shell with
`Stack 16384` over `data/` mounted read-only and compares the outputs with
the host converter's. On 2026-11-23 all 22 were equal on each setup, with
peak allocation 3,296,960 bytes and stack 6,370 of 16,384 bytes. 68 minutes
on `a1200-4m` is accepted (the user, 2026-11-23):

| Stage | `a1200-4m`, minutes | `a1200-030`, minutes |
| --- | ---: | ---: |
| exe (CMOT) | 21.1 | 4.3 |
| inputs (SHA-256 of the 12 files, 42.7 MB) | 15.7 | 1.7 |
| art | 14.5 | 2.3 |
| audio | 9.0 | 1.0 |
| derived | 6.1 | 1.1 |
| rooms, code, text, soundtables, masks, data.win | 1.6 | 0.5 |
| total | 68.0 | 10.9 |

`a1200-060` is not cycle-exact; its 22.9 emulated minutes are not a
measurement of a real 68060.
