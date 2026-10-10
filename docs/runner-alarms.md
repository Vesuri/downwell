# Windows alarm phase

Read-only disassembly of `data/Downwell.exe`, SHA-256
`340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085`.
`tools/verify-runner-alarms.py` pins the reviewed regions; it is a change guard,
not execution of the original game or proof about other runner versions.

## Storage and phase evidence

- The `alarm` string is at `0x7fe358`; registration at `0x430c23` binds the
  getter wrapper `0x42c560` and setter wrapper `0x431260`.
- The underlying getter `0x469060` and setter `0x46bf80` access twelve signed
  32-bit values at instance offset `0xf4 + index*4`.
- The setter wrapper converts VM doubles with `cvttsd2si` at `0x431299`
  before forwarding an integer-valued number to its final integer conversion.
  Finite in-range fractions are truncated toward zero, not nearest-even.
  Thus a positive `random_range(90,160)` lifetime is `90+floor(random(70))`;
  its RNG draw must remain after the preceding image-index sample.
- The phase initializes EBX (alarm index) to zero at `0x509da8`. Within its
  per-index traversal it visits instance lists and filters instance flags and
  creation timestamps before touching an alarm (`0x509dc0`–`0x509e12`).
- At `0x509e15` it reads that alarm. Negative values skip decrement; otherwise
  it reads, decrements and writes back at `0x509e21`–`0x509e2b`. It reads again
  at `0x509e33` and dispatches only when the current value is zero.
- Dispatch at `0x509e42`–`0x509e47` supplies index EBX and event type 2 to
  `0x444910`, with the instance as both self and other. It returns to the
  traversal immediately, not to a deferred event-mask pass.
- After visiting the lists for that index, `0x509e71` increments EBX. The
  table cursor advances eight bytes from `0x93b544` to `0x93b5a4`, covering
  twelve slots in ascending order.

Consequently, all eligible instances' Alarm 0 processing precedes Alarm 1,
and so on. An event's write to a later alarm is seen during this same phase;
a write to an already-visited alarm waits until the next phase. Countdown of
zero retires it to -1 without dispatch. Negative values are left unchanged.

The shop keeper is a concrete mutation case: while sprite 487 is selected,
Alarm 0 writes image index 1 and sets Alarm 1 to 5. Alarm 1 is then decremented
to 4 in that same phase, even if its old value would have fired. Four further
phases trigger Alarm 1, which resets the image and assigns Alarm 0 an
`irandom_range(20,60)` result. That earlier slot is not decremented again until
the next phase. Pre-decrementing every slot and dispatching a saved event mask
changes both blink duration and RNG consumption when both counters were due.
The keeper handlers are hash-pinned in `verify-runner-instance-ids.py`.

### Object and instance traversal

Alarm table construction at `0x47a57c` passes event type 2 and subtype upper
bound 12 to `0x47a330`, the same ascending-resource-ID builder used by Step.
The common table addressing selects count base `0x942540` and object-array
base `0x93b544`; the dispatcher loads these at `0x509d97`. Construction's
inclusive upper bound does not add a thirteenth runtime alarm: this phase
still ends after the twelve slots described above.

Each array entry is resolved through the object map. The successful lookup
at `0x509e51` loads the object pointer and branches to `0x509de6`, which reads
its **exact-object** list head at offset `0x38`, not ancestor-family head
`0x44`. The shared insertion/removal routines reviewed for Step therefore
also establish registration order within each exact object for alarms.
An instance is read from node offset 8 at `0x509df4`; the next node is saved
at `0x509e02` before filtering and dispatch. This is not Draw order or native
array-slot order.

The static audit now establishes alarm-index / ascending-object-ID /
exact-object-registration ordering. It does not establish all list-mutation
or same-phase birth cases: the saved-next walk and creation timestamp filter
still need to be preserved when replacing it with snapshots. In particular,
do not infer that every object born during Alarm 0 may run Alarm 1, or that
recapturing all lifetimes at every alarm index is equivalent. The verifier
guards these alarm instructions and reuses the Step builder/list guards.

`GameplayCollisionOwner::dispatchAlarmPhase` supplies a native mixed-family
captured-lifetime traversal with this ordering, rechecking registered identity
and active/destruction flags before each callback. Storage belongs to the
caller, not the small target stack. The callback decrements its actor's alarm
and dispatches immediately on zero; missing alarm events do not receive a
countdown. Tests mix real box/enemy/debris registrations, preserve same-object
registration order, modify a later debris alarm in the same phase, exclude
newborns from the snapshot, suppress a destroyed later lifetime, and stop after
callback failure. Registered-reference gameplay uses the same dispatch body
for boxes, enemies and debris, with a caller-resident snapshot restricted
to supported families. Native-owned families use the prefix/roster routes
described below. Unbound component worlds retain their enemy-only
fallback. Actor alarm bodies are shared between both routes; Retry preserves
the bound phase callback. A normal-frame test covers debris's thirty-tick
Alarm 0 expiry and cleanup before and after Retry. This is not a complete
global alarm owner: other families, the remaining original birth/mutation
cases, and debris Step's time-stop refunds still require integration.

The sorted snapshot permits skipping an entire contiguous object group when
its immutable metadata has no event for the current alarm. This shortcut is
valid only while the owner's binding revision matches capture; any birth,
retirement, rebind or clear forces the remaining traversal back to live
instance/object checks. No callbacks occur inside an absent group, so finding
its end by binary search preserves callback and countdown order. Present-event
groups still resolve each lifetime and check live flags before dispatch.

The bound combat path retains its sorted snapshot across ticks until a mutation
affects either of its captured slot ranges: `[enemyBase,terrainBase)` or
`[debrisBase,capacity)`. Attach/retire/rebind, actual registration changes, and
changes to exclusion bits `0x100003` within those ranges invalidate it; clear
always invalidates it. Uncaptured terrain births/retirements do not. Position,
image and bounds-dirty writes do not change capture eligibility. Repeated no-op
activation/registration writes do not invalidate it. A sticky dirty flag avoids
revision wraparound; it is reset after capture but before callbacks, so an alarm
that changes captured membership or eligibility forces recapture on the next tick,
without admitting newborns into the current phase. Attaching a world also
invalidates the cache. Reuse omits only capture and sorting, never the live
per-callback checks or alarm countdowns. Explicit caller-snapshot dispatch still
captures fresh; the cache belongs exclusively to the bound combat owner.
The watched ranges must match the registration ranges used to build that
capture. Extending admitted families requires updating both. This observer
does not narrow the global binding revision: any birth, retirement or rebind
during dispatch still disables its absent-group/column shortcuts.
The union of present alarm columns is prepared with that capture and retained
alongside it. An unchanged capture does not rescan its object IDs to rebuild
the union. Recapture recomputes it, including empty captures. A binding change
during a callback still disables absent-column skipping for subsequent alarms;
the retained union never replaces live eligibility or current-object checks.

`ObjectAlarmMetadata::bind` prepares all inherited alarm handlers before
gameplay. Each object has a 12-bit presence mask and a sparse-row offset;
present handlers store the original owning object and code ID in alarm order,
including empty own events. Lookup ranks the lower presence bits and reads
the prepared row, with no inheritance walk or cache eviction during gameplay.
The original pack expands to 389 handlers. The 512-handler capacity reserves
4096 bytes, plus 1544 bytes for 386 masks/offsets and a four-byte object count.
Binding fails closed on insufficient capacity or invalid input; partially
prepared rows cannot be queried. Rebinding rebuilds the table. It never
contains an instance handle, countdown or exclusion flag.
Dispatch still resolves the current object after binding changes and checks
each lifetime immediately before invoking its handler.

The bound combat adapter checks the live compact enemy Alarm 0/1 and box
Alarm 0 counters after lifetime/exclusion checks. Negative values bypass
handler lookup and the countdown adapter, matching `0x509e15`; zero still
enters the adapter and retires to -1. This is not a cached armed-alarm mask:
each read occurs at its original traversal position, so earlier callbacks can
change a later counter in the same phase. Other alarm indices and effect
families keep their existing adapter/validity checks. The general caller-owned
`dispatchAlarmPhase` remains unfiltered because its callback owns countdown
storage and may use a different representation.

Native boxes are excluded from that captured array. They never re-register
or compact, so the phase captures only the box count and the highest issued
box lifetime ID. Box Alarm 0 then advances over that slot prefix at resource
92's position within alarm-index order. Newborns, replacement slots after a
room or configuration reset (lifetime above the ceiling), inactive boxes and
boxes marked for destruction are skipped, as the snapshot would have done.
`REFERENCE_BOX_ALARMS=1` (native build) retains the captured snapshot as
the oracle. `make verify-gameplay-collision-owner` checks the prefix against
newborn, destroyed and replacement boxes, and armed boxes beside idle ones.

### Mutation-owned native enemy order

The native-scenery combat owner maintains enemy alarm entries at birth,
retirement, registration and eligibility changes. Entries carry the exact
object, registration order, handle generation and immutable alarm-event mask;
they never cache countdown values. Room reconciliation updates this owner too.
A phase pins the current bank. Mutations copy it only while it is pinned,
preserving the captured lifetimes without rebuilding or sorting every tick.
Live flags and generation are checked immediately before each typed callback.

This route admits only enemy resource IDs between 92 and 211, between the
box prefix and later captured effect families. Dispatch remains alarm-index
first, then resource and registration order. Unsupported metadata/order
disables the roster and invalidates the captured-list cache, retaining the
complete generic path. This does not broaden supported enemy event bodies or
prove additional original same-phase birth cases. The two 64-entry banks use
ordinary memory, never Chip RAM. `REFERENCE_ENEMY_ALARM_ROSTER=1` retains the
original dispatcher on both targets; `verify-native-enemy-alarm-roster` and
the collision-owner tests cover pinning, reuse, failure prefixes and order.

## Skeleton consequence

Its Alarm 2 sets Alarm 3 to 15. Alarm 3 is then decremented to 14 in the same
phase, even if its old value was 1. After fourteen more alarm phases, Alarm 3
restores `imgSp = 0.3`. A snapshot of all countdown results before dispatch
would incorrectly fire the old Alarm 3 or delay the restart.

`SkeletonTiming::advanceAlarm(index)` supports the required per-slot immediate
dispatch. The future owner must iterate indices outside instances; calling
all four slots for one instance at a time is not generally equivalent.
Tests pin the overwritten-expiring-alarm case, restart interval and zero
retirement. The skeleton remains unregistered pending remaining behavior.

`AlarmBank::advance()` remains a snapshot API for non-dispatch uses. The decoded
`EventScheduler` now instead counts down and dispatches one slot at a time,
with indices outside instances. Tests cover inter-instance writes to later
and earlier slots, overwriting an expiring alarm, and deletion during dispatch.
The existing phase-start ID snapshot still bounds participating instances;
this change does not claim complete original instance-creation eligibility.

The shared gameplay owner also now visits its explicit Alarm 0/1 slots in
global index order with immediate dispatch. This fixes an existing frog case:
Alarm 0 writes Alarm 1 to 60, which must finish that phase at 59, rather than
60 as before. Provisional queued hit delivery and separate hit-stun timers
remain; these are not a complete original event system. Skeleton admission
still requires the additional slots and lifecycle integration.

Validation after owner correction: scheduler tests and sanitizers pass;
paired host natural routes retain exits at 1,339/1,646 updates and the seed-4
shop at 518 updates. The return path remains 43 executed updates; its previous
log printed the zero-based loop index 42, not a changed route duration.
The bounded native CPU replay passes with unchanged digests. The separate
rendered natural gate also passes: 211 clear frames, 168 exit-field frames,
49 shop-HUD comparisons, 43 return updates and 688 total second-level updates,
with an intact stack guard and ordinary 68000 build restored. An initial
attempt incorrectly expected 687/42 based on the misleading host log and
failed; the final run retains the original 688/43 assertions. These checks
do not independently prove the original runner's instance traversal order.
