# Usual-gun motion arithmetic

The authority is object 261's `bulletStandard` Step assembly, not host libm
or the previous fixed-point trajectory. `verify-machine-gun.py` checks both
usual-gun profiles (speed 8, spread 3), Laser Sight's birth-time override,
initialization, muzzle Create and solid-query/movement order against the pinned
UTMT 0.9.2.0 export.

The first Step computes `bDir += random_range(-bdirRand,bdirRand)`. The reviewed
`random_range` wrapper consumes one word even for zero spread. Width 6, lower
bound -3 and addition to 270 are retained as separate binary64 operations.
`bDir` remains a custom real; `image_angle` is separately narrowed to binary32.

Both lengthdir wrappers narrow speed and degrees to binary32, multiply degrees
by float pi, divide by float 180, evaluate the recovered trigonometric program,
narrow its result, multiply by speed and apply the original tiny-fraction snap.
`verify-runner-smoke-geometry.py` pins those wrappers/constants;
`export-smoke-trig.py --check` pins the shared program. These names reflect the
first consumer, not different GameMaker functions. The admitted speed interval
extends through 8; the existing angle/radian restrictions are unchanged.

`MachineGunSourceMotion` preserves absolute-room binary32 x/y and binary64
xsp/ysp. `endpoint()` returns the VM's promoted x+xsp and y+ysp expressions.
The `collision_line` wrapper at 0x528ba0 independently narrows all four inputs
with `fstp dword` before dispatching 0x4fd370. `lineArguments()` models this
boundary without moving the bullet; candidate geometry must use these narrowed
arguments, not the intermediate double sums. `commitPosition(publish)` models the later coordinate arithmetic,
including separate X-before-Y binary32 writes on every update. Its publisher
receives each intermediate position before committing that axis to the motion
owner. A failed second write retains the completed first write; the enclosing
world must latch failure instead of retrying the event. Translating to the native local origin before narrowing is not
equivalent. Backend offsets must be applied after original coordinate storage.

`initialize(random,publishAngle,muzzle)` publishes angle before synchronous
muzzle creation and sets `allSet` only after both callbacks succeed. The
arithmetic-only overloads supply no-op publishers; they are for fixtures,
not a substitute for live ownership.

After wall-line selection and the enemy-line exclusion, `scrBulCheckSolid`
stores `thatWall` and initializes `whileStop=0`. While its point query misses
parent solid 59 it writes Y += ysp/10 followed by X += xsp/10, narrowing at each
setter. It increments the counter and breaks only when it exceeds 100: up to
101 iterations, with no further point query after that final iteration.
`approachSolid` reproduces this sequence with explicit query/publication
callbacks. Division by 10 must not be replaced by multiplication by rounded
0.1. Wall damage/effects/destruction follow the loop; final X/Y movement still
runs after a destruction request. Selector identity, those side effects and
deferred retirement belong to the scheduler, not this arithmetic component.

`BulletSolidScript` transcribes the branch/side-effect sequence around those
operations. It performs the wall-line test, short-circuited enemy-line guard,
fresh wall selection and approach before testing the wall's **direct** parent
against 84. Positive health is reduced without clamping before `hit=1`; a
destruction request does not skip the later explosion branch. Non-breakable
walls create the configured effect at speed 0.5/angle 0 before sound and
destruction. The alternative branch separately performs parent-90 shape and
identity queries, damage/hit, optional effect 605 at speed 0.4 and the retained
real direction, then non-consistent destruction. It never runs both branches.

The callbacks own actual target identities, audio, effects and source storage.
The shared script latches failures and retains completed operations; invoking
it again on a failed runtime cannot replay damage or creation. It is not yet
the live gun Step: complete parent-90 actor admission, target writes/effects and
collision-event dispatch remain prerequisites. `verify-bullet-solid-script` checks 1024 branch
combinations and every callback failure boundary against the pinned VM flow.

The existing cavern builder's box choices are retained in
`DeferredCreateInstance::boxCreate()` using object 92's otherwise-unused
auxiliary payload. Sound 171..174, gem kind 0/2 and the associated frame are
chosen once by the builder. Both backend records adopt `BreakableState`
without consuming RNG again; raw resource-less fixtures use its defaults.
It retains health 1, break sound, gem kind, drilled/autoTile/checked flags and
hit-write storage. This is not the full box Create/Step implementation:
non-cavern/glitch/ground-room variants, original damage/destruction timing,
effects, audio, chain reactions and gem/debris emission still require wiring.

`BoxDestructionScript` transcribes object 92's Step body, separately from a
bullet's damage write. Health <= 0 causes overlap-limited sound, smoke at Y+4,
flash at Y, and sprite 603 at Y+12 (speed 0.5, angle 270, depth -60000), in that
order. Chain reaction creates object 266 synchronously, publishes `myBullet`,
then direction 90 and damage 20 before sound 342. It queries adjacent boxes
up/right/down/left, publishes `target` even on a miss, sets alarm 0 to 5 on
hits, and clears `checked` when auto-tiling is enabled. Global chain-reaction
reads are retained across callbacks rather than cached before child creation.

The destruction request precedes object-370 debris Create: one child in
low-spec mode, two otherwise. Area 4 suppresses gems; other areas spawn 3 or 10
for gem kinds 1 or 2. `alarmStop(0)` runs last, including for intact boxes.
Callbacks must execute the complete child scripts synchronously and retain
the destroyed parent until the event finishes. The helper stops at the first
failed callback without rollback or retry; it does not implement child
lifecycles or attach itself to the live scheduler. In particular, the existing
immediate box-inactivation path still needs replacement. Run
`make verify-box-destruction` for the pinned assembly and branch/failure tests.

The smoke and flash owners expose `boxBurst` for the box script's fixed
arguments, sharing their burst implementation with skeletons. Count variation
zero still calls `irandom_range(0,0)` and consumes two RNG words. Smoke samples
Y then X via five-way choices before object-225 Create; flash samples Y then X
over [-2,2) before object-224 Create's own integer jitter. The caller supplies
the already-offset Y+4 for smoke; these owners do not add it a second time.
Both finish each child Create synchronously and retain their normal phase
cutoffs and animation/motion lifecycles. Burst tests compare full RNG state,
including failed capacity cases; owner tests exercise these actual box births.
These entry points still require wiring to the box event's live callbacks.

`BoxDestructionIntegrationTest` runs the event through actual smoke, flash,
script-impact and debris owners with one shared RNG, across all five area
sprite choices and both low-spec modes. Independent host arithmetic checks
the ordered child positions, velocities, animation values and final RNG state.
An exhausted second-debris slot retains the first child and all earlier
effects, skips alarmStop and cannot replay the event. This fixture deliberately
has no gems or chain reaction, and records the sound callback without an audio
backend. It proves particle creation composition, not live room registration,
cross-family scheduling, audio, reward creation or final pixels.

The stored and native collision owners expose `projectileWallParent` and
`projectileBoxHealth`/`projectileBoxHealthWrite`/`projectileBoxHit` for the
selected lifetime. Parent lookup uses the direct serialized parent, not an
ancestor test. Each access resolves the selection again; no record pointer is
retained across callbacks. Health is binary64 (including fractional overkill),
and health/hit writes do not change activation, collision membership or RNG.
Native marked boxes remain accessible until room cleanup; retired/reused
handles are rejected. These are target adapters for the pending source bullet
integration, not a change to the current live gun's immediate removal path.

`ShootableThing` supplies parent-90 Create fields to both bones and corpses:
`objHp` as a custom binary64 real, including fractional overkill, and the
`hit`/`hitEffect` flags. The
bone Step applies the runner real comparison for `objHp <= 0`, after motion
and only outside time-stop. Corpse Create overrides health to zero without
Rest in Pieces; with it, health remains one and hit effects are enabled. The
corpse's unpaused Step with that upgrade restores health to one while its
private `active` flag is false, before testing health for explosion. Neither
private activity nor damage writes substitute for engine activation.
`StoredBoneRegistration` and `StoredCorpseRegistration` expose selected-world-
handle reads and health/hit writes; the native collision owner dispatches the
corresponding `projectileShootableFields`, `projectileShootableHealthWrite` and
`projectileShootableHit` operations to the selected bone or corpse owner, failing
on unsupported descendants. These resolve the lifetime anew on each access.
They do not invoke a target event or alter engine activation, membership, RNG or
destruction. Marked and inactive targets retain writable fields until cleanup;
retired handles and unsupported numeric values fail. Parent-90 selection and
natural bullet dispatch remain distinct from this target-field contract.
The bullet solid script's positive-health guards also use the runner real
comparison; a value within the zero epsilon band must not take damage merely
because its sign bit is positive.

`MachineGunSourceStep` composes the source motion initialization, synchronous
`scrEffectSpawn` muzzle sequence, solid script and ordered final X/Y setters.
It uses the shot's birth profile (ordinary or gem-high), the inherited
`hitWallFx=101` default, and the raw spread direction for impact effects. The
same registered effect operations handle muzzle and impact births, including
the caller's `myFx` assignment and the four ordered setters. Impact coordinates
are read after the approach loop and before final movement; depth is explicitly
zero. Backends do not supply an alternative impact-effect implementation to
this Step. The standalone solid-script interface retains its callback for
isolated branch tests.
Both worlds' `bulletEffectOperations` bind those births to the room's effect
animation/retirement owner and write `myFx` on the actual retained bullet before
the child's sprite setter. The stored world keeps the source numeric ID; the
native world keeps a generation-checked internal effect handle, not a fabricated
source ID. Each assignment resolves the bullet again after synchronous creation.
Later child-setter failure leaves the completed birth and caller assignment
intact and latches world failure. The effect owner and registry must remain at
stable addresses while their registered records exist.
`withBulletEffects` supplies the world's own shared effect pool and registry
to a synchronous Step execution; it never creates a bullet-only pool. This
allows a retained muzzle to coexist with a later bone impact. Both world-owner
fixtures select and damage a registered bone through the source solid script,
retain it while time-stopped, then execute its resumed Step and create sprite
85 alongside the muzzle without further RNG. These are controlled event-chain
tests with explicit target births and supplied stop state, not natural skeleton
encounter admission or a proof of complete global RNG interleaving.
`stepSourceBullet` supplies the retained bullet's birth profile, first-Step
Laser Sight spread override, and the world's
actual random stream, and latches failures in that world. Invalid or retired
identities fail before draws or effect creation. Its explicit motion/operation
arguments still need wiring to the ordinary-Step scheduler; invoking this entry
point is not permission to rerun a shot already advanced by the legacy path.
The controlled owner tests compare the full pre/post RNG state, including the
spread-plus-material sequence for wall hits, rather than merely counting draws.
The enclosing runtime supplies the same motion object to its query/approach
callbacks and retains the executing bullet through destruction requests.
`verify-machine-gun-source-step` checks both profiles through empty, box, wall,
enemy-suppression and shootable routes, every callback failure prefix, and
second-Step initialization. This shared event composition is not yet wired
into the live schedulers: remaining target/effect/retirement adapters above
still gate replacement of the approximate gun path.

`BulletWallSound` implements `soundBullet` using the authored `initSfxBhit`
table. The selected target's material is read before the random choice:
breakable/thin/foliage select sound 29, rock selects 34/35, and metal selects
32/33. Unknown strings retain table slot zero. Even a singleton choice consumes
the source `irandom_range` call's two RNG words. The script then uses the shared
`gunwall` overlap channel with priority 60 and previous-voice cutting enabled;
sound-script fields belong to the retained executing bullet, not the wall.
Both world adapters use their actual RNG and run-session overlap state.
Material lookup resolves the selected lifetime again and rejects unsupported
objects rather than inventing inherited fields (object 99 has an empty Create
override). `verify-bullet-wall-sound` pins the script/table and material Create
assembly and checks material choices, underwater pitch, overlap ordering and
failure prefixes. World-owner fixtures exercise the retained caller and target;
they do not establish natural wall-impact event or audio-backend dispatch.

`GameplayCollisionOwnerTest` also composes this Step with resource-backed wall
line/point selection, actual box health/hit writes and the registered script
effect owner for its muzzle. Ordinary and gem-high profiles, with and without
Laser Sight, consume
one spread draw, issue three line queries, approach the selected box through
the original point-query loop, and retain both final position writes after the
bullet's destruction request. The damaged box remains active and selectable;
its own Step must produce destruction effects later. This is a controlled
integration fixture, not live projectile admission. A real fired lifetime now
receives the exact angle and intermediate positions, destruction marking and
room cleanup. Starting position, supplied profile and event invocation remain
controlled fixture inputs. Shootable, stone-impact, audio, rendering, natural
scheduling and full shared RNG ordering are not established by this case.

`make verify-source-shot-owners` compares those four scenarios with the stored
world's real collision/effect/lifetime owners. The stored fixture obtains a shot
and box from normal startup, then isolates their positions through the engine
setters; it does not resume normal simulation after rearranging that scene.
The comparison checks bit-exact final position, angle and box health, plus line,
point and position-write counts. Both executables independently require the
synchronous muzzle birth and retained marked projectile. These are host-built
world-owner tests, not an Amiga CPU replay or Windows gameplay capture.

The native-owner fixture additionally covers ordinary-wall impacts with the
registered muzzle/impact effect lifetimes and material sound dispatch before
destruction and final movement. This isolates one non-breakable branch; it does
not enable natural projectile scheduling.

Both live worlds route ordinary shot-consumption through `markBulletDestroyed`.
This handle-based operation publishes the collision exclusion once, retains the
record until cleanup, accepts repeated requests and rejects stale generations.
The stored-world integration test also publishes coordinates through the real
collision owner after marking destruction. This supplies the destruction-request
boundary needed by the recovered Step; it does not complete ordinary-Step
collision scheduling or replace the approximate trajectory.

Resource-owned usual-gun view retirement runs in the selected End Step phase,
after collision processing: `scrOutofview`, then both independent
`scrBulletOov` comparisons. It reads published binary32 Y promoted to the VM
real, not the private trajectory or rounded drawing coordinate. Destruction
marks retain the executing bullet until cleanup; the stored draw list drops
those retired handles before presentation. The native owner includes object
261 in its captured, sorted actor End Step population. These adapters use the
phase-published main-room view, as the existing camera contract requires.
Resource-less motion fixtures retain their combined movement/view-retirement
adapter; they have no room event dispatcher. Collision-driven position tests
cover subsequent End Step retirement in both worlds and all native ownership
variants. Exact ordinary-Step motion and full mixed collision scheduling remain
separate from this connected End Step behavior.

`publishBulletCollisionPosition` exposes checked builtin position publication
on both live worlds. It resolves the projectile lifetime on each call, accepts
marked-but-not-retired shots, and leaves private trajectory integration to the
caller. Native tests drive it with `MachineGunSourceMotion::commitPosition`,
checking each intermediate X-before-Y position in the real collision owner,
the retained previous-position snapshot and rejection of nonfinite input.
The stored owner tests the same boundary after marking destruction. The normal
gun adapters use this publication entry point, but still supply their legacy
trajectory coordinates; this does not yet replace their motion or rendering.
Legacy split-Q20 publication directly rounds its exact signed numerator to
binary32 using 32-bit integer operations and round-to-nearest-even. Throughout
the admitted signed-pixel/20-bit-fraction domain the numerator needs at most 52
bits, so the retained binary64 construction/addition is exact before narrowing.
The direct path therefore avoids intermediate binary64 work without changing
the published bits. The reference remains in `coordinateBinary64`; randomized
host checks and the native field verifier compare it at sign, carry and rounding
boundaries. This optimization does not validate the approximate trajectory itself.
`publishBulletAngle` similarly accepts the already-narrowed original angle and
publishes through the transform owner on both platforms. The stored record
retains its exact binary32 bits, including fractional degrees. The live legacy
initializers now use this API, but still derive their input from the approximate
Q32 direction until source motion is admitted.

## Collision lookup boundaries

`verify-runner-line-point.py` pins the reviewed registrations, wrappers and
dispatch bodies. `collision_point` (wrapper 0x528df0, dispatcher 0x4fd6e0)
narrows X/Y and returns the selected instance ID or -4. Its object-resource
branch starts at the ancestor-inclusive registration list at object +0x44,
skips caller only when `notme` is true, rejects flags 0x100003 and calls
candidate 0x465520. It returns the first hit. It does not call object-family
preparation or flush the spatial index. `collisionPointVm` implements this
object-resource branch in the shared owner, with candidate geometry supplied
explicitly; the candidate still owns lazy bounds refresh. It must not be
replaced with the indexed `position_meeting` path. All/instance-ID selectors
and collect-all are outside this entry point.

Line dispatcher 0x4fd370 differs: it calls 0x500d30 to choose list/spatial
search. The spatial branch flushes dirty bounds, orders endpoints by X (ties
take the reverse branch), forms float differences, and traverses 0x500860
using callback 0x5014c0. That callback filters flags, kind 1, family and `notme`
before candidate 0x4650b0; it stops on the first hit rather than sorting by
distance. The fallback object list also returns its first hit.
`RunnerSpatialInstances::collisionLine` preserves this object-selector
dispatch, passing the original narrowed arguments to candidates and latching
partial preparation/query failures. Special all/instance-ID selectors and
collect-all are outside this entry point.

`nonPreciseLineCandidate` refreshes dirty bounds before rejection, applies
the candidate geometry from 0x46517c, then requires an existing nonempty
sprite or mask resource. The collision pack admits only ordinary box/precise
bitmap modes; special and rotated-box resources are not approximated.
`RunnerLineBounds` preserves the original narrowed endpoints (without
reconstructing endpoint 2 from a float delta), retains order on equal X, and
clips against right+extension rather than the tree gate's unextended right.
Compatibility extension is +1; fractional mode uses float -0.00001.
The API explicitly requests non-precise behavior, as the usual gun does.
Precise bitmap line sampling is not implemented by this candidate.

`RunnerSpatialLine` transcribes the normal/zero arithmetic of the tree gates:
each add/subtract/multiply/divide narrows to binary32, right/bottom broad-phase
edges are extended by float one, and clipping the left endpoint changes the
arithmetic used when clipping the right endpoint. `RunnerSpatialIndex::queryLine`
uses those gates at every node, preserving stored entry order and propagating
an early visitor stop through the whole tree. It passes the original normalized
line to children, not a parent-clipped line. Invalid arithmetic fails the query
without turning it into a miss; earlier callbacks are not rolled back.

The original right-edge strip can reach division by zero: e.g. a vertical line
at X=16.5 against a box ending at X=16 passes the +1 broad-phase gate and then
divides by zero during right clipping. Under masked SSE exceptions the result
is positive/negative infinity, or NaN for 0/0. The final COMISS/JA and
COMISS/JAE comparisons do not take their rejection branches for unordered
NaN. `RunnerSpatialLine` substitutes these exact final predicates without
performing native floating-point division: NaN survives; negative infinity
requires the remaining start Y to be at least top; positive infinity requires
start Y to be below extended bottom. This is not an ordinary geometric
intersection shortcut. Zero-length lines, both vertical directions and
out-of-bounds endpoints exercise the distinct branches.

The arithmetic model continues to assume nearest-even rounding and masked
SSE exceptions; verifying the live Windows thread's FP environment remains
open. Other nonfinite/subnormal/overflow inputs are still rejected explicitly.
Narrow-phase candidate 0x4650b0 remains separate from the tree gate.

Resource-backed query adapters preserve that distinction:
`StoredWorldCollision::collisionPoint` uses direct object-list selection and
`nonPreciseLine` uses the source list/tree dispatcher. Both return a
generation-checked handle, source instance ID and published position snapshot;
a failed query does not publish a miss. The native owner's corresponding
`terrainCollisionPoint` and `terrainNonPreciseLine` return shared collision
handles (not terrain-local slots). They retain the completed-family gate for
60/93. `projectileNonPreciseLine` separately admits the VM's parent selectors
59/83 across registered native walls, boxes and enemies; `projectileSolidPoint`
uses parent 59 with precise=false for the approach loop. These selectors do
not synthesize unimplemented actors or replace collision events. Their targets remain generation-checked across marking,
retirement and Retry. Point queries accept the original precise flag. Line queries
explicitly support only precise=false, not an approximation of bitmap lines.

`projectileShootableMeeting` and `projectileShootableSelection` expose the
separate parent-90 shape queries at the bullet's current coordinates. Native
ownership merges all registered descendants by registration order before the
original list/tree choice and mask test; it does not scan only bone storage or
select the nearest target. Each call queries afresh and restores the temporary
self position. Stored ownership uses the existing resource-backed
`placeMeeting`/`instancePlace` adapters. Mixed bone/corpse tests cover changes
between the boolean and identity queries, deactivation, destruction, retirement
and slot reuse, with the same selection sequence across native ownership modes.

The serialized direct children of parent 90 are GunModule (109), objDeadBody
(263), bulletEnmBone (269), shootable_thing_template (287), Clam (288),
Scattered16 (289), Bottle (290), bubbleDome (291), TorchFire (295) and objGrass
(313); uniDead (349) inherits via objDeadBody. These registered-population
queries do not establish natural births or source fields for those omitted
actors. In particular, enabling the live gun cannot treat them as absent merely
because the bone/corpse query fixture passes.

`LiveCollisionLifecycleTest` exercises the stored adapters against the loaded
room and original terrain masks. `GameplayCollisionOwnerTest` exercises native
identity, terrain bounds, masks and self exclusion. Define
`DOWNWELL_NATIVE_QUERY_PROBE` when compiling the latter with the Amiga compiler
to instantiate both public query paths without host fixture dependencies.

This component is not yet the live trajectory owner: the live Q20/Q32 adapter
must be replaced together with its collision endpoints, angle consumers,
muzzle-effect creation and setter publication. Neither a common implementation
nor its scalar tests alone prove full encounter fidelity. Tests compare
integer-only arithmetic with scalar execution of the recovered trig program,
not a claim that a fresh Windows execution has been captured. The original
runtime FP-environment assumptions remain part of the open work queue.

The source initialization path's `initializeWithEffects` executes
`EffectSpawnScript` after publishing the bullet angle and before setting
`allSet`. The script synchronously creates object 219, publishes the caller's
`myFx`, then assigns sprite, image speed, image angle and depth separately.
It passes the unrounded VM direction to the effect angle setter, not the
bullet's already narrowed angle. Object 219 has no Create event and starts
without a sprite; batching overrides into constructor defaults would change
observable setter and registration order. Its inherited Animation End event
kills the child. Usual-gun profiles select single-frame sprites 603/602 and
speed 0.5. The operation adapter deliberately does not allocate or draw:
runtime ownership, global animation scheduling and depth-zero painter order
must accompany live integration. `make verify-effect-spawn` checks the VM
contract and initialization's publication/failure boundaries.

The gameplay asset pack includes offline planar and DMA variants of muzzle
resources 602/603 at 0 and 270 degrees. The generator's conservative inverse-map
test covers the entire 267–273 degree spread interval, including stationary
points and transparent pixels outside the canvas: both resources match the
quarter turn under pixel-centre nearest sampling at integer anchors. This is
not a proof for fractional anchors or Windows GPU edge rules. Cropping keeps
the source-relative anchor intact. `tests/muzzle-art-test.py`, included in
`make verify-amiga-game`, compares both planar orientations and mirrored DMA
streams against source pixels. Assets alone do not enable muzzle emission or
establish its cross-family Draw order.

Wall-hit resource 101 has all eight unrotated frames in the same offline
planar/DMA pack. `tests/wall-impact-art-test.py` compares their anchored pixels
and both DMA facings with the source texture pages. The native draw list
accepts these frames at integer anchors and zero angle; its scheduler test
checks the 16 updates at speed 0.5 and Animation End retirement. Fractional
anchors still need a verified raster policy. Shootable-hit resource 605 is
asymmetric and uses the raw bullet angle, so it must not reuse the muzzle's
quarter-turn proof; its offline angle-dependent rendering remains unimplemented.

### Original CPU vertex path and fractional anchors

`verify-runner-sprite-vertices` pins a static review of the Steam runner's
ordinary bitmap path. `draw_self` is registered at `0x537e6a`, calling wrapper
`0x531350`: instance X/Y at `+0xb0/+0xb4` are loaded with `movss`, forwarded to
`0x4af6f0`, then to the bitmap renderer. The default no-Draw-event path at
`0x490f65` calls `0x4afb40`, which likewise loads those float fields. Its
ordinary bitmap branch and the explicit Draw branch pass float X/Y unchanged
to texture/atlas builders `0x45c3f0` and `0x45c780`.

Both builders form local bounds from origin, trim and scale, then add float
X/Y directly to the vertex buffer. At near-zero rotation, atlas instructions
`0x45c8ff..0x45c931` perform those `addss`/`movss` operations without an integer
coordinate conversion. The rotated branch uses float radians (degrees times
float pi, divided by float 180), promoted CRT trigonometry and narrowed float
products/sums before vertex stores. The near-zero branch threshold is float
0.001 radians. Integer conversions elsewhere in these routines handle frame
indices, texture metadata or alpha, not an anchor-snapping policy.

This rules out assuming that the CPU renderer first rounds every sprite anchor.
It does **not** prove final Windows pixels: projection/view transforms, shader
offsets, GPU sample centres, edge/tie rules and filtering still need verification.
`tests/impact-angle-art-test.py` also exhibits angle and fractional-anchor
counterexamples for resource 605 under the existing offline nearest-pixel
model. That is a rejection test for generalizing the muzzle shortcut, not an
original-runner capture or a replacement raster oracle. Keep fractional
positions intact until a verified raster policy selects offline variants.

`make verify-default-shader` also decodes the embedded Windows `vs_4_0`
program, rather than treating the alternate GLSL source as Windows evidence.
The HLSL11 payload contains a 1564-byte DXBC container at byte 420. Its input
signature maps position to v0 and texture coordinates to v2; its output
signature maps texture coordinates to o0 and system position to o1. The entire
executable body forwards texture coordinates, copies position XYZ into a
temporary with W=1, then computes four dot products against cb0[43..46].
Reflection identifies these slots as the fifth matrix in `_gm_Matrices`.
There is no explicit round, floor, integer conversion or extra position offset
in this program. The verifier checks the decoded operands as well as input
identity and the complete embedded container hash. Token interpretation follows
[Microsoft's tokenized-program definitions](https://raw.githubusercontent.com/microsoft/DirectXShaderCompiler/main/include/dxc/Support/d3d12TokenizedProgramFormat.hpp).

The camera builder at `0x453a70` supplies width/height from +0x18c/+0x190 and
near/far 0/32000 to `0x6f75c0`, then publishes the result through `0x457680`.
For nondegenerate inputs the matrix builder writes diagonal values
2/width, 2/height, 1/(far-near), 1 and Z translation near/(near-far);
other entries are zero. These are single-precision SSE operations, with no
half-pixel X/Y translation in this builder. The verifier pins these routines
and their 1.0/2.0 constants, not a whole-pipeline mapping.

This does not establish the selected device feature level (the container also
has a downlevel program), constant-buffer upload, composed view/world matrices,
viewport, render-target orientation, sampler state or final pixel coverage.
Do not infer a final rounding rule from these two stages alone. In particular,
the offline resource-605 counterexamples remain model evidence, not GPU captures.

`NonLoopSpriteAnimation` supplies the common sprite-change and Animation End
state machine, also used by the live corpse-expiry child. Its admitted advance
path requires verified nonempty unit-key sprite metadata. After the muzzle
script installs its one-frame sprite and speed, the first animation update
reaches index 0.5 without retirement; the second wraps to zero and requests
destruction. Owners must exclude Step-born effects from the already-started
animation phase; a two-tick state machine alone does not establish that global
ordering or the effect's Draw order.

`ScriptSpawnEffects` specializes the same heap-free `NonLoopEffects` owner
used by corpse-expiry children. Its records start with object-219 defaults,
then receive the script's setters separately. Birth hooks see those defaults;
animation uses a captured lifetime cutoff, and retirement never reuses a key.
The keys are owner-local, not VM instance IDs. Production adapters still need
the world's identity allocation, collision registration and source painter
order; the script caller's `myFx` must retain its assigned source ID even after
the referenced effect dies. Failed publications latch the owner rather than
silently skipping an effect or rolling back prior writes.

The stored-world `scriptSpawnRegistration` bridge allocates object 219 from
the real runtime ID sequence and publishes exact birth coordinates through
`birthAt`, before caller-side sprite/speed/angle assignments. Its hooks bridge
sprite, angle and animation-index changes to the resource collision owner;
Animation End marks the generation-checked world record and excludes it from
collision queries without unlinking the room animation traversal. Explicit
pre-Draw cleanup retires the marked record and its effect together. Integral
effect depths publish through the layer observer and enqueue deferred migration;
unchanged zero remains valid for resource-less fixtures. Fractional depths are
rejected because the stored record's cache is integer-valued. The bridge must outlive its effect
owner and retire registrations before room teardown. It does not install an
automatic Step handler or replace the live gun adapter by itself.

### Selected depth-update path

The box destruction script requests depth -60000 for its sprite-603 impact.
Admitting that write requires layer ownership, not an immediate sort of the
room animation list. `make verify-runner-depth-changes` pins this audit against
both original input hashes and checks GEN8 offset 0x2c: value 2 makes loader
`0x4979e8` select the GMS2 branch (`0xb57f18 != 0`).

The getter at `0x42d820` reads float field `instance+0x178`. The setter at
`0x431ce0` narrows the incoming real to float and compares numerically with
the old value. Equal writes, including signed zero, do not enqueue or replace
it. Changed writes publish immediately, then append the instance to a unique
pending array in first-request order. Subsequent writes change its value but
not its queue position. Retirement removes queued pointers by stable
compaction (`0x50f6c5`, `0x50d740`).

At the maintenance boundary (`0x5108d0`), activation reconciliation precedes
depth service. The selected `0x50d220` branch visits that queue in order and
calls `0x414660` for each instance. It does **not** run the legacy branch's
two-pass removal/reinsertion of the active room list. Therefore applying depth
changes to `RunnerRoomMembership` would incorrectly change future animation
order and would not model the selected presentation structure.

The normal frame calls this maintenance routine after Begin Step
(`0x50dc89`), alarms (`0x50dceb`), ordinary Step (`0x50dd80`), collision
dispatch (`0x50ded1`) and End Step (`0x50defd`). Room entry also calls it at
`0x510813`. The stored scheduler exposes a checked post-reconciliation hook
at those supported phase boundaries, including empty phases, and the stored
runtime services its layer queue there. The borrowed lifecycle depth setter
checks both lifetime and source identity and publishes into that queue;
it does not modify the legacy animation list. Script-effect registries use this
path for integral depth writes; this is not complete runner phase coverage.

The resource-backed native actor adapter uses the shared fixed/dynamic layer
owner, while keeping its animation membership separate. It loads the room's
reservations before the native player birth and attaches authored player and
terrain IDs to those reservations. Dynamic actors without recovered Windows
numeric IDs are explicitly unnumbered: their layer links use checked lifetime
handles, numeric-ID lookup does not find them, and room uninitialization rejects
them unless they have already retired. This does not establish original global
instance lookup/creation order. Native room re-entry requires all registered
native lifetimes to retire before rebuilding the room-owned layer resources;
persistent native actors need a separately verified transfer path. The actor
projection retains inactive-instance layer termination and layer visibility;
retained terrain/background and independent effect passes are not yet a full
mixed source Draw traversal.

The selected handler first requires a room, instance and flag 0x400 (attached
layer element), then resolves the layer through instance field 0x140. It
truncates the requested float depth to an integer and leaves the layer alone
if that matches the existing layer depth. Otherwise:

- A depth distance strictly below 20 (float subtraction, then promoted absolute
  comparison against double 20), a dynamic layer (byte +0x1a), and exactly one
  element (+0x64) permit a layer-depth update through `0x4147b0`, provided that
  element is type 2 and points to this instance. A failed type/identity check
  exits; it is not the migration fallback.
- Other cases detach through `0x4258e0`, reset instance layer ID to -1, and
  reattach through `0x412a30`. That searches for a dynamic layer with matching
  integer depth or creates one. Attachment sets flag 0x400, publishes the layer
  ID and rewrites the instance's depth to the layer's integer depth.

The -60000 box-impact jump takes the migration case when born at depth zero.
Both script-effect registries now publish this jump into their selected layer
owners. Resource-backed world tests check that setter publication precedes
relinking and that the normal phase boundary moves the effect from depth zero
to -60000. Native fixtures without a layer owner still reject changed depths;
there is no legacy re-sorting fallback. This establishes layer migration, not
the full mixed terrain/controller/effect composition or natural box event.

#### Layer element and migration contract

`0x4128d0` inserts an element at the tail when its explicit append argument is
true. Otherwise a type-2 instance element enters at the head, even if its
instance pointer is null. Other element types go immediately after the
contiguous prefix of type-2 elements with non-null, active instances. The first
non-instance, null instance or inactive instance terminates that prefix; this
is not a search for the last active instance anywhere in the list.

`0x4147b0` writes the new integer layer depth before repositioning the layer.
Decreasing depth scans backwards past greater-or-equal depths, placing the
layer before equal-depth peers without merging. Increasing depth scans
forwards; an equal-depth dynamic layer can absorb the moving dynamic layer
when merging is enabled. Without that match it inserts before the first
greater depth, or at the tail. Dynamic-layer creation instead inserts after
equal-depth layers (`0x412820`). These operations cannot share an unqualified
stable-sort rule.

The merge at `0x41487f` updates each element's owner and each non-null type-2
instance's layer ID, then appends the complete source element chain to the
destination tail. It invokes `0x426630` before unlinking and pooling the source
layer. That normalization scans backwards using the saved previous link and
moves inactive instances to the tail. The resulting inactive subsequence is
**reversed**, not stable; other elements retain their forward order. Instance
depth fields are not rewritten by this merge body.

Detachment (`0x425a40`) removes both element/instance hash entries and the
element links, clears the instance's attached flag and layer ID, and clears
the room's current-element pointer if it names the removed element. An empty
dynamic layer is then deleted. Layer disposal (`0x425b60`) guards reentry with
byte +0x19, removes remaining elements if any, unlinks the layer, decrements
the room count, removes its hash entry and returns it to the pool. The empty
dynamic-layer path does not invoke element destruction again.

`RunnerLayerElements` supplies bounded, allocation-free link operations for
insertion, concatenation, removal and inactive normalization. Its slots are
borrowed identities, not original resource or instance IDs. The owning adapter
must detach slots before reuse, provide source type/null/activation predicates,
publish instance layer IDs, and manage layer lifetime and depth. Forward and
backward visitors expose raw links only; neither asserts selected Draw order.
The regression exhausts all 256 activity masks across nine source/destination
splits and checks both link directions and ownership. Binary range hashes pin
the recovered routines; these tests are not live Windows rendering comparisons.

`RunnerLayers` owns bounded layer slots and composes those element lists with
the recovered integer-depth movement and merge rules. Layer IDs are supplied
by its caller and remain separate from reusable slots. It finds the first
equal-depth dynamic layer, retires empty dynamic layers on element detachment,
and retains empty fixed layers. During a merge its checked callback publishes
external instance layer IDs in source-element order before splicing and
normalization. Publication failure is terminal; the new depth and any earlier
external publications are not rolled back or retried. Tests exercise all 64
fixed/dynamic combinations across six source positions, five requested depths
and both merge settings (3,840 cases), as well as inactive element order,
detachment, slot reuse and failed publication.

This is not yet the full runtime layer owner. The room adapter must provide
source ID allocation, fixed-layer loader order, type/null/activation predicates,
hash-equivalent lookup and external attached/current-element publication. The
binary32 depth setter, unique deferred queue and instance migration service
still need live connection, followed by the selected Draw walk in both backends.

`RunnerDepthChanges` models immediate binary32 setter publication and the unique
first-request-order queue. Numerically equal writes preserve the old bits,
including signed zero. Retirement stable-compacts the queue before slot reuse.
Service follows activation maintenance and clears the queue only after every
callback succeeds. Failure is terminal, so a successfully migrated prefix cannot
be replayed. Its callbacks cannot recursively mutate the pending queue; the
selected layer service does not dispatch game events.

`RunnerDepthService` composes that queue with `RunnerLayers`. It truncates the
requested binary32 depth toward zero, converts the old integer layer depth to
binary32, and rounds the subtraction to binary32 before testing absolute distance
strictly below 20. The same-integer-depth path retains the fractional field;
the singleton-layer move also retains it. A failed singleton type/identity
check exits without fallback. Migration detaches, finds/reuses or creates a
dynamic destination, and publishes its integer depth back to the instance.
The signed-int-range normal/zero subset is checked explicitly; subnormal,
nonfinite and out-of-range conversions fail rather than silently approximating
the x86 invalid-conversion result.

Connected component tests cover the queue and layer owner together, including
the -60000 box-impact depth, small-move merging with external layer-ID
publication, retirement, reused layer slots and terminal allocation/publication
failures. A threshold matrix covers both sides of 20, fractional requests,
large layer depths, singleton/multiple elements and fixed/dynamic layers.
These components are not yet invoked by the live world schedulers. Adapters
still need source identity allocation, instance-field/lookup synchronization,
current-element bookkeeping and the original phase boundaries; component
agreement is not proof of a naturally spawned or correctly drawn box effect.

#### Selected ordinary Draw traversal

`0x490dac..0x490dcc` chooses `0x490370` when the current room exists and its
layer count at +0xdc is positive. Unlike depth service, this branch tests the
room count, not the global GMS2 flag directly. The recovered room manifest has
132 layers across 25 rooms, with no zero-layer room (rmMain has six). The
room-list loop at `0x490f11` is the fallback, not evidence for layer traversal.

`0x490370` starts at room layer tail +0xd8 and follows previous +0x70. Invisible
layers (byte +0x18 zero) are skipped, including their begin/end hooks. Visible
layers perform their setup/begin operations, then start at element head +0x5c
and follow next +0x14, saved before element dispatch. The switch at `0x490a64`
maps element type 2 to `0x4904f4`. That branch:

- Skips null instance pointers.
- **Ends the entire current layer's element loop** on instance flag bit 0x2;
  it proceeds to layer-end operations, not to the next element.
- Otherwise requires `(flags & 0x110013) == 0x10` and a non-null object.
- Dispatches ordinary Draw (event 8, subtype 0) when present. Otherwise it
  resolves the sprite and calls `0x4afb40`, or `0x4b4c00` when flag 0x4000 is
  set. A missing sprite emits nothing. The event branch needs no sprite.

After layer-end operations it reads the previous layer link (`0x49081e`).
This explains why inactive-instance tail normalization affects more than
cosmetic order: an inactive entry ahead of drawable entries suppresses them.

`RunnerLayerDraw` implements this traversal and dispatch policy over the shared
layer owner, with checked callbacks for layer visibility, begin/end operations,
element lookup and presentation. Tests cover reverse layer/forward element
order, equal-depth peers, layer termination, hidden layers, callback failures
and 1,024 instance flag/lookup combinations. Non-instance element types remain
backend dispatch responsibilities. The element walk saves each successor before
dispatch (`0x4904c6..0x4904cb`). Synchronous head insertion therefore does not
draw the new element in that pass. A tail insertion is visited only if an
unvisited predecessor subsequently reads its link: appending while drawing the
old tail does not visit the new tail until the next pass. Tests cover each of
three birth positions, head/tail insertion and the following Draw pass. This is
not a frozen frame snapshot. Draw-time element retirement/reuse, layer movement
or removal, layer scripts, shader setup and non-instance rendering still require
adapter integration/audit; callbacks must not perform the unaudited mutations.
It is not yet a replacement for either live renderer's room-list adapter.

#### Activation and layer membership

Room reconciliation (`0x48d630`) scans inactive-room promotions before
active-room demotions. After object membership publication, both branches call
`0x4266e0` (`0x48d777` and `0x48d85f`). That selected GMS2 routine requires the
attached flag, resolves the instance's layer and element, unlinks the element,
and reinserts it at the head for an active instance (`0x412a00`) or tail for an
inactive instance (`0x412c20`). It does not detach/recreate the element or retire
a temporarily empty dynamic layer. Layer and element IDs remain unchanged.
This differs from the backward inactive normalization used during layer merge.

`RunnerLayers::reconcileInstanceActivation` exposes that same-element relinking.
Its caller must validate attachment and identity and invoke it only at actual
room membership transitions, after object registration. Connected component
tests use `RunnerRoomMembership` to drive those transitions and `RunnerLayerDraw`
to observe the result. They verify request-order independence, promotion order,
singleton-layer survival and no reordering during unchanged reconciliation.
The live stored and native world adapters still need to invoke this operation
once their complete layer ownership is established.

#### Serialized layer and instance attachment order

The room-layer reader at `0x413660` walks the serialized layer-pointer array in
order, copies IDs/depth/visibility, and inserts each layer after existing equal
depths (`0x4143f6`). The instance-layer branch (`0x413cdc`) visits the serialized
instance-ID array forwards. Each ID receives an empty type-2 element inserted
with append=false (`0x413df7..0x413e20`), hence at the head. This reverses that
membership array independently of room instance creation order.

Room instance setup resolves the reserved element's layer by source instance ID
before calling attachment (`0x48871f..0x48873d`). Attachment `0x412ab0` reuses an
existing element only when its instance pointer is null; an occupied entry is an
error. It publishes attached state, layer ID and binary32 layer depth. A reused
active entry stays in place; a reused inactive entry invokes activation relinking
to move to the tail. A newly allocated entry instead uses the separate 0x80000
registration flag to choose head/tail insertion. The two flags must not be
conflated.

`RunnerLayerInstances` provides the source-instance-ID lookup and borrowed,
generation-checked instance binding. Reserve in serialized membership order,
then attach in room creation order. All 24 permutations of four instance births
produce the same reserved element order in tests. Additional tests cover
inactive reuse, duplicate IDs/handles, retirement, slot reuse, new dormant
registration and capacity failure. Source instance IDs, store handles and
element slots remain distinct; source element-ID allocation is not fabricated.
Lifetime-handle lookup uses a compact chained index, comparing both slot and
generation. Attach/reserved initialization publish the index; retirement,
uninitialization and rebind release unlink it. Source-ID lookup, first-free
element allocation and all painter-order lists remain unchanged. Colliding
keys follow the chain rather than aliasing another lifetime. The index costs
two bytes per element plus up to 512 bytes of bucket heads; its worst-case
collision chain is still linear. Tests compare collision-heavy lookup with a
full element scan, including stale generations and slot reuse.
This registry does not yet load all non-instance room elements or replace live
room startup. Persistent-room rebinding and full serialized metadata publication
remain integration work.

`PackedRoomLayerLoader` consumes the existing DWRM room/layer/membership tables
into fresh `RunnerLayers` and `RunnerLayerInstances` owners. It preserves
serialized layer order for equal-depth insertion, marks loaded layers fixed,
publishes their metadata (including visibility) through a checked callback, and
reserves instance elements in serialized order before any instance birth. It
checks capacity, IDs and complete unique membership coverage; failure stops
with the already-published prefix intact. Do not retry a failed owner in place.
The DWRM tables omit non-instance element payloads, so this is explicitly an
instance-membership loader, not complete background/tile/shader recovery.

Room-pack tests connect this preparation to the actual `PackedRoomLoader`
creation path through its lifecycle binding and verify layer attachment before
the first instance event. The generated-pack case checks all 25 rooms, 132
layers and 5,186 membership entries, including reversed placeholder order.
The live startup/native adapters still need to own these structures, retain
visibility/state publication, coordinate persistence and retire queued depth
identities before reuse; the connected test does not enable a live Draw switch.

Layer IDs are process state, not room-local slot numbers. Globals `0x925f84`
(loaded maximum) and `0x925f88` (last allocated dynamic ID) start in the PE's
zero-filled virtual data tail. The serialized reader raises the loaded maximum
after each completed layer (`0x4144c0..0x4144d4`). Dynamic creation takes the
signed maximum of these two counters, increments it, and publishes the result
before room insertion (`0x41282c..0x41284c`). `RunnerLayerIds` owns this state;
retain it across room changes and layer disposal. The packed layer loader can
publish into this owner as it loads fixed layers. Tests cover all original
packed rooms' maximum publication, dynamic allocation across different room
owners, slot reuse, insertion failure and explicit exhaustion. Negative and
wrapped IDs are outside the admitted domain and fail rather than silently
emulating signed wraparound. Source element IDs use a separate counter and
are not supplied by this layer-ID owner.

The room resource catalog loop (`0x496cd5..0x496ddf`) calls the room reader
for every resource. That reader loads serialized layers (`0x48b930..0x48b94e`)
and, for room format 3, may allocate a sequence layer. All 25 original rooms
have format 3 and empty sequence arrays, so the latter branch allocates nothing
(`0x4d962c..0x4d9637`). The 132 serialized layer IDs have maximum 131.
The source verifier now checks these raw ROOM facts as well as the loop bytes.
Shared `StartupRuntime` seeds its owned `RunnerLayerIds` from the complete
catalog before first-room loading, preserving the counters across reset.
`verify-room-layer-catalog` exercises desktop and non-desktop startup/reset and
invalid-pack failure without copying live lifecycle owners. This seeds process
identity state only: the live layer structures and renderer switch remain open.

Room initialization's later global-instance pass (`0x413590..0x413610`) skips
already-attached instances. For an unattached instance it first resolves its
saved layer ID, accepting fixed as well as dynamic layers without comparing
depth. Only a missing ID (including -1) triggers truncation of instance depth,
lookup of a dynamic layer at that integer depth, or dynamic-layer creation.
It then invokes ordinary attachment (`0x412ab0`), which publishes the selected
layer's depth. `RunnerLayerInstances::attachRoomFallback` implements this
selection with caller-owned source IDs and checked layer allocation. Tests
distinguish saved fixed-layer identity from same-depth dynamic fallback,
fractional truncation, fixed-only depth matches, append state, already-attached
no-ops and failure paths. The saved-ID branch does not convert the old depth.

This is only the fallback pass, not the whole room transition. The earlier
serialized-element initialization at `0x412e60` has different rebinding and
depth-publication behavior; it must not be replaced by ordinary attachment.
Its type-2 branch (`0x412ea5..0x412fe6`) skips initialized elements and negative
source IDs, then looks up the source instance ID in the global instance hash.
An absent instance leaves the placeholder uninitialized. If the instance has
an attachment and a layer ID other than -1, the runner searches this room's layer
chain for that ID and a type-2 element pointing at this instance. A match is
unlinked and returned to the element pool, without disposing the emptied layer.
The destination element receives the instance pointer; the instance receives
the attachment flag and destination layer ID. No depth write or inactive-tail
move occurs. `RunnerLayerInstances::initializeReserved` supplies this checked
publication policy with borrowed global lookup and old-attachment release
callbacks. The release adapter must implement the same-room search, preserve
the instance lifetime and destination placeholder, and not substitute normal
destruction/empty-dynamic-layer retirement. Tests cover preserved fractional
depth and position, absent/already-initialized entries and callback failures.
The default `initializeReserved` overload now uses the shared
`releaseForRebind` operation: it checks saved layer ID and the exact registered
lifetime in this room, unlinks the matching element with `detachForRebind`,
and frees its registry entry without changing instance state or disposing an
empty dynamic layer. Missing IDs, wrong layers and nonmatching generations do
not release anything. Tests release the old element, reserve a new placeholder
for the same source ID and bind the same instance lifetime, checking that its
fractional depth survives and the old layer remains allocated. This release
does not retire the instance or cancel its pending depth writes.
The caller must also preserve original global-instance hash iteration order
and the subsequent per-layer inactive-tail normalization (`0x41363e`). Neither
is implied by iterating storage slots. Live persistence integration remains open.

The global instance table has 512 head/tail buckets, indexed by source instance
ID masked with 511 (`0x401290`, mask at `0x8a5e54`). Creation appends to each
bucket's tail (`0x488818` and `0x488c54`); the iterator walks buckets in ascending
order (`0x422770`). The room fallback uses that traversal at `0x413549`, including
inactive and destroy-marked instances which remain allocated.
`InstanceStore::captureGlobalInstanceOrder` reproduces this ordering by sorting
a caller-owned snapshot by bucket while preserving insertion order within each
bucket. Stable record compaction preserves that insertion order after retirement
and across persistent room entry; reusing a storage slot does not restore its
old position. The snapshot is valid only for a structurally quiescent pass, not
as a replacement for the original iterator when births or removals happen during
iteration. Its constant-extra-workspace sorter is usable on 68000, but large
snapshot storage must live outside the task stack. This ordering helper does
not by itself connect live room fallback or inactive-tail normalization.

`RunnerLayers::normalizeRoomInstances` exposes the final room-entry pass at
`0x41363e..0x413658`: forward layer order, invoking the existing backward
inactive-to-tail operation for each layer. Call it once after serialized
initialization and global fallback. Repeating it reverses the inactive suffix
again, so it is not a per-frame stable partition. The predicate must reject null
and non-instance elements and cannot mutate the owners. Empty dynamic layers
survive this pass. Exhaustive tests cover all 256 inactive masks across two
layers, traversal order, repeated invocation and terminal-owner rejection.
The stored runtime invokes this sequence through its room-preparation callback.

The shared `RoomLoader::enterChecked` and `PackedRoomLoader::enterChecked`
accept a checked destination-preparation callback after old nonpersistent
instances retire and before the first new birth. The original calls layer
initialization at `0x50ff88`, before the serialized birth loop at `0x50ff9f`.
This boundary lets an owner prepare fixed layers, bind surviving instances and
normalize their elements before any new PreCreate/Create callback observes
them. Preparation must not structurally mutate the instance store. Validation
and retirement failures suppress it; its own failure preserves earlier
retirement but suppresses every subsequent birth and event. The packed-room
fixture now loads its layer reservations through this boundary rather than
preparing them outside room entry. `StartupRuntime` installs the preparation
callback for packed rooms; the separate native gameplay adapter still needs it.

`RunnerLayerInstances::initializeRoom` composes the three instance-layer stages:
forward layer/element initialization (`0x413500..0x413549`), caller-provided
global-instance traversal, and inactive-tail normalization. It propagates stage
failure without executing later stages or rolling back published bindings.
The packed-room preparation fixture connects this operation to a real
`InstanceStore` survivor and the two serialized births: the inactive survivor
resolves its saved fixed-layer ID, moves behind the reserved elements, and
retains its lifetime while the new instances fill those reservations.
This is the type-2 instance projection only; non-instance serialized elements
still need their own initialization, and live owners are not installed yet.

The omitted serialized payload is bounded for this build. Run
`make verify-room-layer-metadata` to export scalar metadata with pinned UTMT
version/commit and the input hash, then cross-check every layer against raw
ROOM records. There are 108 instance layers and 24 background layers, no other
serialized types. All layer offsets/speeds are zero; effect names and property
lists are absent. Six backgrounds reference sprite 740 (`Spr_YoYo_background`)
in Steam-service rooms 0, 1, 2, 3, 5 and 6. The other 18 have sprite ID -1 and
serialized black color. These are resource facts, not permission to omit their
runtime elements: sprite-less background initialization/Draw and source element
ID allocation still need integration. The ignored output is
`generated/room-layer-metadata.json`. DWRM v3 carries the layer type, background
sprite ID, color, five background flags, first frame, animation speed and speed
type in its 48-byte layer record. Layer offsets/speeds remain omitted only under
the pinned zero-value check. The decoder rejects old pack versions, unknown
layer types and background layers with instance memberships. The layer loader
publishes this metadata but still reserves only instance elements; background
element construction and Draw remain integration work. No original asset
payloads are included in the committed extraction tools.

`RunnerLayerBackground` models payload construction at `0x413bc9..0x413cb7`:
unit binary32 scales, separate low-24-bit color and binary32 alpha (high byte
divided by 255), original flags, sprite ID and unconverted animation fields.
It retains sprite-less backgrounds; a missing sprite does not skip creation.
The type-1 initialization jump at `0x412e9e` targets `0x41306b`, which only sets
the initialized byte. All 256 alpha values and 32 flag combinations are tested,
and all 24 recovered backgrounds construct through this shared representation.
This does not yet allocate/link background elements, consume their source IDs,
advance animation or implement Draw; those remain owner integration work.

`RunnerLayerBackgrounds` supplies bounded payload ownership and shared element
slot allocation. It links backgrounds using the existing non-instance insertion
rule (after the contiguous active-instance prefix), dispatches initialization,
and frees only its own element on retirement. The instance registry now accepts
a separate non-instance initialization callback; missing ownership is an error,
not a silently omitted element. The original-room fixture loads all 25 rooms
with both instance placeholders and background elements and runs their mixed
initialization. Slots remain separate from source element IDs: process-wide
element ID allocation, animation, Draw and live owner installation remain open.

Background Draw dispatch (`0x48f0a0`) first checks background visibility, then
looks up the sprite. A missing sprite (not merely ID -1) selects a full-viewport
colored quad at `0x48f1ce..0x48f3e7`; it is not a no-op or an unconditional clear.
The runner temporarily establishes identity matrices, submits the quad with
render-state/blending intact and restores the matrices. Color reconstruction
multiplies binary32 alpha by 255, truncates to signed integer, clamps to 0–255,
then combines that byte with low-24-bit color. Even zero-alpha quads dispatch.
`RunnerLayerBackgroundDraw` supplies checked visibility/resource dispatch and
this color calculation for finite normal/zero alpha in the integer-conversion
domain. All serialized alpha bytes round-trip, with extra fractional/clamping
and failure tests. Original-room tests route all 24 backgrounds through the
mixed layer visitor, selecting 18 solid callbacks and six sprite callbacks.
The callbacks remain backend contracts: viewport-quad rendering, sprite origin
save/zero/restore, sprite tiling/stretching and animation are not implemented
by this dispatcher or enabled in the live renderers yet.

`RunnerLayerLifetimes` composes instance attachment, generation-checked lifetime
state, the deferred depth queue and depth migration over borrowed room layer
owners. Reserved births use their fixed layer; dynamic births find/create a
dynamic layer even if a fixed peer has matching depth. Flag publication and
activation relinking remain distinct calls. Retirement cancels queued depth
writes before storage-slot reuse; stale handles and capacity failures stop the
adapter terminally. Tests connect multi-instance departure, singleton merging,
merged layer-ID publication, a -60000 migration and generation reuse. Stored
runtime ownership is installed independently of Draw traversal; native gameplay
ownership and live depth-setter admission remain separate integration work.

Its room-entry adapter adopts surviving lifetime fields without treating them
as new births, assigning a depth, or resetting a saved layer ID. The room owner
must supply the fields after source teardown; adoption does not transfer a
pending depth queue or perform teardown itself. Duplicate source identities and
occupied/stale handles fail terminally. `initializeRoom` uses those owned fields
for serialized bindings, then a caller-provided original global-hash handle
walk for fallback, followed by the separate inactive-tail normalization. Owned
non-instance elements initialize through the supplied callback. Invoke this
once at the source room-entry boundary, not once per frame: normalization is
not idempotent. Tests retain fractional depth on serialized rebinding, honor a
saved fixed layer during fallback, and carry adopted lifetimes through deferred
depth service and retirement. These tests do not establish persistent-room
gameplay parity or the native gameplay adapter's installation.

Room-layer uninitialization is a separate boundary, not instance retirement.
The original `0x414c50` visits forward layers and forward elements; its type-2
handler at `0x41498b` looks up the source instance ID, clears attachment bit
`0x400` and the saved layer ID to -1, then nulls the element's instance and
initialized flag. It does not overwrite depth or unlink/free the element.
The ordinary nonpersistent-room exit calls this before room destruction at
`0x50e798`. `RunnerLayerInstances::uninitialize` and the lifetime owner's
`uninitializeRoom` model this boundary, including absent global instances and
repeated uninitialization. Non-instance elements require their own callback.
The operation itself does not remove queued depth requests; a later service
on an unattached instance is a no-op, while service after rebinding uses the
new attachment. Tests exercise both timings without asserting that every room
transition schedules them identically. Full room destruction, survivor transfer
and persistent-room storage remain the live room owner's responsibility.

Retirement after uninitialization must not attempt to detach an already-unbound
element: `0x4258f4` skips that operation when the attached flag is clear. The
lifetime adapter cancels the departing instance's pending depth request and
releases its state, leaving the placeholder owned by the room. A packed-room
re-entry test now composes the loader, lifecycle adapter, preparation callback
and global-instance ordering twice: two placed instances retire/reappear with
new generations while one persistent lifetime and its pending depth request
survive replacement of the room's layer storage. This exercises the actual
shared loader boundary, not the live SDL/Amiga scene drivers or the original
runner's complete Room End/Cleanup event sequence.

`RecordLifecycleBinding::observeLayers` adds a borrowed layer observer without
replacing collision ownership or its movement/resource/query callbacks. Ordinary
and positioned births notify it exactly once after successful primary birth;
retirement and activation publication follow the same checked composition.
Membership reconciliation first publishes the primary object membership, then
relinks the layer element. Missing positioned ownership still fails before
either birth callback; observing layers does not make `hasOwner()` claim spatial
query support. Observer failure preserves prior primary publication and must
terminate the calling world, just like existing lifecycle failures. Installing
a second observer is rejected. The binding adds five pointers of storage.

`StoredLayerLifecycle` translates stored source ID/depth/activation into the
shared layer lifetime owner. Tests compose it with packed-room loading and with
the real spatial owner for positioned time-change births and deferred cleanup.
`StartupRuntime` installs this observer before its first packed-room birth.
Its `StoredRoomLayers` owner keeps lifetime and pending-depth storage across
room transitions while replacing uninitialized layer/element/background
storage in place. Serialized backgrounds are initialized through their own
owner. Disposal uses bounded loops, not room-sized stack temporaries. Catalog
layer identities survive reset. Failed publication or preparation stops the
runtime. The garden-grass activation write uses the same publication boundary
as other stored actors, before changing the record's active flag.
Observer installation occurs in member initialization before built-in effect
registries copy the lifecycle binding. Installing it in the constructor body
leaves those earlier copies without layer birth/retirement callbacks. The
production muzzle test checks both attachment and retirement of these lifetimes.

Runtime tests check every allocated instance's generation, source identity,
attachment and activation against its layer element through startup, garden,
gameplay and reset. Existing deterministic state/RNG tapes still pass. This
installation adds bounded storage to `StartupRuntime`; it does not establish
A500 memory fit. The separate native gameplay lifetime owner still needs
installation.

Layer-element allocation retains the lowest-free-slot rule. The instance
registry resumes its scan at the last candidate while layer ownership has
only grown; ordinary detach, rebind detach and room discard invalidate that
hint through the layer owner's vacancy revision. Registry discard also resets
the hint. External non-instance elements are checked by the same owner query,
so holes released by backgrounds cannot be missed. The revision saturates;
once saturated, every allocation scans from zero instead of risking rollover
aliasing. This changes neither element order nor numeric source IDs. Mixed-owner
allocation tests compare the chosen slot with an independent full scan on host
and in the native field-verification gate.

`StoredRoomLayers::visitDraw` supplies mixed element traversal through
`RunnerLayerDraw`: retained serialized visibility, reverse layer order, forward
element order, checked instance/source identity and owned background payloads.
The caller supplies current source flags and event/sprite availability, then
receives separate instance/background callbacks at their layer depths. Hidden
layers perform no instance lookup; unbound serialized reservations are skipped;
inactive instances terminate their layer only. Dynamic layers start visible
even when their depth matches a hidden fixed layer. The caller must not retire
or reuse elements or move layers during this traversal. Renderer integration
must use the background callback at its actual ordered position, not replace
it with a frame-wide clear. Tests cover hidden/fixed/dynamic peers, failure
propagation and background traversal in all 25 original rooms.

The SDL garden renderer uses this mixed traversal for its translated object
Draw bodies and the original rmMenu solid background. The background submits
a clipped viewport rectangle at its layer position, not a framebuffer clear.
This adapter only supports the garden's sprite-less background; it rejects a
sprite background until a resource-backed tiling/stretch path exists. The
gameplay terrain/effect batches have not switched to this traversal. This
does not establish complete custom Draw-event fidelity
or Draw-time mutation support. The garden regression independently enumerates
serialized layers/memberships and runtime births before comparing Draw output.
`DOWNWELL_CAPTURE_GARDEN_TICK=N build/capture-sdl-landing captures/example`
captures the Nth rmMenu update along the bounded full-startup input tape.

`StartupRuntime::visitSourceDrawChecked` centralizes stored instance lookup,
admitted visibility/active/destroyed fields and event/sprite availability for
the garden and gameplay. The live registered-actor and offscreen-debris passes
use its instance-only projection; only explicit resource-less fixtures retain
legacy room-list Draw. Tests hide a copied rmMain player layer and require
that production traversal omits only that layer, preserves dynamic actors,
and stops on callback failure without changing simulation/RNG state.
This is not full gameplay composition: terrain, shops, fields and standalone
effect banks still need their source-ordered boundaries. Both live actor
projections use selected layer traversal and support deferred integral
script-effect depths; that does not prove the omitted composition boundaries.

The native static garden cache likewise separates Create order from painting:
it consumes RNG in placement order, then reorders scenery in place using
reverse serialized layers and reverse memberships. It does not approximate
equal-depth order with a stable OBJT-depth sort. This adds no per-frame work
or second cache-sized allocation, and does not change hardware scrolling.
`verify-amiga-menu-wake-frame` checks the captured foreground/background caches
against original RGBA atlas pixels at source placements, independently of the
planar encoder and blitter. The host placement test first checks VM-derived
Create choices and exact source membership order. The raster check covers the
waking caches. `verify-amiga-garden-frames` captures wake, final title and first
well-entry frames in one bounded run, checking both garden planes and the tall
entry cache against source atlas pixels. Title placement uses objWell Create
and objTitle Step's final-frame offset, at the dynamic depth-zero boundary.
`objWell` Create emits four object-98 children before the object-317 title.
The scene comparison locates the unique menu-room object 317, rather than
guessing its dynamic instance ID: an ID from an incomplete child-creation path
can instead resolve to one of the wall children. This resource-based selection
does not establish original global numeric-ID allocation parity.
The continuity check also requires identical foreground world pixels across
the cache switch and a still-visible parallax background. Neither check proves
the entire title animation, omitted particles or gameplay layering. All these
capture runs restore the ordinary native build even after verification fails.

Native registration uses `GameplayScriptSpawnInstances`, borrowed by the world
and owned by its collision backend. Its appended handle range leaves existing
actor ranges unchanged. Records refer to stable `ScriptSpawnEffects` storage;
collision births use the effect's absolute binary32 coordinates, without native
origin translation. Setter hooks update the same resource-backed collision
properties used by other actors. Registry keys are monotonic owner-local
lifetimes, not Windows numeric IDs. Retire effects before destroying their
owner or changing rooms. Successful runtime spawn wiring retains a checked,
non-allocating binding to that owner/registry pair while records exist. Ordinary
main-room ticks capture its birth cutoff and dispatch animation in room order;
ordinary pre-Draw cleanup retires marked effects. No borrowed stack lambda is
retained. Owners and registries must stay at fixed addresses until their records
are retired. Attempts to mix live owners in one runtime fail before another birth.

Ordinary Retry and room loading clear registered effects through the bound
owner before replacing world records, including unfinished animation. Both
runtimes also retain an explicit typed
`restartFromResultsWithScriptEffects` entry point that clears owned effects
before rebuilding the world, including effects still awaiting Animation End.
It neither fabricates Animation End nor clears the caller's saved identity;
new births cannot revive old handles. No results action performs no cleanup,
and terminal effect failures cannot be reset through this entry point.
Both runtime objects contain a fixed-address, 64-entry script-effect store;
their ordinary spawn overload uses it without an external owner or allocation.
The stored runtime also owns its source-ID registry. Native in-place Retry
preserves the internal store's lifetime sequence before reconstructing the
world, so the collision registry cannot mistake a new effect for an old one.
The bound runtime types are intentionally noncopyable through this ownership.
This adds bounded runtime storage; stock-A500 memory fit remains unproven.

Cross-family teardown order, gun emission and drawing remain separate
integration requirements. Direct low-level
registry births without a runtime binding must still use the explicit scoped
interfaces; they are not automatically adopted into another owner.

Both runtime `spawnScriptEffect` entry points use `ScriptSpawnOperations` for
the same creation and setter sequence. They supply resource frame lookup and
caller identity publication, and latch partial failures into the runtime's
existing terminal effect status. A failed setter retains completed birth and
caller assignment; a subsequent call cannot retry the earlier operations.
Script effect stores are noncopyable and bound to one fixed-address registry,
including after clearing their records. Moving lifetime keys between registries
is rejected before publication. The stored backend publishes a source numeric
ID; the native backend publishes its checked local handle.

`advanceScriptEffectAnimation` consumes a caller-captured phase cutoff and
latches failures into each runtime. The owner publishes every evaluated image
index, including the wrapped index, before marking Animation End destruction.
Marked lifetimes remain linked until explicit room cleanup; later animation
passes skip them. Both backend cleanup entry points retire through the same
effect owner and checked registration, retaining caller IDs. Neither animation
marking nor cleanup failure may be retried on a failed runtime.
Publication failure retains the computed timeline/index but does not run
retirement; later phase calls on the failed runtime do nothing. Integrators
must still capture and dispatch that phase at the original global animation
boundary rather than advancing effects immediately after a Step birth.

Both runtimes' scoped `stepWithScriptEffects` entry points dispatch each
registered effect from the active-room animation traversal, immediately after
that instance's previous-position capture. It captures the birth cutoff before
the tick and releases its borrowed callback on return; collision-phase births
wait until the next tick. Callers retain responsibility for typed pre-Draw
cleanup and room teardown. The native collision owner supplies active-room
traversal for registered enemies, bullets and script effects, preserving that
binding through Retry. Unregistered effect families and automatic live gun
emission still need integration; this does not establish complete cross-family
room creation or animation order.

Registered object-219 effects reach both ordinary gameplay renderers in room
Draw order, independent of owner-local handles. SDL retains published binary32
positions and angles. The Amiga collector uses the offline 602/603 frame 0 or
quarter-turn frame 768, admitting only integer anchors and the verified
267..273-degree interval (or zero rotation). Unsupported fractional anchors,
sprites or angles fail the draw rather than silently rounding or converting
assets at runtime. This presentation support does not enable automatic muzzle
emission from the still-approximate live gun trajectory.

Run `make verify-machine-gun-source-motion verify-smoke-direction
verify-smoke-trig`. The source-motion test covers birth narrowing, raw RNG
endpoints, ordinary/Laser Sight initialization, repeated coordinate writes,
unrounded query endpoints, negative and deep coordinates, unsupported inputs,
and the non-equivalence of rounding at the native local origin.
