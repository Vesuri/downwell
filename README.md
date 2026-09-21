# Downwell — SDL reference and Amiga 68020 port

Behavior-first reimplementation of the Windows Steam release of *Downwell*.
The project starts from GameMaker VM bytecode and resource data, produces an
auditable intermediate representation, implements a portable SDL reference,
and progressively lowers that implementation to native Amiga C/C++ and
selected 68020 assembly.

This repository does not distribute the original game. A locally owned Steam
build lives under `data/`, which is ignored by Git and treated as immutable.

## Repository layout

```text
data/                 local Windows Steam build; read-only and never committed
src/core/             platform-independent game/reference logic
src/platform/sdl/     SDL2 host backend
src/platform/amiga/   native Amiga backend and hardware integration
amiga/                Amiga cross-build, emulator, and debugger entry points
tools/                GameMaker extraction and verification tools
docs/                 architecture, evidence, learnings, and work queue
extracted/            bulk GameMaker exports; generated and ignored
generated/            generated manifests/tables; ignored by default
captures/             reference captures and replay evidence; ignored
build/                host output; ignored
```

## Host smoke build

Requirements: a C++17 compiler, `pkg-config`, and SDL2.

```sh
make
make run
```

The current executable is deliberately only a platform-boundary smoke test. It
opens a logical 320x568 surface, runs a fixed 60 Hz update, and exercises the
input/render seam without incorporating recovered game code or assets.

Controls: left/right arrows move the marker, Space or Z toggles the input
indicator, Escape quits.

## GameMaker extraction

The extractor verifies the known Steam `data.win` hash before doing any work
and writes only to ignored `extracted/` and `generated/` directories:

```sh
make extract
```

Once an extraction exists, its manifest, code exports, failure reports, and
byte-identical container round trip can be checked independently:

```sh
make verify-extraction
```

The current verified result is summarized in
[docs/extraction-report.md](docs/extraction-report.md). Tool pinning and output
details are in [tools/README.md](tools/README.md).

## Amiga toolchain

```sh
. amiga/env.sh
make -C amiga
```

Or use `make amiga` from the repository root. The initial Amiga target checks
the shared cross-toolchain only; it intentionally does not claim that a native
game binary exists yet.

The project architecture and phase gates are in [PROJECT.md](PROJECT.md).
