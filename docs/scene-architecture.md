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

### Retained selection canvas (prepared, not yet connected to live scenes)

`AmigaSelectionLayer.h/.cpp` supplies a reusable 160-pixel content canvas with
176-pixel DMA rows and 608 world-space rows. Two planes occupy 26,752 bytes,
less than the existing smaller display allocation (27,264 bytes). It uses
the imported `Bitmap` masked copy/pattern/rectangle helpers. Full initial
clearing includes the guard word; later partial damage uses `solidRect`,
not the framework's word-rounded `clear`, preserving adjacent pixels.

Commands remain in world coordinates. `view(top)` returns the DMA pointer
without touching pixels for every top row 0..324; a separate plane stride
remains 13,376 bytes. Changed commands still cause local old/new damage,
including intersecting retained commands in original order. Callers must
provide Chip-resident buffers/art, drain before publication or bank replacement,
and invalidate after an in-place artwork replacement.

The host test compares all 325 visible windows with the old renderer, requires
zero repaint and byte-identical storage during camera-only movement, checks
overlapping animation against fresh composition, and tests invalid assets and
removal. It uses a scalar stand-in for the framework operations; passing is
not evidence that the real hardware blitter has executed this new path.

`generate-amiga-hud.py` now also emits an ignored `.selection-mask` and provenance
manifest: the same source exit cover at 22-byte stride, prepared on the host.
All 114,048 bits match the existing gameplay mask's shaft, the padding is zero,
and all 365 cover offsets fit the bank. The original gameplay export is unchanged.

Live style/upgrade routing and Copper support for these taller planes are still
pending. Compiling this translation unit for 68000 and passing host/sanitizer
tests do not establish live integration or a frame-time improvement.

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
