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
