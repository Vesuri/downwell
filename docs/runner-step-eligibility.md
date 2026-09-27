# Creation during a Step phase

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
