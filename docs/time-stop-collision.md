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
For the audited eligible sprite-bound path it saves old bounds, recalculates
current bounds, removes an already indexed instance with retry enabled, and
inserts its new bounds at level zero. It then unlinks the dirty-list entry.
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

`0x501e50` requires an object and either object flags `0x28` or instance flag
`0x20`. It unlinks existing spatial-dirty membership and prepends the instance
at `0x8a6100`. Repeated mutations therefore move an instance to the front;
deduplicating without reordering or using FIFO would change update order.
It then calls `0x501ef0`, which separately maintains the `0x8a60f4` list for
object flag `0x08`. These are two distinct intrusive memberships.

On the audited sprite-bound update path, `0x502ca0` skips instance flags
`0x100003`. Index update additionally requires object flag `0x10` and at least
one of global `0xb57c98`, object flags `0x28`, or instance flag `0x20`.
The generic sprite/mask fallback must not be inferred from this path alone.
`RunnerSpatialDirtyQueue` supplies the slot-based move-to-front membership
and these two eligibility predicates. Retire membership before reusing a slot;
the scheduler still owns mutation hooks, bounds refresh and the separate list.

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
