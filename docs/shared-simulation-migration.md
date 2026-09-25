# One gameplay simulation, two platform backends

The destination is one portable, 68000-compatible gameplay implementation in
`src/core`, used unchanged by SDL and Amiga. The Windows VM/resources are the
behavioral authority. Agreement between two independent adaptations is not a
substitute for this architecture or original-game validation.

Current status: main-room player movement, gun/jump state, animation state and
knockback now use `PlayerMotion` in both runtimes. Currency motion/lifetime/
animation and default main-room camera rules are also shared, along with frog
and bat motion slices, crawler falling/latching/wall-following, wall-snail
attachment/movement/reversal, enemy creation stats and final pose/frame
selection, and the usual bullet's vertical segment arbitration and
fractional-camera expiry.
World storage/adapters, broader enemy behavior, complete bullet state/events,
reward orchestration, special camera modes and menus still need consolidation.
The fixed-point world now lives in core, but SDL's live `StartupRuntime` still
owns a separate world loop. Extracting the class does not complete that switch.
Rendering/storage optimizations do not justify separate gameplay rules.

## Shared live player phase

Both `StartupRuntime::updateMainPlayer` (the live SDL path) and
`GameplayWorld::step` now call `stepGameplayPlayer`. This owns the implemented
player-phase ordering: death/combo Begin Step, movement callbacks, movement,
shop observation/keeper alarms/interactions/presentation, clock and exit
alarms, health publication, death alarms and results opening. The duplicate
orchestration has been removed from both callers. Terrain/head-break adapters,
activation, shot emission and presentation publication remain with each owner.
The shared routine preserves native profiling phase boundaries and detaches
its stack-local callback context before returning.

Focused tests pin delayed death publication and first-death-movement gem
recording, same-update alarm advancement, combo healing before death is
published, completion-effect reset, callback detachment, copied state, control
locks and absence of spurious RNG draws. Existing world fixture digests remain
unchanged. This is live shared simulation code, not a shadow execution mode;
the overall SDL instance-store/world-loop replacement remains incomplete.

Source check: `gml_Object_objPlayer_n_Step_1.asm` calls `scrPlMovement` before
the `global.gameTime` update (block 18) and health/death publication (block
81). `gml_GlobalScript_scrPlMovement.asm` contains grounded `comboDone`
(block 123) and first-death `saveStats` before setting Alarm 0. The test pins
those implemented ordering boundaries, not full VM event fidelity.

Address/undefined-behavior sanitizers pass the focused phase test. The bounded
45-second A500 gameplay-cache run passes 600 updates, 101 fresh-composition
comparisons, seven shop/streak overlay comparisons and three consumable
controller updates. The 256-byte stack guard is intact, and the ordinary
executable is restored with clean 68000 audits. Evidence:
`/tmp/downwell-player-phase-native.log`,
`/tmp/downwell-player-phase-restore.log`.

## First-room generator state handoff

`StartupRuntime::mainBuilderState()` exposes the actual shared generator at
the room boundary. `AmigaAreaOneTerrainBuilder::resetFromRoomEntry()` imports
that state without replaying Create events or consuming random draws; callers
transfer the matching `randomState()` separately. Import is rejected after
terrain generation begins, without clearing the existing backend storage.
This is a pre-generation handoff, not a save-state loader for a populated room.

The live-scene test now exercises this path for seeds 2, 42 and 1234567,
comparing player/enemy state and all 16 RNG words plus cursor/draw count on
240 successive updates per seed, through entrance landing. It also verifies
that a late import fails without losing generated fragments. The independent
native seed-2 checkpoint test remains in place: importing SDL state must not
hide regressions in the existing native startup route.

This prepares a real state boundary for migration; it does not yet replace
SDL's live gameplay loop or the native intro's fixed initial checkpoint.
Focused evidence: `/tmp/downwell-entry-test.log`.
The bounded 45-second cross-target replay passes with unchanged digests
(`/tmp/downwell-entry-replay.log`). The ordinary Amiga executable is restored
and both 68000 audits pass (`/tmp/downwell-entry-restore.log`).

## Generated-instance world handoff

`GameplayWorld::createGeneratedInstance` now owns synchronous shop/exit
creation, breakable registration and enemy admission from builder commands.
It resolves source sprite metadata and shared birth profiles, translates room
coordinates once, and preserves integer movement, alarm and activation state.
The native application callback only maintains its terrain index, retained
tile store and dirty ranges. The host live-scene adapter uses the same core
handoff, replacing its copied admission rules.

The terrain callback remains between exit creation and actor admission, with
shops bypassing plain terrain registration, preserving the existing ordering
and RNG effects. Failure still propagates to the generator instead of silently
discarding births. Focused tests cover all six enemies, breakables, failed
terrain registration, missing metadata, full actor storage, shop/exit callback
order and all RNG words. Both extended reentry tapes pass through this entry
point (`/tmp/downwell-world-birth-focused.log`).

The bounded 45-second A500 gameplay-cache run passes 600 simulation ticks and
101 fresh-composition comparisons, with six generated fragments, four enemies
and 2,338 RNG draws at the last builder step. The 256-byte stack guard remains
intact. The ordinary executable is restored with clean 68000 audits. Logs:
`/tmp/downwell-world-birth-native.log`, `/tmp/downwell-world-birth-restore.log`.
The full host suite passes all eight combat/upgrade variants, both extended
reentries and 22,834 offline sprite comparisons. SDL rebuilds successfully
(`/tmp/downwell-world-birth-suite.log`, `/tmp/downwell-world-birth-sdl.log`).

This is the portable admission interface for the SDL world migration; SDL's
live instance store has not yet switched to `GameplayWorld` ownership.

## Remove the superseded SDL builder

The room-boundary audit found 322 unreachable lines after the unconditional
return following `AreaOneBuilder::stepVisible` and its state synchronization.
This was a complete older builder implementation, including separate enemy
birth rules and pattern/RNG selection. It has been removed, not migrated or
enabled. SDL continues using the same shared builder it used before this edit.

The core-boundary verification now also compiles `StartupRuntime.cpp` with
unreachable-code diagnostics treated as errors, so another abandoned
implementation cannot silently remain behind an unconditional return.
As a negative control, the same compiler command rejects the previous source
at the start of that obsolete block; it accepts the cleaned runtime.
This cleanup reduces the code to audit for the live-world switch; it does not
claim that `StartupRuntime` has started delegating its world Step to
`GameplayWorld`.
The full host suite passes all eight combat/upgrade variants, both extended
reentries and the renderer checks; SDL rebuilds successfully. Logs:
`/tmp/downwell-live-builder-suite.log`, `/tmp/downwell-live-builder-sdl.log`.
No native source or executable was changed by this cleanup.

## Portable fixed-point world extraction

`GameplayWorld.{h,cpp}` now owns the formerly backend-local world simulation.
`AmigaPortRuntime.h` is a compatibility alias, not a second implementation.
The native build and portable replay driver compile the core source directly;
the driver no longer imports any platform header. `Profile.h` and its
C-compatible counters define a hardware-free instrumentation interface. Timing,
vblank handling and measurement storage remain in `AmigaProfile.cpp`.

`verify-core-world.py` checks compiler-produced transitive dependencies for
both the world and the portable replay driver, in ordinary and freestanding
probe configurations. Dependencies must stay in core (apart from the driver
source itself). It is included in `make test`.

This extraction preserves storage, callbacks, update order and compatibility
ring-origin bookkeeping. It intentionally does not claim that the SDL live
world has switched to the extracted class. That migration must preserve its
room/instance interface and remove its replaced simulation branches, with the
existing exact adapter/reentry tests retained during the transition.

The moved implementation matches its predecessor byte-for-byte after the
intended type/include renames. The bounded 45-second cross-CPU replay passes
with unchanged digests (`/tmp/downwell-core-world-native.log`). A separate
45-second-bounded `SKIP_TO_GAME=1 PROFILE_LASER=1` probe run completes 600
gameplay updates and reports simulation/renderer phases and counters through
the extracted interface (`/tmp/downwell-core-world-probes.log`). Both runs
restore the ordinary executable with clean 68000 audits. This verifies the
instrumentation move, not achievement of the 20 ms frame budget.
The full host suite passes all eight combat/upgrade variants, both extended
reentries and 22,834 offline sprite comparisons. SDL rebuilds successfully
(`/tmp/downwell-core-world-suite.log`, `/tmp/downwell-core-world-sdl.log`).

## Shared projectile lifecycle

Both adapters now call `stepMachineGunProjectile` for the complete implemented
per-shot lifecycle: animation, first-Step initialization and collision queries,
motion, published position and view expiry. Their separate implementations of
that ordering are removed. World-specific collision lookup and compact/live-slot
storage remain adapters; the existing enemy-first scheduling is preserved,
not claimed to be a complete reconstruction of the VM event scheduler.

Focused tests cover both absolute and origin-relative coordinates, enemy-hit,
wall-hit and surviving shots, animation/RNG before queries, finishing movement
for consumed shots, and strict expiry after crossing the lower view margin.
They pass under address/undefined-behavior sanitizers. The bounded 45-second
68000 replay passes with unchanged digests; the ordinary build is restored and
both target audits are clean (`/tmp/downwell-projectile-lifecycle-native.log`).
The full host suite passes all eight combat/upgrade variants and both extended
level reentries, followed by 22,834 offline sprite comparisons. SDL rebuilds
successfully. Logs: `/tmp/downwell-projectile-lifecycle-suite.log` and
`/tmp/downwell-projectile-lifecycle-sdl.log`.

## Shared enemy creation profiles

`EnemyCreateProfile.h` is the single definition of health, reward and initial
animation rate for the six supported area-one enemy types. SDL Create handlers,
the procedural builder's deferred births and the native runtime all use it;
the terrain backend's existing enemy classification wrappers only forward to
the shared profile. Rates remain integer twentieths on the target, with an
explicit float adapter for the transitional SDL instance store.

`tools/verify-enemy-create.py`, included in `make test`, checks every profile
against the original Create-event VM assignments to `ehp`, `money`, `imgSp`
and `image_speed`. It pins the input hash and export tool version, verifies
all six object IDs, and uses decimal arithmetic for the recovered rates.
The profile adapter tests also check that unrelated instance state survives
and unsupported IDs remain excluded from native enemy classification.
This removes duplicated constants, not the remaining separate world ownership
or storage adapters. Random draws, alarm setup and movement initialization are
unchanged.

The source check and address/undefined-behavior sanitizer tests pass. A bounded
45-second native replay keeps all four checkpoint digests unchanged, including
weapon digest `0db388a5b1c2a40a` / coverage `3fff`. The normal executable is
restored and both 68000 audits pass (`/tmp/downwell-enemy-create-native.log`).
The full host suite, including all eight combat/upgrade tapes and both extended
reentries, and the SDL rebuild pass (`/tmp/downwell-enemy-create-suite.log`,
`/tmp/downwell-enemy-create-sdl.log`).

## Shared end-of-Step enemy presentation

`EnemyStepPresentation.h` now owns the final pose selection and Q8 frame
normalization for both runtime adapters. Their duplicated branches have been
removed. This includes snail stun, bat flying/hanging/hit poses and the frog's
hit/preparation/flight precedence, forced flight frame and next-Step rate.
It does not relocate world storage or claim the remaining world orchestration
is shared. Renderers continue to consume the selected sprite, not reselect it.

The recovered `enmFrog` and `enmBat` Step 0 assembly retains the pose IDs and
branch precedence used here. Focused tests cover 18 state combinations at five
phase boundaries, including the bat hang threshold, negative turtle walking,
stun overriding frog flight, and preservation of fractional animation state
except on forced frames. Address/undefined-behavior sanitizers pass.
The paired combat tape passes, and the bounded cross-CPU replay keeps all four
digests unchanged, including weapon digest `0db388a5b1c2a40a` / coverage `3fff`.
The ordinary Amiga build is restored with clean 68000 audits. Logs:
`/tmp/downwell-shared-pose-combat.log`,
`/tmp/downwell-shared-pose-native.log`.
The full host suite (including all eight combat/upgrade tapes) and the SDL
rebuild also pass: `/tmp/downwell-shared-pose-suite.log` and
`/tmp/downwell-shared-pose-sdl.log`.

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

## Shared currency motion and source homing

Both currency adapters now own the same authoritative GemMotion state from
src/core. Initialization, loose-gem gravity/damping, attraction, integration
and the provisional collection predicate have one implementation. Float/Q8
fields exposed to renderers are mirrors, not independent simulations.

The recovered objCurrency Create/Step VM and scrCurrencySpawn show that stomp
gems retain Create's gemsp=1 and rotsp=5, whereas proximity acquisition starts
at gemsp=4 and rotsp=32. Attracted Step turns toward global.plx/ply by at most
rotsp, then increases rotsp by .6 (cap 32) and gemsp by .125 (cap 9). The old
direct-to-player, age-derived speed in both adapters was not that behavior;
the native comment claiming an original unsigned-age underflow was incorrect.
Both adapters now target the pre-movement player snapshot. Acquisition still
completes the free-motion update before the next attracted update.

Create's heading candidates are retained rather than discarded. The assembly
evaluates choose's second candidate first, contrary to reading GML left to
right. Shared initialization preserves that evaluation order and draw count.
The spawn location uses rounded instance coordinates, as scrCurrencySpawn does.

Loose positions use 1/12800-pixel units, representing .035 gravity exactly.
The source ordered cap can overshoot for one update; zero horizontal speed
remains zero under sign(xsp)*.05. Shared fixed-point homing uses the existing
CORDIC direction calculation and interpolated sine table. Against independent
double-precision source equations, 2,240 homing updates have maximum velocity
error 1.02203 Q8 units (about .004 pixels/update). It is not bit-identical VM
double arithmetic. Tests cover Create/proximity differences, wraparound,
acceleration, gravity and acquisition ordering. Stomp collection now waits for
all gems, rather than assuming their separate trajectories arrive together.

The strict combat tape retains per-frame currency equality without increasing
tolerances, ending health 3/3, kills 2/2, currency 16/16. Full host tests pass.
Original fidelity remains incomplete: full collision masks/thin walls, water,
time-stop, upgrades, stuck-gem relocation, initial bounce-sound RNG and exact
runner irandom_range behavior for negative fractional bounce endpoints still
need recovery. The existing bounce approximation is explicitly identified in
the shared code. Sharing it removes divergent implementations, not that debt.
The bounded 68000 replay (`64 9f0d3943 e7900d2b`) and 600-tick target
terrain-cache check pass. Those target checks are regression evidence, not
complete gem-trajectory verification against the Windows executable.

## Shared currency lifetime

GemMotion now owns GemLifetime and advances it before any movement or target
acquisition. The adapters no longer increment age or independently decide when
to delete a gem; they retain an age mirror for presentation. The SDL mirror is
no longer 16-bit, since attracted gems can outlive 65,535 updates.

Recovered objCurrency Alarm 0 makes the gem obtainable and starts obtime=180;
Alarm 1 marks disappearing and starts distime=120; Alarm 2 destroys only if
not attracted, and does not rearm itself. Create's unobtime=10 and Alarm 3=15
give the default 10/15/190/310 boundaries. The old adapters deleted attracted
gems at age 310 and allowed loose gems one movement/acquisition update before
expiry. Both now consume the shared one-shot expiry result instead.

Core tests cover every boundary, attracted survival, non-repeating Alarm 2,
the former 16-bit wrap, and expiry before acquisition in the actual motion
update. This models the default schedule, not upgrades/time-stop rescheduling.
Creation-frame participation in the original runner's alarm/Step phases still
needs verification; durations alone do not prove that frame alignment. The
source disappearing flag is exposed but its Draw flashing is not yet wired.
Full host regressions and the bounded 68000 replay (`64 9f0d3943 e7900d2b`)
pass. The replay is a target regression, not a direct test of the 310-tick
gem lifetime; those boundary cases are exercised by the shared core tests.

## Shared currency animation

objCurrency Create's VM chooses image_speed from [-0.5,-0.3,0.3,0.5]. Both
ports previously consumed but discarded that random choice and displayed
(age/2)%4, making every gem animate forward at 0.5. GemAnimation now retains
the choice and advances a signed, wrapping four-frame phase in exact tenths.
The same Create draw remains before the two randomized heading candidates;
no extra RNG draws were introduced. Both SDL drawing and Amiga actor submission
read this shared phase. All four frames were already exported offline.

Tests exercise all four choices through 400 updates each, including reverse
wrap and one-draw initialization. The combat tape compares the multiset of live
gem ages, amounts, selected rates and full phases on every update, accounting
for differing slot reuse, and requires actual live-gem coverage. It passes
along with the full host suite and unchanged health/kill/currency counters.
This recovers the selected rate/direction, not proof of original-runner animation
phase on an instance's creation frame. Time-stop and disappearing Draw flashing
remain unimplemented; the latter is not silently approximated here.
The bounded 68000 replay (`64 9f0d3943 e7900d2b`) and 600-tick target
terrain-cache oracle pass with this frame-selection path. They do not replace
the outstanding original-Windows creation-phase and flashing checks.

## Target gem-simulation verification

The bounded cross-target gate now also executes a separate 400-update,
three-gem fixture on the actual 68000. It records full shared motion, homing,
lifetime and animation state on every update, plus RNG draw count/index and
final RNG words. Coverage requires free motion, attracted motion, proximity
acquisition, terrain contact, expiry and survival beyond expiry. Collection
is observed but not applied in this isolated fixture so long lifetimes are
exercised. It is deliberately not represented as a real gameplay replay.

Host and emulated A500 agree on gem digest `764abdd072bb4bbd` with coverage
`3f`, alongside the existing player digest `9f0d3943e7900d2b`. The host test
pins both digests; the bounded runner compares both and the coverage mask.
Verification-only code is excluded from normal Amiga builds. This closes the
previous lack of direct target execution evidence for gem arithmetic without
claiming the source collision/bounce/creation-order gaps are solved.

## Source currency collection masks

Currency collection now uses the serialized rectangular sprite masks rather
than the shared provisional +/-6 by +/-8 centre-distance test. Source sprite
133 is [-4,4) by [-4,4); sprite 134 is [-6,6) by [-6,6). The player's fixed
mask 31 is [-4,4) by [-4,8), making vertical collection asymmetric. The
objCurrency collision VM calls scrGainGem(getAmount), whose obtainable guard
is retained. Draw selects the appropriate gem sprite/mask by size; object 248
has no serialized explicit mask override. verify-enemy-contact.py now checks
all thirteen player/enemy/currency rectangles against the hash-pinned export.

Gem collection rounds directly from authoritative 1/12800-pixel units using
ties-to-even. The previous route through Q8 could turn a value just above a
half-pixel into an exact tie and shift the result by a pixel. Native gem x/y
mirrors also use direct rounding. Fixtures cover both gem sizes, edge touching,
the asymmetric vertical reach, unobtainable gems and signed half-pixel cases.

The 600-update paired combat gate remains passing with unchanged final counters
and no relaxed tolerance. The gem CPU fixture is explicitly schema 2, digest
`9543220ed0d8e596`, because its collection predicate changed; both small and
large masks are queried. This resolves gem/player rectangular overlap, not
terrain collision, creation-frame mask/event ordering, water or bounce RNG.
The expanded bounded 68000 replay matches this schema-2 digest and coverage
`3f`; the full host suite passes from freshly rebuilt binaries. During this
work, a stale host reference exposed missing header prerequisites in multi-file
test/tool builds. Those executables now depend explicitly on shared headers,
and a simulated GemVerification.h edit was checked to force recompilation.

## Shared default main-room camera

Both runtimes now use MainCameraMotion for the default 480-pixel rmMain
shaft/side-room policy and easing. It owns Q12 position, retained targets and
both slowdown values; renderer-facing float/Q8 values are adapters. The
recovered camMain Step VM uses strict 160/320 player boundaries, clamps shaft
target X to 240, and follows player Y+24. Side rooms set target X to 80/400
without replacing target Y. At exact boundary equality, the old X target is
retained. Returning from a side camera position uses slowdown one on both
axes before restoring shaft slowdown 10/5. These rules previously differed
between SDL and native, especially vertical side-room tracking.

The source death branch retains targets but still executes easing. Native
follow-camera death no longer falls into its synthetic replay scroll path
(one row every other tick). That synthetic path remains only for the explicit
non-following replay configuration. Hardware ring updates still consume the
resulting signed pixel delta; shared simulation does not manipulate DMA.

Focused tests cover side entry/return, exact boundaries, death-target retention,
top clamp and 1,000 updates against the source easing equation (<.002 pixel
error). Native side-entry checks use fractional player X, not rounded pixels.
The initial and 600-update combat comparisons pass without tolerance changes.
Startup digest schema 32 includes authoritative camera state; its menu replay
baseline is deliberately updated to a56e534c0975af1c. Q12 retains sufficient
precision while covering the source room's 300,000-pixel height in signed
32-bit arithmetic; a high-coordinate fixture guards that range.

This is the normal rmMain subset, not all camera modes. Free camera, lockers,
focus, shake, autoscroll, endings and room-end limits remain separate work.
Adapters still supply their current player snapshot and retain existing view
publication rounding; exact source global.plx/ply event timing and final
rounded camera-view publication still need auditing. Shared rules and CPU
arithmetic do not resolve those scheduling/presentation questions by themselves.
Validation: full host suite, bounded 68000 player/gem replay, and the A500
600-tick terrain-cache oracle pass with the final Q12 representation. The
existing replay does not hash camera state; camera rule tests and actual
follow-camera terrain execution provide separate evidence, not full Windows
camera equivalence.
