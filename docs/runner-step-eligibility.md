# Creation during a Step phase

## Collision boundary

The normal Windows frame loop calls Step at `0x50dd76`, reconciles at
`0x50dd80`, and later conditionally enters the collision pass `0x5022c0`
at `0x50decc`. It reconciles again at `0x50ded1`, then calls End Step at
`0x50def3`. The condition at `0x50dec8` skips this pass when BL is nonzero;
the alternative path is not modeled by this boundary.

The collision pass calls pair processing `0x5030d0` at `0x5025cb`. Pair
processing checks overlap and destruction flags, then invokes event type 4
at `0x5031a6` and `0x5031b5`, swapping self/other and selecting the other
object's ID. Solid-instance position rollback/reapplication surrounds those
calls. This establishes a separate phase and bidirectional dispatch, not the
complete candidate ordering, mask rules or solid response semantics.

`tools/verify-runner-collision-phase.py` pins these reviewed regions through
the existing whole-executable hash guard. The scheduler's optional checked
collision callback runs after Step reconciliation and before End Step,
followed by another reconciliation. It does not yet enumerate collision pairs.
Its tests cover phase placement, collision births, activation/destruction and
failure cutoffs, not original collision ordering or backend integration.

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

This does not yet supply live scheduler wiring, precise overlap dispatch or
solid response. The pair buffer's completed-instance set models
one pass; source counter overflow and concurrent mutation during collection
are outside its contract. The runner evidence verifier pins these regions.

## Step creation cutoff

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
Each phase snapshot costs eight bytes per capacity slot; this is not the
Amiga `GameplayWorld` actor scheduler and does not establish source object-list
event traversal.

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
`GameplayWorld` now captures its ordinary-Step enemy count before generation;
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
