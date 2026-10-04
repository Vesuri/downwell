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

Next: recover the A500 gameplay frame budget (section 5), taking its ranked
items in order: generation bursts, then the per-frame runner walks, then
combat composition. Each removes a structure or computation measured by the
[frame-cost audit](amiga-frame-cost.md); tuning small functions and compiler
flags is not the remedy. Cover the garden-to-well transition as well as
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
Average FPS is not the criterion. The [frame-cost audit](amiga-frame-cost.md)
at `b815af63` measures the contact tape at 3,252 PAL fields. Quiet frames take
28.4 ms, combat 85 ms, and time-field frames 37 ms. The worst frames are 701
(172 ms, generation), 844 (121 ms, generation) and 645 (101 ms, contact).

Rules for this section:
- Take items in the order below unless a repeated audit reorders them.
- Every change must remove a named structure or computation whose cost the
  audit measures: at least 3 ms per frame in a window, or 10 ms in a worst
  frame. Do not tune functions that cost under 1 ms. That includes rewriting
  loops, changing types or registers, unrolling, inline assembly, compiler
  flags and helper substitutions.
- Before a change, state the work removed and the expected saving from the
  audit. Afterwards, trace the affected windows, starting each capture one
  frame early (644, 699 and 843 for the worst frames), and run the FPS tape.
- Reject the change if a frame that runs the changed code gets slower.
  Frames that do not run it move by up to about 0.5 ms between builds; that is
  noise, not a regression. The FPS tape's worst gap is quantized to whole
  fields: report it, but do not use it as the gate.
- An accepted change is enabled in the normal build in the same commit. Keep
  the replaced path compiled as a reference selector for oracles, never as an
  opt-in experiment. Compare full RNG state, numeric instance IDs, creation
  and event order, positions and draw order on deterministic replays, and
  pixels where presentation changes.
- After two rejected attempts at one item, move to the next item. Record each
  rejected approach as one row in the audit's
  [rejected approaches](amiga-frame-cost.md#rejected-approaches) table, with
  its commit and patch. Measurements and history belong in commit messages,
  not in this queue.

### A. Generation bursts (frames 701 and 844)

`generateLiveTerrain` takes 93 ms at 701 and 66 ms at 844. Moving Well512 draws
to other frames changes RNG interleaving and is not an option.

1. **Object creation.** `AmigaGameplayGeneration::create` costs 2,568 cycles
   per object at 701 (62 objects, 22.4 ms) and 4,460 at 844 (44 objects,
   27.6 ms). `appendBreakables` still costs 6,034 cycles per breakable.
   Prepare each generated object type's invariant construction data (defaults,
   alarms, layer, depth, sprite bounds, family record template) before the
   level. A birth then becomes a record copy plus position and registration,
   in the original creation order. Target: under 1,500 cycles per object and
   under 2,000 per breakable, about 15 ms saved at 701 and 20 ms at 844.
2. **Run creation.** 23 `createRun` calls take 22.1 ms at 701 (6,828 cycles
   each), including 7.2 ms of `AmigaTerrainRunIndex::publishSpan`; 7.1 ms of
   that is RNG advance and stays. Publish a pattern's spans once per pattern,
   not once per run, and build run records from prepared pattern data.
   Target: under 2,500 cycles per run excluding RNG, about 8 ms saved.
3. **Pattern emission.** 11 `AreaOneBuilder::emitObject` calls take 17 ms at
   701 and 14 take 30 ms at 844 (11,000–15,000 cycles each, including the
   creations above). After A1 and A2, trace again with
   `--focus=AreaOneBuilder::emitObject` and remove the largest remaining
   per-object structure; do not tune the builder's arithmetic.

Verify every item with per-frame digest comparisons against the registered
reference, as `make verify-terrain-ownership` does for terrain, plus the
cross-CPU replay and the native field/terrain oracles.

### B. Per-frame runner walks (every gameplay frame)

The quiet frame takes 28.4 ms, and no function takes more than a tenth of it.
Its gap to 20 ms is the per-actor walks.

1. **Room visits through the collision owner.** The
   `GameplayCollisionOwner::attach` visit hooks cost about 6.5 ms quiet and
   15 ms at 645. In the quiet window they are the animation walk
   (`visitAnimationRetainedChecked`, 1.1 ms), room-layer visits (1.0 ms),
   activation reconciliation (0.6 ms), depth servicing (0.3 ms) and per-visit
   validation (0.2 ms), each entered once per actor per phase. Run per-family
   loops over compact records where no handler has cross-instance effects,
   keeping room order where one does. Reconcile activation and service depths
   at birth, retirement and room entry, not on every visit, and replace
   per-visit validation with a sticky failure flag checked at phase
   boundaries.
2. **Draw collection** (2.7 ms quiet, 6.6 combat, 8.0 at 844) walks the room
   layers after ownership changes. Maintain the actor Draw list incrementally,
   ordered by layer, depth and element order. Local Draw capture already
   skips the walk when nothing changed; this item covers genuine changes.
3. **Captured alarm dispatch** (1.2 ms quiet, 2.4 combat) walks every captured
   enemy and effect for each alarm index. Count each family's alarms down in
   its records in captured order, dispatch only zero crossings, and retire
   unrearmed zeros to -1.

### C. Combat composition (85.4 ms)

1. **Actor drawing** (10.7 ms): CPU planar stamping of actors that did not
   get a hardware sprite. All eight sprite channels are usable since
   `b815af63`; assign sprites before falling back to bobs, and blit the bobs
   that remain from prepared masks.
2. **Currency/smoke composition** (9.6 ms; three `stampPixel` calls at 14,038
   cycles each): use blitter zero-flag tests for final-alpha overlap instead of
   per-row bounds and generic queue enqueues.
3. **Fixed-point conversion on access** (about 10 ms of 32-bit division,
   modulo and multiply helpers; `GemMotion::q8`, `rounded` and `pixel` cause
   about 4 ms). Store positions in the representation their readers need, and
   convert once per step, not per access.

Verify the mixed DMA/planar frame against original captures, preserving
controller priority.

### D. HUD and terrain residuals (only after A–C)

- The ammo gauge costs 11.2 ms at 645 and 1.5–4 ms in typical frames; its
  targets are under 8 ms on a recharge frame and under 4 ms on a shot frame.
  The rejected-approaches table rules out every runtime classifier. If the
  gauge is reopened, start from the staged-transfer patch listed there and
  measure it against the factored default.
- Terrain painting costs 6.4 ms at 844 against a 5 ms target.

### E. Keep native ownership sufficient

- Native ownership rejects every collision query and wall End Step. When an
  original query or wall event enters native gameplay, answer it from native
  state (the grid for terrain selectors, family records for actors) with the
  same first-hit identity and order, verified against the registered
  reference. Do not re-enable per-instance spatial publication for it.
- Actors at the wall depth (100) are rejected, because an inactive wall ends
  that layer's Draw. Supporting one needs the layer's wall-order cutoff.
- The `TERRAIN_RUN_STORE` representation optimises registered terrain, which
  native gameplay no longer keeps. Remove it unless the registered reference
  itself needs it.

### F. Coverage of the measurement

- Three native fixtures time out before their checks run. Restore them before
  relying on them:
  - `verify-amiga-gameplay-active-frame` never reaches its awake-enemy frame
    (already at `d0072b4b`).
  - `verify-amiga-natural-route` stalls after its reentry at tick 3060
    (already at `7452dfe0`).
  - `profile.sh fields-clear` never reaches its profile marker (already at
    `7452dfe0`).
- The contact tape has no horizontal shop, no dense encounter and no
  garden-to-well transition. Add tapes for each before claiming the budget,
  and repeat the audit after every A or B item lands.

### G. Other scenes

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
- The shipped build leaves 11,512 bytes of Chip RAM after startup; the
  resident large-field arcs take 56,376 of the scene bank's bytes. Any new
  Chip residency must come out of that headroom or replace an existing owner.
- The gameplay, HUD, results, shop and clear packs and the cover masks are
  still linked into the executable, so ordinary memory holds them beside any
  Chip copy. Load them as per-scene files like the boot and front banks, one
  whole-file read per scene, and keep the startup failure codes meaningful.
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
