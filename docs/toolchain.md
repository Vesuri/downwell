# Toolchain

## Host reference

The host build uses Clang, GNU Make, `pkg-config`, and SDL2:

```sh
make
make run
```

The build emits dependency files and mirrors the source tree under `build/`.

## Amiga cross-build

The sibling ports use the BartmanAbyss Amiga GCC toolchain installed under
`~/.local`, together with `elf2hunk`, vasm, FS-UAE, and the matching GDB.
Activate it with:

```sh
. amiga/env.sh
make -C amiga
```

The intended CPU is 68020 without an assumed FPU. When the native build lands,
compile portable code with `-m68020 -msoft-float` unless measured target hardware
requirements dictate otherwise.

`KICKSTART` defaults to `~/Documents/RetroPie/BIOS/kick31.rom` and may be
overridden before sourcing `env.sh`.

## GameMaker extraction

Use UndertaleModLib/UndertaleModTool as the primary format implementation and
retain its exact version or commit in the extraction manifest. Use DogScepter
as an independent parser where a second interpretation is valuable.

Bulk exported resources belong in `extracted/`; machine-generated manifests and
tables belong in `generated/`. Neither directory is committed by default.

