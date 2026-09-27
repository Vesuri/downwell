# Creation during a Step phase

## Collision boundary

The normal Windows frame loop calls Step at `0x50dd76`, reconciles at
`0x50dd80`, and later conditionally enters the collision pass `0x5022c0`
at `0x50decc`. It reconciles again at `0x50ded1`, then calls End Step at
`0x50def3`. The condition at `0x50dec8` skips this pass when BL is nonzero;
the alternative path is not modeled by this boundary.

The collision pass calls pair processing `0x5030d0` at `0x5025cb`. Pair
processing checks overlap and then inactivity flag `0x2`, then invokes event type 4
at `0x5031a6` and `0x5031b5`, swapping self/other and selecting the other
object's ID. Solid-instance position rollback/reapplication surrounds those
calls. This establishes a separate phase and bidirectional dispatch, not the
complete candidate ordering, mask rules or solid response semantics.

The precise-overlap routine separately excludes destroyed instances (`0x1`)
and flag `0x100000`; do not confuse that with the wrapper's explicit shift by
one at `0x5030f0`/`0x5030fd`. After inactivity checks, `0x501ef0` prepends OTHER
to the collision queue if its object flags contain `0x8`, without dirtying its
spatial bounds. Both event directions run without another wrapper exclusion
check. The dispatcher still owns its room-transition guards.

`RunnerCollisionDispatch` preserves this checked operation sequence. If either
instance is solid, it rolls back self then other (including path position),
dispatches both directions, and rereads solid flags. Post-event solidity causes
motion preparation for both instances, followed by movement for both, a second
overlap test, and another rollback only if overlap remains. The source uses
binary32 additions when applying motion (`0x5031e3..0x503248`). The portable
protocol does not substitute fixed-point additions: concrete motion/path and
event-context callbacks remain required before live scheduler admission.

`RunnerCollisionMotion` provides the concrete rollback and binary32 coordinate
addition used by that protocol. Rollback publishes previous x/y before restoring
path progress. Reapplication rounds the y and x additions separately before
calling the ordinary position setter, retaining spatial/collision dirtiness.
Component tests compare host binary32 sums (including ties and signed zeros),
then compose paired dispatch with maintained-owner movement and handler-modified
engine speeds. Unsupported numeric values fail before publication. The original
runtime FP-environment audit remains a separate requirement.

The post-collision preparation call `0x462820` concerns paths; it is not the
friction/gravity routine `0x462dd0` used earlier in the frame. The pinned data's
PATH chunk contains zero resources, and recovered assembly currently contains
no path API or engine hspeed/vspeed references. Do not map the port's custom
actor velocity fields onto these engine fields, or infer that they must be
zero: built-in speed/direction writes can also change the engine components.

The previous-position snapshot is at `0x50dbf8..0x50dc0a`, immediately before
the same instance's animation call `0x50dc25`, in active-room traversal order.
It does not add the Step exclusion-mask check. Path progress is also copied
to its previous field when a path state exists. Birth initializes previous
coordinates from its initial position (`0x461957`/`0x46197d`).

The maintained spatial owner now initializes this snapshot at birth and exposes
a checked snapshot operation. The host calls it from the existing room-list
animation traversal before animation and Begin Step, using authoritative
binary32 position rather than rounded drawing coordinates. This adds eight
bytes per owner slot (16 KiB for 2,048 slots), without allocating per frame or
dirtying spatial/collision queues. Legacy bindings without an external owner
remain unchanged. The native collision owner likewise captures each registered
active-room entry immediately before its animation dispatch (currently enemies,
bullets and scoped script effects). Retry preserves the traversal binding.
Unregistered families and path state remain open.

`StartupRuntime::publishCollisionPosition` publishes exact binary32 x/y through
the installed lifecycle owner, then updates integer drawing x/y. It must not
rewrite private actor motion or the legacy `preciseX`/`preciseY` motion caches.
The owner exposes exact current/previous coordinates and routes positional
rollback/reapplication through this setter. Engine velocity is explicit, not
borrowed from custom motion. Stale handles, unsupported drawing conversion or
missing ownership fail the world rather than silently losing a position write.
Live tests cover fractions, snapshot rollback, speed reapplication and unchanged
motion caches/RNG. This does not yet execute the full pair pass automatically;
engine-field setters, handler execution and native publication still need wiring.

`RunnerCollisionEventContext` specializes dispatcher `0x4449e0` for event 4.
No current room (`0x4449e6`) or a pending room change (`0x4449f8`) suppresses
collision execution. The pending-change exception list permits event types
0, 14, 1, 2 and 7, not collision type 4; persistence does not bypass that list.
Lookup happens only after these guards. The defining owner from the handler
entry becomes `event_object`, while the actual opposite object's ID becomes
the subtype. The runner saves/restores those globals at `0x444ab1..0x444b05`.
The portable context similarly restores the outer frame on success or checked
failure, latches nested failures, and rejects recursion above 32 (the source
reports a runtime error at `0x444a97`). The stored-world adapter supplies real
handle and binding resolution, with execution explicitly delegated to its
caller. The caller must supply actual room-transition state on every call;
the legacy host room-change timing has not yet been adopted as source truth.

`tools/verify-runner-collision-phase.py` pins these reviewed regions through
the existing whole-executable hash guard. The scheduler's optional checked
collision callback runs after Step reconciliation and before End Step,
followed by another reconciliation. It does not yet enumerate collision pairs.
`StartupRuntime::stepWithCollisionPass` installs that callback for one ordinary
tick and removes it on return, including failure. `stepWithTimeStopEffects` can
compose it with registered animation, alarm and Step effects in the same tick.
A failed pass stops End Step and post-scheduler gameplay work; recursive scoped
ticks latch failure even if the outer callback ignores the error. The live
fixture executes collected shard contacts at this boundary and verifies that
cooldown contacts consume no RNG and the callback cannot survive into another
tick. Scheduler tests also cover collision births and membership reconciliation.

Normal host gameplay still uses legacy contact loops after End Step, and the
native scheduler has not adopted this pass. Admitting all handlers requires
moving their existing work to the source phases and removing the replaced
routes; do not enable the pass alongside duplicate contacts or skip unknown
handlers. Engine movement and pending-room semantics remain explicit inputs.

### Collision candidate collection evidence

The spatial callback at `0x503000` rejects self and candidates whose instance
stamp (`+0x184`) equals the current collision pass. It looks up event type 4
first on the current instance's object for the candidate's object ID, and only
if absent checks the reverse direction (`0x503030..0x503050`). A match in either
direction appends `(current,candidate)`; it does not execute the handlers yet.
After the current instance's spatial query, `0x502474` stamps that instance.
`0x5025b6` snapshots the accumulated pair count and dispatches in append order.
Thus reverse-pair suppression depends on outer traversal order, not sorted IDs.

`RunnerCollisionPairs` models this collection for externally supplied ordered
candidates, with checked capacity and generation-tagged deferred handles.
`RunnerSpatialInstances::collectCollisionPairs` supplies the maintained dirty
queue and spatial-index traversal. It flushes spatial updates first, normalizes
cached bounds, skips excluded current instances without stamping them, and
does not filter candidate exclusions prematurely. The pair-processing routine
owns those later tests. A missing global index skips the flush/query and clears
the collision queue, distinct from querying an empty tree.

After each eligible query, the runner unlinks the current instance, tests flag
`0x200`, resolves the mask (or sprite if the mask ID is negative), and retains
it only for resources with more than one frame. Two prepend passes preserve
the retained query order for the next frame (`0x50242a..0x5025b4`). The shared
owner uses a temporary slot array (four bytes per capacity slot) and reverse
prepend to produce the same order. Resource resolution and inherited event
lookup use checked callbacks; callbacks cannot mutate lifetimes. Failures
latch the owner and preserve completed mutations rather than retrying.

The stored-world adapter resolves those callbacks from actual lifetime records,
sprite/mask resources and `ObjectPack::collisionEventExists`. Existence follows
both ancestor chains: `0x478830` retains missing owner-ancestor entries, while
`0x478b60` and `0x4782a0` expand collision targets to descendants and preserve
inherited collision entries. `collisionEvent` additionally selects the handler:
nearest target ancestor first, then nearest owner ancestor for that exact
target. `0x47b812` performs owner inheritance for every object before the
expansion pass at `0x47b893`. Expansion at `0x478341` skips an existing child
entry and its subtree. Thus existing, more-specific target entries are barriers,
independent of hash-table traversal order. The later copy from the expanded
parent at `0x478df6..0x478e1f` fills only absent keys.

The selected binding retains its serialized owner, subtype and code. These
are not the dispatch context: `0x50319c..0x5031b5` passes the actual opposite
object ID as event subtype. `StoredWorldCollision::collisionEvent` resolves
current handles when called, not cached object identities from collection.
It deliberately does not add a destruction guard between pair directions.
Both forward and reversed table-propagation tests cover all recovered pairs,
including exact code/owner/subtype; a synthetic cross-specificity test separates
target-first precedence from the incorrect owner-first ancestor walk.
The ordinary
`findEvent` lookup of an exact subtype is not sufficient for collision presence.
The original routines are pinned by `verify-runner-field-lookup.py`.

`StoredWorldCollision::executeCollisionPairs` consumes a sealed buffer once in
append order, composing live precise overlap, collision requeueing, authoritative
position rollback/reapplication and the event context above. The caller supplies
checked engine-velocity/path operations and fresh room-transition state for each
event direction. It must defer physical retirement until the captured pass ends.
Recursive collection or execution fails terminally rather than invalidating the
buffer; a failed callback preserves completed effects and prevents later pairs.

The live integration fixture collects real shard/player candidates, expires the
shards' contact cooldown, then executes recovered handler 2081 through this pass.
It checks precise-hit count, two RNG draws per contact, `noTouch` and alarm 2.
The fixture rejects any unexpected solid-motion callback rather than supplying
invented zero velocities. This is explicit pass execution, not automatic normal
scheduler admission or complete collision-handler coverage on either backend.

The pair buffer's completed-instance set models
one pass; source counter overflow and concurrent mutation during collection
are outside its contract. The runner evidence verifier pins these regions.

## Step creation cutoff

### Object and instance traversal order

The table builder `0x47a330` is called with event type 3 and subtype upper
bound 16 at `0x47a585`. For each subtype it scans numeric object IDs from zero
through `objectCount-1`, looks up that object's resolved event, and appends
qualifying IDs. Therefore the dispatcher's outer order is **ascending object
resource ID**, not room creation order, depth, hash-bucket order or the
ancestor that originally supplied an inherited event.

Counts use `0x941d40 + (type*256 + subtype)*4`; array pointers use
`0x93a544 + (type*256 + subtype)*8`. For Step these become precisely the
`0x942940` and `0x93bd44` tables read by `0x50b7e0`.

The inheritance pass `0x478830` walks ancestors and copies missing event-table
entries into the child; an existing child key is retained. Lookup `0x478f40`
then reads the resolved table. An explicit empty override still has its own
event entry; it is not equivalent to an absent handler.

Within each selected object, `0x50b895` loads head `object+0x38` and follows
node `+0`, reading the instance at node `+8`. Registration `0x478380` appends
to this exact-object head/tail pair `+0x38/+0x3c` **once**, before its ancestor
loop. The loop instead appends to family head/tail `+0x44/+0x48`. Step uses
the first list, so descendants are not dispatched again through the parent's
collision-family list. Same-object dispatch follows registration order;
completed activation reconciliation can change it as described in
[activation fidelity](activation-fidelity.md).

`tools/verify-runner-step-order.py` pins the reviewed builder, inheritance,
registration and traversal ranges to the original executable. This establishes
the ordering contract. The host phase snapshot now consumes exact-object
registration order and sorts by resource ID with registration ordinal as its
tie-breaker. Native enemy alarms and ordinary Step use the same snapshot,
captured from the resource-backed owner's registrations. This orders enemies
within their current scheduling block; it does not yet interleave all native
families according to the complete event table.
Keep the phase cutoff and immediate flag rechecks below when integrating it;
sorting by resource ID alone does not handle reactivation or new lifetimes.

Read-only inspection of the pinned Windows `Downwell.exe` establishes a
phase-local creation cutoff, not a whole-frame exclusion rule.

- Instance initialization at `0x461778` loads the current 64-bit counter from
  `0x9360a0`/`0x9360a4` into instance offsets `0x60`/`0x64`.
- The Step dispatcher `0x50b7e0` saves the old counter at `0x50b813` and
  increments the global counter before traversing event-bearing objects.
- At `0x50b8ab` it filters instance flags with `0x100003`. At `0x50b8b6`
  it compares the instance creation timestamp against the saved cutoff;
  a larger timestamp skips dispatch. Eligible instances are passed as self
  and other to `0x444910`, event type 3 and the requested subtype.
- The frame loop calls this dispatcher with subtype 0 (ordinary Step) at
  `0x50dd76`, then with subtype 2 (End Step) at `0x50def3`. Begin Step uses
  subtype 1 earlier at `0x50dc7f`.

Therefore a bone created by skeleton ordinary Step does not run its own
ordinary Step in that same phase, even if traversal reaches its object list
later. A subsequent Step phase takes a new cutoff; the creation timestamp
does not itself exclude the child from that later phase. This does not prove
collision traversal, draw eligibility, activation flags, or ordering among
instances of the same object. Those remain separate integration questions.

The decoded host's `EventScheduler` snapshots `InstanceHandle` lifetimes rather
than numeric instance IDs. Deleting a not-yet-visited record and reusing its ID
must not transfer phase eligibility to the replacement. Handle resolution also
follows surviving records through compaction without linear ID searches.
Each ordered phase snapshot costs sixteen bytes per capacity slot. The bounded
heap sort allocates no heap storage and is shared portable code; native users
must place a large snapshot outside the task stack. `GameplayWorld` keeps its
64-enemy workspace in the world object, captures ordinary Step eligibility
before generation, and resolves each lifetime again before dispatch. Its
unbound component fixtures use birth order for same-object ties; production
installs the collision owner's registration-order capture before stepping and
preserves that binding through Retry. Runtime object rebinding during an
executing phase and separate effect-family scheduling still require integration
checks.

`GameplayCollisionOwner::dispatchStepPhase` provides the heterogeneous native
counterpart using a caller-owned snapshot. It captures registered player,
terrain, breakable and enemy lifetimes together, sorts by actual object ID and
registration order, and resolves flags/lifetimes again before each callback.
Resource lookup retains explicit empty own events (for example object 99's
End Step 1112 instead of inherited wall code 986). Callbacks execute the selected
binding; neither unknown handlers nor callback failures are implicit success.
Capacity failure happens before dispatch, and checked failures latch the owner.
Do not reuse the supplied snapshot recursively during a callback.

The native scene gate exercises this traversal with a resource-probe callback
and static workspace. Component tests compose it with actual precise wall
events and cover reactivation order, same-phase destruction/slot reuse and
failure cutoff. This is not yet the production End Step scheduler: all live
handlers and remaining families must be admitted in source order, replacing
their existing paths rather than adding a second terrain-only pass.

Destruction bit 1 is part of the Step filter `0x100003` at `0x50b8ab`
and the alarm filter at `0x509dfb`, before alarm decrement. `EventScheduler`
therefore excludes `destroyRequested` records both at capture and at each
dispatch, including subsequent alarm indices. The current handler continues;
the record is not physically removed by the dispatcher. Tests cover destruction
in every Step/alarm phase, a later already-snapshotted sibling, unchanged alarms,
and exclusion on the following frame while storage remains allocated.

The decoded host publishes deferred object membership after each complete Begin
Step, alarm, ordinary Step and End Step phase, before the next phase snapshot.
The Windows frame loop calls reconciliation after those phases at `0x50dc89`,
`0x50dceb`, `0x50dd80` and `0x50defd`. Requests during a handler must not reorder
membership immediately. `RunnerRoomMembership` follows `0x48d630`: promote in
inactive-room-list order, then demote in active-room-list order. Active insertion
uses ascending binary32 depth and inserts after equal keys; demotions append to
the inactive tail. Storage compaction and request order are not substitutes.
The fixed-capacity room lists add 16 bytes per slot plus four list endpoints;
the decoded host's 2,048 slots therefore add 32 KiB plus endpoint storage.

The host supplies room/layer depth for placed instances and VM compatibility
depth for dynamic births. This adapter does not yet model
runtime depth changes, persistent-instance room re-registration, the complete
Windows destruction queues, or the additional manual actor/collision passes
after `EventScheduler`. Nor does it publish spatial-index updates or wire the
Amiga world actor families into these lists. Those integrations remain in the
[work queue](open-work.md). Component publication tests do not establish live
`instance_place` identity equivalence.

`BoneProjectileStore::beginPhase()` captures a monotonic handle cutoff before
any actor executes that phase. `visitPhase()` excludes newer handles and
retired instances, including a newly created child occupying a not-yet-visited
slot. The world must capture before skeleton Step, not just before visiting
bones, and must capture anew for a later phase. Store slot order is not claimed
to reproduce every original instance-list ordering.

Normal and sanitizer tests cover creation after the cutoff, eligibility at
the next cutoff, retirement/recreation of a later slot during iteration and
the existing 2,048 owned creation/RNG cases. These remain component tests;
the gameplay world has not yet adopted the store.

## Correction to existing live enemy adapters

The integration audit found that both existing enemy adapters manually ran
new builder children immediately after the builder, outside the dispatcher.
`GameplayWorld` now captures ordinary-Step enemy lifetimes before generation;
`StartupRuntime` marks records only when visited by ordinary-Step dispatch.
Their manual enemy Step passes exclude newly created records. Later collision
processing is not excluded by this flag.

Both adapters also advance existing enemy animation before Begin Step/alarms,
not inside the later manual Step pass. Newly generated enemies therefore keep
their Create frame, while alarms and hit handlers can reset an already-advanced
frame without an extra same-update increment.

The decoded host's deferred creation path now initializes the serialized sprite
before Create. Previously its missing sprite was hidden by the immediate Step
presentation update. The route fixture explicitly checks that a new airborne
frog retains its serialized sprite 166/frame zero until its first Step rather
than requiring the later airborne pose immediately. The existing cross-adapter
actor-state comparisons remain enforced.
