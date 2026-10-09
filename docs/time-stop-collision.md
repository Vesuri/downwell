# Time-stop collision inputs

## Native player field meeting query

`GameplayCollisionOwner::playerTimeFieldMeeting` exposes the boolean
`place_meeting(xx,yy,82)` input to player Begin Step using the registered player
and its actual sprite/mask/transforms. It delegates to the existing VM place
wrapper and resource-backed compatibility collision path; it does not replace
the player with a point or use the goal-control lock as time-stop state.
Fractional-position tests with both player facings cover hits/misses and
preservation of published/previous-frame coordinates after temporary query
movement. Missing mask resources fail without overwriting the caller's result.
This query alone does not connect player transitions, shard identity lookup,
their effects, or audio to the native Begin Step scheduler.

`playerTimeFieldSelection` uses the same registered query and returns a
`RegisteredFieldContact`: a generation-safe internal handle and copied
published coordinates, or an empty result on a miss. Overlapping-field tests
cover first eligible selection, activation exclusion and clearing a prior hit.
This internal result is deliberately separate from `InstancePlaceResult`.
`playerTimeFieldInstance` resolves the selected typed lifetime to its supplied
source ID and returns that ID with copied published coordinates. A miss returns
the VM sentinel -4. A selected legacy field with no source ID fails without
overwriting the output; passing a slot/generation as `field` is never allowed.
Ordinary fields admit `createTimeFieldWithSourceId`: the source ID is stored
and registered before the synchronous radius RNG draw. Zero, out-of-domain
and duplicate same-family IDs are rejected without consuming that draw.
Registration failures retain the already-published birth prefix. Existing
unnumbered callers remain suitable only for boolean/internal-handle queries.
The supplied-ID path does not supply the missing global numeric allocator or
complete synchronous cross-family creation order. Those remain necessary for
automatic natural `scrTimeShard` calls. Reference/native/native-scenery tests
exercise the numeric query inside the actual player Begin Step transition,
as well as miss and missing-ID behavior.

Both native ownership modes use the shared `RunnerPlaceQuery` for this
selection, rather than returning the first overlap from a family-local scan.
Parent 82 is admitted only when resource ancestry contains the owned ordinary,
goal and entry types (82, 255, 371). Family preparation chooses list versus
spatial traversal from population and tree height; the indexed path flushes
pending bounds and restores the player's cache after temporary movement.
Selected slots are resolved back to generation handles before returning field
coordinates. No numeric VM identity is invented. The same query tests compare
empty, singleton and overlapping mixed-field populations, fractional player
positions and both facings across reference/native/native-scenery ownership.
Blood, currency and box boolean contacts also use this shared traversal and
position/cache publication. Their boolean return does not make traversal side
effects optional. This does not admit arbitrary native selectors.

## Field wind audio ordering

`goalTimeField`'s player collision checks `global.voidWind` first. If it is not
playing, it chooses from sound IDs 349–355 (one runner `choose` draw), assigns
`self.windsnd`, calls `soundPlayOL(sound,90,0,1,"UI")`, stores the returned
voice in `global.voidWind`, then sets that voice's gain to 0.5 immediately.
The initial global voice is -1 (`scrGlobalStuff`).

The UI specialization of `soundPlayOL` selects channel 8, forces `cutPrv=0`,
checks/stops the selected sound resource, and writes its resource ID to
`global.OLchannel[8]`. It plays with priority 90 and no looping, reads the new
voice's gain, multiplies by `global.gameVolume`, publishes gain and pitch 1,
then returns the voice only if it is playing (otherwise -1). Offscreen,
underwater-pitch and no-music branches do not apply to channel 8. The outer
collision still calls gain on the returned value, including -1.

`TimeFieldWind` implements this sequence with a synchronous checked audio
interface. Per-instance sound and shared channel storage are supplied by the
caller; its voice is shared across fields. Failures retain completed writes,
RNG and audio effects and stop later work. Tests cover every operation's failure
prefix, both sound-resource guards, unsuccessful playback and repeated contacts.
The live contact overload also receives the field's `SoundPlayState`: script
locals such as `sfxIndex`, `sndsnd`, `sndgain` and `chanNum` are caller-instance
variables, not global wind state. Stored fields retain these in their instance
record; native entry/exit fields retain them in their typed record. Contact with
a playing global voice does not initialize or overwrite a different field's
caller state. The standalone overload retains a reference caller for isolated
script tests; neither live dispatcher uses it.
`StartupRuntime::dispatchTimeFieldWind` validates live self/other generations
and the selected inherited binding before invoking it. `windsnd` is stored on
the real instance and included in its digest once assigned. The default dispatch
uses `RunSession::fieldWind` and the same `overlapSounds[8]` used by other UI
sound scripts. Ordinary room changes retain them; new-run reset restores the
wind voice to -1 and the channel to 0. Explicit state-provider overloads remain
for reference tests. Live tests cover
exit fields, the start-field descendant, shared playing/expired voices and stale
event rejection. `StoredDitherCollisionPass` admits 255/35 and inherited 371/35
alongside wall pruning when supplied audio; its skeleton/combat entry points
include those pairs in the same stream. Collection, overlap, destruction guards
and paired dispatch are shared, not a second wind-only pass. Player 35 and its
parent have no reverse collision event for these fields. Other time-field
families are not implicitly admitted. The SDL main loop uses the audio-backed
entry with `AudioSDL`: resources 349–355 are decoded by libsndfile before gameplay,
and device-time voice expiry drives the shared guard. Resource IDs and voice IDs
occupy separate ranges; stopping a resource stops all its voices. Playback uses
stereo float mixing with linear rate conversion, not a claim of original-runner
sample equivalence. Capacity exhaustion and unsupported gain inputs fail instead
of pretending to play. This admission uses the default volume of one (the wind
script subsequently sets the voice gain to 0.5); settings integration remains
open. Native device playback and complete native mixed-pass admission remain open.

SDL immediate gain controls use independent resource and voice state. Loaded
asset gain initializes the resource; each newly played voice starts at one.
Resource writes affect existing and future voices without changing their
individual gain values. The mixer multiplies both binary32 controls. Gain
arguments narrow to binary32 and negative values clamp to zero; unsupported
subnormal/nonfinite/range inputs still fail explicitly. Timed ramps are not
silently approximated by immediate writes.

`tools/verify-runner-audio-gain.py` pins the original executable evidence:
`0x558d70` narrows the gain argument with `fstp dword`; `0x49e340` reads voice
gain at `+0x24` or resource gain at `+4`; `0x4a0ef0` selects independent gain
records and clamps negatives; `0x4a3670` multiplies the resource and voice
gains before device publication. Resource loading at `0x4a2fc9` copies the
serialized gain, while voice initialization at `0x49f26b` sets one. The timed
branch records start/end timestamps and separate ramp endpoints.

Timed gain controls share `RunnerAudioGain`. The audio update at `0x4a21f0`
samples the microsecond clock once, updates resources (`0x4a22d2`) and then
active voices (`0x4a23f9`). Its direct call at `0x50f4ea` precedes the main-loop
work at `0x50f53c`; it is not a sample-mixer callback. The clock at `0x4d0230`
converts elapsed performance-counter ticks to integer microseconds. SDL calls
the control update before game update and leaves the device callback to mix the
last published gains. Reads and retargeting do not implicitly advance a ramp.

Interpolation divides elapsed/duration in binary64, narrows the fraction to
binary32, then separately rounds the target-minus-start subtraction, product
and sum in binary32. At `now == end` it still interpolates and retains the ramp;
only `now > end` copies the target exactly and clears it. An immediate write
cancels the ramp. Tests include a target rounded away by interpolation at the
exact endpoint, then restored on the next update. The admitted duration domain
is 0–2,147,483 ms (including the source 200/500 ms ducks); negative durations
and the runner's overflowing signed-32-bit `ms*1000` cases fail explicitly.
This establishes control semantics, not original device sample equivalence or
runtime floating-point-mode verification. Native device audio remains open.

SDL pause/resume preserves voice ownership, playback cursor and gain controls.
The resource forms affect currently matching voices; a later play starts
unpaused. The runner's pause dispatcher/helper (`0x49edb0`/`0x49ee70`) admits
active, playing sources; resume (`0x4a0370`/`0x4a0430`) admits paused sources.
`audio_is_paused` (`0x4a1110`) tests the device paused state. In contrast,
`audio_is_playing` (`0x4a11e0`, voice helper `0x49ec50`) accepts both playing
and paused sources. A resource query succeeds if any matching voice qualifies.
These routines are pinned by the gain verifier as well. Gain update tests only
active ownership, not the device paused state, so ramps advance while playback
is paused. SDL tests exercise the shared `TimeStopAudio` pause/resume/query
entry points, retained cursors, mixed paused/playing resource voices and ramps
on paused voices. This does not yet admit natural player time-stop transitions.

The stored and native Begin Step adapters apply the stopped-player oxygen floor
to shared `RunSession` state before notifying the external effect callback.
This is the final `global.oxygen < 10` / write-10 branch in player Begin Step
blocks 78–79, not an unconditional refill or an integer rounding operation.
`scrPlayerGlobalStat` initializes the sole source-written oxygen maximum to 100;
`scrPlayerInit` copies that maximum on player creation. Shared run reset and
player creation restore 100. Tests cover fractional values immediately below
and above ten, negative values, repeated stopped ticks and both world adapters.
The water depletion/collection events are not implemented by this floor.
Source assembly SHA256: player Begin Step
`46a8178a1b7cf78ff8f14f204047096d00717ea59da6e2cd74d908cf1932fed5`;
`scrPlayerGlobalStat`
`34dfd0f192ca34217da36cdfd001b792a3e7d496d3fc8c3e9967f8ce7ff2266a`.

Registered player damage uses shared `RunSession::playerDamaged`. Player
creation and run reset clear it. Object 35 Alarm 1 clears it at its mixed alarm
position; zero and negative starting counters do not fire. The stored adapter
uses the real record alarm bank. Native player ownership retains a compact
Alarm 1 counter, captures the player alongside the other implemented alarm
families, and invalidates that capture on player lifetime/activation changes.
Explicit alarm writes require the selected live player handle; these setters
do not claim to implement general object-selector lookup. Other player alarm
events are not admitted by the compact native counter. The old area-one
contact countdown remains separate pending replacement of that entire damage
path; it is not a second owner of this source alarm.
`bindContactDamagePlayer` supplies the shared damaged guard and alarm write to
both `scrTypicalDamage` and its nested `scrPDamage`, while retaining the
attacker's caller fields. Pass the borrowed binding through the world's
`typicalPlayerDamage` entry so Timeout births still use its registered path.
The selector callback runs only at the final Alarm 1 assignment, after the
damaged flag write, and must perform the source object-selector lookup then.
It is not a cached handle or a WITH loop.
The same registered state reads Candle (upgrade 7) at `scrPDamage`'s entry
and Timeout (upgrade 17) at the later `scrTypicalDamage` branch. These are
live inventory reads at separate script positions, not a snapshot of both
upgrades at contact entry. `scrIncrementPug`'s switch maps these IDs to
`pugCandle` and `pugTime`. The explicit-event reference path retains external
upgrade callbacks. Registered tests supply conflicting callbacks and check
90/240-tick alarms, conditional field allocation and its Create RNG draw.
The explicit-event-state overloads
remain the reference path. Both world tests deliberately supply a conflicting
external immunity value, verify that the real global wins, and verify that a
second guarded contact neither selects the player nor repeats effects.
The registered binding also performs health subtraction and damage-number
creation through the world. Health uses the admitted integral representation:
binary64 subtraction must have an exact integral result; fractional/out-of-range
results fail rather than round, and negative health is not clamped. Skeleton
and bone contacts use damage one. Number creation reads the portable player's
captured movement globals in absolute room coordinates, not the attacker's
position or the current rounded player sprite. The existing number owner
subtracts four from Y before narrowing the birth position. Both world tests
stop at a deliberately failed subsequent sound call and check that HP and the
number birth survive while the later damaged-flag write has not happened.
This still requires the production sound/controller/jump/effect dependencies;
the existing portable player coordinates are not a substitute for the remaining
source-precision motion migration.
The audio-enabled `bindContactDamagePlayer` overload accepts the attacker's
existing `SoundPlayState`, current sound environment and backend. It executes
the shared `soundPlay(5,90,0,0)` followed by the owned 500 ms music duck at the
source script positions. Skeleton damage reuses the same sound fields as its
throw/hurt/death calls; bones own corresponding sound fields, reset on birth.
Plain damage playback neither cuts previous voices nor overwrites unrelated
overlap-channel fields. Tests cover all eight backend failure positions, both
attacker types, volume/underwater pitch, and playback/ducking through SDL audio.
This adapter does not supply missing native device playback or load the game's
sound resources into the normal SDL scene automatically.
Registered damage's controller WITH uses live world ownership, not external
damage-event callbacks. The stored world visits object 37 and descendants;
the native collision owner supplies its admitted controller membership stream.
Each eligible record receives room speed 40, `slown=23`, and Alarm 0 = 40, in
that order. The speed binding is the same global used by momentDelay and
time-change effects. Missing speed fails at the first eligible controller,
after the hit marker; an empty/inactive WITH performs no speed write. The
explicit-event damage overload retains its original callbacks as an oracle.
This binding does not supply the missing native startup controller births or
establish complete original cross-family membership order.
Alarm 1 VM SHA256:
`e1dcdae86d6152a96607b2d8429582255d012ef2e742490fcd2ed8fcf9bf7d13`.

The registered damage binding also owns `global.pHit`, separately from
`pDamaged`: sound and music duck complete before damage writes 1, and the
controller WITH follows that write. Player End Step advances 1 to 2 and then
to 0, with no time-stop guard. `scrPlayerInit` does not reset this global.
Both world schedulers dispatch that End Step; the native mixed actor phase
captures all admitted families before executing player 35 ahead of boxes 92.
This does not admit the separate oxygen-damage Alarm 9 writer. That event still
needs its complete source body, not an isolated marker assignment.
Player End Step VM SHA256:
`12b8c0939c68c322cb14d15e4f5c28e42fc53e84e14aac274b6003cf8cfe790c`.

The stored registered-skeleton pass also admits skeleton contacts with fields
82, 255 and 371 in that same collected pair stream. It resolves the inherited
83/1066 event and sets health to zero unless `timefieldImmune` is set; it does
not run the death-effects sequence or retire the actor from inside Collision.
Those actions belong to the later source Step. The selected field families have
no reverse skeleton handler. Further descendants require an explicit reverse-event
audit before admission. Field-only dispatch does not require a fake player-damage
provider. This does not install the complete skeleton pipeline in the SDL main loop.
`GameplayCollisionOwner::dispatchTimeFieldWind` applies the same checked
inherited-binding and generation checks to native goal-field lifetimes (255
and 371). It uses the world's RNG and the selected record's `windSound`, not a
family-wide last-sound value. Its default overload likewise uses the owning
run's wind voice and UI channel. Audio failure latches both world and collision-owner failure;
subsequent dispatch cannot repeat the partially executed sequence. This handler
is an ordered-collision callback, not a separate family contact sweep.

Source: recovered `gml_Object_goalTimeField_Collision_objPlayer_n.asm`,
`gml_GlobalScript_soundPlayOL.asm` and `gml_GlobalScript_scrGlobalStuff.asm`.

Player transitions use `TimeStopAudio` with the session's music/wind voices,
the player's `SoundPlayState`, and its caller-local duck fields. Entry/exit
invoke `soundPlay(184/185,80,0,1)`, including resource cut, volume scaling and
underwater pitch. Exit's `bgmDuck(200,0.7)` writes its caller fields first,
skips audio in area 5, and otherwise changes gain only if the music is playing
and its current gain equals 1. The immediate 0.7 gain precedes the 200 ms ramp
back to 1; this is not an unconditional volume overwrite.

`TimeStopEffects::beginStepWithAudio` routes those operations through the same
synchronous shard/Create sequence. Checked audio queries distinguish failure
from a false playing/paused result. `StartupRuntime::beginTimeStopAudioChecked`
preserves that distinction for registered lifetimes: failed queries retain
earlier births/RNG but cannot write `bgmOn` or start the subsequent burst.
Legacy boolean-query entry points are for infallible query providers only.
The source contract is `objPlayer_n` Begin Step and the pinned `soundPlay` and
`bgmDuck` VM scripts; this adapter does not establish actual backend playback
or automatically install a transition in either normal game loop.

The registered transition's shake callback applies `scrSShake(2,2)` to the
admitted main-room camera before notifying the external effect callback.
Caller-local `shakeAmt/shakeDur` are assigned first. The request enables shake,
raises its amplitude to at least 2 and raises the actual camera Alarm 0 to at
least 2. Normal room alarm dispatch clears shake/amplitude on expiry. Camera
Step samples X then Y with inclusive integer ranges [-1,1] and [-2,2], consuming
four WELL words, and publishes the offset position without changing the Q12
easing accumulators. Alarm expiry leaves the prior published offset intact
until Step. A side-lock collision's explicit Y assignment replaces its vertical
offset without replacing X. This covers the admitted time-stop amplitude, not
arbitrary VM-valued shake arguments or the unimplemented camera modes.

`GameplayWorld::requestTimeStopCameraShake` supplies the same effect to the
native followed-camera path. Its compact Alarm 0 uses the shared countdown
semantics in the existing alarm phase; camera Step samples the same shared
shake implementation before computing display-scroll deltas and circular-buffer
origin changes. Retry/next-level reset clears the alarm. Active shake and its
alarm participate in native checkpoints. This is still the native fixed camera
slot, not a source-ID-registered controller in the complete mixed scheduler;
automatic player time-stop activation must call this boundary when connected.

`GameplayWorld::withPlayerTimeTransition` installs a borrowed transition only
for its enclosing tick scope. The shared player phase calls it with the actual
published player record, motion state and rounded world coordinates after
interaction observation, before later player work and room alarms. A rejected
transition retains earlier movement/publication/clock work but prevents later
alarms and camera RNG. Nested scopes fail the world; returning success from the
outer callback cannot hide that failure. The scope removes its callback before
returning. It neither supplies exact private binary64 player motion nor installs
field/effect/audio owners automatically; those are required to enable source
time-stop behavior in the default native loop.

## Ordinary Destroy versus storage retirement

The Windows `instance_destroy` registration at `0x52cd80` selects wrapper
`0x529b30`. With no arguments, the wrapper selects self and enables Destroy;
`0x528490` dispatches to `0x4fdf60` for the ordinary, non-delayed branch.
That routine guards reentry with bit `0x80`, invokes Destroy and Clean Up
events, then marks bit `1` and increments the destruction counter. It returns
without removing spatial/list storage. Downwell object 238 has neither handler.
The bit is immediately excluded by `instance_number`'s object-family scan
(`0x4fe49b`, mask `0x140003`) and the previously audited place-query filters.
`tools/verify-runner-destroy.py` pins these reviewed bytes to the source binary;
it is not runtime proof of generic destruction or managed/delayed branches.

Shard Step therefore publishes a destroy mark before its later population and
out-of-view work, while continuing the current event's final rounded x/y
setters. The stored record and generation remain valid for that continuation;
physical retirement still happens at the explicit cleanup boundary. A second
destroy request in the same event does not republish it. Failed marking is
terminal. Marking must not be confused with eager spatial removal or ordinary
activation changes.

The normal Draw callback (`0x50da30`, labelled `Draw`) invokes the marked-record
sweep `0x48bbb0` at `0x50da4a`, before drawing. The sweep scans the active room
list first, then the inactive room list, testing flag bit 1; it removes the
numeric-ID lookup and unlinks/releases marked records. In contrast, the
inter-phase reconciliation function `0x5108d0` handles activation, rebinding,
depth queues and other reconciliation; it does not call this sweep. The Update
function `0x50db60` has two additional cleanup calls only on its conditional
pre-update event path. Room transitions also force sweeps. These reviewed byte
ranges are pinned by the same verifier.

Room entry specifically broadcasts Room Start via `0x444980` at `0x5107f8`,
reconciles at `0x510813`, then calls `0x48bbb0` at `0x510820`. The broadcast
helper tests exclusion mask `0x100003` at `0x4449b8`, so a destroy-marked
lifetime remains allocated but receives neither Game Start nor Room Start.
These ranges are also pinned by `verify-runner-destroy.py`.

For first-level `startTimeField` (371), VM Create writes scale X, scale Y,
samples radius, then calls `instance_destroy`. Its resource inheritance has
no Destroy/Clean Up handler. The stored runtime publishes the mark through
its collision owner before setting `destroyRequested`; later room Create
events cannot reuse that slot. Room-entry cleanup retires it only after
reconciliation. Failed marking retains the unmarked record; failed cleanup
retains the marked record, and either failure stops subsequent execution.
An entirely unindexed fixture marks its local record; an installed owner
must provide destruction publication rather than silently retaining a
query-visible field. Other effect families still require their owned cleanup
dispatch. Native entrance lifetimes use the corresponding typed-store mark
and room-entry owner sweep; see [native ownership](runner-collision-bounds.md).

Phase-bound `TimeShardEffects` consequently retains marked storage through
alarms, Step and subsequent births until `retireRequested()` is called at an
explicit sweep boundary (or `clear()` for teardown). Marked entries no longer
count, animate, receive Step or draw, but cannot be reused just because the
next Create needs a slot. Unbound component fixtures retain eager cleanup;
production phase bindings always select deferred cleanup. The full frame loop
must still merge cleanup across object families in room-list order; a per-family
sweep does not establish that global ordering.

For an ordered sweep, `TimeShardEffects::retireMarked` removes only the selected
private-handle/source-ID pair. `StartupRuntime::retireTimeShard` routes a selected
world ID through its registered generation and effect owner before removing it.
Live, stale, foreign or failed selections stop the operation without recycling
the selected storage or retiring siblings. These are also the operations used
by the composed stored tick's pre-Draw cleanup.

`InstanceStore::cleanupDestroyed` traverses current room membership, active
before inactive, and dispatches marked records to their family owners. Removal
of the current record is allowed; births/rebinding or removal of other records
during this traversal are not. A successful callback must actually retire its
selected generation. Failure preserves the already-completed prefix and stops
before later families. `StartupRuntime::cleanupDestroyed` routes shards through
their registration and requires an explicit dispatcher for other families. This
boundary is exercised with the live collision owner.

`stepWithTimeStopEffects` keeps its borrowed registrations installed through
the normal pre-Draw cleanup. Destroy-marked shards (238) and time changes (211)
join the same room-order sweep as the other admitted families; there is no
separate shard-first or change-first sweep. Alarm/Step destruction retains both
records and local storage through Collision and End Step. Successful cleanup
then removes the numeric ID and local lifetime together. Rejection retains the
failed lifetime and every later candidate and stops the world without retry.
Tests interleave both families and compare retirement order to their unchanged
active-room traversal, including failure after an accepted retirement. This
composition does not install the real player transition/audio adapter into
the default SDL loop or supply the native global-ID stream.

## Time-change registration boundary

`TimeChangeEffects` supports a borrowed registration binding for object 211.
Its `createAt` narrows explicit private xx/yy once, requests a world ID before
Create, publishes initialized alarms after room-speed assignment and before
the synchronous pause. Registered callers cannot use the coordinate-less
component `create` shortcut. Registered alarms consult world eligibility,
publish changes and mark destruction separately from physical cleanup.
Failures preserve the completed prefix and latch the owner; reset cannot hide
a failed registration or retirement. `StoredTimeChangeRegistration` connects
these operations to actual room records, source resource defaults, shared IDs
and the maintained collision owner. `StartupRuntime` exposes checked Create and
alarm boundaries which latch failures into the world. The live-owner fixture
verifies the object-211 ID preceding a following shard burst, publication before
pause, final alarm value before destruction and failed-pause preservation.
The transition helper's private coordinates and automatic player dispatch still
need wiring; normal frame scheduling must not advance both record alarms and
effect-owner alarms for the same lifetime.

`GameplayTimeChangeRegistration` supplies an integer-only native record adapter
for the same `TimeChangeEffects` lifecycle. Records retain exact binary32 birth
coordinates, authoritative alarms, active/destroyed flags, a monotonic local
generation and a separate original numeric ID. The enclosing world must provide
resource-backed birth, mark-destroyed and retirement callbacks; birth must return
a valid numeric ID. Zero, duplicate or out-of-range IDs fail rather than aliasing
a local slot. Registered effects publish Create alarms before the pause callback.
Global alarm dispatch imports the already-decremented record counters, and
cleanup retires one marked lifetime without sweeping other object families.
Activation writes publish through the native collision/layer owner and update
the registered record's event-eligibility flag together. Room-list relinking
still waits for the ordinary reconciliation boundary. While inactive, neither
the mixed alarm snapshot nor direct registered dispatch decrements the counters;
reactivation resumes the existing countdown rather than rerunning Create.

The native collision owner attaches this adapter to resource-backed room and
layer membership. Its mixed combat alarm snapshot includes object 211; the
shared effect receives each fired event once after decrementing the registered
counter. Marked instances remain owned until Draw-entry cleanup. Retry and
next-level teardown clear the registered effects, preserving the external
effect owner's monotonic local lifetime sequence.

`createTimeChangeWithSourceId` requires an explicit original numeric ID; it does
not introduce a separate dynamic counter. Native actors with unrecovered IDs
remain unnumbered. Natural player transitions must not call this API with
slot-derived or guessed IDs: complete native global ID allocation and player
Begin Step wiring remain open. The caller must retain the effect owner and
room-speed variable until room cleanup or explicit retirement. This does not
change PAL pacing or implement the source synchronous pause in a backend.

The host native-world fixture verifies positioned publication before pause,
14 ordinary alarm updates, deferred cleanup and Retry with reused slots. The
shared field-reentry tape verifies registration, source alarm pacing and
next-level cleanup without inventing additional player movement or RNG calls.
Its IDs and direct alarm calls are controlled verification inputs, not evidence
of the complete original game's creation stream or phase ordering.

## Shard registration contract

`GameplayTimeShardRegistration` is the fixed-address native record adapter for
`TimeShardEffects::bindPhases`. Its owner must provide resource-backed births
with real numeric IDs, image/position publication, animation membership,
population queries, solid-mask queries, destruction marks and retirement. The
adapter retains marked records until the enclosing room cleanup selects them;
an event's final position writes still publish after its destruction mark.
The room scheduler decrements authoritative alarms before dispatch, and all
sibling counters are imported so effect publication cannot undo their countdown.
Local generations survive slot reuse independently of source IDs. Failed owner
callbacks retain the completed prefix and prevent later work/reuse.

`verify-gameplay-time-shard-registration` checks this adapter against the shared
effect implementation. The Amiga `verify-time-shard-registration-compile` target
instantiates the same operations under the real 68000 type/ABI definitions;
its assertions retain their expressions despite the freestanding compatibility
header. This is a compile check, not a target runtime test.

`GameplayCollisionOwner::createTimeShardWithSourceId` connects that adapter to
the resource-backed owner and `GameplayWorld` room animation, alarm and selected
cleanup callbacks. The effect bank is borrowed until room teardown; Retry clears
registered effects before reconstructing the world. Animation eligibility reads
active room-list membership, while population uses the instance-number flag mask
`0x140003`, excluding destruction marks before physical retirement. Birth accepts
an explicit source ID; it does not allocate a family-local substitute. This
boundary does not enable automatic player bursts or schedule the complete
Step/collision/Draw sequence; those remain required by the live integration.

For synchronous transition-script births, `withTimeEffectSourceIds` borrows an
enclosing source-ID allocator for both object 238 and object 211. Allocation
occurs at registered birth, before resource-backed publication and child Create;
it is not a batch reservation. Other synchronous callers can consume the same
stream between these births. The scope rejects nesting, preserves consumed IDs
on failure, and drops its callback on return. Explicit-ID entry points remain
available. Neither route synthesizes IDs from local generations. This bridge
does not establish the complete native global allocation stream, and does not
by itself admit the player transition or its child phases.

`GameplayCollisionOwner::withPlayerTimeStop` composes that allocator with the
existing native player Begin Step hook. It reads the owner's published position,
converts the current portable motion representation to the shared pose, and uses
the registered parent-field queries for both boolean contact and numeric-ID
selection. Supplied room metadata controls the strict side exemption. The shared
transition uses the world's RNG; its shard/change banks and room-speed variable
are also borrowed by the room animation, alarm and cleanup dispatchers. A live
bank or controller clock cannot be replaced with a different owner's state.
Camera shake reaches the world camera before the caller's effect callback.
Audio queries, synchronous pause, oxygen and other effects remain checked caller
services, not implicit no-ops. The scope does not schedule shard ordinary Step
or propagate time-stop to every actor; production admission still requires those
phases and the complete numbered creation stream. Portable pose conversion does
not claim original binary64 player-motion equivalence.

`GameplayCollisionOwner::stepTimeShard` accepts one selected shared lifetime and
borrows the mask pack only for that event. Its solid-parent query merges all
registered descendants in registration order, including compact box ownership
and retained terrain, before the normal population/tree-height preparation and
mask traversal. It does not run separate box/wall/platform collision passes.
The shared effect Step uses the world RNG and real alarm counters, and publishes
its final rounded position even after a destruction request. The caller supplies
the source-published-view retirement predicate; camera publication and global
Step ordering must be established by the enclosing scheduler. The overload
without a retirement callback uses `GameplayWorld::timeShardOutsideView`: the
rmMain view snapshot captured before camera movement, with strict `< top` and
`> top+320` tests on the effect's old published Y. It does not use the newly
scrolled display coordinates or the private post-integration `yy`. This query covers
registered lifetimes, not unimplemented solid families in the scene generator.

The explicit registered-player-contact boundary uses the world's RNG and
publishes the changed alarm-2 cooldown immediately. It checks event eligibility
before consuming random words; inactive or `noTouch`-guarded contacts do no work.
A failed velocity calculation or cooldown publication is terminal, preserving
any already-consumed RNG and velocity assignments rather than retrying them.
The ordinary collision traversal still needs to invoke this boundary.

Registered shard alarm and Step dispatch check the generation-resolved world's
active/destruction state before mutating timers, querying contacts or consuming
RNG. An inactive lifetime stays allocated and can resume later; a failed lookup
is terminal, not a successful inactive result. This implements the supported
active/destruction parts of the runner's `0x100003` event filter. Animation
eligibility is deliberately separate: the original pre-Begin-Step animation
loop traverses room membership, which can differ from pending activation flags.
Registered shard animation therefore resolves active room-list membership via
its generation-checked record, separately from event eligibility. The global
animation traversal and cross-family ordering still need integration.

Phase-bound shard Step obtains `instance_number(fxTimeShard)` from the world
collision owner, not its private effect pool size. The object-resource query
walks ancestor-inclusive registration and filters flags `0x140003`, matching
`0x4fe49b`. Raw registration counts used for collision-search preparation remain
unchanged. Destroy exclusion is immediate while storage survives; inactive and
other source-excluded records do not contribute. Missing/failed ownership stops
Step before later out-of-view work. Tests include two independently registered
shard pools sharing one world and a population-triggered destruction which a
pool-local count would miss. Negative and instance-ID selector branches are not
implemented by this object-resource entry point.

`TimeShardEffects` distinguishes its monotonic phase/lifetime handle from a
world-assigned numeric instance ID. A borrowed registration adapter receives
the narrowed birth coordinates before object-238 Create consumes RNG, then the
completed Create state before `scrTimeShard` overwrites velocity. Resource
defaults and global ID allocation belong to that adapter, not to the effect
store. The burst caller retains the source ID; effect access uses the private
handle. Adapters must outlive the bound store and must not mutate it recursively.

Removal and room cleanup call the adapter before recycling storage. Partial
registration, post-Create publication or retirement failure latches the store:
completed callbacks and RNG draws are never replayed, and failed registrations
are not silently recycled. Reset cannot clear this failure. Numeric IDs must be
positive signed-32-bit values and unique among this store's live registrations.

`StartupRuntime::timeShardRegistration` supplies a fixed-address borrowed bridge
to its actual record store, global numeric-ID allocator and collision binding.
The world burst entry point uses its own RNG and repeated parent-field lookups.
Object-238 records receive resource defaults before Create, and the bridge
publishes Create's image index and alarm fields afterward. Retirement checks
both the effect key and the store generation before erasing the record. The
bridge and effect store must remain alive together; shards must retire before
room teardown. Neither adapter copies nor numeric-ID-only lookups are allowed.

Phase-bound shards publish animation indices, alarm counters and final Step
coordinates through the same registered lifetime. `stepTimeShards` checks the
bridge belongs to the calling world, uses the shared RNG, and sends the source
solid-family (56) probes through the maintained collision owner. Queries retain
the old published caller position until Step's final rounded x/y writes; stopped
shards still move while image speed is zero and lifetime counters are extended.
Publication/query failures are terminal and cannot be reset into a retry.

`dispatchTimeShardContact` routes recovered code 2081 from a collision-event
frame to this registration, validating the self/other generations and exact
object/event binding before RNG consumption. The registration additionally
checks the real record address, effect handle and numeric source ID; missing
or recycled effects fail instead of becoming successful no-ops. The current
entry point is specifically for the main-room player's movement owner.

The native counterpart is `GameplayCollisionOwner::dispatchTimeShardCollision`.
It validates the resource binding and current player/shard handles before using
`GameplayWorld`'s private player velocity signs and shared RNG. Both registered
contact adapters import the current room alarm counters before an eligible
contact publishes alarm 2; the collision script does not reset alarms 0 or 1.
This is a selected-event boundary, not a substitute for ordered pair collection.

`gml_Object_fxTimeShard_Collision_objPlayer_n.asm` reads `sign(other.xsp)` and
`sign(other.ysp)` and multiplies each by a separate `random(2)` draw, then sets
`noTouch` and alarm 2 to 5. Only signs cross the live motion adapter: horizontal
Q8 and vertical fifth-Q8 numerators retain them without rounding through pixel
or engine velocities. This does not establish original precision for the
player's underlying motion. Live tests cover left/right ground jumps, upward
velocity, cooldown RNG suppression and stale/wrong-event rejection.

The automatic transition schedulers do not call these entry points yet, and the
native world's heterogeneous registry still needs its corresponding binding.
Global ordering across effect families and exact synchronous Destroy boundaries
still need scheduler integration. World integration tests cover shared ID/RNG,
resource-backed probes and phase publication, not the remaining global dispatch
or presentation properties.

The stored collision adapter provides an explicit `birthAt` path for these
dynamic effects. It receives the already-narrowed binary32 coordinates and
publishes the spatial birth at that position once, without first inserting at
the legacy integer drawing coordinates. Nonfinite input is rejected before
attaching the lifetime. Existing room/terrain `birth` calls retain their integer
contract. A missing positioned-birth binding is a failure, not an integer
fallback; callers must preserve this boundary when connecting shard registration.

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
`meetingVm` exposes the boolean wrapper with the same query-time mutations;
it does not replace that search with an independent overlap scan.

`StartupRuntime` accepts a borrowed `InstanceLifecycleBinding` at construction,
before the initial room births. Packed/fallback room loading, deferred creation,
room/reset/out-of-view retirement, activation requests and scheduler membership
reconciliation pass through this binding. Rejected callbacks latch failure and
stop later work; a rejected reset retirement must not create replacement births.
The binding's owner must outlive the runtime, and a bound runtime must not be
copied while sharing that owner. Default core construction leaves the binding
empty for unindexed fixtures. The SDL executable instead constructs a
`StoredWorldCollision` before `Game`, passes its binding before room births, and
attaches the runtime before the first update. It loads DWSC collision metadata
and DWSM masks alongside the other generated resources; invalid ownership is a
startup failure, not a silent fallback. `make run` generates those packs.

The owner uses generation-validated slots to resolve current stored records,
not borrowed record pointers that room compaction could invalidate. It cannot
be copied or attached to a second runtime. Its lifecycle callbacks maintain
the graph and spatial index across room/reset/activation/movement/resource
writes; there is no per-frame reconstruction. The production SDL profiler uses
the same owner. Native-world resource registration, separate effect lifetimes,
remaining source writes and routing other collision decisions are still open.

Object identity changes use `InstanceStore::bindObject` with the collision
adapter's `rebind` callback. The callback validates the old handle/object pair
before applying the new object's membership and serialized mask/depth/sprite
defaults. Position and transform fields survive this operation. The store then
publishes the new object ID; it does not copy the new default fields into the
legacy record. Callers must route those legacy field writes separately before
using that record as presentation state. A failed external rebind is terminal,
not permission to continue with the old store identity and a changed owner.

## Player and shard inputs

`make verify-live-collision-lifecycle` attaches the real object-family graph,
sprite collision metadata and spatial owner to the stored runtime's natural
seed-2 gameplay tape. It checks published player/terrain/enemy positions and
activation flags against live records without rebuilding the index. Player
updates call the maintained `place_meeting` owner through `bindQueries`, using
selector 255 and the old published player position before position publication
and shop processing. Goal-stop and death skip the query itself. A rejected
query latches failure before clock accounting or later player work, rather than
falling back to a miss. The native adapter exposes the same checked boundary;
an empty binding explicitly selects the shared unindexed field adapter.
After the tape the test runs precise `instance_place`
queries over generated walls, resolves results through live generation-checked
ownership, checks temporary player-position restoration, then resets the room.
The query adapter accepts a read-only `resolve(handle)` view instead of requiring
a copied or publicly exposed instance container. The clock tests also cover
an overlapping exit, query failure, room retirement and a later-level entrance
descendant. They are not an original traversal-order oracle. Native gameplay
still uses the unindexed adapter until its heterogeneous world records have
resource-backed registration; remaining collision-family scheduling is required.
Motion remains a combined phase before this clock boundary; preserving old
published query coordinates does not by itself prove full source event order.

`bindInstanceQueries` adds the identity-bearing `instance_place` operation.
`InstancePlaceResult` carries the source numeric instance ID, a generation key
and promoted published binary32 coordinates, not a pointer into compacting
storage. A miss is canonical `noone` (-4); failure leaves the caller's result
unchanged and the world adapter latches failure. Missing ownership or a
malformed successful result is not an approximate overlap fallback. The stored
owner runs the maintained search, resolves its selected lifetime, and reads its
published position; it does not select the nearest or newest field independently.

Both worlds expose `playerTimeFieldInstance` for the source parent selector 82.
The shard script must call it again for its second lookup, even when the first
query hit. Integration tests feed those real lookups into `TimeShardBurst`,
including overlapping fields, fractional published positions, room replacement
and the start-field descendant. This connects the lookup contract, not the
whole shard scheduler: global shard birth identities, time-stop transition
dispatch and full original traversal-order verification remain required.

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

## Exit children and Create order

### Room-entry descendant

`startTimeField` (371) is a child of `goalTimeField` (255), not an ordinary
object-82 field or an exit trigger. The main room places instance 101097 at
(240,192), using sprite 616. Its own Create (code 2731) does not call inherited
Create: it writes X scale 2, Y scale 2, samples `128+random(2)`, then destroys
itself only when both `global.area==1` and `global.level==1`. The random draw
therefore occurs even on the first level, before destruction. Discarding the
instance during room decoding would change the source RNG stream.

On later levels its inherited family membership makes it eligible for the
player's goal-field clock query as well as parent-time-field queries. Its own
Draw (2732) resamples the radius unless `pTimeStop`, and draws the same three
outline circles as the exit field, with all three circles outlined. It inherits
the goal-field player Collision event, including the guarded wind-sound choice.
An exit-only contact adapter cannot represent this room-entry descendant.

Room-load Create publishes each scale before updating the stored record and
retires the first-level collision identity only after sampling the radius.
Rejected publication stops the sequence and preserves earlier writes. Shared
`RunSession` state supplies later-level goal/parent contacts and immutable
outline snapshots to both renderers; room teardown retires the field. Native
entry checkpoints include the same ordered Create draw (331 words for seed 2,
325 for seed 4). This is not a special exclusion inside collision queries:
leaving the placed record unhandled would produce a unit-scale phantom on the
first player update. Wind-audio collision dispatch remains a separate integration
requirement; shared contact/outline tests do not prove its RNG or sound ordering.

### Exit-trigger children

`objLineToResult` (306) calls inherited `objLineToMain` Create: set the custom
trigger variable `active=0`, center X at half the room width, and synchronously
create `roomEndSetter` (74). That child's Create finds camera 63 and sets its
room end to `y-176`. The derived event then creates `goalTimeField` (255) at
`(x,y+32)`. Its Create writes X scale 2, then Y scale 2, then samples radius
`128+random(2)`. Neither child has substantive PreCreate code. The trigger's
custom `active` variable must not disable runner collision membership.

The stored runtime flushes earlier builder commands before creating the exit,
then registers these children synchronously through separate checked queues.
Birth publishes serialized resource defaults at unit scale; ordered scale
callbacks update the collision owner before the shared radius draw. The shared
`RunSession::createLevelExit` callback boundary stops later children and RNG on
failure without undoing the preceding camera write or completed births. Both
worlds use that shared camera/radius sequence; native-world collision-registry
registration is still required. An empty publication callback does not install
such a registry.

The live collision gate checks the three instance identities, both scale writes,
the complete post-Create RNG state, and resource-backed direct/parent-family
queries beyond the unscaled radius. Fault cases reject either child birth or
either scale write. These are controlled exit fixtures, not natural-end-of-level
or original traversal-order verification, and the diagnostic query still does
not drive the player's clock.

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

This establishes newest-first equal-depth order for the legacy room Draw
traversal only. The pinned build selects GMS2 layers; the
[depth-update audit](projectile-motion.md#selected-depth-update-path) shows that
layer migration is separate from this room list. Selected layer Draw order
must still be verified before treating these observations as the original
game's complete painter order.

The current port draws unchanged equal-depth dynamic fields newest first.
An exit field must be interleaved at its creation boundary, not unconditionally
drawn before or after all ordinary fields. `OrdinaryTimeFields::advanceDraw`
accepts the ordinary-field count at exit-field birth for this merge. The
callback must not create fields or mutate their owner. Inactive/hidden fields,
depth changes, and interleaving with other Draw-event families require their
own scheduler handling; this is not proof of complete global Draw ordering.
`tools/verify-runner-field-draw.py` pins the reviewed executable bytes, not an
execution trace of the original game.

The active room list is also the animation list, traversed in reverse: Draw
starts at room `+0x84` and follows instance `+0x150`; animation starts at room
`+0x80` and follows `+0x14c`. `RunnerRoomMembership::visitDrawChecked` exposes
this reverse traversal without sorting or allocating another list. Reactivation
at `0x48d69c..0x48d760` reinserts before equal-depth Draw peers, so numeric
birth-ID sorting is not a substitute after activation cycles. Callers must
still apply Draw eligibility and preserve separate custom Draw paths. The
stored garden renderer uses this traversal. Registered gameplay enemies,
bullets and object-219 effects are merged through it on both backends, with
the damage-number/corpse-expiry depth -1000 boundary interposed. The native
owner supplies source depth alongside a resolved family/slot/generation, and
Retry preserves that binding. Retained terrain, shops, field outlines and
controller surfaces still have separate composition paths: this is not yet a
complete ordinary-Draw scheduler across every family.

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
source ordering evidence. A separate identity slot survives compaction;
`resolveBulletLifetime` also resolves marked shots until Draw cleanup. Native
object-261 births are resource-backed, then receive the selected sprite. Existing
shots publish their pre-Begin-Step image phase; first Step publishes the sampled
angle before impact queries, and movement publishes fractional positions.
Impact/expiry excludes the record from queries immediately, retains its lifetime
until `prepareDraw`, and retires it before slot reuse. Room exits and Retry clear
ownership, with terminal failure propagation at each publication boundary.

`ProjectilePublication` narrows the current Q20 position and Q32-turn direction
without discarding fractions at the drawing-pixel boundary. This is an adapter
for the existing approximation, **not** evidence that projectile motion matches
the Windows setters/trigonometry. Live projectile hits still use the old adapters: source
motion precision, collision-event/mark timing and muzzle-effect creation order
still need integration. The
existing enemy-first impact adapter is not replaced by this ownership work.

The SDL stored world retains object-261 records in `InstanceStore`, linking
each compact-array shot to its generational handle. Birth publishes original
resource defaults before the selected sprite/image assignments. Position,
angle and animation publications use the same supported shot state as native;
no pointers into compacting instance records survive callbacks. Consumed shots
are marked immediately and removed in room-list order at the frame adapter's
pre-Draw boundary, after End Step. `cleanupDestroyedIf` selects this owned
family without silently erasing marked records belonging to external effect
owners. Full cross-family cleanup/animation and source collision dispatch are
still separate integration requirements. Level-clear entry retires the shots,
and room reload/Retry invalidates stored handles through normal lifecycle calls.

Native exit creation also registers the object-306 trigger and object-74
camera-limit child in `GameplayExitControllers`, before the object-255 goal
field. The VM's `objLineToResult.Create` calls inherited `objLineToMain.Create`
first: center X, create 74 (whose Create writes camera `roomEnd`), then create
255 at Y+32. Publication precedes each child's camera/radius side effects;
failure stops subsequent births and RNG. The trigger's script variable `active`
is distinct from runner activation and must not suppress its registration.
Original resource sprite/mask/default transform properties apply to both.
Room changes retire these records; Retry preserves the generation cursor.
This records the exit subtree's order, not full room-family creation or cleanup
order, nor Windows numeric instance IDs. The ordinary exit gameplay state still
belongs to `LevelExit` and `MainCameraMotion`; selectors remain gated while
other required families lack registration.

Native enemy handles pair an ownership slot with the monotonically increasing
enemy lifetime key. The ownership slot is independent of the compacted traversal
index: retiring a prefix record must not invalidate surviving handles. Reusing
the freed slot assigns a new key, and lookup checks both parts. Dormant enemies
remain resolvable; dead or retired enemies do not. Retry and level changes clear
slot ownership without rewinding the lifetime counter. These handles remain
world- and family-local; they neither establish original traversal order nor
replace registration and synchronous retirement in the shared collision owner.

`GameplayCollisionView` maps player, enemy, breakable and terrain local keys into
disjoint collision slots (1 + 64 + 512 + 2048). It borrows the actual `GameplayWorld`
and resolves records on every call, so compaction cannot leave cached pointers.
Generation validation still belongs to each store. `resolveEnemyLifetime`
does not apply active/pending filters; an inactive retained lifetime must reach
the collision routine before that routine decides whether to exclude it.
The legacy `findEnemy` gameplay eligibility behavior is unchanged.

This view allocates no record copies and assigns no Windows instance IDs. Its
slot ranges do not imply source traversal order. Fields, projectiles
and other effect stores still need their own lifetime registration, and typed
resource/coordinate adapters must connect the view to the maintained spatial
owner before normal native queries can use it. Tests exercise real records,
inactive retained enemies, compaction, Retry and generation reuse, rather than
only round-tripping key arithmetic.

`GameplayCollisionOwner` connects those typed lifecycle bindings to
`RunnerSpatialInstances` and the recovered object/sprite packs. Birth maps local
enemy/box coordinates through the native (240,176) origin, preserves player
world coordinates and inactive flags, applies compatibility creation depth and
the available native sprite/mask/transform fields, and publishes initial bounds.
Installed move, sprite, mask, image-index, transform, activation, membership and
retirement callbacks subsequently mutate the same owner; no per-frame rebuilding
or copied `InstanceRecord` registry is involved. Attach must identify the exact
world whose player was registered during construction. Later callbacks validate
the actual record address as well as its family and generation.

The production executable does not install this owner yet. Its checked terrain
shape/point query interface accepts only object selectors 60 and 93 and requires
the original mask pack. It shares the runner search, compatibility mask tests,
VM coordinate narrowing and temporary-position restoration with the stored-world
path. Family coverage is checked once against the resource graph; unknown
descendants reject the query rather than silently disappear. Other selectors
remain unavailable while fields and remaining families are absent.
Native birth hooks currently observe initialized actor
state; full source PreCreate/Create interleaving and omitted resource writes
still need migration. Tests bind real worlds and resources and cover dormant
births, published movement/art, compaction, clear/Retry, unrelated-world rejection
and copied-record failure. This establishes ownership at existing boundaries,
not complete native collision fidelity or an A500 memory-fit claim.

Generated walls (60), platforms (93) and invisible wall subtype (99) can publish through an optional
`GameplayTerrainInstances` store passed to `GameplayWorld` at construction.
`createGeneratedInstance` appends after the backend terrain callback succeeds,
before the next generated command. Records retain room coordinates and active
state; coincident cells remain separate instances. Breakables still use their
existing owner and are not duplicated here. Retry and level advance retire
terrain before reusing slots, retaining the monotonic generation sequence.
Callback/capacity failure is terminal and preserves the completed prefix.
Subtype 99 overrides Create and End Step with empty bytecode and retains
serialized sprite 46, mask -1 and engine animation defaults; it must not execute
the parent wall's tile setter. Its PreCreate ancestor chain only calls inherited.
The terrain-source verifier checks this distinction. Native query fixtures
compare 3249 unit-terrain points against resource bounds and mask pixels, check
caller exclusion/restoration for shape queries, subtype membership, and immediate
inactive/destroyed exclusion. They do not prove full live wall scheduling,
fractional original-runner raster mode or later-area rendering.
The store is separately allocated: normal unbound worlds gain a pointer and
room-load guard, not a terrain array. The optional collision owner includes the terrain array
and expanded spatial capacity; its eventual A500 residency must be measured.
This is registration at the current handoff, not original PreCreate execution.
After registration, `terrainCreateProperties` publishes the selected area tile
to `sprite_index`, then frame 23 for thin platforms, then stops animation.
The same property sequence runs in stored-world Create codes 985/1090. Native
sprite/frame setters use the terrain's lifetime binding; the image-speed value
belongs to its record, not the collision-shape properties. Current live callers
select the normal area's tile and disable glitch mode. The helper preserves
the wall glitch branch's additional five-way choice, but live glitch state and
temporary controller/boxFlash tile overrides are not yet connected.
Wall End Step adjacency frames, surrounded-wall tile replacement and
per-instance activation/retirement still need their source boundaries before
enabling live terrain queries. Drawing a neighbor-derived tile does not publish
that frame or retire the corresponding collision instance.

`WallEndStep.h` implements code 986's control flow with explicit external-call
adapters. Its first event issues four shape queries, applies pillar remapping,
and conditionally issues four distinct point queries for corner lighting. A
surrounded wall is marked during that event; only the following event adds
background 739 (source 48,48; size 16,16; position x-8,y-8; depth 100), requests
destruction and then, when requested, deactivation. Destruction must remain an
engine request during the callback: the VM continues executing the event.
Area-five out-of-view runs before either branch. Area-four limboShard choice and
area-zero repeated frame writes preserve the VM's order and RNG calls.

The handler is not installed in either production scheduler yet. It requires
source-equivalent shape/point queries, retained background-tile ownership and
destruction/reconciliation adapters; boolean terrain occupancy is not a generic
replacement for these operations. `verify-wall-end-step.py` interprets the
pinned assembly's integer/control-flow subset against the portable handler's
two-event traces, checking complete modeled state and external-call order.
Query answers, out-of-view and engine side effects are fixtures in that test;
it proves control flow, not their implementation or end-to-end wall fidelity.

The optional native collision owner has a `wallEndStepEvent` adapter for one
scheduler-selected terrain handle. It runs the same portable handler against
resource-backed shape/point queries, publishes image-index writes through
collision ownership, and retains the record through destruction/deactivation.
Checked binary64 additions form room-coordinate query/tile/child positions.
Tile publication, limboShard Create and area-five out-of-view policy are explicit
external callbacks; no generic successful stubs are installed. Inactive or
destroy-requested records are skipped and subtype 99's empty override is honored.
An event failure latches terrain/owner validity without undoing its prefix.
Real-resource 3-by-3 wall fixtures check the first-event surrounded state, the
second-event retained tile at `(x-8,y-8)`, deferred destruction, unchanged cavern
RNG, failure before destruction, and tile survival after ordered cleanup.
This is per-event integration, not a claim that production frame dispatch,
area-four limboShards, area-five retirement or global creation order is complete.

The area-four child is object 379 `limboShard` (sprite 91, depth 0), not a
decorative flower. Its synchronous Create 2769 consumes `irandom_range`,
seven-way `choose`, `random_range`, two more `choose` calls and `random(7)` in
that order. Step 2770 uses time-stop state and solid queries; Collision 2771
targets parentPBullet (89); Draw 2772 has a flash guard. A callback which only
records its birth does not reproduce these effects or the following RNG state.

`StoredWorldCollision` has the corresponding one-handle adapter for
`StartupRuntime`, sharing the portable event handler and resource queries.
Image writes, destroyed exclusion and deactivation go through checked lifecycle
publication; marked wall retirement is an explicit typed cleanup operation.
The adapter reads source positions from the maintained collision owner, not
`preciseX/Y` actor-motion caches, which collision setters intentionally leave
unchanged. Its real-resource fixture moves nine room walls through engine
setters, verifies both events and tile coordinates, then deactivates and retires
the marked lifetime without losing its retained tile. This adapter likewise
does not enable code 986 in production dispatch or reconcile legacy occupancy
caches yet. `drawWallPresentation` is shared by SDL and native terrain
composition: once End Step has selected a frame, it uses published sprite/index
instead of recomputing neighbors. Destroy-requested/inactive walls and subtype
99 are not drawn. Walls with no executed End Step retain the transitional
legacy lookup. Native composition uses registered terrain records when supplied,
so retired records do not reappear from a still-populated occupancy cell. This
does not yet make the production collision cache reflect wall retirement.
Native record-backed composition scans the bounded store for each requested
strip; production installation still needs measured row indexing and cache
invalidation for selected-frame changes, not a full-frame repaint workaround.
The terrain store offers a borrowed checked presentation observer for that
invalidation. It reports room-coordinate wall centers after accepted first
frame selection, changed published frames, visibility changes and destruction.
It includes old/new sprite IDs (-1 for hidden), so the backend can invalidate
both footprints: most tile/glitch sprites are 16-by-16 at origin 8,8, but
resource 70 is 32-by-32 at origin 16,16. The source verifier checks that exception.
Unchanged frame writes and events, hidden walls, post-destruction deactivation
and physical cleanup do not report redundant damage. A rejected notification
is terminal but preserves already-published simulation/collision changes.
Birth drawing and whole-room teardown remain owned by generation/scene reset;
the observer does not replace them. `AmigaWallDamage` accepts these notifications
and unions adjacent/overlapping world-row bands without filling clean gaps. Its
source-specific footprint table rejects unknown artwork; out-of-range coordinates
and capacity fail explicitly. The native renderer uses this queue for newly
retained wall tiles: invalidate terrain raster, prefetch and out-of-view backing
first, then repair each visible band through the existing painter/dual-display
debt path after camera movement. Only successfully repaired bands are acknowledged.
Scene reset discards old-room debts. The registered-owner event fixture feeds
this same queue; production installation of that optional owner/observer is still
required for live frame and visibility notifications. Sparse union/partial-failure
tests establish queue behavior, not complete end-to-end retired-wall cache
fidelity or frame-budget fit. The default queue holds 2048 bands (16 KiB).

The stored collision owner's production lifecycle binding exposes both shape
queries and a separate `positionMeeting` point query. The latter has no caller
handle, forwards binary64 VM coordinate words unchanged to the resource-backed
owner, and publishes its boolean result only on success. An absent binding is
an error, not a successful miss. Older partial owners must explicitly opt in;
they do not acquire a terrain-grid fallback. Live authored-wall tests exercise
the binding against the original collision resources without moving instances
or consuming RNG. This query connection does not enable wall event scheduling
or establish the native world's still-incomplete heterogeneous ownership.

The compatibility `tile_add` script finds the first existing layer with the
requested depth, or creates one, then calls `layer_tile_create` with the source
patch and world coordinates. The wall does not retain the returned tile ID.
`tileCavern` (sprite 739), source `(48,48,16,16)`, is 256 opaque-black pixels.
`verify-wall-background.py` checks the pinned input, export/tool identity,
reviewed helper assembly, atlas hash and each pixel. This exact patch may use
a black fill; other tiles must not inherit that specialization.

`WallBackgroundTiles` retains these patches independently of instances, with
unrounded binary64 positions and depth 100. Its region painter accepts an
explicit raster policy and emits only clipped rectangles, allowing retained
cache repair/newly exposed strips rather than mandatory full-frame repainting.
It preserves duplicate additions and fails capacity/arithmetic errors without
discarding the accepted prefix. This is not a general layer/tile-ID engine.
`RunSession` owns a bounded 2048-patch store (about 32 KiB of coordinates),
cleared on run reset and accepted level-complete room retirement. Checked
append failure latches the session effect failure; rendering does not mutate
the store. The stored-world digest includes its coordinates and validity.
Both world adapters expose this room-owned store. SDL paints the patches after
direct depth-10000 currency and before ordinary walls; Amiga's terrain painter
includes them in fresh terrain composition. New native patches invalidate only
their affected terrain rows, using the existing backing repair and display-buffer
debt paths. Painting failure must not acknowledge those rows as clean.

The wall End Step callback is still not dispatched in either live scheduler;
normal gameplay therefore does not yet populate this store. Populated fixtures
check SDL wall-relative ordering and state immutability, native clipped strip
composition across ring wrap without changing HUD planes, and room retirement.
These do not prove full cross-family depth ordering (especially native direct
effects), source fractional raster policy, memory fit or live wall retirement.
The background-tile integration alone does not authorize retiring walls.

Room terrain enters through `GameplayWorld::loadRoomTerrain`, shared by native
startup/re-entry and scene tests via `AmigaGameplayGeneration::loadRoom`.
The room pack is serialized from `instanceCreationOrder`; filtering that stream
preserves terrain-relative order, without claiming original cross-family order
(the native player still exists before this terrain-only handoff). `rmMain` has
240 authored walls, all unit scale/zero rotation/default image properties.
Records retain the original instance ID and creation-code reference. The three
wall instance-creation scripts (`gml_RoomCC_rmMain_1/2/3_Create.asm`, codes
676–678) set `isPillar` to -1/+1/-1. `wallRoomCreation` applies those assignments
after object Create in the stored scheduler and in the optional native terrain
store. Wall records retain `WallEndStepState`; Create resets its consumed
bookkeeping fields before the room-specific pillar override. The stored-world
state digest includes those fields. An unsupported terrain room-creation code
fails without undoing its already-published birth/Create prefix. This is the
wall-specific room-code path, not a general VM room-script interpreter; it does
not yet drive native drawing, adjacency or retained background replacement.
The native terrain store has a separate checked destruction request: it publishes
the owner's destroyed collision-exclusion bit, then marks the record without
erasing it or changing its handle. A following activation callback can therefore
still resolve the same record, as required by the wall event's destroy-then-
deactivate sequence. Failed publication latches the terrain store; previously
accepted effects remain, and later callbacks do not execute. Repeated accepted
destruction requests do not republish. Room clear retires those retained records
normally. `retireMarked(handle)` accepts a handle from the world's cross-family
cleanup traversal, retires collision ownership before freeing storage, and
preserves surviving record addresses and birth-relative iteration order. Reused
slots receive fresh monotonically increasing lifetime IDs. A compact slot-order
array costs 8 KiB at capacity 2048; it is not a room traversal oracle. Failed
retirement leaves its record available, while successful room-clear prefixes
are removed from the reported population even if a later callback fails.
`GameplayCollisionOwner` also retains room memberships for its registered
player, enemy, breakable and terrain lifetimes, using recovered creation depths
and the shared `RunnerRoomMembership` algorithms. Explicit reconciliation scans
inactive then active room lists and publishes object-family membership changes;
activation flags change earlier at the request. Explicit cleanup scans active
then inactive entries and dispatches each marked lifetime to the supplied typed
retirement callback. A callback that reports success without retiring the
record is a terminal error. Successful earlier retirements survive failure.
These callbacks are not yet installed at the live native scheduler boundaries,
and missing object families/creation ordering still preclude full runner order.
Dynamic depth writes also need publication when those live paths are added.
The room entries and keys add bounded storage proportional to native capacity;
expanded-emulator memory fit is not stock A500 fit.
Generated records have no authored ID. Their local lifetime keys remain separate
from source IDs. Unsupported room transforms/pre-create properties are rejected
when identity tracking is installed, not silently flattened. A room-load guard
rejects duplicate or partially failed loading; Retry/next-level reset it only
after the previous terrain ownership is retired.

The enemy binding includes the explicit mask setter: flying bat Step copies
`sprite_index` to `mask_index` after choosing 153 (flight) or 154 (hit), as shown
in `gml_Object_enmBat_Step_0.asm` blocks 59–63. A live bullet-hit test exercises
all three stun ticks and recovery, checking the maintained mask independently
of artwork. Falling back to the current sprite while leaving mask ID -1 would
miss this state change even when a single-frame collision looked equivalent.

The shared immediate bullet/stomp response adapters provide a checked retirement
callback after rewards and before clearing the active flag or publishing defeat
and bounce effects. Failure retains earlier damage/reward mutations and stops
contact traversal; callers must latch it rather than retry. The host scheduler
routes fatal bullet/stomp responses and snail expiry to its checked activation
binding with `false` before clearing the record's active flag. This removes
collision visibility while retaining the stored lifetime until ordinary erasure;
it does not set `activationPending` as a dormant deactivation request would.
The host's projectile and player-box contact paths also publish this checked
inactivation before clearing object 92's active flag. A rejected publication
leaves the box active, does not increment the destroyed-block or sound counters,
and stops subsequent simulation. This closes stale collision visibility at the
existing removal boundary; it does not implement the original box's delayed
`wallHp <= 0` Step, synchronous smoke/flash/debris creation, or chain reactions
(`gml_Object_objBox_n_Step_0.asm`).
Native breakables use fixed storage slots paired with monotonically increasing
generation keys. Their borrowed lifecycle binding publishes births, contact
inactivation, out-of-view retirement, and retirement before replacement, Retry
or level change. Destroyed boxes retain their inactive stored lifetime until
clear; out-of-view retirement invalidates the key immediately. Retry preserves
the generation counter and binding. Rejected callbacks latch failure without
replaying earlier successful notifications. Coordinates in these records remain
local to the gameplay origin (240,176); a resource-backed adapter must map them
to world coordinates and object 92. The default binding is empty, and these
family-local handles alone do not establish global runner ordering or install
the live spatial registry.
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
index writes outside the supported enemy adapter, other sprite assignment sites
and exact source setter scheduling remain required before full query installation.

`bindAnimation` also forwards the current enemy Q8 image phase after pre-Begin
Step advancement, at explicit bat/frog/snail hit resets, and after final Step
sprite/frame selection. Its conversion narrows the nonnegative integer phase to
binary32 then adjusts the exponent by eight, retaining fractional mask frames
without a target floating-point division. This publishes the existing rational
animation approximation, not recovered runner sequence timing. The owner setter
corresponds to the reviewed `0x432170` store: it changes only `imageIndex`, leaving
bounds flags, dirty queues, cached frame and sequence timeline untouched. The
later animation owner remains responsible for sequence reconciliation.

`bindTransforms` additionally forwards the shared snail attachment's `scaleX`
setter before subsequent motion queries, and its nonzero-velocity `scaleY`
setter after movement but before final position publication. These boundaries
follow `enmSnail_Step_0` VM assignments; a time-stopped attachment still sets X
scale, but does not set Y scale. Rejected publication is a terminal failure, not
snail death: both schedulers check failure before interpreting the motion result.
Crawler Step publishes `imageDirection * 90` as angle after its X/Y setters;
Alarm 0 changes movement but does not publish an angle. Rejection retains earlier
position publication and alarm rearming, but stops later sprite publication. Native
crawler facing is not an X-scale setter (its artwork uses rotated frames).
The supported enemy adapter supplies opaque-white appearance to the original
transform setter's fast-draw flag calculation. Actors with different alpha or
blend must supply their real appearance, not reuse that assumption. Other enemy
scale setters, mask mutations, precise source scheduling and installation of the
full live collision registry remain necessary; these callbacks alone do not
establish original collision selection.

Snake and ground-turtle movement expose a checked presentation boundary after
collision/gravity resolution and before visibility-gated integration. At this
boundary both adapters publish the source sprite selection, then `image_xscale`
only when `xsp` is nonzero. These setters run even when movement is offscreen;
a zero velocity retains the previous scale. Rejection preserves earlier
collision/gravity changes but stops integration and final position publication.
The final shared animation normalization remains the existing approximation;
the adapters do not issue a second sprite setter at that point.

Frog motion separates collision resolution from integration. The shared Step
publishes nonzero horizontal scale from the Q16 velocity after collision response,
then applies the landing latch/alarm, then performs visibility-gated integration.
Grounded frogs still resolve horizontal collision and assign scale, but do not
integrate X. Native facing must not be reconstructed from the rounded Q8 mirror:
a nonzero Q16 velocity may round to zero there. Wall rebound integrates the
reversed velocity in that same Step, as the VM's final `xx += xsp` requires.
Checked hit publication precedes terrain queries; rejection of it or scale stops
later work while retaining completed motion-state changes. Collision geometry,
water behavior, binary64 arithmetic and exact individual hit-field mutation
timing remain incomplete; this is not the original mask solver.

Frog Step block 37 publishes `x=round(xx)`, `y=round(yy)` directly. Both
adapters therefore round the full shared Q16 horizontal and fifth-Q8 vertical
positions, with ties to even; neither rounds its Q8/binary32 presentation
mirror first. The even world origins (240,176) preserve tie parity when
publishing relative coordinates. Near-half fractions must survive until this
publication: 176.500137 is above a tie even if a Q8 mirror reads 176.5.
This removes a second quantization, not the shared motion approximation
described above.

Active bat pursuit calls the checked X-scale binding at `xscaleXsp`, after
blocked velocity components are cleared and before motion integration. The
original helper only assigns scale for `abs(xsp) > 0.2`; the current Q8 motion
path therefore retains facing through magnitudes 51/256 and assigns it from
52/256 upward. This is a source threshold applied to the existing approximate
motion representation, not a claim of original binary64 pursuit arithmetic.
Both adapters retain calculated heading, velocity, collision displacement and
consumed RNG on rejection, but stop integration and later position/sprite writes.
The native final facing update must not overwrite that result using any-nonzero
velocity.

`bindMasks` extends the checked binding to explicit mask setters. After bat X/Y
and sprite selection, shared `finishBatFields` dispatches exactly one source
branch: flying copies the selected sprite to `mask_index`; wall hanging sets
`image_xscale = -hang`; ceiling hanging does not write scale. Hanging preserves
the mask, then sets direction 270 and movesp 2 only after accepted scale
publication. Both adapters store accepted mask changes and use the effective
mask for support, pursuit terrain, activation, player and projectile contacts.
Object 116's serialized mask is -1, so its initial mask follows its sprite.
These contact paths still use the existing rectangle approximation; storing the
mask does not install precise sampling or source-equivalent spatial traversal.

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

`scrPlMovement` publishes `global.plx/ply` from private `xx/yy` before
integration. Skeleton Step reads these globals, not the rounded player builtin
position. The stored `withSkeletonTimeStopEvents` and native `stepSkeleton`
time-state overloads convert the portable player's captured frame-start Q8
position to absolute binary64 coordinates without rounding to pixels. This
conversion is exact for the represented Q8 values; it does not establish that
the portable motion matches the original binary64 integration.
Both bindings read `TimeStopEffects::player.stopped` at ordinary Step dispatch
and reject a failed time-state owner. The effects factory remains responsible
for actual throw, damage and death dependencies. These bindings neither create
player time transitions nor supply placeholder audio/effects. Frozen Step
retains pending hits; unfreezing resumes their processing. Death processing
remains outside the stop guard, as in the source actor.

`RunSession::parentTimeFieldContact` supplies the boolean union of the supported
ordinary and goal masks for player/shard queries. It narrows caller coordinates
once, does not consume RNG, and latches unsupported numeric failures without
publishing a partial result. This boolean query does not select an instance or
replace the independently ordered identity lookup above. The goal-only clock
query remains separate; an ordinary field must not implicitly stop the run clock.

In `objPlayer_n` Begin Step, `scrPlMovement` precedes clock accounting. The
`goalStop` and published-death guards bypass the field collision call itself,
not merely the counter increment. The shared player phase applies those guards
and completes clock accounting before shop input/keeper processing. A failed
field query stops before those later mutations; movement already performed is
retained. The current motion helper still combines collision, integration and
pose work, so this does not establish full source phase equivalence.
The source then applies wrap mode and publishes `x=round(xx)`, `y=round(yy)`.
The shared phase exposes a checked position-publication boundary before shop
processing. The stored host player routes it through its generation-checked
movement binding before changing the record's coordinates. Accepted publication
updates both integer and floating builtin fields to the rounded coordinates
(not the private motion accumulators). Rejection retains the old builtin fields
and earlier motion/clock changes, and stops subsequent phases. Deferred host bullet
emission keeps an explicit copy of the pre-movement muzzle coordinates, not
the newly published record. The native world has a separate published player
record in world coordinates, with a generation-checked single-slot lifetime and
a borrowed birth/movement/retirement binding. Constructor birth is checked;
Retry and next-level entry retire the previous key and preserve the monotonic
generation sequence. Death alone does not retire the player object. Failed
movement retains the old public coordinates, and failed retirement stops before
replacement. As with native enemy/box keys, this identity is family-local, not
an original runtime instance ID. The default binding remains empty; source
sprite/mask/transform publication and global collision registration are still
required before the native player can use the resource-backed owner. Positions here
remain derived from the existing fixed-point motion; this is not a migration
to the runner's full binary64 integration or sequential X/Y setter semantics.
Both adapters publish `scrPlMovement`'s horizontal input `image_xscale` writes
through the shared controls path. Left and right branches publish separately,
before the final both-key speed override; a spinning player skips these writes.
Rejection retains earlier acceleration/accepted scale and stops subsequent
motion, clock and position publication. The opaque-white appearance assumption
matches the currently implemented player path. The separate scale/sprite/index
writes in `playerSpriteControl`, shop-facing overrides, water and wall-side
queries are not supplied by this hook and remain necessary for a full adapter.
Camera/interactable queries follow, then the time-stop transition query uses
private `xx/yy`. `scrTimeShard` performs its own field lookup using the newly
published `x/y`. These are different positions at the same event boundary.

`StartupRuntime::stepWithPlayerTimeStop` composes this boundary with the stored
world's mixed animation/alarm/Step/cleanup dispatch. It reads published binary32
coordinates from the player record, while private coordinates and velocities
come from the current portable motion representation (Q8 positions/X speed,
1280 units per pixel of Y speed). This conversion preserves that representation;
it does not establish equivalence to the original motion arithmetic. Audio
queries are checked services, not default-false answers. A rejected query keeps
the already executed birth/effect prefix and fails the tick before subsequent
phases; it cannot be retried as a fresh transition. Callers still own the bound
registrations, effect bank and audio/effect services for this scoped entry point.

The clock's earlier field query and the later time-stop query must not be
merged merely because both use `parentTimeField`. The existing integer goal
query is not by itself evidence for fractional transition contacts. Remaining
integration and original-runner verification belong in [the queue](open-work.md).
