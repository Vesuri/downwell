# Remaining work

This is the single work queue for the Downwell port. Keep only unfinished work
here; remove items when completed. Completed changes belong in the git log.
Linked technical documents explain source behavior and implementation constraints,
not separate priority queues.

Both SDL and Amiga must behave like the original Windows Steam game. VM assembly,
serialized resources and original-runner evidence are the authority; port-to-port
agreement alone is insufficient. Target Amiga 500/68000. Let gameplay run slower
on PAL; do not add 60 Hz catch-up.

## Working order

The immediate milestone is live skeleton encounters (section 1). Work through
its dependencies in this order:

1. Resolve original field-identity selection and remaining collision semantics.
2. Connect player time-stop transitions, shards and pacing to both schedulers.
3. Connect damage effects, then skeletons and their owned bones.
4. Verify natural encounters against the original, on both ports.

Then broaden into catacombs (section 2) and remaining content (section 4).
Fidelity, performance, architecture, memory and regression work in the other
sections apply throughout. Supporting helpers alone do not close a milestone;
natural encounters must execute the recovered rules and render correctly.

## 1. Finish live skeleton and bone integration

- Verify original runtime FP-mode assumptions and measure the complete smoke
  geometry/effect cost on the native target.
- Verify smoke world collision dispatch against original-runner encounters,
  including multiple simultaneous contacts and wall retirement; extend contact
  shapes with later-area wall families as those areas are integrated.
  Verify live joint currency/smoke
  composition against original-game captures, including shade holes and retained
  damage restoration. Complete death/low-spec direct effect drawing at the
  original world-depth boundaries; do not flatten those paths into an overlay.
- Implement remaining blood, corpse, debris and blast effect dependencies.
  Preserve synchronous child Create, RNG consumption and source mutation order
  before rewards and retirement.
- Connect natural death/stomp/hit events to real effect owners, including flash
  balls; fixture-triggered rendering is not sufficient.
- Connect the player's source Begin Step `checkStop`/`pTimeStop` transitions
  and parent-time-field queries to both world schedulers before activating blood
  dispatch. Preserve transition effects, RNG and audio order; neither goal-stop
  nor shop control-lock is a substitute for this state.
- Add Timeout-damage creation of ordinary object-82 time fields. Connect the
  shared parent-field contact adapter to live transitions. Resolve original
  `instance_place` identity selection, including spatial-index construction,
  updates and traversal, runtime branch selection and activation/rebinding;
  a boolean hit or Draw order cannot select the field
  that determines shard direction. Complete Draw scheduling across other
  object families and live time-stop/activation state.
- Integrate the shared time-shard lifecycle, animation, burst and time-change
  owners with both live schedulers. Wire fractional/mirrored player and shard
  contacts into those events and verify original-runtime rounding assumptions.
  Connect transition pacing and audio rather than adding alternative platform rules.
  Preserve the source distinction between paused gravity/animation and moving
  shards, and the separate alarm cooldown rules. Create-only helpers do not
  establish live integration.
- Register skeletons and owned bones in shared gameplay. Connect animation,
  alarms, Step, collision and End Step, player/bullet damage, rewards, capacity
  failure, out-of-view retirement and both renderers.
- Verify collision-family ordering, same-object traversal and cross-family
  painter order against the original runner, not storage-slot order.
- Verify natural encounters with full state/RNG comparisons, rendered Amiga
  effect captures and dense-encounter profiles.

References: [skeleton effects](skeleton-effect-ownership.md),
[effect surfaces](effect-surfaces.md),
[time-field contacts and identity](time-stop-collision.md),
[phase eligibility](runner-step-eligibility.md), [alarms](runner-alarms.md),
[animation fields](runner-image-fields.md).

Blood time-field integration also requires the runner's rotated precise-mask
sampling and FP rounding rules; see [collision bounds](runner-collision-bounds.md).

## 2. Make catacombs playable

- Connect recovered patterns and symbol dispatch to both live generators,
  including synchronous actor creation and builder state.
- Implement skulls, spiders and remaining catacomb hazards: source-specific
  motion, collisions, alarms, animation and artwork.
- Implement area transition/entry/exit and verify natural cavern-to-catacomb
  progression. Do not substitute cavern actors or patterns.

Reference: [area-two recovery](area-two-recovery.md).

## 3. Close original-game fidelity gaps

- Restore specialized damage/death encounter coverage: the Candle route does
  not take damage, and the phase-six death route ends without kills or combo.
  Retain their real-damage, reward and death/results assertions; use explicit
  deterministic encounter inputs rather than forcing health or enemy state.
- Audit remaining random-choice call sites and full RNG state through creation,
  effects and rewards. Rebuild checkpoints after source-backed corrections.
- Audit remaining event ordering, fractional collisions, activation boundaries,
  time-stop/feedback, special camera modes and overlapping camera lockers.
- Replace approximate live animation where original float/sequence behavior
  changes observable frames or event timing. Cover relevant sprite/index writes;
  a legacy animation helper is not the selected GMS2 sequence path.
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

- Fit worst-case gameplay, style/upgrade navigation and transitions into 20 ms.
  Re-profile after integration changes; average FPS is not the criterion.
- Remove unnecessary terrain recomposition and transfers. Preserve circular
  hardware scrolling, static terrain backing and incremental restoration.
- Profile generation CPU costs without changing RNG consumption or synchronous
  Create ordering. Shop and exit Create effects must precede the next pattern
  cell's random draw; spreading generation over frames is not behavior-neutral.
- Break down ammo HUD costs into geometry, damage scanning and changed-word
  publication. Keep the static rim intact and update only changed pixels.
- Reduce selection layout/content and transition preparation costs while
  retaining static layers and pointer-driven movement.
- Reduce time-field outline coverage construction without changing Draw RNG
  consumption or source geometry. Measure geometry and word publication
  separately; changed-only writes alone do not guarantee cheap preparation.
- Reduce initial selection-canvas and offer-bank preparation stalls. Measure
  asset staging and Chip/Fast residency tradeoffs separately from steady-state
  navigation; preserve original heading, icon and carousel animation.
- Measure new actor/effect costs in dense scenes. Keep a portable reference
  when introducing selected 68000 assembly optimizations.
- Profile controller composition and overlap-aware sprite assignment on dense
  gem/smoke scenes; verify the resulting mixed DMA/planar frame against original
  captures while preserving controller priority.
- Repeat low-overhead timing tapes and correctness gates after changes, with
  separate horizontal-shop and dense-encounter cases. Distinguish instrumentation
  overhead from ordinary gameplay cost.

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
- Maintain reproducible offline assets and 68000 instruction/helper checks
  across supported build configurations.

## 8. Broaden end-to-end verification

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
