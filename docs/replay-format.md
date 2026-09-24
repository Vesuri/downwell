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
| 4–7 | Reserved; write as zero |

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
increment and a deliberate golden-digest update. Startup-state schema 31
includes the room-load result, runtime instance state, gameplay/player/builder
state, and the complete Windows-runner RNG state and draw count. The bounded
startup replay seeded with `decafbad` has a pinned digest of
`9a7487de074ed927` after 13 ticks and transition to `rmMenu`.

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
Amiga must both report 64 frames and `9f0d3943e7900d2b`. The target then cleanly
rebuilds the ordinary joystick-driven executable, preventing verification flags
from leaking into an interactive build.

The gate also runs the same isolated 400-update `GemVerification` fixture on
host and target. Its schema-1 digest is `764abdd072bb4bbd` and required coverage
mask is `3f`: free motion, homing, acquisition, terrain contact, expiry and
attracted survival past expiry must all occur. Every update hashes authoritative
motion, lifetime, homing and animation fields plus RNG draw count/index; final
RNG words and the coverage mask are included. Collection is tested but not
applied, deliberately keeping gems alive to reach lifetime boundaries.

The full result line is:

```text
64 9f0d3943 e7900d2b 400 764abdd0 72bb4bbd 3f
```

Both digests and coverage must agree; a host fixture with missing coverage
fails before comparison. This is cross-CPU verification of the shared code,
not validation of its remaining approximations against Windows. The fixture
is compiled out of normal Amiga builds and does not affect frame time.
