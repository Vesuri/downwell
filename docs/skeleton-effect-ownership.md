# Skeleton effect ownership before live admission

Source: untouched Windows `data.win`, SHA-256
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`,
UTMT 0.9.2.0 export. Resource metadata identifies objects; the corresponding
`Assembly/gml_*.asm` scripts establish argument evaluation and callback order.
This is an integration audit, not a claim that these effects are implemented.

## Death is not just removal plus a gem reward

`global.killCount` belongs to the run, not the room. The VM assigns zero in
`scrGlobalStuff` (called by `scrInitialize`/`ResetPlayer`), increments it in
`scrEnemyDeath`, and reads it in `DeathMenu`/`EndMenu` Draw. `scrNextLevel` and
player Create do not reset it. Both live adapters use `RunSession::kills`,
preserving the results total across rmMain reentry and clearing it on new-run
reset. This ownership rule does not validate their current immediate death
accounting or replace the missing source death-effect sequence.

`SkeletonActor::step` already exposes the source call sequence. Its live owner
must complete each effect's Create synchronously before running the next call.
Batching all effect creation after currency generation changes the shared RNG
stream, even when the final list of visible particles looks identical.

Effect-producing callbacks return success. `SkeletonActor` stops the event on
failed enemy-death setup, blood, smoke, flash, currency, corpse or debris
creation; stomp blood and each Exploding Knees smoke emission have the same
contract. Already completed
child Creates and RNG draws are retained. A false result is terminal for the
owning simulation: do not retry the event, award later rewards, or retire the
actor as though the sequence completed. `verify-skeleton-actor` injects failures
at each of these boundaries as well as checking the successful source order.

The `enemyDeath` callback is checked too: `scrEnemyDeath` VM block 1 updates
combo, calls `gunshotStop`, updates kill/achievement state, calls `momentDelay`,
then calls `levelBeginCue`. That last script's blocks 1–6 create object 372
when `!firstLand && !pTimeStop && area != 5`. Its Create completes before
`firstLand` changes and before the audio work. Failure must retain prior
bookkeeping but stop before blood, currency, retirement and the event tail.
The actor test covers this boundary with and without time-stop; it does not
claim that the live worlds yet implement the script or level-title creation.

Both runtime adapters route flash bursts and pre-Step flash animation through
`RunSession`'s shared `effectsSucceeded` latch. Failed bursts retain earlier
children and consumed random words; subsequent effect calls and animation must
not retry the partial operation. Room/run reset clears the failure only after
retiring owned effects. This is a port capacity/error contract, not a claim
about Windows allocation failure or completed natural skeleton integration.
The SDL `Game` latches simulation failure separately from ordinary quit and
does not dispatch more updates afterward. The executable returns 20 before
rendering/presenting the failed update, matching the Amiga effect-error path.

The current shared cavern stomp adapter checks reward success before its
immediate enemy retirement, defeat count and player bounce. A failed reward
retains prior combo changes and partial gem births, but stops contact traversal
and publication. Both worlds stop before entering that pass if an earlier
effect failed. This does not validate the adapter's immediate-retirement
approximation against the original later enemy death Step.

Bullet impacts similarly return an explicit failed result after unsuccessful
reward creation, retaining prior damage and hit state without counting defeat
or retiring the enemy. Checked projectile dispatch distinguishes failure from
ordinary shot consumption: retain completed animation/initialization, but stop
before later collision queries, motion, position publication and retirement.
SDL compaction preserves the failed shot and untouched tail along with prior
successful retirements. Both worlds stop before later actors after failure.
These capacity-fault contracts do not establish original collision/death timing.

Projectile birth must also report exhausted storage instead of silently dropping
the shot. Both adapters latch session failure before any later projectile Step;
the native adapter stops before alarms/generation on full storage or exhausted
lifetime IDs. Already consumed ammo and emitter changes are retained, and the
full pool is not overwritten. Controlled world tests park existing projectiles
and refill ammo to reach capacity; they are fault tests, not natural encounters.

| Source call | Owned objects / side effects required |
| --- | --- |
| `scrEnemyDeath` | Combo/kill counters, gunshot stop, achievement state, moment delay and level-begin cue |
| `scrBloodfx(10,3)` | **Two** object-220 moving effects, sprite 84; the function ignores the supplied arguments |
| `scrSmokefx(xx,yy,5,1)` | Object 225; variable count and per-child random position/velocity/animation |
| `scrFlashballfx(xx,yy,6,2,10)` | Object 224; variable count, random birth position and additional Create jitter |
| `scrCurrencySpawn(money)` | Existing currency owner, after all preceding effect Creates |
| Death sound 132 | Source position in call sequence; playback need not own RNG |
| `scrDeadBody(206,takenImpact,102)` | Object 263 Create, then sprite/velocity/facing overrides; also modifies the dying skeleton's xsp |
| Two debris Creates | Object 370 twice at published `(x,y-5)`; four RNG draws per Create, no terrain relocation |
| Destruction | Request retirement; finish the current event's impact reset and hit presentation first |

## Creation contracts that cannot be replaced by an arbitrary RNG skip

- Blood emits at published x/y, not the precise xx/yy accumulators. Its two
  angles are `random(20)` and `180-random(20)`, respectively. Object 220's Create
  initializes motion state without RNG; `emitMovingFx` then assigns sprite,
  animation speed, angle and the direction-derived velocity.
- Smoke count calls `irandom_range(-vary,vary)`. For each child, the VM draws
  the **Y choice before the X choice**, then immediately calls object 225's
  Create. That Create samples speed, animation multiplier and direction before
  the next child's coordinates are chosen. It stores xx/yy and leaves the
  constructor's animation index 0 unchanged. Coordinate choices use `choose`,
  not `irandom`. The previous index-1 description incorrectly conflated smoke
  with flash-ball Create, which explicitly writes 1.
- Flash-ball count also uses `irandom_range`. Its per-child Y range draw
  precedes X. Object 224's Create then adds independent integer jitter to X
  and Y and sets image speed to 0.3. Those extra draws must not be omitted.
- Corpse Create inherits object 90's health/hit defaults, then samples X/Y
  velocity and chooses signed animation speed. Keep those samples even though
  `scrDeadBody` subsequently overwrites velocity. The wrapper caps impact at
  four, sets the new corpse's sprite/facing/explosion effect, and replaces the
  **source skeleton's** xsp with facing times 1.5 when its magnitude is below
  0.5. Zero impact still gives the corpse a horizontal speed of magnitude 0.5.
  **Corpse** Create also queries walls/tiles. A blocked birth snaps to the grid
  and runs a bounded random relocation search, then republishes xx/yy. The
  velocity and animation-speed samples precede that relocation loop, so
  terrain-dependent draws follow them.
  This work belongs to object 263, not debris object 370.
- Debris has no parent and its Create does not query terrain. It samples
  xsp=random_range(-2,2), ysp=random_range(-0.5,1), chooses imgSp from
  (-0.5,-0.3,0,0.3,0.5), copies it to builtin image_speed, then samples
  image_index=random(7). Area selects the sprite. Both instance-count branches
  assign alarms 0/1/2 to 30/15/7, so the count does not change the result.

`createDeadBody` implements the `scrDeadBody` wrapper, not object 263's Create
or physics. Its adapter must complete inherited Create, random samples and
terrain relocation before reporting the new lifetime. Sprite, vertical speed,
facing and explosion setters then run in source order. The source's `xsp`
fallback and final corpse horizontal speed calculation follow those writes.
`DeadBodySource` holds caller fields that must be published back to the dying
instance; it is not a copy of corpse state. Setters use normal live mutation
hooks, including sprite assignment's animation/collision consequences.
`verify-dead-body-creation` pins the VM script and checks scalar branch
boundaries, synchronous caller mutations and terminal callback failures.

`CorpseBirth` supplies inherited object-90 defaults and object-263 Create,
including the three initial random draws and terrain-dependent relocation.
Its wall adapter queries parent 57; its tile adapter queries layer 100. The
owner publishes the constructor position before Create and each subsequent
builtin coordinate write through the supplied publication callback.
The source short-circuits wall/tile checks, samples X before Y, tests candidate
walls but origin tiles, increases radius to 36, and applies the last candidate
after 33 blocked attempts. Keep these quirks: correcting them changes behavior
and the random stream. Move-snap uses float division and integer nearest-even
rounding before multiplication. `verify-corpse-birth` checks the pinned Create
events/runner, full RNG state, query coordinates, publication order and failures.
Step, collisions, animation, lifetime ownership and live admission remain
separate work; this test does not establish the world's collision decisions.

`CorpseExpiryEffect` models object 242 (`fxJobutsu`): Create assigns the
single-rounded speed 0.3, with sprite 102's twelve unit sequence keys. Its
parent 219 (`parentNoloopFx`) destroys it at Animation End. Neither object has
a Step or Draw override: no TimeStopBound, movement or drawFx surface route
is applicable. Default position and sprite drawing remain scheduler/backend
responsibilities. The connected alarm test creates the child before corpse
retirement and advances it for its 40-update lifetime; the cross-CPU corpse
replay hashes the child's position and animation too. `setSprite` accepts
verified nonempty unit-track metadata and publishes changed bounds; unsupported
empty sprites and failed publication are explicit failures. Natural corpse
death/expiry production and additional corpse families still need wiring.
`verify-corpse-expiry` pins the VM events, inheritance and sprite metadata.
The gameplay DWAP includes all twelve sprite-102 frames as immutable planar
color/mask payloads, anchored at the original (16,32) origin. Checked
`EffectMaskAssets::corpseExpiry` lookup rejects absent or malformed frames
without modifying the caller's descriptor. No DMA sprite copies or runtime
conversion are needed. The compatibility depth table places this ordinary
sprite at -1000 (not its OBJT default 0, nor the corpse's 1000); the live painter
must merge it with other families at that depth, not the effect surface.

`CorpseExpiryEffects` provides bounded independent child ownership and monotonic
owner-local handles. Birth publishes constructor fields before Create assigns
the 0.3 rate; sprite mutation publishes only on a changed ID. Capture its
animation cutoff at global phase entry, before any family dispatch. Animation
End retires external membership exactly once; a failed publication/retirement
or exhausted capacity is terminal until explicit cleanup. Cleanup can resume
after a partial failure without repeating successful removals. These hooks
must not mutate the same owner and must be recreated after world copies.
The connected alarm/owner test covers survival after corpse retirement, stale
phases across slot reuse and failure cleanup. `RunSession` owns the effects in
both worlds, advances their animation before Begin Step, latches capacity
failure, and clears them on room/run retirement. Its publication hooks currently
have no external spatial registry; integration into a global registry remains
separate. Both renderers place sprite 102 between bullets and player at -1000.
Host rendering retains fractional coordinates; native raster projection uses
the existing nearest-pixel convention. Integer-position command tests cover
both renderers; fractional GPU raster equivalence remains unverified. Explicit
world emission in tests is not evidence of a natural skeleton death chain.
`RunSession::advanceCorpseAlarm` is the shared production adapter for alarms:
it invokes the source lifecycle with synchronous creation into the room-owned
expiry pool, then resolves the requested sprite's unit-track frame count and
requests parent retirement. Its caller supplies resource metadata and removal,
not a second implementation of the event. Pool exhaustion and unsupported
sprite metadata latch session failure. A child created before setter failure
remains owned until cleanup; the parent is not retired on that failure. The
natural corpse scheduler must invoke this adapter for each source alarm index.

`CorpseLifecycle` implements alarms 0–3 and the Knife and Fork player-contact
event. Alarm 0 completes object-242 Create before assigning its sprite and
requesting corpse retirement. Alarm 3 activates the corpse; only an active
corpse, enabled upgrade and living player admit consumption. Consumption
increments the shared bodies-eaten counter, plays sound 83 (80,0,1, UI), then
creates five object-223 effects with freshly read player globals (Y before X)
for each birth, before requesting retirement. These are not object-220 moving
blood effects. Callback failure invalidates the component and prevents retries
from duplicating already-completed effects. The scheduler still owns global
alarm-index order, precise contact admission and actual lifetime removal.
`verify-corpse-lifecycle` covers successful ordering, gates and failure paths;
it does not establish live upgrade support.

`CorpseAnimation` admits the initial sprite 172 and skeleton override 206,
both verified one-key unit-rate sequences. It uses the shared runner sprite
reset decision, publishes changed sprites and leaves same-ID assignments
untouched. Signed Create animation rates continue during time-stop: ordinary
corpse Step never sets image speed to zero. No Animation End handler exists.
Other corpse sprite families and nonzero external image-index writes are not
silently approximated. `verify-corpse-animation` checks 20,480 scalar float
updates plus setter/failure cases; the connected creation test uses this actual
setter with a publication test double. Corpse depth 1000 equals smoke depth,
so live Draw must preserve same-depth instance ordering across both families.

`CorpseDraw` and `DebrisDraw` share `EffectSpriteDraw` routing. Corpse commands
retain the fractional subimage, published coordinates and horizontal scale;
the source keeps vertical scale, angle, alpha and blend at defaults on these
paths. Both death/low-spec direct drawing and normal surface admission remain
distinct. Sprite 172 and 206 planar artwork, including mirrored variants, is
converted offline and checked against source-relative pixel anchors. The shared
asset reader exposes checked borrowed corpse views. Routing/asset tests do not
establish live same-depth ordering or the original GPU's fractional sampling.

`CorpseEffects<Capacity>` owns bounded corpse lifetimes. It publishes the
constructor position and default sprite before synchronous Create queries;
failed publication/Create remains available for cleanup, not silent reuse.
The scheduler captures a cutoff before each global phase and dispatches the
appropriate event per eligible lifetime, retaining outer alarm-index ordering.
Retirement callbacks run before slots are freed. Capacity exhaustion consumes
no RNG, room clearing does not rewind handles, and dispatch failures latch the
owner invalid until explicit cleanup. Callbacks cannot mutate the same store;
they may synchronously create other effect families. The owner retains no
borrowed callbacks, so world copies use freshly bound adapters.
`verify-corpse-effects` checks phase exclusion, capacity, pre-Create publication,
reused slots, culling and cleanup. It is not a substitute for the live world's
shared instance registry, cross-family ordering or collision membership.

`CorpseEffects::createFromDeath` connects `scrDeadBody` to actual owned creation
and field assignment. It calls the common sprite setter and publishes changed
facing values through the world adapter; unchanged resource/scale writes do
not dirty collision state. Custom velocities and explosion ID stay with the
owned corpse. The returned caller fields still must be published back to the
dying enemy. Any failed stage, including exhausted capacity, invalidates this
operation's owner so a partial death sequence cannot be retried. The 3,840-chain
integration gate now uses this entry point, not an independently assembled
test-only implementation of corpse creation and setters.

`CorpseMotion` implements ordinary Step: custom-real gravity and damping,
parent-56 motion probes, bounce/settling and the bounded 1–16 floor search.
Area 3 divides displacement by four without changing stored velocities. It
publishes rounded Y before X. Rest in Pieces reads its level again after the
first object-272 Create, optionally creates four more, then creates three
object-274 effects before requesting retirement. Each birth completes before
the next and failures invalidate the component. The final flash assignment
still executes after successful retirement requests and during time-stop.
Frozen Step refunds only positive alarms 0–2 and leaves alarm 3 untouched.
`verify-corpse-motion` checks 6,272 scalar query tapes, upgrade/freeze gates and
partial failures. The owner still supplies original collision admission,
global scheduling, animation and End Step dispatch; this is not live parity.

Corpse End Step now uses `outOfViewRetirement`, shared with the skeleton event.
It tests published Y against view Y minus 160 / plus 3000 using source real
comparisons, then gates retirement on rmMain. The upper/lower tests retain their
short-circuit order. This path does not run expiration, damage, rewards or
effect creation. The scheduler supplies activation eligibility; custom corpse
`active` is an upgrade flag, not instance deactivation. Both explicit corpse
collision events for parentPBullet and subparentEnemy are empty and pinned as
such. Do not add hit reactions to those event bodies. The opposing object's
collision event can still mutate the corpse through the shared instance system.

`verify-corpse-creation-integration` connects the wrapper, actual corpse Create
and the two following debris Creates on one RNG stream. Across 3,840 chains it
checks full RNG state after relocation and each debris birth, retained corpse
animation speed, post-Create overrides, caller speed mutation, and debris at the
unchanged caller position. Failed terrain admission must prevent all wrapper
setters. Its terrain and sprite-setter adapters are controlled test doubles;
this does not replace live spatial membership or render/animation verification.

`DebrisBirth` implements that object-370 Create with four ordered RNG draws,
custom binary64 motion/rate fields and narrowed builtin animation fields.
`make verify-debris-birth` pins the input/export/resource/assembly identities
and compares 7,168 area/seed cases to scalar arithmetic and the complete RNG
state. Position/lifetime ownership, Step, alarms, collisions and drawing are
not implemented by this birth component; it is not live death integration.

`DebrisMotion` supplies object 370's ordinary Step and alarms 0–2. Unlike
`fxTimeShard`, a failed `TimeStopBound` decision skips the entire motion and
flash-update body. `alarmStop(2)` restores only positive remaining alarm values
after the alarm phase; it cannot reverse an already-fired alarm. Alarm 0 requests
destruction, alarm 1 enables disappearance and alarm 2 clears `noTouch`.

The allowed Step restores `image_speed` from custom `imgSp`, adds 0.1 gravity
when ysp is strictly below -1 (otherwise 0.08), probes horizontal then vertical
motion against object 56 and applies the source damping/animation choice. Its
fractional `irandom_range(-0.5,-0.8)` truncates to zero but consumes two words;
see [integer-range conversion](runner-integer-range.md). No 0.9 drag is applied.
Position writes narrow directly to builtin binary32 x/y before the embedded
test. Escape probes prefer right over left, then up over down using the updated
X. Remaining overlap requests destruction without skipping the final flash
assignment. Zero horizontal speed remains zero rather than acquiring a minimum
direction; nonzero magnitudes below 0.05 are clamped to signed 0.05.

The explicit `DebrisMotion::step` publication callback receives each candidate
binary32 X/Y pair before the changed coordinate is committed. It runs for both
ordinary axis writes and each successful escape write, including equal-value
assignments. Later probes see the accepted coordinates. A rejected write leaves
earlier accepted writes and RNG intact, but does not commit that coordinate,
probe again or update the final flash state. Frozen and destroyed effects do
not call the publisher. The no-publisher overload is for arithmetic-only use.

`make verify-debris-motion` compares query coordinates/order, arithmetic and
complete RNG state across 8,192 branch tapes, plus alarm/freeze/failure cases.
Those tapes also compare every intermediate published position and query count,
and reject each write in turn to verify retained prefixes.
The owner must provide source-equivalent time-stop and precise collision
decisions, publish coordinate mutations to collision ownership, schedule
animation separately and retire requested lifetimes at the proper boundary.
The player collision event is empty (pinned by the source verifier), unlike
the shard's kick response. Draw, pool ownership and live death integration are
outside this Step component's verification scope.

`DebrisAnimation` uses the same `EffectSequenceAnimation` mapping as shards,
with four keys for rock/wood sprites 115/116 and six for sprite 91. Create can
write an index through 7; the runner's float division, fractional-cycle mapping,
near-key snap and sequence normalization must run before adding animation speed.
Do not replace this with an integer frame modulo or merge the two normalization
steps. The shared mapping deliberately admits only these four/six-key tracks
and nonnegative written indices through 7. No Animation End handler exists for
object 370, so wrapping never requests destruction. The global animation phase
uses the previous stored speed before Begin Step/alarms/ordinary Step.

`make verify-debris-animation` checks 200,000 inverse mappings, boundary cases
and 76,800 updates with pause, reverse playback and reuse. The unchanged shard
animation gate separately checks the six-key consumer. These are scalar/source
and portable-code checks, not proof of live event scheduling or rendered parity.

`DebrisEffects<Capacity>` owns bounded storage and monotonic, owner-local
lifetime keys. Capturing a cutoff at global phase entry excludes later births
from that phase, including reused low slots. Clearing a room does not rewind
keys. Animation, individual alarm indices and Step are separate entry points;
the global scheduler must retain its outer alarm-index traversal across types.
Per-effect time-stop and precise collision callbacks return success separately
from their boolean result. Neither a failed query nor capacity exhaustion is
silently replaced by a miss or dropped effect.

Birth and retirement callbacks allow external collision membership to be
published before use/reuse. Adapters are passed per operation, not stored as
borrowed pointers that would become invalid after a world copy. Callback/query
failure latches an invalid owner; subsequent simulation is rejected. Explicit
teardown can release remaining handles without repeating already-successful
retirements. Full capacity rejects a birth before its four RNG draws.
All lifecycle entry points borrow the supplied adapter, including bulk-method
delegation and teardown. Stateful registries must not be copied: retirement
must update the same membership that accepted birth. Regression tests use a
non-copyable adapter and check teardown publication after terminal failures.

`stepPublished` forwards each position candidate with its owner-local handle
and the pre-write effect state. A publication failure latches the entire owner
before later effects can run; the old `step` entry point supplies a no-op
publisher. Registry adapters must use `stepPublished`, not synchronize only
after Step, because collision probes observe intermediate writes.

For room-registry integration, use `createRegistered` and the per-handle
`advanceRegisteredAnimationOne`, `advanceRegisteredAlarmOne` and
`stepRegisteredOne` entry points. They let the room dispatcher choose global
cross-family order rather than iterating this pool's local order. Animation
publishes the resulting image value; alarm/Step destruction publishes a mark
once, without calling retirement. `retained` continues resolving the same
record after that mark, although ordinary `get` and Draw visitation omit it.
Marked instances ignore later event dispatch and continue occupying capacity.
`retireMarked` is the explicit room-cleanup operation that frees the slot.
Do not mix these entry points with the isolated-owner `create`/bulk-phase
methods, which retain their immediate local-cleanup behavior.

The registered-lifecycle test covers alarm and trapped-motion destruction,
retained addresses, no duplicate marking, capacity before cleanup, monotonic
reuse, old-phase exclusion, and terminal image/mark/retirement failures. These
APIs do not themselves install native or stored-world collision records; that
mapping and the world's phase traversal remain live-integration requirements.

`GameplayDebrisInstances` supplies the borrowed-record adapter for that mapping.
It is non-copyable, retains marked records until cleanup, publishes each
coordinate candidate through `RecordLifecycleBinding::publishedPosition`, and
publishes animation and destruction through the corresponding binding hooks.
Birth's binding must publish the completed Create state (position, sprite and
image index); this adapter does not claim pre-Create registration. Selected
animation/cleanup dispatch verifies both registry-record identity and the
effect owner's retained address, not just coincident local IDs. Teardown can
release records after failure, but a failed registry cannot admit new work.
The adapter tests cover fractional movement and failure cleanup; neither live
world's collision-key mapping or global scheduler installs it yet.

`make verify-debris-effects` checks phase exclusion, per-instance freezing,
newest-first local visitation, alarm destruction, callback order, failed
publication/query handling and repeated slot reuse. This local owner is not
yet a unified game-instance registry. Global cross-family event/Draw ordering,
collision-registry wiring, scene residency and live death admission remain the
responsibility of the integrating world scheduler.

`DebrisDraw` implements the pinned Draw event's `!dflash` gate and `drawFx`
route without rounding published coordinates or the fractional image index.
Normal drawing uses strict origin-in-view admission and camera-relative
surface coordinates; death/low-spec drawing uses world coordinates without
surface admission. The compatibility depth table assigns object 370 depth
-1000, after smoke (1000) and before the player (-50000). Invoke the adapter at
that instance's global Draw position, not as a late foreground overlay.
`make verify-debris-draw` tests routing, flash/retirement, boundary admission,
fractional preservation and failure publication. It does not establish original
GPU sampling or live cross-family ordering. All fourteen debris artwork frames
are available offline through checked `EffectMaskAssets::debris` views.

`SkeletonEffectBursts.h` now implements the skeleton's smoke/flash count and
birth-coordinate arithmetic, calling the child creator synchronously per birth.
The scope is exactly the skeleton's (5,1) smoke and (6,2,10) flash parameters;
it is not a general particle engine. Across 1,024 seeds, two burst types and
three capacity limits, 6,144 sequences compare 15,454 child births and the
complete RNG state against host arithmetic. Child Create is a draw-order probe
in this test, not an implemented smoke/flash actor. Sanitizers and 68000
compilation pass without floating-point helpers. The regression is part of
`make test` and available as `make verify-skeleton-effect-bursts`.

`verify-runner-effect-ranges.py` pins the original `irandom_range` wrapper at
0x53c2e0 and its 63-bit/two-word modulo helper at 0x5410c0, plus `random_range`
at 0x53d840. Integer bounds use inclusive width plus the lower bound. Real
ranges draw random(1), multiply by width and add the lower bound; the flash
bounds +/-10 keep that arithmetic exactly representable before position
addition. This is not a general equivalence claim about x87 expressions.

Flash-ball child state is now owned by `FlashBallEffects`: builtin-coordinate
binary32 narrowing, X then Y integer jitter, frame-one start, speed 0.3,
three verified unit sequence keys and Animation End retirement. The fixed pool
keeps stable handles and phase cutoffs. 1,024 seeded full bursts (6,178 births)
match scalar positions/RNG/animation, retire after seven animation updates and
reuse slots without reusing handles. Normal/sanitizer and 68000 compile checks
pass. Exported sprite-96 sequence metadata is now explicitly verified.

`SmokeBirth` now implements object 225's synchronous Create: builtin float
coordinate narrowing before xx/yy, nocol=true, speed=random_range(2,4),
image_speed=float(speed*random_range(0.05,0.13)), direction=random_range(0,359),
and allSet=false. Its 1,024 seeded skeleton bursts compare 5,180 child births
and full RNG state to host arithmetic. `MultiplyBinary64` supplies the normal/
zero nearest-even product using 16-bit limbs and 32-bit intermediates: 100,000
normal products, signs/zero, halfway cases and unsupported-range rejection are
tested. Sanitizers and a 68000 compile pass; the compiled birth path's only
undefined symbol is GameMakerRandom::nextU32, with no FP helpers.

Crucially, fxSmoke Step recomputes the animation rate with a **fourth** random
draw, after calculating xsp/ysp on its first invocation. The birth state does
not mark allSet or consume that draw early. `SmokeMotion` now sequences that
initialization, integrates xx/yy, rounds and publishes builtin coordinates,
then decays velocity by 0.9. Its wall event multiplies smokesp by 0.2 only when
nocol is false; it does not rewrite initialized velocity or animation speed.
Alarm 0 clears nocol and the water event is empty. There is no time-stop guard.
An 81,920-Step/1,024-seed scalar regression covers first-Step draw timing,
position/velocity bits, 512 distance escapes and 512 early-alarm cases, wall
events and complete RNG state. Sanitizers and 68000 compilation pass with no
FP helpers in this component.

Direction remains an explicit interface; the motion test binds the integer
`SmokeDirection`/`SmokeTrig` implementation and compares a scalar model.
Distance uses the native predicate below. World collision dispatch and drawFx
remain separate from this component; it is not a live smoke owner. Source pins:
Create SHA-256 `725336f892c1c024ccb1285545b93f683be5cca16815f0b626c82eaa496a221b`,
Step SHA-256 `4398d86951dc87ba221561327498a924f5b97e14ba65f13b30c71377206e8263`.
Those hashes identify the reviewed UTMT assembly, not a Windows execution test.

### Smoke collision dispatch contract

The smoke owner's `collideWalls` accepts a contact enumerator, not a solid-point
predicate. It uses sprite 84's serialized axis-aligned mask: relative half-open
bounds `[-2,2) x [-1,2)`, placed at published builtin x/y, not private xx/yy.
The enumerator must emit once for each overlapping `parentWall` instance and
must not mutate the smoke owner. Capture the eligibility cutoff at Collision
entry; reusing the ordinary Step cutoff would omit intervening births. Each
contact applies the source smokesp multiplication separately. The nocol gate and
pending destruction suppress queries; query/arithmetic failure propagates.
Water's empty event must not be treated as wall contact. The metadata, target
families and event instructions are checked by `verify-enemy-contact.py`.
Live terrain enumerators are tracked in the single work queue.

Both gameplay adapters dispatch terrain and shop-wall contacts through
`RunSession` after smoke Step. `ShopWorld::visitWallContacts` enumerates the table, three cases and
the extant invisible wall individually, including sold cases. All these child
types descend from parentWall; the counter itself is not a substitute. One-way
platform object 93 is not a parentWall descendant and must not be admitted by
reusing a player-landing query.

The native occupancy grid remains a union for player queries. Its companion
`TerrainWallContacts` retains actual Create records for cavern walls, boxes and
gem chunks (60/92/293), including duplicates, in 16-pixel row buckets. It uses
mask 585 for walls/boxes and sprite 569 for chunks. Queries consult current
box retirement state and emit each overlap separately. The SDL adapter queries
its active instance records with the same mask definitions. Neither adapter
uses the visible artwork bounds as collision geometry.

The native companion costs 32 KiB for 4096 records plus 1 KiB for row heads in
ordinary RAM; it allocates no Chip RAM or scrolling buffers. Capacity failure
propagates through generation. Room/reset clears all heads. Indexes reconstructed
from pattern occupancy are explicitly marked incomplete for contact queries:
only recorded Create events establish instance multiplicity. Both paths currently
cover unit-scale cavern terrain; additional wall families/transforms require
their source masks and lifecycle rules, not guesses from solid cells.

### Native smoke distance gate

`point_distance` at 0x53d5b0 rounds coordinate differences, squares and their sum
to binary32, computes double sqrt, then rounds that result to binary32 before
returning a promoted VM real. For smoke's builtin-coordinate inputs and fixed
`> 16` comparison (epsilon 1e-5), the first accepted root is 16+6*2^-19.
Its lower rounding midpoint squared lies strictly between squared-distance
floats 0x4380000b and 0x4380000c. `SmokeDistance` therefore preserves the float
arithmetic and tests the sum against 0x4380000c without calculating a square root.
It validates that inputs are exactly promoted builtin floats and explicitly
rejects unsupported ranges; it is not a general point_distance implementation.
102,560 coordinate cases and 512 root-boundary floats match the scalar wrapper;
the integrated 81,920-Step test, sanitizer runs and 68000 compilation pass.
The native compile fixture binds both geometry operations directly; it requires
only RNG, memory and integer arithmetic helpers, not FP or trig library calls.

`verify-runner-smoke-geometry.py` pins the distance wrapper and the direction
wrappers at 0x53c8e0/0x53c9b0. Direction narrows both arguments, multiplies the
angle by **float pi** then divides by float 180, promotes for trig, narrows
the result, multiplies by float length and applies a near-integer adjustment
with float 1e-4. The source trig path is described below. Plain host double
sin/cos is a separate scalar comparison, not evidence of matching the original
runtime's FP environment. This is not an executed Windows smoke capture.

`SmokeDirection` now transcribes the surrounding lengthdir arithmetic for
smoke's nonnegative speed <=4 and angle <=359. Integer significand division by
180 preserves nearest-even binary32 angle conversion; product narrowing and
Y negation follow the wrapper. The reviewed modf fast path (0x759d50..0x759de9)
stores the truncated integer and returns the signed fractional part. The wrapper
snaps only when abs(float(fraction)) < float(1e-4), retaining signed zero;
values just below an integer are not rounded upward. 100,028 scalar cases and
snap-boundary cases pass, including sanitizers and 68000 compilation. Only
integer division/modulo helpers and explicit callbacks/RNG are external.
`SmokeTrig` evaluates the straight-line SSE polynomial paths from the original
0x74b5f0/0x74b7b0 routines with integer-only binary64 operations. The exporter
decodes only the reviewed instruction subset and emits both operations and the
64-row constant table into ignored `generated/smoke-trig.inc`. Its manifest
records executable, exporter, path, table and output hashes plus decoder version;
`--check` verifies reproducibility. The table has static storage, not task-stack
storage. Evaluation preserves each scalar/packed lane operation and rounding
boundary; it does not replace the source polynomial with host libm.

Single-axis consumers can call `smokeCosine`, which evaluates only the recovered
cosine path. Damage-number `lengthdir_x` uses it; two-axis smoke/shard motion
retains `smokeTrig`. Both interfaces have the same admitted input domain and
atomic failure behavior. The numeric test compares the single-axis result's
binary64 bits with the pair result as well as the scalar source-program oracle,
including signed zero and reduction-boundary neighbors. No lookup approximation,
precomputed direction quantization or changed rounding is introduced.

The supported contract includes smoke and shard radians in [-11,11] under
nearest-even FP mode, with signed-zero results. The runner's entry guards at
0x74b621 and 0x74b7e1 strip the sign before range testing; the polynomial itself
receives the signed argument. Direction wrappers admit speed [0,6] and angles
[-585,585], without normalizing angles before the source float operations.
Other tiny/general inputs and the runner's alternate
FP-environment fallbacks are not implemented. Establishing the original runtime
FP mode and native execution cost remains necessary before live admission.
The scalar test evaluates the same exported operations with host doubles and
separately compares float-rounded output with libm; it is not a Windows runtime
capture. See the git log for verification results.

`SmokeEffect` now composes motion with the verified sprite-84 nine-key unit
sequence. The runner constructor initializes image_index at offset 0x94 to
zero at 0x46190e; object 225 has no parent and its Create does not change that
field. Animation advances before Step: its first update uses Create's sampled
rate, subsequent updates use first Step's replacement. Other 7 requests
retirement on the first wrap. Across 1,024 seeds, 38,059 animation updates match
a scalar float model, with lifetimes 18..89 updates; re-creation clears the old
retirement flag. Normal/sanitizer tests and 68000 compilation pass without FP
helpers. Exported metadata now includes sprite 84's full sequence and the
verifier checks all nine keys. The lifetime test supplies stationary geometry
callbacks intentionally; it does not prove native direction or Draw fidelity.

The flash pool now resides in shared `RunSession` ownership. Both runtime adapters
advance it in their pre-Step animation work; room/run retirement clears it
without a whole-pool stack temporary. Both render paths consume current frames
and camera-relative anchors, with newest-first order within the effect family.
Only active entries are visited. Bursts are still fixture-triggered rather than
connected to natural death, because the earlier smoke/blood/corpse/debris calls
must preserve RNG order too. The existing renderer's class-grouped painter
order and nearest-even integer effect anchors are retained; full cross-family
creation-order and Windows fractional-raster fidelity remain unproven.

Offline-art
preparation uncovered noncanonical colors: every opaque pixel in sprite-96
frames 0/2 is RGB 254 and in frame 1 is RGB 7 (436 opaque pixels per frame).
The exact four-pen exporter initially rejected those colors. Follow-up export
of shader 0 (`shaderTemplate`, selected by the default DOWNWELL palette) now
resolves their color classes. The embedded GLSL thresholds red/blue at 0.5,
classifies light using green >= 0.8, and dark using the remaining low-channel
sum. Thus opaque gray 254 maps to light, and gray 7 maps to dark. This is not
a general grayscale threshold: intermediate gray can map to the special blue
class. The generator only accepts the two reviewed opaque colors for sprite 96;
its general four-pen guard remains unchanged. Transparent pixels stay
transparent; dark flash pixels use opaque black pen 3, not transparent pen 0.

`export-default-shader.sh` records tool version/commit and source hash in ignored
generated JSON; `verify-default-shader.py` pins the reviewed GLSL and the VM's
default palette selection. This is embedded shader evidence, not execution of
the Windows D3D shader. The default shader's final light/dark RGB values are
250/256 and 5/256; our existing white/black Amiga pens preserve the classes,
not exact final Windows RGB. Full alternate palettes and SDL postprocessing
remain separate work.

All three flash frames now have offline planar, mirrored and DMA records.
The independent artwork test evaluates the source shader classes and checks
90 skeleton/bone/flash facings, including rejection of unreviewed colors and
semi-transparent inputs. Pack reproducibility and existing garden/exit-mask
checks pass. The game pack grows from 320,958 to 323,118 bytes (+2,160), SHA-256
`d029f6b121ad7761f496887dba2491d0ac821f26055f4e4af63523c853355de9`.
No Amiga runtime asset conversion is added. SDL loads three isolated canonical
textures once instead of altering shared atlas pixels. A real SDL framebuffer
test compares all pixels against independent source color classification over
red, proving that the dark frame occludes rather than becoming transparent.
Production renderer-command tests cover camera movement and retirement on both
adapters. This is fixture-based integration, not naturally triggered effects or
a rendered native-emulator comparison; full death presentation remains open.

The birth contract does not establish corpse relocation/physics or debris
motion/collision adapters, nor their composition with the other death effects.
No request may silently succeed when required child storage is exhausted.

## Integration boundary

The current cavern contact/bullet helpers retire enemies and emit rewards
immediately. Do not route skeleton lethal damage through those shortcuts.
Skeleton bullet and stomp handlers instead reduce health and leave Step to run
the sequence above. Existing shared player movement can perform the bounce,
but a complete effect owner must also provide recharge effects, damage effects
and Exploding Knees child creation when applicable.

Next owner work should join shared actor/projectile/effect storage and phase
dispatch, then expose the resulting retained state to both renderers. Fixed
capacity, creation order, synchronous Create RNG and event-boundary retirement
are part of that contract. Offline artwork alone does not supply this ownership.
