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
  Terrain/actor presentation and much transition orchestration remain in
  `DownwellAmiga.cpp`; SDL startup orchestration also needs consolidation.
- Meet the worst-case 20 ms budget. The latest style-navigation measurement is
  45.45 FPS over 180 updates, with a maximum three-field gap. Combined upgrade
  content/layout/simulation work and transition preparation also exceed budget.
  Keep hardware scrolling, retained terrain and incremental damage intact.
- Broaden end-to-end play coverage: natural exits, shops and side-room entry/
  exit, dense encounters, less common upgrade states and repeated room/run
  transitions. Existing fixtures are not proof of every live interaction.
- Audit original event order, special camera modes, fractional collisions,
  time-stop/feedback effects and Windows rasterization. Resolve disagreements
  using VM/resources or original-runner captures, not SDL alone.
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
