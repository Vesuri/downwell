# Time-stop collision inputs

## Query mutation contract

The object-selector `instance_place` wrapper (`0x4fe560..0x4fe8b1`) is not a
read-only first-hit scan. It prepares the selector before choosing traversal.
For a spatial traversal it then flushes the dirty queue and refreshes dirty
query bounds with publication enabled. Only then does it save position/bounds,
move to the requested position and refresh bounds without publication. The
spatial candidate callback also invokes precise collision with publication
disabled. Restoration calls the normal position setter, then copies the saved
cached bounds back; it does not restore the old dirty flags or queue position.

The list path instead moves directly, invokes precise collision with publication
enabled, and restores position through the setter without restoring cached
bounds. `RunnerPlaceQuery` models these sequences and stops on the first hit.
Its caller must provide eligible, lifetime-resolved candidates in source order;
it is not yet the live registry, nor the all/instance selector or collect-all
variant. Port-side callback failures attempt position restoration and require
the owner to latch failure rather than use partially changed index state.

`StoredCollisionLifecycle::placeVm` connects that traversal to an already
maintained instance store, object graph and resource-backed collision owner.
It resolves generation-checked handles for bounds refresh and precise mask
tests, using the owner's temporary query coordinates rather than rereading the
store's rounded position. Missing or mismatched candidates are failures, not
false hits or silently skipped instances. Forward bounds trigonometry, inverse
mask trigonometry and rounding remain explicit policies. This entry point does
not populate or rebuild the index: births, writes, activation, rebinding and
retirement must already have been routed through the owner in source order.
Live scheduler wiring remains in the shared work queue.

`StartupRuntime` accepts a borrowed `InstanceLifecycleBinding` at construction,
before the initial room births. Packed/fallback room loading, deferred creation,
room/reset/out-of-view retirement, activation requests and scheduler membership
reconciliation pass through this binding. Rejected callbacks latch failure and
stop later work; a rejected reset retirement must not create replacement births.
The binding's owner must outlive the runtime, and a bound runtime must not be
copied while sharing that owner. Default construction leaves the binding empty:
it does not install a collision registry or change query selection. Position and
resource writes, separate effect lifetimes and native-world registration still
need explicit routing before installing the full collision owner in gameplay.

Object identity changes use `InstanceStore::bindObject` with the collision
adapter's `rebind` callback. The callback validates the old handle/object pair
before applying the new object's membership and serialized mask/depth/sprite
defaults. Position and transform fields survive this operation. The store then
publishes the new object ID; it does not copy the new default fields into the
legacy record. Callers must route those legacy field writes separately before
using that record as presentation state. A failed external rebind is terminal,
not permission to continue with the old store identity and a changed owner.

## Player and shard inputs

The source player object 35 uses sprite 31 as its explicit collision mask,
independent of the visible animation sprite. Shard object 238 uses sprite 91
and has no explicit replacement mask. Both sprites have origin (12,12), a
24x24 canvas and a single stored collision bitmap.

`make verify-time-stop-masks` exports only to ignored generated data, checks
the input hash before and after extraction, and verifies every stored bit:

| Sprite | Solid pixels (half-open) | Local untransformed bounds |
| --- | --- | --- |
| 31, sprPlayerIdle | [8,16) × [8,20) | [-4,4) × [-4,8) |
| 91, sprPoisonPotPieces | [8,16) × [10,13) | [-4,4) × [-2,1) |

There are no holes inside either stored collision rectangle. This permits a
rectangle specialization for their mask samples, but does not justify rounding
the query position first. Against the precise goal-field mask, the runner still
performs its transformed per-pixel intersection after the compatibility-mode
bounds test. Float narrowing, inverse sampling and negative-scale alignment
must agree with [the runner collision rules](runner-collision-bounds.md).

## Ordinary fields

`parentTimeField` (object 82) itself is instantiated by builder symbol `O`
and by `scrTypicalDamage` for the Timeout upgrade. Its sprite 597,
`maskCircle24`, has a 48×48 precise mask and origin (24,24), at unit scale.
It is not the scaled exit field. The stored bitmap contains 1,741 solid pixels;
each row is contiguous, so exact row intervals may replace bitmap storage,
but a geometric circle approximation may not replace those intervals.

Create initializes radius bounds to 24 and 26 and consumes one random word
for `24 + random(2)`. Draw resamples before checking camera visibility unless
`global.pTimeStop` is set. It draws white outline circles at radius and
radius−1, followed by a red outline at radius+1. Drawing never changes the
collision mask. `OrdinaryTimeField` separates radius advancement from snapshot
painting so repaints cannot consume RNG.

The mask exporter records sprite IDs, source hash and extractor version/commit.
`make verify-ordinary-time-field` generates ignored row tables and a manifest;
the verifier pins all bitmap bytes and the audited Create/Draw assembly.
Builder object-82 initialization already consumes the Create random word:
world registration must retain that result, not call Create again and consume
a second draw. Remaining ownership, lookup and rendering work is in the queue.

`TypicalPlayerDamage` preserves `scrTypicalDamage`'s caller variables and
synchronous call order. The immunity check follows argument assignment;
direction is calculated before `scrPDamage`, then knockback, shake and the
surviving-health hit effect run before Timeout. Timeout is outside the death
and health conditions. Its object-35 traversal re-reads `pTimeStop` for each
instance and uses published instance coordinates, not player globals. New field
creation does not itself update `pTimeStop`. The two achievement writes occur
after that traversal. `RunSession::typicalPlayerDamage` latches adapter/creation
failure without replaying earlier effects; its current field adapter accepts
integer published coordinates only.

The component test verifies this ordering, gates, synchronous field RNG and
partial capacity failure against the reviewed VM script. Math, `scrPDamage`,
`scrFjump`, hit-effect ownership and original player traversal remain explicit
adapter contracts. This does not make the simplified live enemy contact pass
source-equivalent or establish natural Timeout behavior; those integrations
remain in the shared work queue.

`PlayerDamage` models the nested `scrPDamage` call separately: it records the
source name and selects caller `hitInvi` (Candle 240, otherwise 90) before
testing death and immunity. A permitted hit subtracts health, computes direction
again, creates the damage-number effect, plays sound 5 and ducks music, then sets
`pHit`. Each controller receives room speed 40, `slown=23`, then Alarm 0=40 in
that order. Regardless of whether health loss was permitted, the script finally
sets `playerDamaged` and assigns object-35 Alarm 1 from the caller's `hitInvi`.
The property assignment is a separate adapter operation, not a substitute
player `with` traversal. Nested calls may mutate globals; they do not cause the
outer damage branch to retest its entry guards or choose Candle immunity again.
`RunSession::damagePlayer` propagates failure without retry or rollback. Its
tests verify this event contract; the live number-effect, audio, controller and
player-property adapters are still required before replacing contact handling.

Object 221's `PlayerDamageNumber` stores published binary32 x/y separately
from binary64 velocities. Create uses `xsp=999` as a sentinel, `ysp=3` and
Alarm 0=60. The first Step resolves `lengthdir_x(2,enemydir)`; every Step
writes Y then X before damping both velocities by binary64 0.8. It has no
time-stop guard, parent or sprite. Alarm 0 requests destruction. The wrapper
subtracts four from precise `global.ply` before creation narrows the position,
then assigns amount/direction after Create; the effect's separate creation and
argument-assignment methods preserve that boundary for its owner.

Draw is ordinary text at compatibility depth −1000, not a sprite or `drawFx`
surface. It selects centered horizontal alignment, draws black text at the
four cardinal one-pixel offsets, then red text at the center and restores left
alignment. The current font and vertical alignment are retained; color remains
red. The backend must format the source `"-" + string(dmgamt)` and preserve
fractional coordinates. Component tests compare each float position write and
double velocity update with a scalar reference and verify ordered text commands;
they do not prove final font rasterization or live scheduler integration.

`PlayerDamageNumbers` owns room-local lifetimes with non-reused keys and global
phase cutoffs. Its source wrapper computes `ply-4` before constructor narrowing,
publishes the birth, completes Create, then assigns amount/direction. Newest-first
Step and Alarm traversal excludes births after the caller's phase cutoff.
Retirement and room cleanup invalidate handles before slot reuse; partial cleanup
can resume after failure without repeating completed retirements. Creation or
mutation failure disables simulation until cleanup. `RunSession` supplies the
shared owner, failure propagation and room/run cleanup. Live scheduler call sites
advance its Alarm 0 after Begin Step and its motion in ordinary Step in both
worlds. These events mutate only the number itself: no source RNG, child creation,
collision membership or cross-family side effects. Ordinary-Step births are
excluded until the next update; previously surviving numbers expire at Alarm 0
before that frame's Step. The world integration test compares 60 updates of both
schedulers, including a native generation-time birth. The cross-CPU replay's
`DAMAGE_NUMBER` case exercises the actual session math path at eight directions;
it does not exercise natural damage dispatch.

Both rendering backends consume the merged foreground stream. SDL emits the
five original text passes using recovered font 2. Amiga uses offline numeric
glyph masks (IDs 65100–65110), drains outstanding blits, then writes the small
glyphs with the CPU into the retired display buffer. Only their touched rows
are marked for restoration; the terrain backing remains unchanged. The text
marker participates in sprite assignment so DMA actors cannot incorrectly cover
foreground text. Glyph advances are eight pixels with no numeric kerning.

The current formatter accepts exact signed 32-bit integer damage, including
zero and negative values, and rejects unsupported values without rounding them.
General VM real-to-string formatting and fractional raster placement still need
original-runner verification. Focused tests inject effects: they verify SDL
commands, native Draw ordering and scalar planar pixel equivalence across all
word shifts and ring wrap, not complete player-damage dispatch.

The shared area-one contact response creates a one-point damage number after
subtracting health and before assigning immunity or applying knockback. Both
world adapters pass pre-movement Q8 player globals and published caller x/y,
translated to absolute room coordinates before the runner's binary32 argument
narrowing. Direction uses `RunnerPointDirection` with the portable finite atan
candidate; its original-runner evidence limitations remain those documented in
[point-direction arithmetic](runner-point-direction.md). Birth consumes no RNG,
immune contacts do not duplicate it, and capacity/math failure latches the world
and stops the contact pass without rolling back health. The world test covers a
natural SDL encounter and native-world collision fixtures; the cross-CPU replay
also compares fractional contact coordinates, direction and subsequent motion.

This is not yet the complete `scrPDamage`/`scrTypicalDamage` integration. Damage
source storage, controller slowdown, audio, hit effects, Timeout and replacement
of the approximate contact knockback/dispatch remain unfinished. Original-game
encounter captures and native contact frame-cost measurements are still required.

`ForcedPlayerJump` models the caller-owned `scrFjump` fields and VM write order:
assign arguments/default 2.7, replace either signed zero vertical argument with
that default, set global spin, synchronously recharge, clear player grounded,
conditionally write nonzero horizontal speed, then write negated vertical speed.
It re-reads caller fields after recharge instead of snapshotting them across a
synchronous script call. `RunSession` latches callback failure without replaying
earlier mutations. The shared player's contact path uses this ordering through
an adapter for its existing Q8 impulse and fixed vertical argument of 2.

Exactly coincident caller/player coordinates produce direction zero and hence
positive horizontal impulse 3, rather than no impulse. Other directions still
use the legacy fixed-point normalized vector in `PlayerMotion`; this is not the
original `point_direction`/`lengthdir_x` pipeline and remains migration work.
Component tests cover default height, signed zero, caller mutation during
recharge and partial failure; the damage replay includes forced-jump writes on
both CPUs. These do not prove original-runner contact dispatch or general motion
precision.

Damage numbers and corpse-expiry sprites share compatibility depth −1000.
`RunSession` assigns both a common monotonic Draw-birth key and merges their
newest-first owner lists in linear time, without temporary pointer arrays or
family-priority overrides. It rejects unregistered/duplicate keys instead of
inventing an order. Room/run cleanup never rewinds keys; exhaustion fails before
creating another effect. These keys describe only painter insertion order, not
original instance IDs or collision traversal. Other equal-depth families must
join this same ordering when integrated. The projection is read-only and returns
adapter failure to the caller; backend rendering must consume the merged stream.

`OrdinaryTimeFields` exposes port-owned lifetime keys separately from array
indices. As object 82 has no individual retirement, one room's births form a
contiguous key interval; lookup needs two counters rather than a key per field.
Room teardown invalidates the interval without rewinding births. `GameplayWorld`
preserves the next key across its placement-new Retry reconstruction. Exhaustion
rejects further births instead of wrapping to an old key, and adoption consumes
no additional RNG. Keys are local to an owner, not original numeric instance IDs
or evidence of source query ordering. Candidate selection still requires the
original object/spatial traversal, eligibility and precise-mask rules below.

## Dynamic field Draw ordering

`TimeFieldOutlines` takes a read-only snapshot of ordinary and goal fields in
this merged order. It stores visible circle centres and Q8 radii, expands one
three-ring outline at a time, and consumes no random values. SDL and the native
retained overlay use the same presentation snapshot. Collision still uses the
serialized masks, never these randomized outline radii.

The native overlay saves underlying bits by viewport word, combines overlapping
fields in painter order and restores only departing coverage. Its capacity is
bounded by all words intersecting the 160-by-284 viewport, including partial
edge words, rather than by one circle's expected footprint. Storage is ordinary
RAM; this adds no Chip bitplanes or scrolling repaint path. The 24-segment/Q8
integer raster remains an adaptation, not an original Windows GPU coverage proof.

The compatibility depth table maps both `parentTimeField` and `goalTimeField`
to 10000 (name/depth array slots 48 and 328). The compatibility
`instance_create` obtains that depth through `object_get_depth` and calls
`instance_create_depth`; zero depths in the serialized object records are not
the dynamic-instance depth.

Read-only Windows disassembly gives this chain:

- Registration `0x52cd31` binds `instance_create_depth` to `0x529840`.
- The wrapper calls `0x4fdcf0`, which calls room allocation `0x4878f0`
  before dispatching PreCreate and Create.
- Allocation writes depth at instance offset `0x178` and calls `0x487610`
  with the room list at offset `0x80`.
- Insertion traverses from the list head, comparing new depth with stored
  depth (`0x17c`). The `jae` at `0x487657` inserts before an equal-depth node;
  only a smaller depth advances through next links (`0x150`).
- Draw traversal at `0x490f11` starts at room offset `0x84` and follows those
  next links. The tile-interleaved traversal at `0x48e9a0` uses the same head
  and next links, dispatching event type 8/subtype 0.

Thus equal-depth dynamic fields whose depth is unchanged draw newest first.
An exit field must be interleaved at its creation boundary, not unconditionally
drawn before or after all ordinary fields. `OrdinaryTimeFields::advanceDraw`
accepts the ordinary-field count at exit-field birth for this merge. The
callback must not create fields or mutate their owner. Inactive/hidden fields,
depth changes, and interleaving with other Draw-event families require their
own scheduler handling; this is not proof of complete global Draw ordering.
`tools/verify-runner-field-draw.py` pins the reviewed executable bytes, not an
execution trace of the original game.

## Unrotated transformed sampling

The unrotated precise path at Windows runner `0x4b9227..0x4b93f6`
narrows reciprocal scales to binary32. For each negative reciprocal it
subtracts one from the corresponding position before inverse sampling.
Each subtraction, multiply and origin addition rounds separately to binary32;
sample coordinates are truncated before canvas and stored-mask lookup.

Compatibility bounds at `0x46616d..0x466328` compute the lower edge as
`float(float((margin-origin)*scale)+position)` and the upper edge as
`float(float(width*scale)+lower)`. They round both edges, swap if needed,
and subtract one from the upper inclusive edge. Computing the upper edge
independently from the position is not the same arithmetic contract.

`TimeStopGoalContact` specializes these rules for the two rectangular masks
above against either an integral-centred, scale-2 goal field or a unit-scale
ordinary field. The overload selects the actual resource mask: sprite 597 uses
its 48-by-48 bitmap, not a resized goal mask or the randomized Draw radius.
It returns half-open bounds internally, then tests shared solid pixels in X/Y
traversal order. It does not handle rotated actors or transformed field instances.
Unsupported numeric
inputs fail explicitly. The nearest-even bounds assumption still requires
original-runtime FP-mode verification; source pinning and host scalar tests
are not execution of the Windows collision routine.

## Player event boundary

### Field identity lookup is not Draw traversal

`scrTimeShard` calls `instance_place(x,y,parentTimeField)` twice on a hit and
uses the chosen instance's centre for `point_direction`. Boolean contact alone
does not determine that direction when multiple fields overlap.

The pinned Windows registration at `0x52cd08` binds `instance_place` to
`0x529e50`. That wrapper narrows query coordinates to binary32, calls
`0x4fe560`, and returns the selected instance's ID at offset `0x7c`, or `-4`
for no instance. The callee temporarily moves the querying instance and
restores its position before returning.

The selector `0x500d30` distinguishes object and instance selectors and can
return either list or spatial traversal. For object selectors, `0x500f8e`
compares object offset `0x4c` with spatial-root offset `0x04`; `jl 0x5010c0`
selects the list path. This is not a fixed field-count threshold inferred from
our storage capacity. Flag checks and index preparation also affect dispatch.

The object-list path at `0x4fe7c1` follows object offset `0x44`, list next at
offset zero and instance pointer at offset eight. It skips instance flags
`0x100003`, calls precise contact at `0x464d30`, and returns on the first hit
when no collection argument is supplied (the ordinary wrapper supplies zero).

Ancestor-list registration is independent of the depth-sorted Draw list:

- The constructor calls object binding `0x46b830` at `0x4618e9`. Binding
  removes previous membership through `0x479880`, resolves the object, and,
  when its registration argument is enabled, calls `0x478380`.
- `0x478380` appends a 12-byte node to the object's own list (head `+0x38`,
  tail `+0x3c`, count `+0x40`). Each node stores next, previous and instance
  pointers at offsets zero, four and eight.
- It then appends to the ancestor-inclusive list (head `+0x44`, tail `+0x48`,
  count `+0x4c`) for the object and every ancestor, following object `+0x04`.
  A derived goal field therefore participates in the parent's registration
  order rather than a separate derived-object pass.
- `0x479880` unlinks a matching instance from these ancestor lists by repairing
  both neighboring links and head/tail pointers. It decrements the count
  without reordering surviving nodes.

Consequently the list branch chooses the oldest registered eligible intersecting
instance. Birth order is equivalent only while membership remains unchanged;
rebinding can move an existing instance to the tail. The
[activation reconciliation contract](activation-fidelity.md#deferred-membership-reconciliation)
distinguishes immediate flags from delayed membership changes: completed
deactivation/reactivation cycles append again, but requests canceled before
reconciliation need not reorder membership. This result does not determine
spatial-index ordering or justify using Draw order for identity queries.

`RunnerObjectMembership` models object registration independently of storage
and Draw order. With an immutable parent graph, filtering one common binding
list preserves each own/ancestor-inclusive list's relative order. Bind appends,
including a rebind of the same identity; unbind repairs links without moving
survivors. Queries return stable slots plus opaque lifetime keys and can stop
at the first eligible hit. Eligibility flags and precise contact remain caller
responsibilities. Negative selectors such as `all` are not object queries.

The fixed registry stores 16 bytes per slot plus its three list counters;
queries scan the common list rather than allocating a node for every ancestor.
The owner must provide a validated immutable parent graph and unbind retired
slots before reuse. Active requests are not immediate unbind operations: apply
the reconciliation contract above.

The decoded host's `InstanceStore` owns this registry alongside its generation
handles. Successful births bind, and erase/clear/transient room retirement
unbind before slot reuse. Copies retain self-contained links; compaction leaves
registration order unchanged. `bindObject` is the explicit object-change boundary,
including same-object rebinding. `publishObjectMembership` changes registration
without changing the lifetime or active flag, and repeated publication is a
no-op. Direct writes to a record's `objectId` bypass this contract and must not
be used. `visitObject` returns the matching lifetime handle and const record.

The explicit `bindObject` callback can update an external owner's membership
before changing the stored object ID. It receives the old record and fails
without changing that record if publication fails. The collision owner's
`rebindObjectMembership` preserves the lifetime and appends even a same-object
binding, with registration controlled explicitly rather than by active flags.
This is only `0x46b830..0x46b8b2` membership behavior, not the complete object
property operation. The subsequent source code copies mask/depth, changes flags
and sprite, and queues bounds refreshes; for an unprepared non-solid object when
global preparation is disabled, it calls `0x501f70` to remove spatial/dirty
membership without destroying the instance. Those property effects must be
connected separately before treating this as a complete live object change.

`applyBindingProperties` supplies the collision-side flag and queue ordering
around an explicit sprite-setter callback. It consumes the runner flag word,
not serialized DWOB flags. Solidity sets `0x28` and marks queues; clearing
solidity clears `0x20` and conditionally removes spatial membership. Visibility
and persistence are written before the sprite callback, managed-object and
bounds-dirty flags afterward. Callback failure latches owner failure before
the final writes. Tests enumerate 256 low object-flag combinations with both
global preparation modes and both empty/existing queues. Mask/depth defaults,
the sprite setter itself, complete runtime object flags and live installation
remain the caller's responsibilities; this is not yet full object-change parity.

`bindObjectFromPacks` now composes the known-object ordinary path using DWOB v3,
DWSC and `bindInstanceFlags`: validate resources/property flags, rebind membership,
copy mask and serialized depth, then apply source flags and the resource-backed
sprite setter. Compatibility dynamic creation overrides depth separately after
this binding; room placement likewise supplies its own layer depth. The borrowed
`Properties` state keeps sprite fields, sequence position, cached frame and
binary32 depth together without platform dependencies. All 386 original objects
are exercised with registration enabled and disabled; incomplete property flags
fail before mutation. Unknown object definitions are explicitly unsupported.
Live ownership, mutation routing and post-binding birth publication are still
required before replacing production collision queries.

`StoredCollisionLifecycle` is the explicit bridge from loader birth snapshots
to this resource-backed owner. It narrows the stored integer birth coordinates,
binds resources, applies placed scale/angle/index and room/dynamic depth, then
prepares the object family and publishes bounds before game callbacks. It also
forwards retirement, activation requests and deferred membership publication.
The adapter borrows its owner, per-slot properties, packs and graph; rebuild it
around copied runtimes rather than retaining references to an original store.
It does not infer subsequent direct record writes or choose between a live
actor's integer position and separate fractional motion owner. Tests pass it
through `RoomLoader` with original field metadata and verify publication before
PreCreate, placed overrides, transition retirement and failure propagation.
Production installation still requires all later mutation paths to use the same
owner and the separate compact gameplay pools to participate in its identities.

The adapter's explicit `move` takes binary32 coordinates from the actor motion
owner; it does not substitute the diagnostic integer `InstanceRecord::x/y`.
Its refresh entry point reuses the stored sprite properties during dirty passes.
Activation, movement and refresh validate both lifetime and object identity before
selecting graph flags. Deferred-queue tests cover inactive dynamic birth, the
compatibility depth override, command image-index writes, Create ordering,
deferred deactivation, reactivation and a fractional move followed by refresh.
Wrong-object snapshots and stale handles are rejected. Routing each live actor's
actual setter sites through these entries remains required.

`RunnerSpatialInstances::removeSpatialMembership` exposes the live-instance
`0x501f70` operation separately from retirement. It removes indexed bounds with
root retry, clears only the indexed flag and unlinks both dirty queues, retaining
the object registration, cached state and lifetime. `retire` then additionally
unbinds object membership and ends that lifetime. Tests force mismatched cached
bounds, verify that list queries can still find the unindexed instance, and
republish it through an eligible bounds refresh without allocating a new handle.

The store preserves persistent memberships when compacting for room entry;
source-specific room-transition rebinding still belongs to the room owner.
Decoded-host deferred activation publishes at event-phase boundaries; other
gameplay-family registration and spatial/list branch selection are not connected
yet. These counts therefore
must not replace original-runner field selection until that integration is
complete. Store ownership alone is not a complete `instance_place` implementation.

The spatial path calls `0x5007f0`, then recursively traverses `0x5006f0` in
ascending node-entry order, pruning bounding boxes. Callback `0x5016a0`
filters self, flags and object/ancestor membership before precise contact.
At `0x501751` it stores the first matching instance and returns false, stopping
the recursive traversal. The Draw-list links are not used by these paths.

Spatial insertion starts at `0x4ffd40` and recursively descends through
`0x4fff80`. Nodes hold at most six 24-byte entries, with entry count at offset
zero and tree level at offset four. At the requested insertion level, a node
with space appends the entry; a full node calls split routine `0x5010e0`.
Descent chooses the child with least bounding-box area enlargement, breaking
equal enlargement by smaller existing area and retaining the earlier entry
on a complete tie. These area operations use binary32 SSE arithmetic.

If insertion splits the root, `0x4ffdb9` increments its level for a new root.
That root receives the old root first and the split sibling second. Thus the
selector's comparison against root `+0x04` is against tree height, not entry
count or a fixed population threshold. Splitting and subsequent updates can
change traversal order; the append rule alone does not establish oldest-first
lookup once the tree branches. The removal contract is below; the set/order
of instances populating the shared index still needs auditing.

### On-demand object-query preparation

`0x500f85..0x5010d4` distinguishes three cases after object resolution:
zero ancestor-inclusive population means no candidate; population strictly less
than root height selects list traversal; equality selects spatial traversal.
Only the spatial case can prepare new index members. Object flags `0x28` or the
global all-prepared byte `0xb57c98` bypass that preparation, not the query itself.

Otherwise `0x4798f0` runs before visiting the ancestor-inclusive list. That helper
sets object flag `0x20` unless `0x28` is already set and recursively visits the
object's child table at `+0x08`. The loader at `0x47b740..0x47b78a` resolves the
parent ID stored at `+0x5c`, stores its pointer at `+0x04`, and inserts the child
pointer keyed by its resource ID into that parent's `+0x08` table. This is the
direct-child graph, not an event list or the ancestor-inclusive instance list.

`RunnerObjectFamilies` builds equivalent parent/child links from a validated
object pack. Its stack-free traversal sets `0x20` and stops at any `0x28` node,
including that node's descendants. Sibling ordering affects no external operation
here, so resource order is sufficient for this flag-only traversal; it must not
be reused as an assertion about source event or collision traversal. Copies own
their links, and clear/capacity failures invalidate access. Runtime flags must be
provided separately, not copied from the serialized DWOB property bits. The
live registry binding remains separate from topology construction.

Initial spatial flags come from `RunnerObjectFamilies::bindSpatialFlags`:
the reset routine `0x478550` starts with `0x12`, whose spatial-relevant part is
`0x10`. Event inheritance (`0x478830`), collision-target expansion (`0x478b60`
and `0x4782a0`) and endpoint marking (`0x47a116`/`0x47a2bf`) establish bit `8`
on collision owners and targets, including their descendants. An override may
change the handler but does not remove the collision entry's participation.
The owning object is marked even if a target lookup fails.

For this participation flag alone, the result is the union of the descendant
sets of both endpoints of each serialized collision event. The portable loader
computes that union without materializing all inherited event pairs. It supplies
only bits `0x10`, `8` and later `0x20`; these are sufficient for the audited
spatial gates but are not the full runtime property/event-cache word. This does
not determine collision dispatch order or initial spatial insertion order.

Preparation follows registration-list order, skipping instance flags `0x108003`
(including already-indexed `0x8000`). Dirty bit `8` causes a bounds refresh with
update argument zero. It normalizes cached bounds into a local box, inserts it,
then sets `0x8000`. This differs from both hit eligibility and ordinary dirty
publication. The object-selector branch has no separate sprite-resource lookup
before insertion; the `all` branch does, so those paths must not be merged.

`RunnerSpatialObjectPreparation` encodes this object-selector operation using
explicit family-marking, ordered-member, bounds-refresh and insertion callbacks.
It does not populate the live world or establish the object's initial runtime
flags. Failure makes partially prepared state unusable for subsequent queries
until the owner recovers it. Tests cover branch boundaries, preparation gates,
ordered insertion, normalization and already-indexed suppression, not a complete
original-runner execution trace.

### Dynamic birth publication

Dynamic allocation `0x4878f0` finishes object/room registration before calling
`0x501dc0`. That routine ensures the shared index exists, calls bounds refresh
`0x466010` with update argument **one**, then enters the separate collision queue
through `0x501ef0`. The outer wrapper `0x4fdcf0` dispatches PreCreate and Create
only after allocation returns. Eligible initial bounds therefore enter the index
before user Create code; deferred end-of-phase insertion is not equivalent.
Bounds publication still uses the sprite/mask and preparation gates described
below. A later dirty pass can remove/reinsert that birth, changing tree order.

`InstanceRecord` retains sprite and explicit mask IDs separately. Deferred
births copy both from object metadata before invoking their Create handler.
Packed-room loading resolves both defaults during preflight when object metadata
is supplied, then installs them before PreCreate; Create must not reset values
changed by PreCreate. `StartupRuntime` supplies that metadata on initial entry
and room changes. Resource identity storage does not itself register spatial
membership or publish bounds; those hooks still belong at the birth boundary.

Room and deferred loaders accept an explicit lifecycle adapter. `birth` runs
after stored properties are initialized but before PreCreate/Create, and room
entry invokes `retire` through the store before invalidating old handles. Adapter
failure returns `lifecycleFailure` and stops later callbacks; deferred batches
latch that result so retry cannot replay earlier Create side effects. Room-load
failure is terminal for the current transition, not an atomic rollback of prior
retirements or births. Tests connect the room adapter to spatial attachment and
immediate bounds publication, checking persistent handles and slot reuse across
two rooms. Production stores without a registry use the no-op adapter; live
collision-owner installation and activation/mutation hooks are still required.

Immediately before direct publication, `0x4879ca..0x4879f3` checks the new
object's `0x28` flags. If absent, it walks parents and, on a prepared ancestor,
sets `0x20` on the new object's record only. `RunnerObjectFamilies::prepareBirth`
models this transition. Unlike query-time family marking, it changes neither
descendants nor siblings; it also leaves an already-prepared object unchanged.

Object binding `0x46b8b2..0x46b968` supplies the explicit mask, default depth,
sprite and property flags. Solidity sets instance `0x20` and bounds-dirty `8`;
non-solid binding clears `0x20` and may remove old spatial membership when no
preparation flag applies. The final binding step marks bounds dirty and invokes
the spatial dirty-queue helper. These property and birth-publication operations
still require live registry integration; family-flag tests alone do not establish
correct instance insertion histories.

### Spatial split contract

`0x5010e0` copies six old entries and appends the incoming seventh entry to a
temporary array. It passes minimum occupancy two to `0x4fc680`, which calls
seed selection `0x500490`. For normal finite ordered bounds:

1. Evaluate each pair in ascending `(i,j)` order using union area minus the
   first area minus the second area, rounding each operation to binary32.
   Greatest waste wins; complete ties retain the earliest pair. The first
   seed belongs to group zero, the second to group one.
2. For each unassigned entry, calculate each group's area enlargement and
   subtract group-zero enlargement from group-one enlargement. Negative
   difference prefers group one; zero prefers group zero.
3. Select the largest absolute difference. Equal differences prefer the
   candidate whose preferred group has fewer entries; otherwise retain the
   earlier candidate. Assign it and update that group's bounds and area.
4. When either group reaches five entries, assign all remaining entries to
   the other group, preserving minimum occupancy two.
5. Rebuild the original node (group zero) and new sibling (group one) by
   scanning the original seven-entry array in order. Do not append seeds
   first or retain assignment order: neither is the resulting traversal order.

`RunnerSpatialSplit` implements this partition with integer-only binary32
rounding through the portable numeric helpers. It rejects inverted bounds,
subnormal/nonfinite inputs and unsupported arithmetic without changing output.
`make verify-runner-spatial-split` compares it with a scalar binary32 reference
and pins the audited executable regions. This is a component check, not
execution of the Windows routine or proof of live field-selection fidelity.

### Removal, condensation and dirty-instance updates

Recursive removal `0x502b40` visits intersecting children in ascending entry
order. At a leaf it compares instance pointers in entry order; successful
removal copies the last entry over the removed slot and decrements the count
(`0x502c68`). It returns zero for success, one for not found.

On return from a child, a surviving count of at least two causes its parent's
bounding box to be recomputed. Otherwise the child is prepended to a detached
node list and its parent slot is removed with the same last-entry replacement
(`0x502c15`). This is unlike object-list removal: spatial removal can change
the first matching instance even without changing any survivor's coordinates.

`0x502a80` processes detached nodes in list order, reinserting each node's
remaining entries in ascending array order via `0x4ffe70` at that node's tree
level, then recycles the detached node. Ancestors detached later during unwind
are prepended ahead of descendants. After reinsertion, a root with one child
and positive level is replaced by that child. `0x502990` optionally retries a
failed removal using the entire root's bounds, rather than only the supplied
old bounds.

Dirty processing `0x502ca0` walks the intrusive list rooted at `0x8a6100`.
It skips entries with instance flags `0x100003`. When `0x46bff0` returns null,
it calls `0x466010(update=1)` unconditionally: neither bounds-dirty bit `0x08`
nor publication eligibility gates this recalculation. The bounds routine owns
missing-resource handling and publication. The non-null resource path saves old
bounds, calls `0x59ea10` (falling back to `0x466010` on failure), then separately
publishes eligible bounds. These paths must not be conflated.
After refresh it reads the next dirty node, then unlinks the current entry.
Thus movement is not simply an in-place bounding-box update. Object-flag
construction and all mutation callers still require integration evidence before
reproducing a whole encounter's tree history.

The position setter `0x46bc60` checks the two binary32 coordinates when its
`0x46bff0` lookup returns null. An unchanged position then does nothing;
otherwise it writes the coordinates, sets instance flag `0x08` and calls
`0x501e50`. Do not mark dirty merely because a setter was invoked. The non-null
lookup path bypasses the unchanged-position comparison.

`RunnerSpatialPosition` implements that mutation boundary for already-narrowed
coordinates: ordinary no-op writes preserve queue order (including `+0` versus
`-0`), special sprite state forces the update path, and actual mutations set
the bounds-dirty bit before moving eligible memberships to the head. The second
collision queue is updated only through the source's object-flag `0x08` path.
Invalid numeric values or required slot bindings fail before mutation.

The queue slot must be a stable registry handle. `InstanceStore` supplies
store-local slot/generation handles and resolves them through compaction;
`handleAt` refers to a record in creation-order iteration, while `resolve`
finds its current location. Erase, room retirement and clear invalidate handles;
persistent instances keep theirs. Generation exhaustion fails creation instead
of wrapping. Store copies carry index mappings, not pointers to the original.
Consumers must still remove queue/index membership before slot reuse and check
generation on deferred references. Whole-store replacement resets the identity
domain and requires resetting associated consumers.

`RunnerSpatialInstances` is the shared ownership boundary for these handles.
It joins object registration, cached binary32 positions/bounds, spatial traversal
and both dirty queues. Object-selector queries use `RunnerPlaceQuery` and return
a generation-checked handle, not a compacted index or a resource ID. Resource
resolution, precise pair tests and the validated object graph remain supplied
by the caller. It is not yet installed in the live gameplay schedulers: every
relevant birth, mutation, activation/rebinding and retirement must be connected
before its first-hit results can replace existing contact paths.
Its `publishBirth` boundary performs ancestor preparation and immediate bounds
publication before adding collision-queue membership. The scheduler must invoke
it before PreCreate/Create, not defer it to a later dirty flush. It does not
invent a spatial-queue insertion when earlier property setters did not make one.

The retirement boundary follows `0x501f70..0x502098`: for indexed instances,
normalize cached bounds, remove with root retry and clear `0x8000`; then unlink
both pending memberships. Only after that may the external store reuse the
slot. Missing-resource refresh can leave an old index entry elsewhere, which
is why cached-bounds-only removal is insufficient. Owner tests exercise this
case with actual `InstanceStore` compaction and slot-generation reuse. Clearing
the spatial owner preserves generation high-water marks; replacing the entire
external identity domain requires replacing its spatial owner too.

`InstanceStore` accepts explicit retirement callbacks on erase, clear and room
entry. They run while old handles and records still resolve, before slot
invalidation or compaction; room entry excludes persistent instances. This
allows the spatial owner to remove its bounds and both queues before slot reuse.
Callbacks may not mutate the store. If a batch callback fails, store records
remain intact but earlier external retirements are not rolled back; callers must
stop rather than retry a partially retired simulation. Existing no-callback
overloads remain for stores without external memberships. Supplying these hooks
at all live lifecycle call sites is still required; the overloads do not attach
a registry implicitly or add per-instance callback pointers.

Immediate exclusion flags and deferred object registration are distinct APIs.
An unregister operation alone does not retire a lifetime or its spatial state.
Callbacks cannot create, retire or rebind instances during a query. Mutation
failure latches the owner unusable until clear; stale external handles fail
without retiring a replacement. These contracts do not substitute for source
activation scheduling or asset/mask fidelity.

The live stored-instance scheduler uses `runStepChecked` to observe terminal
owner failures before dispatch and after phase, event, alarm-eligibility and
membership-publication callbacks. A failed callback must not permit another
instance's handler, alarm decrement or reconciliation pass. Already completed
mutations are retained, not rolled back. Both live world entry points reject
further updates after effect failure; registry publication still needs its
resource-backed adapter at the integration points described above.

`compactInstanceSlots` and `InstanceStore` record-array offsets are not stable
queue keys. The compacted gameplay-family arrays still need registry binding;
the position adapter is not yet installed on live stores. Handle metadata costs
one slot index plus one record index and generation per capacity entry, without
changing the serialized resource IDs or `InstanceRecord` payload.

Ordinary bullets in `GameplayWorld` carry nonzero internal lifetime IDs, like
the effect owners' opaque handles. `findBullet` resolves only live shots and
follows creation-order compaction. New births cannot reuse a retired ID; the
sequence survives level changes and Retry, and exhaustion fails explicitly.
These keys are not reconstructed Windows instance IDs and must not be used as
source ordering evidence. Mapping them into a shared spatial registry, including
synchronous membership retirement, remains necessary before live queries use it.

Native enemy handles pair an ownership slot with the monotonically increasing
enemy lifetime key. The ownership slot is independent of the compacted traversal
index: retiring a prefix record must not invalidate surviving handles. Reusing
the freed slot assigns a new key, and lookup checks both parts. Dormant enemies
remain resolvable; dead or retired enemies do not. Retry and level changes clear
slot ownership without rewinding the lifetime counter. These handles remain
world- and family-local; they neither establish original traversal order nor
replace registration and synchronous retirement in the shared collision owner.

The shared immediate bullet/stomp response adapters provide a checked retirement
callback after rewards and before clearing the active flag or publishing defeat
and bounce effects. Failure retains earlier damage/reward mutations and stops
contact traversal; callers must latch it rather than retry. The host scheduler
routes fatal bullet/stomp responses and snail expiry to its checked activation
binding with `false` before clearing the record's active flag. This removes
collision visibility while retaining the stored lifetime until ordinary erasure;
it does not set `activationPending` as a dormant deactivation request would.
The native compact enemy store uses the same borrowed binding specialized for
its own record type. It publishes fully initialized births, checked replacement
and out-of-view retirement, frontier activation, fatal-contact/snail inactivation,
and old-record retirement before Retry or the next level. Retry retains the
binding and generation counter. Failure is terminal, with completed earlier
callbacks retained. Native handles remain family-local; object-list traversal
and other families are not supplied by this binding.
The default bindings remain empty until their resource-backed owner is installed.
This boundary is not evidence
that the current immediate death response matches the runner's deferred enemy
death events. Whole-world room-clear ownership across other families still needs
integration.

`bindMovement` additionally routes the supported enemy Step's explicit published
position assignments to `StoredCollisionLifecycle::move`. The callback receives
the old record and proposed runner binary32 world coordinates before the record
changes; native local positions add the `(240,176)` origin. The bat, snake and
snail Step VM code explicitly stores `round(xx)` into `x`, then `round(yy)` into
`y`. These are not the fractional motion accumulators. The current integer
publication helper is restricted to that representation, with no conversion work
for an empty binding. A rejected update latches failure before later actors.
This is not a query-time synchronization pass, and does not yet cover player,
other effect families, mask/transform assignments or original scheduling
differences in the adapted enemy steps. Those remain prerequisites to installing
the full live query owner.

`bindSpriteChanges` extends that binding with the current enemy Step's sprite
selection, before updating the stored sprite ID and final image index. The
resource-backed adapter uses the existing sprite setter's frame-count lookup,
timeline/index reset and dirty-queue rules; assigning the same ID is a no-op in
that owner. Failed publication stops later handlers without rolling back earlier
motion. The natural seed-2 tape currently exercises same-ID publications only;
changed-ID coverage comes from explicit setter fixtures and real resource-pack
tests, not a claim of natural pose coverage. A wall-less snail must stop after
inactivation, without publishing later positions or sprite selection. Animation
index writes, other sprite assignment sites and exact source setter scheduling
remain required before full query installation.

Smoke, blood and flash owners preserve their birth counters when cleared.
In-place world reconstruction for Retry transfers these counters into empty
owners, just as it does for damage numbers and corpse-expiry effects. A restore
cannot overwrite live records, rewind a counter or revive an exhausted counter.
Zero denotes exhaustion, not the first key. Clear and Retry must not make an
old phase cutoff eligible to advance new effects. These owner-local keys are
not Windows instance IDs or cross-family traversal order; registry binding and
retirement still have to respect the contracts above.

`0x501e50` requires an object and either object flags `0x28` or instance flag
`0x20`. It unlinks existing spatial-dirty membership and prepends the instance
at `0x8a6100`. Repeated mutations therefore move an instance to the front;
deduplicating without reordering or using FIFO would change update order.
It then calls `0x501ef0`, which separately maintains the `0x8a60f4` list for
object flag `0x08`. These are two distinct intrusive memberships.

`RunnerSpatialDirtyQueue::flush` preserves that outer skip and callback/unlink
order, including refreshing clean queued instances. Its callback must dispatch
the appropriate resource path or fail for unsupported resources. Publication
eligibility is not a refresh predicate. A callback may reorder other entries;
nodes prepended ahead of the current entry remain pending for a later flush.
The current slot must stay alive and queued throughout its refresh. A failure
leaves that entry queued without rolling back earlier publications, so callers
must latch failure. Retire membership before reusing a slot; the scheduler still
owns mutation hooks, resource dispatch and the separate collision list.

Direct bounds publication has a different gate. The tail of `0x466010` clears
instance bounds-dirty flag `0x08`, then calls `0x5020a0` only when its update
argument remains enabled. A missing mask/sprite disables that argument. Calls
that request bounds computation without publication must remain distinct.

`0x5020a0` permits an already-indexed instance (`0x8000`) past the `0x100003`
exclusion; it still requires object flag `0x10` and the global/object/instance
preparation condition above. It normalizes both old and new rectangle endpoints,
removes existing membership with root-bounds retry, inserts the new rectangle
at level zero and sets `0x8000`. Not-found removal does not suppress insertion.
There is no unchanged-bounds shortcut, so even an identical republished rectangle
can change traversal order through last-entry removal and append.

`RunnerSpatialBoundsUpdate` connects those operations to the shared index.
Its tests distinguish direct publication from the outer dirty-pass skip,
check unchanged-bounds reordering and reversed endpoints, and preserve flags
on unsupported input. The caller must propagate failures and must not invoke
publication on the source's no-update/no-resource paths. This adapter does not
replace live sprite lookup, bounds calculation or mutation scheduling.

`RunnerSpatialNode` supplies the six-slot node, last-entry replacement and
original-order split distribution using index links rather than native pointers.
Its component tests cover every removal slot, reappend, internal/leaf distinction,
split ordering and failure preservation. It does not yet own condensation,
index traversal or the dirty queue.

`RunnerSpatialIndex` owns fixed-capacity nodes for insertion, removal and
broad-phase queries. It descends by least binary32 area enlargement (then least area, then
earliest slot), propagates splits, and creates roots with the old child before
the new sibling. Queries use closed bounding boxes and depth-first ascending
entry order; a callback can stop the entire traversal at the first precise hit.
An invalid insertion, unsupported arithmetic or exhausted node capacity latches
failure and prevents queries until reset. Invalid query bounds alone do not
change a valid index. Removal implements last-entry replacement, underfill
detachment, original-level reinsertion and single-child root collapse. Recycled
nodes return to a free list only after their surviving entries are reinserted.
Its optional root-bounds retry follows the source wrapper. Not-found reports a
successful operation with `removed=false`; mutation failure leaves that output
untouched and disables the index until reset. Live dirty-instance scheduling
and eligibility remain outside this owner; it is not an `instance_place`
replacement yet.

The scalar reference test compares ordered query results and first-hit stops
through multi-level insertions and remove/reinsert sequences, not just unordered
hit sets. It also covers empty queries, touching bounds, split/root capacity
exhaustion, stale-bounds retry, root collapse, reclaimed-node reuse and reset. This
does not establish original-game encounter ordering until the same instances
and mutation sequence populate the index.

`tools/verify-runner-field-lookup.py` pins these reviewed bytes. It does not
prove spatial-tree construction/update order or the
selector outcome during a particular original-game encounter. Those must be
established before assigning a source-equivalent field identity to shard bursts;
do not substitute Draw order, storage order or nearest-centre selection.

### Published and private player positions

`RunSession::parentTimeFieldContact` supplies the boolean union of the supported
ordinary and goal masks for player/shard queries. It narrows caller coordinates
once, does not consume RNG, and latches unsupported numeric failures without
publishing a partial result. This boolean query does not select an instance or
replace the independently ordered identity lookup above. The goal-only clock
query remains separate; an ordinary field must not implicitly stop the run clock.

In `objPlayer_n` Begin Step, `scrPlMovement` precedes clock accounting. The
source then applies wrap mode and publishes `x=round(xx)`, `y=round(yy)`.
Camera/interactable queries follow, then the time-stop transition query uses
private `xx/yy`. `scrTimeShard` performs its own field lookup using the newly
published `x/y`. These are different positions at the same event boundary.

The clock's earlier field query and the later time-stop query must not be
merged merely because both use `parentTimeField`. The existing integer goal
query is not by itself evidence for fractional transition contacts. Remaining
integration and original-runner verification belong in [the queue](open-work.md).
