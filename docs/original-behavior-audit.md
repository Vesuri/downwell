# Original-game fidelity audit

Both ports must reproduce the Windows game. SDL is a development comparison,
not an oracle. Cross-target replay proves execution consistency for its tape,
not equivalence with Windows. Historical SDL-parity notes do not supersede this.

See also [activation lifecycle findings](activation-fidelity.md) for the
source-backed frontier/phase correction and remaining region-boundary gaps.

[Side-camera locker findings](side-camera-lockers.md) document the now-wired
VM-backed registration, latch/release and collision-phase camera snaps on both
adapters, plus the remaining original-runner and achievement caveats.

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

Entry-dither Create copies parent-camera 63's x, reads view 0's published y,
and sets Alarm 0 to 120. Step adds 16 to y and recopies camera x; Alarm 0
destroys it. Draw targets `global.surfaceFx`. Camera Create itself does not
publish the view, so substituting camera.y minus half the viewport height
for that getter is not justified by these events. Neither a dummy ID nor a
generic overlay reproduces this controller.

The stored adapter currently queues only 66/64 in player Create, omitting 73
and the full initializer's child births. Native terrain loading retains the
authored player/wall IDs but is not complete cross-family room creation.
Consequently, explicit-ID time-change tests do not establish the natural
global ID stream. This source check also does not prove original runner room
phase timing, initial view publication or rendered entry-mask parity.

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
