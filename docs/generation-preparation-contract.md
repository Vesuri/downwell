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
path without consuming dictionary slots. The table occupies 8,530 bytes of
ordinary storage per 68000 owner (8,532 on the host, not Chip RAM), owned for
the same lifetime as its immutable resource defaults. The FPS profiler reports
startup preparation separately using the OS field counter before takeover;
gameplay-only FPS does not measure that startup cost.

The four prepared integer offsets are retained alongside their binary32 table.
Each admitted shape also retains its four selected integer endpoints (48 bytes
for all three shapes), so live pixel-bound publication adds coordinates to
those endpoints without repeating dictionary selection. `atReference` retains
the dictionary-based integer calculation; every prepared grid point is compared
against it and the binary32 sprite-bound oracle. Repreparation, failed shape
admission and owner copies keep the same resource-lifetime contract.
An admitted terrain birth supplies both representations to the collision owner,
so journal admission need not decode those binary32 endpoints back to integers.
`refreshPreparedTerrain` retains ordinary bounds/flag publication, normalization,
and removal before insertion; nonscheduled owners use the reference refresh.
Neither normalization nor deferred servicing may write caller storage. The
journal owns its operation snapshot, and the instance cache retains the
source's unnormalized rectangle. Tests overwrite caller scratch immediately
after publication, then compare ordered queries after deferred replay with
the ordinary wrapper, including both ordered and reversed endpoints.
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

The run-store journal can instead record an immutable terrain geometry key
(shape, prepared X column and Y row). This captures the initial default bounds,
not the instance's current sprite or position. The prepared table must outlive
the journal and must not be re-prepared while keys remain readable. Replay
expands the key into the normalized rectangle; the ordinary rectangle journal
remains the reference and fallback. Each key still counts as one operation:
pressure drains, service budgets, population admission and failure ordering
are unchanged. The key occupies existing operation storage, so this removes
rectangle publication work but does not reduce the journal's reserved capacity
or eliminate the instance bounds cache. Ordered-query tests cover pressure,
retirement/reuse, scratch overwrites and eager fallback.

A prepared first publication with an all-positive-zero cached rectangle and no
index-membership bit skips old-rectangle normalization and removal. It writes
the original cache and flags at the same point, validates new bounds only when
publication is admitted, and normalizes the tree copy using its already decoded
integer endpoints. It never normalizes the stored cache. Nonzero/signed-zero
caches and re-publications use the ordinary wrapper. Invalid-input, suppressed-
publication and capacity-failure prefixes are compared against that wrapper.

## Birth publication

### Eligibility is not geometry observation

The collision registry exposes generation-checked `contains`, value-based
`instanceFlags`, and ordered `visitRegistrationFlags` separately from full
`resolve`. A failed flag read leaves its output unchanged. Like full resolution,
individual lifetime/flag reads remain available after terminal failure for
inspection of partial publication; ordered traversal still requires a valid
owner. Membership and ancestry cannot change inside a registration visitor.

Scheduler consumers must request only what they observe:

| Consumer | Required state |
| --- | --- |
| Typed binding validation and post-retirement check | Lifetime presence and object identity |
| Step/alarm eligibility, Draw exclusion, activation reconciliation, cleanup | Live generation and current flags |
| `instanceNumber` | Registration/ancestry and flags masked by `0x140003`; no geometry |
| `collisionPointVm` candidate admission | Registration order and flags masked by `0x100003`; candidate owns geometry refresh |
| Bounds refresh and collision candidates | Position, geometry and relevant flags |
| Time-field contact result and box time-stop position | Published coordinates |
| Explicit state witness | Complete source-visible record |

Flag values are read again at the same callback boundaries as before; they
are not cached across callbacks. Returning a value rather than a flag pointer
allows a future terrain run to combine common flags with per-cell exclusion
bits without materializing a full spatial record. The scalar ownership build
retains full-state resolution for eligibility as a differential reference.
This API separation does not yet defer or remove any stored terrain state.

`instanceNumberReference` and `collisionPointVmReference` retain the full-state
read at these two additional boundaries. Neither population counting nor an
excluded point candidate may force geometry materialization. Point admission
still reads flags for each registration at its original callback boundary;
an eligible candidate is not a promise that its geometry is unchanged.

### Constraints on an observation-driven terrain owner

These are distinct observations, not one deferred birth callback:

- `contains`, `currentHandle`, registration counts and eligibility must see
  each successfully published lifetime immediately. Their representation can
  be run identity plus current per-cell flags; they must not drain pending
  geometric work.
- `resolve` returns a borrowed full-state pointer. Multiple candidate states
  can be held simultaneously (`collideCompatibility` holds two), so one shared
  scratch record is not a valid replacement. Mutation and full-state inspection
  need a stable projection, distinct from flag-only reads.
- Initial bounds are published **before** terrain Create changes the sprite.
  `changeSprite` dirties flags/queues but does not replace cached bounds or the
  old tree rectangle. Deriving historical bounds from the current sprite would
  change removal, refresh and tree traversal. Record the published geometry
  separately from current appearance, including when the latter is dirty.
- A query's family preparation, dirty-queue flush and historical tree replay
  have different ordering contracts. `point`, `collisionLine` and `place` can
  read index height while choosing a path; this is already an observing boundary,
  before candidate callbacks. `collisionPointVm` has no such height read or
  global flush. Do not insert either into its flag-only admission path.
- A full-state projection must not replay lifetime allocation, room/layer
  attachment or Create. Those have already taken effect in the authoritative
  identity/order records. Replaying the existing birth transaction at the first
  query both repeats side effects and moves the generation stall to that query.

The compact representation therefore needs immediate identity/flags/order,
immutable publication history, and independently derived full-state views.
Tree pressure/failure rules and intervening query callbacks still constrain
history coalescing; future run cells cannot become visible before their Create.

### Appearance is not a single interchangeable record

The native terrain lifecycle exposes sprite and integer image-index setters,
activation, destruction and room/End Step callbacks. It does not expose the
actor binding's mask, transform or depth setters, nor dispatch terrain animation.
Consequently the constructor mask/depth and unit transform can be shared by
object rather than copied into every terrain cell. This restriction belongs to
the native terrain owner, not to the generic runner/reference implementation.

There are nevertheless three separate observations:

| Observation | Authority and mutation boundary |
| --- | --- |
| Presentation sprite/index | `GameplayTerrainInstancesRuns::CreateValues`, after each successful lifecycle callback |
| Collision sprite/index | The ordinary runner sprite/index setters, including index resets inside the sprite setter |
| Cached and indexed bounds | The last bounds publication; initially constructor geometry, before Create chooses the area sprite |

These cannot be aliased merely because normal fresh births agree. For example,
set a wall's image index to 15, then repeat Create with missing sprite -1. The
runner setter resets its index to zero, while wall Create does not write the
presentation index, which remains 15. Repeating the same sprite is a setter
no-op. Platform Create instead explicitly writes 23 after the sprite change.
Neither Create updates the previously published bounds. Marker (99) Create is
a no-op. The whole-owner test exercises these boundaries in all four ownership
variants, across all six area tiles, without consuming RNG.

Therefore a replacement may share immutable appearance defaults, but must
retain the independent mutable collision index and the historical geometry.
Expanding a read from the current presentation tile alone is not equivalent.
The ownership API must also allow two simultaneous property/state observations;
a single overwritten scratch record cannot replace borrowed pointers in pair
queries. Temporary `place` queries restore coordinates through setters, while
only their spatial branch restores the saved bounds cache. Current position is
not sufficient to reconstruct every historical cache either.

This is a storage/observation contract, not justification for preserving the
full per-cell runner transaction. Allocation, order publication, eligibility
and historical geometry must be owned once; constructor-property copies and
derived-state construction belong at their actual observation boundaries.

### Terrain record and ownership publication

`PreparedTerrainCreate` compiles ordinary object-60/object-93 Create for the
six area tiles before gameplay. It evaluates `terrainCreateProperties` with
glitch disabled and resolves the sprite setter against immutable constructor
defaults and sprite frame counts. An admitted plan consumes no RNG, ends with
image speed zero, and optionally writes the platform's index 23. The 12 plans
occupy 192 bytes on the host; they contain no live instance or family state.

The opt-in run-store's initialized birth applies a plan only after the entire
birth has succeeded. At this private boundary no external callback has changed
the constructor's zero index/timeline or cached frame -1. Thus any sprite-setter
reset would rewrite the same values. Apply only the changed sprite, its dirty
flag/queue effect using **current** family flags, and an explicit index write.
Keep initial published bounds and tree history unchanged. Typed image speed,
wall-art range and Draw-cache invalidation are still published synchronously.
Failures during birth occur before any of these Create writes, as before.

Glitch Create, non-area tile overrides, failed plan admission and the scalar
ownership reference retain the script/setter path. The plan is not valid for
repeated Create on an existing instance or after an arbitrary callback. No
builder choices, health-dependent values or random choices are prepared here.

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

### Borrowed homogeneous builder runs

`AreaOneBuilder::TerrainRun` may enter `GameplayWorld::createGeneratedTerrainRun`
without being flattened into independent core birth callbacks. This is a trusted
backend interval, not the general callback API: its sink may update the backend's
terrain index/tiles/dirty rows and reject a cell, but may not query or mutate core
ownership, create another instance, or change the stage/area. Arbitrary observers
use the original per-cell entry point. The Amiga run sink satisfies this contract.

The run executor still consumes the builder RNG immediately before each cell,
then runs the backend, typed allocation, birth publication and Create in that
order. It neither reserves future lifetimes nor consumes future RNG. A rejected
backend cell leaves core allocation and the world's failure flag untouched;
failure after typed allocation latches failure with its published prefix intact.
Reentry or reexecution is rejected before consuming RNG. Builder tests compare
all executors at every callback, including callbacks that consume additional RNG.

Immutable object defaults and the ordinary Create plan are selected once for the
run. Family preparation occurs at the first cell's original publication point;
subsequent same-object births cannot change it inside the trusted interval.
Dynamic layer lookup/creation likewise occurs only when the first cell reaches
layer attachment, after spatial publication, not at run entry. Each later cell
still admits its own layer capacity and order key. The typed owner hands the
fresh identity and live Create outputs directly to this transaction, without
constructing a complete `Record` for a callback to the same owner.

Unsupported defaults/geometry retain per-cell birth. The run path still owns
full per-cell spatial state, properties, queue entries and geometry publication;
sharing setup does **not** constitute their replacement by compact run state.
Whole-owner witnesses compare run emission, backend-failure prefixes, full
properties/state and subsequent queries with the retained scalar implementation.

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

Successful compact births also avoid constructing a temporary ordinary
attachment record. `attachFreshUnnumbered` resolves or creates the same dynamic
layer, allocates the same lowest free element, publishes its identity and
inserts it at the same head. There is no named placeholder or saved layer ID
on this path. Inactive births still enter at the head until reconciliation.
On failure it publishes the ordinary adopted/partially attached record, with
the source depth before attachment and the layer's encoded depth afterward.
The general `attachRoomFallback` remains the reference. Direct fixtures compare
layer topology, registry entries and failure-state fields, including failed or
malformed layer creation, duplicate/out-of-range identities and full capacity.
This removes discarded attachment state, not the persistent per-cell element.

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

The witness also has a separate cold-burst phase: 128 mixed terrain births,
interleaved enemy births and unrelated RNG draws, without per-birth state or
Draw inspection. Coincident walls make first-hit identity observable. Four
variants retain active cells, deactivate without reconciliation, reconcile
deactivation, or request destruction before the first query. Point, excluded-
self point and line selections record the full selected handle, not only a
hit boolean. Subsequent temporary placement queries check restoration of
current/previous coordinates and record resulting flags and bounds. Queries
must leave all RNG words, index and draw count unchanged. These cases compare
selected versus scalar ownership, not an independently captured Windows run,
and do not establish a latency bound for the first observing query.

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

The direct registry's element payload contains only source ID and lifetime
handle (12 bytes). Presence is derived from a nonzero source ID or generation:
serialized placeholders retain their source ID when uninitialized, while
unnumbered bindings retain a live generation. Retirement/rebind release clears
both. Attachment publishes the handle before attaching links, so a failed
attachment preserves the same present-but-unattached prefix. There is no
independent presence flag to write at every birth. The general hash registry
retains its explicit flag as the reference; the direct/general lifecycle
witness compares placeholders, initialization, reuse and duplicate rejection.
This does not eliminate the separate per-element links or lifetime records.

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

Family selection tests ancestry once per run, before expanding matching
lifetimes. Population reads sum matching run lengths without constructing
per-cell identities. `RunnerObjectFamilies` supplies immutable parent links;
query preparation changes only flags, not ancestry. Callbacks can refresh
instance state but cannot mutate registration or topology during these walks.
Selected instances still run callbacks individually, in original order, with
the same early-stop prefix and modulo-32-bit generation arithmetic. The
expanded `visitReference`/`countReference` paths remain compiled by tests,
alongside comparisons with scalar ownership. This reduces observation work;
it does not remove per-block birth transactions or bound tree replay latency.
The cross-target spatial verification includes a separate run/scalar family
fixture: full counts and ordered early-stop prefixes after interior removals,
rebindings and clear. A native gameplay tape without those selectors is not
evidence of this path's query cost; use the explicit query fixtures as well.

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

Whole-owner `clear` is different from callback-bearing cleanup: no intermediate
removal can be observed. Registration and room-run owners invalidate the slot
locators and release each descriptor once, in the same order as cell-by-cell
removal. Registration revision advances by the old population, invalidating
cached projections with the same unsigned wrap behavior. Room marks and both
active/inactive heads are reset. The retained `clearReference` paths perform
the original individual removals; fragmentation and subsequent full-capacity
reuse must agree. Individual retirement and cleanup callbacks are not batched.

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

Terrain lifecycle callbacks borrow values for the duration of the call; their
record address is not a game identity. `ownsIdentity` validates a live local
slot/generation and its immutable object, coordinates and authored binding.
`read` supplies a caller-owned snapshot, leaves output unchanged for a stale
handle, and remains available for terminal-failure inspection like `find`.
Mutable Create results must still be read at each callback's original boundary.
The snapshot verification configuration passes an independent temporary to
every terrain lifecycle callback and compares the full ownership witness with
both the normal adapter and the scalar address-based reference. This enables
run-backed records without requiring permanent per-cell callback addresses;
it does not itself replace the current record array or defer publication.

`TERRAIN_RUN_STORE=1` selects the run-backed terrain store; the scalar store
remains the default and reference. Its `find` result owns a value snapshot, not
a pointer to mutable storage. Re-read the handle after a setter or event to
observe changed fields. Snapshot equality identifies the owner and lifetime,
not equality of captured fields. Row traversal rejects whole runs before
expanding live cells, using the same conservative 16-pixel artwork margin;
unbounded artwork requires full traversal. Neither traversal permits structural
mutation. Host option tracking rebuilds the SDL objects when this flag changes;
native comparison runs use clean builds and restore the default afterward.

In that configuration, the spatial owner obtains terrain object IDs and
generations directly from the typed run store. Its ordinary identity array
excludes the terrain slot range. The typed allocation byte distinguishes
pristine, allocated-but-unpublished, spatially published and spatially retired
records; it also retains the old first-use/reuse distinction. Destruction marks
do not retire identities. Spatial retirement precedes typed retirement during
the callback, and clearing the spatial owner must not permit reattaching the
same typed lifetime. Only a new typed allocation resets that admission state.
The external identity adapter is for immutable typed terrain families: a
different object family requires a different typed lifetime, not mutation of
an independent spatial object ID. Ordinary actor owners retain their mutable
object binding and per-slot generation high-water marks.

Terrain object registration in that configuration is a presence/order stamp in
the same typed mutable record, not a second linked list or run locator. Actors
retain their own registration list. Own/ancestor population counts use the
three terrain-family counters; actor-only walks never expand terrain. A mixed
walk observes a cached slot projection sorted by publication stamps, including
rebindings. Typed retirement invalidates this projection even when it bypasses
the registration facade. Stamps are two-word ordering keys, not numeric game
instance IDs. Membership mutation is forbidden inside visitors; projected and
retained visitors detect it across both the actor facade and typed terrain.
The projection is derived state: generation cost must include its first
observation when measuring this representation, not just cheap stamp writes.

Room membership uses separate authoritative depth, insertion/deactivation order,
active/inactive membership and cleanup-mark fields. These are not object
registration stamps or layer element order. Actor and terrain room projections
are independently invalidated and merged at observation: active order is depth
then insertion order, inactive order is deactivation order. Reconciliation
observes inactive first, then the updated active order; cleanup observes active
first. Each move is visible before its publication callback, retaining terminal
failure prefixes. Cleanup callbacks may remove their current member only.
Marked cleanup sorts only its sparse pending slots by these same keys; it does
not materialize complete room order for a single destruction. Removing the
current slot leaves a hole until the pass ends, keeping successors stable.
Both successful and failed passes compact those holes before returning, so
later retirement never searches an accumulated history of dead entries.
Clean retained-animation passes observe actors alone. If a callback dirties
terrain positions, the remaining mixed order resumes after that actor, without
resnapshotting earlier terrain. No game callback is deferred by these projections.

### Retained terrain layer observations

Removing a terrain cell's ordinary layer entry is not equivalent to removing it
from the logical layer. The replacement must retain these source observations:

- `RunnerLayerDraw::action` observes the live inactive flag before testing
  whether an instance has an actor Draw operation. An inactive retained cell
  ends that layer's walk even though an active retained cell draws nothing.
  An earlier actor's Draw callback can change that flag before the cell is
  reached. A cutoff captured only at frame entry is therefore insufficient.
- Flag publication and element reconciliation are separate. The first changes
  the cutoff at its old position; reconciliation moves that same logical
  element to the head or tail. Inactive normalization scans backwards and
  moves entries to the tail, reversing their forward order rather than doing
  a stable partition. Head/tail order and concatenation must survive grouping.
- Retained-only dynamic layers remain allocated. Retiring the last actor does
  not free the layer, and another actor at that depth reuses its existing ID.
  Only final logical-element retirement disposes it. A later replacement
  consumes a new process-global ID through `RunnerLayerIds`.
- `RunnerDepthService::apply` uses **logical element count**, not the number
  of stored groups, for the singleton small-depth-change branch (0x414660).
  An actor beside retained terrain must detach and acquire the destination
  dynamic layer; it must not move the old terrain layer as though alone.
  Serialized placeholders and non-instance elements also contribute.
- Increasing-depth merges publish destination layer IDs before concatenation
  and inactive normalization (0x4147b0). Failure retains the publication prefix.
  Empty-layer disposal, capacity checks and source-ID consumption must occur at
  their original boundaries, not at deferred projection construction.
- Draw saves each raw successor before dispatch (0x4904c6). Any grouped
  representation must preserve that observation point for births and flag
  writes during Draw, not merely match a static sorted snapshot.

`retainedLayerBoundaries` in `RunnerLayerDrawTest.cpp` pins the live cutoff,
retained-only lifetime, logical singleton decision and resulting process-ID
sequence against the existing source-backed primitives. The surrounding suite
supplies merge/failure, normalization and Draw-time birth tests. These gates
do not establish that an unintegrated grouped representation is equivalent.

`RunnerLayers<L,E,true>` separates retained logical population from physical
element links. `retain` and ordinary attachment share the same `E`-element
admission limit. Retirement disposes a dynamic layer only when both populations
reach zero. This opt-in storage adds one count per layer and a total, not an
array per terrain cell. The two-parameter/default type has no retained counters.

A retained owner uses `moveLogical` to publish the entire logical source in its
actual order, then normalize the combined logical destination after physical
concatenation and population transfer. Its callbacks must maintain retained
order and cutoffs as well as physical entries; `normalizePhysicalInstances`
does only the physical part. Publication failure preserves the source population
and completed publication prefix; normalization failure preserves concatenation
before source-layer disposal. Ordinary `move` refuses a retained merge for which
it cannot supply those callbacks. This API alone does not route gameplay terrain
away from ordinary layer entries; that requires the authoritative terrain owner
to supply ordering and publication, not a no-op callback.

The instance registry, lifetime owner, background owner and packed-room loader
accept the same layer-storage type. Logical exhaustion must be reflected by
`firstFreeElement`, not just rejected in `attach`: the compact birth path finds
a free slot before publishing registry identity and attached state. On exhaustion
it still consumes a newly required dynamic layer ID, but leaves the adopted
lifetime unattached and publishes no element, matching physical exhaustion.
`logicalCapacityFailure` compares those complete prefixes, and
`retainedLifetimeIntegration` exercises ordinary depth service and retirement
against retained logical population.

The opt-in run-store gameplay path uses `RunnerLogicalLayerOrder` for generated,
unnumbered terrain. Its authoritative mutable terrain record holds layer slot,
published layer ID, inactive flag and order key; birth and retirement bypass
the ordinary element registry and lifetime records. Authored room instances
retain the ordinary path. Actor depth service still sees the full logical
population when deciding whether a layer is a singleton.

Head and tail order keys reproduce insertion and activation order. Only a
merge or room normalization materializes and sorts the combined logical walk;
source layer IDs publish in that walk's order before concatenation. Inactive
terrain supplies a cached per-layer Draw cutoff. A Draw callback's newly born
head must not retroactively stop an already-started walk, whereas an inactive
write to an upcoming terrain record must take effect immediately. The walker
therefore retains the preceding actor's pre-callback order key as its lower
bound. `RunnerLogicalLayerOrderTest` compares merges, mutations and these live
callback cases with the physical linked reference.

Live non-instance insertion into a retained layer is not admitted by this
adapter: resource backgrounds load before terrain. Typed generated terrain
does not expose a depth setter; attempts through this facade fail rather than
silently targeting an ordinary lifetime. The ordinary capacity-sized arrays
remain allocated in this experimental integration. Matching ownership
witnesses does not justify enabling it by default: native timing and removal
of that redundant storage remain required.

`attachPreparedTerrainBirth` is an owner-controlled attachment/publication
transaction for prepared geometry. Its intervening room-binding callback may
observe creation depth but must not observe or mutate spatial state. Fresh
zero bounds are therefore materialized only on a failed prefix; a successful
transaction writes the final prepared bounds once. Lifetime admission,
membership revisions, dirty-queue ordering, live family preparation and journal
admission retain their original ordering. Unsupported initial states use the
separate-operation reference. The reference remains callable for verification.

The initialized-allocation callback receives its record directly from the
terrain store. When that store is the world's bound authoritative store, it
does not deserialize the same run again to compare the record to itself.
The allocator-owned, prepared birth carries that admission through spatial,
registration, room and retained-layer publication: these owners must not each
look up the same immutable run identity again. The typed allocation has already
validated slot/family/capacity and reset reused mutable state; the collision
owner verifies that it is the world's bound terrain store and that the encoded
handle matches the callback's generation. Only that callback-free interval can
use the admitted entry points. Other bindings and ownership contexts retain
ordinary generation/object and record-resolution checks.

Direct registration still advances its original clock, family count and both
logical bind revisions, rejecting clock exhaustion before publication. Direct
room publication advances the room clock/revision and starts active/unmarked;
it cannot be used for a rebind with old cleanup marks. Retained-layer birth
still resolves/creates the dynamic layer and allocates its ID at the original
point, retaining capacity failures, element counts and inactive cutoffs. It
omits only fresh-slot identity/empty-state rediscovery. No observer can run
between typed admission and those writes. These entry points neither batch
separate births nor change their RNG/Create order, and do not remove the
remaining spatial state, property or dirty-queue storage.

Generated terrain may bind one initialized-birth transaction. The run store
allocates the typed identity first, and the built-in collision owner completes
ordinary birth publication before executing `terrainCreateProperties`. Create
writes borrow the live mutable values, so an early failure leaves the allocated
identity and the completed write prefix intact. The admitted handle cannot be
retired or reused between those built-in operations; sprite publication can
therefore use the established ownership without another typed-record lookup.
No callback, RNG draw, or later pattern cell is deferred or preconsumed. This
entry point uses the existing generated-cell non-glitch Create path; standalone
Create, authored-room creation and custom lifecycle observers retain their
separate callback path. Binding the transaction is only allowed before births.

Compressing only the deferred bounds journal does not replace terrain
ownership. Exact encoded-coordinate runs can remove repeated rectangle
snapshots, but still require per-cell admission, population tracking and
historical-union checks. The associated lifetime, property, layer and queue
publications remain. Do not use a smaller live descriptor count as evidence
of a cheaper generation frame. Preserve operation-count pressure boundaries
and partial-service behavior when changing the journal's representation;
descriptor-count pressure would move observable replay/failure timing.

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
The live executor advances an integer X coordinate by the admitted stride
after each successful callback, except the final cell. It does not derive the
next coordinate from borrowed command storage. `executeReference` retains
the indexed `x + cell * stepX` calculation; both share the single-execution
guard, including recursive execution and retries after partial failure.
This changes coordinate arithmetic only, not the per-cell birth transaction.

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
