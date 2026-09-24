# One gameplay simulation, two platform backends

The destination is one portable, 68000-compatible gameplay implementation in
`src/core`, used unchanged by SDL and Amiga. The Windows VM/resources are the
behavioral authority. Agreement between two independent adaptations is not a
substitute for this architecture or original-game validation.

Current status: main-room player movement, gun/jump state, animation state and
knockback now use `PlayerMotion` in both runtimes. World storage/adapters, enemy,
bullet, reward, camera and menu simulation still need consolidation. Native
world classes contain domain logic that does not belong under `src/platform`.
Rendering/storage optimizations do not justify separate gameplay rules.

## First migration slice

The sections below record the migration chronologically; later sections
supersede the earlier descriptions of what remained unimplemented.

`PlayerControls.h` owns the default non-water horizontal input rules and their
constants. Both existing runtimes call it; their former rule branches have
been removed. Focused source-backed tests execute this same implementation
with both temporary arithmetic adapters. `global.noControl` now explicitly
zeros horizontal velocity as the original script does.

The float adapter is transitional compatibility, not the final architecture.
It permits extraction without simultaneously changing every stored coordinate.
The final shared state must use one fixed-point representation on both CPUs.

The uncommitted native-only ceiling experiment was removed. The confirmed
ceiling omission remains unresolved and must be fixed in shared collision code,
including consistent rounding, masks and event order. It must not be lost from
the backlog merely because the separate experiment was discarded.

## Remaining migration

1. Consolidate player state, vertical integration, jump/gun state and collision
   ordering in core; provide adapters for existing presentation consumers.
2. Consolidate enemy, bullet and reward state/update code, retaining original
   event timing, masks and deterministic RNG consumption.
3. Make SDL and Amiga invoke the same world update. Remove the replaced
   simulation branches, rather than leaving two permanent selectable modes.
4. Keep the Amiga terrain cache, circular DMA scrolling, Copper lists, blitter,
   offline sprites, audio and input translation in its backend. SDL retains its
   own rendering/audio/input backend, not different gameplay.
5. Replace port-versus-port gameplay agreement tests with shared-core rule tests
   against VM evidence, plus cross-CPU replay and backend presentation tests.

Cross-target execution checks remain useful during migration. Their passing
status does not imply that this migration or original-game fidelity is complete.

First-slice validation: full host suite and bounded A500 replay pass with
unchanged native digest `bfbbd762b9b62a59`; both 68000 audits are clean.
SDL was rebuilt. The known ceiling and extended-combat failures remain open.

## Player-state extraction

`src/core/PlayerMotion.{h,cpp}` now owns the previously native player state and
update routine. The Amiga player header is only a compatibility alias; there is
no platform-specific player implementation file. Both the host test build and
the actual 68000 build compile the core implementation. Its behavioral tests
have moved to `tests/core/PlayerMotionTest.cpp` and include the core directly.

This relocation intentionally changes no simulation behavior. SDL's
`StartupRuntime::updateMainPlayer` has **not yet** been replaced. In particular,
the extracted implementation lacks upward ceiling collision handling that the
SDL path currently provides. Wiring SDL to it without resolving that omission
would remove implemented behavior, not establish fidelity. The original
`scrCheckCollisionWith` uses rounded predictive probes and a diagonal fallback;
`scrWallCol` also handles breaking boxes above the player. Simply transferring
the SDL axis loop would not completely reproduce those source rules either.

Next migration gate: finish integrating the shared source-backed collision
contract, then route SDL player motion, jump/shoot state, stomp and contact
responses through `PlayerMotion`, removing its old authoritative state updates.
Rendering mirrors may remain temporarily, but must not become a second writer.

Extraction validation: full host suite passes, including the relocated core
test; bounded cross-target replay retains `64 bfbbd762 b9b62a59`. The 68000
instruction and 64-bit-helper audits pass. This verifies the relocation, not
completion of SDL integration or original-game fidelity.

## Shared ceiling collision

`SolidCollision.h` transcribes `scrCheckCollisionWith` VM blocks 1..40:
rounded predictive probes, integer contact walks and the diagonal fallback
(including its reset of both displacements at the vertical limit). Tests cover
those branches, ties-to-even with negative coordinates, fractions and zero
velocity. The fixed arithmetic uses fifths of Q8, preserving the player's
gravity residual without floating-point or 64-bit target operations.

Both current runtimes now call the same vertical branch for upward contacts.
The complete two-axis routine is tested but is not yet the runtime collision
path: the current callers still integrate horizontal movement separately.
Calling the whole routine with a synthetic zero horizontal velocity was tested
and rejected: its rounded horizontal probe could snap fractional X into a wall.
The vertical-only adapter explicitly preserves X, with a regression fixture.

This removes the native missing-ceiling response. The full host suite passes.
The extended combat tape still fails, but its former first movement mismatch
at tick 666 is gone. The first combat mismatch remains at tick 702 (health 3/4),
and the first reported movement mismatch is now 704 after that knockback.
Final health is 3/3, kills 2/1, currency 18/6. These are diagnostic results,
not new expected values or relaxed acceptance criteria.

Still required: box-above destruction and its downward impulse, shared mask
semantics, full source event order, and replacing the SDL player state/update
with `PlayerMotion`. Neither the existing side-contact loop nor the floor
resolver should be mistaken for the complete original collision script.

Target verification: the bounded 64-frame cross-CPU replay passes with
unchanged digest `bfbbd762b9b62a59`, and both 68000 audits are clean. This replay
checks target execution stability, not the newly added ceiling case; the
source-branch fixtures and the longer host combat trace exercise that case.
The normal Amiga executable and playable SDL binary were rebuilt.

## Shared box-above response

The ceiling response now follows `scrWallCol` assembly blocks 32..35 in both
runtimes: query the player mask at `(xx, yy-16)` against object 92, select the
first overlapping box, and use downward velocity 1 (otherwise zero) before
same-frame vertical integration. `ceilingResponse` and `playerMaskMeetsBox`
own those rules; world adapters own the instance mutation.

The Amiga adapter emits the head-broken box through the existing destroyed
terrain list, so the backing terrain and visible buffers receive the existing
cache invalidation. That list now holds bullet capacity plus one, accounting
for a head collision and bullet hits in one frame. The player callback is
bound only during the update and cleared afterwards: no stack callback context
survives the frame or a later runtime copy/reset.

Controlled host fixtures exercise the SDL live room and the portable native
runtime: a box 24 pixels above the starting player is broken on the third
held-jump update; the player ends 11 pixels above the start, moving downward
without becoming grounded. Native tests also check the exact cache-update
coordinates and absence of repeated invalidation. Shared tests check the
yy-16 probe, ordinary ceiling response and inclusive mask edge overlap.

Limitations retained explicitly: adapters currently retire the box immediately,
as the existing bullet adapters do. The original marks wallHp zero and processes
destruction in the box's ordinary Step, with sound, smoke/flash/debris, optional
embedded currency and upgrade-dependent chain reactions. That complete event
and its ordering still need a shared world implementation; this change does
not claim those effects are present. Original collision-mask selection and
the full two-axis script also remain ahead of final player-state consolidation.

Validation: full host suite passes; bounded cross-target replay still returns
`64 bfbbd762 b9b62a59`, with clean 68000 instruction/helper audits. Both playable
binaries were rebuilt. The extended combat failure remains unchanged at tick
702; this box fixture does not resolve enemy/contact-state divergence.

## SDL now executes PlayerMotion

`StartupRuntime::updateMainPlayer` now calls the same `PlayerMotion::step` that
Amiga calls. SDL's independent gravity, horizontal/vertical movement, jump,
gun delay/recoil and animation-selection branches were removed. Stomps and
contact knockback also update the shared player. Legacy SDL-facing fields are
presentation/diagnostic mirrors populated by `syncMainPlayerState`, not a
second simulation. Bullet creation and sound requests consume state transitions.

SDL world adapters scan its existing instance store for solids, support and
boxes. They are rebound on every update and cleared afterwards, so copying a
runtime does not retain callbacks into the old instance store. The Amiga keeps
its indexed terrain queries and hardware rendering backend. Both paths now use
the same Q8 state; SDL no longer uses the transitional float control adapter.

Source `scrPlayerInit` gun delay (30 updates), held-entry fire lock and
`global.noControl` are explicit shared initialization/control inputs. The live
Amiga entry now initializes these too; synthetic grounded replay fixtures keep
their explicitly configured state. Entry lock/delay/control tests cover this.

Regression changes are not new original-game oracles: the former SDL wall
position 292.8 becomes 292.9375 with Q8 acceleration; both use clear mask pixel
292. The wall assertion now checks that pixel rather than a float remainder.
The longer tape selects right-room pattern 3 instead of 6, with the same row
count and six rather than seven special markers. The recovered pattern 3 has
no I/J torch, so its utility-object assertion reflects that; the independent
builder tape retains an explicit torch-emission check. The generator itself
was not modified. These checkpoints test the migrated simulation, not exact
Windows RNG consumption for that route.

The scene test now requires exact Q8 horizontal agreement throughout its main
comparison tape and exact vertical agreement through entry, landing and the
initial jump/shoot sequence, replacing former pixel tolerances. Later world
adapter/camera tolerances remain explicit. Extended combat still first differs
at health tick 702: enemy motion/contact decisions have not yet been unified.

Known approximations in the shared player are not promoted to source truth:
Q8 horizontal constants, the existing axis/support adapters instead of full
two-axis VM collision order, integer knockback distance, and simplified box
retirement remain fidelity work. Sharing these rules makes corrections apply
to both backends; it does not by itself prove the rules correct.

SDL replay hashing now includes the authoritative player's timers, sub-Q8
residuals, animation phases and input locks, not merely its float mirrors. A
unit fixture distinguishes two delayed guns with identical display fields but
different countdowns; callback addresses are deliberately excluded.

Migration validation: full host suite passes, with no change to the startup
replay pin. Bounded 68000 cross-CPU replay retains `64 bfbbd762 b9b62a59`; the
600-tick A500 terrain-cache oracle also passes with the live entry path. Both
68000 audits are clean. SDL was rebuilt and the normal Amiga build restored
after the skip-to-game verification. Extended combat remains a failing gate,
not a suppressed or relaxed test.

## Shared active-bat motion

`BatMotion.h` replaces both native ten-degree-bin/cross-product steering and
SDL floating-point steering. Both adapters execute the same fixed-point update
for active, non-stunned bats. Source references are `enmBat_Create_0`,
`enmBat_Step_0` (including the VM's ordered movesp branches and strict signed
turn tests), and `scrPlMovement`'s publication of global.eplx/eply before player
position integration. `PlayerMotion` retains that frame-start snapshot for
both backends; the former paths targeted the post-movement player instead.

The shared routine consumes exactly one RNG word per active update and retains
fractional random_range jitter in Q16 degrees. Speed is represented in exact
fortieths: +.05 inside the cone, -.025 outside, and a separate -.2 when above
1.25. Thus 1.225 accelerating to 1.275 becomes 1.075, not a clamp to 1.25. Zero
angular difference does not turn. Native Q8 storage round-trips all these speed
steps; both adapters preserve Q8 positions and velocities.

"Exact fortieths" describes the rational constants, not GameMaker's binary
floating-point accumulation. Speed comparisons near 1 and 1.25 still require
original-runtime validation for floating-point boundary effects, just as Q8
velocity integration is not an exact double-precision trajectory oracle.

CORDIC computes point_direction without soft-float, and a mathematical Q15
quarter-sine table supplies integer-degree lengthdir components. Host tests
compare all quadrants and several radii against atan2, with maximum observed
angle error 0.00174774 degrees (required bound .004), plus every table angle
against sin within half a Q15 unit. This is bounded numeric approximation, not
bit-identical GameMaker double math; a decision extremely close to a threshold
can still differ. A 1000-update fixture checks translation invariance between
the backends' coordinate systems. Live scene comparisons also check exact bat
position, direction and velocity whenever the paired bat is active.

The prior first combat mismatch at tick 702 moves to 712. At 712 the bat's
precise coordinates agree (298.418,2326.55), but player damage differs. Native
contact currently uses a truncated player pixel and rounded enemy pixel;
SDL uses precise positions. Its generic contact rectangle is not yet the
original sprite-mask collision event. Source typStompable also compares to the
pre-movement global.ply. This is the next shared-contact migration, not a
reason to silently widen collision tolerances. Extended combat remains failed.

Other open bat fidelity work: initial hanging/activation and hit-stun branches,
full sprite-mask scrCheckCollisionWith (current adapters still query points),
and original time-stop/water/upgrade behavior. Sharing steering is not proof
of complete bat or game fidelity.

Validation: full host suite (including angle/speed/translation fixtures),
bounded 64-frame cross-target replay and 600-tick A500 terrain-cache oracle
pass. The canonical replay digest remains `bfbbd762b9b62a59`; that short tape
is not a standalone proof of bat accuracy. Target instruction and 64-bit
helper audits pass. Normal SDL/Amiga builds are restored after verification.

## Shared enemy contact classification

`EnemyContact.h` replaces both backends' generic contact region with the
recovered axis-aligned player/enemy mask rectangles. A metadata verifier checks
all eleven supported sprite rectangles against the pinned original export,
including the source hash and extractor version. Frogs, ground turtles, snails
and crawlers retain fixed masks instead of using animation bounds. Instance
coordinates use ties-to-even rounding; unit flips transform pixel-cell edges
about the sprite origin. Edge-only touching is not overlap.

The shared stomp decision follows typStompable's strict comparison of enemy yy
against the pre-movement global.ply + 2, not the newly integrated player y.
Fixtures cover the short bat mask, asymmetric horizontal flip, fixed masks,
fractional stomp thresholds and coordinate translation between the adapters.

The 600-update combat continuation now finishes with matching health 3, kills 2
and currency 16, with no intermediate disagreement in those counters. It is now
enabled in the default host test target. Its existing movement check only
rejects vertical differences greater than three pixels: this is not an exact
whole-world state or original-runtime proof. No tolerance was widened to pass.

Remaining contact fidelity work includes knockback's integer direction
approximation, enemy activation/event ordering, and original-runtime boundary
validation. Unit-flip rectangular masks do not cover arbitrary scaling,
rotation or precise per-pixel masks. Sharing this classifier does not establish
complete gameplay parity.

Validation for this change: the bounded 64-frame 68000 cross-target replay
retains `bfbbd762b9b62a59`; the bounded A500 600-tick terrain-cache rendering
oracle passes. Instruction and 64-bit helper audits remain clean. These target
checks complement, rather than replace, the host contact and combat fixtures.

## Damage launch corrections

The VM for scrTypicalDamage targets global.plx/ply, not the player's already
integrated position. Shared contactKnockback now uses that frame-start Q8
position and the enemy's rounded instance position. Keeping distance in Q8
also removes the former large integer-pixel normalization error: a (1,1)
diagonal no longer produces a full three-pixel horizontal impulse.

scrFjump's VM calls scrRecharge and only assigns xsp when its horizontal
argument is nonzero. Damage launch now refills the default eight-round gun
and preserves existing horizontal speed for a zero impulse. Tests exercise a
moving, ammo-depleted player and check pre-movement targeting, recharge,
vertical launch and diagonal magnitude. Both backends use this same routine.

This is not full scrFjump/scrRecharge fidelity yet: spinJumping presentation,
jetFrame reset, recharge effects/audio/shake, variable weapon capacity, and
coincident point_direction behavior remain to be reconciled. Integer square
root and Q8 velocity remain approximations of GameMaker double math.

## Shared default-style spin state

PlayerMotion now owns spinJumping for directional ground jumps, stomps and
damage launches. VM references: scrJump sets spin for left/right input and
resets image_index; scrFjump sets spin without resetting image_index;
scrPlMovement suppresses ordinary facing changes while spinning and clears
spin on landing; scrUpButtonFunctions clears it on unlocked held fire.
playerSpriteControl selects spriteSpin, configured as resource 1 at speed .3.

The eight-frame animation is shared by both renderers. Twentieth-frame phase
retains existing .1, .25 and .3 animation fractions when a forced jump switches
to spin; a normal jump resets phase. Input still accelerates horizontally
while the spin-facing lock is active. Tests cover reversal, release, shooting,
stomp and landing; the extended combat test now checks player sprite/frame
agreement on every update as well as its prior counters and movement checks.

The host asset generator supplies all eight planar frames, mirrors and prepared
hardware-sprite streams. There is no new runtime sprite conversion. Resident
art increases to 274,440 bytes, validated by the bounded 512 KiB Chip A500
startup allocation check before increasing the art-only regression cap.

Still incomplete: spin wall-contact poses/wall kicks, ending overrides,
non-default styles and exact original runner image-phase timing at event
boundaries. The normal-jump first frame follows the existing host animation
timing convention; cross-backend agreement alone does not prove that timing.

Validation: full host suite and extended combat presentation checks pass;
the renderer compares 13,916 offline variants. Bounded 68000 replay and A500
600-tick terrain-cache checks pass, with clean target instruction/helper
audits. Normal SDL and Amiga builds are restored. The short replay checksum
is unchanged; it is not a complete digest of animation state.

## Player-complete cross-target checkpoint (schema 3)

PortCheckpoint now calls PlayerMotion::appendStateDigest instead of sampling
only integer position, ammo, grounded state and jump/shot counters. Thus the
actual 68000 replay comparison includes subpixel positions/velocities,
fractional remainders, pre-movement position, shot delay, input locks, spin
state and animation phase. Both SDL state hashing and target verification
reuse the same serializer. Callback addresses remain excluded deliberately.

A regression fixture gives two players identical display fields but different
shot countdowns and requires different checkpoints. Schema 3's standalone
fixture is `860ff3c3941bc1a5`; the unchanged 64-input tape's host trajectory is
`9f0d3943e7900d2b` instead of schema 2's `bfbbd762b9b62a59`. This is an explicit
serialization change, not a gameplay correction or a relaxed comparison.

Scope remains limited: the checkpoint retains only the first bullet and does
not yet serialize the complete enemy/gem/world state. The tape is still 64
updates. Normal Amiga builds compile out checkpoint calculation, so the extra
verification work does not consume the playable build's frame budget.

Validation: bounded host/68000 replay agrees on `64 9f0d3943 e7900d2b`;
target instruction and helper audits pass, and the verifier restores the
normal Amiga build. No input tape or gameplay behavior changed in this slice.

## Shared frog preparation decision

`FrogBehavior.h` implements the enmFrog Alarm 1 decision shared by both
adapters. The VM requires grounded AND scrInView(0,0,0); the latter uses strict
inequalities at all four view edges. It then compares global.plx to rounded
instance x with GT, so equality chooses left. The adapters now supply the
pre-movement player snapshot and the camera before its ordinary Step update.
The former implementations omitted the visibility gate and used the newly
integrated player position; native also incorrectly chose right at equality.

Fixtures cover all four edges, fractional camera/player positions, airborne
retry, translation between coordinate systems, and the native alarm path
where player movement crosses the frog's X within the same tick. A frog on
the exact right view edge retries alarm 1 at 60 rather than starting alarm 0.

This does not finish frog consolidation. Horizontal/vertical collision order,
full source-mask probes, gravity integration, hit-stun and water handling still
differ or are incomplete. Camera state itself also remains independently
implemented; sharing the visibility predicate does not fix camera differences.

The corrected alarm changes the finite combat tape's outcome on both ports:
health remains 3/3, kills become 1/1 and currency 12/12 (previously 2/2 and
16/16). All per-update comparison checks still pass and the tape still
exercises kills and collection. No test tolerance or coverage was reduced.

## Frog fractional displacement and strict motion diagnostic

Native frog gravity retained fifth-Q8 velocity remainders but discarded them
when integrating yy. Starting at y=1, its first two updates displaced 114 Q8
units instead of 115 + 1/5. The source .15 gravity and yy += ysp require both
velocity and position residuals. FractionalMotion::integrateFifthsQ8 now keeps
that displacement, including negative velocities; landing clears the residual.
The forty-update free-fall fixture checks the closed-form displacement and
both remainders, rather than comparing against SDL's implementation.

`DOWNWELL_VERIFY_FROG_MOTION=1` adds a strict Q8-normalized position/velocity
comparison to the live scene test. Native remainders are included before
rounding, avoiding a false mismatch from comparing its integer component to
SDL's complete float. Initial entry motion now passes this stronger check.
With `DOWNWELL_VERIFY_COMBAT=1` as well, the first remaining disagreement is
tick 478, actor 2: native Y=498508 versus SDL Y=498509 Q8 units; X and velocity
agree. This diagnostic is deliberately still failing for the extended tape,
not enabled as a supposedly passing full-precision default gate.

Source collision audit: enmFrog Step checks parentThinwall (87) before parentWall
(57), then rechecks support before integrating. sprFrogIdle's fixed rectangle
is x[-6,6), y[-5,8); sprThinPlatform's is x[-8,8), y[-8,-6). Existing native
and SDL point-query movement does not yet implement this sequence faithfully.
In particular, the native seven-pixel side probe differs from SDL's six-pixel
probe, and SDL currently resolves vertical motion before horizontal motion.
These are unresolved migration targets, not reasons to pick one port as oracle.

Default host tests and combat counters pass unchanged. Bounded 68000 replay
and A500 terrain-cache checks pass; normal target build and clean 68000 audits
are restored. Full frog physics consolidation remains unfinished.

## Frog landing remainder and leading edges

The tick-478 trace exposed an inconsistency in the previous correction:
native landing advanced preciseY by whole pixels, preserving a fractional
position, but cleared its fifth-Q8 remainder. SDL retained that fraction.
The provisional native landing now preserves both components. It must clear
the remainder only when a future source-mask resolver actually snaps Y.
This repairs internal arithmetic; it does not claim that the provisional
fractional landing position is the original runner's correct contact position.

The next strict divergence was tick 518: native's seven-pixel left probe
rebounded before the source frog rectangle touched the wall. Both adapters
now derive their side probe from the recovered fixed mask: -6 on the left,
+5 on the right (the right edge is exclusive). Fixtures check one-pixel
clearance in each direction and persistence of the full resting Y position.

The strict combat diagnostic now first fails at tick 521: native X=-16588
versus SDL X=-16589 Q8 units after rebound, with matching normalized velocity
and Y. Native still represents rebound speed with -(xsp*77)/256 instead of
the source -0.3 and lacks horizontal sub-Q8 accumulation. This is the next
numerical migration target; the strict diagnostic remains unresolved.
DOWNWELL_TRACE_FROG_MOTION=1 (with the strict diagnostic enabled) prints raw
landing/jump state from tick 474 onward for reproducing this audit.

## Shared frog horizontal state and rebound

Both adapters now own FrogHorizontalMotion in src/core rather than independent
float and Q8 rebound calculations. Its authoritative position and velocity use
Q16; SDL floats and native Q8 values are mirrors, never reintegrated inputs.
Alarm preparation/launch update this shared velocity directly. SDL state hashes
include the authoritative values and initialization flag.

The VM's `push.d -0.3; mul.d.v` replaces the native 77/256 factor. Rebound is
rounded to Q16, with horizontal integration retaining that fraction across
frames. Signed repeated-rebound fixtures stay within one Q16 unit of double
math; 100 steps following the first rebound stay within .001 pixel of 60 pixels.
These are explicit numeric approximation bounds, not bit-identical VM doubles.

This removed the tick-521 discrepancy. The next difference at tick 555 was
native xx integration on the landing frame; enmFrog Step integrates xx only
when still airborne after support resolution. Native now makes that decision
after resolving landing, and a targeted fixture guards it.

The strict combat comparison now reaches tick 727: matching X/velocity but
native Y=536320 versus SDL Y=536576 in relative Q8 units. The diagnostic also
compares the full shared Q16 horizontal state, not merely its rounded mirrors.
Vertical motion and full source-mask collision are still not consolidated;
current point probes are provisional and their ordering is not a complete
scrCheckCollisionWith implementation. The strict full-frog gate remains failing.

Validation: default host suite and combat counters pass; the bounded 68000
replay retains `9f0d3943e7900d2b`, and the A500 terrain-cache oracle passes.
Target instruction/helper audits are clean. Normal SDL/Amiga builds are
restored after verification; no runtime sprite generation or scrolling
architecture changes are involved.

## Shared frog vertical arithmetic; strict tape now passing

FrogVerticalMotion owns both ports' dry gravity, terminal speed, vertical
integration and provisional support decision. Position and velocity are in
fifths of Q8, representing .15 gravity exactly without float accumulation.
Native reconstructs this state losslessly from its whole/remainder fields;
SDL stores it authoritatively and only exports float mirrors. Both adapters
use the shared horizontal state's pixel query instead of re-quantizing their
different position mirrors before checking support.

The previous tick-727 landing discrepancy disappears. The strict entry plus
600-update combat test now passes and is enabled in make test. In addition
to the prior Q8 checks it compares full Q16 X/velocity and full fifth-Q8
Y/velocity/support state. Closed-form free-fall, cap, support-removal and
upward-launch fixtures guard the numerical state independently of SDL.

This deliberately retains the existing centre/foot point collision adapter;
it is not the original mask-based scrCheckCollisionWith implementation. The
core header labels that limitation. Thin platforms, full wall masks and
collision ordering, in-view movement gating, hit-stun and water handling
remain fidelity work. Matching two adapters that now share code is a useful
regression check, not independent proof of original-game behavior.

Validation: full host suite (now including the strict frog continuation),
bounded 68000 replay and A500 600-tick terrain-cache oracle pass. Both target
instruction/helper audits are clean; normal builds are restored. The short
cross-CPU checksum remains `9f0d3943e7900d2b` and is not a substitute for the
longer host-side actor-state comparison.

## Frog movement visibility

enmFrog Step's VM calls scrInView(0,0,32) immediately before integrating xx/yy.
The shared predicate now enforces strict camera edges with an extra 32 pixels
only at the bottom. Both adapters pass the previous rounded instance position
and current camera view, separately from Alarm 1's zero-margin preparation
test. Shared motion routines gate displacement while continuing gravity and
their existing collision responses; they do not freeze the whole frog.

Native no longer skips active frogs solely because they are below its legacy
generic enemy wake threshold. That skip suppressed source gravity accumulation
outside the movement view. A native fixture starts a frog below that threshold
and verifies unchanged X/Y plus terminal downward velocity after sixty steps.
Core fixtures cover the strict bottom extension, unchanged top/side edges,
offscreen gravity, re-entry movement and offscreen rebound.

The full strict frog/combat tape remains passing with unchanged final counters.
This does not complete source collision fidelity: masks/thin platforms, exact
collision-event ordering, time-stop, water and hit-stun remain open.

## Shared frog hit response and motion orchestration

The recovered enmFrog Step VM block 9 clears hit/preparation, resets image_index
and ysp, multiplies xsp by 0.8, and sets alarms 2/0/1 to 5/0/60. Alarm 2 clears
hitStun. Both adapters now use the shared FrogHitState and stepFrogMotion:
hit consumption/damping, vertical motion and conditional horizontal motion have
one implementation in src/core. Adapters still translate coordinates, mirror
state, and apply alarm/image/audio requests; complete runtime consolidation is
not yet done. This is not a second Amiga-specific enemy implementation.

Sprite 173's four damage frames, mirrors and hardware-sprite words are generated
offline. The damage pose takes priority over flight, retaining the previous
animation speed as in the source. A native bullet fixture verifies survival,
next-update response, preparation cancellation and five-tick expiry. The strict
600-update paired combat continuation now compares pending/ready/stun state
and requires an actual surviving frog hit with sprite 173 in both adapters.
Final counters are health 3/3, kills 2/2 and currency 16/16. Shared unit tests
also exercise the combined hit/movement ordering rather than just its pieces.

The queue models an existing enemy updated before a newly created bullet.
General source instance ordering (including an older bullet and newer enemy)
is still unresolved. The five-tick countdown does not yet implement arbitrary
Alarm 2 dispatch or time-stop semantics. Source full-mask collision, water,
impact effects and complete audio behavior are also not claimed. Q16 damping
is rounded fixed point, not exact GameMaker double arithmetic.

The complete host suite passes with 14,700 offline sprite oracle comparisons.
The bounded A500 allocation check reports 277,320 resident asset bytes and
16,792 Chip bytes remaining. This headroom is not an audio memory budget.
The bounded 68000 replay passes (`64 9f0d3943 e7900d2b`), as does the
600-tick target terrain-cache oracle. The narrow replay does not cover full
enemy state; the longer host paired tape provides the frog-state coverage.

## Shared frog alarm effects and landing latch

FrogBehavior now owns Alarm 1 preparation/retry and Alarm 0 jump decisions,
including velocity units, preparation state, stored launch direction, alarm
restart and sound resource IDs. Both runtimes apply those same effect records
to their storage; there is no separate Amiga policy for those decisions.
The landing-edge latch is also shared. Source evidence is the recovered
enmFrog Alarm 0/1 assembly, Create's jumpsp=4, and Step blocks 27–30.

The Amiga alarm adapter no longer clears grounded in Alarm 0: the original
alarm changes velocity, not support. Simultaneous frog alarms now dispatch
0 then 1, matching the shared EventScheduler, instead of the previous native
1 then 0. A native fixture covers simultaneous expiry; shared fixtures cover
both launch directions, failed preparation, airborne jump cancellation and
landing-edge rearming. Original runner-wide event ordering remains a separate
verification task; agreement with our scheduler is not proof of that behavior.

Adapters still own alarm countdown/dispatch and state storage, and Amiga audio
does not yet consume the shared sound requests. This extraction does not claim
complete runtime consolidation or full original-game fidelity.
The full host suite and bounded cross-target replay (`64 9f0d3943 e7900d2b`)
pass after this extraction; normal SDL and Amiga builds are restored.
