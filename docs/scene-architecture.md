# Scene boundaries

The required destination is a dedicated scene class and translation unit for
publisher, credits, controls, garden, style selection, gameplay and upgrade
selection. Platform rendering is separate from shared, source-backed game
rules. This document is a migration inventory, not a claim of completion.

## Current ownership at a glance

All seven Amiga scene classes below have separate `.h` / `.cpp` files in
`src/platform/amiga`. A class existing does not imply that every responsibility
has already moved out of the application loop.

| Scene | Owner | Remaining boundary |
| --- | --- | --- |
| Publisher logo | `AmigaPublisherScene` | Shared startup timing and application publication |
| Credits | `AmigaCreditsScene` | Shared startup timing and application publication |
| Controls | `AmigaControlsScene` | Shared startup timing and application publication |
| Garden | `AmigaGardenScene` | DMA retirement, publication and transition orchestration |
| Style selection | `AmigaStyleScene` | Shared startup rules and application publication |
| Gameplay | `AmigaGameplayScene` plus dedicated terrain/actor components | Scene owns Copper lifecycle and backing/staging ranges; transfer execution/display coordination still needs consolidation |
| Upgrade selection | `AmigaUpgradeScene` | Display binding, sprites, publication and next-level transition |

Style and upgrade selection share `AmigaSelectionLayer` for retained content,
local damage repair and pointer-based reveal scrolling. They retain separate
source-backed selection trajectories rather than forcing identical animation.
Splash scenes share `AmigaSplashCache` and the splash Copper implementation.

This is an Amiga presentation split, not yet a symmetrical seven-scene split
across both backends. SDL still uses the broad `StartupRuntime` path. Further
consolidation must share behavior in `src/core`, not duplicate it inside the
new platform-specific scene classes.

### Gameplay side-view staging ownership

`AmigaGameplayScene` now owns the staged column and retained world-row interval.
It plans at most 48 rows per update, with the existing 16-row lookahead, and
commits validity only after a clean terrain transfer. Column changes, dirty
terrain, shop art changes, destroyed blocks and completed swaps invalidate
the same scene-owned state. Room entry resets the range separately from Copper
slot invalidation: rebuilding a list does not itself change retained pixels.
The main loop still performs transfers. The scene now also owns the active
backing interval as described below; display coordination remains unfinished.

The host test models the actual row/column stored in every circular-buffer row
over 20,000 updates. It checks 15,897 covered views, 378 failed-copy invalidations
and 2,069 upward strips, alongside the existing 600-frame Copper lifecycle
test. Both the normal test and ASan/UBSan pass. Logs:
`/tmp/downwell-scene-staging-host.log` and
`/tmp/downwell-scene-staging-sanitized.log`.

The 120-second-bounded native camera gate passes 600 updates, seven transitions
(six staged swaps and one forced fallback), four camera locks and three
releases. All 119 terrain comparisons and 14 alternating-display checks pass,
with the stack guard intact. Logs: `/tmp/downwell-scene-staging-native.log`
and `/tmp/downwell-scene-staging-native-gdb.log`. The wrapper restores ordinary
startup, and both 68000 audits pass. This verifies the ownership refactor on
the synthetic camera fixture, not natural side-room traversal or a new FPS
measurement.

### Active backing interval and discontinuous scrolling

`AmigaGameplayScene` now tracks active backing validity as well as staging.
Generation, shop changes and destroyed blocks use one offscreen-invalidation
routine. Visible damage still uses the existing clipped repair path. Ordinary
scrolling retains the eight-row lookahead and repaints only newly exposed
rows. Pointer-based scrolling and the two independent display-copy debts are
unchanged; this moves no simulation or RNG into rendering.

Previously a sufficiently large camera snap could request more than 320 rows,
repeatedly overwriting the circular buffer with intermediate rows never shown.
Such requests now cover only the destination view and eight rows on either
side: 300 rows for a 284-row view. The caller paints before publication. This
does not make a full destination refresh free or claim a 20 ms frame budget.

The independent backing-row/revision test covers 20,000 updates, 207 camera
snaps, offscreen damage and reversals. All visible rows match; 17,813 updates
request no backing refresh. Explicit large forward/backward jumps each request
300 rows. Normal and ASan/UBSan runs pass, including the earlier staging and
Copper lifecycle tests. Logs: `/tmp/downwell-scene-backing-host.log` and
`/tmp/downwell-scene-backing-sanitized.log`.

The bounded native camera gate passes 600 updates, seven transitions (six
staged/one fallback), 119 terrain comparisons, 14 display checks, four camera
locks and three releases, with no mismatches and an intact stack guard.
Logs: `/tmp/downwell-scene-backing-native.log` and
`/tmp/downwell-scene-backing-native-gdb.log`. This native tape checks ordinary
live transitions; the large skipped-interval case is covered by the host row
oracle, not a new native snap fixture. Ordinary startup is restored, and both
68000 audits pass.

## Upgrade scene: first extraction

`AmigaUpgradeScene.h/.cpp` owns completion entry, input edge retention,
selected-artwork bank state and presenter lifetime. It invokes the existing
shared `GameplayWorld` / `LevelClearController`; it does not introduce a
second upgrade algorithm. Long-lived state remains in static storage, not the
small AmigaDOS stack. The main loop must retire DMA before scene entry and
drain outstanding blits before replacing a bank.

`AmigaUpgradeScene` now owns the retained border/content renderers and
world-space draw commands. `AmigaClearPresenter` remains the independent
verification oracle, not the live rendering path. The scene now owns both
borrowed selection Copper wrappers and their build/update validity. Entry
invalidates both lists; a failed patch forces a rebuild on the next attempt.
Sprites, final Copper publication and next-level transitions still remain in
`DownwellAmiga.cpp`. Moving these into the scene is unfinished; the extraction
must not become a wrapper around a permanently monolithic main loop.

The display-owner test covers two entries, 600 alternating frames, mask
retirement and failure recovery; ordinary and ASan/UBSan runs pass. The bounded
native gate passes 466 completion frames, 192 exit-field frames and two
re-entries, including all three upgrade cursors on each entry. Logs:
`/tmp/downwell-upgrade-owner-host.log`,
`/tmp/downwell-upgrade-owner-sanitized.log`, and
`/tmp/downwell-upgrade-owner-native.log`. Ordinary startup was rebuilt afterward;
both 68000 audits pass. This gate still uses injected exits.

## Remaining consolidation

### Publisher, credits and controls owners

Each now has its own header/translation unit and borrowed Copper-list storage.
`AmigaPublisherScene` prepares the narrow logo list; `AmigaCreditsScene`
prepares the wide credits list. `AmigaControlsScene` owns the held controls
list and both alternating dither lists, including the blank-backed final fade.
Their common machinery remains `AmigaSplashCache` and
`DownwellSplashCopperList`; there are no duplicate image converters or new
Chip allocations. The existing startup controller retains all input/timers.
The application still publishes through `AmigaPlatform` and owns display
retirement/blanking used by subsequent scenes.

Failed preparation makes a scene unpublishable; failed fade preparation does
not advance the front slot. Held controls return their prepared list without
rebuilding it. Host lifecycle tests cover preparation, 240 held frames,
both fade slots, invalid input, failed preparation and recovery; ASan/UBSan
pass. These doubles verify ownership, not actual hardware execution.

The bounded full startup gate also captures publisher, credits and controls
at their presentation boundaries and checks their existing hashes. A per-run
capture bitset rejects a missing boundary rather than accepting an old file.
The final 180-second-bounded run passes all three hashes, original splash
durations, garden Copper checks, 138 style comparisons and the stack guard.
Normal startup is restored and both 68000 audits pass. Evidence:
`/tmp/downwell-splash-scenes-final.log`,
`/tmp/downwell-splash-scenes-final-gdb.log`, and
`/tmp/downwell-splash-scenes-sanitized.log`. This is a native regression check,
not a new Windows-runner capture or a completed seven-scene migration.

### Retained selection canvas (live upgrade and style paths)

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
Style selection also uses `AmigaSelectionLayer` for its content, retaining its
existing periodic border and five-plane priority/masking arrangement. Its
dedicated Copper list accepts the taller content-plane stride while retaining
the original border stride and palette rules.

- Publisher, credits and controls have dedicated `AmigaPublisherScene`,
  `AmigaCreditsScene` and `AmigaControlsScene` classes and translation units.
  They share `AmigaSplashCache` and the existing splash Copper builder; the
  controls owner manages its held list and both alternating dither slots.
  Startup timing/input remains in the existing controller. Application-level
  blanking and final platform publication remain in the main loop.
- Garden now has `AmigaGardenScene.h/.cpp`, owning prepared parallax artwork,
  cached title frame, offline sprite selection and both Copper patch states.
  It delegates to the existing menu cache/scroll helpers. DMA retirement,
  publication and verification captures remain application responsibilities.
- Style selection now has `AmigaStyleScene.h/.cpp`, owning both alternating
  frame states, preparation/re-entry reset and Copper build-versus-patch
  state. It delegates drawing to `AmigaSelectionLayer`. The application
  still retires DMA and publishes the completed list, and shared startup
  rules remain in `AmigaSceneRuntime`. This extraction introduces no new
  Chip allocation or change to drawing/selection trajectories. The old renderer's
  prepared-art cache remains single-scene scratch; this is not a concurrent
  multi-instance renderer. Both selection scenes now share the retained
  content-layer implementation; the old style renderer remains the native
  full-redraw verification oracle.
- Both selection screens should retain world-space text and icon layers and
  use bitplane pointers for common reveal/camera translation. Source-specific
  item trajectories, easing, bobbing and shadow animation must remain intact.
  Style's common vertical reveal now changes the viewport pointer without
  moving commands or repainting the text. Independent icon animation and
  cursor changes still repair local retained damage.
- Gameplay logic lives in core `GameplayWorld.cpp`, but the separate SDL
  `StartupRuntime` world loop and Amiga display orchestration still need work.
  `AmigaGameplayRows.h/.cpp` now owns the two display rings' independent
  dirty-row debts, initialized margins, conservative pending ranges and
  backing-copy synchronization. Terrain staging, BOB restoration, laser/goal
  field repair and verification query this owner instead of global arrays.
  `AmigaGameplayScene.h/.cpp` owns both Copper programs and their readiness.
  Terrain/actor presentation still remains in the application; these are
  incremental boundaries, not a completed gameplay scene extraction.
  Overflow BOB rasterization now lives in `AmigaGameplayActors.h/.cpp` with
  explicit artwork, retired display, dirty-row and laser inputs. Actor
  collection now lives in `AmigaGameplayDrawList`; terrain composition now lives
  in `AmigaTerrainPainter`, while raster storage/transfer/invalidation belongs
  to `AmigaTerrainRaster`. Hardware-channel assignment lives alongside the actor
  renderer. The move preserves painter order, clipping, circular row
  addressing and the native scalar verification oracle.

Hardware pointer scrolling is wired for both completion layers and the style
reveal. Full seven-scene class separation remains unfinished.

### Live horizontal camera verification

`make verify-amiga-gameplay-camera` builds a separate `CAMERA_VERIFY=1`
fixture starting at gameplay, with a 120-second emulator deadline. Its exit
trap restores the ordinary startup executable, including on failure.
The fixture replaces collision callbacks with a flat floor and supplies a
600-update left/right tape. Shared movement and camera logic, terrain
generation, retained rasters, prefetch, display swaps and publication remain
the production paths. Exactly the first horizontal transition discards its
prefetch tag to exercise the fallback path; later transitions can use real
staged buffers. None of this is compiled into normal builds.

The gate requires all three camera columns, at least six transitions, both
staged and fallback paths, and checks of both alternating displays after
each transition. Existing fresh-terrain comparisons, display-versus-backing
checks (excluding explicitly dirty actor rows), reserved-asset checks and
the stack guard must remain clean. This is a rendering regression fixture,
not a natural shop playthrough, complete actor-composition oracle, performance
measurement or comparison with a new Windows capture.

The fixture now also registers two source-normalized side-camera lockers at
different heights. It requires at least three collision locks and releases.
The final integrated run observes four locks/three releases with the same
six staged swaps and one forced fallback. Prefetch predicts the destination
height through a read-only shared camera copy; the old same-height assumption
missed all seven transitions once real locker snaps were enabled. See
`side-camera-lockers.md` for integration evidence and scope.

The native run passes with seven transitions (six staged, one fallback),
119 fresh-terrain comparisons and 14 display checks, zero raster/display
mismatches and an intact stack guard. The shared `main-camera-test` also
passes. Evidence: `/tmp/downwell-camera-transitions-v3.log` and
`/tmp/downwell-camera-transitions-v3-gdb.log`. Earlier fixture attempts
correctly failed coverage: first because player-level callbacks were rebound
by the world, then because every transition found a prepared buffer. The
final fixture binds collision through the world and explicitly exercises the
missing-prefetch case.

### Live shop interaction verification

`make verify-amiga-gameplay-shop` builds `SHOP_VERIFY=1` with direct gameplay
entry and a 120-second emulator deadline, restoring the normal build on exit.
`AmigaShopVerification` creates a shop beside the grounded player and sends
ordinary input edges through the live world. It checks opening, held-confirm
debouncing, an unfunded purchase, a test-only free battery purchase, deferred
grant on the next controller update, leaving, reopening sold stock and leaving
again. The free price isolates purchase timing from gem collection, as in the
existing paired SDL/Amiga shop fixture. No test code is enabled in shipping.

The native loop compares its actual retained shop HUD against fresh scalar
composition every frame after the shop artwork bank is loaded, preserving
the surrounding HUD and restoring the foreground streak in source draw order.
The passing run covers all eight state checkpoints and 13 whole-HUD comparisons
with zero mismatches and an intact task-stack guard. Evidence:
`/tmp/downwell-live-shop.log`, `/tmp/downwell-live-shop-gdb.log`. The paired host
scene/shop test also passes, including both subsequent cavern-level reentries
(`/tmp/downwell-live-shop-host.log`). This is live native interaction coverage,
not a natural generated-shop approach, independent Windows raster oracle,
or a frame-time result.

### Terrain raster boundary

`AmigaTerrainPainter.h/.cpp` now owns fresh composition: indexed or serialized
room walls, source wall-neighbor frame selection, row-indexed tiles with
destroyed-block suppression, decorations and shop fixtures in the existing
order. Renderer, room/index/tile data, world, artwork bounds and clipping/view
coordinates are explicit inputs. It does not allocate, step simulation, own
the raster, choose dirty spans, publish Copper programs or schedule DMA.
The cache wrapper still marks damage and selects refresh versus transfer;
this extraction does not add any repaint or change circular-buffer behavior.

The bounded native camera gate passes seven transitions, 119 fresh comparisons
and 14 display checks with zero failures. A separate bounded Laser Sight
capture matches all 99,968 SDL pixels at its verification frame. These are
regressions against the current representation, not original Windows proof
or a performance improvement claim. Evidence:
`/tmp/downwell-terrain-painter-camera.log`,
`/tmp/downwell-terrain-painter-camera-gdb.log`, and
`/tmp/downwell-terrain-painter-laser.log`.

`AmigaTerrainRaster.h/.cpp` owns the three-column Fast-RAM raster, dirty rows,
shop-art revisions and ring transfer. The native load path retains the existing
68000 assembly, 22-byte strips and 48-byte Chip row stride. Long loads still
pipeline disjoint completed spans of at most 128 rows into the retired display
and acknowledge only that display's row debts. There is no new framebuffer,
asset conversion or per-frame terrain repaint. The application calls the
separate `AmigaTerrainPainter` and decides when to refresh the cache; this is
not full terrain-scene extraction.

A portable transfer counterpart enables byte-exact host tests across all 320
origins, three columns and five heights (4,800 cases), covering both sides of
the 96-row pipeline threshold. Tests require correct stored/loaded planes,
untouched allocation guards and side margins, exact pipeline copy counts and
independent display debts. Shop invalidation tests cover top/bottom clipping,
off-raster rows and unchanged revisions producing no new damage. ASan/UBSan
pass (`/tmp/downwell-terrain-raster-host.log` and
`/tmp/downwell-terrain-raster-sanitized.log`). These tests exercise the portable
copy; native cache comparison is required separately for the assembly twin.
The final 120-second-bounded native cache gate passes 600 ticks and 101
fresh-composition comparisons with no terrain/display mismatches and an intact
256-byte stack guard (`/tmp/downwell-terrain-raster-final-native-gdb.log`).
This run includes the final invalidation extraction. The compiler defines
`__m68k__`, selecting the assembly load path. Normal startup is restored and
the 68000 instruction/helper audits pass; no 20 ms performance claim is made.

### Actor collection boundary

`AmigaGameplayDrawList` projects the shared `GameplayWorld` without stepping
or mutating it. It preserves enemies, shopkeepers/prompts, player, planar ray,
bullets and gems in their existing painter order, including the replay-only
player anchor and omitted replay enemies/gems. Canvas bounds still filter
fully invisible actors; this is not hardware-sprite clipping. Resource IDs,
frames, facing and GameMaker coordinate rounding are unchanged.
The fixed-capacity list has an explicit append guard; failure prevents frame
publication and records the responsible resource. Its storage remains static
and constant-initialized, avoiding both the small task stack and freestanding
C++ local-static guard functions.

Host tests project 600 stepped worlds in each layout and compare every emitted
command with independently assembled expected order/coordinates: 3,647 normal
commands and 3,047 replay commands. Collection leaves the world checkpoint
unchanged. Missing-art failure followed by successful recollection clears old
diagnostics. The normal test passes ASan/UBSan. Evidence:
`/tmp/downwell-actor-collection-host.log` and
`/tmp/downwell-actor-collection-sanitized.log`.
After correcting local-static initialization, the 45-second-bounded native
Laser Sight capture matches all 99,968 SDL pixels at tick 180
(`/tmp/downwell-actor-collection-laser-v2.log`). The separate 120-second-bounded
cross-target replay gate passes its complete host/native result vector
(`/tmp/downwell-actor-collection-replay.log`). These checks cover both target
build modes, not original Windows completeness. Normal startup is restored;
the 68000 instruction and helper audits pass.

### Overflow actor renderer boundary

The host actor test uses synthetic planar artwork and an independent pixel
loop to check 576 combinations of clipped anchors, three ring origins, both
display slots and laser-before/after-actor order. It requires byte-exact display
pixels including guard bytes, unchanged terrain backing/HUD and only the
drawn display's actor rows marked dirty. Pre-existing damage in the other
buffer survives. ASan/UBSan pass. Evidence:
`/tmp/downwell-gameplay-actors-host.log` and
`/tmp/downwell-gameplay-actors-sanitized.log`.
The 120-second-bounded native overflow fixture passes 100 scalar comparisons
with no BOB, display-buffer or terrain mismatches
(`/tmp/downwell-gameplay-actors-native-gdb.log`). Normal startup is restored
and both 68000 audits pass. The synthetic host tests use the scalar renderer;
the native fixture supplies the actual blitter-path check.
The separate Laser Sight fixture uses a 45-second-bounded native run from
`SKIP_TO_GAME=1`, captures tick 180, and compares the decoded 352x284 DMA
window with SDL at the same simulation tick. All 99,968 pixels match
(`/tmp/downwell-gameplay-actors-laser.log` and
`/tmp/downwell-gameplay-actors-laser-gdb.log`). This is a deterministic
cross-port frame check, not proof of Windows GPU line rasterization.

### Hardware actor assignment boundary

`AmigaGameplaySpriteAssignment.cpp` implements `AmigaGameplayActors::assign`.
It resolves offline multipart sprites from front to back, assigns channels
0..7, stops at the first failed fit or planar laser marker, and hides unused
channels. It never clips hardware sprites or generates their pixels. The
remaining prefix stays in original painter order for the BOB renderer.
The live application retains the six regular slot / player / overflow mapping
with a compile-time agreement check. Resolution and draw failures propagate
to the existing no-publication failure path; later-part failures now also
report the responsible sprite/frame instead of leaving stale diagnostics.

Host tests exhaust 11,520 combinations of multipart widths, channel limits
0..8 and laser-marker positions. They check exact channel contents and hidden
slots, no backfilling behind a failed fit, unchanged out-of-viewport anchors,
empty lists and initial/later resolution or draw failures. ASan/UBSan pass.
Evidence: `/tmp/downwell-sprite-assignment-host.log` and
`/tmp/downwell-sprite-assignment-sanitized.log`.
The 45-second-bounded Laser Sight capture again matches all 99,968 visible
SDL pixels at tick 180, now through the extracted assignment path
(`/tmp/downwell-sprite-assignment-laser.log`). The normal build is restored
and both 68000 audits pass.

The assignment extraction also passes the separate 120-second-bounded
ordinary gameplay gate: 600 ticks, 101 fresh-terrain comparisons, no display
or terrain mismatches, and the 256-byte stack guard intact
(`/tmp/downwell-sprite-assignment-cache-gdb.log`). This fixture has no BOB
comparisons; the preceding BOB-specific gate and actor tests remain separate
evidence rather than implied coverage from this run.

### Gameplay Copper lifecycle

`AmigaGameplayScene` now decides whether each retired slot requires a full
build or only ring-pointer patches. Startup lists reference the backing
bitmap, so both slots are explicitly invalidated before normal display-ring
frames (and before other scenes reuse them). Re-entry and completion retain
their existing invalidation boundaries. Failed build, update or exit-mask
patch leaves that slot invalid; callers do not publish a failed frame.
The application still owns DMA retirement and final platform publication.
No new Chip storage, game rules, Copper instructions or scrolling arithmetic
are introduced by this extraction.

The host lifecycle test covers initial backing lists, 600 alternating frames,
exit-mask introduction/removal, one-slot and two-slot reuse, invalid slot
indices and recovery after build/patch/mask failures. ASan/UBSan pass:
`/tmp/downwell-gameplay-scene-host.log` and
`/tmp/downwell-gameplay-scene-sanitized.log`.
The bounded native re-entry test passes all three results destinations,
640 overlay Copper checks and 200 subsequent gameplay updates with no scene
mismatches and an intact stack guard
(`/tmp/downwell-gameplay-scene-native-gdb.log`). The separate exit-wipe test
passes 48 presented frames, validating both cover DMA pointers and mask-bank
bytes (`/tmp/downwell-gameplay-scene-exit-gdb.log`).
The full startup gate also passes all three splash hashes, original splash
durations, garden Copper checks and 138 style comparisons before gameplay,
with the stack guard intact (`/tmp/downwell-gameplay-scene-startup-gdb.log`).
Normal startup is restored and both 68000 audits pass. These native gates
are bounded at 180 seconds (startup/re-entry) and 120 seconds (exit wipe).

### Gameplay ring synchronization boundary

The extracted row component preserves the existing geometry and DMA policy:
first synchronization initializes the full 384-pixel row; subsequent terrain
repairs copy only the 176-pixel strip at x=96. Each display ring independently
owes changed backing rows. Wrapped damage is split into physical runs by the
same scan, and acknowledged pipelined copies are not copied again. A clean
ring returns without any copy or blitter drain. Reset still requires world
preparation to mark its completed rows before synchronization.

The focused host test independently tracks row debts and expected two-plane
pixels through 10,000 damage/wrap/pipeline cases and repeated re-entry. It
checks exactly which rows are copied, untouched side margins, full-width
initialization and zero DMA for repeated unchanged syncs. ASan/UBSan pass.
Native results-restart passes all three destinations, 640 overlay Copper
checks, mismatch counters, the stack guard and 200 subsequent gameplay updates.
Evidence: `/tmp/downwell-gameplay-rows-host.log`,
`/tmp/downwell-gameplay-rows-sanitized.log`, and
`/tmp/downwell-gameplay-rows-native-gdb.log`. These checks establish extraction
equivalence, not full original-game parity or a new 20 ms performance claim.
The separate 120-second-bounded `SKIP_TO_GAME=1` cache gate reaches 600 ticks
with 101 fresh-terrain comparisons, zero terrain/display mismatches and an
intact stack guard (`/tmp/downwell-gameplay-rows-cache-gdb.log`). This fixture
has zero overflow-actor comparisons; it must not be cited as BOB coverage.
The dedicated 120-second-bounded BOB fixture separately passes 100 scalar
comparisons with zero BOB, display-buffer or terrain mismatches
(`/tmp/downwell-gameplay-rows-bobs-gdb.log`). Normal startup is restored after
both tests; the 68000 instruction and helper audits pass.

### Garden ownership verification

The garden owner borrows the existing three canvases and two alternating
Copper slots; it allocates no additional Chip memory and preserves the
existing scrolling arithmetic. Preparing after gameplay invalidates both
lists and the cached title. Returning to the same presentation only invalidates
the retired Copper slots. Ordinary frames select offline player sprites and
patch pointers/scroll words; title artwork changes only when its frame changes.

The host lifecycle test checks 240 unchanged frames with one artwork
preparation, one title update and two initial list builds, followed by pointer
patches. It covers well-entry canvas selection, re-entry and preparation/
title/sprite/Copper failures. ASan/UBSan pass. Native startup passes all three
splash hashes and original durations, the garden patched-versus-fresh Copper
oracle, 138 style comparisons and the 256-byte stack guard. Evidence:
`/tmp/downwell-garden-scene-host.log`,
`/tmp/downwell-garden-scene-sanitized.log`,
`/tmp/downwell-garden-scenes-native.log`, and
`/tmp/downwell-garden-scenes-native-gdb.log`.
The separate 180-second-bounded results-restart gate passes all three
destinations, including surface/garden re-entry, and reaches 200 subsequent
gameplay updates. It checks 640 overlay Copper updates, scene mismatch
counters and the stack guard (`/tmp/downwell-garden-reentry-native-gdb.log`).
Both gates restore ordinary startup and pass the 68000 audits.

### Retained style content

`AmigaStyleDrawList` keeps the original carousel parabola, integer rounding,
run animation, arrow rules and selected-player descent. A world-space center
of 302 and viewport `160 - styleYOffset` reproduce the original screen center
`142 + styleYOffset`. The reveal's viewport stays in rows 160..288, within the
608-row canvas. Each alternating slot retains its own world-space commands.

The two canvases borrow the game and HUD buffers retired during style selection
(each fits 26,752 bytes). Existing scene-cache memory retains the periodic
border and entry/exit masks. No Chip allocation is added. Verification uses
the unused first two old style-frame slots in the scene cache, not the live
HUD canvas. Long-lived command/damage arrays are in static placement storage,
not the 4 KiB AmigaDOS stack. Re-entry invalidates both canvases and Copper
states before memory is reused.

Host comparisons exercise 364 frames covering all five cursors, reveal and
confirmation against full source-order draws using original sprite IDs and
individual labels. Eighty-eight unchanged command updates leave storage
byte-identical and report zero repaint; allocation guards remain intact.
ASan/UBSan pass. The bounded native check matches 105 full-redraw frames;
normal, locked and selected capture hashes are unchanged. Evidence:
`/tmp/downwell-style-retained-pixels.log`,
`/tmp/downwell-style-retained-sanitized.log`, and
`/tmp/downwell-style-retained-native-gdb.log`.
The bounded results-restart run also passes all three destinations, 640
overlay Copper checks, the stack guard and 200 final gameplay updates with
no style, garden or gameplay comparison failures. Evidence:
`/tmp/downwell-style-retained-reentry-gdb.log`.

The bounded navigating style profile (`SKIP_INTRO=1 PROFILE_NAVIGATE=1`)
measures 43.48 FPS over 180 updates/207 PAL fields with low-overhead counting,
maximum two fields per update. Detailed probes separately measure retained
content at 9.51 ms average/27.78 ms peak and drain at 0.42 ms average/11.10 ms
peak, with zero clock clamps. These are current measurements, not a claim
of a measured speedup over the previous implementation or of 50 FPS.
Selection-change damage still needs optimization. Both runs restore normal
startup. Logs: `/tmp/downwell-style-retained-profile.log` and
`/tmp/downwell-style-retained-fps.log`.

### Style scene ownership checks

The extracted scene passes host lifecycle tests for two independently retained
slots, first-build versus later Copper patching, re-entry at reused addresses,
insufficient storage, failed preparation/rendering and failed Copper build
retry. These use rendering doubles to test ownership, not to claim hardware
pixel correctness. ASan/UBSan also pass.

The real A500 skip-intro test covers reveal, locked-style navigation and
confirmation, with 105 frames matching the unchanged full-redraw oracle.
The emulator is bounded to 120 seconds and normal startup is restored.
Evidence: `/tmp/downwell-style-scene-native-gdb.log` and
`/tmp/downwell-style-scene-sanitized.log`.
The separate 180-second-bounded results-restart test passes all three
destinations, 640 retained overlay Copper checks, style/menu/gameplay renderer
failure checks, the stack guard and 200 final gameplay updates. This verifies
real memory reuse, beyond the lifecycle doubles. Normal startup is restored.
Evidence: `/tmp/downwell-style-reentry-native-gdb.log`.

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

### Border geometry cache

Each alternating upgrade display now owns an `AmigaSelectionBorder`. Its
world-space commands depend only on area and seam row; `tileY` is removed
before rasterization and applied through the viewport pointer. With unchanged
geometry and storage, rendering returns before constructing or comparing the
76 commands. An area/seam change takes the normal retained-damage path.
Destination/bank rebinding and explicit invalidation remain part of the key;
equivalent offer-bank replacement preserves the geometry cache as well.

Host comparisons cover source area seams, bank replacements and rebinding to
a different destination, with invalid seam rows rejected. This is separate
from the animated text/icon layer, which still rebuilds its layout each frame.
The recovered `gml_Object_Result_Draw_0.asm` blocks 1–5 emit these loops using
only `global.area`, `bordery`, `tileMaxy`, and the common `tiley` translation.
`gml_Object_Result_Create_0.asm` initializes `tileMaxy` to 36; no other recovered
Result event assigns it (PlayMenu has its own separate initialization).
This source evidence, rather than SDL
output alone, justifies the cache key and its 76-command bound.
The bounded native run passes 432 completion comparisons, 192 exit-field
comparisons, both clears, 60 third-level updates and the stack guard with
this cache live (`/tmp/downwell-border-geometry-native-gdb.log`).

### Offline upgrade text outlines

The clear exporter now keeps the original name/description masks in frame 0
and adds a precomposed frame 1 under the same synthetic resource ID. It matches
`gml_GlobalScript_scrDrawBorderTextJp.asm`: black at x+1, x-1, y+1, y-1, then
white foreground. One-pixel padding is reflected in the anchor, including
centered multiline descriptions and owned-name suffixes. Animated red heading
shadows are not baked or simplified.

The live upgrade draw list uses one masked planar command per outlined label
instead of five; the old presenter retains the five-pass path as its oracle.
Selected banks include both frames. A conservative bound over every area,
level, offer and ownership state is 25,556 bytes, below the existing 31,756-byte
bank capacity. No new Chip allocation is introduced. The full Fast-resident
clear pack grows from 60,224 to 112,112 bytes. The ignored clear-v2 manifest
records the frame convention, extractor version and source/language/pack hashes.

Host checks compare all 71 variants at four clipping positions (284 cases),
plus 360 animated overlap/ownership/selection frames against the five-pass
oracle. Bank tests cover both representations and allocation/HUD/mask guards.
The 240-second-bounded native run passes 432 completion frames, 192 exit-field
frames, two clears and 60 third-level updates, including the stack guard and
68000 audits, also after the ordered bank-selector change below. Evidence:
`/tmp/downwell-outlined-merge-native-gdb.log`. Host outline
tests also pass ASan/UBSan, and regenerating the pack with `--check` is exact.
The expanded bank selector validates the source directory's strict ordering
once and merges the sorted requested keys, rather than scanning every source
record for each request. Duplicate or unordered directories fail before any
destination write. Selected metadata/payloads and guard bytes are independently
checked, including under ASan/UBSan.
The game-profile export remains byte-identical (`--profile game --check`),
so the shared outline helper has not altered credits/style/gameplay assets.

### Bank payload copies

The bank builder now uses standard `memcpy` for the validated header,
directory records and already-planar payloads. Inspection of the 68000 output
showed the former indexed payload loop using one `move.b` per iteration;
the new call reaches the existing runtime's aligned `move.l (a0)+,(a1)+`
loop, with byte handling for other alignments/tails. No new assembly or
AmigaOS call was introduced, and validation still precedes destination writes.

Host tests exercise all 16 combinations of source/destination byte alignment,
compare complete bank bytes, require unchanged source data and intact prefix/
suffix guards, and also pass ASan/UBSan. Existing selected-payload, malformed
directory and bank-capacity tests remain in place.
The bounded A500 regression passes 432 completion frames, 192 exit-field
frames, both clears, 60 third-level updates and the stack guard after the
copy change (`/tmp/downwell-bank-copy-native-gdb.log`). Both 68000 instruction
and helper audits pass.

### Exact heading-pixel specialization

The live upgrade renderer uses `LevelClearHeadingPixels` for the seven red/
white heading passes. It retains the existing Q30 cosine sampler and exactly
reproduces the general layout's binary64 arithmetic followed by binary32
round-to-nearest-even and floor. This is a raster specialization, not a new
animation rule. The general shared layout remains the SDL/reference path;
both paths share command ordering and all other layout through
`drawLevelClearWithShadows`.

For passes 0..6, the products and coordinates are exactly representable in
binary64. Anchors 80 and 96 remain in the binary32 binade [64,128), while
278 remains in [256,512). In Q30 units the half-ULP thresholds are therefore
4096 and 16384. Repeated unsigned addition yields the integer and fractional
offset; a fractional remainder at or below the threshold rounds back to the
integer coordinate (ties choose its even mantissa). Larger remainders floor
one pixel lower. No 64-bit multiplication or general coordinate normalization
is needed for these heading passes.

Tests compare one million Q30 samples, all 360 angle samples, endpoints and
rounding-boundary neighbours against an independent double/float expression;
selected cases also compare the portable `HudReal` implementation. All 360
animated overlay command lists match the original path. ASan/UBSan pass.
The bounded native run passes 432 completion-plane comparisons, 192 exit-field
comparisons, both clears, 60 third-level updates and the stack guard:
`/tmp/downwell-heading-native-gdb.log`. This establishes equivalence to the
existing raster policy, not a new claim of Windows GPU pixel-center fidelity.

### Eliding immediately overwritten masked passes

`AmigaSelectionLayer` skips a paint operation when the next command uses the
same resource, frame and position. Both supported operations (asset-colour
masked copy and constant-pen masked write) replace every set mask bit in
both planes. The later operation therefore completely supersedes the earlier
one even when their pens differ. There is no blending or destination sampling
that would require the discarded intermediate value. Different positions or
frames are not combined, and source ordering is otherwise unchanged.

Validation and retained damage still use the complete original command list;
only submission of the overwritten paint is eliminated. Thus a later change
or removal still repairs the old area, and an invalid command cannot hide
behind a valid replacement. This optimization is in the reusable selection
layer, not an upgrade-specific alternate animation path.

Host tests exercise all 360 heading angles with five final pen modes, clipped
positions and retained updates against the independent full-pass renderer
(1,800 cases). They also count masked operations: seven coincident passes
produce only the final two single-plane pen writes. Existing border, bank
replacement, outline and camera-only no-repaint tests remain enabled.
These cases pass ASan/UBSan, including rejection of an invalid overwritten
command without pixel changes. The 240-second-bounded native comparison
passes 432 completion frames, 192 exit-field frames, two clears, 60 third-level
updates and the stack guard. Evidence:
`/tmp/downwell-overwritten-native-gdb.log` and
`/tmp/downwell-overwritten-sanitized.log`.

### Individual retained style labels

Style content no longer submits the precomposed 160x128 text panel. It uses
the existing individual title/name/description assets at their original
anchors, after the actors in the original full-redraw order. The standard
description keeps its forced white pen; title/name retain their exported red
shadows. No assets change. Independent bounding rectangles keep ordinary
actor damage from intersecting the panel's empty interior, and changing the
style name no longer invalidates the full panel area.

The 364-frame host comparison across all five cursors and its sanitizer run
pass. The bounded native comparison matches 105 frames across reveal,
locked-style navigation and confirmation. Evidence:
`/tmp/downwell-style-labels-host.log`,
`/tmp/downwell-style-labels-sanitized.log`, and
`/tmp/downwell-style-labels-native-gdb.log`.

Under the same 180-update navigation script, low-overhead throughput improves
from 43.48 to 45.45 FPS (198 PAL fields). Detailed retained drawing averages
8.64 ms, down from 9.51 ms; its peak falls from 27.78 to 26.04 ms. Drain
averages 0.34 ms, peak 5.84 ms. The low-overhead maximum gap is three fields
(previously two), so this is not a worst-case deadline improvement or a
50 FPS result. All three saved frame hashes remain unchanged. Both bounded
profiles finish, report readiness, and restore normal startup. Evidence:
`/tmp/downwell-style-labels-profile.log` and
`/tmp/downwell-style-labels-fps.log`.
