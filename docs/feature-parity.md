# SDL / Amiga feature parity

Both ports must reproduce the original Windows game; SDL is not the oracle.
See [the source-backed audit](original-behavior-audit.md) for current fidelity
findings and combat results; the chronological notes below include superseded
checkpoints. Do not count a matching
port-runtime replay on two CPUs as proof that StartupRuntime and
AmigaPortRuntime implement identical behavior.

## Current evidence and remaining work

### Shared Begin Step activation phase

`MainRoomActivation.h` now owns frontier advancement, traversal, eligibility
and activation counting for both runtime stores. Platform/storage adapters only
provide world coordinates and apply the active/pending flag changes. The native
adapter converts its local origin; the host adapter rounds its Q8 conversion
upward so a fractional centre below the frontier is not activated by truncation.

The source assembly's strict view-bottom comparison and single 16-pixel advance
are preserved. Tests cover threshold equality, fractional-centre exclusion,
storage-order callbacks, active/retired/non-enemy exclusions, repeated updates,
empty stores and full serialized room coordinates. This removes one duplicated
simulation phase, not the two runtime orchestrators as a whole. Region inclusion
still uses the existing enemy-centre approximation; full GameMaker region/mask
semantics remain unproven and must not be inferred from these tests.

Validation: full host suite (including both combat continuations and the new
activation test), normal SDL build, bounded A500 600-tick terrain-cache check,
and cross-target replay pass. Replay retains
`64 9f0d3943 e7900d2b 400 8d8e8642 25e003ee 7f`; normal target build is restored
and 68000 helper/instruction audits remain clean.

### Published view used by gameplay queries

Normal gameplay now uses camMain's rounded published view for bullet expiry,
out-of-view retirement, crawler/walker/frog visibility, frog alarm targeting,
instance activation and live builder thresholds on both platforms. These
scripts query `__view_get`, not camMain's private xx/yy easing accumulators.
The shared camera exposes Q12 view edges, while its smoothing state remains
fractional. Builder evaluation still observes the previous camera within the
current port phase ordering; this does not claim to recover the entire runner
event schedule. The synthetic non-following replay camera remains explicit.

Tests cover all four edges across positive/negative half ties and bullet expiry
at the rounded top/bottom equality boundaries. Earlier notes describing the
fractional camera accumulator as authoritative for these view queries are
superseded: Q12 is the helper representation, not permission to bypass the
published source view. Arbitrary fractional helper fixtures remain useful for
testing strict comparisons independently of the normal camera producer.

Verification: full host suite and explicit camera boundary tests pass. Fresh
SDL/native landing and scrolling captures each match all 99,968 pixels without
changing either baseline. The bounded A500 600-tick terrain-cache check and
cross-target replay pass; the latter retains digest
`64 9f0d3943 e7900d2b 400 8d8e8642 25e003ee 7f`. Normal builds are restored,
with clean 68000 instruction/helper audits. These checks do not establish full
original-runner event ordering or whole-game fidelity.

### Source-rounded camera/player and scrolling parity

The scrolling mismatch described below is resolved for the tick-383 fixture:
fresh native and SDL captures now have **zero differing pixels out of 99,968**.
The active baseline is `df857829d2cfa20f8d0780e65164a602f489b6aff57e832b3dfc18340e94fc8a`.
`verify-amiga-gameplay-active-frame` reads the native capture's simulation tick
and replays SDL to that update before a strict full-window RGB comparison.
It neither translates images nor excludes actors/HUD.

Corrections were derived from original evidence, not just image alignment:

- Serialized rmMain view 0 follows object 63 (camMain). Its Step assembly
  publishes `round(xxFinal)` / `round(yyFinal)`. Shared Q12 nearest-even pixel
  accessors now feed both rendered views; fractional easing state is retained.
  The previous native truncation and SDL fractional rendered view disagreed
  with that published position. The corrected tick-383 view is (160,1315).
- Player Begin Step publishes rounded x/y, and `playerDraw` uses those instance
  coordinates. SDL previously drew precise player state; native sprites used
  truncated offsets. Both now draw the rounded published position.
- Native ammo-label clearing erased part of the gauge's lower rim. Ammo-change
  redraws now restore its base/interior/outline order. This remains dirty-state
  rendering, not a new per-frame HUD repaint.
- `scrGemmyText` assembly emits red left/up, black right/down, then white.
  Both previous text paths were incomplete. A shared five-pass layout now
  drives SDL glyph draws and Amiga prepacked glyph blits; an order/color test
  covers the passes.

Camera tests include positive/negative Q12 half ties and preserving precise
state. Visibility/lifetime publication was subsequently corrected as described
above; complete event-phase scheduling remains open. Streak positioning/animation
and full original HUD effects
also remain beyond this five-pass text correction.
The full host suite, fresh landing and scrolling pixel gates, bounded A500
600-tick terrain check (101 compositions, intact stack margin), and canonical
cross-CPU replay all pass. Replay digest remains
`64 9f0d3943 e7900d2b 400 8d8e8642 25e003ee 7f`; this fixture does not by itself
cover the changed scrolling camera. The image/ring gates provide that coverage.
Normal SDL/Amiga builds are restored and the 68000 audits remain clean.

### Scrolling capture and SDL rasterization audit

Historical diagnosis (superseded by the source-rounded correction above):

The native active-gameplay capture now publishes its simulation tick, Q8 player
position and displayed integer camera origin. The current capture is tick 383,
player Q8 `(68056,371404)`, view `(160,1314)`. The old active-frame checksum
still fails and has **not** been refreshed; current bytes hash to
`903fa3be5bd02f6942b80bab4f94ef722c08d14f28ce15560f3fbab2ae103907`.
Terrain/cache checks pass, but do not establish visual parity.

`capture-sdl-landing OUTPUT_DIRECTORY 383` now reproduces the native input
sequence beyond landing and captures that exact gameplay update (the room
creation frame is excluded from simulation tick numbering). The SDL player
position matches, while its fractional view Y is 1314.65. This exposed a host
renderer defect: SDL logical-size scaling rasterized fractional geometry at
window resolution, producing nonuniform 2x2 logical pixels. Original
`scrInitialize.asm` calls `surface_resize(application_surface,160,cameraHeight)`
and controller Draw GUI scales the resulting surface; it does not rasterize
the game afresh at window resolution.

SDL now draws to a fixed 380x284 composition texture (containing the clipped
160x284 game view), then nearest-scales it for presentation. Screenshot readback
uses that target's dimensions, so PNGs are logical 380x284 frames. The capture
tool presents and redraws before reading back, exercising target restoration.
The first-landing image remains an exact 99,968-pixel native match.

The active capture still has **2,571 differing pixels** after this correction.
A diagnostic one-row offset reduces interior-world differences from 2,521 to
211; 43 HUD pixels also differ. That offset is diagnostic only: the strict
comparator still fails and applies no alignment or masking. Resolve camera
raster rounding and the remaining actor/HUD differences against original
rendering semantics before changing the active baseline. The regular Amiga
build was restored after the failed baseline check.

### Full-path presentation recheck (2026-09-24)

After the shared combat/depleted-health changes, `verify-amiga-scenes` passes
on the bounded A500 configuration (512 KiB Chip, 8 MiB Fast, 4 KiB task stack).
It traverses startup, garden, well entry, style selection, landing and scrolling
gameplay with an activated enemy. Splash publication was checked for all 180
publisher, 180 credits, 240 controls, 60 fade-out and 60 fade-in frames; the
garden copper patch oracle reported no failures. This closes the outstanding
full-path regression check for these changes, not Windows visual equivalence.

`verify-amiga-controls-frame` also passes the static DMA-window assertions and
the recorded planar-frame checksum. Its decoded 160x284 image was inspected:
the leftmost player illustration is intact. This is a captured bitmap plus
copper-configuration check, not a new recording of the monitor output.

The first-grounded-gameplay capture reached its breakpoint and passed its
terrain/backing-buffer assertions, but initially failed its old frame checksum.
Expected `89d8eaead3edbf99b5738020aed05948218659069e6c5682b2476936a1197ba8`;
captured `be204c3e9423423263047d6b745aeb19853dcf160e83770476d557a9a08f18a7`.
The old checksum was introduced in `ef26d11`, before live emitted terrain
rendering (`f6daa65`) and subsequent actor behavior corrections. The retained
old PNG and current decoded image visibly differ in terrain edge selection
and enemy presentation. A subsequent fresh SDL capture resolves this baseline
mismatch: its complete displayed region matches the current native capture
with **zero differing pixels out of 99,968**. The captured SDL landing is
player (240,1200), view (160,1058), sprite/frame (31,0), main update 165.
Both current renderers select area-one wall art from the four cardinal wall
neighbours, as in `objWall_n_Step_2`; the older native image predates the move
from forecast patterns to emitted live terrain. The first-landing checksum is
now refreshed to the current value above, backed by that exact comparison.

`verify-amiga-gameplay-frame` now also builds `capture-sdl-landing`, replays the
full startup/locked-style visit/usual-style selection with a 6000-update bound,
and renders the first landing through the actual SDL backend. Comparison
requires uniform integer scaling, crops only logical x=16..367 to match native
DMA, and rejects every differing RGB pixel. It includes terrain, actors and
HUD, rather than silently masking dynamic regions. The target uses a unique
ignored capture directory and restores the normal Amiga build on success.
The comparator has dimension, crop, single-pixel mismatch and nonuniform-scale
tests. The subsequent active-scrolling audit is recorded above.
Exact port-to-port agreement at this landing does
not establish unimplemented source pillar/diagonal rules or general Windows
presentation fidelity.

Scope inventory: `PlatformSDL` exposes rendering, input, frame timing and PNG
capture, but no sound playback operation; startup records sound requests.
`Game::renderMain` draws terrain, actors, player, bullets and currency, and
`AreaOneBuilder` has shop-related geometry. These are not an implemented audio
backend or complete shop/purchase/upgrade UI. Neither those original-game
features nor the original death/restart flow can be declared complete merely
by matching the currently implemented SDL subset. They remain in the broader
original-game fidelity scope.

### Depleted-health control/contact gate

Both runtimes use `PlayerLifeRules.h` to suppress movement/fire input once
health is depleted. SDL previously continued accepting those controls while
Amiga suppressed them locally. The shared contact response now also rejects
stomps at depleted health, preventing post-death bounce, kills and rewards in
both ports. Quit/pause requests are not discarded by this gameplay filter.
Core tests cover living/dead input and a fatal hit followed by a potential
stomp; the native runtime retains its existing death-control/camera test.

Original evidence: `scrEnemyStomped.asm` starts with a `global.death` guard;
`scrPlMovement.asm` guards live control with the same state.
`objPlayer_n_Step_1.asm` sets death and clamps health after its movement calls.
The current port still equates depleted health with death immediately, rather
than reproducing that full phase sequence. This is a shared implementation
of the existing death boundary, not a claim that death animation, momentum,
alarm/menu timing or the original same-frame collision behavior are complete.
The full host suite, bounded 600-tick A500 terrain-cache check and canonical
cross-CPU replay pass. The latter two are regression checks, not death-scene
coverage; full death presentation and restart parity remain unproven.

### Creation-ordered bullet storage

Native bullet storage now stably compacts surviving shots before appending a
new shot, using the same core slot-compaction policy as currency storage.
Previously its first-free-slot allocation could place a newborn ahead of an
older survivor, unlike the host's dense creation-ordered traversal. A focused
test retires the oldest of two shots and checks that the third is appended
after the survivor. Both combat tapes additionally compare every live bullet
in creation order (the existing one-pixel X / three-pixel Y tolerances remain).
This removes a storage-induced ordering divergence; it does not establish
the original runner's complete collision-event ordering or allocation-failure
behavior. The fixed sixteen-shot capacity is unchanged.
The full host suite, 600-tick A500 terrain-cache check and canonical cross-CPU
replay pass with this ordering fix. The replay digest is unchanged; the new
hole-reuse regression and ordered combat comparisons provide the direct
coverage for this change.

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
| Bullets and gems | Shared usual-bullet segment arbitration, recovered enemy rectangles and published-view expiry, currency motion/lifetime/animation and source collection masks; 400-update gem CPU fixture | Bullet collision-event scheduling, spread and wall selection; source bounce/terrain semantics and dense encounters |
| Actor presentation | Offline hardware sprites plus masked planar overflow; no actor discarded just because DMA slots are full | Overlap/draw-order comparison with SDL; broader crowded-scene timing |
| Enemies and rewards | Initial live tape plus 600-update combat continuation; matching health, kills, currency, ordered gem motion and player presentation | Longer routes, dense reward bursts and broader actor states remain unverified |
| HUD | Health, ammo, currency and streak implementations; exact landing/full-ammo and scrolling/empty-ammo images including currency/streak 2 | More damage, refill and streak-expiry image coverage; complete source HUD effects |
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

Both adapters now call `src/core/StandardBulletImpact.h` for the implemented
usual gun's enemy response: ten damage, pending frog/snail/bat hit state,
fatal reward/retirement, and immune turtle ricochet. The immune branch of
`gml_GlobalScript_scrTypicalBulletDamage.asm` explicitly calls `choose(30,31)`
before `soundPlay`; both ports previously omitted that RNG consumption. The
shared handler now consumes the choice even without an audio backend. Tests
cover both sound choices, nonfatal/fatal hits, duplicate-hit rejection and an
airborne native shot absorbed by a turtle with exactly two WELL draws.
Rendering activation no longer gates the native bullet collision query.

This does not recover the entire bullet system. Both adapters still invoke
the shared damage response during the sweep rather than reproducing the original
collision-event phase. Their wall selection/storage,
impact-position handling remain to be consolidated and
validated; angled spread, other guns and piercing are outside this helper.
Pending hits are still delivered on the next Step and fatal rewards are still
immediate. Original hitAngle/hitDmg, damageStun, momentDelay and impact effects
remain incomplete; matching this handler is not proof of full event-order or
RNG fidelity.

After this consolidation the full host suite (including both combat tapes and
16,562 offline sprite comparisons), bounded A500 600-tick cache check (101
fresh compositions and intact stack margin), and cross-CPU replay pass.
The replay remains `64 9f0d3943 e7900d2b 400 8d8e8642 25e003ee 7f`; its
player/currency fixture does not exercise enemy impacts. Impact coverage is
provided by the host unit/integration fixtures and paired combat tapes, not
by that unchanged replay digest. Both target instruction audits remain clean.

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

Bullet enemy-line queries now use the same recovered rectangular masks and
unit flips as player/enemy contact, instead of a universal 16×16 box. Resource
IDs and bounds are checked by `verify-enemy-contact.py` against the pinned
export. Tests cover snake margins, its asymmetric flipped mask, exact line
endpoints and empty masks. Queries use each actor's published integer x/y,
not its next movement position. This improves the line query; it still does
not reproduce the separate original bullet/enemy collision-event dispatch.
The combat tape's firing cycle is offset by one update to retain the existing
mandatory surviving-frog-hit coverage with these smaller source masks; no
state tolerances or coverage assertions were relaxed.
The test retains `DOWNWELL_COMBAT_FIRE_PHASE` for reproducing alternate tapes:
phases 1–4 pass this strict comparison, while 5–9 currently fail the live
currency animation/state comparison. Phase 0 no longer covers a surviving frog
hit. These are explicit remaining coverage/fidelity issues, not evidence of
completed combat parity; investigate event/reward ordering and RNG consumption
before attributing them to rendering.
For phase 5 the first reported gem difference is tick 890: two native versus
one SDL live gems, currency 2 versus 4, with one kill on each. Thus this failure
includes collection/lifetime state, not merely animation. The default phase-1
tape ends health 3/3, kills 3/3, currency 38/38. Full host tests, the bounded
600-tick A500 cache check and the unchanged cross-CPU replay pass; normal
builds were restored and the 68000 audits are clean.

#### Phase-5 divergence traced to crawler motion

Run the strict scene test with `DOWNWELL_COMBAT_FIRE_PHASE=5` and
`DOWNWELL_TRACE_GEM_MOTION=1` to print player Q8 state, published/precise enemy
positions, and authoritative gem positions/velocities (1/12800 pixels).
Native enemy coordinates and SDL trace coordinates are both relative to
(240,176); actor storage indices intentionally differ.

The first crawler (object 119) is created at tick 618. At tick 619 its native
Q8 Y is 622745 versus SDL 622746: native gravity adds 51 Q8 units, while the
SDL path adds 0.2 pixels. At tick 621 native remains airborne at Y=623102,
while SDL has landed at Y=622899. The native alarm path subsequently snaps
precise coordinates to integer x/y; SDL retains fractional coordinates.
By tick 861 their published crawler Y values differ by two pixels. Its death
at tick 862 emits three gems with identical initial velocities but native
positions exactly 25600 gem units (two pixels) lower. Attraction consequently
starts on different updates, producing the tick-890 collection discrepancy.

This is upstream of shared gem motion, not an animation/rendering fault.
Do not compensate reward positions or change collection radii. Both current
crawler paths also simplify the recovered behavior: Step uses
`scrCheckCollisionWith(57)` before latching, and Alarm 0 follows walls using
four directions, rectangle `place_meeting` tests and visibility checks.
The next implementation must consolidate those source rules rather than copy
either existing approximation. This trace establishes the reproduction and
causal chain; it does not fix crawler fidelity.

#### Shared crawler movement replaces the divergent adapters

`CrawlerMotion` now owns fifth-Q8 position and velocity, latching and four-way
wall following for both runtimes. Step uses the shared recovered solid solver,
exact .2 gravity without the old seven-pixel cap, and the 64-pixel lower view
margin. Alarm 0 handles inner/outer corners in either traversal direction,
loses its latch when all diagonal contact probes are empty, and integrates
only within the source's strict view bounds. The alarm interval remains two
updates. Both old platform-specific movement branches have been removed.

Rectangle queries use mask 278 against the present aligned 16×16 wall cells;
four corners suffice for this smaller 10×10 mask. This adapter is not a
general-purpose arbitrary-shape collision implementation. Published integer
coordinates remain separate from precise state, as in the recovered events.
The shared state is now included in startup digest schema 33; the menu replay
pin is `c26f108728127ae9`.

The default and phase-5 scene tapes now compare exact crawler position,
velocity, direction, latch and alarm state on every live paired update, in
addition to existing combat/frog/gem checks. Phase 5 passes (health 4/4,
kills 2/2, currency 2/2), superseding the collection failure above. Phases
1–5, 7 and 8 pass; phase 6 lacks a surviving frog hit and phase 9 has no
currency coverage. No equality checks were relaxed to accommodate those
missing-coverage runs.

Still incomplete: crawler hit-stun and source sprite transitions, time-stop
integration and Amiga quarter-turn presentation. SDL receives the source
movement angle; native sprite orientation remains a backend follow-up. This
is a shared movement migration, not a claim of full crawler fidelity.

The quarter-turn presentation follow-up is now implemented: the host asset
generator prepares all four orientations of both crawler frames, using
synthetic frame indices `turn * 256 + frame` under resource ID 278. Transparent
padding is cropped offline (with a minimum one-word width), retaining the
source anchor and existing Chip-RAM budget. Both masked-planar overflow and
DMA sprite bodies are prepared ahead of time. Amiga selects the orientation
from shared crawler direction and traversal mode; it does not apply the
generic horizontal-facing flip to this rotated actor.

Renderer tests independently rotate the baseline pixels around the anchor,
including circular-buffer wrapping, and compare every quarter-turn variant.
The prepared DMA streams additionally pass 15,288 scalar-oracle comparisons.
This closes rotation, not the hit-stun/sprite-transition and time-stop work.

Validation: the full host suite now runs both default and phase-5 combat
tapes; both pass with exact crawler-state assertions. The bounded A500
600-tick cache oracle and cross-CPU replay pass, retaining the player/gem
replay digests. Normal SDL/Amiga builds are restored and both 68000 audits
remain clean. Crawler state is covered by the live scene comparison, not by
the older narrow player/gem CPU fixture.

#### Shared wall-snail motion

Both runtimes now use `SnailMotion` for source `enmSnail` attachment and
movement. Its fifth-Q8 state retains exactly 0.1 pixel/update, replacing the
native 26/256 speed approximation. Initial wall checks use the recovered
rectangle mask, including rejection of embedded spawns; motion calls the
shared solid-collision solver and reverses on either a vertical collision or
loss of the source's wall-support point. Horizontal and vertical flips are
shared state, and published coordinates come directly from that state rather
than rounded float mirrors. Both duplicate movement branches were removed.

Focused tests cover 1,000 updates in each direction, attachment on either
side, embedded/detached retirement, wall-end reversal, and attachment before
the time-stop movement guard. The scene comparator checks exact shared snail
state whenever a live pair is present; this is not a guarantee that each tape
contains one. Startup digest schema 34 includes snail state; the menu replay
pin is `b81f52fa9040024a`.

Still outstanding: snail hit reactions and associated sound/RNG/alarm changes,
the damage sprite, and connecting the shared time-stop guard to full world
time-stop behavior. Rectangle queries use the existing area-one wall adapters,
not a complete implementation of every original wall subtype. No claim of
complete snail or world-event fidelity follows from this extraction.

Validation: full host suite (including default and phase-5 combat), bounded
A500 600-tick cache check and the existing player/gem cross-CPU replay pass.
Normal builds are restored; both 68000 audits are clean. The cross-CPU fixture
does not itself exercise every snail branch; those are covered by the shared
motion unit tests and native adapter fixture.

#### Shared surviving-snail hit response

`SnailHitState` now queues surviving bullet hits for both runtimes, resets
animation, sets hit-stun, extends Alarm 2 by five, clears Alarm 0 and makes
one RNG draw for the 50–80 Alarm 1 interval. The first extension starts at
the disabled value -1 (four updates); later hits extend the remaining timer.
The alarm remains zero on its firing frame before becoming inactive next
frame, matching the existing shared alarm-bank convention. Alarm 2 clears
hit-stun; the following Step selects normal snail art again.

Source sprite 178 is now exported with both Y orientations and X facings for
masked planar and DMA drawing. Transparent padding in both normal and damaged
snail frames is cropped offline, preserving anchors and the existing resident
art budget. Independent flip checks and 15,680 DMA/scalar comparisons pass.
The native integration fixture fires a usual bullet into an awake snail,
requires health 10 and a queued hit, then checks damage art, alarms and recovery.
Unit tests additionally cover repeated-hit timer extension and RNG draw count.
Startup digest schema 35 includes this state; its menu replay pin is
`9a7db713c820e867`.

This supersedes the missing surviving-hit presentation above, but not the
broader timing caveat: hit flags use the existing next-enemy-update queue,
not a fully recovered collision-event scheduler. Alarm 1 uses an integer
projection of the source random range; fractional alarm setter semantics
still need Windows-runner confirmation. Fatal-hit sequencing, world time-stop
and actual Amiga sound output remain incomplete.

#### Ground-turtle walking flag correction

Recovered GroundTurtle Create chooses `walking` from +1/-1, and Alarm 1
negates it. Its Step bytecode uses `conv.v.b` both to choose idle/walking art
(blocks 33–35) and to guard horizontal integration (blocks 41–42). Both ports
previously moved during either phase and selected walking art for any nonzero
value. A shared `groundTurtleWalking` predicate now gates movement and selects
sprite 186 for the negative phase, 188 for the positive phase; airborne sprite
187 is unchanged. This follows GameMaker's numeric boolean conversion, not
C++ nonzero truthiness ([official bool documentation](https://manual.gamemaker.io/lts/en/GameMaker_Language/GML_Reference/Variable_Functions/bool.htm)).

The native fixture now checks that a negative phase leaves X unchanged and
uses idle art, then advances through the next recovered alarm and requires
movement and walking art to resume together. Zero and airborne presentation
are tested separately. The following shared-motion migration replaces the
previous separate snake/turtle collision and integration branches.

#### Shared currency motion/collection phase and expanded CPU fixture

`CurrencyStep` now owns the implemented per-gem phase on both platforms:
advance motion/lifetime, return immediately on expiry, check the recovered
collection mask, and apply shared accounting only on collection. It explicitly
distinguishes the player's frame-start homing target from the current
collection position. Adapters retain only storage retirement, survivor
presentation conversion and collected notifications.

Focused tests cover the obtainable boundary, expiry without terrain/RNG or
collection side effects, small versus large masks, distinct homing/collection
positions, and attracted survival at the expiry alarm. The long cross-CPU
fixture still exercises 400 motion updates without applying collection; an
additional fixture now applies small/large collection, 300 account timer
updates, and expiry-before-collection. Digest schema 3 has expected gem digest
`8d8e864225e003ee` and coverage `7f` (the new bit is collection/account checks).
The 64-update player digest remains unchanged.

Validation: full host suite, both combat tapes with exact ordered gem state,
16,562 offline sprite comparisons, and bounded A500 600-tick cache verification
pass (101 comparisons and intact stack guard). Expanded cross-CPU output is
`64 9f0d3943 e7900d2b 400 8d8e8642 25e003ee 7f`; the new collection/account
coverage bit is required by the host reference. Both 68000 audits are clean.

This consolidates existing phase semantics. It does not establish complete
Windows collision-event order, upgrade effects, exact terrain bounce or
Create's embedded-wall relocation. No asset or hardware-scroll changes are
required for this migration.

#### Shared currency account and source timer boundaries

Both runtime stores now embed the same `CurrencyAccount`: spendable currency,
run-wide gameGem, streak and timer. Collection updates this state once through
the shared implementation. Amiga previously lacked the separate run-wide
counter. The SDL digest retains the same four serialized fields/order.

Original scrControlBeginstepCheck assembly blocks 21–24 decrement a positive
timer and then independently clear the streak if the timer is nonpositive.
Both old ports used an else branch, clearing one tick late. The shared Begin
Step update now expires exactly on the transition from one to zero, before
collection. A gem collected on that frame starts a new streak. scrPlayerInit
also sets timerStart+120; main-room player creation now establishes that
420-tick entry grace period instead of zero. Ordinary collection resets it to
the source default 300 ticks.

Focused fixtures test tick 299/300, collection on expiry, separate run totals,
entry initialization, paused/time-stopped timer guards and the source +180
less-resistance option. The latter guards/option are available in the shared
account but are not yet connected to complete gameplay pause/time-stop or
upgrade systems. Combat comparisons assert all four account fields each
frame. Achievement calls, gem-high weapon changes and collection effects are
not supplied by this accounting migration; global event ordering beyond this
Begin-Step countdown still needs work.

Validation: full host suite, both combat tapes with per-frame account and
ordered gem-state checks, 16,562 offline sprite comparisons, bounded A500
600-tick cache verification (101 comparisons, intact stack guard), and
cross-CPU player/gem replay pass. The narrow replay digest is unchanged;
these checks do not prove pause, upgrades or full original event ordering.

#### Shared reward creation and stable gem traversal

`CurrencyReward` owns the implemented area-one gem creation sequence for both
ports. `scrCurrencySpawn` assembly blocks 4–15 emit tens first, then twos until
the remainder is nonpositive (an odd reward therefore rounds upward). Each
accepted birth initializes shared GemMotion once, preserving its Create RNG
sequence. The caller supplies storage and the explicit remaining capacity.
The existing port overflow policy still drops excess births without consuming
RNG; this is not a claim about original runner allocation failure.

Native rewards previously reused the first inactive array slot, inserting new
gems ahead of surviving older ones. Stable compaction before appending births
now preserves their traversal order like the dense host store. This matters
when a later gem bounce consumes RNG, not just for display ordering. Native
fixtures retire the first of fourteen gems, emit another reward, and assert
that the older survivors remain first. Split/odd-remainder, empty capacity,
nonpositive rewards and per-birth RNG tests cover the shared creator.

SDL stomp rewards now use published enemy x/y, matching the script's
instance_create arguments and native adapter, rather than precise xx/yy.
Paired combat checks compare gems in order and include exact position,
velocity, attraction and animation, replacing the sorted age/value/animation
multiset that could conceal reordering. Original global instance-event order,
Create's embedded-wall relocation and non-area-one gem modifiers remain open.

Validation: full host suite, both combat tapes with unsorted exact-motion gem
checks, 16,562 offline rendering comparisons, bounded A500 600-tick cache
verification (101 comparisons and intact stack guard), and cross-CPU
player/gem replay pass. The replay digest remains unchanged; both 68000 audits
are clean. Normal builds were restored. These are finite replay checks, not
proof of complete original reward/currency semantics.

#### Shared implemented enemy-contact response

`EnemyContactResponse` now applies the implemented contact response for both
ports: classification, reward callback, enemy retirement/defeat accounting,
stomp bounce, health loss, default 90-tick immunity and damage knockback.
Adapters supply storage, reward emission and presentation notifications;
native death-screen state is still updated by its adapter. The renderer's
`awake` flag no longer suppresses active-instance contact handling.

`scrTypicalDamage` assembly block 2 uses instance x/y for its direction, not
precise xx/yy. Both adapters now explicitly supply published coordinates for
knockback; SDL previously re-rounded precise coordinates. The contact-mask
classification still uses the existing shared rectangle approximation. Core
response fixtures assert a single damage/impulse during immunity, stomp during
immunity, single reward/defeat accounting, fatal knockback, and published
coordinate use. Existing runtime tests exercise actual player movement.

Validation: full host suite (including both combat tapes and 16,562 offline
sprite comparisons), bounded A500 600-tick terrain/cache verification with
intact stack guard, and cross-CPU player/gem replay pass. Replay digests remain
unchanged and the 68000 instruction/helper audits are clean. These fixtures
cover the implemented contact path, not all original combat features.

This is consolidation of the currently implemented default-style combat, not
completion of original collision/death semantics. Immediate stomp retirement
and reward remain approximations: the original subtracts 100 health and sets
hit/stomped flags for subsequent enemy death handling. Original global.death
timing, immunity upgrades, alternate styles, slow-motion, effects and audio
still require recovery. They should be implemented once in this shared path,
not patched independently in SDL and Amiga.

#### Shared frog presentation and delayed animation-rate state

`FrogPresentation` now owns frog sprite selection, forced rising/falling poses,
and the separate `imgSp` state for both adapters. Frog Step assembly blocks
1–2 copy imgSp into image_speed before movement. Blocks 49–57 select damage,
idle, preparation or flight afterward: idle writes imgSp=0.2, flight writes
imgSp=0 and forces frame 0/1, and damage/preparation preserve imgSp. The old
adapters assigned image_speed immediately and treated every grounded frog as
an idle-rate write. Matching ports were therefore not enough to establish
fidelity to this source control flow.

The shared state returns the rate at Step entry and updates the desired rate
only in those source branches. Storage adapters convert twentieths to their
existing float/Q8 animation representations. Fixtures cover landing, takeoff,
zero/downward velocity, preparation and stun precedence; paired gameplay
checks compare desired and applied rates independently. Startup digest schema
39 serializes the added state; the menu baseline is `4df000d8a197a6bb`.

Validation: full host suite and both combat tapes pass. Native-runtime
fixtures also assert delayed landing/takeoff rate changes directly, without
using SDL as their oracle. The bounded A500 600-tick cache check passes with
101 composition comparisons and its stack canary intact. Cross-CPU player/gem
replay retains its digest; both 68000 audits pass. That replay is not a
complete frog-animation comparison against the Windows runner.

This preserves the current automatic-animation advancement phase, rather than
claiming that phase has been established against the Windows runner. Float
versus rational accumulation and world time-stop remain separate fidelity
questions. No runtime asset conversion or scrolling change is introduced.

#### Shared main-room lifetime pass

`MainRoomLifetime.h` now owns the retirement iteration, implemented-object
policy, active-instance check and strict fractional-camera boundaries. SDL
exposes its instance records; the native storage adapter exposes enemy and
breakable records with their world-coordinate offset. Neither adapter owns a
second copy of the decision. The supported area remains one; the policy's
area-five stone guard is tested but does not implement area-five gameplay.

The previous native enemy pass rounded the camera and required the rendering
`awake` flag. Original `scrOutofview` assembly uses strict LT/GT comparisons
against viewY-160 and viewY+3000, guarded by room 11; the implemented enemy End
Steps call it without a render-visibility condition. Active offscreen enemies
now retire accordingly. Inactive pending instances remain pending, since
inactive instances do not receive End Step. The old fixture labelled an active
far-away enemy as a future inactive instance; separate fixtures now distinguish
those states instead of preserving that native-only exemption.

Fixtures cover both exact boundaries, one Q12 subpixel on either side, room
filtering, the stone area guard, unrelated objects and inactive instances.
Paired combat tests additionally compare enemy active/pending flags, rather
than skipping state checks whenever one adapter has already retired an enemy.
The camera trace setter now updates the authoritative Q12 state as well.

Validation: full host suite, both combat tapes with exact active/pending
checks, 16,562 offline rendering comparisons, bounded A500 600-tick cache run
(101 comparisons and intact stack guard), and cross-CPU player/gem replay
pass. The replay digest is unchanged and both 68000 audits are clean. These
finite tests do not cover every lifetime interaction or original event order.

This consolidates the pass, not the entire event scheduler: both adapters still
invoke it after their current combat/currency work. Original global event
ordering and destruction-event side effects remain separate fidelity work.

#### Shared hanging-bat activation and area-one terrain lifetime

`BatBehavior` now owns support discovery, wake decisions, hit/pursuit selection
and sprite selection for both adapters. The source checks ceiling support
first, then right and left walls (only hanging on a wall if unsupported below).
Ceiling hang is -10; left/right hang are -1/+1. The VM's numeric boolean branch
means -1 tests left support, not the right-support branch that C++ nonzero
truthiness would select. Hanging art is sprite 157 or 162, with four frames
and recovered wall-facing direction. Waking changes art immediately but does
not integrate pursuit until the next Step. Hit handling can wake a hanging
bat. Player and support-loss wake sound requests remain independent branches.

The native renderer's `awake` flag no longer suppresses live bat simulation:
the source bat Step has no such visibility guard. Paired tests compare live
support state and exact flight state, including before render awakening.
Focused fixtures cover ceiling precedence, strict player Y threshold, both
wall sides, retained support and removal of supporting geometry. Hanging
art and mirrored/DMA records are generated offline. Transparent padding in
area-one enemy frames is cropped with source-anchor pixel reconstruction
checks rather than increasing the resident-art budget.

This route exposed an existing lifecycle mismatch through an offscreen gem:
native terrain let it bounce on an old stone cell, while SDL had retired that
cell and therefore consumed one fewer RNG draw. Original objWall_n End Step
assembly blocks 0–1 guard `scrOutofview` with `global.area == 5`; the supported
area-one stone walls must not use that rule. SDL now retains them. Original
objBox_n End Step calls scrOutofview unconditionally; native breakable boxes
now retire after simulation outside the same strict viewTop-160/viewTop+3000
window. Retirement does not count as a bullet-destroyed block or repaint the
scrolling buffers. Dedicated fixtures distinguish retained stone from retired
boxes. Interior stone-to-tile conversion remains a separate unimplemented
source optimization; this correction does not claim all wall lifecycle modes.

Startup digest schema 38 includes bat activity; its menu replay baseline is
`9401c8d6d642244e`. World time-stop, original collision-event timing, exact
runner fractional mask conversion, and actual Amiga audio remain incomplete.

Validation: the full host suite passes, including exact live bat-state checks
on both combat tapes (health/kills/currency 3/3/22 and 3/2/12 on both adapters)
and 16,562 offline sprite comparisons. The bounded A500 600-tick run passes
101 terrain-cache comparisons and the new 256-byte task-stack guard. The
cross-CPU player/gem fixture remains `64 9f0d3943 e7900d2b 400 9543220e
d0d8e596 3f`; that narrow fixture does not itself exercise hanging bats.
Both 68000 audits pass. Separate runtime orchestration still needs migration
into the shared core; these checks do not establish whole-game equivalence.

#### Shared bat rectangle collision response

`resolveBatMotion` replaces the previous center-point displacement rejection.
It passes the recovered bat rectangle (sprite 153/154, including its asymmetric
X bounds and current facing) through the shared `scrCheckCollisionWith`
algorithm, zeros only blocked velocity components, preserves facing if X
velocity becomes zero, and then integrates. Bat Step assembly blocks 36–44
confirm this order and the target object 57. The current adapters' solid
objects 60 and 92 inherit from 57; platform object 93 does not and is excluded.

Q8 bat state converts exactly to the collision helper's fifth-Q8 units; no
floating-point collision implementation is added to the target. Focused
fixtures cover wing contact before origin contact, mirrored left contact,
last-clear floor placement, and the source X-then-Y diagonal fallback. The
scene comparator now checks awake bat positions, velocities, direction and
facing exactly across adapters, as well as hit and alarm state. Current default
combat finishes at health 3/3, kills 3/3 and currency 36/36; both adapters
follow the same changed route without relaxed comparisons.

The terrain query supports the current aligned 16x16 cells, not arbitrary
thin or precise masks. Windows-runner fractional place_meeting conversion,
other wall families and time-stop remain open. This is
source-backed collision control flow, not a claim of complete bat fidelity.

Validation: full host suite and SDL build pass, including both mandatory
combat phases (phase 5 retains health 4/4, kills 2/2 and currency 2/2).
Bounded cross-CPU player/gem replay passes unchanged, and the normal Amiga
build is restored with clean 68000 instruction/helper audits. The CPU replay
does not hash bat collision state; the dedicated tests above cover that scope.

#### Shared surviving-bat hit response

The active-bat movement adapter now uses shared `BatHitState` and
`stepActiveBat`. Recovered enmBat Step assembly block 15 sets hit-stun,
Alarm 0 to three, direction to hitDir, speed to three and image_index to
zero. Pursuit (including its random jitter draw) is skipped while stunned;
the new speed is not immediately integrated as knockback. Alarm 0 (code
1231, verified from the object pack) clears stun, enabling pursuit again.
Both adapters select sprBatHit 154 during this interval and normal 153 after.

The usual downward-bullet adapter supplies direction 270. This is not a
complete implementation of aim/spread or other weapons. Delivery retains
the existing next-Step queued-hit approximation used for frog and snail;
collision-event timing and fatal-hit side effects remain separate work.
Hanging activation and wake/support checks were subsequently added by the
shared behavior described above. World time-stop and exact runner mask
semantics remain missing. This earlier helper specifically covers active
flying bats; BatBehavior now wraps it with support and wake handling.
Amiga audio output remains unimplemented.

Hit art, mirrored planar variants and DMA sprite records are generated on
the host; no runtime sprite conversion is introduced. Tests cover no movement
or RNG while stunned, resumed pursuit, a surviving usual-bullet hit across
three native ticks, and paired live bat hit/alarm state. Startup digest schema
37 includes the bat hit state; its menu baseline is `e2675f511dd56e7d`.

Bat frame padding is cropped offline with original-anchor pixel reconstruction
checks, preserving the resident-art budget of 278,000 bytes. Full host tests
and SDL build pass, including 15,778 DMA/scalar sprite comparisons and explicit
three-tick hit fixtures in both adapters. The bounded A500 600-tick cache
oracle and player/gem CPU replay pass, followed by a normal target rebuild.
Those emulator fixtures do not specifically force a surviving bat encounter;
the dedicated host integration fixtures cover that behavior. Both 68000
instruction and 64-bit-helper audits remain clean.

#### Shared alarm countdown across runtime stores

Both the SDL twelve-slot alarm bank and compact native enemy slots now call
`AlarmBank::advanceValue`. A positive alarm fires when it reaches zero; zero
becomes -1 on the next alarm phase without firing again. Slots without a
serialized/inherited event remain untouched. The previous native loop kept
zero indefinitely and decremented every positive slot, including the snail's
unused Alarm 1 written by its hit response.

The native presence masks for all six supported enemy objects are checked
against `ObjectPack::findEvent` for all twelve slots, including inheritance.
Snail Alarm 2 also uses the same countdown primitive. Compact storage does
not require allocating twelve integers per enemy. Focused fixtures verify
zero retirement and preservation of unused snail slots, while the paired
scene test now compares live enemy Alarm 0/1 values exactly every update.

This consolidates countdown semantics, not the complete world scheduler.
Event body dispatch, creation/activation timing and queued collision hit
responses still have separate adapters. Event presence does not imply that
every recovered event body is implemented (notably bat/crawler hit response).

Validation: full host suite (including both exact live-alarm combat tapes),
SDL build, bounded 68000 player/gem replay and normal target rebuild pass.
The narrow CPU replay digest is unchanged; it does not replace the host
alarm fixtures or prove the complete original event scheduler.

#### Shared snake and ground-turtle movement

`WalkingEnemyMotion` now owns the dry-world movement algorithm for both
adapters. It follows the recovered enmSnake/enmGroundTurtle Step order:
gravity, eight/twelve-pixel ledge probes, horizontal collision walk and
reversal, vertical collision distance, grounded/facing updates, then
visibility-gated integration and coordinate publication. A paused turtle
retains its horizontal velocity and still falls. A collision reversal still
integrates the reversed velocity in the same Step. Approaching a floor does
not prematurely mark an airborne actor grounded.

Authoritative position and velocity use 1/1280-pixel units, preserving .2
gravity exactly on both CPUs. Native Q8 and host float members are mirrors,
not independent simulations. Offscreen motion probes run while integration
and published coordinates remain gated by the source view margins (32 for
snake, 64 for turtle). Tests compare the complete live walker state exactly
across both adapters, alongside focused landing/wall/ledge/pause fixtures.

The longer startup tape now selects source right-room pattern 6 rather than
3, retaining 95 generated rows and four generations. Pattern 6 contains the
additional J torch marker (seven special markers total). Its incidental
combat walk no longer encounters an enemy; a controlled generated-snake
fixture now verifies bullet death and physical rewards independently of
that route. Mandatory natural-combat scene tapes are unchanged: the default
finishes at health 3/3, kills 4/4, currency 50/50, and phase 5 at 4/4, 2/2,
2/2. Startup digest schema 36 includes walker state; the menu replay pin is
`235dc06eb7314970`.

This does not establish full original-game equivalence. Water/time-stop
integration, collision-event scheduling, and exact Windows-runner fractional
`place_meeting` conversion remain unaudited. The terrain adapter retains the
existing rounded rectangular-mask convention and aligned 16x16-cell queries;
it is not an implementation of arbitrary thin or precise collision shapes.
The original Step assembly is the algorithm source; agreement between the
two ports alone is not evidence for those remaining runtime semantics.

Validation: full host suite and SDL build pass; the bounded A500 600-tick
terrain-cache oracle and cross-CPU player/gem replay pass, followed by a
normal target rebuild. The replay remains
`64 9f0d3943 e7900d2b 400 9543220e d0d8e596 3f`. That narrow CPU fixture does
not hash walking enemies; exact walker comparisons are host-side adapter
tests. Both target 68000 instruction and 64-bit-helper audits are clean.

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
