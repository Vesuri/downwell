# Shared simulation contract

Both backends must execute the same portable game rules in `src/core`.
The Windows VM and serialized resources are authoritative; matching SDL and
Amiga output alone does not establish original-game fidelity.

## Ownership

- `GameplayWorld` owns portable world simulation. `AmigaPortRuntime` is a
  compatibility alias, not an independent rules implementation.
- `PlayerMotion` owns player state. Presentation mirrors must not become a
  second writer of authoritative state.
- World adapters provide instance lookup, collision queries, coordinate
  projection and presentation hooks. They must not introduce different rules.
- Bind stack-local callback contexts only for the operation that uses them;
  detach them before returning, copying state or resetting a world.
- `Profile.h` defines hardware-free instrumentation. Native timing and
  measurement storage belong in the Amiga backend.

## Ordering and state transfer

Creation callbacks are synchronous. Preserve child Create events, RNG draws,
source-owner mutations, reward emission and retirement order. Rendering and
storage optimizations must not reorder these operations.

A room-entry handoff transfers generator state and the matching full RNG state
before generation begins. It is not a save-state loader for an already populated
world and must not replay Create events.

Use source-backed numeric semantics where rounding changes decisions, collisions
or animation. A shared fixed-point approximation is not automatically faithful
to GameMaker floating-point behavior.

## Verification boundaries

`tools/verify-core-world.py` checks transitive dependency boundaries. Component
tests check individual contracts; deterministic world replays check integrated
state and RNG; native execution checks target arithmetic and rendering.
None replaces comparison with original-runner behavior.

The sole migration backlog is [remaining work](open-work.md#6-finish-scene-and-backend-separation).
Implementation chronology and past test results belong in the git log.
