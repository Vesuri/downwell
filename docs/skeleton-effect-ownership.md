# Skeleton effect ownership before live admission

Source: untouched Windows `data.win`, SHA-256
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`,
UTMT 0.9.2.0 export. Resource metadata identifies objects; the corresponding
`Assembly/gml_*.asm` scripts establish argument evaluation and callback order.
This is an integration audit, not a claim that these effects are implemented.

## Death is not just removal plus a gem reward

`SkeletonActor::step` already exposes the source call sequence. Its live owner
must complete each effect's Create synchronously before running the next call.
Batching all effect creation after currency generation changes the shared RNG
stream, even when the final list of visible particles looks identical.

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

`DebrisBirth` implements that object-370 Create with four ordered RNG draws,
custom binary64 motion/rate fields and narrowed builtin animation fields.
`make verify-debris-birth` pins the input/export/resource/assembly identities
and compares 7,168 area/seed cases to scalar arithmetic and the complete RNG
state. Position/lifetime ownership, Step, alarms, collisions and drawing are
not implemented by this birth component; it is not live death integration.

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
