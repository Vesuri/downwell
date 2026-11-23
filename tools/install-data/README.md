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
