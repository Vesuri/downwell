# Scene boundaries

The required destination is a dedicated scene class and translation unit for
publisher, credits, controls, garden, style selection, gameplay and upgrade
selection. Platform rendering is separate from shared, source-backed game
rules. This document is a migration inventory, not a claim of completion.

## Upgrade scene: first extraction

`AmigaUpgradeScene.h/.cpp` owns completion entry, input edge retention,
selected-artwork bank state and presenter lifetime. It invokes the existing
shared `GameplayWorld` / `LevelClearController`; it does not introduce a
second upgrade algorithm. Long-lived state remains in static storage, not the
small AmigaDOS stack. The main loop must retire DMA before scene entry and
drain outstanding blits before replacing a bank.

Drawing helpers are still `AmigaClearPresenter`, `AmigaClearOverlay` and the
border renderer. Display-buffer binding, sprites, Copper publication and
next-level transitions remain in `DownwellAmiga.cpp`. Moving these into the
scene is unfinished; the extraction must not become a wrapper around a
permanently monolithic main loop.

## Remaining consolidation

- Publisher, credits and controls currently share splash cache/runtime paths.
  They need distinct scene controllers, sharing prepared-screen machinery.
- Garden has menu renderer/cache/scroll translation units, but lifecycle and
  presentation orchestration remain in the main loop.
- Style selection has `AmigaStyleRenderer.cpp` and its Copper class. Upgrade
  selection must share retained layer/scrolling infrastructure with it.
- Both selection screens should retain world-space text and icon layers and
  use bitplane pointers for common reveal/camera translation. Source-specific
  item trajectories, easing, bobbing and shadow animation must remain intact.
  The existing style reveal still redraws on vertical movement; do not copy
  that limitation into the shared implementation.
- Gameplay logic lives in core `GameplayWorld.cpp`, but the separate SDL
  `StartupRuntime` world loop and Amiga display orchestration still need work.

The scrolling completion-border fast path is live; hardware pointer scrolling
for the completion overlay and full scene-class separation are not yet done.

## Verification of the first extraction

The 150-second-bounded A500 run from `SKIP_LEVEL=1` passes two completion
sequences, 432 clear-screen planar comparisons, 192 exit-field comparisons,
60 third-level updates and the 256-byte stack guard. The earlier 90-second
attempt timed out during the second clear without reporting pixel errors;
it was stopped before rerunning. Log: `/tmp/downwell-upgrade-scene-native.log`.
The expanded border oracle passes 3,840 frames, including under address and
undefined-behavior sanitizers. The preceding full host suite also passes
(`/tmp/downwell-contact-pass-suite.log`). These are regression checks, not
proof of full Windows presentation fidelity or completion of this migration.
