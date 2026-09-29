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

A prepared first publication with an all-positive-zero cached rectangle and no
index-membership bit skips old-rectangle normalization and removal. It writes
the original cache and flags at the same point, validates new bounds only when
publication is admitted, and normalizes the tree copy using its already decoded
integer endpoints. It never normalizes the stored cache. Nonzero/signed-zero
caches and re-publications use the ordinary wrapper. Invalid-input, suppressed-
publication and capacity-failure prefixes are compared against that wrapper.

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

Fresh prepared collision binding has one callback-free interval after the
initial registration append. For an admitted ordinary constructor that remains
registered, rebinding the same object/lifetime at the same tail cannot change
registration payload or order. The selected owner therefore records that
second logical bind through `repeatFreshTailBinding`: both membership revision
increments and the owner binding/capture revision increment remain, without
another identity lookup or tail traversal. Only immutable resource/graph reads
may intervene. General rebinding, deregistration, unusual constructor states
and invalid-resource failure prefixes retain the ordinary path. The scalar
whole-owner configuration also retains that path for comparison. This does not
merge separate births, advance Create, or defer an observable mutation.

The admitted ordinary path also consolidates collision-state/property
initialization inside that interval. It copies semantic state fields once,
overrides previous position with the constructor position, and installs final
resource defaults without first clearing properties that are immediately
overwritten. Both logical owner revision increments are combined with
saturation, and capture observers are notified once. The membership revision
still records the complete append/rebind sequence.

Admission reads only immutable defaults and current graph flags. Unsupported
states, invalid resources, unregistered construction and invalid handles use
`attachObjectPreparedReference`, preserving failure prefixes such as an
attached lifetime with reset properties after resource-validation failure.
Registration failure retains the first revision increment and incoming state,
without publishing defaults. No new cache or pool is introduced; this does not
remove the separate terrain, room, layer or bounds records.

### Derived static layer lifetime state

The selected gameplay layer owner can leave an unnumbered, nonprojected static
lifetime compact. Its registry entry already holds the exact handle and source
ID, and its attached layer holds the published integer depth and layer ID.
Only active/inactive state needs separate publication. This avoids adopting a
second full lifetime record at birth. Named instances and projected actors keep
ordinary records. A failed compact attachment still publishes the same adopted
state and partial attachment prefix as the scalar path.

`RunnerLayerLifetimes::resolve` materializes compact state into the existing
per-slot record on first observation, yielding the same stable borrowed pointer
as the ordinary owner. A depth write also materializes before entering the
pending-depth queue. Activation/reconciliation and retirement do not require
materialization. Layer merges derive compact layer IDs from the updated links.
This opt-in contract requires all depth/attachment mutations to go through the
lifetime owner: an unobserved compact element cannot be moved by direct external
`RunnerLayers` writes. In gameplay, whole-layer depth movement requires a
singleton, whose depth request has already materialized its only instance;
other migrations detach the requesting element, leaving compact peers' depths
unchanged. Room teardown still rejects surviving unnumbered lifetimes.

The compact marker costs one byte per lifetime slot (3,067 slots in gameplay),
plus alignment. Existing full-record capacity remains reserved for observation
and mutation fallback; this removes repeated publication, not reserved storage.
The scalar configuration retains eager lifetime adoption as its oracle.

### Whole-owner differential boundary

`make verify-gameplay-collision-owner` compiles the gameplay owner twice:
its selected run-based room/object membership, and the scalar counterparts
selected by `DOWNWELL_SCALAR_TERRAIN_OWNERSHIP`. The latter is a verification
configuration, not a platform fork. Both retain identical spatial scheduling,
resource defaults, layer ownership, game logic and publication callbacks.
The complete existing owner assertion suite executes in both configurations.

The additional lifecycle witness compares full textual field values rather
than only a final hash: terrain handles/records, collision state and previous
coordinates, sprite properties, full RNG state, family populations, captured
End Step order and projected actor Draw order. It observes mixed object births,
inactive writes before reconciliation, reactivation, pending destruction,
cleanup, slot reuse, and the separate birth/Create boundary. Unrelated RNG
draws remain between Creates. A rejected backend terrain sink is observed
before and after rejection; that sink precedes native ownership and Create.
The ignored `build/ownership-{runs,scalar}.txt` files expose the first divergent
publication. They are comparison output, not independently captured original
game trajectories. Existing source-backed fixtures remain necessary.

This gate deliberately reads state at each boundary; it tests any future lazy
materialization under observation. Those reads must not be added to performance
tapes: the purpose of compact ownership is to avoid preparing unobserved state.
The witness does not cover every query, queue-failure prefix or native timing;
it complements, rather than replaces, those separate gates.

The prepared binding and collision registry borrow the supplied birth `State`
until attachment takes its own snapshot. They do not carry aggregate copies
through both call boundaries or retain a pointer to the builder's local state.
The snapshot still initializes previous coordinates from the initial position;
the caller's previous-coordinate fields and the source lifetime remain untouched.

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

Gameplay layer ownership uses the same bounded lifetime-slot domain for direct
element lookup. `RunnerLayerLifetimes` rejects a second live generation in an
occupied slot before attachment; `GameplayRoomLayers` therefore opts into
`RunnerLayerInstances`' direct index. Each entry stores element+1 (zero means
unbound), and lookup still checks the element's full generation. Retirement,
same-room rebind and room uninitialization remove the mapping before reuse.
This replaces hash-chain lookup only: element allocation, source-ID lookup,
room initialization/global iteration and Draw order remain unchanged. The
general registry defaults to its full-handle hash, including support for
concurrent generations with the same slot. Direct mode explicitly rejects
out-of-domain slots and overlapping lifetimes; it is not a transparent option
for owners lacking the bounded, single-live-lifetime invariant.

## Ordered registration runs

The scheduled collision owner uses `RunnerRunMembership` for object
registration. Adjacent registrations in the terrain slot domain share a run
only when object, slot sequence and generation sequence agree. The descriptor
stores the first slot/generation and count; a compact slot locator finds the
descriptor. Generations are lifetime identities, **not** original numeric
instance IDs. This does not repair the separate numeric-ID recovery gaps.

Enumerations expand each run at its original registration position, preserving
own/ancestor selection and first-hit order. Interior retirement splits a run;
re-registration appends at the tail, including reactivation. Actor registrations
remain singletons and retain their maintained capture order. The original
scalar membership remains selectable with `RunRegistration=false` on
`RunnerSpatialInstances` and is compiled by the differential tests.

Descriptors reserve the full slot capacity so splitting cannot introduce a new
failure boundary. Consequently this first integration reduces live registration
payload writes, not reserved memory: it adds six bytes per slot plus eight
owner bytes versus the retained scalar layout. This changes object registration
only, not the complete terrain representation. Create/RNG/publication ordering
is unchanged.

## Ordered room runs

`RunnerRunRoomMembership` stores consecutive terrain slots at identical depth
in one active/inactive room-list descriptor. It replaces per-block room links,
depth keys, marked-list links and animation skip links. The slot locator only
identifies a descriptor; destruction is a per-slot bit plus a marked count on
each run. Actors remain singleton runs. Insertion searches run depths for the
same stable upper bound as the scalar list; equal-depth order is unchanged.

Removing an interior member splits its run. Activation reconciliation still
visits inactive order before active order, moving each member and publishing
its callback synchronously. Failed callbacks retain exactly the moved prefix.
Cleanup visits active then inactive room order and saves the raw successor
before allowing retirement of the current member. Mark-free runs are skipped
without changing cleanup order. No new failure boundary is introduced: the
descriptor pool reserves enough space for complete fragmentation.

Animation omits the unchanged terrain interval by jumping across run spans.
An observed coordinate write makes the current remainder and next pass full,
as with `RunnerRoomMembership`; already visited snapshots are not revisited.
This removes maintained per-block projection links, not the source previous-
position publication itself. The scalar owner remains compiled in differential
tests for all four walks, fragmentation, reactivation and terminal failure.
Layer ownership, bounds, dirty queues and terrain lifetimes are still separate
per-cell structures; room runs do not complete their representation change.

### Layer representation constraint

Replacing only layer-element next/previous links with forward/reverse spans
does not remove the terrain birth transaction. The lifetime, element registry,
identity mapping, retained-Draw state and bounds still receive per-cell writes;
each ordinary successor lookup also acquires a descriptor indirection.
Reserving enough descriptors for arbitrary fragmentation adds eight bytes per
element plus allocator state compared with the scalar 16-bit links. A layer-run
design should therefore replace those associated per-cell publications too,
not introduce a second representation around the existing attachment path.
Keep saved-successor Draw insertion, inactive-tail reversal, lowest-free IDs
and partial-failure state as explicit differential-test boundaries.

### Dirty-queue representation constraint

Compressing only dirty-queue links leaves the repeated birth/property/bounds
publication calls intact. The common repeated-head mark is already a no-op;
adding descriptor lookup to that path can cost more than extending consecutive
terrain runs saves. A unified terrain representation should derive queue
membership from its own ordered records rather than maintain another run pool.

Queue consumers require exact prepend order, not merely the same set of slots.
Refresh reads the successor **after** its callback: a callback can move an
upcoming member to the head, and that member is then skipped by the current
flush. Excluded members are removed without refresh; failure retains the
current member and the unprocessed suffix. Full fragmentation and slot reuse
must not introduce capacity failures absent from the scalar queue. Compare
these intermediate states as well as generation timing; fewer birth stores
alone are not evidence of a cheaper complete frame.

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
