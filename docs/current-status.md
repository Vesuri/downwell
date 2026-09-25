# Current completion checklist

The Windows Steam build and its VM/resource evidence are the ultimate oracle.
SDL is a development comparison. Neither passing a CPU replay nor matching the
two ports proves original-game completion. The active parity goal is unfinished.

## Immediate queue

- **Now:** integrate recovered catacomb skeleton/bone behavior into the shared
  runtime. `SkeletonActor` now composes animation/attack decisions with movement,
  ordered death callbacks and player contact. It is not in the live enemy pool.
  Next connect instance/projectile ownership and global phase scheduling,
  precise contact predicates and actual effect sinks, then both renderers.
  `skeleton-effect-ownership.md` records the audited synchronous Create/RNG
  dependencies; effects cannot be omitted or batched after reward creation.
- **Next:** connect the catacomb builder and remaining actors/hazards, with
  original-source checks and bounded cross-target route verification. Do not
  enable incomplete actors merely to advance the area counter.
- **Continuing obligations:** worst-case 20 ms performance, natural-route
  coverage and scene ownership cleanup. Keep these distinct from new-feature
  completion; detailed measurements and outstanding coverage are below.
- **Later/full scope:** remaining areas, weapons, styles, bosses, audio,
  persistence and stock-A500 memory validation.

This queue describes priorities, not a replacement for the full goal. The old
`open-work.md` is an early milestone inventory, not the current ordered queue.

### Joined skeleton/bone encounter regression

Smoke Create now has a shared integer-only implementation (`SmokeBirth`),
verified across 5,180 births/1,024 seeds against scalar coordinates, sampled
speed/direction/image speed and full RNG state. Its normal binary64 multiplier
passes 100,000 products and rounding/range tests; sanitizers and 68000 compilation
pass without FP helpers. First Step's additional animation-rate draw is kept
separate from Create. Smoke motion/collision/lifetime and live ownership are
still open; see `skeleton-effect-ownership.md` for exact scope and source hashes.

Flash-ball requests now have fixed shared ownership, real Create-time jitter
and animation/retirement state through `FlashBallEffects`. Its 1,024 seeded
burst tests compare 6,178 births to scalar arithmetic, including coordinate
narrowing, creation-phase exclusion, seven-update Animation End and slot reuse.
Normal/sanitizer and 68000 compilation checks pass. Sprite 96's three unit
sequence keys are now exported/verified. The pool now belongs to `RunSession`:
both gameplay adapters advance it before Step, room/run retirement clears it,
and both renderer projections consume it. Public burst emission uses absolute
room coordinates and the owning world's RNG. Empty pools do no per-slot work;
draw order among effects is newest-first even after slot reuse. Natural death
callbacks remain unconnected pending the other synchronous effects.
The source RGB 254/7 palette issue is resolved for offline art:
the pinned embedded default shader maps those colors to light/dark, now
converted to our existing white/opaque-black pens for sprite 96 only. All 90
skeleton/bone/flash planar/DMA facings and pack reproducibility checks pass.
The pack is 323,118 bytes (+2,160); the ordinary Amiga build passes both 68000
audits. This preserves color classes, not exact Windows shader output RGB.
See `skeleton-effect-ownership.md` for the precise remaining boundary.

`make verify-flash-render-paths` passes fixture bursts through the production
host renderer and Amiga draw-list collector. The actual headless SDL framebuffer
also matches all three source-classified frames over a red background, including
opaque-dark occlusion. SDL prepares isolated mapped textures once at load time;
it does not change shared source atlases or convert pixels per draw. Room
retirement, shared gameplay and sanitized owner tests pass. The bounded native
CPU replay retains all existing digests and restores ordinary startup with both
68000 audits clean (`/tmp/downwell-flash-world-native.log`). This native tape
does not create flashes; rendered Amiga effect capture remains open.

**Full renderer-test route coverage restored:** the old input sweep repeatedly
jumped across the entrance opening: after 4,769 updates no enemy had activated.
Terrain inspection confirmed 55 generated rows and the centre destructible
blocks already removed; generation had not stalled. The damage controller now
steers toward x=240 until below the entrance bowl (y=1264), then resumes its
existing combat sweep. It reaches recovery after 2,402 main-room updates,
checking 45 hidden and 45 visible invulnerability frames. The subsequent
natural-death controller now approaches nearby live enemies at matching height
instead of firing at every threat; unrestricted pursuit was rejected because
it could stall above terrain. The full optimized `build/game-render-test`
passes, including real fatal damage, both white flash frames, Retry, shops and
level transitions. No production logic, terrain, health or damage assertions
were changed for these routes. This is host renderer evidence, not a Windows
comparison or a new native-rendered gate; it is not a claim that all tests pass.

Smoke/flash burst emission now has a shared implementation preserving count,
Y-before-X sampling and synchronous child Create interleaving. The skeleton's
two source argument sets pass 6,144 sequence tests with 15,454 child births,
complete RNG comparison and partial-capacity failure coverage. Sanitizers and
68000 compilation pass, and original range wrappers are pinned by
`verify-runner-effect-ranges.py`. The burst-only test's child callback models
draw consumption only; flash ownership is tested separately above, while smoke
state and live effect rendering remain unfinished. See
`skeleton-effect-ownership.md`; skeleton admission remains off.

After the shared player bounce signature change, the bounded native CPU replay
passes unchanged gameplay/weapon digests and all motion/bone lines, including
`BONE_ANIMATION 1024 7376a886 0`. The 180-second-limited verifier completed and
the ordinary Amiga build was restored with clean 68000 instruction/helper
audits. Evidence: `/tmp/downwell-stomp-cross-replay.log`. This checks the current
live paths and existing replay fixtures; it does not exercise a native skeleton
encounter or establish frame time. The death-effect audit is now recorded in
`skeleton-effect-ownership.md` so future owner integration preserves child
Create ordering and terrain-dependent RNG rather than just reward totals.

The skeleton stomp body now preserves `scrEnemyStomped` ordering rather than
using the cavern adapter's immediate-retirement shortcut: blood, style-dependent
forced jump, shake, 100 health loss, stomp sound, recharge, hit/stomp flags,
then optional Exploding Knees blast Create, sound choice and four smoke calls.
Death/rewards remain deferred to Step. Blast allocation failure is explicit;
it does not roll back the already executed stomp. Player globals are re-read
between synchronous smoke calls. Source: the extracted script's VM assembly.
The shared `PlayerMotion::stompBounce` accepts the style, retaining default
behavior and using -1.5 instead of -2.5 for Boulder. A 120-case stomp matrix
uses that real player code and checks callback ordering, all five styles,
death suppression, all explosion choices and failed child allocation. Normal,
sanitizer, shared player and joined encounter tests pass; the composed stomp
fixture compiles for 68000 without floating-point helper references. These
are event/adapter tests, not live Exploding Knees or skeleton admission: actual
blood/blast/smoke effect ownership and world phase wiring remain open.

The shared skeleton now exposes its fixed sprite-208 contact rectangle and a
precise player-contact adapter. Hit/throw artwork does not change that explicit
mask. Dispatch geometry uses published x/y; the event compares published enemy
y to precise global.ply and player ysp with runner tolerance, then re-reads
player grounding/Y after stomp. It deliberately has no added time-stop guard.
The metadata verifier also pins the Collision event's full instruction sequence.
10,086 pose/facing/edge cases and 150 precise/mutation cases pass, including
sanitizers; the joined encounter, bone and existing enemy-contact tests pass.
The composed 68000 fixture compiles without floating-point helpers. Actual
stomp/damage effects and global collision scheduling still belong to the live
owner integration; this adapter does not enable skeleton spawning.

`make verify-skeleton-encounter` now runs the actor and owned projectiles through
animation, global alarm slots, ordinary Step, synchronous throws and deferred
projectile retirement together. Its separate host float/double model compares
57,600 updates across 64 seeds, including exact animation, alarms, facing,
projectile trajectories and every RNG word. The tape covers initially inactive
actors, repeated attacks, hit-stun, time-stop/resume, aligned throws, both
facings and slot reuse: 736 throws and 640 out-of-view retirements. New children
are excluded from their birth Step and retain their initial animation speed.
The regression is included in `make test`. The normal and address/undefined-
behavior sanitizer runs pass, as do the existing actor and bone-store tests.

This is integrated component evidence, not live world admission or a Windows
execution capture. The test's rectangular projectile retirement bounds are
fixture geometry, not a claim about the original viewport. Collision-family
scheduling, real damage/effect sinks, renderer consumption and connection to
the live world remain unfinished.

### Latest correction: creation-phase enemy timing

Both live enemy adapters now skip ordinary Step for children generated during
that phase, and advance existing enemy animation before Begin Step/alarms.
The decoded host now initializes serialized sprites before Create rather than
relying on an immediate presentation Step. See `runner-step-eligibility.md`.
This corrects existing gameplay, not just the future skeleton components.

Full decoded startup and deferred-instance tests pass. Gameplay tests pass
normally and with address/undefined-behavior sanitizers. The old simultaneous
frog-alarm assertion was stale after the earlier alarm-order correction; it
now explicitly checks Alarm 0's jump and replacement Alarm 1 countdown of 59.
Creation-frame tests require unchanged position, frame and alarm, including
the newborn frog's serialized sprite 166 before its first airborne Step.

Paired natural routes pass with unchanged completion limits and state checks:
seed 99 exits at 1,339 updates, seed 4 at 1,646; both complete 211 upgrade and
120 re-entry updates. Seed 4 reaches its shop after 518 updates and returns in
43. Evidence: `/tmp/downwell-creation-phase-natural.log`.
The bounded rendered Amiga route also passes: exit 1,646, 211 completion
frames, 168 exit-field frames, 49 shop-HUD comparisons, 43 return updates and
688 second-level updates total. The task-stack guard passes. Evidence:
`/tmp/downwell-creation-phase-native.log` and `amiga/.run/gdb-startup.log`.

### Previous verification: original alarm ordering

The decoded scheduler and shared gameplay's explicit enemy Alarm 0/1 slots
now dispatch immediately in global ascending alarm-index order. This corrects
the frog's Alarm 0-to-1 rearm countdown; see `runner-alarms.md`. Focused
cross-instance mutation/deletion tests and sanitizers pass. Host natural
routes still exit at 1,339/1,646 updates, followed by 211 upgrade and 120
re-entry updates. Seed 4's next-level shop approach remains 518 updates and
return remains **43**. The earlier report of 42 mistakenly read the zero-based
loop index printed by the test; its completion assertion required 43 executed
updates throughout. The log now prints the actual route counter.
Evidence: `/tmp/downwell-alarm-order-natural.log`.

The bounded native CPU replay passes unchanged, including motion and bone
Create fixtures. The ordinary target build is restored with clean 68000
instruction/helper audits (`/tmp/downwell-alarm-order-native.log`). This is
followed by a separate passing rendered natural-shop gate: 211 clear frames,
168 exit-field frames, 49 shop-HUD comparisons and 43 return updates, with
688 second-level updates total and the task-stack guard intact. The first
attempt used the mistaken 42-update expectation and failed that assertion;
the corrected bounded run restores the original expectations, not relaxed
comparisons. Evidence: `/tmp/downwell-alarm-order-natural-corrected-native.log`
and `/tmp/downwell-alarm-order-natural-corrected-gdb.log`. The emulator stopped
and the ordinary build was restored after both attempts.

## Implemented and checked

- Publisher/credits/controls, garden/well, style selection and first-area
  gameplay have native presentation paths. The full startup sequence reaches
  scrolling gameplay after the shared selection-renderer changes; splash
  publication durations, garden Copper patches, 138 style comparisons and
  the task-stack guard pass the bounded gate
  (`/tmp/downwell-strict-sequence-gdb.log`). This is not a Windows capture.
- `GameplayWorld` supplies shared player, supported actors, projectiles, gems,
  combat, shop/result state and run ownership. Adapter tests cover deterministic
  movement, combat/upgrade variants, death/survival and next-room re-entry.
  See `original-behavior-audit.md` and `shared-simulation-migration.md` for the
  exact scope and original-source caveats.
- The live Amiga loop implements level exit, upgrade selection and subsequent
  cavern levels. Two-clear verification compares 466 completion frames,
  192 exit-field frames and both re-entries. Both upgrade entries exercise
  cursors 0–2 and held-key edge filtering; see `upgrade-navigation-verification.md`.
  This supersedes the old claim that
  the result room or all level advancement remains unimplemented. The fixture
  injects exits; it does not prove a natural full-level playthrough.
- `make verify-natural-exit` adds input-only paired host routes for seeds 99
  and 4: 1,339/1,646 gameplay updates to naturally generated exits, 211 upgrade
  updates and 120 second-cavern updates each. Terrain inspection selects
  ordinary movement/fire input; no exit, player position, health or terrain is
  injected. Gameplay state is compared each update, with upgrade effects,
  offers, inventory and RNG checks. This is host adapter evidence, not a
  native rendering gate. Seed 4 additionally reaches an unmodified generated
  shop in cavern two after 518 updates and compares opening, held fire and
  Leave, including menu state and generated stock. No currency or shop items
  are injected. A purchase with 18 gems against a 150-gem price is rejected;
  43 subsequent updates return the player and camera to the main shaft.
  Successful naturally funded purchasing remains unverified.
- `make verify-route-sweep` additionally checks seeds 0–7 with deterministic
  recovery inputs, requiring all eight generated exits alive within 2,500
  updates each. The corrected-stream rerun compares 15,118 gameplay updates,
  including full RNG state every update, across 95 generated enemies. This broadens host encounter
  coverage; it is not a native rendering or original-Windows comparison.
- The seed-4 native natural-route gate now reaches the generated exit at
  update 1,646, compares 211 upgrade frames and 168 exit-field frames with no
  failures, and completes 120 second-cavern updates with an intact stack guard.
  `make verify-amiga-natural-route` runs this bounded check and restores the
  ordinary build. The extended gate passes the same natural shop approach,
  insufficient-funds attempt and return after the level-dependent spawn fix:
  688 second-cavern updates, 49 retained shop-HUD comparisons with no failures,
  and 43 return updates. It does not
  establish every shop layout or fresh composition of all terrain pixels.
- Results-menu tests cover all three destinations and 200 final gameplay
  updates, including display-memory reuse and the task-stack guard.
- A dedicated native horizontal-camera fixture covers all three columns and
  seven live transitions: six prepared-buffer swaps and one forced fallback.
  It passes 119 fresh-terrain comparisons and 14 alternating-display checks
  with no mismatches. This uses synthetic flat collision geometry, not natural
  side-room/shop traversal; that end-to-end coverage is still outstanding.
- A live native shop fixture passes opening/held fire, insufficient funds,
  purchasing, next-update battery grant, leaving and reopening sold stock.
  All 13 retained HUD frames match fresh scalar composition. The corresponding
  paired host scene/shop test passes. Placement and a free test item are
  injected; this does not establish a naturally generated shop playthrough.
- Style and upgrade selection share `AmigaSelectionLayer`. Static content is
  retained; common vertical movement changes bitplane pointers. Style content
  borrows retired game/HUD buffers without another Chip allocation. Its
  364-frame host test covers all five cursors; 105 native style comparisons and
  the three saved frame hashes pass. See `scene-architecture.md` for logs.
- Hardware sprites and planar artwork are exported offline. Target builds
  retain 68000 instruction/helper audits. Native gates use hard time limits
  and restore ordinary startup afterward. The emulator uses Fast RAM; stock
  A500 memory fit has not been established.

## Remaining work — do not mark complete

- **Immediate fidelity correction:** original-executable inspection proves
  `choose` uses one WELL word modulo argument count, whereas existing
  translations often use two-word `irandomSmall`. The distinct shared
  `chooseIndexSmall` primitive is tested. Shared cavern generation and reward
  choices plus garden tree/grass, host title-event, shared gem, immune-impact
  and shop farewell/response
  choices are migrated;
  remaining call sites need a VM-backed
  audit and coordinated migration, followed by rebuilt checkpoints and route
  verification. See `runner-choose.md`. This takes priority over new-area work;
  earlier paired replays prove adapter agreement, not original RNG equivalence.
  Regenerated seed-2/seed-4 entry checkpoints now match decoded startup in
  all RNG words. The corrected host routes pass at 1,339/1,646 gameplay updates,
  each followed by 211 upgrade updates and 120 re-entry updates. After the
  additional level-dependent spawn correction, seed 4 reaches its next-level
  shop after 518 more updates and returns in 43 updates after an
  insufficient-funds attempt (18 gems, price 150). The corrected eight-seed
  sweep also passes, with unchanged input-only controller and completion limits.
  Garden reconstruction passes an independent VM-call/RNG-state oracle (77
  Create words). The complete startup test now passes with corrected fragment-2
  entrance and side-room-0 expectations; solid support, recharge, enemy creation,
  projectile and special-marker checks remain enforced.
  Title RNG order tests pass, but omitted particle visuals remain omitted.

- Normal cavern spawning now uses the VM's level-dependent probability and
  level-two/three crawler-cell choice, shared by both adapters. Independent
  RNG-state/boundary tests and sanitized 64-level generation pass. Hard-mode
  spawning remains unsupported; see `original-behavior-audit.md`.

- Complete the seven scene boundaries. All seven now have named Amiga scene
  classes; gameplay's owner controls its Copper lifecycle and backing/staging
  validity ranges. Transfer execution remains in the application loop.
  Terrain composition now belongs to `AmigaTerrainPainter`, with actors and
  raster storage already separate. `AmigaGameplayGeneration` owns synchronous
  generated-object registration, index/tile insertion and damage bounds; the
  paired host tests use that same native component. Cache-refresh/display coordination and
  much transition orchestration remain in `DownwellAmiga.cpp`; SDL startup
  orchestration also needs consolidation.
- Meet the worst-case 20 ms budget. The latest style-navigation measurement is
  45.45 FPS over 180 updates, with a maximum three-field gap. Combined upgrade
  content/layout/simulation work and transition preparation also exceed budget.
  After the RNG/cavern-spawn corrections and word-wide tile composition, the
  earlier 600-update gameplay tape ran at 35.09 FPS (855 PAL fields), with a
  three-field maximum gap (60 ms), improved from 34.36 FPS / four fields.
  After the creation/animation-phase corrections and offline skeleton asset
  addition, the latest run is 34.80 FPS (862 fields), maximum four fields
  (80 ms). This is not an isolated asset-cost comparison.
  The separate instrumented run identifies terrain recomposition as the
  dominant cost in its worst frame, not generation. The older 36.36-FPS
  measurement describes a different generated layout and is superseded.
  See `gameplay-performance-current.md` for scope, costs and next work.
  Keep hardware scrolling, retained terrain and incremental damage intact.
- Broaden end-to-end play coverage: natural exits, shops and side-room entry/
  exit, dense encounters, less common upgrade states and repeated room/run
  transitions. Existing fixtures are not proof of every live interaction.
- Audit original event order, special camera modes, fractional collisions,
  time-stop/feedback effects and Windows rasterization. Resolve disagreements
  using VM/resources or original-runner captures, not SDL alone.
  Side-camera locker registration/latch/release now runs on both adapters;
  four native locks and three releases pass with unchanged staged-swap
  coverage. See `side-camera-lockers.md` for source evidence and remaining
  achievement, multiple-overlap and original-runner caveats.
  Enemy activation now uses resource-backed bounding-box overlap with the
  source activation rectangle instead of centre Y alone. Natural native and
  extended host combat gates pass; historical-runner boundary/transform
  details remain unverified. See `activation-fidelity.md`.
  The shared side-room reward choice now includes the VM's low-health crate
  bias and style-1 override in the original RNG order; see
  `original-behavior-audit.md`. Full reward-object behavior remains separate.
- Audio, complete persistence/unlocks/settings, later areas, remaining weapons/
  styles/enemies, bosses and ending flow are not complete. Distinguish features
  absent from both ports from actual Amiga-versus-implemented-SDL gaps.
  Area-two pattern recovery now has a VM-backed export/verification target
  (50 patterns). Shared catacomb symbol dispatch passes 306 operation-trace
  cases, retaining exact builtin calls and synchronous emission order, but is
  not yet wired into either live generator. Singleton-choice RNG consumption
  is now resolved by original-executable inspection (one word; see above).
  Gameplay remains disabled pending integration,
  actors/hazards and artwork. Shared animation now supports exact fortieth-frame
  rates for the skull's 0.175 while preserving existing actor state and passing
  both natural host routes; new actors remain unregistered until their behavior
  is implemented. See `area-two-recovery.md`.
  Shared skeleton throw orchestration now preserves synchronous child Create,
  conditional RNG choices and velocity overrides in VM order; 9,216 state/trace
  cases and sanitizers pass. Live skeleton integration remains outstanding;
  the helper does not enable a new actor.
  Bone Create/Step/collision behavior now has a host numeric/event reference,
  with 1,024 RNG cases and 102,400 motion updates passing plus sanitizer checks.
  That host reference deliberately retains VM-style doubles; the target
  arithmetic components below are checked against it. Animation and live
  collision/damage dispatch remain integration work, not completed parity.
  An integer-only binary64 addition/pixel-rounding candidate now matches the
  host numeric reference and cross-compiles to 68000 without library helpers.
  Native numeric replay now matches 24,000 updates; an isolated two-add/
  pixel-round batch costs about 0.46 ms/update on the emulated 68000. It is
  still not deployed: complete projectile integration and live cost remain.
  `BoneMotion` now matches 201,600 host two-axis updates and a 7,000-update native
  replay. Its isolated complete-motion cost is about 0.90 ms/bone update;
  lifecycle, animation, collision and rendering costs are not included. No
  new live actor is enabled yet.
  `BoneProjectile` now wraps motion with source-ordered Step, wall/player
  collision and deferred destruction requests; 6,300 reference cases and
  sanitizers pass, and 68000 code generation needs no helpers. Instance owner,
  animation, real collision dispatch and presentation remain unwired.
  Target-side bone Create now retains the exact sampled binary64 ranges using
  integer-only conversion, with 600,000 conversion tests and full Create/RNG/
  motion comparisons passing. It remains a component, not live spawning.
  Native Create verification now matches all sampled fields and complete RNG
  state for 1,024 seeds (`BONE_CREATE 1024 ef0f74f5 0`), with a host double
  reference independently checking that digest. The 9,216 skeleton-throw cases
  now exercise target Create, velocity override and signed animation rate,
  rather than only the double reference. Existing native replay lines remain
  matched; the ordinary executable was restored with clean 68000 audits.
  Evidence: `/tmp/downwell-bone-create-native.log` and `amiga/.run/gdb-out.log`
  (the latter is overwritten by subsequent runs). This does not verify live
  projectile ownership, animation advancement, collision dispatch or drawing.
  `SkeletonTiming` now implements the four recovered alarm bodies and Step's
  hit/throw-stop/time-stop timing writes. The 64-case host matrix and sanitizers
  pass. Original-runner inspection now proves immediate dispatch in global
  ascending alarm-index order; Alarm 2's write to Alarm 3 counts down in the
  same phase. The new per-slot API and restart tests preserve this distinction
  from snapshot dispatch (see `runner-alarms.md`). Global owner integration
  and animation advancement remain incomplete, so live admission stays off.
  Original-runner image-field inspection additionally proves that builtin
  `image_index`/`image_speed` setters narrow VM doubles to binary32. The
  skeleton timing component now preserves that boundary rather than treating
  its speed as the unchanged custom `imgSp` double. Integer-only conversion
  passes 100,000 host cases and sanitizers, and a 68000 compile has no helper
  references. Advancement/wrapping and live actor animation fidelity remain
  unfinished; see `runner-image-fields.md`.
  The ordinary legacy animation update path is now recovered and implemented
  as an integer-only helper: single-rounded advancement, one upper-wrap
  subtraction and post-store Animation End indication. Its 40,000 scalar
  comparisons and skeleton's 100 time-stop/advance updates pass. Live phase
  ownership, resource-path selection and negative-frame rendering remain
  integration work; this does not enable skeletons or change live animation.
  Follow-up proves this build selects GMS2 animation and that advancement
  precedes Begin Step/alarms. The metadata export pins all 741 playback records;
  skeleton/bone sprites have unit per-game-frame speed but also V2 sequences.
  Mapping that sequence data to the runner's selected update branch remains
  necessary; the legacy helper alone is not an original-build parity claim.
  Full sequence-key verification now proves one unit-length frame key per
  skeleton/bone frame, looping mode and no moments/messages. Runner inspection
  shows a separate timeline and two-remainder wrapping for both signs, unlike
  the legacy helper. The temporary legacy advancement binding was removed
  from `SkeletonTiming`; timeline, frame-track evaluation and GML index-write
  synchronization must be implemented before live skeleton integration.
  `SequenceTimeline` now preserves the looping branch's signed remainder and
  intermediate binary32 rounding using integer-only arithmetic. Its 132,000
  host reference cases and sanitizers pass; 68000 compilation needs no helper
  symbols. Frame-track evaluation and index/sprite-write synchronization remain
  unwired, and no new actor is enabled by this arithmetic component.
  `UnitSequenceFrames` now evaluates the verified consecutive frame keys and
  detects GML index-write differences using the original float-rounded
  subtraction and 0.00001 double threshold. 50,804 host cases, sanitizers and
  helper-free 68000 compilation pass. Inverse index mapping, sprite changes
  and live ownership remain unfinished; the component is not a playable actor.
  `SkeletonAnimation` now joins timing, sequence advance, frame evaluation and
  the skeleton's explicit frame-zero writes. Idle/throw/hit sprite changes
  preserve or reset the frame according to the reviewed setter. 32,000 host
  updates and sanitizers pass with helper-free 68000 compilation. Arbitrary
  nonzero frame-write reconciliation remains rejected, and the component is
  not yet connected to the live enemy pool or throw/presentation decisions.
  `SkeletonDecisions` now connects activation, hit, facing and throw gates to
  that animation state, retaining the final hit-stun override after the owner's
  motion/death work. 4,608 scalar cases, sanitizers and existing animation/throw
  regressions pass; 68000 compilation needs no helpers. Live projectile/actor
  ownership remain integration work. The frame-gate comparison audit now
  proves strict comparisons against +/-0.00001 in the real/integer runner
  path. The corrected gates pass 387 adjacent-float boundary checks and the
  twenty-versus-twenty-one animation-advance regression; see
  `runner-frame-comparisons.md`. Owner activation/facing comparisons still
  need this same source-backed treatment when wired to live coordinates.
  `SkeletonActor` joins those decisions to the remaining ordinary Step body:
  grounded skips X integration only; Y integration remains unconditional when
  not time-stopped. Death runs even during time-stop, emits the source callback
  order and defers storage removal until after final impact/hit presentation.
  Player contact re-queries grounded state after the stomp callback, matching
  the two independent VM IFs. 4,500 fractional movement updates, death/order
  fixtures and contact-mutation checks pass normally and under sanitizers;
  the composed event compiles for 68000 without external arithmetic helpers.
  This is still an event component, not live registration: End Step out-of-view,
  inherited setup/effects, phase ownership and precise predicate adapters remain.
  `BoneProjectileStore` now supplies fixed-capacity child ownership, stable
  handles and explicit event-boundary retirement. `SkeletonBoneCreation`
  connects the throw helper to real stored child state: Create completes before
  sound, then choice RNG, velocity/spin overrides and the shot latch. Its live
  entry accepts a precise horizontal-separation predicate, avoiding integer
  rounding of the player merely to choose the RNG branch. 2,048 seed/alignment
  cases cover child state/RNG, exhaustion without draws/latching, deferred slot
  reuse and stale handles; sanitizers and the existing 9,216 throw cases pass.
  The composed 68000 fixture compiles without floating-point helpers, but does
  reference memcpy and integer multiply/modulo helpers. No timing improvement
  or live gameplay is claimed: creation-frame eligibility, animation, collision
  scheduling and presentation still need connection to GameplayWorld.
  Bone animation is now part of the owned projectile itself: default speed one,
  four-frame unit-sequence advance before Step, binary32 sampled-speed writes,
  reverse looping and zero speed on time-stop/death. `drawFrame()` exposes the
  frame for either renderer. Independent host arithmetic checks 102,400 updates
  across sampled positive/negative rates and stop/resume, alongside the existing
  6,300 lifecycle cases; sanitizers pass. The bounded native CPU replay adds
  `BONE_ANIMATION 1024 7376a886 0`, matching an independently checked host digest.
  Existing replay digests and task-stack guard also pass. Evidence:
  `/tmp/downwell-bone-animation-native.log`. This is CPU/state evidence, not
  live projectile rendering or an encounter-performance claim.
  Original Step creation eligibility is now audited: the dispatcher snapshots
  the creation counter and increments it before events, excluding children
  created during that phase; later phases take independent cutoffs. The bone
  store exposes explicit phase snapshots with tests for creation and slot
  reuse during dispatch. See `runner-step-eligibility.md`; collision traversal
  and original same-object list ordering remain outside this proof.
  Bone contact geometry now uses the metadata-verified sprite-460 rectangle
  (-5..4 X, -4..4 Y) at published coordinates, not the precise accumulators or
  changing artwork. Shared dispatch adapters connect player overlap and a
  caller-supplied wall rectangle query to the existing event bodies; the latter
  avoids a per-pixel terrain scan. 3,300 player/edge/time-stop/inactive cases,
  empty-mask checks, ordered wall effects and existing lifecycle/ownership
  tests pass normally and with sanitizers. This does not choose collision-family
  order or connect damage/effect sinks to the live world yet.
  Skeleton hit/idle/throw, corpse and bone artwork is now included in the
  offline gameplay pack. Living skeleton/bone frames include cropped planar,
  mirrored planar and split DMA variants; the corpse is planar-only. All 78
  anchored planar/DMA facings match the recovered source pixels, and full pack
  reproducibility, garden sprite and transition-mask checks pass. The addition
  is 98 records (2,352 directory bytes) plus 9,216 payload bytes: 11,568 bytes
  total, raising the pack to 320,958 bytes. A bounded SKIP_TO_GAME profile starts
  and completes with the larger pack; ordinary build restoration and 68000
  audits pass. No live skeleton rendering is claimed until the actors are wired.
  Logs: `/tmp/downwell-skeleton-assets.log`, `/tmp/downwell-skeleton-assets-native.log`.
  Skeleton activation/facing now accept precise binary64 player globals through
  `stepAtPlayer`, using the runner epsilon rather than rounded pixel predicates.
  End Step implements source `scrOutofview`: strict viewY-160/viewY+3000 bounds,
  main-room-only deferred destruction without enemy death/reward callbacks.
  125 near-equal activation/facing/speed combinations and 84 view-boundary/room
  combinations pass alongside the existing actor/decision tests and sanitizers.
  Full collision/effect ownership and live registration remain outstanding.
  Inherited skeleton Create now includes subparentEnemy's `move_snap(16,16)`
  before copying x/y into xx/yy. Runner wrapper 0x52a600 calls 0x4ff4b0, which
  divides the binary32 coordinate by the snap spacing, uses x87 `fistp` with
  the runner's nearest-even rounding, then multiplies back. The integer-only
  path supports integer spawn inputs, including builtin binary32 narrowing;
  positions outside the published int32 domain fail atomically. 8,193 signed
  coordinate pairs and large-value/overflow tests pass under sanitizers, as do
  updated bone-spawn fixtures. Inherited time-field collision now sets health
  to zero unless immune, leaving death effects to the next ordinary Step.
  See `tools/verify-runner-move-snap.py` for the pinned disassembly guard.
  The skeleton's machine-gun damage branch now preserves source hit writes and
  callback order: bullet destruction, hit direction/angle/damage, health loss,
  damage stun and moment delay, sound, fractional impact, effect, then the
  additional delay only when not hit-stunned. Lethal health does not retire the
  enemy or pay rewards in the collision callback; ordinary Step owns death.
  `takenImpact` is now binary64: nine damage gives 1.8, not integer 1. Tests
  cover 48 health/damage/stun/activation combinations, unsupported weapon
  rejection, seven-shot lethality and exact fractional impact reaching the
  later corpse callback. Normal/sanitizer and 68000 compilation checks pass.
  This branch is deliberately scoped to the current non-consistent,
  non-exploding 9/15-damage machine-gun bullets; live collision/effect sinks
  and skeleton admission are still unfinished.

## Working evidence

- `scene-architecture.md`: current scene ownership, retained presentation,
  focused checks and performance measurements.
- `feature-parity.md`: detailed chronological adapter and feature evidence;
  later entries may supersede earlier limitations.
- `original-behavior-audit.md`: original-source findings and fidelity caveats.
- `level-clear-profile.md` and `amiga-profiling.md`: measured costs, not FPS
  promises inferred from operation counts.
- `completion-checkpoint-history.md`: preserved former checklist, including
  obsolete "next integration" plans. Do not treat it as current task state.

Use `SKIP_TO_GAME=1` for gameplay work, `SKIP_LEVEL=1` for completion tests and
`SKIP_INTRO=1` for style tests. Test the complete startup sequence separately
when shared display ownership changes. Keep original `data/` untouched and
commit cohesive source/test/documentation changes without extracted assets.
