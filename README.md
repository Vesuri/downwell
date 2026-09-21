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

## Host reference build

Requirements: a C++17 compiler, `pkg-config`, and SDL2.

```sh
make
make test
make run
```

The current executable runs a fixed 60 Hz model of the recovered startup path:
the true `rmInit` entry room, desktop splash timing, and deterministic
transition to `rmMenu`. `make run` regenerates the native room pack and loads
the recovered room definitions, so the transition creates all 192 serialized
`rmMenu` instances alongside the persistent controller. Their object event
logic is not executed yet. The program renders an asset-free diagnostic at
GameMaker's native 160x284 application-surface size; original art is not yet
loaded.

Space or Z advances the splash sequence, and Escape quits. `make test` checks
the automatic 731-tick desktop transition, input-driven skipping, the
non-desktop path, key recovered initialization state, alarms, instance
lifecycle, event phases, replay validation, and the state oracle without SDL.
Room-entry tests also pin serialized creation order, persistence, lifecycle
phases, and atomic failure behavior.

The cross-target replay and state-comparison contract is documented in
[docs/replay-format.md](docs/replay-format.md). Confirmed and still-provisional
GameMaker behavior is tracked separately in
[docs/runtime-semantics.md](docs/runtime-semantics.md).

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

The audited room metadata can then be lowered into the ignored native room pack:

```sh
make generate-room-pack
make verify-room-pack
```

Its endian-explicit format is documented in
[docs/room-pack.md](docs/room-pack.md). `make verify-room-pack` checks both
deterministic generation, the portable C++ decoder, and startup entry into the
fully populated recovered `rmMenu` state.

The complete object/property/event graph has a separate native lowering:

```sh
make generate-object-pack
make verify-object-pack
```

The `DWOB` format is documented in
[docs/object-pack.md](docs/object-pack.md). Its code-entry IDs retain an exact
link to recovered GML and VM assembly for the native event dispatcher.

## Amiga toolchain

```sh
. amiga/env.sh
make -C amiga
```

Or use `make amiga` from the repository root. The initial Amiga target checks
the shared cross-toolchain only; it intentionally does not claim that a native
game binary exists yet.

The project architecture and phase gates are in [PROJECT.md](PROJECT.md).
