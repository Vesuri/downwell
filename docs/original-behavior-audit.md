# Original-game fidelity audit

Both ports must reproduce the Windows game. SDL is a development comparison,
not an oracle. Cross-target replay proves execution consistency for its tape,
not equivalence with Windows. Historical SDL-parity notes do not supersede this.

See also [activation lifecycle findings](activation-fidelity.md) for the
source-backed frontier/phase correction and remaining region-boundary gaps.

[Side-camera locker findings](side-camera-lockers.md) document the now-wired
VM-backed registration, latch/release and collision-phase camera snaps on both
adapters, plus the remaining original-runner and achievement caveats.

## Enemy-death level-begin cue

`levelBeginCue` returns without side effects when `firstLand` or `pTimeStop`
is true. Outside area 5 it synchronously creates object 372 at the caller's
position **before** writing `firstLand=1`. It then conditionally stops falling
sound 186, selects `areaBgm[area]`, and, if that music is not playing, sets gain
to one and invokes `soundPlayOL(bgm,100,1,1,"music")`. Area 5 omits the child
and playback: after firstLand/falling audio it stops the old BGM before selecting
the replacement. Area and BGM reads following synchronous operations must use
their current values, not entry snapshots.

The shared run-session operation is exposed through both runtime APIs and
uses the existing overlap-sound implementation/channel state. First-land,
player-stop, BGM, caller sound fields and environment are supplied by their
owners, as are actual child creation and the area-music lookup. This does not
admit a natural enemy-death caller or a level-number actor; those require real
source-global bindings and registered synchronous creation. Failures preserve
completed writes and suppress subsequent operations. VM guards and scripted
audio tests establish this ordering, not original-runner playback fidelity.

Object 372 (`levelNumber`) is parentless/nonpersistent and has Create, three
alarms, and Draw, but no Step event. Its localized text and measured width use
different source inputs: width measures `string_hash_to_newline(levelTxt)`,
whereas the character array uses the original string. Its one-based characters
start with offset -2; the first is visible immediately. Alarm 0 reveals later
characters every four phases, then arms Alarm 1 for 120. Alarm 1 starts exit
and arms Alarm 2 for four; index-ordered dispatch counts down those newly armed
later slots in the same phase. Alarm 2 writes `txtAt[txtShown][1]`, not
`txtAt[txtExit][1]`, and eventually requests destruction.

`LevelNumber` represents those events using caller-owned character tokens and
source-measured width. Draw changes offsets only when surfaceFx exists. Entrance
increments after drawing; exit increments before its visibility test. Four
separate view reads are preserved in the surface-coordinate calculation rather
than algebraically cancelling them. Each character invokes the shared black-border
sequence: write caller sx/sy/txt, set black, draw at (+1,0), (-1,0), (0,+1),
(0,-1), set white and draw at (0,0). It leaves white selected; a failed draw
retains the completed prefix without advancing that character's entrance offset
or resetting the surface target. Backend character operations must apply the
source hash-to-newline conversion and active font/alignment/alpha state.
The stored registry pairs that private state
with a resource-backed room record and consumes the enclosing numeric-ID stream
before synchronous text initialization. Its scoped alarm adapter receives the
already-decremented slot from the normal room phase, writes back newly armed
slots, marks destruction, and retires through normal deferred cleanup. Draw can
be invoked at the record's shared layer position; absent surfaceFx leaves text
motion untouched. Clear the borrowed registry before room teardown.
`LevelNumberInstances` provides the native typed lifetime with the same event
state, externally supplied source IDs, positioned birth, activation, deferred
destruction and retirement. It deliberately has no pool-wide Step/Draw pass:
the world must supply mixed alarm/layer visits. Inactive actors retain alarms
and letter offsets. The native room owner registers its resource-backed identity
and source ID, includes it in the mixed alarm capture, and retires marked actors
at Draw cleanup. Gameplay room retirement and Retry clear its nonpersistent
lifetimes while retaining the owner binding. The three ownership modes exercise
the same 142-tick three-character lifetime and later-slot alarm countdowns.
Natural source-ID allocation, localized text, active-font measurement and
surface-text Draw dispatch remain unconnected; explicit test births do not
admit the natural level-begin/death caller.

`generate-area-titles.py` reads the shipped UTF-8 `language.ini` and writes all
nine languages' area0–area5 labels as Unicode tokens to ignored generated data,
with an input/tool/output-hash manifest. `LevelNumberText` constructs normal and
hard-mode suffixes for the integer RunStage domain, retains the original
characters for `string_char_at`, and supplies hash-converted characters to the
current font owner's measurement callback. Missing keys yield the source's
single-space default. This text construction is tested across the extracted
labels but is not yet bound to the live language/font state or natural births.

### Original title-width measurement

Read-only inspection of the pinned Windows executable identifies string_width
registration at 0x537D32 (name 0x811D04, wrapper 0x535D50). The wrapper converts
the string to character tokens and calls 0x45AA40 with both optional arguments
set to -1. That routine resolves the selected font from global 0x8A5DF0 (or
the default font), builds lines through 0x45ADC0, and returns their maximum
integer width. Its line-width method is 0x44A2A0:

- Look up each character through 0x449830. Missing characters retry U+25AF;
  if that is absent too, the advance is zero.
- Read the signed 16-bit glyph advance at record offset 0x0A, convert to
  binary32, multiply by the font's binary32 scale at offset 0x8C, add the
  previous integer width converted to binary32, then truncate back to integer
  with CVTTSS2SI. The string_width caller supplies zero inter-character spacing.
- There is no call to the separate kerning lookup at 0x449920 in this path.
  The current SDL drawText lineWidth lambda adds kerning and therefore is not
  an oracle for original string_width.

`tools/verify-runner-text-width.py` pins the reviewed wrapper, font setter,
maximum-line-width routine, glyph lookup and per-line arithmetic. Normal
scrInitialize block 54 selects resource 2; levelNumber Create does not select
a font itself. This is static executable/VM evidence, not an execution
comparison. Font scale loading, line splitting and live active-font publication
still require binding/audit before the measurement callback can be admitted. Do not replace
this callback with character-count times eight or kerning-aware SDL layout.

The pinned resource manifest gives all eight shipped fonts scaleX=scaleY=1.
The font-pack v1 generator now rejects any other scale rather than discarding
it. `FontPack::stringWidth` measures this unit-scale domain: signed glyph
advances, U+25AF fallback, no kerning, CR/LF line maximum, and NUL termination.
It rejects cumulative widths outside the exactly representable binary32 integer
range instead of substituting a wider sum. Localized-title tests use the actual
pack for all 54 labels and both suffix modes; nonzero kerning pairs are checked
separately to distinguish width measurement from drawing. Runtime active-font
binding and font-loader scale publication remain separate requirements.

## Result-room population and persistent alarms

The exported `objLineToResult` Alarm 1 calls `room_goto(16)` unconditionally.
Room 16 is `rmResult`, a nonpersistent room with one authored instance:
`Result` (object 325), numeric ID 102522, at (80,144). Object 37
`objControlerN` is persistent; object 325 and the exit wipe (68) are not.
These are serialized flags, not an assumption based on visible presentation.
`tools/verify-enemy-death.py` guards this metadata and the relevant VM events.

The room Create only sets view-camera 0's size to (160, g_cameraHeight).
Neither it nor Result Create writes `room_speed`. Result Create sets
`pTimeStop=1`; that gameplay global is not an instruction to suspend runner
alarms. A retained controller's pending moment-delay restoration must not be
reinitialized or postponed merely because the upgrade screen is visible.
Controller Create must not run again for that survivor.

This is a destination-room scheduler, not the previous gameplay scheduler
running behind a menu. Nonpersistent old-room lifetimes must leave before
the destination's authored birth, with the persistent records retained and
rebound through the room lifecycle. Result's alarms participate in the same
alarm-index / resource-ID / exact-object-registration traversal described in
[runner alarms](runner-alarms.md). In particular, Result Alarm 0 creates a
new object 68 synchronously and then arms its own Alarm 1 to 90. A blanket
controller-alarm pass before all Result alarms is not generally equivalent:
for each alarm index, object 37 precedes 325, but Result Alarm 0 precedes
controller Alarm 1. Same-phase birth eligibility remains governed by the
runner's creation filter, not unconditional recapture after each event.

Stored Result entry uses the resource-backed room loader: old nonpersistent
records leave, survivors retain their handles, and authored instance 102522 is
registered before its PreCreate/Create. Its Create publishes y=142 before the
shared selection initializer consumes gemSp's RNG word. A failed birth prevents
that initializer and latches the partial handoff; a failed Create preserves its
completed prefix. The synchronous request supplies style/heart-goal inputs only
during that load and is cleared on return. It is not a second persistent global.

The native path retires its registered gameplay families before entering the
shared Result state, preserving persistent controller ownership. With the native
lifetime owner bound, it reads room 16's sole authored object 325 from the room
pack, registers its source ID/position, then publishes y=142 before the shared
initializer consumes RNG. Birth or position failure preserves the completed
prefix and prevents that initializer. Departure retires the Result handle;
Retry retains the bound owner but not the room-local lifetime. This admission
does not yet load rmResult's layer catalog/rebind persistent elements.
Its destination alarm adapter captures registered controllers and Result once,
then uses the resource-backed alarm dispatcher; no old gameplay alarms run
behind the menu. The admitted Result must remain active. The scalar exit wipe's
Alarm 0 is advanced immediately before Result Alarm 0, after controller Alarm 0,
so a wipe born by Result waits until the following phase. This preserves order
for that admitted population but does not establish registered wipe ownership
or arbitrary Result activation/populations. Unsupported fired controller events
stop the phase before later Result counters or Step. Controller callbacks still
require the shared room-speed state, not a PAL scheduling constant.
Neither path yet supplies complete source-ordered Room End/retirement
event dispatch or all Result Create side effects. Result clocks still belong
to the shared selection state, not the stored record's generic alarm bank.
The room-population checks are not proof of those missing event integrations
or an original-runner execution comparison.

Result Alarm 0's stored exit wipe uses the shared numeric allocator and normal
object-68 birth at (0,0), followed by X then Y publication and Alarm 0=15.
The checked birth hook completes before the shared wipe snapshot is enabled or
Result Alarm 1 is armed. Rejected birth/setters retain the allocated ID and
accepted record writes, latch failure, and suppress later alarm effects.
The stable handle leaves with rmResult. The stored destination alarm phase uses
one captured population, interleaving the controller's registered alarms,
the wipe's Alarm 0 and the shared Result clocks by alarm index/resource order.
Unsupported fired controller events fail the phase after its completed prefix;
they are not silently skipped. Moment-delay restoration requires the actual
room-speed state to be borrowed by the caller. The wipe's registered Alarm 0
feeds the shared effect state, and the shared Step publishes movement afterward;
these are not two independent timers. Create currently consumes that adapter's integer view snapshot, not a
complete generic camera-cache read. Fractional view publication, arbitrary
wipe populations and complete activation-aware mixed event dispatch are not established
by this single natural-departure integration.

The admitted ordinary Step order is wipe (68), then Result (325). The wipe
compares its y against the pre-Result view bottom and publishes its movement
before `scrResult` eases `global.ply` and `yy`. Result's builtin y is
`round(yy)`, not `global.ply`; both adapters publish that rounded value from
the shared motion state using ties-to-even rounding. Rejected publication
retains the earlier alarm/wipe/motion prefix and stops later presentation.
The VM guard pins the final `yy`/`round`/y sequence and the enclosing Step's
subsequent wind-audio branch. That audio/RNG branch and complete mixed room
Step dispatch remain outside this admitted two-object adapter.

## Main-room controller births and numeric IDs

`verify-runner-instance-ids.py` pins both the executable's GEN8-based counter
and the relevant exported room/event evidence. `rmMain` contains 244 authored
instances: 240 walls, builder 100856 at creation index 0, player 100855 at 1,
`objDitherAbove` 100854 at 2 and `startTimeField` 101097 at 147. These authored
IDs must not consume fresh dynamic IDs or be replaced by native family slots.

Player Create first runs `scrPlayerInit`, then inherited `parentLife` Create
(sets alive), then creates depth meter 66 and calls `scrSpawnCamera` (creates
64 and stores `myCam`). Only in room 11 it subsequently creates entry-dither
73 at (0,0), after the level-one style/achievement assignments. The earlier
initializer can already create decoy 256 and one or two drones 249; this is
not a fixed three-ID reservation independent of upgrades. Called scripts and
child Creates still require their own complete synchronous birth accounting.

`PlayerChildCreation` expresses this ordered birth/post-Create sequence without
owning an allocator or instance storage. Its create operation returns the captured
child ID only after synchronous Create succeeds; the reverse-drone and camera
callbacks receive that ID even when nested births make IDs nonconsecutive.
The stored player Create calls this shared sequence. Native integration must
supply real registered children and the complete allocator, not no-op callbacks.
`GameplayWorld::createPlayerChildren` supplies the native lifetime operations
for rmMain, retaining `droneNum`, the returned camera source ID, post-Create
camera flags and the entry wipe's published-view read. Its allocator callback
is required at each actual birth, including the wipe; allocation failure keeps
the already created prefix and permanently fails that world. Retry clears the
children before a new player lifetime can execute the sequence.
`withPlayerChildCreation` borrows that allocator for an authored-room load and
executes the sequence immediately after player registration, before the
compatibility camera snapshot and later authored instances. Late installation
after player birth or room loading is rejected. The borrowed callback is cleared
on return, including failure. The default caller still must provide the complete
enclosing numeric-ID stream rather than a local counter.

Entry-dither Create copies parent-camera 63's x, reads view 0's published y,
and sets Alarm 0 to 120. Step adds 16 to y and recopies camera x; Alarm 0
destroys it. Draw targets `global.surfaceFx`. Camera Create itself does not
publish the view, so substituting camera.y minus half the viewport height
for that getter is not justified by these events. Neither a dummy ID nor a
generic overlay reproduces this controller.

The stored adapter synchronously creates 66/64 inside player Create, completing
each child's PreCreate/Create before the next birth and before the next authored
room instance. It passes the parent's destination room, not the still-current
old room, to the shared allocator. A rejected child birth retains the allocated
prefix, prevents later room births and latches failure. Menu/main-room tests
check child adjacency, destination ownership, numeric IDs and both child-failure
boundaries. Successful camera creation returns that child's captured numeric
ID and publishes it to the player's `myCam` field only after Create completes.
The reference is included in stored-state digests; it is not reconstructed
from a storage slot or the allocator cursor after possible nested births.
Failed births do not publish a result. Camera Create initializes freeCam and
autoScroll to false; the caller then enables freeCam for ground rooms 20–24
and rooms 10/17, and explicitly clears autoScroll in room 11. Stored-state
digests include these flags. Live menu/main-room tests cover their final values;
this does not implement the remaining alternate-camera Step paths or prove
the other rooms' full behavior. Entry-dither 73 has stored-world ownership as
described below. Conditional companion births are synchronous too: Heart
Balloon (256), then one or two Drones (249), then the depth meter/camera.
Their actual registered lifetimes own the Create state in a three-entry sparse
store; no dummy IDs are reserved. `droneNum=1` follows the first Drone's
successful Create; the second Drone's xdir/ydir become 180 only after its
Create returns. A failed birth retains prior children and prevents later
children and authored room instances. The two-drone branch is tested with an
explicit value-two input, not by bypassing the normal one-upgrade offer cap.
The stored scheduler advances their initial two-/four-key sprite sequences in
the existing active-room animation pass, with binary32 image speeds and
publication to the collision owner before committing the new timeline. Neither
object has an Animation End event. Failed publication stops the simulation
without advancing the local timeline or retrying the write. This covers the
initial sprites, not the balloon's later sprite changes.
The stored alarm dispatcher executes the shared source bodies: Drone Alarm 2
clears `shotDelay`, Drone Alarm 3 is empty, and Balloon Alarms 0/1 clear
`noHold`/`hitStun` respectively. They consume no RNG and do not reset other
companion state. The shared runner countdown owns eligibility and the zero
value on the firing frame; the bodies do not force the alarm to -1.
Balloon End Step (2187) tests `global.pugDecoy` and requests self-destruction
when it is zero. The stored runtime publishes the destruction mark before
changing the record, then retires the registered lifetime during the cleanup
phase. Its custom companion state is released only after retirement succeeds;
a rejected publication retains the accepted prefix and stops execution.
Native `GameplayPlayerControllers` also admits one Balloon and two Drone
lifetimes with supplied source IDs. Their Create state, initial binary32
animation speed/index and second-Drone direction override use the same shared
representation. Initial sprite animation runs in the existing room traversal;
simple alarms run in the mixed resource-ordered alarm phase, not an extra family
tick. Rejected publication or an unsupported fired body fails the owner. Retry
retires these records with the other player children. This explicit admission
does not allocate the automatic global birth stream.
Balloon End Step joins the admitted skeleton/drone-projectile End Step snapshot
in object-resource order. Both runtimes use the same upgrade guard; the native
path reads it at event dispatch, publishes the destruction flag before local
mutation, and retains the lifetime until Draw-entry cleanup. Inactive or already
destroyed balloons are excluded, and retired slots receive a new generation on
reuse. This admitted actor phase is not the complete mixed-world End Step pass.
Companion motion, Drone's firing Alarm 0, Draw, automatic native births and the rest of player
initialization remain incomplete; correct child IDs alone do not prove those.
Drone Step adds two to each custom direction every tick without modulo. Its
lengthdir inputs therefore exceed the current `SmokeDirection` domain
(and its X radius is 24, beyond that helper's length limit of 8).
Wrapping angles or substituting host trigonometry would not preserve the
original wrapper's float conversion and argument reduction.
The recovered `SmokeTrig` program admits the original straight-line dispatch:
the sign-cleared top sixteen bits of the binary64 radian input must be in
`0x3030..0x40f5`, with zero handled separately. In particular, 90112 radians
is excluded, while its binary64 predecessor is included. The source dispatch
at `0x74b621` / `0x74b7e1` is verified by `export-smoke-trig.py`.
Larger arguments call `0x770b1f` / `0x770c0f`, which execute x87 `FCOS` /
`FSIN` and use `FPREM1` if hardware argument reduction reports out of range.
Those fallback operations are not part of the portable polynomial. Expanding
its admitted range does not establish arbitrary-angle companion support or
the original process's live FP environment.
Native terrain loading retains the authored player/wall IDs but is not complete
cross-family room creation. Ordinary Amiga entry uses `authoredRoom` player-birth
timing: construction prepares the room views without registering a player, the
collision owner attaches, and the serialized room walk registers object 35 at
index 1 before dither/wall creation. Builder lifetime and setup run at index 0
as detailed below. Retry and
subsequent level entry preserve this timing; an attempted gameplay tick before
the player is born fails rather than simulating an unregistered player. A failed
birth stops the room walk and cannot be retried. The initial/Retry compatibility
camera snapshot is advanced only after the birth callback, and the backend then
rebases its display origin. The source ID is 100855, not a dynamic allocation.
Already-entered standalone/replay fixtures retain construction-time birth;
resource-less fixtures retain the explicit late ID handoff. Neither route yet
owns the complete controller population or synchronous player-child
sequence, so this does not establish the global dynamic-ID stream.
`GameplayPlayerControllers` provides native lifetime storage for player-created
objects 66 and 64. Its resource-backed owner uses supplied source IDs and local
generation handles separately, retains binary32 position publications, and
registers activation/layer membership. Retry and next-room teardown retire these
records before reconstruction; local generations are not rewound. This is an
explicit-birth boundary, not automatic player Create. `createCamera` completes
the shared `CameraCreateState` body before returning the child handle. Create
computes initial accelerations from the pre-Create coordinates, publishes X
then Y, and only then copies accepted public values into private xx/yy. A failed
setter retains earlier writes without returning a completed child. These use
the existing binary64 arithmetic contract; original process FP-state verification
remains separate. Controller Step/Draw and allocation through the full synchronous
child stream must still be connected. No invisible placeholder births or fixed
ID reservations are used.
The room-entry field likewise resolves authored object 371's ID 101097 before
its lifecycle birth, scale publications and random-radius Create. Its registered
layer lifetime retains that ID; first-level destruction and Retry retire the
old generation before reusing the authored identity. Resource-less standalone
fixtures remain explicitly unnumbered. Fresh native room loading executes the
field's Create at authored index 147, after 144 walls and before the remaining
96 walls, retaining its destruction mark until room-entry reconciliation.
Consequently, explicit-ID time-change tests do not establish the natural
global ID stream. This source check also does not prove original runner room
phase timing, initial view publication or rendered entry-mask parity.

### Native room-entry boundaries

Authored `objDitherAbove` (61, sprite 128, parent 59) is distinct from the
player-created entry wipe (73). Its Begin Step (990) first publishes camera
family 63's X. It then compares camera Y minus its own Y with 272 (normal)
or 208 (low-spec), advances Y by 16 once when the threshold is met, and always
rounds Y before checking `global.ending` for self-destruction. This is not a
catch-up loop. Its wall Collision event (991) enters `other` and destroys the
contacted wall; Draw (992) is `draw_self`, not the entry wipe's effect surface.
The assembly hashes and event metadata are pinned by the instance-ID verifier.
The stored dispatcher executes Begin Step through `DitherAboveStep`, publishing
X, optional incremented Y, and rounded Y separately through the lifecycle
owner. For this controller the precise coordinate fields mirror published
builtins, including external position writes; they are not motion accumulators.
Camera reads require one active camMain in the same room and reject ambiguous
or alternate cameras instead of choosing storage order. Low-spec and ending
inputs are normalized booleans supplied through `configureDitherAboveEnvironment`
and included in stored replay state; ordinary desktop entry defaults both to
false. Full settings/ending owners still need to supply their changes.
The stored collision handler checks the resource-selected event and marks the
contacted plain wall through its lifecycle owner, retaining it until cleanup.
It rejects unsupported target families rather than omitting their Destroy
events. SDL's ordinary update installs `StoredDitherCollisionPass` at the
post-Step/pre-End-Step boundary. It collects the complete owner queue in source
order, executes only plain-wall/dither pairs through the paired wrapper and
defers retirement to ordinary frame cleanup. Other collision families remain
on the existing gameplay adapters; this migration pass must be replaced, not
run again alongside a complete dispatcher. A collected-pair scheduler fixture
also dispatches wall contacts alongside shards; neither fixture verifies the
original runner's simultaneous-contact order.

The native `GameplayDitherAbove` store is separate from `GameplayEntryDithers`:
one authored room controller, with source ID 100854 and a distinct lifetime
generation. Fresh room loading admits it at serialized index 2, before the
first wall, in both prepared and decoded traversal. The audit-only explicit
admission also reads the room record instead of consuming the dynamic ID
stream. Terrain-only adoption does not replay this Create; importing the
already-executed controller state remains necessary there.
All native owner modes retain its object/layer lifetime;
the reference owner also publishes collision geometry. Once admitted, the
ordinary Begin Step executes the shared script against the previous published
camera position; ending marks it for the owner cleanup boundary, and Retry
invalidates the old handle without restarting its generation counter.
Native Draw projects this controller at its retained world depth, separately
from the entry/exit transition overlays. The offline pack retains the full
320-by-80 black edge (65006); generation verifies the other 720 rows are solid
black. The compositor clips a finite black rectangle and the edge, writes only
the visible display buffer, and records restoration damage without changing
the terrain backing. Both commands remain planar occluders during hardware
sprite assignment. The binary-alpha approximation is the same as the existing
transition masks, not proof of the original GPU's alpha-1 hole blending.
Native wall contacts are not enabled yet. Controller creation/movement/Draw
does not establish complete room ordering or wall retirement equivalence.
Neither registering an inert native record nor relying on generic
out-of-view terrain retirement reproduces this source lifecycle. Integration
must preserve the wall's destruction mark and the subsequent phase's retirement,
including ordered simultaneous contacts; an immediate occupancy erase is not
by itself evidence of equivalent collision ordering.

`PreparedRoomTerrain` retains all 244 authored records in serialized creation
order, not only the player and 240 walls. Backend terrain commands use indices
into that complete sequence but emit only objects 60/93/99. Preparation does
not execute events or publish lifetimes. `loadRoomTerrain` is still a terrain
handoff: retaining the controller records does not execute their Create events.
`loadRoomTerrainWithStartField` is the fresh native entry path; it dispatches
the field at its serialized boundary and performs cleanup after the traversal.

There are distinct existing entry inputs, which must not be conflated:

- Ordinary native initial entry restores the builder's seed-2, draw-330
  checkpoint, then executes entry-field Create (draw 331). The builder's
  authored position is (-16,704), distinct from the player's (240,176).
  `gml_GlobalScript_scrBuilder` block 13 creates object 78 at `self.x-100`,
  `self.y`; its initial X is therefore -116. All native entry checkpoints
  retain this position, including the seed-4 and seed-8 verification entries.
- Native Retry/next-level entry resets the builder on the world's current RNG
  and subsequently executes entry-field Create. It must not restore the initial
  seed/checkpoint.
- The scene parity/terrain-ownership fixtures can adopt a `StartupRuntime`
  entry: `mainBuilderState()` and `randomState()` already include synchronous
  room creation. Replaying builder or field Create after importing that cursor
  would consume those effects twice. Adopting state is not a new source birth.

The stored adapter runs builder Create (code 1002) at authored index 0 and
field Create (2731) at index 147. Native builder ownership is published at
authored index 0 with source ID 100856, before the production player's birth
at index 1. Its typed record retains position and identity, while generation
and RNG state remain in the existing shared builder. A failed builder birth
stops entry before player, terrain or field births. Retry/next-level teardown
retires that identity before publishing a replacement generation.
Native fresh-entry callers perform builder setup immediately after that
lifetime's birth, before the traversal reaches the player. Initial entry
restores the audited startup checkpoint at that boundary; Retry/next-level
entry runs the shared builder Create on the current RNG with the authored
coordinates. Failure stops the room traversal without retrying Create or
consuming the entrance field's RNG. Adoption of an already-entered stored room
does neither operation. The initial checkpoint is still not a complete replay
of every startup controller birth, and native player-created
controllers/companions are not yet a complete synchronous birth stream.
These omissions prevent claiming full room-order parity. The builder
joins resource-backed lifetime, spatial and layer ownership; full mixed event
and Draw scheduling remains a separate integration requirement.

`scrBuilder` blocks 187 and 207 write `self.y += 16` after each newline's
pillars and each extra-pillar row respectively. Shared scalar, run and prepared
generation retain these setter boundaries, including the completed prefix on
failure. The stored adapter publishes each setter through its lifecycle owner
before the next synchronous child Create; copying only the final frontier into
the instance record is not equivalent. Native generation binds the same row
publication points to the builder's registered position for each Step.

Non-glitch wall Create consumes no RNG, but that does not make relocating
field lifetime publication or cleanup equivalent. Terrain failure before the
field consumes no field RNG; failure afterwards retains its marked lifetime
and prevents room-entry cleanup and later terrain publication.

The native adapter's compatibility entry snapshot includes one camera Step.
That update runs after successful player birth, not before its callback; the
callback observes unadvanced camera offsets and the initial ring origin. Retry
likewise restores the pre-Step state before publishing its real bound player
lifetime, then prepares the same visible snapshot. A failed birth must not
advance the camera or ring. This ordering does not create the omitted native
camera/depth-meter instances or establish the startup numeric-ID stream.

`EntryDither` expresses the shared Create/Step/Alarm behavior without owning
room traversal or numeric IDs. Its ordered reader callbacks preserve the x
assignment before the view read, and the y increment before the next camera
read. Built-in coordinate writes narrow to binary32 using integer-only native
arithmetic. Read/arithmetic failures retain the accepted prefix and latch
failure. Destruction is a request, not immediate storage retirement. The room
scheduler must exclude destroyed/inactive instances and either decrement the
compact alarm or call `fireAlarm` after its own decrement, never both.
`verify-entry-dither` compares fractional/large-coordinate Step results to a
host floating-point transcription and checks the 120-update lifetime and
failure prefixes. A freestanding 68000 probe compiles without undefined
helpers; this is not emulator execution or live scene integration. Draw still
requires the original effect-surface path and independently published view.

`StoredEntryDitherRegistration` connects that state to stored instance lifetimes
and the shared numeric allocator. Each coordinate setter is published before
the next source read, not as an end-of-event batch. It imports externally
written coordinates for Step, honors active/destroyed state, and marks Alarm 0
destruction without immediately recycling the lifetime. Explicit retirement
removes its owner registration before freeing storage. Tests use the recovered
object pack and cover fractional positions, IDs, publication failure, inactive
Step, expiry, retained destruction and reuse. The runtime exposes a borrowed
registration factory; its caller must supply authoritative camera/view reads,
dispatch and room cleanup. `stepWithEntryDither` scopes the registration into
the real stored-room alarm/ordinary-Step traversal; its callback does not
survive the tick. Object-73 dispatch without that owner fails instead of
silently freezing the effect. `cleanupEntryDither` retires selected marked
records through global cleanup and delegates other families explicitly.
`withEntryDitherRoomCreation` additionally scopes the owner into the actual
player Create event, after camera creation/mode setters and before the next
authored room instance. It uses the parent's destination room and the prepared
resource view Y, while the caller supplies the parent-camera X lookup. Both
creation and event callbacks are removed when the scoped operation returns.
The resource-backed live collision test now enters through this player event,
verifies the contiguous camera/dither IDs and destination room, then exercises
120 main-room ticks: one decrement per tick, 119 Step calls, retained destruction
and owner/layer retirement. Fast-forward includes the first new-room update,
so its return snapshot has Y=16/alarm=119, not the Create snapshot Y=0/alarm=120.
The test checks the single-camera case; alternate camera lookup/publication
still needs verification. The stored runtime now also owns a default registration:
player Create admits the effect after the camera, the normal room scheduler
dispatches its alarms/Step, and global cleanup retires the destroyed record.
Reset, Retry and room changes clear that registration before room teardown.
The default camera read requires one live object-64 parentCamera descendant in
the destination room and uses its published binary32 X. The supported camMain
Step publishes `round(xxFinal)` there, not its fractional easing accumulator;
external setters still publish their supplied binary32 value.
Multiple/alternate cameras fail rather than guessing object-read order.
Explicit scoped registrations retain caller-controlled cleanup for audit tests.
Unindexed reference worlds opt into local-store publication, retaining identical
IDs and lifetimes; a configured collision owner must still accept every write.
Resource-backed tests cover automatic expiry and Retry before expiry in addition
to the scoped traversal above. Host rendering consumes those registered records.
The native gameplay tick dispatches registered entry Step after camera Step;
automatic native births and full companion creation ordering remain open. The
explicit-ID native lifetime and immutable presentation fixtures do not close
those creation requirements.

The offline game-pack generator also exports `amiga-game.entry-mask` and its
input/tool-hashed provenance. Its 48-byte rows contain transparent padding,
the original sprite-129 opacity edge, then solid padding, at logical x=110.
It occupies 31,104 bytes, the same size as the exit cover. The native backend
loads it into the retired-garden overlay bank with tracked copper-slot residency;
see [effect surfaces](effect-surfaces.md) for the retirement fence and overlap
restrictions. `AmigaEntryMask::offset` selects a full
284-row window without shifting or modifying the bytes. The compact solid tail
requires screen y >= -116 (the original sprite's bottom then lies outside the
display); unsupported positions fail without changing the output pointer
offset. Rendering a lower bottom requires a different representation, not
clamping to solid. The independent texture test checks every row, including
HUD margins, for 917 integer screen positions. It uses the existing binary
opacity threshold, not an assertion that source alpha=1 holes are bit-identical
to zero-alpha GPU blending. Automatic birth timing, fractional raster policy and
original-runner presentation verification remain open.

## Horizontal control arithmetic

`scrPlMovement`'s non-water living-player path applies left and right input
in that order, then forces zero speed when both are held. Each acceleration
branch clamps only in its input direction; an opposite-directed external
impulse is not capped by that branch. With neither key held, VM blocks 77–80
subtract `sign(xsp)` times ground/air friction and then test `abs(xsp) < 0.3`.
Do not replace this with a sign-crossing test: sufficiently small grounded
speed reverses and remains nonzero after friction.

The audited UTMT assembly `gml_GlobalScript_scrPlMovement.asm` has SHA-256
`3ff0e28b9dcef221ed9dd6a1ceb801bef746d7538016547fbe49a60af0177644`.
`PlayerControlsTest` compares branch behavior against a direct source-order
transcription over grounded/airborne, both input bits and control-lock states.
Its fixed/float cases verify rules within each arithmetic adapter; they do
not establish VM binary64 storage fidelity, water behavior, wall feedback or
the complete movement script. Those remain governed by the single work queue.

## Cavern level-dependent enemy spawning

`gml_GlobalScript_builderCavern.asm`, blocks 12–38, consumes
`random(global.level + 2) <= global.level` for every pattern cell. Normal
cavern levels 1–3 therefore use inclusive uint32 cutoffs `55555555`,
`80000000`, `99999999`, respectively. The former implementation used the
level-one cutoff throughout. Symbol `c` additionally consumes `choose(0,1)`
only after passing that gate and only at level >= 2; nonzero creates crawler
119 synchronously, including its Create-event RNG consumption.

The shared builder now uses `CavernEnemyChoice` for both platforms. Tests
compare 110,592 cell decisions and complete RNG state against an independent
floating-point transcription, plus exact cutoff boundary cases. The 64-level
emission regression fingerprint is now `970b28efcbf63992`; it is not an
original-runner capture. First-level entry checkpoints are unchanged.

This implements the supported normal-mode path. The VM's short-circuited
hard-mode fallback (`random(5) > 1` after a failed primary gate) remains
unimplemented, along with live hard-mode selection; this change does not
claim hard-mode fidelity.

## Side-room reward selection at low health

`gml_GlobalScript_scrBuilder.asm`, blocks 141–152, first chooses/alternates
resource 293 (gem chunk) and 109 (crate). At `global.playerHp <= 2`, a selected
non-crate consumes `choose(0,1,1,1,1,1)` and switches to a crate on nonzero.
Only afterward does style 1 force a crate. The low-health random draw must
still occur for that style; suppressing it changes all subsequent generation.
The chosen result, including overrides, remains the next alternation state.

The previous shared builder omitted both overrides. `BuilderRewardChoice`
now preserves this VM branch order; both live adapters supply current health
before the synchronous builder Step. No health is changed by the choice,
and rendering does not draw RNG. Unit tests compare 61,440 choices across
seeds, health thresholds, styles and successive retained selections against
an independent general-63-bit-choice oracle, including every RNG word.
The existing 64-level high-health ordered generation digest remains
`ffcdfc23dc638be7`. Unit and ASan/UBSan logs:
`/tmp/downwell-reward-bias-unit.log` and
`/tmp/downwell-reward-bias-sanitized.log`.

The eight input-only routes retain their existing outcomes with exact adapter
and RNG comparisons (`/tmp/downwell-reward-bias-sweep.log`). The bounded native
route passes 211 upgrade frames, 135 exit-field frames, 49 shop-HUD comparisons
and 43 return updates, with zero failures and an intact stack guard. Normal
startup is restored, both 68000 audits pass, and SDL is rebuilt. Evidence:
`/tmp/downwell-reward-bias-native.log`,
`/tmp/downwell-reward-bias-native-gdb.log` and
`/tmp/downwell-reward-bias-sdl.log`. These live routes are surrounding
regressions, not dedicated coverage of every low-health reward branch.

This verifies which reward object is created, not complete crate contents,
pickup effects or original-runner presentation. The discrepancy was found
while auditing the shared builder ahead of area-two implementation; the
later-area transition remains deliberately disabled until its own builder,
actors, hazards and artwork exist.

## Currency disappearing flash

Both renderers now consume `GemLifetime::visible()` instead of drawing every
live gem unconditionally. The original `objCurrency` Create sets `dflash=-1`;
Alarm 1 sets `dissapearing=1`; Step blocks 75–76 negate `dflash` while that
flag is set; Draw blocks 7–8 call `drawFx` only when the numeric flag converts
to false. Negative one is false under GameMaker's conversion, not C++'s
nonzero truth rule. With default unfrozen alarms, age 189 is the first hidden
frame and age 190 is visible. Alarm 0's write to Alarm 1 is decremented later
in the same phase, as is Alarm 1's write to Alarm 2; expiry is therefore at
age 308. See [runner alarm ordering](runner-alarms.md). Attracted gems that survive
Alarm 2 continue alternating. Repeated rendering never advances the phase.

This is a draw-only predicate. Hidden gems still move, attract and collect;
the fixture explicitly collects one on its first hidden update. Unit tests
compare against an independently toggled signed flag through 400 updates.
The real SDL renderer test uses a spawned gem and checks ages 0, 188, 189,
190, 307, 308, 309 and 65536, preserves other gems, and checks that repainting
changes neither lifetime nor RNG.

The shared gem CPU fixture schema 9 includes visibility, first-Step choice,
same-phase chained alarms and binary32 animation before expiry. Its digest is `ffa629f6f72c7cff`, coverage
`3ff`. The fixture requires both visible and hidden disappearing gems to
remain alive. This CPU-equivalence check does not establish Windows-exact
currency movement, general time-stop/event scheduling, drawFx palette effects
or audio fidelity; those remain in the open-work queue.

### Currency animation arithmetic

Sprites 133 (`sprGemS`) and 134 (`sprGemM`) each have four consecutive unit
keys and playback speed 1 frame per game frame. The metadata guard explicitly
checks these two resources. Their signed Create image speeds narrow to binary32.
The selected runner adds the speed in binary32 and, on crossing a boundary,
wraps with `float(fmod(float(fmod(position,4)+4),4))`.

`make verify-currency-animation` compares the integer-only sequence evaluator
with independent host float arithmetic for all four Create choices over 4096
updates each. The shared live `GemAnimation` helper must agree in both float
state and visible frame, without consuming update RNG. Resetting it must clear
the sequence position. At speed +0.3, update 10 has image-index bits `403fffff`
(just below 3), selecting frame 2, not the frame 3 of exact decimal tenths.
The helper admits only the four Create speeds and an internally maintained
four-frame position; arbitrary image-index/speed writes need the full owner.

Both world adapters advance existing gems before Begin Step and alarms, not
inside currency movement. Combat-born gems therefore retain image index zero
for their birth tick and advance for the first time on the following tick.
The paired contact tape checks these boundaries in both live worlds. The
isolated CPU fixture explicitly advances before expiry as well. No gem
Animation End callback runs, and animation itself consumes no RNG.

Currency still needs registered active-room membership, pending activation
semantics, and time-stop image-speed writes; the current standalone family
pass does not replace those ownership requirements. Currency movement and
alarm scheduling remain provisional, including same-tick birth eligibility.
The arithmetic test does not prove the Windows process's live FP mode, event
phase parity, or presentation parity. See the source image-field and FP-mode
audits before treating the scalar comparison as a complete runtime oracle.

## Exit-field timer guard

### Live Draw RNG

The field's once-per-update Draw RNG is now connected in both live adapters
through `RunSession::gameplayPresentationStep`. VM Draw blocks 0–1 update
`radius` unless `pTimeStop`; blocks 2–5 check the strict camera bounds only
after that random draw. Goal-stop is not time-stop. An absent/retired field
does not draw RNG, and the explicit time-stop input preserves the radius.

`GoalTimeField::drawSnapshot` is now const and has no RNG argument: repeated
buffer repairs or host paints cannot change simulation. The existing 10,000
event test still checks exact binary64 radius, outline order, camera bounds
and every RNG word, now also testing repeated snapshot paints. A live world
fixture independently checks 100 updates against one expected RNG draw per
update; paired gameplay tests compare field lifetime and radius as well.

The corrected RNG stream changes post-clear generation. The focused tape now
reaches seven enemies/15 shots in level two and two enemies/15 shots in level
three, with exact adapter comparisons and active-enemy coverage retained.
Those changed populations are not a reason to preserve the previously missing
Draw side effect. Log: `/tmp/downwell-field-draw-focused.log`.

Both live renderers now use `GoalFieldOutline` for the white radius/radius-1
and red radius+1 outlines. Wind audio/associated RNG remains unimplemented;
this does not establish complete field presentation or global VM Draw ordering.

### Retained circle presentation

The shared outline uses 24 segments, the documented default circle precision
(the recovered game has no `draw_set_circle_precision` call). Reference:
[GameMaker circle precision](https://manual.gamemaker.io/lts/en/GameMaker_Language/GML_Reference/Drawing/Basic_Forms/draw_set_circle_precision.htm).
Q8 radii, Q15 unit vectors and integer line rasterization are a portable
adaptation; exact Windows GPU edge coverage and same-depth object ordering
remain unverified. Both backends currently place the field above static
terrain/shop fixtures and below moving actors.

The Amiga overlay retains sparse words separately for the two display rings.
Unchanged geometry on unchanged underlying rows performs zero framebuffer
writes. Changed geometry restores only departing coverage and directly
publishes changed final words; it does not clear continuing coverage first.
Dirty terrain/BOB rows are restored before the normal backing-row transfer,
then the outline is reapplied. The pristine terrain backing is never painted.
The caches and shared scratch require roughly 60 KB of Fast memory, no added
Chip allocation, and no large stack-local arrays.

The renderer fixture compares 512 changing-radius, horizontal-camera and
circular-origin cases against fresh scalar rendering, including dirty rows,
retirement and preservation of unrelated pixels. It explicitly requires zero
writes for unchanged frames. Address/undefined-behavior sanitizers pass this
fixture and the existing 22,834 offline sprite comparisons
(`/tmp/downwell-circle-sanitized.log`). The native clear verifier additionally
requires visible field frames and compares them before BOB drawing; host
comparisons alone are not proof that the live native path executes.

The final 90-second-bounded A500 check passes 192 visible field comparisons,
432 clear-screen comparisons, two clears and 60 third-level updates, with
zero pixel failures and the stack guard intact. Log:
`/tmp/downwell-field-final-clean-native.log`. Earlier instrumented runs timed
out in the byte-by-byte full-ring comparison; testing row damage once and
using the existing aligned comparison helper removes its per-byte division
cost. This is verifier overhead, not a claimed gameplay frame-time gain.
The full host suite and SDL build pass (`/tmp/downwell-field-final-tests.log`,
`/tmp/downwell-field-final-sdl.log`). Normal startup is restored after native
verification, with clean 68000 instruction and 64-bit-helper audits.

For the preceding Draw-RNG-only change, the 60-second-bounded A500 transition
check passed two clears, 432 planar
comparisons and 60 third-level updates with this corrected stream. The normal
build is restored and both 68000 audits pass. Logs:
`/tmp/downwell-field-draw-native.log`, `/tmp/downwell-field-draw-restore.log`.
The exact-radius/snapshot test also passes address/undefined-behavior sanitizers.

### Precise collision and timer

The shared live player phase now supplies the missing `goalTimeField`
overlap guard to the run clock on both SDL and Amiga. Original
`objPlayer_n` Begin Step tests `place_meeting(x,y,goalTimeField)` before
incrementing `global.gameTime`, and only later assigns `x=round(xx)` and
`y=round(yy)`. The query therefore uses the previous published player position,
not the newly integrated position used by shop observation.

Object 255 uses sprite 616 (`sprCircleArea`), a precise 128x128 mask with
origin (64,64); its Create assembly sets both image scales to 2. The animated
Draw radius is unrelated to this mask. `tools/generate-goal-mask.py` uses pinned
UndertaleModTool 0.9.2.0 to recover the actual mask, verifies the input hash and
mask hash, and losslessly compresses each row to an interval. All generated
payloads and provenance remain ignored. The query needs at most seven row
checks and no floating-point, square root, repaint or runtime conversion.

The player-phase test exhaustively compares 83,521 positions against the raw
mask, and checks timer behavior while crossing the mask and after retiring
the field. This corrects the implemented timer guard only: the field's wind
audio/associated RNG still needs original-game integration; Draw integration
is described above.

The focused test also passes address/undefined-behavior sanitizers. The bounded
60-second A500 clear/reentry check passes two clears, 432 planar comparisons
and 60 third-level updates; the ordinary executable is restored with clean
68000 audits. Logs: `/tmp/downwell-goal-field-native.log` and
`/tmp/downwell-goal-field-restore.log`. These checks validate the implemented
guard and existing transitions, not the still-missing field effects.

## Current extended-combat checkpoint

The historical tick-600/666/702 failures below are superseded by the current
shared-simulation work documented in `shared-simulation-migration.md` and
the git log. Both 6,000-update continuations now pass the combat and
death/survival checks with live fractional machine-gun spread: firing phase 6
finishes with health 0, kills 7, currency 74 and best combo 2 on both adapters;
phase 5 survives with 2, 6, 76 and 2. Phase 1 now survives to the exit, so it
cannot serve as a death-coverage tape. Previous totals from the vertical-shot
implementation and temporary ten-damage profile are historical evidence, not
an oracle to preserve after source-backed corrections. Phase 6 requires
at least 120 updates with both players dead; phase 5 explicitly requires
survival through 6,000 updates. The paired comparison checks
death/result state, input suppression, reward motion, HUD state and combat
outcomes throughout, not just those final totals.

`make verify-gameplay-death-state` runs both cadences. The combat loop also
requires exact equality of the published integer camera view, Q8 player
position and grounded state on every update, including death. Entrance and
traversal comparisons now require exact published positions as well; actor
subpixel adapter projections allow only 1/256 pixel. The old multi-pixel
allowances and asymmetric-landing skip have been removed, not widened.

This is host execution of both adapters, not a new native CPU replay or a
Windows recording. It establishes broader implemented-feature agreement,
not complete original-game fidelity. Existing source-boundary caveats remain
open; older failure descriptions are retained as investigation history.

## Default player horizontal movement

Inspected export: `extracted/utmt-0.9.2.0/Assembly/`.
`gml_GlobalScript_scrPlayerInit.asm` lines 313–355 initializes acceleration
0.2, ground maximum 2, air maximum 2.5, ground friction 0.4 and air friction
0.1. `gml_GlobalScript_scrPlMovement.asm` applies a reversal impulse of 2
when grounded versus 1 airborne, and explicitly zeros xsp when both direction
buttons are held. Its right-input branch runs last, setting facing right.

Both ports now distinguish these states. Native movement previously used the
airborne maximum, friction and reversal impulse, and treated both buttons as
neutral friction. Focused native tests assert the recovered ground maximum,
friction, reversal and immediate simultaneous-key stop independently of SDL.
Acceleration/friction still use rounded Q8 increments: this correction is not
a claim of exact integration or complete movement fidelity.

The corrected 64-frame native replay digest is `b112ca1f7174a6e1`. A changed
digest is expected when fixing movement; the source-backed assertions, not
the digest itself, establish why the behavior was changed.
The bounded 45-second A500 cross-target verifier passed all 64 frames with
that digest. Native player and port-runtime tests pass; the live entrance
comparison passes with the current in-progress collision changes present.

## Historical gaps at the initial combat checkpoint

This list records the initial investigation, not the current implementation.
Subsequent shared movement, collision-mask, bat/frog and animation work
supersedes several entries; use the current checkpoint and linked migration
notes above to assess remaining work.

- Player collision uses a landing resolver and corner probes rather than
  `scrCheckCollisionWith`'s rounded/ceil swept mask tests, including diagonal
  resolution and ceiling response. Fire/support ordering also needs review.
- Bat steering substitutes a coarse heading table and cross-product heuristic
  for the VM's point_direction, random_range and lengthdir operations. Hanging
  and hit-stun states are incomplete.
- Frog Step resets alarm 1 to 60 on the first grounded frame (`landed`), handles
  ceiling collisions, hit-stun and view gating. Native behavior is incomplete.
- Enemy/player contact uses a common proximity rectangle rather than recovered
  masks. A matching SDL result would not validate that approximation.
- Exact animation rates and sprite resource cycles are useful corrections but
  do not establish animation scheduling or all hit-state presentation.

At that checkpoint the extended combat comparison was a failing gate, not
proof of fidelity. Its current status is recorded at the top of this document.

## Fractional downward contact

Both `scrPlayerPlatformCollision` and `scrCheckCollisionWith` assembly compute
the vertical probe as round(yy) + ceil(abs(ysp))*sign(ysp). The SDL top-contact
helper now rounds yy, and applies to the shared top edge of walls/boxes as well
as thin platforms. At the observed (308.8,1984.2) floor contact, the old SDL
adaptation missed support and fired instead of jumping. Both ports now start
the extended tape grounded at y=1984. This fixes this contact case, not the
full two-axis/mask collision semantics listed above.

The startup test's first side room is now pattern 6 rather than 3. Generation
count 4, rows 95, one side room and zero shops remain pinned. Pattern 6 in
scrRightPat1 contains a J torch marker in addition to v,O,_,@,{,}; tests now
expect that torch and seven special markers. Pattern selection here is a
regression baseline for this adapted simulation, not a Windows reference trace.
The previous incidental kill before that checkpoint no longer occurs; an
explicit bounded 1800-step alternating-movement/fire continuation preserves
the mandatory kill/reward coverage (one kill, three gems, health 4 observed).

The extended SDL/native combat gate still fails: first >3-pixel player-Y
difference at tick 593, first kill mismatch at 601. Final health 3/3, kills
2/1 and currency 22/4. Do not widen tolerances to conceal this difference.

## Empty-ammo gunboot braking

The tick-593 divergence above was traced to the empty-charge branch, not a
floor collision: SDL reduced falling speed at tick 590 while Amiga did not.
`gml_GlobalScript_scrPlayerShootN.asm` lines 357–395 sets noAmmoYsp=3,
clamps a faster downward ysp, sets alarm 2 to 16 and enables shotDelay.
Amiga now implements that branch without creating a bullet or spending ammo.
Focused tests drain all eight rounds, verify the three-pixel fall displacement,
and verify that holding fire waits sixteen updates before braking again.
Smoke, EMPTY text and sound remain separate presentation omissions.
Full host tests pass. The bounded A500 canonical replay passes with unchanged
digest `b112ca1f7174a6e1`, and both 68000 audits are clean. That short replay
does not cover ammo depletion; the focused host test supplies that coverage.

After this correction the first >3-pixel Y difference moves to tick 666.
The kill mismatch at tick 601 remains even with player positions within a
pixel: native (172,2096), SDL (171.1,2096.7). Final health 3/3, kills 1/1 and
currency 12/4 still fail the extended combat gate. This supersedes the earlier
combat endpoint, not the outstanding mask/ordering concerns.

## Bullet muzzle coordinates

`scrPlayerInit` sets the default shootLeg to -1. `scrPlayerEmitBullet` adds it
to the supplied muzzle X and negates it on each emission. Amiga previously
always used -1; it now alternates -1/+1 for actual default-gun shots.
`scrPlayerShootN` captures instance x and y+4 before the Begin Step integrates
xx/yy. Amiga now captures rounded pre-movement coordinates instead of using
the post-movement position. SDL now uses the instance coordinates rather than
unrounded precise coordinates, with player x/y rounding using the same
ties-to-even helper as enemies.

Independent native tests check both alternating origins and a moving airborne
shot's pre-movement rounded origin followed by its eight-pixel bullet Step.
The canonical native digest changes to `bfbbd762b9b62a59`.
Full host tests and the bounded A500 replay pass with that digest; the 68000
instruction/helper audits are clean. The playable SDL build was rebuilt too.
These are default-gun corrections, not implementation of other weapon types.

The extended comparison still fails: SDL kills the later snake at tick 600
while native has not. The snake positions differ by one pixel (198/199 world
X), relevant to a bullet at the collision boundary. The inactive native bat
versus active SDL bat also needs investigation. Final health 2/3, kills 2/1,
currency 6/6: matching total currency does not establish matching combat.

## Frog landing timer

`enmFrog_Create_0.asm` initializes landed=1. `enmFrog_Step_0.asm` blocks
26–30 clear it while airborne, and on grounded && !landed set landed=1 and
alarm[1]=60. Both ports now retain this separate latch; a grounded flag alone
cannot preserve the original edge-triggered timer behavior. Native tests
remove support, restore it, require alarm 60 on landing and 59 on the next
grounded frame. The live comparison also checks the frog landing latch.
The added SDL digest field changes its startup checksum to `9a7487de074ed927`.

This changes the later frog's jump timing but does not close the extended
combat gate: the first kill mismatch remains tick 600, player-Y mismatch 666,
and final health/kills/currency remain 2/3, 1/1, 6/6. Full frog collision,
hit-stun, water and visibility semantics remain incomplete.
Full host tests, the live scene/latch comparison, and the bounded A500 canonical
replay pass. The target digest remains `bfbbd762b9b62a59`; that short tape does
not exercise frog landing. Native unit tests provide the landing-edge coverage.
Both 68000 audits are clean and the playable SDL build was rebuilt.

## Default bullets and ordinary walls

`bulletStandard_Step_0` calls `scrBulCheckSolid` before applying velocity.
The latter's assembly queries a segment against resource 59 (solid walls),
but bypasses this branch when the same segment intersects resource 83
(enemies). A normal non-wave bullet is destroyed by an ordinary solid wall;
breakable walls instead receive damage. Native code previously handled only
enemies and its explicit breakable list, allowing shots through ordinary walls.
It now checks the remaining vertical segment against its solid resolver.
Focused tests assert ordinary-wall absorption without a breakable record and
preservation of the existing enemy-line precedence.

This is not a complete translation of the original bullet system. Source
collision masks, overlapping wall/box selection, bullet direction randomization,
hit effects and the separate enemy collision event remain incomplete. SDL's
pixel-by-pixel first-hit ordering also differs from the source's enemy-line
guard and must not be used to override that evidence. The current extended
combat endpoint remains health 2/3, kills 1/1, currency 6/6 and fails.
Full host tests and bounded A500 canonical replay pass, with unchanged digest
`bfbbd762b9b62a59` and clean 68000 audits. The focused wall fixtures, rather
than that short replay, exercise the newly added wall-stop branch.

## Player gravity precision and the disputed snake hit

Tracing the later snake showed identical precise X/Y through tick 599. The
apparent one-pixel actor difference at 600 was a consequence of its earlier
death in SDL, not the cause: the native bullet was one pixel behind vertically.
Its muzzle had rounded differently after cumulative player gravity error.

`scrPlayerGlobalStat` initializes grav=.2 and maxgrav=7; `scrGravity_n` uses
those values for the default non-water state. Native player velocity previously
added 51/256 each update. It now retains signed fifth-Q8 velocity and position
remainders, including the -4.4 jump impulse and variable-height release. Landing
clears position residuals, while recoil, braking and knockback set the velocity
residual consistently. A 100-step exact-rational test covers integration and
terminal-speed clamping without using SDL as its expected-value generator.

The extended tape now agrees on the snake hit and initial reward collection.
Its first combat-state mismatch moves from tick 600 to 702 (health 3/4); a
player-Y difference still begins at tick 666. Final health 2/3, kills 2/1,
currency 18/6. Full combat remains a failing gate. This supersedes the earlier
snake-motion suspicion; do not alter snake speed to compensate for muzzle drift.
Full host tests and the bounded A500 canonical replay pass; the short replay
digest remains `bfbbd762b9b62a59`, with clean 68000 audits. It does not cover
the long-tape drift; the rational integration test and traced combat continuation
provide that evidence.
