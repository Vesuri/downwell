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
items in order. Each removes a structure or computation measured by the
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
at `b99c2f69` measures the contact tape at 3,821 fields. Quiet frames take
31.5 ms, combat 89 ms and time-field frames 67 ms. The worst frames (645,
701, 844) take about 180 ms (9 fields) each, for different reasons.

Rules for this section:
- Take items in the order below unless a repeated audit reorders them.
- Every change must remove a named structure or computation whose cost the
  audit measures: at least 3 ms per frame in a window, or 10 ms in a worst
  frame. Do not tune functions that cost under 1 ms in the audit. That
  includes rewriting loops, changing types or registers, unrolling, inline
  assembly, compiler flags and helper substitutions. The quiet frame is flat
  by construction; tuning its parts cannot reach 20 ms.
- Before a change, state the work removed and the expected saving from the
  audit. Afterwards, run the FPS tape (total fields and worst gap) and trace
  the affected windows. Discard the change if any of the three worst frames
  gets slower, whatever the average.
- Keep the replaced path compiled as the reference. Compare full RNG state,
  numeric instance IDs, creation and event order, positions and draw order on
  deterministic replays, and pixels where presentation changes.

### A. Worst frames (each about 180 ms)

1. **Contact damage-number direction (frame 645: 34.8 ms in one call).**
   `RunSession::contactDamageNumber` evaluates `RunnerPointDirection` through
   the software binary64 `portableAtan`. That is 14 divides and 14
   multiplies, 246,892 cycles. Replace the runtime series with precomputed
   tables:
   - Presentation precision is sufficient here (explicit user decision):
     preserve a bounded visual result, not floating-point rounding bits.
     Finish shared-default integration of `TABLE_CONTACT_ATAN=1`, including
     the SDL build, and rerun the combined full-tape/worst-frame gates after
     the remaining ownership changes. Keep the 514-byte table's
     1/256-degree bound and compiled unsupported-input/reference paths;
     do not restore a large exact-floating-point lookup.

   Target: under 1 ms. Verify every enumerated input, or the full probe set,
   against the oracle, plus the contact-tape replay digests. See
   [point direction](runner-point-direction.md).
2. **Health HUD on damage (frame 645: 25.7 ms).** `drawHealthNumberLayers`
   draws each digit as layered CPU assets (14.1 ms), and `redrawHealth`
   repaints the hearts and meter (11.7 ms): 23 asset draws at 9,000 cycles
   each.
   - Finish default integration of `RETAIN_HEALTH_HUD=1`: compare matched
     native worst-frame captures and the full FPS tape with the retained
     reference after the remaining ownership changes. Do not accept a local
     HUD saving as proof of no regression in the shipping configuration.
   - Preserve the offline glyph composition, retained frame/dividers, changed
     number regions and final-word publication while resolving that gate.

   Target: under 4 ms on a damage frame.
3. **Finish native-family birth preparation.** Bring box creation below
   2,000 cycles per birth without reviving per-instance registry nodes or
   changing Create timing. Prepare invariant construction data before the
   level rather than tuning individual sub-millisecond helpers. Preserve
   registration order for alarms,
     active-before-inactive depth order for cleanup, dynamic layer lifetime,
     and the distinction between destruction marking and retirement.
   - Prioritize A4–A6 before this remaining birth-preparation work, then return
     here before B. The fresh complete-frame trace measures box births at
     10.91 ms in frame 844, versus 26.08 ms for generic terrain painting and
     roughly 23 ms for field outlines; recharge still costs about 30 ms.
     These are larger architectural work-removal targets. The birth target
     is deferred, not removed or considered achieved.
   - Enable the accepted native family path in the normal build after its
     complete timing/fidelity gates; retain `NATIVE_BOXES=0 NATIVE_FIELDS=0`
     as the registered comparison path with population-only layers.

   Verify the target with a per-frame comparison against the
   registered reference, as `make verify-terrain-ownership` does for terrain.
4. **Terrain painting of new rows (frame 844: 28 CPU tile draws, 26.1 ms;
   3.8 ms per combat frame).** `AmigaTerrainPainter::paint` draws each tile
   through the generic `amigaRendererDrawAsset` at about 5,500–9,000 cycles.
   - Bring `TERRAIN_DIRECT`'s approximately 6.5 ms cache composition below the
     5 ms target, including fallback and staging coverage. Keep the scalar
     painter and native pixel oracles compiled. Measure the complete
     compose-and-publish cost as well as the painter; moving work outside a
     profiling scope is not a saving.
   - Prioritize the remaining large A5/A6 work before this smaller terrain gap,
     then return to A3/A4 before B. Do not chase sub-millisecond helpers to
     close it. The terrain target is deferred, not achieved or relaxed.
   - Finish normal-build integration and combined timing/fidelity gates for
     prepared cache composition and its batched fallback. Keep the options
     off by default until those gates pass; retain `TERRAIN_DIRECT=0
     TERRAIN_BATCH=0` as the original compositor comparison.
   - Preserve ordered transparent/opaque-black overlap, decorations,
     registered wall presentation, partial invalidation, ring wrapping and
     transactional fallback. Do not reintroduce an all-phase asset bank or
     the intermediate surface/readback for ordinary direct-cache runs.

   Preserve circular hardware scrolling, static backing, incremental
   restoration and the [terrain update contract](terrain-update-contract.md).
   Target: under 5 ms in frame 844.
5. **Ammo gauge (frame 645: 29.9 ms; frame 844: 14.3 ms; 4–7 ms per typical
   frame).** Its cost is the number of lines rewritten, not how each line is
   composed (see the audit's experiments).
   - Jiggle: a recharge shifts the gauge for four frames, and each shift
     recomposes and compares all 166 interior lines. Represent the column as
     constant-value line intervals (run middles, fill, dividers). Diff the
     previous and new interval lists, and write only lines whose final word
     changes.
     Do not rebuild general interval lists or solve ordered row overlaps on
     the 68000 every frame; the [native experiments](amiga-frame-cost.md#ammo-gauge-experiments)
     rule out those implementations. Likewise, replacing the middle with a
     per-line final-word publisher is insufficient, and rebuilding uniform
     eight-line certificates adds work without removing recharge composition.
     Move preparation off the critical frame or represent the changing column
     as a prepared transfer, with an explicit memory budget and the existing
     ordered compositor as fallback. Do not add another per-frame classifier.
     The host segment probe proves a fixed-capacity representation and measures
     its bank, but its interior-only native use fails the timing gate. Include
     the surrounding gauge/rim/label reconstruction in the replacement, and
     eliminate runtime run-selection work rather than tuning its inner loops.
     Do not ship the naive full-gauge bank either: the
     [complete-gauge probe](amiga-frame-cost.md#complete-gauge-representation-budget)
     verifies its pixels but budgets 268–435 KiB. Factor independent
     shell/number states away from column animation, and budget selection,
     transfer staging and changed-region publication together before a native
     replacement. Preserve the ordered renderer and unsupported-state fallback.
     Revise the experimental `AMMO_FACTORED=2` complete-column path before
     shipping it: recharge is still 10.6–11.3 ms, and the full-tape maximum
     field-gap gate remains open despite the instruction-trace saving. Remove
     the redundant low-plane animation pass using the generator's proven
     independence/sparse-state representation; preserve its red dividers, rim
     and fused label. Retain mode 1 as the comparison, qualify inherited row
     groups and changed artwork, then repeat native pixels, replay and both
     complete-frame and presentation-pacing gates. See the
     [factored experiment](amiga-frame-cost.md#factored-gauge-pixel-correct-experiment-pacing-gate-open).
     Take this remaining A5 work before
     A3/A4 and B, without treating the remaining A6 gates as complete. The HUD
     targets remain unchanged.
   - Finish normal-build integration of `AMMO_DAMAGE_RANGE=1` and
     `PREPARED_AMMO_DIVIDERS=1` after the combined HUD timing/fidelity gates.
     Retain full-scan and sprite-compositor comparisons, nonconsecutive
     animation handling, and larger-capacity/changed-artwork fallbacks.

   Target: under 8 ms for a recharge frame and under 4 ms for a shot frame.
6. **Time-field outlines (22.5 ms preparation and 5.6 ms drawing per frame
   while fields are visible; 23.3 ms in frame 844).** The tape's field is one
   pulsing 24–26 px circle whose raster changes almost every frame.
   `amigaFieldWords` costs about 1,000 cycles per outline word. Prepare each
   small-circle raster's outline image once and blit it with saved backing,
   without changing Draw RNG consumption or source geometry. Target: under
   5 ms per field frame.
   - Reuse the platform Bitmap/blitter backend and retain the sparse overlay
     as the compiled oracle/fallback. Verify mask-only background restoration,
     actor/laser ordering, overlapping fields, clipping, circular-buffer wraps,
     repeated prepare calls, and switching between optimized and fallback paths.
   - Finish `PREPARED_FIELDS=1` with `PREPARED_FIELDS_BACKING=5`: bring the
     resident large-field bands and noncentral column fallback below the
     under-5-ms target, measure
     wrapped and overlapping small-field workloads against that target, then
     complete normal-build integration and the combined timing/fidelity gates. Do not
     count the single-circle improvement as completion of A6 or replace the
     native pixel checks with host-only copies. Use
     `PREPARED_FIELDS_BACKING=4` for separate prepared restore/paint comparison,
     `PREPARED_FIELDS_BACKING=3` for generic resident-band transfer comparison,
     `PREPARED_FIELDS_BACKING=2` for pristine-small/column-large comparison,
     `PREPARED_FIELDS_BACKING=1` for pristine-small/sparse-large comparison,
     `PREPARED_FIELDS_BACKING=0` for the saved-background comparison and
     `PROFILE_FIELD_REFERENCE=1` for the sparse comparison; selectors change
     initialized data, not executable code.
     Further large-field work must eliminate repeated coverage restoration
     and publication, not reintroduce runtime prepared-row expansion:
     repeating that traversal
     for restoration and drawing made the level-exit workload slower. See
     the [large-field experiment](amiga-frame-cost.md#large-field-traversal-experiment).
     Do not pursue another per-word CPU layout rewrite: the vertical-run
     experiment did not justify its memory or improve complete-frame pacing.
     Evaluate removing a whole restoration/publication pass or prepared DMA
     composition, with explicit mixed-field ordering, dirty-row handling and
     Chip-bank/staging ownership. Use the
     [representation audit](amiga-frame-cost.md#large-field-representation-limits)
     to budget bitmap residency and blit count before expanding assets.
     Whole-row retirement alone is also ruled out by the
     [paired native experiment](amiga-frame-cost.md#large-field-row-restoration-experiment):
     exact offline spans remove scheduling scans, but copying terrain rows
     and repainting the outline leaves complete frames effectively unchanged.
     Account for source staging, old-mask restoration and drawing in the same
     budget; moving work into row copying is not sufficient. For centres
     182–198 the resident bands avoid per-word drawing and prepare horizontal
     transfer geometry once, but combined preparation/drawing still costs
     roughly 12 ms in the exit window. Eliminate
     another measured composition pass, rather than
     tuning sub-millisecond helpers. Preserve the column fallback until broader
     placements have equivalent pixel and timing coverage, and verify full
     scene/reentry memory behavior with the remaining 11,512 Chip bytes.
     Chained adjacent-raster CPU XOR packets are also ruled out by the
     [paired native timing](amiga-frame-cost.md#rejected-adjacent-raster-xor-packets):
     they reduce pixel coverage but worsen complete frames. Any differential
     replacement must avoid repeated CPU packet traversal and budget its
     representation up front; the all-pairs word table exceeds 2 MiB.
     Prepared whole-operation transfer lists also lost in
     [paired native timing](amiga-frame-cost.md#rejected-prepared-transfer-list-batch).
     Prioritize eliminating a transfer pass over another queue-scheduling
     variant. Build on the opt-in fused terrain/field pass elimination
     (`PREPARED_FIELDS_BACKING=5`). Keep the
     full-rectangle ownership contract, departing-extent retirement, dirty-row
     sync, mixed-field order, laser/actor order and fallback transitions.
     The [native fusion measurements](amiga-frame-cost.md#fused-terrainlarge-field-composition-experiment)
     do not yet meet the under-5-ms field target; reduce the remaining
     composition work before closing A6. Do not overwrite retained content merely
     because it lies inside an arc rectangle.
   - Explicitly partition the shared scene bank: terrain batching already
     uses its first 44,304 bytes. Field images/backing must not overlap that
     scratch or survive a front-bank reload without rebinding.

After A1–A6 the generation bursts remain at roughly 100 ms. That is about
26 ms of synchronous Well512 draws, 22 ms of run creation (7.1 ms of it
`publishSpan`) and pattern emission. Moving draws to other frames changes RNG
interleaving and is not an option. Re-audit before choosing the next
generation item.

### B. Per-frame runner walks (every gameplay frame)

The per-actor runner walks cost about 7 ms in quiet frames, about 13 ms with
enemies in view and about 14 ms in combat. Together with the gauge strips
they are the gap between the 31.5 ms quiet frame and 20 ms.

1. **Draw collection** (2.7 ms quiet, 6.3 combat, 10.7 in frame 701)
   rebuilds the actor list through the generic layer walk after every
   ownership change. Maintain the actor Draw list incrementally, ordered by
   layer, depth and element order.
   Finish normal-build integration of `LOCAL_DRAW_CAPTURE=2` after combined
   shipping-configuration timing/fidelity gates. Retain mode 1 as the global
   capture reference, the separate layer order/cutoff guard, and original
   registration/alarm revisions. External families gaining actor Draw must
   join the Draw invalidation domain. This exclusion does not replace
   incremental maintenance for genuine list changes.
2. **Captured alarm dispatch** (1.2 ms quiet, 5.0 enemies) walks every
   captured enemy and effect for each alarm index. Count each family's
   alarms down in its records in captured order, dispatch only zero
   crossings, and retire unrearmed zeros to -1. Native ownership already
   skips box groups while no live box has Alarm 0 set.
3. **Animation walk** (2.1 ms quiet, 4.1 combat) decodes and revalidates each
   visited actor. Run per-family loops over compact records where no
   Animation End handler has cross-instance effects, keeping room order
   where one does.
4. **Activation reconciliation and validation** (1.1 ms quiet). Validate
   handles and owner state at birth, retirement and room entry, not on every
   per-frame visit. Keep a sticky failure flag checked at phase boundaries.

### C. Combat composition (combat window, 88.8 ms)

After B, measure combat again. The audit's composition costs are: actor draws
10.6 ms (damage text 5.5, five or six CPU asset draws at 7,400 cycles);
currency/smoke 9.7 ms for three stamps at 13,872 cycles each; terrain ring
sync 6.4 ms; effect-surface publication 2.5 ms.
- Actors: hardware sprites or blitter bobs instead of CPU planar stamping.
- Currency/smoke: blitter zero-flag tests for final-alpha overlap instead of
  per-row bounds and generic queue enqueues.
- Damage text: prepared glyph words.

Verify the mixed DMA/planar frame against original captures, preserving
controller priority.

### D. Keep native ownership sufficient

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

### E. Coverage of the measurement

- The contact tape has no horizontal shop, no dense encounter and no
  garden-to-well transition. Add tapes for each before claiming the budget,
  and repeat the audit after every A or B item lands.

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
