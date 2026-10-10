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
Native title identities also participate in ordinary and cached actor Draw
traversal at their compatibility creation depth (-100000), not the OBJT depth
(0). Both worlds expose an unconditional surface traversal that preserves title
and entry-cover depth order separately from conditional drawFx particles. The
compositor dispatches that pass during death/low-spec rendering, but not when
the effect surface is absent. Both backend surface paths reject an unbound live
title instead of mistaking its family-local handle for another effect.
`Game::renderMain` now paints bound titles through the shared surface adapter:
`withLevelNumberText` borrows the glyph bank and current text state for an
enclosing render, independently of the stored actor-registration binding. It
reads the host world's current camera on each source view access and emits
surface-local glyph coordinates. The actual Game render test compares published
surface words to a per-pixel bordered-glyph reference, checks Draw-only letter
movement/final color, then verifies scope exit rejects further unbound drawing.
This explicitly supplied state is not automatic admission of natural callers.
The native draw list likewise borrows a glyph bank/current text state, collects
no title BOB and does not advance text during collection. The unconditional
production surface callback invokes its shared title adapter. Currency hardware
promotion declines while any registered title exists, preserving later title
occlusion of earlier gems. Tests cover reference/native/native-scenery owners,
including death-mode composition and a complete per-pixel border/glyph check.
The native scene must still supply live text state before natural admission;
this binding does not initialize that state or allocate source IDs.
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

Both worlds expose `beginLevelCueWithTitle`: the shared cue synchronously creates
the registered object 372 and initializes its localized text using the supplied
font pack before firstLand or audio publication. `LevelNumberTextContext` borrows
language, font ID and hard-mode settings; these and the real run stage are read
inside child Create after positioned registration. The stored world consumes its
normal source-ID stream; native callers must supply the enclosing allocator.
Invalid font measurement retains the published child but fails the cue before
firstLand/audio. This adapter is tested with both world owners, not yet installed
at the natural enemy-death/first-landing call sites.

`generate-title-glyphs.py` prepares all 412 glyphs from font resource 2 as
word-padded, MSB-first alpha masks (8,180 mask bytes, plus descriptors and a
shared zero plane). It verifies every covered atlas texel is white and alpha is
binary, then checks all packed pixels and padding. The generated manifest pins
the extraction manifest, atlas, source build, generator version/hash and output.
`FontGlyphMasks` borrows this immutable ordinary-memory bank; white uses mask/zero
color planes and opaque black aliases both planes to the mask. Glyph bearings
are retained; missing glyphs use U+25AF and then no ink. Other font resources and
colors are rejected, not thresholded or substituted.

`LevelNumberSurfaceDraw` joins those glyphs to the shared event for the admitted
left/center/right horizontal and top/middle/bottom vertical, opaque text state.
It borrows current font/alignment/alpha/color rather
than setting defaults, retains the helper's final white color, and handles each
single-token hash/newline/NUL as no ink. The compositor's surface-local emitter
avoids a second camera subtraction and uses its supplied raster policy. Tests
compare the entire composed title against a per-pixel border/glyph reference at
large nonzero camera coordinates in normal, death and low-spec modes; absent
surfaces retain offsets. Unsupported text states fail with the source mutation
prefix retained. This still requires live renderer-state/target binding and
verification of the target raster policy before admitting natural callers.

The complete exported VM font-setter inventory has nine entries. Initialization
(`scrInitialize`, block 54) selects resource 2. The other eight are Draw events
of `Obj_YoYo_textbox`, `Obj_YoYo_text`, `Obj_Steam_UGC`,
`Obj_Steam_Leaderboard_Entry`, `Obj_Steam_StatsInfo`, `Obj_Steam_StorageInfo`,
`Obj_Steam_OverlayInfo` and `Obj_Steam_GeneralInfo`. Neither these objects nor
their descendants occur in the authored instances of any non-`Room_Steam_`
room. The verifier checks the full writer set and transitive parent closure.
This narrows live font-state admission to dynamic births/event calls and the
initialization handoff; it does not prove those events unreachable or justify
resetting the font at each title Draw. Alignment and alpha are separate state.

`verify-runner-text-draw.py` pins draw_text registration/wrapper 0x5323A0 and
alignment setters 0x45A9C0/0x45AA00. Horizontal/vertical alignment are globals
0x9359BC/0x9359C0, not title-local defaults. The VM playerDraw sets horizontal
center in block 5 with no explicit restoration; the title event sets neither
alignment, font nor alpha. This does not establish the final live state: intervening
Draw calls still need tracking. The draw_text wrapper narrows X/Y with FSTP m32;
0x459D9C subtracts signed-truncated half line width for center or full width for
right, with a binary32 addition. The shared adapter now preserves those two
rounding boundaries, using offline unit-font advances for single tokens. Tests
include values beyond 2^24 that distinguish input narrowing from integer-only
placement. The font loader at 0x44A077 initializes the height to zero and keeps
the maximum signed glyph field at +8 (serialized sourceHeight). The height
accessor at 0x44A280 multiplies this integer by binary32 scaleY and truncates;
its apparent "M" string argument is unused. The single-token title domain thus
uses the whole font's 10-pixel line height, not each glyph's visible bounds or
the 12-pixel spacing used by some offline multi-line text. The vertical path at
0x459CEC subtracts half/full total line height for middle/bottom, after input
narrowing and with binary32 subtraction. All nine alignment combinations are
tested. Final glyph-quad raster sampling remains unverified beyond the existing
integer placement cases.

The atlas glyph path at 0x44751F loads the signed bearing from glyph +0x0C,
scales it and adds it in binary32 before building the textured quad. Applying
the bearing as a compositor integer origin after rounding is not equivalent:
for glyph `1`, X=0.5 plus bearing 1 reaches 1.5 before sampling. The shared
adapter now performs this addition and emits a zero-origin asset. The regression
checks both the emitted real coordinate and the existing nearest-even sampler's
result. This proves the ordering correction, not the original GPU's sampling rule.
Direct FONT verification also pins font 2 scaleX/scaleY=1, the baseline-related
fields at +0x28/+0x2C=0, 412 glyphs and maximum height 10. The baseline read at
0x447289 therefore contributes zero for this font. Expanded quad borders,
texture filtering and device raster rules still require evidence before claiming
fractional-placement parity with Windows.

The stored runner's `withLevelNumberDraw` borrows the title registration only
for its enclosing render, while `drawLevelNumber` resolves the original room
record back to that registration at the layer callback. The synchronous ops
bridge forwards target, view, color and glyph calls without copying event state.
Nested binding and unbound dispatch reject; the binding is released on both
successful and failed rendering. The stored-world lifetime test exercises this
through the unconditional layer traversal, including absent/present surfaces,
Draw-only letter movement and the retained 142-tick alarm/destruction sequence.

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

### Native generated-birth boundary

The dynamic creation stream cannot be attached only to the terrain sink.
`AmigaGameplayGeneration::create` takes several different routes into
`GameplayWorld::createGeneratedInstance`:

| Birth | Current native publication boundary |
| --- | --- |
| Wall 60 / platform 93 | Direct backend publication in native mode; scalar, run and prepared paths converge on `birth` |
| Terrain 99 | Generic terrain callback followed by terrain Create |
| Breakable 92 | Prepared breakable publisher or generic `appendBreakables` path |
| Shop 277 | `registerShop` returns before the terrain callback |
| Time field 82 | `adoptTimeField` returns before the terrain callback |
| Side camera locker 62 | Its family Create returns before the terrain callback |
| Level exit 306 | `registerLevelExit` runs before the terrain callback |
| Ordinary enemy | Terrain callback precedes `configureEnemy` |

`birth` currently appends dynamic terrain without a source numeric ID, and
the prepared breakable publisher calls `appendGeneratedBreakable` with its
default zero ID. The families' lifetime handles do not repair this omission.
Merely supplying IDs to those two calls would skip the other creation routes
and any synchronous children. An allocator at the terrain callback would
therefore produce a different numeric stream even if every terrain ID were
unique. Likewise, assigning an ID after `registerShop` or `registerLevelExit`
is too late to establish the parent's identity before its Create effects.

The source contract is one allocation before construction, followed by
synchronous Create with that same allocator available to children. Authored
room IDs do not advance it. The stored `DeferredInstanceQueue::applyChecked`
retains the allocated ID locally and advances the shared cursor before its
Create handler, so a caller must not recover the parent's ID as cursor minus
one after that handler returns. Native generated-birth integration must
preserve that boundary across the table above, not infer a stream from the
terrain representation or a batch's final population.

`GameplayWorld::createGeneratedInstanceWithSourceIds` is the checked native
handoff for this stream. It borrows the allocator by reference, publishes each
allocated terrain ID to the backend and reference terrain owner, and retains
IDs on boxes, ordinary enemies, fields and lockers. Field commands adopt their
already-sampled radius without consuming Create RNG again. Exit registration
borrows the same allocator for its parent and children; shop registration must
be supplied by its enclosing concrete owner and cannot fall back to compact
stock. Unknown families and failed allocation/publication stop the handoff.
`AmigaGameplayGeneration::withSourceCreation` routes scalar, run, prepared
terrain and prepared-box births through this handoff while borrowing the
enclosing allocator. Runs and prepared terrain use their ordered scalar
publication within the scope so nested births/failures observe each prefix.
This does not supply a startup cursor: normal scene admission, concrete native
shop ownership and the complete room-wide stream remain required.

Object 289 is `Scattered16` (parent 90), not a spike. Its native creation
owner retains the builder's chosen scale/frame without repeating RNG. It
publishes birth, image index zero, X scale, sprite 550, mask 550 and the chosen
image index in Create order; inherited shootable state precedes the child's
hitEffect=1. Numeric identity joins the resource-backed room/layer/spatial
owner and lifetime generations survive Retry. Creation ownership alone does
not admit a complete scenery actor.

The Scattered16 Step calls out-of-view destruction before its health/active
test, without returning after that call. On break it creates currency (2),
attracts only `myGem` when stomped, creates two debris instances with a -0.5
Y-speed setter after each synchronous Create, chooses sound 171–174, plays it
at volume 50, changes sprite to 551, then clears the script variable `active`.
That last write is not runner deactivation. Neither mask nor image index is
reset. Player contact first tests descending motion, sets HP/stomped, bounces
with 1.5 for style 2 or 2.5 otherwise, shakes (2,4), then optionally creates
Exploding Knees blast 276 before choosing its sound 91–93. Alarm 0 clears
hitStun. `ScatteredEvents` preserves these event boundaries; its effect adapter
must perform real source-ordered child births and setter publication.

`GameplayWorld::stepRegisteredScattered` composes that Step with registered
currency and published debris creation, retaining reward/script-sound fields
on the caller. It requests each child ID from the borrowed room cursor at the
birth boundary. Sprite and deferred-destruction publication use the scenery
lifecycle binding; the room cleanup retires the marked record. Native and
reference collision-owner tests cover this chain, but do not establish normal
scene admission or the stored SDL adapter. Player velocity comparison uses the
runner's real-comparison epsilon, including tiny positive speeds that compare
equal to zero.

The stored runtime uses the same `ScatteredScriptState` and event functions.
Its prepared builder entry retains the chosen frame/mirror without new RNG,
and publishes the explicit mask 550. `withScatteredEvents` dispatches Step and
alarm 0 through the ordinary room scheduler; it composes registered currency,
owned published debris, audio and sprite publication. New debris must join
the same tick's later phases (`stepWithOwnedDebris`), not a separate particle
tick. The stored-room fixture verifies this composition and subsequent source
layer traversal. The scope still requires installation with natural player
contacts, child lifecycles and production audio/settings before normal scene
admission; fixture stomp callbacks do not implement player damage/bounce.

`RegisteredScatteredContact` retains the caller's `scrFjump` and `scrSShake`
fields, reuses the shared forced-jump script, and creates Exploding Knees through
the registered blast owner before choosing sound 91–93. Unlike enemy stomp it
does not check player death, emit blood/smoke or a stomp sound, or recharge a
second time. The native collision entry validates resource event 2388 and both
lifetimes. Player selector writes and synchronous recharge effects are explicit
borrowed dependencies; the compact `stompBounce` helper is not their substitute.

`registerLevelExitWithSourceIds` supplies that boundary for the exit chain:
306 is allocated and registered before its Create; 74 and 255 each request
their own ID immediately before child registration. Exit-controller and goal
field records retain these IDs, including layer publication. No contiguous
three-ID reservation or post-Create cursor subtraction is used. Allocation
failure retains the completed prefix, stops later births and prevents goal
field RNG when that field was not born. The enclosing generated-instance
caller still needs the complete source stream; the legacy no-ID entry point
does not establish it.

Side-camera lockers (62) retain generation-checked lifetimes in their existing
coordinate store. Registration observes the supplied birth position first;
Create then publishes the X setter to 80 when x < 160, or 400 when x > 320
in rmMain. At either boundary no X setter runs. The native collision owner
binds this lifecycle for normal generator calls, including source-ID layer
publication when an ID is supplied. Teardown retires before invalidating the
record; Retry preserves the generation cursor and owner binding. Birth,
position-publication and retirement failures are terminal. This registration
does not replace the existing camera-contact adapter with the full ordered
collision dispatcher, nor supply the missing global source-ID allocator.

### Shop creation and the shared numeric stream

Shop 277's VM Create calls inherited Create, starts/mutes the area-specific
radio when applicable, then creates these children synchronously:

| Order | Resource | Birth position relative to Shop |
| --- | --- | --- |
| 1 | Table 278 | (x+64, y) |
| 2–4 | Case 279 | (x+45/64/83, y−9) |
| 5 | Invisible wall 98 | (x, y+8) |
| 6 | Keeper 280 | (x+18, y) |
| 7 | Rising text 212 | (−100, −100), absolute |

Stock selection and rejection-sampling RNG follow those births. The next
three children are item 283, created in case order at each retained case's
current x and y+1, not reconstructed from the Shop origin. Each item completes
Create before Shop writes its `whichUg` and the corresponding case's
`storedItem`. Only after all three does `scrShopAssignText` choose the farewell
text. The hash-pinned evidence is in `verify-runner-instance-ids.py`; it does
not prove inherited events or either backend's live implementation complete.

Both existing generation adapters bypass generic instance creation for Shop
and call the compact `ShopWorld` representation directly. That representation
retains stock/visual state but not the parent plus these ten registered
lifetimes. Thus a complete dynamic numeric stream cannot be recovered by
numbering the existing terrain and enemy records alone. Registering the
children after all stock/text RNG would also lose synchronous Create ordering.

`ShopChildCreation` retains the returned IDs and the two child phases.
`ShopWorld::createRegistered` composes them around stock generation and before
farewell selection. Its required owner registers the parent and performs
inherited setup, completes each child Create, reads actual retained
positions and performs the item/case field writes. Missing or rejected work
fails the enclosing status, retains the accepted prefix and prevents later
births/RNG; the phases cannot be retried. The default compact creation path
remains separate until the frontend supplies every required child owner.
`GameplayWorld::registerShopWithOwner` supplies the native world's actual RNG,
health, area and membership ownership to this same registered path. It can be
called by the generator's Shop callback while borrowing the enclosing numeric
stream; it does not allocate a second stream or substitute compact creation
after failure. This entry does not itself supply native child lifetime owners
or admit their later events in the frontend scheduler.
The shared registered path itself starts sound 192 (201 in area 3) at priority
100 with looping enabled, retains the returned handle and immediately sets
its gain to zero with zero transition time. Area 5 retains radio=-1 and makes
neither call. Rejected playback/gain publication stops before the first child
or stock RNG. The inherited parentInteractable Create has no calls, hence no
additional births or RNG; both that body and the radio branch are hash-pinned.
The parent lifetime event entries in both owners use `ShopLifetimeEvents`.
End Step reads view Y once for the strict lower bound (Y−320), and again for
the upper bound (Y+3000) only if needed. Outside those bounds it reads room,
and only in room 11 queries/stops the current radio and marks the parent for
destruction. Other 5 (Room End) only queries/stops radio, without destroying
the parent or children. Radio is reread for stop after the playing query.
Publication failure prevents later effects. Native registered parents join the
existing mixed End Step snapshot and pre-Draw retirement; children remain in
their own lifetimes. `withShopRadioEvents` supplies required playing/stop
callbacks, and an out-of-view parent without that scope fails instead of
silently skipping radio shutdown. View Y comes from the same live world End
Step value as the other registered actors. Stored parents join End Step and
deferred cleanup through `withShopParentLifetimeEvents`, using the published
view captured for source events, not the camera's later render position.
Their retired parent entry is released separately from the retained children.
The stored event adapter also recognizes Other 5, but both room-transition
dispatches and concrete device-audio bindings remain to be connected;
registered-world tests do not establish normal frontend shop retirement or
the parent's still-unimplemented ordinary Step.

`StoredShopChildRegistration` implements these six child resources' synchronous
Create bodies in real `InstanceStore` records, borrowing the room counter and
lifecycle publisher. Birth publication precedes Create setters; item image-index
publication precedes its zero image-speed assignment. The retained side state
holds material, case health/flags, keeper flags, item selection and rising-text
variables; the latter's alarms are in the instance's actual alarm bank. Teardown
retires records before invalidating their handles, without resetting the shared
counter. The source check includes each child's PreCreate ancestry and verifies
that the invisible wall's ancestors have no Create body to execute. This owner
does not itself admit the children's later Step, Alarm or Draw events, and is
not yet installed by either normal generation adapter.

`GameplayShopInstances` retains the same parent and child lifetimes in a native
generation pool, with binary32 builtin values represented as words rather than
host floats. Its borrowed creation adapter takes the enclosing allocator and
audio owner; it never reserves IDs ahead of child Create. `ShopChildState`
supplies the same six child Create bodies to both owners. The native pool
publishes positioned births before setters, retains rejected prefixes, and
does not reuse generation handles after teardown. `GameplayCollisionOwner`
binds its positioned births and image setters to the shared collision tables
and source-ID-bearing room/layer lifetimes in reference and native modes.
World room teardown clears the pool before resetting compact Shop state, and
Retry preserves the binding without preserving old generation handles. This
registration does not project their Draw or admit the parent
events; those are required before normal frontend generation admission.
Rising text and item selection can join `withEncounterSteps` through
`stepRegisteredShopChild`, with the caller supplying the real upgrade-array
lookup. Rising-text alarms use the existing mixed alarm phase; alarm 0 marks
the record for the existing pre-Draw cleanup pass rather than erasing it in-place.
Both owners execute the same `ShopChildState` Step bodies, including two
upgrade-array reads when the item's sprite changes and the text's Y setter
before velocity damping. There is no independent shop-family tick.
Native shop sprite changes resolve frame counts from the complete collision
metadata and apply `RunnerSpriteChange` to the owning record as well as its
collision publication. A shorter or absent sprite can implicitly reset the
image index; same-ID writes retain it and an explicit collision mask is not
replaced. This reset belongs to the sprite setter, not a second image-index
publication. Keeper sequence advancement uses the same bounded unit-track
owner on both backends, dispatched before alarms and Step. The five recovered
sprites have 4/5/2/4/2 frames and unit per-game-frame playback. The only explicit
index writes in these keeper events are zero and blink frame one on the
two-frame angry sprite. The runner inverse mapping at `0x50ed71`–`0x50ee09`
reduces exactly for these values (zero or the exactly representable half-cycle);
other inverse writes are rejected rather than approximated. Changed-sprite
resets also reset the retained timeline. This admission does not implement
the other shop objects' animation or Draw paths.
Registered case Step and Alarm 0 share `ShopChildState` event bodies on both
owners and join their existing mixed phases. The source user variable
`destroyed` is not `instance_destroy`: an opened case remains registered. Its
opening writes `opened=1`, calls `soundPlay(10,50,0,1)`, writes frame 2 and
mask 582, invokes `scrFlashballfx(x,y,3,0,5)`, then writes `global.criminal=1`
only if `storedItem` is set. Required effect/global callbacks cannot be omitted;
failure retains the accepted prefix and latches the enclosing status. Alarm 0
resets image/flashing only before opening. The bullet event only sets health
to zero when the other sprite is 107; it does not itself set `destroyed`.
There is no evidenced health-to-`destroyed` transition for this case in the
pinned build. The complete hash-checked 2,501-root assembly corpus writes
`destroyed` only in the three Creates (case, gem chunk and gem sphere) and the
two gem Steps. Case ancestry is 279 → 57 → 56, with only PreCreate on the
ancestors. The corpus has no dynamic variable setters, eval, instance changes,
explicit event dispatch or indirect script/method calls that supply a missing
case write. `verify-main-room-lifetime.py` guards these facts. Do not transplant
the gem health-driven destruction path into shop cases. Both room tests retain
the closed case after the bullet event sets health to zero; the explicit flag
write covers only the dormant opening branch, not natural breakage. Ordered
bullet contact admission remains open. Sound/flash/criminal callbacks preserve
the dormant branch but are not evidence of a missing natural damage caller.
Registered keeper alarms use the same shared handler body in the native and
stored owners. The native mixed dispatcher advances each slot immediately
before its handler and takes `irandom_range(20,60)` from the world RNG; rejected
image publication stops before rearming or drawing randomness. The stored
entry executes the handler only, after its room dispatcher has decremented
the counter. `ShopKeeperEvents` implements the ordinary Step's ordered setters
and retained reaction flags. Its native world entry reads source player-X and
area from the live world, narrows builtin writes, and borrows world RNG. It
joins the mixed Step capture at object 280, not an independent family pass.
Criminal state and synchronous enemy-284 creation are explicit dependencies;
destruction precedes replacement Create and does not suppress the event tail.
`StartupRuntime::withShopKeeperEvents` supplies the stored world's player
globals, area and RNG to that same Step and admits its alarm handlers and
pre-Draw retirement through the existing room dispatcher. The owner scope
requires a criminal-state reader and synchronous replacement creator; failure
retains completed writes rather than inventing the replacement or undoing the
destruction mark. Native keeper animation and normal frontend installation
remain separate requirements; these handlers do not make the keeper a complete
actor.

The rising-text event entries publish the Y setter before multiplying binary64
`ysp` by `decreaseBy`. Alarm 1 sets `flashing`; Step then negates the integer
`drawing` field (it is not a boolean toggle). Alarm 0 publishes a destruction
request; storage survives until the enclosing retirement phase. These entries
do not invent a time-stop guard absent from the source. They skip inactive or
already-destroyed records and retain completed mutations on publication failure.
Draw and automatic frontend admission remain separate from those event entries.
`StartupRuntime::withRisingTextEvents` admits object 212 to the room's existing
alarm and ordinary-Step dispatch and pre-Draw cleanup. Its alarm callback only
executes the event: the shared alarm phase has already decremented the actual
record, unlike the standalone countdown adapter. The borrowed owner must match
the room and outlive the scope; absent ownership fails when an event fires.
The registered room fixture checks tick-60 flashing and tick-90 retirement
through the collision and layer owners, without admitting other shop families.

Rising-text Draw measures hash-to-newline text with the current font, retains
`stWidth`, `xxx`, `overXview` and `underXview`, and gives the right-edge adjustment
priority over the left-edge adjustment when text exceeds both bounds. It sets
horizontal alignment to centre even on a hidden flashing frame, then restores
left alignment. The shared Draw entry delegates the original four-black-pass
border text operation. Odd widths retain
half-pixel placement rather than rounding before alignment.

The shared `Game` layer walk dispatches object 212 through
`withRisingTextDraw`; an unbound live text fails rendering rather than silently
vanishing. `RisingTextRenderer` borrows current font/alignment/color state and
the matching original glyph metrics, converts hashes to newlines, and emits
four complete black text passes followed by the white pass through `Renderer`.
It projects world coordinates into the gameplay viewport. Its currently
admitted text domain is bounded ASCII, full opacity and top vertical alignment;
other states reject rather than use invented defaults. The normal frontend
still needs to install the owner and renderer scope alongside the other shop
families. The Amiga scene frontend does not yet bind this route.

Item 283's ordinary Step reads `global.ug[whichUg][2]`, compares it with
`sprite_index`, and reads the table again only when the setter executes.
`withShopItemEvents` installs that source-table reader in the existing mixed
Step phase. Its registered setter publishes the new collision sprite without
resetting Create's image index or speed. Item 283 has no custom Draw event:
`Game` draws its retained sprite/frame/transform at its layer position, and
`drawShopFixtures` omits an item's compact copy when its source child ID exists.
Player collection (`scrGainUg`, sound and destruction), the remaining shop
families and automatic frontend admission are not covered by this Step scope.

The selected Windows numeric truth helper at `0x56a390` dispatches real/bool
payloads to a strict comparison with 0.5, signed int32 to `SETG` against zero,
and signed int64 to an equivalent high/low-word comparison. Thus both numeric
representations of -1 are false, unlike implicit C++ boolean conversion. This
is the relevant +/-1 `drawing` domain; it does not claim support for every VM
value type. `verify-runner-numeric-truth.py` pins the helper, its type table and
threshold against the original executable hash.

`StoredShopCreationOwner` is the concrete owner accepted by
`ShopWorld::createRegistered`: it registers resource 277 in that same room
store/counter, retains the inherited interactable state and delegates child
operations to `StoredShopChildRegistration`. Its required audio adapter is
borrowed; playback/gain rejection propagates to the same failure state used by
the room owner. The parent record and compact Shop state remain linked until
explicit teardown, which must precede `ShopWorld::reset`. This supplies the
creation boundary, not automatic admission of later shop events.

`StartupRuntime::withRegisteredShopCreation` borrows that owner around the
existing `registerShop` entry used by the builder. It rejects nested or foreign
room owners and restores the dispatch binding on scope exit. `RunSession`
supplies the same health, area and membership-card pricing context as compact
creation; the room supplies the actual RNG and numeric counter. Positioned
births require a real lifecycle owner, not a birth-then-move fallback. The
normal frontends do not install this scope until the later event paths are
admitted.

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
