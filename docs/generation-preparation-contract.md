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
at owner construction, before any instance births. Its shared offset table contains
binary32 positions and translated default collision bounds for the 16-pixel
grid, x=-256..752 and y=0..8176. Live birth performs lookup/copy instead of
integer encoding, float-to-integer position decoding and bounds translation.
Only initial default geometry uses this table: subsequent sprite/mask changes,
external moves and collision refreshes keep their ordinary live path. Unsupported
positions or unavailable default geometry fall back without modifying outputs.
No lifetime, graph preparation flag, RNG draw, Create setter, room order or
spatial publication moves earlier. All axes and objects share up to four distinct
coordinate offsets, including zero for the position. The pinned resources fit
this dictionary; a changed resource requiring more offsets takes the general
path without consuming dictionary slots. The table occupies 8,482 bytes of
ordinary storage per 68000 owner (8,484 on the host, not Chip RAM), owned for
the same lifetime as its immutable resource defaults. The FPS profiler reports
startup preparation separately using the OS field counter before takeover;
gameplay-only FPS does not measure that startup cost.

The four prepared integer offsets are retained alongside their binary32 table.
An admitted terrain birth supplies both representations to the collision owner,
so journal admission need not decode those binary32 endpoints back to integers.
`refreshPreparedTerrain` retains ordinary bounds/flag publication, normalization,
and removal before insertion; nonscheduled owners use the reference refresh.
`RunnerSpatialJournal::insertPrepared` borrows the exact decoded endpoints of
the supplied rectangle. This is a trusted geometry contract, not a general
unvalidated-coordinate entry point: the terrain preparer admits only exact
integer positions/offsets and positive zero. The caller normalizes the integer
endpoints in the same way as the publication wrapper normalizes the binary32
rectangle. The journal still evaluates current membership, population limits,
historical union area and queue pressure at each call. It stores the original
binary32 operation snapshot, so later tree servicing/query order and eager
fallback remain unchanged. No lifetime, query-preparation flag, Create effect
or random draw moves into preparation.

## Birth publication

The terrain pool constructor prepares every unused record's default state.
First use writes only the birth-specific identity, coordinates, activation and
source binding before synchronous publication; `objectId == 0` identifies these
untouched slots because admitted terrain objects are 60, 93 and 99. Retirement
and room clearing retain the nonzero object ID, so recycled slots take the full
reset path. Wall state is not cleared again by Create unless room creation or
End Step has touched it. The dirty marker is set before those callbacks, so a
repeated or nested Create still resets state at its original completion point.
This does not preallocate observable lifetimes or advance gameplay callbacks.

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

Room traversal obtains the current handle from the collision registry rather
than maintaining a second per-slot identity array. The gameplay owner privately
owns both registries: birth attaches collision state before room binding;
retirement removes collision state before room unlinking. These intervals have
no external dispatch, and failure prevents further world dispatch. A successful
room-list visit therefore refers to the same live generation. This applies only
to synchronous slot walks, not saved event snapshots, query results or layer
elements: those retain their original generation-bearing handles so reuse cannot
retarget them. `currentHandle` returns no handle for retired or out-of-range
slots, but still returns the lifetime of a deregistered/inactive object.

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

### Batching admission

Ordinary wall/platform Create bodies contain local property stores only; wall
glitch mode adds `choose`. That does **not** make consecutive plain cells a
draw-free run. `builderCavern` block 12 consumes `random(level+2)` before the
symbol dispatch at block 19 for every cell, including `1`, `|` and `-`.
Hard mode can consume an additional word in block 14. The pinned terrain
verifier checks these blocks as well as the Create bodies.

A future batch interface must retain **cell RNG -> complete that birth/Create
-> next cell RNG**, even if its backend shares resource lookups and reserves
storage once for the run. It must not first materialize random choices for the
whole span. The terrain test includes an eager-span counterexample: final RNG
state and total draw count agree, but glitch-wall artwork differs. Per-birth
outputs/observations, not just end-of-span RNG state, are required evidence.

The generic emitter remains synchronous and may consume RNG, change health or
fail after partial effects. A batching API therefore needs an explicit ordered
per-cell execution contract, with scalar fallback for unsupported cases. Do not
reinterpret the current borrowed single-command callback as a buffered queue.
Room creation code, shops/exits, actors, conditional cell choices and row-edge
RNG must retain their source boundaries. No bulk publication path is enabled by
this audit; safe publication/failure preflight still requires implementation
and equivalence tests against the existing immediate executor.

`AreaOneBuilder::TerrainRun` exposes a borrowed same-row sequence of ordinary
walls (`1`/`|`) or thin platforms (`-`) in normal cavern levels 1–3. Runs never
cross another symbol or a row boundary. `execute` consumes the per-cell RNG
word immediately before invoking that cell's synchronous emitter, and stops
at its first failure. The callback's command remains borrowed only until it
returns. An optional run emitter can prepare backend lookup/storage from the
run geometry, then must call `execute` exactly once and propagate its result;
it must not retain the run or command. Configure the emitter after builder
reset, which resets all builder state, and explicitly enable terrain runs.
The core defaults to scalar execution, selected once per pattern rather than
adding run detection to every scalar cell. Without a run emitter, the original
single-instance callback receives the run's cells in order. Disabling terrain
runs retains the scalar dispatch as the test reference. Neither path changes
instance publication timing.
A run rejects a second execution, including after failure, without additional
RNG or callbacks. Reporting success without completing the run is rejected.

The Amiga generation observer opts into these runs. The shared world's
`createGeneratedTerrain` transaction performs backend publication, lifetime
creation and terrain Create without dispatching unrelated shop/actor types;
the generic creation entry point delegates to the same transaction for terrain.
It reads the area's tile after the backend callback, retaining that callback's
observation/mutation boundary. `AmigaTerrainRunIndex`
admits the complete row geometry without writing anything, sharing object-cell
flags and row/column setup across its cells. Unsupported geometry takes the
scalar emitter, rather than failing early and losing a successful prefix. Each
cell still publishes contacts, occupancy, landing candidates and tile storage
before its core birth/Create. Contact or tile capacity failures therefore retain
the same partial state. Dirty-row coverage is published once, after the first
successful backend cell and before its core Create; subsequent cells share that
row. The collector's enemy/decorative outputs do not apply to these two terrain
objects. Reset/import reestablishes the observer configuration at the next step;
replacing the scalar observer clears the associated run observer. Passing false
to `AmigaGameplayGeneration::bind` selects the scalar comparison path. Collision,
room and layer registrations remain per-cell; combining their work is separate
from this backend setup sharing.

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
