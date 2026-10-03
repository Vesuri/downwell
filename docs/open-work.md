# Remaining work

This is the single work queue for the Downwell port. Keep only unfinished work
here; remove items when completed. Completed changes belong in the git log.
Linked technical documents explain source behavior and implementation constraints,
not separate priority queues.

Both SDL and Amiga must behave like the original Windows Steam game. VM assembly,
serialized resources and original-runner evidence are the authority; port-to-port
agreement alone is insufficient. Target Amiga 500/68000. Let gameplay run slower
on PAL; do not add 60 Hz catch-up.

## Priority and scope

Next: recover the A500 gameplay frame budget (section 5) by removing emulated
runner state that nothing observes, per-element traversal overhead and CPU
composition, following the [frame-cost audit](amiga-frame-cost.md); compiler
tuning is not the remedy. Cover the garden-to-well transition as well as
steady gameplay. Then finish live
skeleton encounters, following the dependency order in section 1, make
catacombs playable (section 2), followed by the remaining
content (section 4). Apply the fidelity, performance, architecture, memory and
verification requirements in sections 3 and 5–8 to each integration; they are
not deferred cleanup phases.

Supporting helpers alone do not close an item. Completion requires the shared
behavior, both backend integrations and relevant original-game verification.

## 1. Finish live skeleton and bone integration

### A. Resolve collision identity and ordering

- Connect source-equivalent `instance_place` selection to the live world:
  spatial-index population, updates and traversal, runtime branch selection,
  activation/rebinding and lifetime-safe instance lookup. A boolean hit or Draw
  order cannot identify the field that determines shard direction.
  Extend the SDL clock-query owner's coverage to identity-bearing and other
  gameplay queries; register the remaining native field, effect and projectile
  lifetimes in the resource-backed owner before replacing its unindexed
  collision adapters. Complete original room creation order across families,
  retaining source numeric instance IDs separately from native family-local
  generation handles for serialized bindings and global lookup order.
  Include player-created depth meter/camera/entry-dither and conditional
  companion births, not just visible actors; preserve entry-dither's published
  view read and effect-surface Draw. See the
  [main-room birth audit](original-behavior-audit.md#main-room-controller-births-and-numeric-ids).
- Replace the usual-gun Q20/Q32 trajectory approximation with original setter
  and trigonometric semantics before enabling projectile selectors. Preserve muzzle-effect
  creation, collision-event ordering and destruction timing around movement.
  Integrate the [source arithmetic contract](projectile-motion.md) with absolute
  room coordinates and original collision-wrapper rounding; replace both live adapters,
  not just their displayed positions.
  Connect non-precise line selection and ordered setter publication before
  enabling the source-ordered spatial line path for live bullets.
- Verify collision-family ordering, same-object traversal and cross-family
  painter order against the original runner, not storage-slot order.
- Complete the recovered selected GMS2 layer/element ordering and verify the
  box impact's deferred -60000 depth override in full live composition.
  Keep layer Draw order separate from room animation order; audit existing
  room-list-based Draw adapters against the selected runner branch. See the
  [depth-update audit](projectile-motion.md#selected-depth-update-path).
  Complete native ownership for omitted room/controller instances and mixed
  composition across retained terrain, fields and separate effect passes; complete live
  depth-setter publication for remaining actor families, activation ordering and Draw-time structural
  mutation handling; preserve inactive-instance termination of each layer walk.
- Resolve remaining precise-mask sampling and original runtime FP-mode/rounding
  assumptions, including rotated masks required by blood time-field contacts.

### B. Connect time-stop state and effects

- Connect inherited wind-audio collision behavior for room-entry
  `startTimeField` (371) and exit fields, preserving sound guards and choice RNG
  on both platforms. Verify their cross-family Draw order against the runner
  when entry, ordinary and exit fields coexist.
- Connect player Begin Step `checkStop`/`pTimeStop` and parent-time-field queries
  to both world schedulers before blood dispatch. Goal-stop and shop control-lock
  are not substitutes. Preserve transition effects, RNG and audio order.
- Add Timeout-damage creation of ordinary object-82 time fields. Connect the
  parent-field contact adapter and complete Draw scheduling across object
  families, including live time-stop and activation state.
- Integrate time-shard lifecycle, animation, burst and time-change owners with
  both schedulers. Wire fractional/mirrored player and shard contacts, transition
  pacing and audio. Preserve moving shards while gravity/animation are paused,
  and the separate alarm cooldown rules.
  Connect the native time-change registration to the complete global numeric-ID
  stream before enabling automatic player transitions; its explicit-ID entry
  point is not a substitute for source-ordered allocation.

### C. Connect damage effects and actors

- Complete the shared player-damage scripts in natural contacts, including
  source storage, audio/controller side effects, hit effects and immunity ordering.
  Verify live damage-number placement/direction against original captures and
  measure the native contact-math cost, not just port-to-port state agreement.
  Replace non-coincident fixed-point knockback normalization with the source
  point-direction/lengthdir path and preserve its precision through player motion.
  Extend integer-only damage text formatting if live source paths require VM
  real formatting; verify fractional text placement against the original.
- Connect debris to source-ordered natural breakable destruction and complete
  its production scheduling, including the player's live time-stop state.
  Replace the removal-only box Step with the full ordered audio, smoke, flash,
  impact, chain-reaction, debris and reward sequence before lifetime retirement.
- Implement remaining blood, corpse and blast dependencies. Preserve
  synchronous child Create, RNG consumption and mutation order before rewards
  and retirement.
- Connect natural death/stomp/hit events to effect owners, including flash balls.
  Complete death/low-spec direct drawing at original world-depth boundaries,
  rather than flattening those paths into an overlay.
- Register skeletons and owned bones in shared gameplay: animation, alarms,
  Step, collision, End Step, player/bullet damage, rewards, capacity failure,
  out-of-view retirement and both renderers.

### D. Verify natural encounters

- Verify smoke collision dispatch, simultaneous contacts and wall retirement
  against the original; extend contact shapes as later wall families enter play.
- Compare joint currency/smoke composition with original captures, including
  shade holes and retained damage restoration.
- Compare full state/RNG through natural skeleton encounters, capture native
  effects and profile dense scenes, including complete smoke geometry costs.

References: [skeleton effects](skeleton-effect-ownership.md),
[effect surfaces](effect-surfaces.md),
[time-field contacts and identity](time-stop-collision.md),
[phase eligibility](runner-step-eligibility.md), [alarms](runner-alarms.md),
[animation fields](runner-image-fields.md).

Precise-mask reference: [collision bounds](runner-collision-bounds.md).
FP-mode evidence and remaining runtime checks: [CRT startup](runner-fp-startup.md).

## 2. Make catacombs playable

- Connect recovered patterns and symbol dispatch to both live generators,
  including synchronous actor creation and builder state.
- Implement skulls, spiders and remaining catacomb hazards: source-specific
  motion, collisions, alarms, animation and artwork.
- Implement area transition/entry/exit and verify natural cavern-to-catacomb
  progression. Do not substitute cavern actors or patterns.

Reference: [area-two recovery](area-two-recovery.md).

## 3. Close original-game fidelity gaps

- Audit remaining random-choice call sites and full RNG state through creation,
  effects and rewards. Rebuild checkpoints after source-backed corrections.
- Audit remaining event ordering, fractional collisions, activation boundaries,
  time-stop/feedback, special camera modes and overlapping camera lockers.
  Finish remaining phase-aware published-view reads, including camera dirty
  setters, initial room publication and native time-effect queries; see
  [runner camera view](runner-camera-view.md).
- Replace approximate live animation where original float/sequence behavior
  changes observable frames or event timing. Cover relevant sprite/index writes;
  a legacy animation helper is not the selected GMS2 sequence path.
  For currency, integrate registered active-room membership, pending activation
  semantics and time-stop speed writes; see the currency animation audit.
  The standalone pre-Step animation pass does not close these ownership gaps.
- Complete side-room reward objects and omitted title/particle effects.
- Verify fractional raster positions, draw order, effect surfaces, shaders and
  alternate palettes against original-game captures.
- Verify Laser Sight's original collision-line rounding, mask behavior and GPU
  rasterization; host/native agreement alone does not establish these.
- Fill resource/manifest gaps needed by new features: masks, glyph/kerning data,
  room layers and extension dependencies. Preserve IDs and input/tool hashes;
  check decompiled code against VM instructions.

References: [behavior audit](original-behavior-audit.md),
[random choice](runner-choose.md), [activation](activation-fidelity.md),
[side cameras](side-camera-lockers.md).

## 4. Complete remaining game content

- Remaining areas, enemies, hazards, bosses and ending flow.
- Remaining weapons, styles, upgrades and their interactions; hard-mode rules.
- Complete shared projectile state and lifecycle before dependent upgrades:
  damage, direction, speed, range, penetration, artwork, creation RNG and
  collision order. Use it for Popping Gems, Gunpowder Blocks, Drone, Hot Casing
  and weapon-specific Laser Sight behavior rather than backend-specific rules.
- Complete blast/corpse lifecycles for Blast Module, Rocket Jump, Rest in Pieces
  and Knife and Fork; gun-module hit/reshuffle behavior for Reverse Engineering;
  companion/time-field lifecycles for Heart Balloon, Drone and Timeout; and
  Safety Jetpack input, fuel, reset and thrust.
- Complete upgrade pickup/grant/HUD effects and validate each upgrade through
  a live deterministic encounter, original-game evidence and bounded native
  execution. Inventory or offer support alone does not close an upgrade.
- Music and sound on both platforms, including Amiga conversion, mixing,
  timing and memory residency.
- Persistence, unlocks, achievements where applicable, settings and complete
  restart/continue behavior.

Implement shared behavior and connect both backends. A feature absent from both
ports does not count as parity with the original.

Reference: [upgrade IDs and source entry points](upgrade-coverage.md).

## 5. Meet the Amiga frame budget

Fit worst-case gameplay, style/upgrade navigation and transitions into 20 ms.
Average FPS is not the criterion; re-profile after integration changes. The
[frame-cost audit](amiga-frame-cost.md) attributes the gameplay budget to
emulated runner state that no query reads, per-element traversal overhead,
CPU composition and software arithmetic, not to game rules. Remove work
rather than tuning it; compiler flags are not a substitute.

Every step keeps the replaced path compiled as the reference. Compare full
RNG state, numeric instance IDs, creation and event order, positions and draw
order on deterministic replays with intervening gameplay RNG and health
changes, then check native timing with the trace and FPS tapes.

Native gameplay now constructs the collision owner with native ownership
(`GameplayCollisionOwner::Ownership::native`): terrain lives only in the
occupancy grid, and actors keep only their order state (object, flag word,
sprite index, registration, room and layer order), not positions, bounds or
spatial history. `make verify-terrain-ownership` compares this against the
complete registered reference every frame. Wall-contact multiplicity lives in
the occupancy grid's cells, and native wall/platform runs publish as spans
after their draws. A plain gameplay frame's simulation step runs while the
previous frame's Copper list waits for its VBlank; the frame fences before
its first display, overlay or Copper write. Contact-tape traces measure quiet
frames at 31 ms, combat 73 ms, time-field frames 66 ms and the generation
bursts at frames 701 and 844 at 183-185 ms.

### A. Keep native ownership sufficient

- Native ownership rejects every collision query and wall End Step. When an
  original query or wall event enters native gameplay, answer it from native
  state (the grid for terrain selectors, family records for actors) with the
  same first-hit identity and order, verified against the registered
  reference. Do not re-enable per-instance spatial publication for it.
- Actors at the wall depth (100) are rejected, because an inactive wall ends
  that layer's Draw. Supporting one needs the layer's wall-order cutoff.
- The opt-in `TERRAIN_RUN_STORE` representation optimises registered terrain,
  which native gameplay no longer keeps. Remove it unless the registered
  reference itself needs it.

### B. Remove the remaining per-element traversal

- Alarm dispatch (1.2 ms quiet, 3 ms combat) still walks every captured
  enemy and effect for each alarm index; native ownership skips box groups
  while no live box has Alarm 0 set. Count the remaining families' alarms
  down in their records in captured order, dispatching only zero crossings
  and retiring unrearmed zeros to -1.
- The animation walk (4.7 ms per combat frame) still decodes and revalidates
  each visited actor; bullet animation is 1.4 ms of it. Run per-family loops over
  compact records where no Animation End handler has cross-instance effects,
  keeping room order where one does.
- Draw collection rebuilds through the generic layer walk after every
  ownership change. Maintain the actor Draw list incrementally, ordered by
  layer, depth and element order.
- Validate handles and owner state at birth, retirement and room entry, not in
  every per-frame visit (`valid()` and view lookups cost 2–3 ms per frame).
  Keep a sticky failure flag checked at phase boundaries.
- Generation bursts: frame 701 still spends about 100 ms in generation, 30 ms
  of it the per-cell builderCavern RNG draws. The two border walls of each
  pattern row still take the scalar Create chain (about 1,500 cycles each).
  Box and field births pay generic layer attachment (about 1.8 ms each).
  Keep each cell's Create and RNG draw synchronous; spreading generation
  over frames changes RNG interleaving.
- Measure steady gameplay and the garden-to-well transition; a short
  `SKIP_TO_GAME=1` sample does not cover the reported worst transition stall.

### C. Render with the blitter and precomputation

- Currency/smoke composition costs 9 ms per combat frame for about three
  stamps: each still pays about 14,000 cycles of per-row bounds and three
  generic blitter-queue enqueues, plus a drain before the final-alpha test.
- Currency/smoke: use blitter zero-flag tests for final-alpha overlap.
  Profile overlap-aware sprite assignment on dense gem/smoke
  scenes. Verify the mixed DMA/planar frame against original captures,
  preserving controller priority.
- Actors: blitter bobs or hardware sprites instead of CPU planar stamping.
- Time-field outlines cost 28 ms per frame while fields are visible (about
  16% of contact-tape frames). The tape's field is one pulsing 24-26 px
  circle whose raster changes almost every frame, so no word set can be
  retained; the CPU spends about 1,000 cycles per outline word on raster,
  matching, restoration and publication. Blit prepared outline images per
  small-circle raster with saved backing instead, without changing Draw RNG
  consumption or source geometry. Scrolling partly visible large circles
  could reuse rows visible in both views; this tape does not exercise it.
- Ammo HUD: each recharge redraws the whole gauge for four jiggle frames
  (24-30 ms each on the contact tape) and each shot's row animation costs
  15, 11, 11 and 5 ms. CPU composition costs about 1,000 cycles per gauge
  row, even as one fused pass in registers. Compose column 18 with the
  blitter from Chip-resident layer columns instead: background and rim at
  the meter offset, the fill, and row art with dividers at four times the
  meter offset, keeping the CPU for row-art updates and the label. Only
  about 8.5 KB of Chip RAM remains after startup allocations, so the layers
  must fit in a few KB (single-row masks with negative modulos, gauge rows
  only) or free Chip elsewhere.
- Terrain rows: blit tiles or prepared wall strips into the ring. Preserve
  circular hardware scrolling, static terrain backing and incremental
  restoration. Keep external coordinate writes on the observed publication
  path; see [terrain update constraints](terrain-update-contract.md). Reduce
  speculative side-view preparation and cache misses on late direction changes
  or generation invalidation. Extend natural horizontal-shop coverage while
  preserving ring ownership and both display buffers' debts.

### D. Use cheaper exact arithmetic

- Remove 32-bit multiply/divide helpers from per-frame paths using shifts,
  conditional-subtract rings and exhaustively verified reciprocal multiplies.
  Fail the build if they return, like the existing 64-bit helper audit.
- Add integer fast paths to the remaining per-frame binary64/binary32
  emulation, keeping the full emulation as fallback and oracle.

### E. Re-profile the residue

- Repeat the audit's trace windows. Dense combat and generation bursts are
  expected to remain over budget after A–D. Address per-instance combat
  simulation next: gems, bullets, and damage numbers, including live contact
  direction, first-update trigonometry and damage-text drawing. Retain
  original numeric rounding and painter order, and use the contact tape to
  separate these from general combat. Then address pattern emission.
- Measure new actor and effect costs in dense scenes. Keep a portable reference
  when introducing selected 68000 assembly optimizations.
- Repeat low-overhead timing tapes and correctness gates after changes, with
  separate horizontal-shop and dense-encounter cases. Require tapes that reach
  generation bursts, and distinguish instrumentation overhead from ordinary
  gameplay cost.

### F. Other scenes

- Reduce selection layout/content and transition preparation costs while
  retaining static layers and pointer-driven movement.
- Reduce initial selection-canvas and offer-bank preparation stalls. Measure
  asset staging and Chip/Fast residency tradeoffs separately from steady-state
  navigation; preserve original heading, icon and carousel animation.

Reference: [profiling procedure](amiga-profiling.md).

## 6. Finish scene and backend separation

- Move remaining cache-refresh/display coordination and transition orchestration
  out of `DownwellAmiga.cpp` into scene/backend classes. Consolidate SDL startup
  orchestration too.
- Keep publisher, credits, controls, garden, style selection, gameplay and upgrade
  selection independently owned; share style/upgrade presentation where useful.
  Keep game rules in `src/core`, not duplicated in platform backends.
- Consolidate remaining world scheduling and state adapters so both platforms
  invoke the same simulation. Remove replaced rule branches rather than
  retaining alternative platform-specific implementations.

Reference: [scene architecture](scene-architecture.md).

## 7. Establish memory fit and failure handling

- Measure Chip/Fast RAM, task stack, asset residency and peak transition usage.
  Establish the A500 memory requirement; expanded-emulator runs do not prove
  stock-machine fit.
- Exercise allocation/capacity failures and scene teardown/re-entry. Preserve
  meaningful nonzero exit codes for startup failures.
- Propagate terminal failures through the remaining direct actor, reward and
  collision loops: stop later work in the same frame without rolling back
  completed effects. Checked event dispatch alone does not cover these loops.
- Maintain reproducible offline assets and 68000 instruction/helper checks
  across supported build configurations.

## 8. Broaden end-to-end verification

- Isolate the unexplained native main-replay digest mismatch seen without
  per-frame tracing. Spatial/effect subtests matched; adding per-frame capture
  made two subsequent builds pass. Passing traced builds do not establish the
  cause or prove the mismatch fixed. Preserve a failing executable and symbols
  before restoration, then distinguish build-sensitive behavior from fixture
  or execution-state differences.
- Cover naturally funded purchases, all shop/side-room layouts and camera
  transitions, dense combat, uncommon upgrade combinations and repeated runs.
- Compare new behavior through deterministic host/native replays, including
  complete RNG and owned-instance state.
- Compare fresh terrain/HUD composition with retained buffers on natural routes;
  synthetic geometry alone does not establish rendering fidelity.
- Compare original Windows behavior/captures with both ports through startup,
  selection, gameplay, death, upgrades and later-area progression.
- Run all relevant regression gates before claiming parity. Component tests,
  CPU replays and screenshots each prove only their respective scope.
- Prepare release packaging, compatibility tests and installation documentation
  for the measured CPU/memory requirements, without bundling copyrighted inputs.

## Closing work

Remove an item only when its shared behavior, both backend integrations and
relevant verification are complete. Record implementation and test results in
the cohesive commit, not a second status document or a completed-items section.
Keep format specifications, source audits and reproducible test instructions in
the linked technical references; keep priorities and remaining work here.

Use `SKIP_TO_GAME=1` for gameplay, `SKIP_LEVEL=1` for completion and `SKIP_INTRO=1`
for style tests. Test full startup when scene/display ownership changes.
Time-limit emulator runs, stop owned sessions and restore normal builds.
Never modify `data/` or commit copyrighted exports/generated payloads.
