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
items in order: generation bursts, combat
composition and the per-frame runner walks. Each removes a structure or
computation measured by the [frame-cost audit](amiga-frame-cost.md); tuning
small functions and compiler flags is not the remedy. Cover gameplay entry
from the well as well as steady gameplay. Then finish live
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
measures `22df6961` on the contact tape: 2,980 PAL fields. Quiet frames take
26.3 ms, combat 67.4 ms and time-field frames 35.4 ms. Budget examples are
701 (106.7 ms: generation on top of combat), 844 (92.1 ms: generation),
645 (94.0 ms: contact) and 660 (106.3 ms: combat). Menu
scenes (`profile.sh transition`) peak at two fields; measure the cold gameplay
handoff separately with `profile.sh entry`.

### How to work this section

The previous round spent most of its commits on one parked item (the ammo
gauge) and left ten opt-in flags behind. Do not repeat that.

- Work the items below in order. Each names the structure to remove, its
  measured cost and a target. Implement the whole structural change in one
  attempt; do not sweep variants, modes or parameters.
- Do not touch code that costs under 1 ms in the windows it runs in. That
  includes loop rewrites, type/register changes, unrolling, inline assembly,
  compiler flags and helper substitutions. If a profile shows nothing of 3 ms
  or more in the target, the item is done: move on.
- Before starting, write in the commit message the work removed and the
  expected saving from the numbers here. Afterwards trace only the affected
  windows (start a capture one frame early: 644, 699, 843) and run the FPS
  tape. Repeat the full audit only after each lettered group lands.
- Gate: reject the change only if a frame that runs the changed code gets
  slower. Untouched frames move by up to about 0.5 ms between builds; that is
  noise. The tape's worst gap is quantized to whole fields: report it, never
  gate on it.
- An accepted change is the default in the same commit. Keep the replaced path
  only as a reference selector for oracles. Do not add experiment flags, mode
  numbers or parallel candidate implementations to the tree.
- Two rejected attempts at an item end it: record one row each in the audit's
  [rejected approaches](amiga-frame-cost.md#rejected-approaches) table (commit
  and patch), then take the next item. Measurements belong in commit messages.
  Do not add narrative or per-experiment results to this file or the audit.
- Verify each change with the full RNG state, numeric instance IDs, creation
  and event order, positions and draw order on deterministic replays
  (`make verify-cross-replay`, `make verify-terrain-ownership`, the per-frame
  ownership digests), plus the native pixel oracles (`profile.sh terrain`,
  `profile.sh fields`, `profile.sh ammo`) when presentation changes.
- Delete your own traces under `tmp/trace/` once their numbers are in a commit;
  each 100-field capture is about 450 MB and the disk is nearly full. For
  several queries on one capture, export `--collapsed` stacks once instead of
  re-running `--focus`.

### A. Generation bursts (frames 701 and 844)

`generateLiveTerrain` costs 42.81 ms at 701 and 41.05 ms at 844. A whole
builder pattern is emitted synchronously, as in the original; the draws and
creations must stay in that order. Further changes must remove remaining
publication or Create machinery, not reorder those effects.

**Remaining budget shortfall, not another encoding sweep.** Generation at
701 still exceeds the 30 ms target. Event cells take 20.15 ms and prepared
terrain publication 13.92 ms (nested within generation). At 844 these are
26.35 and 6.87 ms. The next generation attempt must eliminate a named
remaining structure, not retune the prepared row program or its helpers.

   - Further actor/time-field publication work must replace shared ownership
     as a whole; individual lifetime helpers are below the work threshold.
     Do not retune the generated-box command path. The audit's event
     Create costs are 20.15 ms at 701 and 26.35 ms at 844. One enemy
     configuration at 844 costs 3.59 ms, including about 3.07 ms in lifecycle
     birth. Establish the removable structure and saving before changing it;
     charge mutation costs in the affected-frame gate. Do not tune `nextU32`,
     constructors or small choice helpers individually.
   - Fixed-terrain publication remains a shortfall, but both immutable-row
     owner attempts are closed (see the rejection table). Do not retry
     copy-on-write rows, sparse overrides, prepared row encodings, copy loops
     or individual publication helpers. This owner item has exhausted its
     two attempts; proceed to the other shared ownership/composition work.
     No deferred source-visible Create or later terrain exposure is permitted.
   - Preserve source-ordered terrain segments separated by event cells: never
     expose later terrain to an earlier Create. Retain the scalar reference
     for registered terrain, instance IDs, and capacity/failure prefixes.
   - Keep coincident-wall counts, landing candidates, platform record order,
     dirty rows, RNG interleaving and the optional pillar tail exact.
   - Recheck full-state/ownership replay and native terrain pixel oracles.
     `REFERENCE_PATTERNS=1` selects the retained cell interpreter.
   - Native breakables cost 2,156 cycles per birth, still above 1,500. At 844,
     input/slot/grid publication is 2.59 ms and chronology/layer metadata
     1.26 ms. Do not tune individual writes or copy loops for this threshold.
     Keep `REFERENCE_BREAKABLES=1` as the oracle, including room re-entry and
     failure prefixes. See the
     [allocation contract](terrain-update-contract.md#native-breakable-allocation).

### B. Combat composition shortfall

Currency/smoke preparation still costs 9.72 ms in combat; Draw collection
costs 5.03 ms, row synchronization 4.32 ms and effect publication 2.72 ms.
Review the complete producer/compositor/consumer path, preserving source
painter order, opaque-black replacement and shade subtraction. The two
rejected effect approaches remain closed: a new proposal must remove their
exact-query or CPU source/mask-extraction cost, not repeat either layout.
The delta-register stamp batch is also closed: it shifts work into queued
dispatch and regresses the first affected frame. Do not sweep its scheduling.
Offline shifted CPU stamps into the retained raster are closed too; do not
retune their word interpreter, layout or CPU/DMA selection.
Keep per-producer/presentation state and final pixels as oracles. Do not
reopen small sprite assignment or gem-position helpers.

### C. Quiet-frame and alarm shortfalls

Alarm capture/dispatch remains a budget shortfall. Its two structural attempts
are closed in the [rejection table](amiga-frame-cost.md#rejected-approaches);
do not retry per-frame capture preparation. Any later proposal must eliminate
the rebuild itself, including cold birth/retirement frames, rather than change
its temporary array layout.

Target: quiet frames under 22 ms; current mean is 26.3 ms. Player and walking
enemy motion cost about 5.6 ms together, but each is under 3 ms in this window.
Leave their individual helpers alone. A quiet-frame proposal needs a shared
work structure to remove, measured across its callers, not a collection of
unrelated sub-millisecond edits. Alarm dispatch is 1.02 ms quiet, 2.07 ms in
combat and 4.39 ms at 645.

### D. Parked until A–C are done

- Ammo gauge: 11.2 ms at 645, 1.5–4 ms in typical frames. Every runtime
  classifier is ruled out (rejected-approaches table). If reopened, start from
  the staged-transfer patch listed there, measured against the factored
  default.
- Contact damage number at 645: 8.1 ms, of which `RunnerPointDirection::evaluate`
  is 4.3 ms (binary64 arithmetic around the 0.9 ms table atan).
- Health HUD at 645 (4.0 ms); terrain painting at 844 (6.4 ms against 5 ms).

### E. Keep native ownership sufficient

- Native ownership rejects every collision query and wall End Step. When an
  original query or wall event enters native gameplay, answer it from native
  state (the grid for terrain selectors, family records for actors) with the
  same first-hit identity and order, verified against the registered
  reference. Do not re-enable per-instance spatial publication for it.
- Actors at the wall depth (100) are rejected, because an inactive wall ends
  that layer's Draw. Supporting one needs the layer's wall-order cutoff.

### F. Coverage

Add a dense-encounter tape before claiming the budget. Include the natural
two-level horizontal shop tape (`profile.sh shop`), not only the contact
tape, in subsequent audits. Repeat the audit after each lettered group lands.

### G. Other scenes

The earlier style-reveal-only tape peaks at two fields (splash 34.7 ms,
garden 40.5 ms, style
34.5 ms, instrumented). Reduce selection layout/content and transition
preparation costs, keeping static layers and pointer-driven movement. Reduce
the initial selection-canvas and offer-bank preparation stalls. Measure asset
staging and Chip/Fast residency separately from steady-state navigation.
Preserve the original heading, icon and carousel animation.

Remove the synchronous cold gameplay handoff exposed by `profile.sh entry`.
The last style frame owns that work; the first gameplay frame must not be
discarded either. Separate field binding, asset staging, raster/backing setup
and source-visible world creation before choosing what can be prepared early.
Do not move RNG draws or source-visible Create effects ahead of confirmation.

Reduce remaining mutable room-layer reset/publication at level reentry,
measured with the natural shop tape (`PROBES=1 bash amiga/profile.sh shop`).
Keep immutable metadata admission and the no-survivor proof outside repeated
placeholder searches. A further attempt must remove a measured structure,
not tune reset loops. Preserve serialized placeholders, source IDs, background
initialization order, failure prefixes and later actor attachment; retain
the generic layer lifecycle as the oracle (`REFERENCE_ROOM_PREPARE=1`).

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
