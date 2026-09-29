# Generation preparation boundary

Preparation must remove repeated work, not run future gameplay earlier. Shared
RNG, Create callbacks and lifetime publication order remain authoritative.
Unfinished work remains in [open-work](open-work.md).

| Work | Safe preparation boundary | Live dependency |
| --- | --- | --- |
| Sprite descriptors, object defaults, local bounds | Asset load | Instance sprite, mask and transform overrides |
| Serialized room layers/reserved elements | Room entry after teardown | Surviving identities, room IDs, allocation state |
| Pattern symbols/relative geometry | Asset load or pre-level | Variant choice, reversal, world translation |
| Pattern/reward/enemy choices and random Create defaults | Original builder execution point | Shared RNG, health, style, preceding Create effects |
| Instance properties | Prepare immutable defaults | Lifetime, position, activation, overrides, family flags |
| Translated bounds | Prepare local geometry | Position, sprite/mask overrides, scale, angle |
| Registration, room order, layer attachment | Prepare lookup metadata | Membership, equal-depth order, IDs, retirement/reuse, failures |
| Spatial tree operations | Prepare queue/admission storage | Ordered history and next observing query |

`GameplayCollisionOwner` already prepares 741 sprite descriptors,
`PreparedObjectDefaults<386>` and `PreparedSpriteGeometry<741>` before births.
They are not decoded from assets at every birth. Another cache must identify
additional eliminated work rather than duplicate those tables.

`PreparedTerrainBirthBounds` prepares the three terrain defaults (60, 93, 99)
at owner construction, before any instance births. Its axis tables contain
binary32 positions and translated default collision bounds for the 16-pixel
grid, x=-256..752 and y=0..8176. Live birth performs lookup/copy instead of
integer encoding, float-to-integer position decoding and bounds translation.
Only initial default geometry uses this table: subsequent sprite/mask changes,
external moves and collision refreshes keep their ordinary live path. Unsupported
positions or unavailable default geometry fall back without modifying outputs.
No lifetime, graph preparation flag, RNG draw, Create setter, room order or
spatial publication moves earlier. The table occupies 16,134 bytes of ordinary
storage per 68000 owner (16,140 on the host, not Chip RAM), owned for the same lifetime as its immutable
resource defaults. Preparation latency still requires native measurement;
gameplay-only FPS does not measure that startup cost.

## Birth publication

`GameplayCollisionOwner::Binding::birth` resolves the generation-checked record,
establishes room-space coordinates, then performs:

1. `RunnerSpatialInstances::attachObjectPrepared`: attach lifetime, initialize
   properties, rebind registration, publish initial flags/queues.
2. `RunnerRoomMembership::bind`: insert using creation depth. Equal-depth order
   is observable and separate from layer-based Draw order.
3. Apply actor sprite/transform overrides; `publishBirth` refreshes bounds.
   Preparing only the default object's rectangle is insufficient.
4. `GameplayRoomLayers::birth`: adopt lifetime, resolve/reserve element, attach
   it and invalidate the affected Draw projection.

The callback returns before the builder continues emitting. A fused native birth
path may eliminate intermediate stores and redundant lookups within callback-free
intervals, but must preserve the next callback's observations and each failure's
partially published state. Moving Create effects to a later frame is not allowed.

Static terrain participates in activation, destruction, room order and layer
termination even when its bitmap is retained. Removing those records because
they do not move changes ownership semantics. Unnumbered records have source
ID zero; indexing serialized source IDs cannot accelerate those births.

## Terrain transaction boundaries

`AreaOneBuilder::emitPattern` reuses one local `DeferredCreateInstance` for
ordinary walls/platforms. Each callback borrows that command only until it
returns; subsequent cells overwrite its object and position. A preparation
queue must own any retained data, not store a pointer to this command. Actor,
reward and decoration commands still take `emitObject` and its live RNG path.

On Amiga, a terrain callback enters these distinct owners in order:

1. `AmigaGameplayGeneration::create` invokes the shared world transaction.
2. The backend callback appends contact geometry, updates cell/landing lookup,
   appends relevant drawable tiles and extends dirty-row coverage.
3. `GameplayTerrainInstances::append` allocates a lifetime and exposes the
   actual record to synchronous birth. Its binding attaches collision state,
   initializes prepared defaults, inserts room order, publishes bounds and
   attaches the layer element.
4. `createProperties` executes the source terrain Create setters, including
   sprite/index publication, before the next pattern cell can execute.

These are not repeated copies of one disposable render object. Their consumers
include contacts, landing, source collision selection, animation ordering,
activation, destruction and Draw. A compact terrain birth path must preserve
each consumer's observable publication and terminal-failure prefix while
eliminating intermediate calculation/lookup inside callback-free intervals.
Preparing default command payloads alone does not eliminate this owner work.

Layer ID and dynamic-depth resolution retain one last-query result each,
including misses. Consecutive attachments at the same depth can reuse the
lookup, but still execute their own lifetime/element publication. The dynamic
result is the first matching dynamic layer in source traversal order, never
an arbitrary equal-depth peer. Layer insertion, unlinking, room discard and
depth publication invalidate these results. Invalidation precedes a changed
depth because a merge-publication callback can inspect the partially updated
owner; subsequent relinking invalidates anything queried by that callback.
Failed merges keep the same observable partial state as the uncached walk.
The two key/slot/valid records use no per-instance storage and do not prepare
future layer identities early.

Generation profiling must include these callbacks, not just `emitObject`:
ordinary pattern walls/platforms bypass that command-construction bracket.
`GENERATION_DETAIL=1` separates backend cell, landing and tile work from the
outer registration scope; ordinary `PROBES` does not. Compare exclusive
brackets without adding nested inclusive totals, and use coarse generation
timing/ordinary FPS separately to check instrumentation distortion.

## Equivalence and measurement

`AreaOneBuilderTest` interleaves gameplay RNG and health changes with generation,
including failed Create callbacks. A prepared path must retain command order,
RNG state and partially advanced builder state at those boundaries. A fixed-seed
whole-level comparison is insufficient.

Owner changes also need retirement/reuse, equal-depth insertion, inactive-tail
reconciliation, room re-entry and partial-failure tests. Shared-core equivalence
does not prove complete original-game fidelity. Native replay must exercise the
changed owner path.

Compare identical natural routes and coverage, including bursts after frame 600.
Report preparation latency and ordinary/Chip memory separately from live-frame
cost. Do not sum nested inclusive timings or report probe-instrumented FPS as
shipping performance. See [profiling](amiga-profiling.md) and
[spatial scheduling](spatial-publication-scheduling.md).
