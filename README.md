# Downwell — SDL reference and Amiga 500 port

Behavior-first reimplementation of the Windows Steam release of *Downwell*.
The project starts from GameMaker VM bytecode and resource data, produces an
auditable intermediate representation, implements a portable SDL reference,
and progressively lowers that implementation to native 68000 Amiga C/C++ and
selected 68000 assembly.

This repository does not distribute the original game. A locally owned Steam
build lives under `data/`, which is ignored by Git and treated as immutable.

See [remaining work](docs/open-work.md) for the single current work queue.
Completed changes are recorded in the git log.

## Startup shortcut

In the SDL window, press **F12** (Fn+F12 if required by the keyboard) to save
the current rendered framebuffer as a PNG in `~/Pictures/Screenshots/`.
Captures use the actual renderer resolution, including HiDPI scaling and
letterboxing, without the window border. Timestamped names never overwrite
existing captures. The save path or an error is printed to the console.

To build directly into STYLE SELECT for testing, skipping logos, credits,
controls and the garden:

```sh
make SKIP_INTRO=1                 # SDL
make run SKIP_INTRO=1             # build and run SDL
make amiga SKIP_INTRO=1           # Amiga
# Or, from amiga/: . ./env.sh && make SKIP_INTRO=1
```

The default is `SKIP_INTRO=0`. Switching the value rebuilds the target's objects
automatically. This sets `DOWNWELL_SKIP_INTRO`; it skips presentation, not the
asset/controller initialization required by later scenes. Style selection keeps
its normal entrance animation and confirmation behavior. Use normal startup
for garden profiling and intro replay checks. The headless regression check is
`make build/skip-intro-test && build/skip-intro-test`.

To skip directly into gameplay with Usual Style, including skipping STYLE SELECT:

```sh
make run SKIP_TO_GAME=1          # SDL
make amiga SKIP_TO_GAME=1        # Amiga
```

`SKIP_TO_GAME` defaults to `0` and takes precedence over `SKIP_INTRO` when
both are enabled. Normal controller/room initialization is preserved; the
player starts at the normal level entrance. Both toggles trigger automatic
rebuilds when changed. Use `SKIP_TO_GAME=0 SKIP_INTRO=0` for the full startup.
Headless check: `make build/skip-to-game-test && build/skip-to-game-test`.

To test the first level's completion sequence without playing the descent:

```sh
make run SKIP_LEVEL=1           # SDL
make amiga SKIP_LEVEL=1         # Amiga; then run with amiga/run.sh
```

`SKIP_LEVEL=1` implies direct gameplay startup and takes precedence over the
other skip options. It places the normal level-exit trigger at the player's
starting position: the exit alarms, wipe, completion screen and upgrade
selection still run normally. It does not generate an entire level offscreen
or fabricate score/rewards. Only the first level is skipped; after choosing
an upgrade and leaving the completion room, the next level plays normally.
The default is `0`; switching it automatically rebuilds both targets.
Use `SKIP_LEVEL=0 SKIP_TO_GAME=0 SKIP_INTRO=0` for normal startup.
Headless regression: `make test-skip-level`.

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

The host build uses `-O2` with debug symbols and floating-point contraction
disabled (no fast-math). For instruction-by-instruction debugging, use
`make HOST_OPT=-O0`. Changing this option automatically rebuilds the SDL objects.

The native player/scroll slice also has a real host-versus-emulated-A500 replay
gate. With the Amiga toolchain and FS-UAE installed, run
`make verify-cross-replay`; both targets must produce the pinned 64-frame
trajectory digest before the normal interactive Amiga executable is rebuilt.

Use Left/Right or A/D to move, Space or Z to jump/fire and confirm selections,
and Escape to quit. Use the startup shortcuts above for focused testing.

Remaining behavior, rendering, performance and verification work is tracked only
in [the work queue](docs/open-work.md). Implementation history and past test
results belong in the git log, not this build guide.

The cross-target replay and state-comparison contract is documented in
[docs/replay-format.md](docs/replay-format.md). Confirmed and still-provisional
GameMaker behavior is tracked separately in
[docs/runtime-semantics.md](docs/runtime-semantics.md).

The default, locked, and accepted style-selection states have emulator-backed
raster gates (`make verify-amiga-style-frame`,
`make verify-amiga-locked-style-frame`, and
`make verify-amiga-selected-style-frame`). The full scene verifier visits the
locked carousel entry before returning to and accepting the default style; the
last gate pins the hardware-dither transition into the well.

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
link to recovered GML and VM assembly for the native event dispatcher. The
portable reader validates inheritance and resolves inherited bindings without
host-endian or structure-layout assumptions.

Sprite/frame/atlas metadata has its own deterministic `DWSP` lowering,
documented in [docs/sprite-pack.md](docs/sprite-pack.md). The corresponding
decoded texture-page PNGs are produced by the guarded extraction and remain
ignored alongside other copyrighted derived data.

`make run` loads both packs. Implemented startup events are now selected through
their recovered object/event bindings and VM code IDs; resolved event calls
without a native translation remain visible in runtime diagnostics rather than
being mistaken for implemented behavior. `make verify-native-data` exercises
the complete room-to-object-to-code routing path.

## Amiga toolchain

```sh
. amiga/env.sh
make -C amiga
```

Or use `make amiga` from the repository root to build the native executable.

The project architecture and phase gates are in [PROJECT.md](PROJECT.md).
