# Remaining work

This is the single work queue for the Downwell port. Keep only unfinished work
here; remove items when completed. Completed changes belong in the git log.
Linked technical documents explain source behavior and implementation constraints,
not separate priority queues.

Both SDL and Amiga must behave like the original Windows Steam game. VM assembly,
serialized resources and original-runner evidence are the authority; port-to-port
agreement alone is insufficient. Target Amiga 500/68000. Let gameplay run slower
on PAL; do not add 60 Hz catch-up.

## Next integration milestone: live skeleton encounters

Finish section 1 before broadening into catacombs. Supporting helpers alone do
not close this milestone: natural encounters must execute the recovered rules
and render correctly on both targets. Performance work in section 5 applies
throughout integration, not only after content is complete.

## 1. Finish live skeleton and bone integration

- Verify original runtime FP-mode assumptions and measure the complete smoke
  geometry/effect cost on the native target.
- Finish smoke world collision dispatch and connect its compositor to both
  live renderers, preserving the original effect-surface composition and
  restoring previous effect damage from retained terrain on Amiga.
  Migrate currency's `drawFx` path alongside smoke: retain gem colors and apply
  shade to their combined surface, not to a smoke-only overlay above direct gems.
- Implement remaining blood, corpse, debris and blast effect dependencies.
  Preserve synchronous child Create, RNG consumption and source mutation order
  before rewards and retirement.
- Connect natural death/stomp/hit events to real effect owners, including flash
  balls; fixture-triggered rendering is not sufficient.
- Register skeletons and owned bones in shared gameplay. Connect animation,
  alarms, Step, collision and End Step, player/bullet damage, rewards, capacity
  failure, out-of-view retirement and both renderers.
- Verify collision-family ordering, same-object traversal and cross-family
  painter order against the original runner, not storage-slot order.
- Verify natural encounters with full state/RNG comparisons, rendered Amiga
  effect captures and dense-encounter profiles.

References: [skeleton effects](skeleton-effect-ownership.md),
[effect surfaces](effect-surfaces.md),
[phase eligibility](runner-step-eligibility.md), [alarms](runner-alarms.md),
[animation fields](runner-image-fields.md).

## 2. Make catacombs playable

- Connect recovered patterns and symbol dispatch to both live generators,
  including synchronous actor creation and builder state.
- Implement skulls, spiders and remaining catacomb hazards: source-specific
  motion, collisions, alarms, animation and artwork.
- Implement area transition/entry/exit and verify natural cavern-to-catacomb
  progression. Do not substitute cavern actors or patterns.

Reference: [area-two recovery](area-two-recovery.md).

## 3. Close original-game fidelity gaps

- Restore surviving-frog-hit coverage in the strict combat gate: the traversal
  reaches its scrolling/retirement requirements but leaves the first frog
  behind before combat. Retain exact state and all encounter assertions, and
  reverify both firing phases, upgrade variants, death and survival continuations.
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
- Music and sound on both platforms, including Amiga conversion, mixing,
  timing and memory residency.
- Persistence, unlocks, achievements where applicable, settings and complete
  restart/continue behavior.

Implement shared behavior and connect both backends. A feature absent from both
ports does not count as parity with the original.

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
- Measure new actor/effect costs in dense scenes. Keep a portable reference
  when introducing selected 68000 assembly optimizations.
- Repeat low-overhead timing tapes and correctness gates after changes, with
  separate horizontal-shop and dense-encounter cases. Distinguish instrumentation
  overhead from ordinary gameplay cost.

References: [gameplay performance](gameplay-performance-current.md),
[level completion](level-clear-profile.md), [profiling](amiga-profiling.md).

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
