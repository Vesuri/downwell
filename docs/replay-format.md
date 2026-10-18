# Deterministic replay and state digest

The SDL reference and Amiga target use the same deliberately small wire format.
It contains input levels and an RNG seed only; platform lifecycle events and
host timestamps never enter the simulation stream.

## Replay version 1

All multi-byte integers are unsigned little-endian values. Headers are exactly
16 bytes, followed by one byte per 60 Hz simulation tick.

| Offset | Size | Meaning |
| ---: | ---: | --- |
| 0 | 4 | ASCII magic `DWRP` |
| 4 | 2 | Format version, currently 1 |
| 6 | 2 | Tick rate, currently 60 |
| 8 | 4 | Frame count |
| 12 | 4 | Initial RNG seed |
| 16 | N | One packed input byte per frame |

Input bits are stable across targets:

| Bit | Input level |
| ---: | --- |
| 0 | Left |
| 1 | Right |
| 2 | Fire / confirm |
| 3 | Pause |
| 4 | Cancel (gamepad face 2) |
| 5–7 | Reserved; write as zero |

Bit 4 uses a previously zero reserved bit. Existing version-1 captures retain
their exact inputs; new Cancel captures require a reader with this extension.

`quit` is intentionally excluded because closing a host window is not game
behavior. Edge-triggered controls are derived from adjacent replay frames, just
as they are from adjacent live-input samples. Readers reject wrong magic,
version, tick rate, truncated payloads, and trailing bytes.

## State digest

`StateDigest` is 64-bit FNV-1a with every integer explicitly added in
least-significant-byte-first order. It never hashes raw structures, pointers,
padding, native booleans, or host-endian memory.

Each state family starts with its own schema number and appends fields in a
documented fixed order. Changing the field order or meaning requires a schema
increment and a deliberate golden-digest update. Startup-state schema 49
includes the room-load result, runtime instance state, gameplay/player/builder
state, and the complete Windows-runner RNG state and draw count. The bounded
startup replay seeded with `decafbad` has a pinned digest of
`bf4d5aed1cf0989e` after 13 ticks and transition to `rmMenu`. Schema 49 includes
stored projectile records and each live shot's lifetime link, in addition to
its binary32 sequence phase. Existing bullets advance before Begin Step,
while newborns retain frame zero.

The current code provides memory encoding/decoding, seeded RNG initialization,
state comparison, and a cross-target native-port runner. File recording and
general playback in the SDL platform layer remain to be added.

The freestanding Amiga build compiles the same `ReplayReader`, input packer,
and input unpacker, omitting only the host-side `std::vector` encoder. Live
port-1 joystick levels are packed and immediately decoded through this boundary
before use. A replay source can therefore replace the sampler without changing
the eventual native runtime step function. PAL still consumes one replay input
per displayed frame; it deliberately does not synthesize extra 60 Hz steps.

## Native cross-target gate

`src/core/PortReplay.h` contains a bounded 64-frame version-1 replay. It covers
jump edges, held fire, airborne gunboot shots, horizontal acceleration in both
directions, landing/recharge, bullet expiry, and 32 scroll-ring advances. The
shared `AmigaPortRuntime` folds a schema-3 checkpoint from every frame into a
rolling FNV-1a trajectory digest. Each checkpoint contains the packed input,
complete shared player state (including fractions, timers and animation),
scroll origin, and first-bullet activity/position. This is not a whole-world
checkpoint. A transient difference in the included fields cannot disappear
before the last frame.

Run the complete gate with:

```sh
make verify-cross-replay
```

The target builds a verification-only 68000 executable, boots it as an A500 in
FS-UAE, feeds the embedded bytes through the same `ReplayReader` used on the
host, and reads the completed digest at a named GDB breakpoint. The host and
Amiga must report identical frame counts, digests and coverage. The host runner
prints the expected result for the current source; do not use an older stored
transcript as the authority. The target then cleanly
rebuilds the ordinary joystick-driven executable, preventing verification flags
from leaking into an interactive build.

The gate also runs the same isolated 400-update `GemVerification` fixture on
host and target. Required coverage is defined by `gemVerificationCoverage`:
free motion, homing, acquisition, terrain contact, expiry and
attracted survival past expiry must all occur. Every update hashes authoritative
motion, lifetime, homing and animation fields plus RNG draw count/index; final
RNG words and the coverage mask are included. Collection is tested but not
applied, deliberately keeping gems alive to reach lifetime boundaries.

The gate also checks player death, machine-gun, binary64 motion, bone creation/
animation, spatial mutations and corpse execution. Inspect the complete current
host result with:

```sh
make build/port-replay-reference
./build/port-replay-reference
```

All digests and coverage must agree; a host fixture with missing coverage
fails before comparison. This is cross-CPU verification of the shared code,
not validation of its remaining approximations against Windows. The fixture
is compiled out of normal Amiga builds and does not affect frame time.

`CorpseVerification` runs 32 seeded owned creation/motion/animation/expiration
tapes, hashing published and precise positions, velocities, animation, alarms
and full RNG state, followed by the actual expiry child's animation lifetime.
Its 2,464 independent checks must report zero failures;
matching failure counts are not success. The standalone regression is
`make verify-corpse-replay`. Verification scratch storage uses aligned static
bytes and placement construction, not the small Amiga task stack or guarded
local-static initialization. The debugger checks the stack safety margin before
accepting results. This fixture uses controlled no-contact queries; it does not
prove live spatial integration, original-game fidelity or frame-budget fit.

The native gate retains a 256-byte stack canary and records the first failing
checkpoint without resetting it. Diagnostic stage numbers are: 1 startup/gameplay,
2 gems, 3 player death, 4 machine gun, 5 motion, 6 motion benchmark, 7 bones,
8 bone benchmark, 9 bone creation, 10 bone animation, 11 spatial mutations and
12 corpses. Both spatial and corpse fixtures reconstruct their scratch owners
in static storage; they are sequential test workloads, not reentrant APIs.
## Native frame diagnostics

The bounded native replay gate also compares a `FRAME` record for every input
tick. After the one-based frame number, twelve hexadecimal 32-bit words encode
the checkpoint high/low halves, player Q8 x/y and velocity x/y, ammo, grounded,
ring origin, first-bullet presence and first-bullet pixel x/y. Signed values
retain their two's-complement bits. The checkpoint remains the full shared
player-state check; these extra fields help localize a difference, not replace
the original-game behavioral evidence. Trace storage is test-only and kept off
the native task stack; tapes exceeding its capacity are rejected.

On a comparison mismatch, `amiga/verify_replay.sh` retains the failing executable,
ELF symbols, debugger output, expected/actual output and SHA-256 hashes in a
fresh ignored `amiga/.run/replay-failure.*` directory before normal-build
restoration can overwrite the image. Passing diagnostic builds do not alone
resolve a mismatch observed in another build.

To check whether collecting the trace changes a failure, build the native gate
with `REPLAY_VERIFY=1 REPLAY_FRAME_TRACE=0` and run `amiga/verify_replay.sh`
with `REPLAY_FRAME_TRACE=0` in its environment. This removes native per-frame
capture and storage, but retains every aggregate and component check. The host
runner's matching switch is `--no-frame-trace`. The build and verification
settings must agree; tracing defaults to enabled. Neither mode changes normal
game builds.
