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
60/93; these methods do not authorize queries against incomplete projectile
parent families. Point queries accept the original precise flag. Line queries
explicitly support only precise=false, not an approximation of bitmap lines.

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
`NonLoopSpriteAnimation` supplies the common sprite-change and Animation End
state machine, also used by the live corpse-expiry child. Its admitted advance
path requires verified nonempty unit-key sprite metadata. After the muzzle
script installs its one-frame sprite and speed, the first animation update
reaches index 0.5 without retirement; the second wraps to zero and requests
destruction. Owners must exclude Step-born effects from the already-started
animation phase; a two-tick state machine alone does not establish that global
ordering or the effect's Draw order.

Run `make verify-machine-gun-source-motion verify-smoke-direction
verify-smoke-trig`. The source-motion test covers birth narrowing, raw RNG
endpoints, ordinary/Laser Sight initialization, repeated coordinate writes,
unrounded query endpoints, negative and deep coordinates, unsupported inputs,
and the non-equivalence of rounding at the native local origin.
