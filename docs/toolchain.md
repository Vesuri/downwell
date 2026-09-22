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

The baseline machine is an Amiga 500 with a 68000 and no FPU. Compile all
portable target code with `-m68000 -msoft-float`; any future routine using a
newer CPU must remain an optional, runtime-selected optimization rather than a
requirement. The emulator launcher deliberately defaults to an A500 model.

The link audit rejects compiler helpers for 64-bit multiplication, division,
modulo, and variable shifts, as well as recognizable post-68000 instruction
families in the final disassembly. Ordinary 32-bit division currently uses the
toolchain's 68000 implementation; hot occurrences can be removed later from
profile evidence without changing the compatibility baseline.

The current launcher gives that model 512 KiB of Chip RAM and an 8 MiB Fast RAM
expansion so CPU compatibility can be tested independently of the final memory
budget. `CHIP_MEMORY` and `FAST_MEMORY` can override those values. Reducing the
ordinary-memory footprint to a practical A500 configuration remains a separate
measured task.

`KICKSTART` defaults to `~/Documents/RetroPie/BIOS/kick31.rom` and may be
overridden before sourcing `env.sh`.

## GameMaker extraction

Use UndertaleModLib/UndertaleModTool as the primary format implementation and
retain its exact version or commit in the extraction manifest. Use DogScepter
as an independent parser where a second interpretation is valuable.

Bulk exported resources belong in `extracted/`; machine-generated manifests and
tables belong in `generated/`. Neither directory is committed by default.
