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
items in order: generation bursts, the contact damage number, combat
composition, HUD text and quiet frames. Each removes a structure or
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

Target <=20 ms frames during active gameplay and smooth style/upgrade navigation.
Scene handoffs are not gameplay frames: judge them by total transition latency
and visible stalls, not a 20 ms per-frame limit. Average gameplay FPS alone is
not the criterion. The [frame-cost audit](amiga-frame-cost.md)
measures `7deed269` on the contact tape: 2,792 PAL fields. Quiet frames take
26.5 ms, combat 53.1 ms and time-field frames 35.8 ms. Budget frames are
701 (91.1 ms: generation on top of combat), 646 (76.4 ms: the contact damage
number's first step), 645 (89.6 ms: contact), 660 (53.9 ms: gem counter and
streak change) and 844 (87.0 ms: generation). Menu scenes
(`profile.sh transition`) peak at two fields; measure the cold gameplay
handoff separately with `profile.sh entry`.

### How to work this section

An earlier round spent most of its commits on one parked item (the ammo
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
  windows (start a capture one frame early: 644, 659, 699, 843) and run the
  FPS tape. Repeat the full audit only after each lettered group lands.
- Gate on the frames the budget is about, not on every frame. Accept when
  the targeted cost falls, no budget frame (645, 646, 660, 701, 844) or window
  worst frame gets slower by more than 0.5 ms, and no window mean the change
  runs in gets worse. Another single frame may rise by up to about 1 ms when
  the change saves several times that elsewhere; report it in the commit.
  Untouched frames move by up to about 0.5 ms between builds; that is noise.
  The tape's worst gap is quantized to whole fields: report it, never gate on
  it. Do not reject an 11 ms saving for a 0.6 ms move in a 70 ms frame.
- For scene handoffs, measure the complete transition through the first
  interactive frame, including preparation charged to earlier frames. Small
  differences such as 8 ms or one or two PAL fields are not rejection grounds
  by themselves. Accept a substantial overall improvement even when VBlank
  waiting shifts into the following frame. Preserve smooth ongoing animation
  and gameplay; distinguish added computation from redistributed waiting.
  Saving estimates and attempt limits guide prioritization, not automatic
  rejection of useful changes. Reassess candidates blocked solely by an
  obsolete timing gate without starting another parameter/variant sweep.
- Quote frame numbers from the report's per-frame table for the frame you
  name, and re-check a surprising number with a short capture of that frame
  before committing it. The `36d8a0f2` message gives 660 as 84.5 ms; the
  frame measures 90.2 ms (a saving of about 16 ms, not 22).
- An accepted change is the default in the same commit. Keep the replaced path
  only as a reference selector for oracles. Do not add experiment flags, mode
  numbers or parallel candidate implementations to the tree.
- Two rejected attempts at an item end it: record one row each in the audit's
  [rejected approaches](amiga-frame-cost.md#rejected-approaches) table (commit,
  and the removed code as a patch in `docs/rejected/`), then take the next
  item. Measurements belong in commit messages. Do not add narrative or
  per-experiment results to this file or the audit.
- Verify each change with the full RNG state, numeric instance IDs, creation
  and event order, positions and draw order on deterministic replays
  (`make verify-cross-replay`, `make verify-terrain-ownership`, the per-frame
  ownership digests, and `profile.sh overflow-state` for that encounter), plus
  the native pixel oracles (`profile.sh terrain`,
  `profile.sh fields`, `profile.sh ammo`, native frame dumps against the
  reference selector) when presentation changes.
- Delete your own traces under `tmp/trace/` once their numbers are in a commit;
  each 100-field capture is about 500 MB. For several queries on one capture,
  export `--collapsed` stacks once instead of re-running `--focus`.

### A. Generation bursts (frames 701 and 844)

`generateLiveTerrain` costs 40.84 ms at 701 and 35.62 ms at 844. A whole
builder pattern is emitted synchronously, as in the original; the draws and
creations must stay in that order. Further changes must remove remaining
publication or Create machinery, not reorder those effects.

**Remaining budget shortfall, not another encoding sweep.** Generation at
701 still exceeds the 30 ms target. Event cells take 17.00 ms and prepared
terrain publication 13.92 ms (nested within generation). At 844 these are
19.81 and 6.87 ms. The next generation attempt must eliminate a named
remaining structure, not retune the prepared row program or its helpers.

   - Further actor/time-field publication work must replace shared ownership
     as a whole; individual lifetime helpers are below the work threshold.
     Do not retune the generated-box command path. One enemy configuration
     at 844 costs 3.76 ms, including 3.23 ms in lifecycle publication.
     Establish the removable structure and saving
     before changing it; charge mutation costs in the affected-frame gate.
     Do not tune `nextU32`, constructors or small choice helpers individually.
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
   - Native breakables cost about 2,058 cycles per creation at 844 (thirteen, 3.77
     ms), still above 1,500. Do not tune individual writes or copy loops for
     this threshold. Keep `REFERENCE_BREAKABLES=1` as the oracle, including
     room re-entry and failure prefixes. See the
     [allocation contract](terrain-update-contract.md#native-breakable-allocation).

### B. Contact damage number (frames 645 and 646)

The remaining contact-pair damage-number work is 10.84 ms, above its
10 ms target. At 645, creation costs 5.29 ms. The retained post-difference
direction calculation is below 3 ms. At 646, first Step costs 5.55 ms, including
3.39 ms in the certified motion decoder and wrappers. Subsequent motion and
damping cost about 2.1 ms per frame; leave their individual helpers alone.
Preserve bit-identical direction, speed and positions and keep frame 646
under 80 ms while removing remaining threshold-sized computations.

- Keep the [finite-domain certificate](runner-point-direction.md#lossless-contact-motion-representation)
  and portable cosine oracle. `REFERENCE_CONTACT_MOTION=1` selects the retained
  first-Step program. Mutated, arbitrary and fallback directions must not be
  silently admitted to the certified domain.
- A further change must remove a named remaining computation across the
  contact adapter/first Step, not tune sub-millisecond decoder or arithmetic
  helpers. Do not substitute trigonometric identities or reduced precision.
- A faster generic binary64 multiply is a helper substitution, not this item.
- Keep exact Q8 coordinate admission and the full-wrapper fallback
  (`REFERENCE_CONTACT_COORDINATES=1`). Do not tune the sub-millisecond
  coordinate adapter or the now-under-3-ms direction calculation.

### C. Combat composition shortfall

Remaining effect work is the raster fallback for mixed producers, actual
shade subtraction, entry cover and sprite-channel overflow. Measure those
cases separately from eligible gem-only frames before another compositor
attempt. Use the effect-coverage census described in the profiling procedure
to establish that the chosen window actually exercises each proposed path;
producer-phase timings alone do not prove smoke/debris or raster coverage.
Use the seed-8 route-4 tape for shade subtraction and channel overflow. Preserve
the topmost unshaded hardware suffix, shaded raster prefix, and
controller/ordinary-actor boundary when changing
the retained raster. Its remaining nine-gem-window composition costs 3.22 ms;
do not tune sub-millisecond stamp helpers. Mixed producers and entry cover still
need their own native workload and pixel evidence.
Current combat draw collection costs 4.48 ms and row synchronization 1.50 ms.
The supplied baseline's raster preparation/publication costs no longer describe
this hardware-gem-only workload; do not use them to justify fallback changes.
Review the complete producer/compositor/consumer path, preserving source
painter order, opaque-black replacement and shade subtraction. The two
rejected effect approaches remain closed: a new proposal must remove their
exact-query or CPU source/mask-extraction cost, not repeat either layout.
The delta-register stamp batch is also closed: it shifts work into queued
dispatch and regresses the first affected frame. Do not sweep its scheduling.
Offline shifted CPU stamps into the retained raster are closed too; do not
retune their word interpreter, layout or CPU/DMA selection.
Row-debt representation has exhausted its two attempts: disjoint ranges and
shared word coverage with cached field/laser footprints are closed. Do not
retune their range layout, bit scans or footprint maintenance. Proceed to the
remaining producer/compositor/consumer structures, not another debt encoding.
Keep per-producer/presentation state and final pixels as oracles. Do not
reopen small sprite assignment or gem-position helpers.
Keep `REFERENCE_EFFECT_COMPOSITION=1` as the separate-plane oracle. Further
composition work must remove a threshold-sized structure, not tune stamp
lookup, row geometry or the sub-millisecond bitmap setup. Preserve the
retained alpha raster's exact queries and uncommon-mask fallback.

### E. Quiet-frame shortfall

Pursue shared quiet-frame work, not another enemy alarm-order variant. Do not
retune roster insertion, pinning, snapshot caching, merge loops or negative
counter dispatch. The rejected borrowed Step snapshot and generic temporary
alarm-array layouts remain closed. Any remaining effect-alarm proposal must
first meet the work threshold in a workload that actually exercises it.

Target: quiet frames under 22 ms; current mean is 26.5 ms. Player and walking
enemy motion cost about 5.6 ms together, but each is under 3 ms in this window.
Leave their individual helpers alone. A quiet-frame proposal needs a shared
work structure to remove, measured across its callers, not a collection of
unrelated sub-millisecond edits.

### F. Parked until A–E are done

- Ammo gauge: 11.2 ms at 645 and 646, 1.5–4 ms in typical frames. Every
  runtime classifier is ruled out (rejected-approaches table). If reopened,
  start from the staged-transfer patch listed there, measured against the
  factored default.
- Health HUD at 645 (4.05 ms); terrain painting at 844 (6.37 ms against 5 ms).

### G. Keep native ownership sufficient

- Native ownership rejects every collision query and wall End Step. When an
  original query or wall event enters native gameplay, answer it from native
  state (the grid for terrain selectors, family records for actors) with the
  same first-hit identity and order, verified against the registered
  reference. Do not re-enable per-instance spatial publication for it.
- Actors at the wall depth (100) are rejected, because an inactive wall ends
  that layer's Draw. Supporting one needs the layer's wall-order cutoff.

### H. Coverage

Add a dense-encounter tape before claiming the budget. Include the natural
two-level horizontal shop tape (`profile.sh shop`), not only the contact
tape, in subsequent audits. Repeat the audit after each lettered group lands.
Particle-heavy natural coverage depends on the destruction/effect integration
in §1: manually emitted smoke/debris fixtures or a bullet-only census do not
prove that coverage. Keep the input-only seeded search's source/runtime parity
checks enabled when selecting a tape.

### I. Other scenes

The earlier style-reveal-only tape peaks at two fields (splash 34.7 ms,
garden 40.5 ms, style
34.5 ms, instrumented). Reduce upgrade-selection layout/content and transition
preparation costs, keeping static layers and pointer-driven movement. Reduce
the first border-canvas, animated overlay and offer-bank preparation stalls.
Measure asset staging and Chip/Fast residency separately from steady-state navigation.
Preserve the original heading, icon and carousel animation.

Reduce the overall cold gameplay handoff exposed by `profile.sh entry`.
Measure the last style frame and first gameplay frame together under the
transition gate above; do not discard either. Remaining synchronous work includes
initial terrain raster and backing/HUD
setup. Move immutable preparation out of confirmation without overwriting
the garden/style display owners or introducing another long frame earlier.
Do not move RNG draws or source-visible Create effects ahead of confirmation.
Do not retry bounded clean-style raster preparation or sweep row budgets,
scheduling, dictionary gather loops or encodings. Proceed to remaining
backing/HUD setup and room publication structures; do not count relocated
copy work as eliminated work.

Reduce remaining mutable room-layer reset/publication at level reentry,
measured with the natural shop tape (`PROBES=1 bash amiga/profile.sh shop`).
Keep immutable metadata admission and the no-survivor proof outside repeated
placeholder searches. A further attempt must remove a measured structure,
not tune reset loops. Preserve serialized placeholders, source IDs, background
initialization order, failure prefixes and later actor attachment; retain
the generic layer lifecycle as the oracle (`REFERENCE_ROOM_PREPARE=1`).
Mutation-owned occupancy row extents are rejected: tracking during room
publication consumes most of the removed reset work. Do not tune their range
updates or partial-clear loops; pursue a different ownership structure.

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

- Restore the damage-number shared-world capacity-failure/reset regression:
  `StartupRuntime::reset` must recover a session after number capacity is
  exhausted. The committed reference also fails this assertion; do not weaken
  the failure/reset expectation to make the test pass.
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
