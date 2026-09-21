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
increment and a deliberate golden-digest update. Startup-state schema 5 includes
the room-load result plus the stable instance list, transforms, creation-code
references, layer/depth state, and all twelve alarm slots for every instance.
The bounded-fixture startup replay has a pinned digest of `b24a4e764346cf26`
after 13 ticks and transition to `rmMenu`.

The current code provides memory encoding/decoding and state comparison. File
recording/playback in the SDL platform layer will be added once the shared RNG
and command-line runner are in place.
