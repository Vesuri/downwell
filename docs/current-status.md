# Current completion checklist

The Windows Steam build and its VM/resource evidence are the ultimate oracle.
SDL is a development comparison. Neither passing a CPU replay nor matching the
two ports proves original-game completion. The active parity goal is unfinished.

## Immediate queue

- **Now:** integrate recovered catacomb skeleton/bone behavior into the shared
  runtime. Numeric Create/motion and lifecycle components are tested, but are
  not live actors. Next implement source-backed animation/event scheduling,
  instance ownership and collision dispatch, then connect both renderers.
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

### Latest verification: original alarm ordering

The decoded scheduler and shared gameplay's explicit enemy Alarm 0/1 slots
now dispatch immediately in global ascending alarm-index order. This corrects
the frog's Alarm 0-to-1 rearm countdown; see `runner-alarms.md`. Focused
cross-instance mutation/deletion tests and sanitizers pass. Host natural
routes still exit at 1,339/1,646 updates, followed by 211 upgrade and 120
re-entry updates. Seed 4's next-level shop approach remains 518 updates, but
return is now **42**, superseding the older host 43-update result below.
Evidence: `/tmp/downwell-alarm-order-natural.log`.

The bounded native CPU replay passes unchanged, including motion and bone
Create fixtures. The ordinary target build is restored with clean 68000
instruction/helper audits (`/tmp/downwell-alarm-order-native.log`). This is
not a rerun of the native rendered natural-shop gate; its older 43-update
expectation needs rechecking before that gate is cited for this revision.

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
  current 600-update gameplay tape runs at 35.09 FPS (855 PAL fields), with a
  three-field maximum gap (60 ms), improved from 34.36 FPS / four fields.
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
