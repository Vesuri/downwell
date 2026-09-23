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
