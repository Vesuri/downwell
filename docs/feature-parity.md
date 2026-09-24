# SDL / Amiga feature parity

Both ports must reproduce the original Windows game; SDL is not the oracle.
See [the source-backed audit](original-behavior-audit.md) for current fidelity
findings and combat results; the chronological notes below include superseded
checkpoints. Do not count a matching
port-runtime replay on two CPUs as proof that StartupRuntime and
AmigaPortRuntime implement identical behavior.

## Current evidence and remaining work

The playable executable now generates terrain and enemies incrementally from
the live RNG stream. The forecast/integration-foundation notes below describe
the migration history, not the current executable. Native/SDL actor-by-actor
comparison now uses the live path: the initial tape asserts matching creation
counts and active-actor presentation, plus bounded positional differences.
The extended combat tape now passes after shared player, bat and contact-mask
migrations. This finite tape is not evidence of completed gameplay parity.
Frame-rate figures from earlier forecast builds must also be remeasured.

| Slice | Current evidence | Remaining parity work |
| --- | --- | --- |
| Publisher, credits, controls, garden, well, style select | Native implementations and decoded scene comparison tests | Full-sequence display regression after gameplay changes |
| Player, camera, landing, jumping, gunboots | Shared player and default main-room camera; SDL/native comparison through entrance and scrolling | Special camera modes and original event timing; longer combat tapes; position checks still allow tolerances |
| Area-one geometry and endings | Shared live builder, emitted-object collision/rendering, terrain cache oracle | Live SDL replay comparison; side-room reversals and offscreen destruction coverage |
| Bullets and gems | Shared usual-bullet segment arbitration and fractional-camera expiry, currency motion/lifetime/animation and source collection masks; 400-update gem CPU fixture | Bullet masks, spread and collision-event scheduling; source bounce/terrain semantics and dense encounters |
| Actor presentation | Offline hardware sprites plus masked planar overflow; no actor discarded just because DMA slots are full | Overlap/draw-order comparison with SDL; broader crowded-scene timing |
| Enemies and rewards | Initial live tape plus 600-update combat continuation; matching health, kills, currency and player presentation | Gem trajectories, ordering and broader actor states remain unverified |
| HUD | Health, ammo, currency and streak implementations | State-driven image comparisons for damage, depletion and streak expiry |
| Death | Native control lock and death timer | End-to-end SDL/native death-state comparison |

Sound playback, complete shops/upgrades, later areas and a complete death/restart
UI must not be described as Amiga parity omissions merely because the Windows
game has them: establish which portions SDL actually implements first.

The original Windows game remains the ultimate oracle. A disagreement between
ports must be resolved against its VM/resource evidence or a Windows capture,
not by automatically changing Amiga to agree with SDL.

### Extended combat gate (passing; enabled in make test)

The usual vertical bullet's eight-pixel segment and enemy-before-wall
arbitration now live in `src/core/StandardBulletStep.h`, called by both
runtimes. `gml_GlobalScript_scrBulCheckSolid.asm` blocks 1–4 check the full
wall line and negate the full enemy-line query before entering wall handling.
The old SDL per-pixel loop could process a nearer wall before discovering an
enemy farther along that same segment. A focused test covers that case and
both segment endpoints.

This does not recover the entire bullet system. Both adapters still apply
enemy damage directly during the sweep rather than reproducing the original
collision-event phase. Their provisional enemy boxes, wall selection/storage,
impact-position handling remain to be consolidated and
validated; angled spread, other guns and piercing are outside this helper.

Validation after this extraction: full host suite (including the 600-update
combat comparison), bounded A500 600-tick terrain/cache oracle and cross-CPU
replay all pass. Replay remains `64 9f0d3943 e7900d2b` with the separate
400-update gem digest `9543220e d0d8e596` and coverage `3f`. Normal SDL and
Amiga builds were restored; both 68000 audits are clean. These fixtures do not
prove complete bullet-event fidelity to Windows.

Bullet expiry now also uses one shared Q12 rule. The bytecode for
`scrBulletOov` compares strictly above `viewY + viewHeight + 32` and below
`viewY - 100`; equality survives. Amiga previously used `>=` against the
rounded display offset and omitted the upper-screen check. Both runtimes now
use the authoritative fractional camera state, independent of DMA scrolling.
Tests cover equality, one fractional unit outside, and a subpixel camera
change across an integer bullet coordinate. End Step scheduling remains part
of the wider event-order consolidation, not proven by this helper.

Run the scene test with `DOWNWELL_TRACE_COMBAT=1` for a diagnostic continuation,
or `DOWNWELL_VERIFY_COMBAT=1` for a nonzero exit on disagreement. It adds 600
steps, alternates horizontal direction every 90 frames and holds fire for five
of each ten frames. Both targets must exercise kills and currency collection.
The default suite enables the strict continuation. It checks player sprite/frame
and health/kill/currency equality on every update. Frog motion additionally
compares authoritative Q16 horizontal and fifth-Q8 vertical state throughout
entry and combat; this check is enabled by DOWNWELL_VERIFY_FROG_MOTION=1 in
the default target. Its existing player movement guard
still permits up to three pixels of vertical difference; it is not an exact
whole-world comparison. After source-backed frog preparation corrections
and the shared hit response, current final counters are health 3/3, kills 2/2
and currency 16/16. The tape requires a surviving frog hit and compares its
pending hit, five-tick stun and damage sprite on both adapters.
See [shared simulation migration](shared-simulation-migration.md)
for the source-backed corrections and outstanding fidelity work.

Historical diagnostic before those migrations: first player discrepancy was
tick 343: native jumps (second jump), SDL
fires (second shot), with y=1979.6 versus 1984.2. First health/kill/currency
disagreement is tick 626 (native one kill, SDL zero). At the end health is 4/3,
kills 3/3 and currency 22/24. This is a floor-contact/action mismatch upstream
of combat, not evidence that reward amounts themselves should be changed.
The recovered objPlayer_n Begin Step calls scrGravity_n, scrPlMovement and
scrWallCol; the latter uses scrPlayerPlatformCollision/scrCheckCollisionWith
rounded/ceil swept probes. Neither current simplified player collision path
fully modeled those routines. The current shared player still has collision
approximations. Resolve remaining behavior from the Windows evidence
before choosing a port's outcome as correct.

## Crowded actor rendering

Simulation capacities are shared in `src/core/GameplayLimits.h`. The six regular
actor DMA buffers plus channel 7 remain a separate rendering limit. A multi-part
enemy either fits completely in the remaining channels or becomes one masked
planar BOB; it is never cut in half by channel exhaustion. All eight channels
are assigned in reverse SDL painter order (gems, bullets, player, enemies).
Hardware sprites still rely on playfield priority at horizontal edges.

Overflow BOBs use offline planar assets (including mirrored enemy variants),
the existing Bitmap/blitter path, and the inactive scrolling display buffer.
The terrain backing is never modified by actors. Touched physical ring rows
are marked for restoration on reuse; unchanged terrain remains cached. BOB
clipping is confined to the gameplay area and handles the ring boundary.

SCENE_VERIFY compares actual BOB blits against a scalar compositor. The
`amiga/verify_gameplay_bobs.gdb` script forces all non-player actors through
this path until at least 100 actor draws have been checked, including sparse scenes. Run it against a
`SKIP_TO_GAME=1 SCENE_VERIFY=1 BOB_VERIFY=1` clean build using the bounded startup verifier and
`READY_MARKER=DOWNWELL_GAMEPLAY_BOBS_READY`.

Initial normal gameplay timing after this change: 600 frames / 601 PAL fields
(49.92 FPS), one two-field frame at tick 528. This is a regression from the
earlier sparse-rendering 600/600 result, not a blanket 50 FPS claim. Preserve
the visible actors while fixing the remaining deadline miss.

Validation for this slice: full host suite passed (including 12,348 offline
hardware-sprite comparisons and mirrored planar/ring restoration checks);
the 600-tick cache run passed 146 fresh-terrain comparisons; forced BOB mode
passed 102 scalar-versus-blitter actor comparisons with no pixel mismatch.
The latter is a correctness test with expensive instrumentation, not an FPS
measurement. Its earlier 600-tick form exceeded the 45-second wall-clock limit;
the smaller test exits as soon as its draw coverage requirement is satisfied.

## Wall-snail orientation

The Amiga renderer now follows SDL's downward-crawling vertical flip. This is
also explicit in recovered `gml_Object_enmSnail_Step_0.asm` block 20:
`image_yscale = -sign(ysp)`. Sprite resource 177 remains unchanged; synthetic
frame `256+n` contains the offline vertical flip of source frame `n`, with
origin Y adjusted to preserve the anchor. Both horizontal facings have planar
and DMA variants. No target-side pixel flip or conversion is introduced.

Tests compare every flipped frame against a reflected original raster, test
the direction reversal, and compare all prepared DMA bodies with the pixel
oracle (13,132 comparisons including the new variants).

## Actor overlap order

SDL's `Game::renderMain` paints enemies, player, bullets and gems in that order.
The previous fixed channel-zero player overrode bullets/gems; allocating later
enemies to hardware after an earlier BOB also reversed painter order.
`AmigaActorChannels` now allocates the frontmost actors to the lowest DMA
channels. Once an entire actor cannot fit, it and every actor behind it use
BOBs in original back-to-front order. No channel is reused by a smaller actor
behind the first BOB. Player BOBs also use offline mirrored planar variants.
The channel planner is tested for 256 one-/two-part actor sequences at every
channel budget from zero to eight.

The target forced-BOB test passed 100 scalar-versus-blitter comparisons with
the player included. Startup now also has a 512 KiB Chip allocation check;
see `amiga-memory.md` for the reproduced shortage and offline deduplication fix.
Final normal-build timing for the ordered renderer: 600 frames / 603 PAL
fields (49.75 FPS), with a three-field frame at 528 and a two-field frame at
574. These misses remain open; the former 50 FPS result is not applicable to
this build. The playable SKIP_TO_GAME build is restored after profiling.

## Enemy lifecycle comparison

The scene parity test now checks two actual SDL enemy retirements against the
native runtime, in addition to strict cleanup-boundary unit tests. Native
awake enemies retire outside `viewTop-160 .. viewTop+3000`, matching
`gml_Script_scrOutofview` VM code. Retirement clears the activation-pending
state so an old enemy cannot wake again. Future prebuilt, not-yet-awake enemies
are deliberately excluded from this live-instance cleanup.

`DOWNWELL_COMPARE_ACTORS=1` on the scene test emits both actor populations.
This reveals an unresolved **creation-timing mismatch**, not merely rounding:
after the current tape, SDL's first turtle is at (228,2521), whereas the native
turtle is at (214,2139). SDL incrementally creates fragments when the builder
is within 128 pixels of the viewport bottom; native terrain is prebuilt and
enemy movement uses a different wake gate, while alarms already run. The
next simulation work must reproduce creation/alarm/RNG ordering and terrain
availability. Do not widen position tolerances to call this parity, or replace
SDL with the native runtime to conceal it. The static raster cache can remain
precomputed, but it must not dictate when simulation objects become live.

The shared `AreaOneBuilder::stepVisible` now owns the recovered strict
visibility gate and executes at most one builder event per tick. SDL uses this
entry point; the native live-builder migration remains outstanding. Integer
callers pass `ceil(viewBottom)` to preserve fractional boundary crossings
without target floating point. Tests cover invisible/end-of-level no-ops,
single-step behavior after camera jumps, full ordered Create-event payloads
and complete RNG state against direct generation. An interleaved RNG test
also confirms that gameplay random draws change later generated content:
the current full-level forecast cannot be treated as the live world.

`AmigaAreaOneTerrainBuilder` now provides the native incremental adapter with
caller-owned buffers and an externally supplied RNG. Its existing full-level
forecast caller uses the same adapter, preserving the pinned 4,770-draw
forecast. A new native test compares every emitted fragment, enemy Create
state (including grounded/active flags and alarms), decoration and full RNG
state against individual shared-builder events, both with and without
intervening random draws. Buffer overflow latches failure until an explicit
reset so a partially emitted event cannot be replayed accidentally.

This is not yet live-game integration: the playable runtime still forecasts
the level. Remaining integration must update collision availability and the
terrain raster cache as fragments arrive, and invoke generation between the
existing-enemy alarm phase and the post-scheduler bullet/enemy updates. Newly
created enemies must not have alarms aged on their creation tick. The source
reference's `stepInternal` runs the event scheduler, flushes deferred creates,
then runs bullets, enemies, interactions, currency and lifecycle in that order.

Verification of the adapter refactor: full host suite passed; the bounded A500
gameplay-cache run passed 600 ticks and 146 fresh-composition comparisons.
These checks preserve the existing forecast renderer, not yet SDL/live-world
parity. The ordinary `SKIP_TO_GAME=1` executable is restored after verification.

The native builder now exposes every actual Create event to a caller-owned
observer. `amigaAddTerrainObject` can incrementally add those events to a
collision index, updating only the affected upward landing links. This is
important beyond timing: pattern symbols `2` and `3` are probabilistic in
`AreaOneBuilder::emitPattern`, while the old full-forecast collision path
treats both as unconditional terrain. Live integration must use the emitted
objects for collision **and rendering**, not just a growing pattern prefix.
The native generation test checks every collision cell and next-landing link
after each event against an independent full-grid reconstruction from emitted
objects, including RNG-interleaved generation. Invisible events change nothing.

`AmigaPortRuntime::appendBreakables` preserves prior block activity and
destruction counts, supports the indexed path, and rejects oversized batches
before mutation. It avoids resetting all breakables whenever a section arrives.
These incremental APIs are integration foundations, not yet the playable
runtime's live terrain source. The old forecast path remains until its renderer
and event-order dependencies are migrated together.

Incremental-collision verification: full host suite passed, including every
cell/link after each generated section; cross-target replay retained
`64 95d0aaea 1be7256f`. This replay confirms no regression in its existing
64-tick scope, not full live-generation parity.

Native frame ordering now advances existing enemy alarms after player movement
but before the builder-generation hook and bullets. The hook receives the
actual enemy RNG stream and the fractional-camera viewport bottom rounded up
for the strict visibility gate. Creation can supply recovered grounded/awake
flags; new enemies move and animate that tick but their alarms first age on
the following tick. Generation failure is latched instead of retrying a partial
event. Regression tests assert this ordering and shared RNG consumption.

The host native-terrain test now connects the actual `AmigaPortRuntime` to the
incremental builder, emitted-object collision and appended breakables for 300
ticks. It lands on generated terrain and has only the generated enemy prefix,
not the prebuilt full-level population. This integrated simulation fixture is
not yet the playable executable: its cached raster renderer still needs to
consume actual emitted tiles before switching that executable to live terrain.

Frame-order validation: host suite and the integrated 300-tick fixture pass;
cross-target replay retains `64 95d0aaea 1be7256f`. The untraced A500 cache
check passed 600 ticks / 146 comparisons on the unchanged binary after an
earlier run and repeat timed out with a `B-Trap F346` at `0x23ff54` (inside
that build's asset range). A startup-only check passed with 26,584 Chip bytes
remaining; a per-tick debugger trace also completed with a null generation
callback throughout. The intermittent target failure is **unresolved**, not
a proven callback or allocation failure; later successful runs do not erase
it. Keep this separate from the already-fixed startup memory shortage.

The playable Amiga terrain compositor now consumes actual emitted terrain:
all walls participate in four-neighbor frame selection, while a bounded
row-linked tile list preserves block Create frames and thin-platform frame 23.
The same emission observer populates collision and breakable state. This
removes the unconditional rendering/collision of probabilistic `2`/`3` symbols,
fixed frame-zero generated walls and fixed frame-16 blocks. The circular
display rings and cached planar row transfers are unchanged. Host tests compare
tile row order and every stored frame with the emitted Create commands,
including an RNG-interleaved generation schedule and capacity/reset cases.

Generation in the executable is still a forecast, not yet live. Its raster
invalidation must also cover newly created walls' neighboring rows when the
live generator is enabled; otherwise changing wall connections can leave stale
cached images. This is now a timing/cache-invalidation migration rather than
a second independent terrain representation.

Actual emission exposed a formerly hidden storage shortage: the forecast
contains 146 breakables, while the earlier pattern-only collector/runtime
allowed 128; the RNG-interleaved test has 160. Native storage is now 512, with
16-bit lookup IDs and regression coverage through ID 300. This is Fast RAM,
not additional Chip residency. The emission path rejects overflow rather than
silently dropping blocks. The main routine's persistent fragment array also
moved to static storage after measuring only 376 stack bytes remaining in the
prelude Create callback on the default 4 KiB stack.

Emitted-terrain validation: full host suite passed; A500 cache verification
passed 600 ticks with 146 fresh-composition comparisons; cross-target replay
retained `64 95d0aaea 1be7256f`; the normal `SKIP_TO_GAME=1` build passed the
bounded startup memory check. The intermediate startup-check-5 failure was
the 128-block capacity, not Chip RAM exhaustion. Earlier intermittent target
trap evidence remains open rather than being silently reclassified.

## Live generation in the executable

`DownwellAmiga.cpp` now binds the native generation phase to the shared
visibility-gated builder rather than calling the full-level forecast. Create
events append collision, breakables and row-indexed tile graphics and configure
new enemies with their actual initial flags. The builder receives the runtime's
RNG after existing alarms, before bullets and enemy motion. Decorations and
fragment counts used by the compositor come directly from the live builder.

New background objects dirty their affected raster rows (including the adjacent
wall rows), invalidate staged side views and trim stale hidden retained rows.
Any visible intersection is refreshed; unchanged rows still use the private
backing and double-buffered circular display rings. Only the initial room is
precomposed at entry, not future generated sections.

The bounded A500 run reached 600 ticks with 5 generated fragments, 3 created
enemies and 2,350 RNG draws at the last builder phase, with no generation
failure. A deterministic strip sweep over all three camera columns supplements
the encounter-dependent cache checks: 101 cache/fresh-composition comparisons
passed with zero mismatches. The minimum coverage gate remains in place;
it was not reduced to accommodate the shorter travel in this run.

The full host suite and cross-target replay also pass. The normal live
`SKIP_TO_GAME=1` executable passed the bounded startup check with resident
assets 267,528 bytes and 26,584 Chip bytes remaining; it is the restored build.
This does not yet prove actor-by-actor SDL parity or the new path's 20 ms budget.

### Live scene-comparison fixture

The SDL/native scene input tape now binds the native incremental generator,
shared gameplay RNG and exact emitted collision objects, matching the executable
instead of installing the complete forecast before simulation. The separate
forecast/spatial-index oracle remains covered. The tape checks generation
success, an incomplete generated terrain prefix, exact enemy population/order,
player movement and gunboot/landing behavior, and out-of-view retirement.

`DOWNWELL_TRACE_ACTORS=1` reports the first sampled position difference larger
than one pixel; it is a diagnostic, not an actor-parity assertion. Currently it
reports tick 171: the first snake is at (221.5, 1298.99), moving left, versus
SDL (223, 1298.2), moving right. Source snake Step uses `place_meeting` with its
mask and rounds final positions; the two hand-written point-probe adaptations
still need reconciliation against that source. Passing this fixture does not
prove all actor motion or animation matches.

The reported executable exit was not reproduced by a fresh bounded check:
the normal `SKIP_TO_GAME=1` build passed allocation with 26,584 Chip bytes
remaining and reached 64 frame-loop iterations without a reported failure.
This is limited startup evidence, not a claim that the reported exit is fixed.
Startup and internal loop failures now return AmigaDOS status 20, while an
explicit normal quit returns 0; no error text is printed.

### Enemy coordinate rounding

The snake, turtle, frog, bat, snail and crawler Step assembly all call `round`
when copying their precise positions into instance x/y. Native code previously
truncated signed Q8 values toward zero; SDL used `std::round` (halves away
from zero). Both now use nearest, ties-to-even, consistent with the
[GameMaker round reference](https://manual.gamemaker.io/lts/en/GameMaker_Language/GML_Reference/Maths_And_Numbers/Number_Functions/round.htm).
The target implementation uses only integer arithmetic. Its relative origins
(240,176) are even, preserving ties-to-even parity in world coordinates.

Tests cover positive/negative half ties, every Q8 value in [-1024,1024]
against the portable floating-point oracle, both world-origin translations,
and actual enemy Steps with positive/negative fractional displacement.
This corrects instance-coordinate quantization, not the incomplete collision
adaptation. The live trace still differs: first sampled >1-pixel mismatch is
now tick 172, native (222,1297.99) moving left versus SDL (223.5,1298.2)
moving right. Full actor-motion parity remains open.

### Fractional solid probes and first-snake regression

SDL's point-probe adaptation compared fractional coordinates directly with
inclusive integer tile bounds. Between adjacent tile centres 0 and 16, x=7.5
incorrectly missed both tiles. The query now floors to a pixel before testing
the recovered 16-pixel extent. Native snake/turtle probes likewise floor their
precise Q8 positions rather than truncating signed relative coordinates or
using rounded display positions. Vertical collision uses the X after horizontal
movement, retains the fractional Y when advancing to contact, and records
landing immediately, matching the reference adaptation's ordering.

The first snake now agrees within one pixel throughout the sampled live tape,
including retirement; these X/Y comparisons are assertions, not optional logs.
The Q8 floor helper is exhaustively compared with `std::floor` across
[-1024,1024]. The first optional diagnostic difference moves to the frog at
tick 242 (native y=1860.16 versus SDL y=1859.15). Other enemies and full
mask-based `place_meeting` semantics remain unfinished; this is not a claim
that point probes fully reproduce the original GameMaker collision system.

Validation: full host suite passes. The bounded 68000 cache run reaches 600
simulation ticks with five generated fragments and three enemies, 101
fresh-composition comparisons and zero reported cache/buffer mismatches.
No overflow BOB comparisons occurred in that run; it does not cover that path.

### Frog support and generation event order

The native frog now rechecks support each Step instead of remembering a
grounded flag indefinitely when a destructible block disappears. A regression
removes its support before its jump alarm expires and requires immediate
falling. Its point probes also use precise floored coordinates and retain
fractional landing positions consistently with the reference adaptation.

The optional `DOWNWELL_TRACE_BIRTHS=1` trace identified a separate timing issue:
native generation used the new camera position, whereas objBuilder runs
before camMain in the reference's ordinary Step instance order (both are
Step_0 events, not End Step). Generation now uses the previous camera value.
The first four enemy births match SDL at ticks 134,166,237,265; this prefix is
asserted at each sampled comparison, alongside a direct first-camera test.
Later births still differ (native tick 340 versus SDL 339), accompanying the
existing player/camera drift. The first >1-pixel actor diagnostic is now the
frog at tick 272: y=1954.86 versus 1955.9, with native gravity 38/256 rather
than exactly 0.15. Neither the later birth timing nor complete frog collision
semantics is claimed solved.

### Frog gravity remainder and broader position assertions

Frog gravity now retains fifths of a Q8 velocity unit: 0.15 pixels/frame² is
38 + 2/5 in Q8, not a constant 38. A one-byte remainder is cleared on
configuration, jumping, landing and terminal-speed clamping. The implementation
uses integer arithmetic only. A 40-frame ballistic test checks velocity and
position against exact 0.15 integration; separate tests check landing and
terminal-speed resets.

Every paired actor in the live SDL/native tape now has mandatory X/Y assertions
within one pixel at each sampled comparison. There are no >1-pixel diagnostic
differences on this tape. It still does not assert all animation frames or
complete lifecycle state, and a temporarily missing newborn is not position
compared. The last two births remain one frame late (340 versus 339), so full
actor parity is not proven. Host tests and the bounded 600-tick Amiga terrain
cache verification pass with the integer remainder enabled.

### Shared enemy presentation resources

SDL previously wrapped every enemy at two animation frames. Shared
`EnemyPresentation.h` now provides the recovered sprite choices and actual
1/2/4/6-frame cycles to the SDL simulation/drawing and native simulation.
Sprite-pack tests compare all eleven cycle lengths with serialized metadata.
SDL now also selects the frog's rising frame 0 / falling frame 1, stops its
airborne animation, and restores grounded animation speed. The live tape
asserts the airborne pose. No native cycle was shortened to match SDL's bug.

Animation phase parity is still incomplete: native Q8 animation speeds and
ground-contact timing can produce different frames, even where positions
agree. This change fixes resource selection and cycle truncation, not all
animation scheduling or hit-stun presentation.

### Native animation-rate precision

Native animation speeds now use exact twentieths of a frame, retaining a
fifth-Q8-unit remainder alongside the Q8 image phase. This represents 0.1,
0.15, 0.2 and 0.25 without a permanently rounded increment. Explicit airborne
frog poses clear the remainder when setting their fixed frame. Tests check
2,000 updates at each supported rate and require the active bat to reach
frame 3 on its fifteenth update (the former Q8 approximation only reached 2).
This is still integer-only and does not change the hardware sprite asset path.
Host tests and the bounded 600-tick target cache check pass (101 composition
checks, zero reported mismatches). Final live-tape frog/turtle frame indices
still differ, so exact rates alone do not prove animation-state parity.

### Frog landing-pose parity

`DOWNWELL_TRACE_ANIMATION=1` reports the first active sprite/frame mismatch per
enemy type, alongside grounded flags and precise Y. It exposed SDL's frog
losing support at tick 299: native stayed grounded at y=2127.08, while SDL
briefly fell again at y=2127.3. The reference adaptation now rechecks its
existing support at yy+1 instead of requiring the next fractional fall to
reach a solid pixel again, consistent with the recovered frog Step support
check. This also removes repeated airborne-pose resets after landing.

On the current live tape every paired active frog now has exact assertions
for grounded state, sprite ID and integer animation frame, with required
coverage of both airborne and landed states. The earlier snake mismatch at
tick 138 remains (native airborne, SDL grounded); this does not establish
all-enemy animation parity or frog hit-stun/long-jump coverage.

### Snake/turtle gravity integration

Native snake/turtle gravity now retains the 1/5 Q8-unit velocity remainder
of 0.2, and a separate remainder when integrating velocity into position.
Keeping only velocity precision still leaves the fifth update just short of
an integer pixel boundary, delaying landing. A 30-frame exact-rational test
checks both speed and position; the first live-tape snake now also has exact
grounded/sprite/frame assertions while active. Its former tick-138 mismatch
is gone. The optional animation trace next reports a later snake at tick 343,
after the separately documented one-frame-late birth. Full parity remains open.

### Same-frame player edge fall closes the sampled birth gap

Native player gravity was skipped whenever the frame began grounded. If
horizontal movement then left a platform, that first airborne frame moved
down by zero, unlike SDL/recovered Step ordering. Gravity now precedes support
loss; a player that remains supported clears vertical speed at the end of
the update. A focused test requires positive downward displacement on the
very first frame without support.

With this correction the entire current live tape has matching enemy counts
at every sampled frame, including the last pair born at tick 339. All paired
active actors now have exact grounded/sprite/integer-animation-frame assertions,
in addition to the one-pixel position checks. This supersedes the earlier
reported birth/frame differences on this tape, not the broader outstanding
requirements (additional tapes, combat states, shops, later scenes and visual
coverage). Full feature parity is still unproven.
The full host suite, canonical cross-target replay (`64 95d0aaea 1be7256f`)
and bounded 600-tick Amiga terrain cache check pass after this ordering change.
