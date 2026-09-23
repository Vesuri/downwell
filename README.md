# Downwell — SDL reference and Amiga 500 port

Behavior-first reimplementation of the Windows Steam release of *Downwell*.
The project starts from GameMaker VM bytecode and resource data, produces an
auditable intermediate representation, implements a portable SDL reference,
and progressively lowers that implementation to native 68000 Amiga C/C++ and
selected 68000 assembly.

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

The native player/scroll slice also has a real host-versus-emulated-A500 replay
gate. With the Amiga toolchain and FS-UAE installed, run
`make verify-cross-replay`; both targets must produce the pinned 64-frame
trajectory digest before the normal interactive Amiga executable is rebuilt.

The current executable runs a fixed 60 Hz model of the recovered startup path:
the true `rmInit` entry room, desktop splash timing, and deterministic
transition to `rmMenu`. `make run` regenerates the native room pack and loads
the recovered room definitions, so the transition creates all 192 serialized
`rmMenu` instances alongside the persistent controller, then applies the room
code's runtime-created `ditherFade` as instance 105186. Most object event logic
is not translated yet. The program renders at GameMaker's native 160x284
application-surface size and now draws the recovered `sprPlayerIdle` frame
directly from its original texture atlas in the diagnostic menu view. Placement
remains centered until the recovered camera-spawn/follow behavior is translated.

Space or Z advances the splash sequence and confirms the unlocked style; the
recovered falling-dither transition then enters `rmMain`. Escape quits.
The first gameplay-room frame renders the recovered player and the room's
authoritative wall layout, including its derived four-neighbour tile frames.
The initial gameplay slice applies recovered subpixel gravity, airborne left
and right acceleration, wall collision bounds, fall animation, and camera bias.
Space or Z now fires the default eight-charge machine gun while airborne, with
the recovered seven-tick cadence, recoil, shoot pose, downward bullets, and
on-screen charge gauge.
The first-run `objBuilder` prelude now extends the recovered shaft walls ahead
of the descending camera; randomized room templates are not selected yet.
`make test` checks the automatic 731-tick desktop transition, input-driven
skipping, the non-desktop path, key recovered initialization state, alarms,
instance lifecycle, event phases, replay validation, and the state oracle
without SDL.
Room-entry tests also pin serialized creation order, persistence, lifecycle
phases, and atomic failure behavior.

Splash stages 2 through 6 now draw their recovered Devolver logo, full 4:3
credits composition, controls, and dither sprites. The Amiga credits are
precomposed once into the fixed HUD planes before presentation and displayed
with the wide fetch; they do not invoke a glyph compositor or repaint during
their three-second hold. The controls transition uses the recovered fractional
dither-frame sequence as a third hardware bitplane: the copper repeats one
8-line mask tile over the narrow display, so animation only swaps copper lists
and never repaints the controls raster.

The three nonblank native splash rasters have independent emulator-backed
regression gates. `make verify-amiga-splash-frame` pins the complete Devolver
publisher frame, `make verify-amiga-credits-frame` pins the wide credits, and
`make verify-amiga-controls-frame` pins the recovered controller/keyboard
instructions. In this Steam data, dither frame 11 is transparent, so the first
stage-5 frame is identical to stage 4 before the recovered frame index begins
to change.

The cross-target replay and state-comparison contract is documented in
[docs/replay-format.md](docs/replay-format.md). Confirmed and still-provisional
GameMaker behavior is tracked separately in
[docs/runtime-semantics.md](docs/runtime-semantics.md).

Both the default and locked style-selection states have emulator-backed raster
gates (`make verify-amiga-style-frame` and
`make verify-amiga-locked-style-frame`). The full scene verifier visits the
locked carousel entry before returning to and accepting the default style.

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

Or use `make amiga` from the repository root. The initial Amiga target checks
the shared cross-toolchain only; it intentionally does not claim that a native
game binary exists yet.

The project architecture and phase gates are in [PROJECT.md](PROJECT.md).
