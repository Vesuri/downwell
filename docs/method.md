# Porting method and inherited lessons

This project adopts the parts of the Rescue on Fractalus, Attack of the PETSCII
Robots, Revs, and Vette workflows that transfer to a GameMaker VM source.

## The reference hierarchy must stay explicit

The Windows build and its VM bytecode are the behavioral authority. Decompiled
GML is a readable hypothesis. The SDL implementation is a convenient executable
model. The Amiga implementation is the shipping target. A downstream layer
cannot prove itself by agreeing only with another downstream approximation.

## Preserve a lossless seam before rewriting

Store exact VM assembly and resource metadata next to normalized output. That
is the equivalent of keeping the original 6502/68000 instruction stream beside
a transliteration: it makes decompiler errors, coercions, and event-order
assumptions auditable.

## Separate shared behavior from platform machinery

Keep SDL and Amiga display, audio, timing, input, storage, and lifecycle code
behind a small boundary. Do not let the host backend's conveniences leak into
game state or simulation semantics.

## Determinism is the cheapest useful oracle

Use fixed ticks, controlled RNG seeds, recorded input edges, and compact state
digests. Compare persistent state and important events, not only screenshots.
Every negative observation needs a positive control proving the relevant code
path ran.

## Optimize only a measured target

Before changing a representation, state what the cost is proportional to and
measure the old complete path. Preserve a portable C/C++ twin for assembly and
specialized Amiga routines. Prefer in-process differential checks; cross-run
timing comparisons easily change the workload.

## Generated and source material have different ownership

Commit extraction code, schemas, hand-authored tables, and small manifests.
Keep the original build, exported textures/audio, captures, emulator state, and
byte-for-byte derived assets local and ignored.

