# Toolchain

## Host reference

The host build uses Clang, GNU Make, `pkg-config`, and SDL2:

```sh
make
make run
```

The build emits dependency files and mirrors the source tree under `build/`.

## Amiga cross-build

The port uses the shared BartmanAbyss Amiga GCC toolchain under `~/.local`,
together with `elf2hunk`, vasm, FS-UAE, and the matching GDB. AmigaXDev installs
it with the Kickstarts (`make setup`); `amiga/env.sh` sources its shared
`~/.local/share/amiga/env.sh`, and AmigaXDev's `docs/environment.md` describes
the variables (`docs/reference/port-toolchain.md` has the generic build rules).
Both are in the installed shared docs under `~/.local/share/amiga/docs/`; hardware manuals
are under `$ADCD`.
Activate it with:

```sh
. amiga/env.sh
make -C amiga
```

Use `make -C amiga STRIP=1` for a symbol-stripped `out/Downwell.exe`.
The default is `STRIP=0`; switching either way regenerates the executable
without requiring a clean build. The `.elf` retains its debugging information,
and `run.sh` launches the selected `.exe` as usual. Stripping does not change
the game's code/assets or establish that its runtime memory requirements fit.

The baseline machine is an Amiga 500 with a 68000 and no FPU. Compile all
portable target code with `-m68000 -msoft-float`; any future routine using a
newer CPU must remain an optional, runtime-selected optimization rather than a
requirement. The emulator launcher deliberately defaults to an A500 model.

The link audit rejects compiler helpers for 64-bit multiplication, division,
modulo, and variable shifts, as well as recognizable post-68000 instruction
families in the final disassembly. Ordinary 32-bit division currently uses the
toolchain's 68000 implementation; hot occurrences can be removed later from
profile evidence without changing the compatibility baseline.

The current launcher gives that model 1 MiB of Chip RAM behind the ECS
8372A Agnus (the target machine) and an 8 MiB Fast RAM expansion, so CPU
compatibility can be tested independently of the final Fast RAM budget. `CHIP_MEMORY` and `FAST_MEMORY` can override those values. Reducing the
ordinary-memory footprint to a practical A500 configuration remains a separate
measured task.

`KICKSTART` defaults to the shared Kickstart 3.1 (`kick40063.A600`) and may be
overridden before sourcing `env.sh`. The verification scripts are debug runs:
they are silent and open their window behind the others (`WINDOW=none` runs
without one). `amiga/run.sh` plays with sound in front; `DEBUG=1` adds the gdb
stub, and `SOUND`, `WINDOW` and the other shared FS-UAE options apply.
`run.sh` runs in `amiga/.run/instances/<INSTANCE>` (default `default`), with
its own drives, state, pidfile and gdb port. It only ever stops that
instance's previous emulator, and the verification and profile scripts (which
use `amiga/.run`) never stop it. `INSTANCE=b ./run.sh` runs a second emulator
beside the first. Checks clean and rebuild `amiga/out` in their own
configurations while they hold `tmp/amiga.lock`, so an instance runs its own
snapshot of `out/` (in its `build/`). It refreshes the snapshot only when the
lock is free; while a check holds it, it runs the previous snapshot and says
so. `DOWNWELL_EXE=path` runs that build directly instead.

## GameMaker extraction

Use UndertaleModLib/UndertaleModTool as the primary format implementation and
retain its exact version or commit in the extraction manifest. Use DogScepter
as an independent parser where a second interpretation is valuable.

Bulk exported resources belong in `extracted/`; machine-generated manifests and
tables belong in `generated/`. Neither directory is committed by default.
