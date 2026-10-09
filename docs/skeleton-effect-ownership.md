# Skeleton effect ownership before live admission

Source: untouched Windows `data.win`, SHA-256
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`,
UTMT 0.9.2.0 export. Resource metadata identifies objects; the corresponding
`Assembly/gml_*.asm` scripts establish argument evaluation and callback order.
This is an integration audit, not a claim that these effects are implemented.

## Registered bone event contract

`SkeletonInstances` supplies fixed, generation-checked object-120 ownership.
The stored bridge allocates from the room's numeric ID stream; native creation
requires an explicit source ID. Both register constructor geometry before the
inherited 16-pixel snap, then retain the completed shared Create state without
consuming RNG. Local handles and source IDs remain distinct. Destruction marks
retain the record until cleanup; Retry/room teardown preserve native generations.
The registered room animation pass uses `SkeletonAnimation`, not the older
generic enemy frame approximation. Native alarms join the captured global alarm
traversal. Stored alarms import the room's already-decremented values before
dispatch, so subsequent synchronization cannot rewind their countdowns; Alarm 2's
Alarm 3 write is visible to the later slot in the same phase.

`withSkeletonPublishedEvents` supplies stored ordinary/End Step handlers with
the registered publisher; `withSkeletonEvents` retains its state-only adapter.
The publisher installs sprites 205/207/208 through the resource owner's sprite
setter and applies the unit-sequence index/timeline reset. It publishes facing
through image_xscale and rounded positions without rewriting private xx/yy.
Stored builtin fields and maintained collision geometry are visible immediately
to subsequent callbacks, not only after the event. The fixed mask remains 208.
After dispatch, the shared store rejects mismatches between actor state and its
published sprite, facing or coordinates instead of silently accepting bypassed
setters. Fractional external position writes are outside this rounded-event
adapter's admitted domain and fail before publication. Failure retains previous
successful writes and does not advance local sprite state after a rejected
callback or permit a retry; partially completed backend work is not rolled back.
Collision/death dependencies remain admission requirements:
these entry points do not enable natural
generation or silently substitute a no-op event. Engine activation is separate
from custom skeleton `active`.

The registered skeleton owner also implements End Step's sole call,
`scrOutofview`, from published Y and the previous camera view. The strict bounds
are view Y minus 160 and plus 3000, gated by room 11. It marks destruction and
leaves cleanup to the world; it neither sets health to zero nor runs rewards or
death effects. The stored skeleton scope owns this phase internally rather than
delegating it to the external ordinary-Step callback. The native registered-actor
End Step pass includes it beside drone retirement and ignores bones, which have
no End Step event. Empty stores avoid that pass. Both adapters preserve the
published Q12 view when converting to the source numeric representation; the
stored adapter currently admits only the main room. This does not establish full
cross-family phase ordering or dirty-camera behavior outside the admitted path.

`SkeletonActor::stepPublished` runs the checked ordinary Step through that
publisher. Image-speed and alarm writes also reach stored builtin fields before
sound or child-creation callbacks. It preserves activation, hit-stun writes,
throw-frame tests, rounded X then Y publication, time-stop's alarm compensation,
and the death script's ordered callbacks. Death remains outside the time-stop
guard; the final hit-stun sprite write can run after destruction is requested.
The effects adapter must complete synchronous child Creates or fail, retaining
the completed prefix and latching failure rather than retrying it. The scalar
`stepAtPlayer` remains a successful-event reference, not a backend authority.

Skeleton Step's sounds 133 (hurt), 134 (throw) and 132 (death) all invoke
`soundPlayOL(sound,50,false,true,"enemymove")`. Both worlds expose this through
the shared run's channel 4 and retain the script's caller variables in the
skeleton actor. The world binding supplies the published actor Y, not private
`yy`, and the ordinary-Step view snapshot (Q12 stored, Q8 native), preserving
its fractional coordinate. It does not reread the followed camera's current
position. The stored binding requires rmMain and its supported 284-pixel view;
the native gameplay world likewise uses rmMain's 284-pixel height. The caller
supplies volume and water state, not view coordinates.
The shared sound script stops an already-playing copy of that sound even when
offscreen, suppresses the subsequent start outside the inclusive vertical view,
and applies volume and underwater pitch to an admitted voice. Failed backend
operations latch simulation failure and retain completed writes. This binding
requires an actual audio backend; it does not provide a successful silent one.

The stored scoped Step handler and native captured-Step adapter use this same
body. Controlled owner fixtures advance the source animation until it throws a
registered bone, checking parent publication and the five shared RNG draws of
that throw. These injected, fixed-player-pose encounters establish the adapter
boundary, not natural generation, complete native cross-family scheduling or
original-runner parity. Production damage/death dependencies are not replaced
with the tests' deliberately rejecting effect adapters.

The inherited `subparentEnemy` Draw event is byte-identical to the bone Draw:
`drawShade(); draw_self()`. Neither the skeleton's custom inactive state nor
hit-stun suppresses it. Registered skeletons occupy native projected layer
elements, including the prepared Draw cache; engine activation and destruction
control their eligibility. SDL reads the stored published sprite/index,
coordinates and transform. Amiga draws the existing offline 205/207/208 frames
with the published facing and integer room position (fractional external
positions remain outside that backend's admitted domain). Both submit the
published shade anchor to controller surface composition. Native hardware-only
currency/entry-cover paths must decline promotion while a registered enemy's
shade intersects the view, just as for bones. This draw integration does not
admit natural births or replace the missing collision and death dependencies.

`BoneProjectile` has checked publication entry points for live owners, while
its scalar event methods remain the successful-event reference. Step compares
the promoted builtin image speed to custom `imgSp` using the VM real tolerance;
it does not unconditionally rewrite the speed. It writes `ysp`, `xx`, `yy`,
then separately publishes rounded X and Y. Failure of the Y publication retains
the new X and both custom accumulators. Health depletion writes speed zero,
mask 582, both velocities zero and custom `active=0`, then requests destruction
and creates effect 85. The subsequent view check still runs after successful
health destruction and can request destruction again. Custom `active` here is
not the engine's instance activation flag.

Wall collision requests destruction before its effect. Player collision checks
custom `active`, conditionally performs damage outside time-stop, then creates
the effect before requesting destruction. Every publication/query/effect can
fail. The world-owned success latch preserves completed operations and prevents
any later operation or retry. Animation publishes its evaluated image index
before committing the new timeline. The bone has no Animation End event.
`verify-bone-publication` pins these VM event bodies and checks successful state
against the retained scalar reference, plus each failed callback prefix. These
entry points do not by themselves supply registered lifetimes, live selectors,
player damage, effects, scheduling or backend rendering.

`dispatchBoneCollision` on each world adapter accepts a resolved collision-event
context, verifies both lifetimes and the resource-selected inherited binding,
then uses `BoneProjectile::collideInWorld`. Only bone wall/player source handlers
2291/2292 are admitted. Native retained scenery resolves the opposite wall from
its generation-checked terrain run, not a fabricated generic instance-table
entry. The existing registered impact owner creates sprite 85
at the caller's published position and assigns its returned identity. A wall
impact follows the destruction mark; player damage and impact precede that mark.
Player damage is a checked synchronous callback, suppressed by time-stop or the
bone's custom inactive flag as prescribed by the source. Failure stops subsequent
operations and latches the owning simulation. Event execution does not consume
impact RNG or retire the caller prematurely. These adapters do not choose
collision candidates or silently implement the missing player-damage script;
the production paired dispatcher and damage owner must supply those operations.

`dispatchSkeletonCollision` similarly validates the source player handler
(120/1266) or inherited parent-time-field handler (83/1066), including the
actual opposite object's subtype. Player handling preserves two independent
tests: a synchronous stomp is followed by fresh grounded/global-player-Y/self-Y
reads before deciding whether damage also runs. Failed stomp or damage latches
the world and stops the event. There is no invented time-stop guard. The field
handler only sets health to zero when `timefieldImmune` is false; it does not
run death effects or retire the skeleton. Death remains ordinary Step work.
Stored dispatch publishes the resulting health through its retained instance
record. Stomp and damage script owners are explicit dependencies, not no-op
fallbacks. Controlled player callbacks and real inherited exit-field bindings
verify these boundaries, not natural pair selection or player damage fidelity.

`BoneProjectileInstances` supplies generation-checked fixed ownership separately
from the scalar store. Its caller supplies numeric source IDs. Registered birth
publishes the serialized object-269 geometry and layer membership before the
three synchronous Create RNG draws. Engine activation is separate from the
bone's custom `active` field. A destruction request keeps the lifetime readable
through the event; cleanup cannot reuse an executing record. Explicit teardown
does not reset the generation stream.

The stored-world bridge uses that world's real numeric allocator and RNG.
`stepWithBones` joins the existing room animation/Step/pre-Draw cleanup phases,
publishing through the resource-backed collision and layer owners rather than
scanning a private bone list. Both copies retire together, and a rejected second
position write retains the successful first write. Activation requests use the
room's pending-membership boundary: deactivation receives the old room list's
final animation but no Step; reactivation rejoins Step before the next animation
pass. Its scoped event adapter
still supplies time-stop, collision/effect/damage/view behavior; this is not yet
automatic skeleton throwing or native admission. The bridge must be cleared
before room teardown. Capacity failure is a terminal port error before another
ID or Create RNG word is consumed, not a claim about Windows allocation failure.

`RegisteredSkeletonThrow` connects the throw block to either registered bone
owner through `throwRegisteredBone` / `throwRegisteredBoneWithSourceId`.
Those explicit-argument entry points remain references. The live
`throwSkeletonBone` / `throwSkeletonBoneWithSourceId` bindings use the shared
sound script and read captured player X and the actor's current facing after
Create, sound and the vertical choice. Aligned X skips the horizontal choice
and uses positive 1, regardless of facing. The admitted Q8 player coordinates
and integer actor coordinates make this zero-separation test exact; this does
not establish full binary64 player-motion equivalence. Failed aiming retains
the created child and vertical choice without publishing arc setters or shot.
It retains the caller's child handle, distinct numeric ID and arc variables, finishes child Create
before sound 134, consumes the vertical choice before the conditional horizontal
choice, and writes ysp, xsp and sampled spin without resetting child coordinates
or changing builtin image speed. Failed sound retains the registered child and
three Create RNG draws but does not choose arcs or latch the shot. Native callers
still supply a source numeric ID; the local handle is not a replacement for it.
This entry point does not itself register skeletons or schedule their throws.

`withBoneEvents` scopes the stored bone dispatch around one caller-supplied tick;
`stepWithTimeStopAndBones` composes it with the registered time-effect scheduler.
Bone Step reads `TimeStopEffects::player.stopped` after that tick's player Begin
Step transition, not a sampled prior-frame boolean. Animation still precedes
the transition, so entry can advance one final frame before image speed becomes
zero; an exit resumes motion immediately but animation uses the earlier zero
speed. Paused health destruction is deferred until the first unstopped Step.
The impact/view adapters are the real world owners, and cleanup follows the
collision boundary. Nested scopes fail without running another tick or clearing
the outer failure latch. The source pose/audio/transition callback is still
supplied explicitly; this does not enable natural player transitions or fill
the missing native numeric-ID/scheduler dependencies.

`BoneProjectile::stepInWorld` is the common publication/effect/view adapter for
both worlds. Native `GameplayCollisionOwner::stepBone` resolves the shared
registered lifetime selected by `dispatchStepPhase`, then calls
`GameplayWorld::stepBoneRoomEffect`; it does not introduce a per-family walk.
Health destruction creates the real registered object-219 impact, and leaves
the bone readable until normal pre-Draw cleanup. Effect failure is terminal in
both the world and collision owner. The phase caller must still supply the live
player time-stop state and execute the other selected object families; this
adapter does not replace the native world's existing mixed phase scheduler.

The native `GameplayCollisionOwner` also binds the shared store as its own
object-269 family. Its disjoint slot range does not overlap drone bullets;
room animation and pre-Draw destruction cleanup resolve the same generation.
Reference ownership publishes original absolute coordinates, image indices and
mask writes to the spatial owner. Native ownership retains them in the typed
bone record and publishes lifetime/layer flags without populating a spatial
tree that the native queries cannot yet consume. Retry and level teardown
clear bones before reusing the room and retain the generation stream.
`createBoneWithSourceId` uses the world's shared RNG but requires the caller's
numeric ID: it does not fill the missing native global allocator. Automatic
throwing, ordinary Step/collision dispatch, time-stop/damage adapters
and replay serialization remain required for natural encounters.

Bone view queries resolve the retained lifetime's published binary32 x/y and
apply `scrInView(0,0,0)` with the VM real-comparison tolerance. All four edges
are excluded, including equality; these are origin checks, not mask bounds.
Stored and native adapters use the normal ordinary-event camera snapshot,
not the camera target updated in that frame. Native camera offsets are converted
to absolute room-view coordinates before testing. Time-stopped bones still query the view, and health
destruction does not suppress the final query. Explicit dirty-camera setters
and complete room-entry view publication remain outside these normal-follow
adapters; this is not a replacement for the camera-manager work queue.

Bone impact adapters use the source `scrFxNol` subset: synchronously create
object 219 at the bone's currently published coordinates, assign the caller's
`myFx`, set sprite 85, then set image speed to binary64 0.3 narrowed by the
builtin setter. They do not call the longer script's angle or depth setters.
Object 219 has no Create callback/RNG. Its child is independently owned and
survives bone retirement. The stored caller retains the true numeric child ID
and effect lifetime; the native caller retains only the typed effect lifetime,
not a fabricated source ID. Failure after birth keeps the child/reference and
stops later setters and retries. Both adapters use their existing owned script
effect bank and room animation/cleanup binding.

Object 269's Draw calls `drawShade` then `draw_self`: sprite 460 uses self drawing
at the registered layer position. Its shade joins the existing controller-surface
subtraction, with the same view/low-spec/control guards and even-pixel snapping
as enemy shades. SDL uses the stored published coordinates,
image index and transforms. The native owner includes bones in both retained
and prepared actor traversals with disjoint cache tokens and generation checks;
engine activation/destruction affects drawing, not the custom damage `active`
field. The Amiga collector admits the four unrotated frames at integer published
positions and rejects fractional positions rather than silently rounding them.
The existing offline planar/cropped and DMA frames supply the artwork. Neither
backend adds a separate bone overlay or advances animation while drawing.
Native hardware currency/entry-cover promotion falls back to the ordinary
surface compositor while an on-screen bone shade is present, so promotion
cannot bypass its subtraction.

The native draw collector also accepts the six unrotated sprite-85 frames at the
integer positions written by bone Step. Planar/cropped variants are exported
offline with source/tool provenance, using the existing asset path; no runtime
conversion is added. Rendering admission is not a claim that natural skeleton
collisions now run.

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

`RunSession::recordEnemyDeath` publishes combo and kill counters in this order
and uses overlap channel 1 for checked gunshot audio. It rereads the channel
after the playing query, as the VM does. Both world entry points delegate to
this shared implementation. The owned entry clears `RunSession::noKill` at the
achievement write, after the kill counter and before moment delay. Both adapters
initialize it at player Create, before child creation; new player lifetimes reset
it even when run counters survive a room transition. The explicit publication
callback remains a failure-prefix reference, not the default state owner.
Moment delay and the level cue remain mandatory checked callbacks; they must
observe the completed counter/achievement prefix and failure stops later calls.
Counter overflow beyond the current
unsigned storage is a terminal unsupported state rather than wrapping. The
entry is not an alternative family-wide death sweep.

Both adapters also expose an owned `beginLevelCueWithTitle` entry. It reads the
supplied live time-stop owner's player flag, uses shared `RunSession::firstLand`
and music resource state, and creates the registered title synchronously with
the supplied language/font context. `scrInitialize` sets music to 193 and the
area table to 194–198; the checked lookup rejects areas outside 1–5. Player Create
clears first-land without resetting music. Ordinary retry preserves music, including
the native runtime's in-place reconstruction; a full controller initialization
sets it to 193. The stored startup getter and cue share that same resource state.
Failed title Create leaves first-land clear; successful Create sets it before
audio queries. Explicit state/lookup providers remain the reference path. Native
title allocation still requires the enclosing source-ID stream, and normal
death callers still need the complete effects/cue composition.

`StartupRuntime::recordSkeletonDeath` composes the ordered shared bookkeeping,
its bound controller `momentDelay`, and the owned title/music cue using the
skeleton's published x/y and retained sound-script caller fields. The native
`GameplayCollisionOwner::recordSkeletonDeath` binds its source-ordered controller
walk to the same world composition. Neither accepts a successful replacement for
delay or cue. The caller still supplies the source gunshot-cut condition, live
time-stop owner, language/font context and audio; native allocation still comes
from the enclosing ID stream. These entries implement the `enemyDeath` part of
skeleton Step, not the later blood, smoke, flash, reward, corpse or debris work.

Both runtime adapters route flash bursts and pre-Step flash animation through
`RunSession`'s shared `effectsSucceeded` latch. Failed bursts retain earlier
children and consumed random words; subsequent effect calls and animation must
not retry the partial operation. Room/run reset clears the failure only after
retiring owned effects. This is a port capacity/error contract, not a claim
about Windows allocation failure or completed natural skeleton integration.
The SDL `Game` latches simulation failure separately from ordinary quit and
does not dispatch more updates afterward. The executable returns 20 before
rendering/presenting the failed update, matching the Amiga effect-error path.

Box-specific smoke and flash entry points likewise use each world's existing
shared RNG and this failure latch. Smoke receives the script's already-offset
`y+4`; the owner must not apply the offset again. `BoxBurstWorldCheck` exercises
both live world adapters, comparing children and the full RNG state against the
separately verified burst primitives, including exhausted smoke/flash banks and
suppression of subsequent requests after failure. This is an ownership/wiring
check, not natural destruction admission: persistent debris scheduling and the
live player time-stop dependency of `alarmStop(0)` remain required before the
normal box Step can invoke the complete sequence.

The box-owned `spawnBoxScriptEffect` adapter retains `scrEffectSpawn`'s `myFx`
assignment on the caller immediately after synchronous child creation, before
sprite validation and the remaining setters. The stored world retains the
source numeric instance ID; the native world retains a generation-checked
handle, not a fabricated VM ID. Both re-resolve the caller after creation.
A later setter failure retains the child and caller assignment and terminates
further effect work. Resource-backed tests cover fractional placement, setter
ordering, unchanged RNG and the partial-failure prefix in both worlds. This
entry point is not yet admission of the complete natural box destruction Step.

Both live adapters retain the usual bullet's box damage until objBox_n Step:
`scrBulCheckSolid` blocks 11–13 subtract the birth profile's damage from positive
`wallHp`, set `hit`, and consume the non-consistent bullet. Already exhausted
health does not receive another subtraction or consume that bullet. A head hit
instead writes zero health (`scrWallCol` block 34). Neither query inactivates
the box. The current removal subset runs at object-92 ordinary Step, before
object-261 bullet Step: head hits and expired alarms remove the box that tick,
while bullet damage waits until the next box Step. Destruction is marked before
Draw cleanup retires the lifetime. Native terrain damage is emitted at that
same removal boundary, not when the projectile query first hits.

This corrects the existing removal timing; it is not the complete source event.
The full ordered audio/smoke/flash/impact/chain/debris/reward sequence and live
`alarmStop` still need to replace the removal subset using `BoxDestructionScript`.
Do not treat the lack of new particles or RNG consumption here as source parity.
The host/native lifecycle tape checks expiry, retained destruction and cleanup;
native contact tests additionally check the intervening live damaged-box frame
and failure of destruction publication.

`BoxDestructionScript::runRetained` accepts a caller reader rather than keeping
one borrowed record/coordinate snapshot across synchronous callbacks. It rereads
before subsequent emissions, neighbor queries, post-destruction debris births
and the two independent gem checks. Thus a child that changes the caller's
coordinates affects later children, and changing `haveGem` from 1 to 2 during
the first currency script can admit both source branches. Failed reads retain
the completed prefix and latch failure; the fixed-caller `run` wrapper is for
scalar fixtures, not a live storage binding.

Both collision owners expose generation-checked readers using published
binary32 coordinates, including fractional native positions rather than the
rounded box drawing cache. A destruction mark keeps these fields readable;
actual retirement or reuse invalidates the caller without changing the output.
The native lifecycle tape checks that boundary. These adapters still need to
replace the live removal-only Step together with the full effect/reward owners.

`withBoxDestruction` on both worlds borrows a retained reader and operations
factory for one tick. `BoxStepDispatch` invokes the shared script at object-92
ordinary Step, including healthy boxes' `alarmStop`, rather than running an
extra world tick. The stored reader takes world handles; the native reader
takes local breakable handles and must encode them for the collision owner.
The binding rejects nesting and releases its callbacks even when the tick
fails. Destruction statistics and native terrain damage follow the script's
actual destruction mark, not exhausted health alone. Callbacks must not retire
the caller; pre-Draw cleanup owns that boundary. The unbound gameplay path is
still the removal subset until complete audio/effect/time-stop adapters are
installed. Scheduled failure-prefix checks use real smoke, flash and impact
owners, then reject debris creation; they do not prove complete destruction
or natural encounter parity.

`BoxWorldEffects` supplies the shared script's overlap sound, smoke, flash, impact
and object-370 creation calls to either world. Its explicit audio backend and
game-volume input execute the source `waterThings` script before particle
creation, retaining caller sound fields and the channel-7 assignment. The
sound ID is the already-evaluated script argument, not a later reread of box
state. Failure at any audio call stops subsequent emissions without consuming
their RNG. Impact creation publishes the
caller's `myFx`; debris creation returns the actual registered lifetime, with
stored numeric IDs resolved back to world handles and native generation handles
kept local. The adapter's `alarmStop(0)` invokes the world's retained box alarm
bookkeeping after the caller's checked `timeAllowed` query. Both resource-backed
collision owners expose that query separately from alarm mutation, preserving
the published shape, field-before-side evaluation and post-destruction lifetime.
Healthy-box scheduling tests cover the positive-alarm refund and terminal query
failure before the `i`/alarm writes. The source player's live `pTimeStop` input
remains required; the adapter does not substitute a goal/shop pause flag.
The remaining control operations (globals, chain contacts,
rewards, destruction and the time query) are required from the caller rather than
successful no-op defaults. The scoped box and owned-debris tick adapters can be
nested to keep creation, later debris phases and cleanup in one world frame.
Scheduled no-chain/no-reward fixtures check the complete WELL state at the end
of emission, both debris birth states, box cleanup and next-frame survival. They
provide test audio/time state and are not the ordinary gameplay binding.

`StoredBoxChainPublication` connects the chain branch to the registered stored
world: object-266 Create consumes the global numeric ID stream, while returned
generation handles remain lifetime keys. `myBullet` and `target` retain source
numeric IDs on the box; an empty neighbor query assigns `noone` (-4). Neighbor
selection uses the collision owner's `instancePlace`, and alarm/checked writes
require live caller and target generations. Projectile setters operate on the
registered drone state. Failed Create leaves earlier sound/particles intact but
does not publish a bullet, arm neighbors or mark destruction. A retired caller
cannot mutate surviving neighbors.

`withDroneBulletEvents` allows the same tick to borrow box, drone and debris
dispatch. New chain bullets retain their Create state through their birth Step
cutoff; their later phases use the existing room scheduler. Scheduled fixtures
check four neighbors, missing-neighbor assignment, exhausted projectile storage,
retained caller fields, full emission RNG and retirement. `BoxWorldEffects`
routes the chain shot through plain `soundPlay` on both worlds, preserving its
caller field writes, previous-sound cut, gain and underwater pitch. Unlike the
break sound, it does not modify the overlap channel. Water state and volume are
explicit inputs; no goal/shop flag substitutes for them. The scheduled stored
chain checks all six plain-audio failure prefixes after projectile creation,
before neighbor activation or destruction. Audio backends and global inputs
are fixture-provided. This stored-world binding
does not admit natural chains or supply native global instance identity.

Gunpowder Blocks creates object 266 (`bulletDrone`), not an explosion. Its
Create overrides the serialized sprite 458 with 459, sets speed 7, damage 4,
zero spread and alarm 0 to 16; the box subsequently assigns direction 90 and
damage 20. `DroneBulletSourceStep` shares the source motion/event contract:
first Step consumes `random_range(0,0)`, publishes the initial angle before
`allSet`, then publishes it again in the ordinary body. Alarm 0 enables a
binary64 speed multiplication by 0.8 on each Step. Below speed 3, animation is
enabled once at image speed 0.5. Solid checking precedes separate builtin x/y
writes, including after a destruction request. End Step runs `scrOutofview`
then `scrBulletOov`; animation end requests destruction and Draw requires
`allSet`. Scalar host traces and the native tape cover this contract. Live
object ownership, inherited hit memory, collision-event dispatch, animation
and both renderers remain unconnected; this helper does not enable the upgrade.

`stepWithSolid` composes that update with the existing `BulletSolidScript`.
It supplies current damage/direction, hit sprite 101 and the drone's false
consistent/wave/explode defaults. The adapter uses the same absolute-coordinate
motion for approach and final movement: approach publishes Y then X in tenths
of the velocity, while the final Step publishes X then Y even after destruction.
The composed tape covers solid walls, live/exhausted boxes, empty space and an
enemy-line-suppressed wall with a shootable object. It checks both ordinary
damage 4 and the box's override to 20, along with every callback failure prefix.
The native tape runs those same successful routes. This does not supply the
remaining resource-backed selectors or register live drone instances.

The drone state also exposes the existing-instance alarm and animation passes.
Alarm 0 advances from 16 to 0, enables deceleration, then retires to -1 without
refiring. The six-key sprite 459 uses a zero-speed initial timeline; after Step
enables speed 0.5 it advances in the following frame's animation pass. For a
birth during ordinary Step, the controlled tape decelerates at relative tick
16, enables animation at tick 19 and wraps/requests destruction at tick 31,
before that tick's alarms or Step. The wrapped index is published before the
animation-end callback. Failed publication cannot invoke destruction, and a
failed destruction callback cannot be retried by continuing the same tape.
Owners must still supply active-room, birth-cutoff and destruction eligibility;
the standalone state does not invent a separate family-wide scheduling pass.

`DroneBulletInstances` supplies shared fixed storage and lifecycle publication.
Room-provided source IDs are independent of monotonically increasing local
generations. Constructor membership publishes sprite 458 and image speed 1
before Create sets speed zero and changes the sprite to 459; completed Create
then exposes speed 7, damage 4 and alarm 16. A partial birth is retained when
publication fails. Publishers continue resolving the same marked generation
through the remaining event, including final position writes. Retirement is
deferred and cannot reuse an executing record. Explicit teardown can release
partial births without resetting generations. The ownership tape exercises
inactive/destroyed eligibility, post-destroy movement, reuse, stale handles,
capacity/duplicate-ID rejection and failure cleanup on host and 68000. The
live world adapters still need to register this family with resource-backed
collision/layer ownership and their shared phase ordering; no natural births
are enabled by the store alone.

The final box `alarmStop(0)` adapter also resolves its caller again after the
time-bound query. A callback which retires the caller cannot publish into its
old storage, and a nested ownership failure remains terminal even if the
callback returns success. Both world lifecycle tests exercise these failure
boundaries; they do not assert that the original time-bound query retires boxes.

`SoundPlayScript` models the original plain and overlap-aware audio scripts,
including caller scratch fields and partial-failure ordering. The overlap
channels are footsteps/gunshot/gunwall/merchant/enemymove/music/waterEnemy/
waterThings/UI/boss (0–9); unknown names retain channel 0. Enemy movement and
water-enemy channels suppress offscreen playback but still stop the selected
resource. Channels 7–9 likewise stop that resource, not the previous channel
entry, and force `cutPrv` off. Other cutting channels stop the prior entry.
Gunshot invokes the supplied `bgmDuck` callback before setting channel 1.
Gain is multiplied by game volume; underwater pitch is 0.55 except for overlap
channels 5–8. Muted music is started, configured, then stopped as in the VM.
An offscreen script exits without a voice result; a completed overlap call
returns -1 when the new voice is not playing. Plain sound returns the voice
without the final playing query. `TimeFieldWind` uses this shared UI path.
The channel entry reference must be selected with `channel(name)` by the owner;
the environment supplies published caller/view coordinates and current globals.
The bounded native tape exercises the box's waterThings route and each audio
failure prefix. Actual audio backends, shared channel ownership, duck behavior
and natural box-event admission remain integration requirements; these tests
do not establish audible fidelity.

Both worlds expose `playBoxBreakSound` against a retained object-92 lifetime.
The operation uses its recovered `breakSound`, stores scratch fields on that
caller and shares run-owned `OLchannel[7]`. The supplied volume is the current
source game-volume value; this channel has no view, water-pitch or music-mute
gate. A backend failure latches the existing world effect failure and blocks
retries without undoing preceding writes. Both world adapters have fault-prefix
tests; the bounded native lifecycle tape invokes a real retained box and checks
that Retry zeros its channel. The eleven channel entries are initialized/reset
to zero as in `soundInitialize`; room retirement does not clear them.
This does not implement the rest of soundInitialize's emitters/sample groups.
Sound scratch storage is separate from builder `BreakableState`, so creation
commands do not carry it. It costs 32 bytes per native retained box (16 KiB at
512 slots), plus 44 bytes for shared channel entries. The general stored-world
instance record retains the same fields for future non-box callers; stock A500
memory fit remains unproven. These entry points still await complete natural
destruction admission and actual audio backends.

`CurrencyCreate` implements the object-248 Create event before reward-script
overrides. It retains binary64 velocities and `imgSp` separately from builtin
float position/image-speed fields. Area 5 consumes an extra random draw for
negative vertical speed; areas 3/4 quarter gravity and cap it at 0.5. Attractor
changes radius/speed/alarm 3, not initial attraction. Direction evaluates both
integer-random arguments before `choose`, and retains negative angles.
Embedded-wall relocation first tests parentWall 57, then tile layer 100 only
on a wall miss. It snaps and publishes x/y separately. Candidate y offsets
range from -radius through zero; candidate tile tests remain at the snapped
origin. The source grows radius through 68 despite `rrmax=32`, makes at most
33 attempts and applies even a final blocked candidate. This shares the
existing checked 16-pixel move_snap operation with corpse creation.
Scalar host tests cover all five areas, Attractor, wall/tile blocking, bounded
search exhaustion, full RNG state and query failure prefixes. A controlled
maximum-range tape runs on the 68000. The existing live `GemMotion` initializer
remains approximate: registered currency ownership, reward-script caller
writes and the exact Step/animation path still need integration before this
Create event replaces it. These tests are not proof of complete live rewards.

`StoredCurrencyRegistration` supplies fixed-address object-248 Create storage
and source reward setters for the stored world. It allocates a global numeric
instance ID and publishes resource-backed membership before executing Create.
Wall queries use the actual parent-57 collision owner; layer-100 tile queries
remain an explicit required callback. Separate relocation x/y writes update
the registered shape and builtin position. Successful Create returns the world
generation handle; a failed Create retains its consumed ID, partial state and
membership without assigning a return handle. Capacity exhaustion precedes
Create RNG. Explicit cleanup also retires failed births.
Resource-backed fixtures compare complete WELL state for clear placement,
tile relocation, wall search exhaustion, tile-query failure and capacity
exhaustion, and check amount/size/attraction writes and global ID allocation.
`withCurrencyEvents` borrows this registration for the stored world's actual
animation, alarm, Step and cleanup phases. Animation advances the four-frame
133/134 sequence with binary32 speed and publishes its index to the collision
owner. Alarm dispatch imports the current world bank before each event and
publishes writes immediately; Alarm 0 starts Alarm 1 and Alarm 1 starts Alarm 2
in time for the same ascending-index pass to decrement them. Alarm 3 enables
attraction. The expiry adapter checks attraction, executes plain sound 166 at
priority 30 with explicit gain/water inputs, then marks destruction. An audio
failure retains the lifetime. Alarm 4 movement and ordinary Step still require
explicit event callbacks, not successful defaults. Marked gems remain readable
until pre-Draw cleanup retires both records. Scheduling fixtures shorten the
source durations to check chained countdowns, attraction, animation, marked
retirement and terminal audio failure. The motion callback remains a fixture,
not a complete source implementation. This binding is not yet a
complete scheduled/rendered actor or replacement for the live reward array.

`CurrencyStepEnvelope` implements the ordinary Step's first sound choice,
stuck-wall counter/destruction request, time-stop control and final coordinate
publication around two required motion callbacks. An unattracted frozen gem
sets image speed to zero, refunds only positive alarms 0–3, leaves caller `i`
at 4 and becomes obtainable. Attracted gems bypass the wall/time-stop branch.
The disappearance sign toggle and separate rounded X/Y publications still run
after a stuck-wall destruction request; private `xx`/`yy` remain fractional.
The stored registration supplies actual parent-57 and parent-82 mask queries
and lifecycle publication. Fixtures verify full first-Step sound RNG, repeated
freeze, alarm boundaries, continuation after destruction and failed callbacks.
Neither a successful no-op mover nor the existing fixed-point gem mover is
installed as a substitute for the required free/homing source branches.

`CurrencyHomingSource` supplies the attracted branch: binary64 `gemdir`,
`rotsp`, `gemsp`, `pldir` and `dirdif`, one signed 360-degree difference
adjustment, turn-before-acceleration, epsilon-aware caps, separate lengthdir
calls, then private X/Y accumulation. `stepWithHoming` connects it to registered
currency while requiring explicit player coordinates, math and free-motion
callbacks. Host-double control-flow tests cover turns, caps and failure prefixes;
a scheduled twelve-tick fixture checks published positions while time is stopped.
The lengthdir component's explicit speed-nine entry point retains the original
speed-eight limit for existing callers. Its arithmetic is compared against the
float multiply/truncate/snap wrapper independently of trig evaluation.

`CurrencyHomingMath` requires an explicit atan provider. Tests using
`portableAtan` do not establish original x87 identity. Gem direction is not
normalized by the VM: an extended orbit exceeds the older [-585,585] degree
admission limit. The currency wrapper now admits [-720,720] degrees using the
same rounded multiply/divide and recovered trig program, not modulo reduction
or a host-libm fallback. The runner's exponent dispatch admits the corresponding
float radians. A 2,000-step orbit crosses the old limit without normalizing its
stored heading; source-program comparisons cover the extended wrapper domain.
Original atan/FP-mode evidence and a complete live input-domain argument remain
requirements for natural admission; this wider guard is not arbitrary-angle
support.

`CurrencyFreeMotion` supplies ordinary Step's non-attracted branch, separately
from Alarm 4's different gravity/bounce-gain program. It evaluates gravity
before thin-wall (87) and wall (57) collision scripts, preserves both ordered
friction multiplications at combined contacts, and uses the runner's truncated
[-1,0] integer bounce range. Bounce sound follows the published view-Y test.
Attraction checks precede water drag; acquisition changes the next Step's path,
while the acquisition frame still integrates its current velocity. Water uses
the recovered `position_meeting` wrapper at published x/y (including its
precise-mask semantics), not private xx/yy or a substituted collision-point call.

The stored `stepSource` adapter joins this branch with homing, the checked
binary64 collision solver, registered mask/point queries and source sound.
It retains the collision target and X/Y results; the solver's internal walk
scratch values are not exposed as caller script fields. Explicit view, volume,
water and math inputs remain required. Scalar tests compare arithmetic and full
RNG through thin/wall contacts, drag, attraction, gravity cap/overshoot and every
callback failure prefix. A registered gem fixture falls onto a real box and
checks integer bounce, audio and rounded publication. Collection, Draw, complete
native ownership and natural reward-caller admission are separate requirements.

`CurrencyGainScript` preserves the object-248 player collision's `scrGainGem`
sequence: obtainable gate and caller `gemAmount`, currency write and achievement
thresholds, game/streak/timer writes, original controller reset, Popping Gems or
UI sound, pickup effect, Gem Powered recharge, and destruction last. Projectile
Create precedes the single spread draw and its setters. Player coordinates are
reread after the synchronous muzzle effect. All controller/effect/projectile
operations remain mandatory callbacks; script success is not a substitute for
their live implementations.

`CurrencySpawnScript::runRetained` separates caller reads from individual
`moniAmount`, `atrState` and `myGem` publications. Each live accessor must
resolve the selected generation, including after destruction marking and
synchronous child Create. Child amount/size writes keep their selected child;
the later attraction write rereads the caller's `myGem`. Amount subtraction
also rereads the caller, and the twos loop never returns to the tens loop.
The stored world's `spawnRegisteredCurrency` uses this boundary with the real
room ID allocator, resource parent lookup, collision registration and shared
RNG. Script-facing child values are numeric IDs; setters resolve them back to
the registry's generation handles. Caller fields remain the responsibility of
the required read/write accessors, not a temporary snapshot copied back at
script exit. This entry point does not admit natural rewards or supply the
native world's incomplete cross-family numeric-ID stream.
The native `GameplayWorld::spawnRegisteredCurrency` uses that same retained
script and real currency owner. Its mandatory allocator supplies each source
ID before synchronous Create; allocation is never derived from a pool handle.
Child setters resolve the numeric ID to the current currency generation, so
retirement removes the binding. Caller accessors and Create collision/tile
queries remain required. Allocation or Create failure retains the script's
preceding writes and any partially created child, without publishing a new
`myGem` or executing tail setters. This entry point does not fill the missing
global birth stream.
For stored boxes, `spawnBoxCurrency` binds these accessors to the actual
generation's `boxReward` fields, including after destruction marking. Position
comes from a required maintained-owner read on each access, never from the
record's private `preciseX/Y` motion caches or rounded drawing coordinates.
Missing position ownership fails before child Create; the preceding caller
amount/attraction writes and destruction effects remain published. These cold fields
are separate from `BreakableState`, so ordinary box Step snapshots do not copy
them. `boxCurrencyOperations` supplies the destruction script's currency call
without replacing its required time-stop, chain or other effect operations.
The native world's `spawnBoxCurrency` binds the same caller fields to a
generation-checked cold sidecar, retained after the destruction mark and
invalidated on retirement. Slot reuse clears it at the room boundary, not in
the generation loop. This costs 7,168 bytes with the 68000 ABI (512 callers),
without widening `Breakable` or its ordinary Step snapshots. The native
collision owner's destruction reader obtains published room coordinates
from the retained static box record, rather than its absent generic geometry.
That optimized representation accepts only unchanged position publications;
fractional/moving setters fail explicitly until full native geometry supports
them. The reference owner retains fractional setter coverage. Natural native
rewards still require the complete numeric-ID allocation adapter and event
composition.
`GameplayWorld::boxCurrencyOperations` supplies the reward operation to
`BoxWorldEffects` without replacing its sound, smoke, flash, impact, debris,
chain or time-stop dependencies. The scheduled native-scenery/reference fixture
runs the full non-chain destruction event with this adapter: reward Create is
after both debris births and before alarmStop. It compares the combined RNG
state, keeps the marked box until Draw-entry cleanup, and excludes the newborn
currency from the already captured Step phase. Failed reward Create leaves
the earlier effects and partial child intact and skips alarmStop. This is
explicit event composition; normal gameplay still needs the complete birth
stream and live control/time-stop services.
The stored scheduler composes currency and box dispatch scopes around the same
owned-debris tick. A reward born in ordinary Step misses that phase's cutoff
and the preceding animation/alarm passes, but may Draw after the marked box
is retired. Its first following tick advances animation and countdowns before
currency Step; a frozen Step still consumes the one-time sound selection and
refunds the positive source clocks. Draw's medium-sprite/mask publication does
not grant early Step eligibility. This phase contract is independent of the
unfinished live player time-stop publisher and currency surface compositor.
`CurrencyGainAccountOperations` exposes the separate writes on either world's
real account, rejecting values outside its nonnegative uint32 storage domain.
The stored registry's collection entry retains caller state and source RNG.
A resource-backed gem/player event fixture deliberately fails at UI sound and
checks that preceding account updates survive without destroying the caller.
Controller resets and audio/effects in that fixture are instrumented, not full
backend implementations. Matrix tests cover both achievement thresholds,
upgrade branches, unobtainable suppression and ordered failure prefixes.

The controller-local streak rise is separate from account totals and timers.
`scrDrawHudInitStuff` initializes it to zero; `scrGainGem` and
`nonGemGainGem` reset it. HUD Draw first tests positive streak and absence of
death, then evaluates `ascend += (16 - ascend) / 8`. The world-space anchor is
rounded only after adding 24 and 4 to view Y (landscape), or 50 and 4
(portrait), and subtracting the new ascent. Room, pause and flash visibility
checks follow this update; a Step timer or a visibility-gated update differs.
Landscape text is six pixels below that anchor, and its gem icon five below.
`GemStreakRise` preserves those binary64 operations and the retained rounding
contract. Runtime FP-mode assumptions remain subject to the runner audit.
The admitted singleton controller belongs to each world's run state. Selected
collection updates it after the required controller callback succeeds; this
does not replace ordered iteration over a future complete controller registry.
The Amiga's prepared digit words are translated at publication, with an old/new
footprint union for shop restoration. Verification repaints reuse the chosen
position instead of advancing controller Draw again.

The stored world's `spawnCurrencyPickupEffect` binds `scrFxNol(105, 0.8)` to
the existing owned object-219 registry. It reads the currency caller's published
coordinates, stores the child's global numeric ID in `myFx` before sprite
validation, and leaves constructor angle/depth untouched. Child animation and
Animation End retirement use the existing world scheduler independently of
the currency caller's lifetime. A failed sprite lookup retains the birth and
caller assignment, stops before the speed setter and latches terminal failure.
This entry point still needs the complete live collection operation binding;
it does not enable natural rewards or native currency ownership by itself.

`RunSession::playCurrencyPickupSound` retains the source UI overlap channel
alongside the other run-owned channels. The stored-world entry selects the
registered currency's actual `SoundPlayState`; it executes sound 167 at priority
60 through `SoundPlayScript`, not a request counter. Channel 8 stops the sound
asset's previous playback, publishes its selection before play, applies game
volume and keeps pitch at one regardless of water. All seven backend-operation
failure prefixes retain preceding script/channel writes and latch the world
failure. Instrumented backend tests validate commands, not audible playback.

The stored currency registry's Draw boundary executes the source size-dependent
sprite write and conditional mask write before testing `dflash`. Both gem
resources have four frames; the existing runner sprite-change rule preserves
an in-range index and resets an out-of-range index only on a changed sprite.
These writes reach the spatial owner, so a medium gem's larger mask is visible
to subsequent queries. Integer `dflash=-1` is false in the runner and permits
`drawFx`; positive one suppresses it without suppressing appearance writes.
The compositor remains a required checked callback: this boundary alone is not
live shaded currency rendering or native Draw integration.

The native `CurrencyInstances` owner supplies registered object-248 birth,
source numeric ID, local generation, synchronous shared Create and checked
mark/retirement. `GameplayCollisionView` resolves those lifetimes independently
of provisional gems; room teardown clears them. Its creation entry requires an
explicit source ID and required wall/tile query callbacks. This is not yet the
complete global allocation stream or native terrain-query adapter. The native
animation dispatcher advances registered currency before Begin Step/alarms,
using the source four-frame sequence, signed speed and separate retained image
index. Currency has no Animation End callback, so wrapping cannot retire it.
Later alarm integration still fails explicitly for these lifetimes; normal
gameplay uses provisional gems. Native memory residency and capacity admission
remain to be measured. Clearing the typed owner resets its failure state only
after every retained lifetime has been successfully retired.

### Currency Alarm 4 activation audit

Currency Alarm 4 contains a separate motion program: hard gravity while rising,
uncapped gravity otherwise, decaying bounce gain, no water drag, and a final
`alarm[4]=stepRate` rearm. It must not be substituted for ordinary Step.
However, the pinned currency Create starts alarms 0 and 3, not 4. Ordinary
Step's variable-index freeze loop covers only 0 through 3. The complete export's
literal alarm-4 writers are its own rearm, FINALBOSS, BatteryCharge, Result,
Credit, and `scrCheckInWater`. The latter has one bytecode call site, in player
Begin Step; it does not activate currency's alarm. The source verifier checks
that writer/call-site census so a changed export forces another review.

There is no identified natural activation path for currency Alarm 4 in these
reviewed sources. This is not a proof for arbitrary variable-index writes or
external mutation. The checked scheduler therefore continues to require an
explicit handler if that event is actually reached; do not initialize it or
route it to ordinary Step merely to supply otherwise-unused behavior.

`visitRegisteredCurrencyDraw` enters that boundary through the stored world's
source layer traversal, preserving its effective depth and active/destroyed
filtering. `CurrencyDraw` then uses the shared `drawFx` placement rules for the
admitted default transform: normal draws target the effect surface at fractional
view-relative positions, while death/low-spec commands retain absolute world
positions for direct drawing at their layer boundary. The surface emitter
rejects direct commands and resolves the four-frame currency artwork without
rounding the published position. Original GPU raster sampling is not established
by this adapter; callers still own rasterization and cross-family composition.

The shared live reward wrapper now supplies the run's area to `GemMotion`.
Its fixed-point adapter preserves the area-5 extra RNG draw after both ordinary
velocity draws, and areas 3/4 use gravity 112/12800 and cap 6400/12800. As in
the VM Step, a velocity just below the cap can overshoot for one update before
the following update clamps it. These constants are captured at birth and
reset when a motion record is reused. The shared native tape compares full
Create RNG state across all five areas; host tests also check the velocity
projection against VM random expressions. This does not remove fixed-point
motion, implement embedded-wall relocation or enable later-area generation.

`CurrencySpawnScript` preserves `scrCurrencySpawn`'s caller fields and its two
distinct denomination loops. `atrState` is set only when the caller's direct
parent is 83 and `stomped` compares equal to one. Each object-248 Create runs
before assigning caller `myGem`, then the child receives getAmount, size (tens
only), and optional attraction. Remaining `moniAmount` is decremented last;
an odd/fractional final remainder can become negative. The with environment
keeps its selected child for the amount/size writes, while the subsequent
attraction setter rereads the caller's myGem. Caller fields are reread after
synchronous callbacks, and the twos loop never jumps back to tens.
Checked failure retains preceding children, caller writes and RNG consumption.
Host tests join this script to the real CurrencyCreate event and compare its
full RNG state with a scalar reference, including each operation failure
prefix. The bounded 68000 tape covers a mixed 10/2/2 reward and terminal
capacity exhaustion. Source numeric IDs/native lifetime handles remain the
owner adapter's responsibility. The older live CurrencyReward helper has not
yet been replaced: registered creation, caller publication and exact gem Step
must be connected together before enabling natural reward events.

The live shared gem loop now executes the source first-Step sound choice:
size zero selects 168/169/170; other sizes select 163/164. It copies bounceSnd
to mySound and sets allSet, consuming one WELL word exactly once, even for an
already attracted gem. This happens after the reward's size override and before
movement/terrain queries; an alarm-expired gem never reaches Step. Reusing a
GemMotion record resets this state. CurrencyCreate shares the same fields for
the future exact scheduler. No sound is played by this selection itself.
Gem replay fixture version 9 includes these fields, resulting RNG state,
default alarm boundaries and pre-alarm binary32 animation; its CPU-equivalence digest is
`ffa629f6f72c7cff` (coverage 1023), not an original Windows gameplay capture.
Alarm 0 writes Alarm 1 before that later slot is decremented in the same phase;
Alarm 1 likewise writes Alarm 2. Consequently the default unfrozen gem first
flashes hidden on update 189 and expires on 308, not 190/310. An attracted gem
survives that one-shot expiry and continues alternating visibility. Explicit
per-index alarm dispatch is the host timing oracle. This corrects the existing
live age-based path, not the remaining time-stop refunds or mutable alarm state.

Box Alarm 0 is dispatched by both normal schedulers: the shared event body
sets `wallHp` to zero, without emitting effects or retiring the box. Native
storage keeps one 32-bit countdown per box; it does not allocate twelve slots
for this one-event object. The mixed native alarm snapshot includes registered
boxes in object-ID/registration order, costing an additional 8 KiB of resident
snapshot capacity plus 2 KiB of countdowns at the 512-box limit, not task stack.
Host tests cover countdown, zero retirement without a second dispatch, negative
values and stale/destroy-marked lifetimes; the bounded native reentry tape also
arms a box through ordinary ticks. This does not enable Gunpowder Blocks or
the still-incomplete natural box Step and its `alarmStop(0)` dependency.

The checked `stopBoxAlarm0` adapters now provide that script operation for an
explicit Step-boundary call. They query `TimeStopBound` with the box's registered
published coordinates and mask; the reference owner also covers fractional
side-boundary positions. Native static boxes use their retained integral room
position and the recovered solid 16×16 mask 585, intersected with the exact
ordinary/entry/exit field rows. This boolean query does not require a generic
box geometry record and does not provide instance identity or traversal order.
Allowed time leaves both the alarm and `self.i` untouched; frozen time writes
`i=0`, increments only a positive countdown, then writes `i=1`. Zero and negative
alarms are not revived. Failed queries leave those fields untouched and latch
failure. An unrepresentable increment of INT32_MAX is rejected rather than
overflowing native signed arithmetic; normal source delays are small.
This adds four bytes of script-index storage per retained box-state record.
Both host world adapters have real-mask/boundary tests; the 68000 reentry tape
tests two controlled refund ticks before expiry. The native query adapter is
also cross-compiled. Neither adapter is automatically called by normal box Step
yet: the real player time-stop owner and complete destruction sequence remain
required before that admission.
The post-destroy event tail retains the record but not its collision eligibility:
compatibility collision rejects flag 1 on either participant. Consequently a
destroy-marked box inside the room sides can receive a refund despite overlapping
a field. A retained-lifetime test covers this distinction; do not bypass the
original collision guard just because the event has not returned yet.
The native solid-field query applies that same caller-flag guard to currency:
a gem marked destroyed by its stuck-wall check must not regain field contact
in the remainder of its Step. The cross-owner fixture compares 507 box positions
around all three field kinds, alarm refunds before/after destruction, and the
currency self-exclusion. It verifies the box mask metadata and every stored bit
before relying on the specialized rectangle.

Both box destruction entry points publish the destruction mark before changing
the retained record flag; repeated requests do not republish it. The object-92
ancestor chain has no Destroy/Clean Up events. The stored frame's pre-Draw sweep
owns marked-box retirement, while the native collision owner's `prepareDraw`
retires its marked box slots. Out-of-view sweeps must not erase those records
early. Older terrain, landing, head-contact, bullet, smoke-wall and laser query
paths also exclude marked records, even while their active flag remains true.
The distinction permits synchronous post-destroy child creation and alarm work
without allowing later actors to collide with the dying box. This lifecycle
contract does not replace the remaining natural box Step integration or terrain
redraw notification for that full destruction path.

Debris lifetime sequences can be carried across owner reconstruction without
reusing retired handles or admitting new children to old phase cutoffs.
Restoration requires successful cleanup of all registered children, including
destruction-marked records that no longer contribute to the visible count.
An exhausted sequence remains exhausted after clear/reconstruction. These
contracts are covered by the shared birth-sequence and registered-debris tests.
Both worlds own a 64-entry debris bank and expose creation through their existing
registered lifecycle bridge. Retry clears registered particles before room
teardown; native placement-new reconstruction preserves the bank's birth
sequence. Native next-level preparation also clears active and destruction-marked
debris before replacing the room: object 370 is nonpersistent. The transition
test checks registry/collision removal, unchanged RNG, retained monotonic birth
sequence and rejection of old handles after a new-room birth.
Real collision-owner tests cover creation, cleanup and rebirth in
both worlds. Borrowed banks remain available for isolated tests, but cannot be
mixed with a populated production bank. `stepWithOwnedDebris` routes the stored
bank through the ordinary room scheduler using an explicit checked time-stop
callback. Its test covers animation, alarm countdown/refund, five frozen frames,
motion and cleanup at expiry. The native owned-bank per-instance alarm/Step
hooks have corresponding freeze/motion/expiry checks. Native alarms now run
alongside registered boxes and enemies in the ordinary frame's ordered alarm phase;
native `stepWithOwnedDebris` additionally captures registered debris at Step
entry and runs motion before the existing Step reconciliation. Its checked
time-stop and terrain callbacks are scoped to that tick, never retained on
the caller's stack afterward. A forty-frame native test compares animation,
countdowns, freeze refunds, motion, presentation fields and expiry against the
source primitives, using real collision-owner terrain queries. Callback
failure stops further particle work. This is a controlled integration path,
not a complete mixed-actor source traversal: the native loop still interleaves
some earlier actors' Step/collision work. Live player time-stop and
natural destruction admission remain unfinished; these dispatch adapters do
not establish a complete live debris feature.

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

`BloodEffect::stepPublished` preserves the moving-effect Step boundaries:
capture `imgSp` before querying `TimeStopBound`, conditionally publish speed,
update xx then yy, then call `roundPosition`, which publishes **Y before X**.
A failed query keeps the prior speed capture; a failed X publication keeps
the completed Y write and precise accumulators. A failed callback latches the
event's supplied success flag and prevents retry. `BloodEffects::stepChecked`
uses this body so queries observe the captured speed; `step(bool)` remains the
successful-state scalar reference. This does not by itself supply blood's world
registration, mixed-family dispatch or rendering ownership.

`BloodInstances` supplies a separate fixed, generation-checked registered owner
for the object-220/sprite-84 specialization; `BloodEffects` remains the scalar
owner reference. Birth registers the serialized sprite-less instance before
the no-RNG Create. The caller's `myFx` callback then observes completed Create
state before sprite 84, speed 0.5, angle and velocity assignments. Constructor
coordinates narrow to binary32, while direction math consumes the original
angle argument. Failed operations retain the allocated numeric identity and
completed prefix. Animation publishes its index before marking Animation End
destruction; retirement cannot reuse an executing lifetime.

`StoredBloodRegistration` binds that owner to real instance records, collision
geometry and layers, with the room numeric-ID allocator. Sprite, angle, speed,
ordered position and destruction changes are published through this bridge.
`bloodRegistration`/`spawnBlood` expose its creation boundary; the borrowed bank
must be cleared before room teardown. These are not automatic death-script
births: room-phase admission, time-field queries and rendering are still
required before natural blood effects can use this path.

The native `GameplayCollisionOwner` binds the same `BloodInstances` store to
object/layer membership and reference collision geometry. The native identity
table retains its existing on-demand geometry policy. Explicit source-ID births
preserve caller and setter order; they do not invent the missing global native
allocator. Room animation dispatch advances these records and Animation End
marks them for the normal destruction boundary. Blood has no alarm or End Step
event and is excluded from those family walks. Room handoff/retry clears the
bank while preserving its binding and generation history. Ordinary Step still
requires a checked time-stop query supplied by its caller; automatic death
births and Draw admission are not enabled by registration alone.

`StartupRuntime::withBloodEvents` admits a borrowed blood bank to the stored
room's animation, ordinary Step and deferred cleanup phases. Its query callback
runs inside the checked Step after the first `imgSp` capture, not before event
dispatch. `bloodTimeAllowed` obtains the current room width only when stopped;
the registration then queries parentTimeField (82) through its resource-backed
rotated mask before checking the strict side bounds. An unstopped query bypasses
collision work. The callback must read the source player's current `pTimeStop`;
this interface does not infer it from goal/shop locks.

Native `bloodTimeAllowed` uses the shared source field-family place query over
registered lifetimes, including list/tree preparation, precise rotated masks
and temporary position/cache publication. It no longer substitutes a
family-local boolean overlap scan, even though its result is boolean.
`BloodGoalContact` remains a standalone geometry specialization, not this
owner's production query adapter. Reference/native fixtures compare fractional
contacts against ordinary, entry and exit fields and check restored positions.
`stepBlood` invokes the query after saved-speed capture. Automatic player
`pTimeStop` and mixed-family ordinary Step admission remain separate
integration requirements.

`stepWithTimeStopBonesAndBlood` composes the stored blood dispatch with the
existing bone/shard/time-change tick, without advancing the world twice. Blood
reads `stop.player.stopped` inside its ordinary Step query, after the player's
Begin Step callback. Enter/hold/exit transitions therefore freeze/resume bones
and out-of-field blood from the same state. A failed synchronous transition
retains earlier animation but prevents both ordinary Steps and the collision
pass. The transition callback still owns the precise player pose and actual
audio/controller effects; this composition does not supply those missing live
adapters or the native mixed-family scheduler.

Registered currency can surround that tick with `withCurrencyEvents`; its
ordinary Step must read `stop.player.stopped` at dispatch, not capture the
previous tick's value when installing the scope. `stepSource` then freezes
out-of-field free currency and refunds positive alarms 0–3 after the alarm
phase, but attracted currency continues homing without that refund. The joint
lifecycle fixture exercises both branches alongside bones and blood through
enter/hold/exit, using source free motion rather than a no-op motion callback.
A failed Begin Step transition prevents currency initialization and its sound
choice RNG as well as alarm/Step dispatch. This is scoped scheduler evidence;
the fixture still supplies the player pose and transition services explicitly.

Object 220 is visible, depth 0, has no parent and defines no Draw event. Its
default Draw therefore belongs in the ordinary layer traversal, not a particle
overlay or the smoke controller surface. The SDL game renderer admits these
registered records with their published sprite, fractional position, index,
scale and angle. Animation End excludes the marked record before retirement.
The existing Amiga sprite-84 bank serves unrotated smoke; it is not sufficient
for moving blood's two random angle ranges. Native blood Draw admission still
requires offline rotated variants and verified raster selection.
`tests/blood-angle-art-test.py` rejects treating those ranges as unrotated and
half-turn artwork, and rejects rounding fractional anchors before sampling.
It uses the same nearest-pixel model as the projectile artwork probes, not a
Windows GPU oracle. The original CPU vertex path preserves fractional positions;
the remaining whole-pipeline raster uncertainty is described in
[projectile motion](projectile-motion.md#original-cpu-vertex-path-and-fractional-anchors).

- Blood emits at published x/y, not the precise xx/yy accumulators. Its two
  angles are `random(20)` and `180-random(20)`, respectively. Object 220's Create
  initializes motion state without RNG; `emitMovingFx` then assigns sprite,
  animation speed, angle and the direction-derived velocity.
  `StartupRuntime::spawnBloodBurst` routes these births through the stored
  room's numeric-ID allocator and blood registry. Its caller callback receives
  each child identity at the `myFx` write boundary, before child setters. A
  rejected first/second publication retains the corresponding allocated child,
  completed prefix and one/two RNG draws, and prevents replay. It does not
  replace the caller's own `myFx` storage or admit natural death events by itself.
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

`StoredCorpseExpiryRegistration` is the resource-backed room path for object
242. It allocates from the enclosing numeric-ID stream, publishes constructor
geometry before the synchronous Create rate assignment, and retains failed
births for explicit teardown. Its alarm adapter executes the already-fired
corpse alarm body: child Create, source sprite override, then parent destruction.
The child joins ordinary room layer traversal at its serialized creation depth;
it does not inherit the compatibility painter's synthetic -1000 depth.
`withCorpseExpiryEvents` dispatches animation from the mixed room walk and
retirement from cleanup. A child born during alarms first animates next frame.
Animation End publishes its index before marking destruction, without unlinking
the current animation iterator. This registered path does not yet replace the
legacy RunSession pool in both backend renderers or enable natural deaths.

`CorpseExpiryInstances` supplies the corresponding native typed storage. It
uses the same `CorpseExpiryEffect` state and unit-track advancement, takes an
explicit source numeric ID from the enclosing creation stream, and keeps that
ID separate from its generation handle. Positioned constructor publication
precedes Create, inactive records do not animate, and Animation End publishes
the index and destruction mark without freeing the slot. Failed publication
retains the partial lifetime for teardown. The native resource owner registers
these lifetimes as object 242 with their supplied source IDs and serialized
depth, dispatches animation in the mixed room walk, and retires marks at the
pre-Draw cleanup boundary. Live and cached actor traversal route them to the
Amiga planar draw list, outside the compatibility foreground-effect pool.
Room/Retry teardown clears them without resetting the generation counter.
Natural corpse alarms still need native admission and a complete source-ID
allocation stream; the explicit-ID entry point does not invent that stream.
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
Destruction does not retire at the end of an event or on a subsequent birth.
Marked lifetimes remain resolvable through `find`, but are excluded from `get`,
event dispatch and drawing. Only the enclosing room cleanup invokes
`retireRequested`; retirement callbacks run before slots are freed. This follows
the ordinary destruction versus room/Draw-entry cleanup distinction pinned by
`verify-runner-destroy.py`. Capacity exhaustion consumes
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

`StoredCorpseRegistration` performs the same Create and death-wrapper sequence
against the stored room registry. Constructor geometry and resource metadata
are registered before Create RNG and queries. Parent-wall queries use object
57, tile queries use layer 100, and each relocation setter publishes immediately.
The wrapper stores the room's numeric child ID, not a corpse-local handle, then
publishes sprite and facing changes on that same lifetime. Failed creation or
publication retains the accepted prefix for explicit teardown and latches the
world failure. `StartupRuntime::createRegisteredCorpseFromDeath` supplies the
room allocator, RNG and Rest in Pieces state; the caller still owns publication
of the returned dying-enemy fields.
`SkeletonInstances::Publisher::deadBody` supplies a source view whose private
script fields alias the live skeleton: body sprite, capped impact, child ID
and horizontal velocity are retained as each statement executes, including
failed event prefixes. Spawn coordinates come from published builtins rather
than the motion accumulators; facing is read from the published scale whenever
the script requests it. The stored runtime's registered creation entry accepts
this view without copying those mutations back at the end. This connects the
corpse portion of the death caller, not its preceding blood/reward services.
`withCorpseEvents` admits registered corpses to the stored room's animation,
alarm, ordinary/End Step and cleanup phases. Animation publishes the evaluated
index before committing its timeline. Event dispatch imports the room's current
alarm bank and published builtin state; `alarmEvent` executes the body without
a second countdown. Step adapters must publish position and destruction through
the supplied publisher; unmatched state is rejected. End Step uses the retained
view and the shared out-of-view test, with removal deferred to room cleanup.
`stepInWorld` uses that lifetime's transformed resource mask for parent-82
time-field and solid-56 movement queries. TimeStopBound queries the field before
testing its strict side-boundary exemptions; query failure leaves motion
unexecuted. Frozen Step refunds alarms 0–2, not activation alarm 3. Position
setters publish Y before X through the same registered owner. Area, Rest in
Pieces and synchronous alarm/death children remain caller-supplied checked
dependencies. This scope is not default admission of the
complete death chain, nor does it provide native registration or Draw routing.

`CorpseDraw::surface` resolves the existing immutable planar corpse frames,
including pre-mirrored variants. It retains fractional world anchors for the
compositor, skips flash-hidden/destroyed/out-of-view and zero-scale draws,
and rejects transforms outside the admitted unit-scale artwork. It does not
choose cross-family order: smoke and corpse share a depth, so the caller must
merge their source room order rather than append a separate corpse pass.

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
The adapter tests cover fractional movement and failure cleanup.

The native `GameplayCollisionOwner` now installs this registry with a disjoint
key range after script effects. `GameplayWorld::spawnDebris` accepts borrowed
fixed storage, consumes the world's RNG, and publishes the completed Create
state through the resource-backed owner. Debris uses compatibility depth -1000
instead of the OBJT default. Room animation captures the owner's cutoff and
dispatches in the existing cross-family room order; explicit Draw-entry cleanup
retires marked particles. Retry clears borrowed storage before reconstructing
the world and retains the registry and monotonic identities. The native-owner
tests cover all five area sprites, full birth RNG, fractional position writes,
animation publication, marked-record retention and retry. This does not yet
connect natural combat births, debris alarm/Step dispatch or
Draw submission. Those paths must be integrated
before enabling the effect in normal gameplay.

`GameplayCollisionOwner::debrisPlaceMeeting` supplies the native registered
shape-query adapter for Step's `sParentSolid` (56). It uses the same binary32
argument narrowing, family preparation, candidate dispatch and caller-position
restoration as the reviewed `place_meeting`/`instance_place` search. It does not
replace the broad solid family with wall-only, point or occupancy-grid queries.
The resource-backed regression checks 60,516 quarter-pixel probes across all
three debris sprites against walls, boxes and thin platforms, plus target
deactivation/destruction and stale-caller failure. These pairs have rectangular
collision resources; this matrix does not verify every precise-mask or rotated
family member. Shop furniture and other still-unregistered solid actors remain
a coverage gap before natural debris Step admission.

Selected-event dispatch is available through `advanceDebrisRoomAlarm` and
`GameplayCollisionOwner::stepDebris`. The latter accepts a shared room key,
resolves the lifetime again, consumes `GameplayWorld`'s RNG, queries registered
solids and publishes every coordinate write through the registry. The caller
must supply a checked per-instance freeze decision derived from `TimeStopBound`;
there is no default that silently treats missing time-field state as running.
Both entry points preserve phase cutoffs and destruction marks; they do not
sweep the debris pool or retire during Step. A failed decision/query poisons the
world and stops later dispatch rather than repeating partially completed work.
The integration fixture dispatches selected particles through the real room
Step snapshot for 16 seeds and 40 frames, comparing full state/RNG with the
source-motion component using independent rectangular terrain overlaps. It also
checks newborn exclusion and terminal decision failure. This is not yet normal
gameplay scheduling: cross-family alarm dispatch, live time-field decisions,
production scheduler admission and rendering remain required before natural birth admission.

`StoredDebrisRegistration` provides the SDL stored-world counterpart, retaining
both the effect address and generation-checked `InstanceRecord` identity. It
publishes completed Create fields at fractional `birthAt`, uses compatibility
depth -1000, and forwards movement, animation, solid-family queries and destroy
marks through the existing collision binding. `StartupRuntime::spawnDebris`
and selected `stepDebris` consume the world's RNG, not a separate effect stream.
Room cleanup retires through the particle owner before erasing the real record.
The resource-backed fixture covers all five area variants, mirrored alarm
refunds, collision exclusion and slot reuse. Keep the registration and effect
owner alive until cleared. Spawn installs room-owned animation and cleanup
bindings; reset, room changes and Retry clear the particle owner before removing
stored records.

`StartupRuntime::stepWithDebris` scopes the checked freeze decision and event
adapter to one real scheduler tick. Animation follows room traversal; alarm
bodies run only after the generic scheduler decrements the record bank. Shared
`DebrisMotion::alarmEvent` is distinct from `advanceAlarm`, so firing does not
decrement twice. At ordinary Step, the adapter copies the scheduler's whole
bank, including countdowns which did not fire and zero-to-inactive changes,
before the shared motion code can refund positive alarms. Explicit manual
`advanceAlarm` remains for callers that own the countdown themselves and must
not be combined with scheduler advancement. A normal tick with registered
debris but no scoped event adapter fails rather than silently omitting Step.
The full scheduler fixture covers 40 ticks, five frozen frames, single alarm
dispatch, post-destruction exclusion, cleanup and Retry. It supplies freeze
decisions explicitly; wiring original live time-stop state, both renderers and
natural combat births remains unfinished.

Both ownership adapters expose `debrisFrozen` using shared `TimeStopBound`:
when the caller supplies true source `pTimeStop`, query the particle shape
against parentTimeField (82) before testing strict `x<160` / `x>room_width-160`
exemptions. False `pTimeStop` skips field queries entirely. SDL reads current
room width from serialized metadata; the native caller supplies that width.
The particle's explicit mask is resource 598, regardless of its area-specific
visible sprite; the extraction verifier pins this mapping. A 4,225-position
native test compares time-field overlap against the two original bit masks,
including edge-only contacts a centre-point query would miss. Both backends
test fractional side boundaries; SDL also tests a goal-field descendant.
These adapters require the original player's current `pTimeStop` value at Step
dispatch, not a cached frame-entry value, goal stop or control-lock substitute.
They do not themselves wire the player's Begin Step transitions into both
production schedulers.

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

Registered debris now participates in the native room actor Draw visitor and
SDL's stored room Draw traversal. Both controller producers enumerate that
order, resolve the original flash gate and append offline debris colors before
shade subtraction. Death/direct fallback draws at the room actor boundary,
without publishing the same particle again through the controller. The SDL
borrowed lookup checks the stored generation and source ID before returning
the shared particle; the native visitor resolves its lifetime-backed record.
No coordinate or RNG update occurs during this traversal.

`DebrisDraw::frame` admits the nonnegative 0..7 indices produced by Create and
sequence animation. The ordinary bitmap runner at `0x4af747..0x4af78b` promotes
the float subimage, floors it, narrows/truncates and applies frame-count modulo.
The positive finite SSE helper path is pinned in the sprite-vertex verifier.
Both backends use this selector, including pre-animation values beyond the
artwork's frame count. Unsupported state fails rather than indexing other
offline assets. Tests cover all fourteen frames through the compositor and
registered native/SDL drawing, flash changes, cleanup and direct fallback.

This does not enable natural box/death births, settle equal-depth ordering with
the separately owned foreground effects, or prove fractional GPU sampling.
Controller coverage and native direct drawing currently use the existing
nearest-pixel backend policy; SDL direct drawing retains its floating anchor.
Original-game fractional raster comparison remains required. Neither an
injected-particle rendering test nor the normal camera replay proves natural
combat effects are complete.

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

The supported contract includes smoke, shard and currency radians in [-13,13] under
nearest-even FP mode, with signed-zero results. The runner's entry guards at
0x74b621 and 0x74b7e1 strip the sign before range testing; the polynomial itself
receives the signed argument. The default direction wrapper retains speed [0,8]
and angles [-585,585]; currency explicitly admits speed [0,9] and angles
[-720,720], without normalizing angles before the source float operations.
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

### Drone projectile registration boundary

The native resource-backed owner binds a separate `DroneBulletInstances<32>`
store for object 266 (parent 89), rather than impersonating usual bullets (261).
Its collision-view slots follow the existing families; these internal slots
are not numeric GameMaker IDs. Callers must supply the source ID explicitly.
Construction publishes binary32 room coordinates with serialized sprite 458,
then the synchronous Create override publishes sprite 459. Angle, image index
and subsequent position writes update the same retained owner record.

Room animation and alarm dispatch include this family, and Draw-boundary
cleanup retires destruction-marked lifetimes. Retry and next-cavern teardown
clear the borrowed store while preserving its generation sequence. The
positioned-actor lifecycle binding supplies transforms and destruction without
pretending that the store itself implements collision queries.

The shared resource-backed registration tape runs on host and 68000. It checks
fractional coordinates, parent population, setters, activation, post-destruction
position publication, retirement and slot reuse. Host scheduler tests also
exercise the actual room alarm/animation callbacks and Retry. These are injected
lifecycle checks, not original-game visual comparison or a stock-memory fit test.

Natural births remain disabled: full Step collision adapters, enemy
damage, native numeric-ID allocation, projected layer drawing/offline angle
artwork and source-ordered box admission still need integration.
The 32-slot bound is an implementation limit, not a recovered original limit.

`StoredDroneBulletRegistration` binds that same shared store to SDL's instance
records and collision/layer lifecycle, using the world's numeric-ID allocator.
It preserves fractional constructor positions and publishes sprite, angle,
image-index, movement, activation and destruction writes to both representations.
The bridge must stay at a fixed address and be cleared before room teardown.

`stepWithDroneBullets` scopes its callbacks to the real room animation, alarm,
Step and End Step phases. The global stored-world alarm countdown is imported
before shared dispatch; copying an older shared countdown back would undo that
tick. Animation-end destruction excludes the subsequent alarm/Step dispatch.
Missing dispatch ownership and stale registrations fail rather than silently
running an incomplete projectile. The caller still supplies Step
operations and explicit pre-Draw retirement. No natural source births use this
entry point yet. Stored-world regression tests exercise these phase and lifetime
boundaries with injected projectiles; the phase-routing fixture does not claim
to implement or verify the missing live solid/enemy collision adapter.

`ProjectileViewRetirement` implements the drone End Step's two source scripts
in order. `scrOutofview` checks y against view y minus 160 and plus 3000,
destroying only in room 11. `scrBulletOov` then independently checks below
view y plus view height plus 32 and above view y minus 100, in every room.
All comparisons use the runner real-comparison contract. Destruction does not
short-circuit the remaining script, and later expressions read published y and
view fields again rather than reusing a pre-destruction snapshot.

The shared host/68000 tape checks strict boundaries, both room branches,
repeated destruction after a synchronous position mutation and every callback
failure prefix. The two source-script hashes are pinned alongside the drone
events. Both normal-main-room adapters invoke this composed End Step path with
the preceding published view, not the camera's newly followed position. The
native adapter preserves the Q12 snapshot when converting it to binary64 and
skips traversal/conversion when the store is empty. The SDL scoped scheduler
owns End Step internally rather than asking the external Step callback to
implement it. Other SDL rooms are rejected pending their view adapter.
Both use retained projectile positions and mark destruction for later cleanup.

Host tests place projectiles on and one pixel beyond the published lower edge;
the 68000 room-lifecycle tape also verifies an injected out-of-view projectile
is marked by the actual world update. Explicit camera setters, dirty-camera
transitions, external writes outside the admitted position publisher and full
cross-family End Step ordering remain outside this path's evidence. This does
not make the approximate camera-motion adapters authoritative or enable
natural projectile births by itself.

## Corpse-consumption particle (object 223)

Knife and Fork creates five `fxBlood` instances (223), not the separate blood
actor 220. The resource uses sprite 92 (`sprWaterSplash`, eight frames), depth
zero, no parent and default Draw. VM events 2004–2008 define Create, Alarm 0,
Step and water/wall contacts; all three termination events call
`action_kill_object`. There is no Animation End handler.

Create samples `random_range(-2,1)` into `ysp`, then `random(1)` followed by
`choose(1,-1)` for `xsp`. It sets alarm 0 to 120, writes **global** `grav=0.2`,
sets builtin `image_speed=0.3`, then samples `irandom(7)` for `image_index`.
The global write must occur before the remaining animation/RNG operations.

Step publishes `x+xsp`, then rereads and publishes `y+ysp`; it divides `xsp`
by 1.01, adds current global gravity to `ysp`, then rereads Y and subtracts
current global fall speed with a third position publication. Builtin position
writes narrow to binary32; custom speeds retain binary64. No time-stop guard
appears in this event. `verify-consumption-blood` pins the VM instructions and
resource identity; the portable event implementation is `ConsumptionBloodEffect`.

## Wall-created limbo shard (object 379)

The area-four wall End Step creates `limboShard` synchronously after publishing
its image index. Object 379 uses sprite 91, has no parent and has no alarm
handlers. It is not an `fxTimeShard` subtype. `verify-limbo-shard.py` pins its
five event bodies, resource bindings, input hash and extractor version.

Create consumes seven RNG words: two for integer horizontal velocity, one for
vertical choice, one for ascent speed, one for animation-rate magnitude, one
for its sign and one for image index. Its cap is 16, or 5 with lowSpec. Custom
motion fields retain binary64; builtin position and image fields use binary32.

Step's TimeStopBound guard covers the firing response, animation rate, wall
bounce, ascent and flashing. Inertial movement, damping, rounded position
publication and retirement checks remain outside that guard. The firing strip
uses strict bounds. A bullet contact or firing response sets noTouch and
alarm 2; there is no source alarm handler that clears noTouch. Do not borrow
the time shard's cooldown handler. Population overflow alone does not destroy
the actor: it also must be outside the view. The final above-view test still
runs after an earlier destruction request.

Collision code 2771 inherits through parentPBullet (89), but the collision
context retains the selected bullet's actual object subtype and generation.
The stored and native `dispatchLimboShardBulletCollision` adapters validate that
binding and resolve the selected lifetime before asking its source-motion owner
for custom `xsp`/`ysp`. They never derive these fields from engine velocity or
rounded position differences. The horizontal sign scales one shared RNG draw;
vertical velocity is copied without builtin-field narrowing. Inactive, marked
or noTouch shards do not read velocity or consume RNG. An unavailable source
owner is an explicit failure. The adapter contract does not admit the older
Q20/Q32 bullet trajectory as source motion.

Draw calls draw_self when `!dflash`; the authored -1/+1 states use VM numeric
truth, so the negative phase is visible. This is direct default sprite drawing,
not the time shard's effect-surface routing. `LimboShard` implements the event
state and checked synchronous publication boundary; it does not own world
registration, TimeStopBound queries, animation advancement or Draw submission.
