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

Next: finish live skeleton encounters, following the dependency order in
section 1. Then make catacombs playable (section 2), followed by the remaining
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
  collision adapters. Complete original room creation order across families.
- Verify collision-family ordering, same-object traversal and cross-family
  painter order against the original runner, not storage-slot order.
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

### C. Connect damage effects and actors

- Complete the shared player-damage scripts in natural contacts, including
  source storage, audio/controller side effects, hit effects and immunity ordering.
  Verify live damage-number placement/direction against original captures and
  measure the native contact-math cost, not just port-to-port state agreement.
  Replace non-coincident fixed-point knockback normalization with the source
  point-direction/lengthdir path and preserve its precision through player motion.
  Extend integer-only damage text formatting if live source paths require VM
  real formatting; verify fractional text placement against the original.
- Implement remaining blood, corpse, debris and blast dependencies. Preserve
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
  Reduce speculative side-view preparation for views the player never enters,
  and cache misses on late direction changes or generation invalidation. Check
  throughput as well as worst-frame gaps. Extend natural horizontal-shop
  coverage while preserving ring ownership and both display buffers' debts.
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
- Reduce live contact direction, first-update damage-number trigonometry and
  damage-text drawing costs; retain original numeric rounding and painter order.
  Use the contact profiling tape to distinguish these from general combat work.
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
- Propagate terminal failures through the remaining direct actor, reward and
  collision loops: stop later work in the same frame without rolling back
  completed effects. Checked event dispatch alone does not cover these loops.
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
