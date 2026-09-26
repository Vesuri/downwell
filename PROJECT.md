# Downwell port architecture

## Goal

Reproduce the behavior of the Windows Steam build of *Downwell* on an Amiga
500 with a 68000 CPU. Preserve behavior first, then replace generic GameMaker
semantics with simpler native structures whose equivalence is testable.

## Architecture

```text
immutable GameMaker inputs
        ↓
lossless extraction (resource manifest + VM assembly)
        ↓
auditable IR (normalized AST plus exact source references)
        ↓
portable deterministic core + SDL2 backend
        ↓
native Amiga backend and data conversion
        ↓
measured 68000 assembly twins
```

The IR has two complementary views:

1. A lossless view preserving bytecode instructions, resource ordering/IDs,
   object-event bindings, room data, and asset metadata.
2. A readable view containing decompiled GML or a normalized AST, with every
   node traceable back to the lossless view.

Neither generated C++ nor decompiled GML may silently become the only source of
truth.

## Directory ownership

- `src/core`: deterministic game state, GameMaker compatibility semantics,
  object/event scheduling, collision, room transitions, and generated tables.
- `src/platform/sdl`: windows, host input, presentation, host audio, diagnostic
  overlays, replay capture, and reference-state dumps.
- `src/platform/amiga`: display takeover, Copper/blitter/sprite use, Paula audio,
  native input, memory placement, and target-only optimized routines.
- `tools`: extraction, normalization, asset conversion, trace comparison, and
  manifest validation.
- `amiga`: cross-build and FS-UAE/GDB entry points.

## Correctness gates

- Hash and identify every input file before extraction.
- Preserve numeric resource IDs and serialized order.
- Retain VM assembly for every code entry, including entries whose high-level
  decompilation fails or looks suspicious.
- Drive host and Amiga builds with the same input/RNG replay format.
- Compare important state at fixed ticks: room, score, player state, RNG,
  active instances, spawn/destruction events, and collision outcomes.
- Treat visual comparisons as supporting evidence, not the sole oracle.
- Keep a portable implementation beside each assembly replacement and provide
  an in-process or replay-based differential test.

## Work tracking

[Remaining work](docs/open-work.md) is the only backlog. This document defines
the architecture, not a second project plan. Completed work and verification
results belong in the git log.
