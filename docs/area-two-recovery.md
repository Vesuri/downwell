# Area-two data recovery

Run `make verify-area-two-patterns` to export and verify the catacomb tables.
The ignored output is `generated/downwell-area-two-patterns.json`. This is
an auditable intermediate representation, not a native asset pack or a
playable area-two implementation. The existing area-one pack is unchanged.

The exporter reads UTMT 0.9.2.0 VM assembly from the pinned Windows build
(`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`, bytecode 17).
It accepts only literal array assignments with a zero initial index, exact
unit increments and the source's final `[100]` maximum-index store. Computed
indices, extra array writes, unexpected string operations and unknown escapes
are rejected. It does not evaluate recovered GML.

Each family retains its original script/code IDs. Each record retains its
source index, original string resource ID, decoded
raw text (including CR/LF/tabs), normalized row view and every row's width.
The manifest records the original input hash, extraction tool version/commit,
manifest hash, each input assembly hash and exporter version/hash. Generated
payloads stay ignored; neither source game files nor bulk recovered patterns
are committed.

| Source array | Patterns | Row width |
| --- | ---: | ---: |
| `partStart` | 4 | 9 |
| `partMain` | 29 | 9 |
| `partEnd` | 5 | 9 |
| `partLeft` | 2 | 19 |
| `partRight` | 7 | 19 |
| `partShopR` | 3 | 19 |

Total: **50 patterns, 722 rows, 8,078 cells**. The two left-room tables are
preserved even though `scrLevelPat2` and common builder setup set
`leftRoomAmt=0`; exported data presence is not a claim that it is selected.

Tests include synthetic malformed instructions, exact raw-text/string-ID
preservation, irregular row widths and CRLF assembly files. All real VM
literals are also compared against the independently decoded GML view;
disagreement fails verification instead of changing the VM export. Repeated
generation followed by `--check` verifies deterministic output.
The final gate and a second independent `--check` pass; evidence is in
`/tmp/downwell-area-two-patterns-final.log`. No emulator run is needed for
this exporter-only change, and no playable build options were changed.

## Builder integration contract

`scrBuilderCreation` selects `scrLevelPat2` for area 2. VM setup gives
`extraColumn=3`, `levelLength=13+global.level`, `genCount=-10`, and
`rightRoomAmt=choose(2,2)` (overridden to 3 on level 3). That apparently
redundant choice still consumes RNG. The per-character builder dispatches
to `builderCatacomb`, not `builderCavern`; its symbol/RNG/object rules must
be implemented before reusing the common pattern traversal.

`src/core/CatacombBuilderSymbols.h` now implements the area-specific symbol
dispatch for levels 1--3, transcribed from VM blocks 17--61. It keeps every
`choose` argument list, the overwritten `%` `irandom(level)` call, the
hard-mode override order and synchronous right-then-left paired skull Create
calls. Common terrain/reward symbols remain the common builder's responsibility.
An emission failure stops immediately; unsupported levels fail before any
operation. Coordinates are supplied after the common traversal computes them.

`make verify-catacomb-symbols` checks 306 operation-trace cases: every choice
index (including duplicate values), all three levels, both hard-mode states,
both random-threshold outcomes, object IDs/coordinates and emission failures.
The same cases pass AddressSanitizer/UndefinedBehaviorSanitizer. Pattern export
verification still passes with the unchanged output hash above.

This is shared semantic code, **not yet connected to either live generator**.
Its operations interface intentionally does not bind `choose` to WELL words:
in particular, the singleton `choose(126)` is present in the VM but its runner
word consumption has not been established here. The operation-trace test proves
dispatch/call ordering, not historical Windows RNG equivalence. Integration
also needs concrete synchronous actor creation and any observed builder-instance
state such as `spawnChance`; this helper currently owns only emission decisions.
No emulator run or frame-rate improvement is claimed for this isolated change.

**Follow-up:** the singleton uncertainty above is now resolved by direct
inspection of the Windows executable: `choose` consumes one word, even for
one argument. See `runner-choose.md`. Existing cavern choice translations
must be corrected before using their RNG checkpoints as integration evidence.

## Actor integration findings

The next actor is not just another entry in `EnemyCreateProfile`: that table's
nonzero health also admits objects to `GameplayWorld::createGeneratedInstance`.
Adding stats before motion, collisions, presentation and alarms exist would
silently register an incomplete actor. Keep that supported-actor gate intact.

`enmJumpySkull` Create VM explicitly assigns `imgSp=0.175`. Existing supported
actors use twentieth-frame rates, which cannot represent this value.
`advanceEnemyImageFortiethsQ8` now supports 7/40 without floating-point target
work. Its residual unit remains fifths of one Q8 phase unit (256/40=32/5).
The existing twentieth-frame API delegates with twice the rate, preserving
all current stored rates and residuals. Forty updates at 7/40 produce seven
frames exactly. This represents the recovered rational rate, not a claim of
bit-identical historical floating-point accumulation at every boundary.

The presentation tests check 50,000 old/new-equivalent updates and 50,000
fortieth-rate updates, including zero speed, nonzero initial residuals and
rate changes. They pass normally and under address/undefined-behavior
sanitizers. Both natural host routes, upgrades, re-entry and the seed-4 shop
return pass (`/tmp/downwell-fortieth-animation-natural.log`). These are
existing-actor regression checks, not a playable skull verification.
The ordinary Amiga build and its 68000 instruction/64-bit-helper audits also
pass (`/tmp/downwell-fortieth-animation-amiga.log`). No emulator was launched
for this arithmetic-only change, and no new timing measurement is claimed.

Further source inspection identifies important prerequisites: skull Step uses
thin-wall versus solid collision handling, hit stun, viewport-gated movement,
landing re-arming and time-stop alarm compensation; its collision events can
also trigger jumps with a different random range. Skeleton throwing requires
synchronous bone Create before its velocity overrides; the bone Create itself
draws random values. Skeleton also uses alarms 2 and 3, beyond the two direct
alarm fields of the current shared enemy state. Spider Create makes ordered
line-collision queries to decide and shorten its hanging web. These cannot be
replaced with cavern behavior just to make the new patterns run.

### Skeleton throw orchestration

`SkeletonDecisions` now joins `SkeletonAnimation` with the recovered Step
activation, hit, sprite, facing and throw gates. The owner supplies precise
coordinate comparisons rather than rounding player coordinates into this
component. Initial activity is true, as in Create. A hit activates the actor,
extends Alarm 1, resets the image, emits sound 133 before stopping `imgSp`,
arms Alarm 3 and clears the hit flag. Idle-to-throw entry can subsequently
restore `imgSp=0.3` in that same Step; this order is intentional.

The throw callback runs only above frame 4 with an unset shot latch, before
horizontal velocity can override facing. Above frame 6, nonzero desired speed
stops and arms Alarm 2 once. Time-stop skips these decisions but not the final
hit-stun sprite override. That override is exposed separately so the owner
can preserve motion/death/effect processing before it, rather than moving it
earlier for convenience. Callback capacity failure returns failure without
setting the shot latch. These callbacks still need the live projectile owner.

4,608 scalar decision cases cover sprites, threshold sides, activity, hits,
time-stop, latch state, player comparisons and motion sign. Sanitizers and
animation/throw regressions pass; the combined 68000 compile has no undefined
helper symbols. This is component evidence, not a complete source-runner frame
trace: VM numeric-comparison tolerance at near-equal frame boundaries still
needs checking, along with live allocation, motion, death and collision order.

`SkeletonTiming.h` now owns the four alarm values and the timing writes from
the skeleton's Alarm 0–3 and Step events. Alarm 0 clears `edamaged`; Alarm 1
clears `hitStun`; Alarm 2 resets `image_index` and `eShotDelay` and writes 15
to Alarm 3; Alarm 3 assigns the exact binary64 0.3 to `imgSp`. These bodies
are present in the exported VM assembly, including Alarms 2 and 3.

Step copies `imgSp` to `image_speed` before processing a hit or stopping a
completed throw. Those later writes therefore do not change the current
Step's image speed. A hit adds five to Alarm 1 (including -1 becoming 4),
resets the image index, stops the desired rate and writes 30 to Alarm 3.
Time-stop sets image speed to zero and compensates only positive Alarm 0;
it does not freeze the other alarms. Tests cover 64 combinations and the
throw/restart sequence, including repeated hits and delayed speed changes.

The component does not advance images. Original-runner disassembly now
resolves cross-alarm scheduling: indices are processed globally in ascending
order with immediate dispatch, so Alarm 2's write to Alarm 3 is decremented
in the same phase. `advanceAlarm(index)` exposes this ordering to the future
owner; see `runner-alarms.md` for addresses and remaining instance-order scope.
No skeleton is enabled by this component, and no native frame-time claim is made.

`SkeletonThrow.h` implements `enmSkeleton_Step_0` VM blocks 21–26 as a shared
operation sequence: synchronously create object 269 at `(x,y-10)`, emit sound
134, choose vertical speed from -3/-3.1/-3.2, conditionally choose horizontal
speed from 1/0.8/1.2 times current facing, assign velocity, reverse animation
according to horizontal direction, then latch the shot. Exact alignment with
the player skips the horizontal choice and sets +1 regardless of facing.
The helper exposes velocities as integer tenths so these constants do not
acquire Q8 rounding before a projectile representation is selected.

`make verify-skeleton-throw` compares operation order and all RNG words for
9,216 seeded/alignment/facing combinations, plus allocation failure. Its
Create callback consumes the three explicit bone-Create random-range draws
before either throw choice. The tests pass normally and under ASan/UBSan.
This is a contract test for synchronous creation, not an implementation or
verification of the bone's complete inherited Create or lifecycle. The sound
adapter must preserve `soundPlayOL(134,50,0,1,"enemymove")` at the callback.

This helper is not yet wired into live enemies. Active/image-frame/shot-delay
gating, bone storage and motion/collisions, alarms, hit/death effects and actor
registration remain necessary. No new playable actor, native timing result,
or original-runner playback comparison is claimed by these unit tests.

Live catacomb integration is tracked in [the work queue](open-work.md).
Recovered tables alone do not justify removing the later-area guard or
substituting cavern logic, and do not establish native performance or
Windows visual fidelity.

### Bone projectile numeric/event reference

Manifest object 269 (`bulletEnmBone`, sprite 460) inherits object 90
(`parentShootableThing`). The parent's Create sets health/hit flags and has no
random calls. Bone Create explicitly draws horizontal speed, vertical speed,
then animation speed. `BoneProjectileReference` preserves these double-valued
operations as a host oracle, not a deployed 68000 implementation. Compile this
reference with `-ffp-contract=off`: the initial test exposed host fused
multiply-add differences compared with separate VM instructions.

Step follows VM order: restore animation rate, apply gravity, accumulate
position, round, handle health destruction, then perform strict anchor-based
`scrInView(0,0,0)` culling. Time-stop pauses motion/health cleanup, but not
culling. Wall collision destroys before emitting effect 85 at speed 0.3;
active player collision applies `scrTypicalDamage(1,3,2)` only when not
time-stopped, then emits the effect and destroys even during time-stop.
Callbacks preserve the source's continued execution after instance_destroy;
the eventual instance owner must defer storage retirement appropriately.

`make verify-bone-reference` passes 1,024 full Create RNG-state cases,
102,400 motion/time-stop updates and collision, health, rounding and strict
viewport-boundary cases, normally and under ASan/UBSan with FP contraction
disabled. These tests establish the reference and event contract, not an
independent Windows execution trace. Draw (`drawShade`, `draw_self`), animation
advancement, masks, live collision dispatch and damage/effect implementations
remain external. A target motion representation still needs comparison against
this oracle before enabling bone actors in either playable backend.

### Numeric representation check and throw/Create integration

The skeleton throw tests now invoke the real bone numeric-reference Create,
not a callback that merely burns three RNG words. All 9,216 cases check the
child position, overridden velocities, signed randomized animation rate,
unchanged initial image speed and complete RNG state. Normal and sanitizer
runs pass with floating-point contraction disabled.

Exact integer tenths are **not** an exact substitute for repeated VM double
addition. New regression cases demonstrate differing rounded Y positions:

| Starting Y | Initial vertical speed | Moving updates | VM reference Y | Exact-tenths Y |
| --- | --- | --- | --- | --- |
| 0 | -3 | 5 | -13 | -14 |
| 32 | -3.1 | 10 | 7 | 6 |
| 128 | -3.2 | 25 | 81 | 80 |
| 704 | -3 | 114 | 1017 | 1018 |

Stopped Steps are interleaved in these tests and must not advance either
trajectory. These are numeric counterexamples, not claims that all cases
occur before live wall collision/offscreen retirement. No constant epsilon
or rounding-mode change was introduced to hide them. Native integration
must either preserve the relevant floating-add rounding or establish a
source-backed bounded domain where a cheaper representation produces the
same observable positions. A simple tenths implementation is not yet accepted.

### Integer-only binary64 motion arithmetic

`MotionBinary64` provides the narrow alternative: normal finite binary64
addition and ties-to-even signed pixel rounding, implemented with pairs of
32-bit words. It uses explicit guard/round/sticky bits and preserves cancellation,
signs and rounding carries. It rejects subnormal operands/results, NaN,
infinity and overflow rather than silently approximating them. No multiply,
divide, host double or 64-bit integer operation appears in the implementation.

`make verify-motion-binary64` checks 200,000 mixed-sign/magnitude additions
against host double bits, plus 24,000 bone-trajectory updates (velocity bits,
position bits and rounded pixels), signed zeros, cancellation, halfway cases,
and rejection without mutation. Normal and ASan/UBSan runs pass. The fixture
`tests/platform/amiga/MotionBinary64Compile.cpp` cross-compiles at `-O3
-m68000 -msoft-float` to a self-contained 2,398-byte function with no undefined
symbols: no floating-point or 64-bit arithmetic helpers. Disassembly is saved
at `/tmp/downwell-motion-binary64-68000.s`.

This is not yet linked into live projectiles. It still needs native execution
comparison and frame-cost measurement before selecting it for the bone owner;
code-generation success alone does not prove a 20 ms budget. The skeleton's
fixed choice constants can be supplied as reviewed binary64 words; converting
random animation speed and the remaining lifecycle is separate work.

### Native numeric replay and cost

The cross-target replay now additionally executes `MotionBinary64Verification`:
24,000 velocity/position/rounded-pixel updates, whose host checksum is checked
against independently evaluated double trajectories. The A500/68000 run matches
`adb59284` with zero failures; the benchmark final-state checksum also matches
`000505fa`. The four pre-existing gameplay/gem/death/weapon digests still match,
and the 256-byte task-stack guard survives. Integer-coordinate conversion is
also tested against 10,000 host double conversions. Host sanitizer tests pass.

A separate 1,000-update native batch, without per-update hashing, takes 23 PAL
fields: approximately **0.46 ms/update**, with less than 0.02 ms/update endpoint
quantization uncertainty. This measures two binary64 additions and pixel
rounding plus loop/failure bookkeeping, under the verification display/DMA
configuration. It is not the cost of complete bone motion (horizontal updates,
animation, collisions, rendering and owner traversal are not included), nor
a live-game frame-time guarantee. No storage allocation occurs in the loop.

Evidence: `/tmp/downwell-motion-native-replay.log` and
`/tmp/downwell-motion-native-gdb.log`. The emulator had a 180-second deadline,
was stopped afterward and the ordinary executable was restored. Both native
instruction/helper audits pass. Arithmetic is still verification-only, not
yet installed in live projectile state.

### Complete launched-bone motion component

`BoneMotion` now owns both binary64 positions, both velocities and rounded
pixel coordinates using integer-only arithmetic. `launch` accepts the source
skeleton's supported velocity choices as integer tenths, resolves their exact
binary64 constants, and consumes no RNG. The full child Create must still run
before this override. Each Step preserves VM order (gravity, X, Y, pixel
rounding); stopped Steps do nothing. Unsupported initialization or arithmetic
fails without partially updating the motion state.

The host test compares 201,600 two-axis updates across all seven horizontal
choices, three vertical choices, 32 starting positions and interleaved time
stops with `BoneProjectileReference`, checking all position/velocity bits and
rounded pixels. ASan/UBSan pass. The 7,000-update native replay checksum
`2992916f` matches a separately stepped host double oracle, as does benchmark
checksum `c2a61ce4`. Existing replay digests and the task-stack guard still pass.

The isolated full-motion benchmark takes 45 PAL fields per 1,000 updates:
approximately **0.90 ms/bone update**, less than 0.02 ms/update endpoint
quantization uncertainty. It includes three additions, two pixel roundings,
failure checks and state publication; it excludes animation, collisions,
effects, rendering and actor traversal. This cost must be budgeted before
enabling many simultaneous bones; it is not a 50-FPS result.

Evidence: `/tmp/downwell-bone-motion-native.log` and
`/tmp/downwell-bone-motion-native-gdb.log`. The bounded emulator completed,
stopped and restored ordinary startup with clean 68000 audits. The motion
component is ready for ownership integration, but no skeleton or projectile
has been enabled in the playable game. Health/collision event handling,
animation and presentation remain outstanding.

### Integer-only bone event state

`BoneProjectile` now joins verified two-axis motion to post-launch Step and
collision handling. It owns health, active/mask state, animation selection
(instance default, sampled rate or stopped), and a destruction-request latch.
Callbacks retain the original effect/damage order and do not free storage.
In particular, Step still queries view membership after health destruction;
wall contact destroys before its effect, while player contact emits damage
(unless time-stopped), effect, then destruction. An inactive bone ignores
player contact but not wall contact. View membership is supplied by the owner
so fractional camera coordinates are not silently rounded.

`make verify-bone-projectile` compares 6,300 combinations of starting position,
velocity, health, activity and time-stop against the recovered reference,
including half-pixel viewport edges and both collision dispatches. It checks
motion bits, pixel coordinates, animation state, mask, destruction requests
and ordered effects. Normal and ASan/UBSan tests pass. The expanded 68000
compile fixture includes Step and both collisions with no undefined symbols.
This is code-generation evidence, not a new native lifecycle replay.

The component deliberately does not sample or advance animation, allocate
instances, run mask collisions, apply player damage or render effects itself.
Those belong to the forthcoming shared projectile owner/backend integration;
the callbacks must be wired rather than dropped. It is not registered in live
skeleton spawning yet, and no new frame-time claim is made.

### Exact target-side bone Create ranges

`RandomBinary64` now computes the Windows `random` scale operation
`(uint32 / 2^32) * positiveScale` using three 32-bit product limbs and one
ties-to-even rounding. Its partial products are 16x16-bit multiplies; no
64-bit arithmetic or target floating point is required. It is restricted to
finite nonnegative scales with normal results (or zero), not a general
floating-point multiplier. Invalid/unsupported inputs preserve the output.

`BoneProjectile::create` uses it for all three explicit random-range calls,
then applies the lower-bound additions with `MotionBinary64`. Inherited
parent flags and default animation selection are reset, and the sampled rate
is retained without converting to Q8 or decimal tenths. Skeleton overrides
must still occur afterward, without removing the two overwritten draws.

Tests pass 600,000 exact scaled-word cases, 1,024 complete Create values and
full RNG states, 102,400 subsequent two-axis/time-stop updates with unmodified
random velocities, and the existing 6,300 event cases. ASan/UBSan also pass.
The conversion fixture cross-compiles for 68000 without undefined helper
symbols. This change has no native execution/timing claim yet and is not
connected to playable spawning. Animation advancement, shared instance owner,
collision dispatch and presentation remain integration work.
