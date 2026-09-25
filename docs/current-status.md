# Current completion checklist

The Windows Steam build and its VM/resource evidence are the ultimate oracle.
SDL is a development comparison. Neither passing a CPU replay nor matching the
two ports proves original-game completion. The active parity goal is unfinished.

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
  cavern levels. Two-clear verification compares 432 completion frames,
  192 exit-field frames and both re-entries. This supersedes the old claim that
  the result room or all level advancement remains unimplemented. The fixture
  injects exits; it does not prove a natural full-level playthrough.
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

- Complete the seven scene boundaries. All seven now have named Amiga scene
  classes, but gameplay's owner currently controls only its Copper lifecycle.
  Terrain composition now belongs to `AmigaTerrainPainter`, with actors and
  raster storage already separate. Cache-refresh/display coordination and
  much transition orchestration remain in `DownwellAmiga.cpp`; SDL startup
  orchestration also needs consolidation.
- Meet the worst-case 20 ms budget. The latest style-navigation measurement is
  45.45 FPS over 180 updates, with a maximum three-field gap. Combined upgrade
  content/layout/simulation work and transition preparation also exceed budget.
  The current 600-update gameplay tape runs at 36.81 FPS, with a five-field
  maximum gap; instrumented generation dominates the largest sampled stall.
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
- Audio, complete persistence/unlocks/settings, later areas, remaining weapons/
  styles/enemies, bosses and ending flow are not complete. Distinguish features
  absent from both ports from actual Amiga-versus-implemented-SDL gaps.

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
