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

`AmigaUpgradeScene` now owns the retained border/content renderers and
world-space draw commands. `AmigaClearPresenter` remains the independent
verification oracle, not the live rendering path. Display-buffer binding,
sprites, Copper publication and next-level transitions still remain in
`DownwellAmiga.cpp`. Moving these into the scene is unfinished; the extraction
must not become a wrapper around a permanently monolithic main loop.

## Remaining consolidation

### Retained selection canvas (live upgrade path; style migration pending)

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
and invalidate after an in-place artwork replacement unless every referenced
resource is proven equivalent and the layer is explicitly rebound (see below).

The host test compares all 325 visible windows with the old renderer, requires
zero repaint and byte-identical storage during camera-only movement, checks
overlapping animation against fresh composition, and tests invalid assets and
removal. It uses a scalar stand-in for the framework operations; passing is
not evidence that the real hardware blitter has executed this new path.

`generate-amiga-hud.py` now also emits an ignored `.selection-mask` and provenance
manifest: the same source exit cover at 22-byte stride, prepared on the host.
All 114,048 bits match the existing gameplay mask's shaft, the padding is zero,
and all 365 cover offsets fit the bank. The original gameplay export is unchanged.

The upgrade path now uses two border canvases in the retired game/HUD buffers
and two content canvases in the scene cache, without additional Chip allocation.
The complete finite source border loops are retained, including area seams;
tile phase and camera position select a viewport by pointer offset. Common
content translation also changes only pointers. Independent icon/shadow motion
still damages its old/new rectangles. Identical command lists skip asset
lookups, Bitmap construction and pixel writes.

`DownwellSelectionCopperList` uses the same centered 160-pixel window as style
selection, a ten-word fetch, 22-byte rows and modulo 2. PF1 holds borders;
PF2 holds content above player sprites. The offline cover supplies BPL5/6
during the wipe. Lists are built once per alternating slot, then only pointer
and plane-count words are patched. The narrow window starts two beam pixels
right of the landscape shaft; the player sprite receives the same translation.
Style selection has not yet migrated to the shared retained layer.

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

Hardware pointer scrolling is now wired for both completion layers. Full
scene-class separation and sharing this backend with style selection remain.

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

## Retained upgrade integration verification

The live retained path passes two clears, 432 visible-plane comparisons with
the old presenter, 192 exit-field comparisons and 60 third-level updates on
the A500 configuration. The stack guard and 68000 instruction/helper audits
pass. Unlike the former whole-buffer comparison, the clear check compares
all 160x284 visible pixels in both two-plane layers: unused storage differs
by design. Debug failure dumps expand the compact view into the old geometry.
It does not verify physical display placement or claim Windows pixel parity.

The first 150-second attempt stopped at its deadline after the first clear,
with zero mismatches. The updated fast path passed with a 240-second hard
deadline; the emulator was stopped and the ordinary startup build restored.
Evidence: `/tmp/downwell-retained-native-gdb.log` and
`/tmp/downwell-retained-native-v2.log`.

Host/sanitizer checks cover 325 camera positions with no pixel writes and
360 border frames crossing area seams. Both compact and wide cover banks
pass 256 artwork selections with allocation/HUD/mask guards intact. Existing
completion motion, selection, session, layout, trig and raster tests pass.

### Retaining borders across offer-bank replacement

Offer/inventory changes rebuild the selected artwork bank, moving resource
directory and payload offsets. This does not change the four border resources
selected for the current area. The upgrade scene now retains those canvases
and rebinds their bank bounds instead of invalidating them. The layer stores
commands and rectangles, not pointers into individual records, so subsequent
damage resolves artwork through the new directory. Content layers still
invalidate because their selected resources really can change.

This optimization requires the same immutable source pack, size, area and
level as scene entry; a different stage/source must enter a new scene. The
host test builds 128 real replacement banks across all four areas and requires
byte-identical canvases with zero repaint, then verifies subsequent damage
against fresh composition. The same cases pass ASan/UBSan.

The bounded native regression also passes 432 completion frames, 192 exit-field
frames, two clears, 60 third-level updates and the stack guard after this
change. Evidence: `/tmp/downwell-border-rebind-native-gdb.log`.
