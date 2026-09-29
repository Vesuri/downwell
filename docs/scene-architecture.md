# Scene and rendering contracts

Game rules belong in `src/core`; scene classes own platform presentation.
The sole backlog is [remaining work](open-work.md). Implementation history and
past test results belong in the git log.

## Scene owners

The Amiga scene translation units are `AmigaPublisherScene`,
`AmigaCreditsScene`, `AmigaControlsScene`, `AmigaGardenScene`,
`AmigaStyleScene`, `AmigaGameplayScene` and `AmigaUpgradeScene`.
Class existence alone does not prove complete ownership; outstanding separation
is tracked in [the queue](open-work.md#6-finish-scene-and-backend-separation).

Splash scenes share `AmigaSplashCache` and splash Copper machinery.
Style and upgrade selection share `AmigaSelectionLayer`, but preserve their
distinct source selection trajectories.

## Display lifecycle

Use the platform framework for Copper publication and DMA retirement.
Prepare an inactive list or patch only safe, relevant words; do not rebuild
complete lists for pointer-only scrolling. Failed preparation must prevent
publication. Drain outstanding blits before publication or asset-bank replacement.

Keep asset conversion offline. Use prebuilt hardware sprite images and change
their pointers/control words rather than reconstructing images each frame.

## Retained selection layers

The content canvas is 160 pixels wide, with 176-pixel DMA rows and 608 world
rows. Each plane occupies 13,376 bytes. A viewport for top rows 0..324 changes
pointers, not pixels. Initial clearing includes the guard word; local damage
repair must preserve neighboring pixels, including partial words.

Commands remain in world coordinates. Repair changed commands' old and new
bounds and intersecting retained commands in painter order. Identical command
lists and camera-only movement require no repaint.

Callers supply Chip-resident buffers/art. Invalidate after in-place artwork
replacement unless referenced resources are proven equivalent and explicitly
rebound. Preserve scene-specific palette, mask and playfield-priority rules.

Style selection's immutable border and two opacity masks occupy a different
part of the shared cache from its reference-frame scratch area. Garden entry
prepares them; garden scrolling uses separate menu planes and leaves them
untouched. Only that direct garden-to-style transition may preserve the border
cache, and only with the same asset-bank addresses and successful prior
preparation. Gameplay reuses the storage, so other entries rebuild it.
Both content canvases and both borrowed Copper slots are invalidated even when
the border is retained. Cache retention never implies display-buffer retention.

For the original 16-by-16 border tiles with vertical origin 8, compose two
anchor rows to establish the first complete 16-row period, then extend it by
disjoint planar copies. Clear only the seed strip: copies write every remaining
pixel, including guard words. Changed tile geometry takes the general drawing
path. The native capture verifier compares all 300 border rows against original
texture pixels, independently of the repeated-strip implementation.

The mask edge comes from offline planar art. Staging fills the two constant
tails and copies the edge through `Bitmap`, with a separate zero guard word
for gradient rows. The regions do not overlap; there is no clear-then-redraw
of the same mask pixels. Drain completion before publishing the mask to DMA.

## Gameplay presentation boundaries

- `AmigaGameplayGeneration`: synchronous builder registration and generation
  damage. Game state and RNG effects remain in core.
- `AmigaTerrainPainter`: fresh terrain composition, with explicit data, artwork
  and view inputs. It does not allocate, step simulation or schedule DMA.
- `AmigaTerrainRaster`: backing raster, dirty rows, shop revisions and ring
  transfers.
- `AmigaGameplayRows`: independent damage/copy debts for both visible buffers.
- `AmigaGameplayScene`: Copper lifecycle and backing/staging validity.
- `AmigaGameplayDrawList`: read-only projection of shared state in painter order.
- `AmigaGameplayActors` and sprite assignment: planar actors and hardware
  channel assignment without changing simulation.

Use a circular terrain backing store and pointer-driven scrolling. Restore
moving-object damage from retained terrain and draw newly exposed rows.
A discontinuous camera jump prepares the destination interval, not every skipped
row. Commit staging validity only after successful transfer; failed or replaced
content invalidates the corresponding range.

Partition mixed cache requests into contiguous clean and dirty row runs. Copy
clean runs from the retained raster; compose and store only dirty runs. Explicit
partial-width repairs remain separate, preserving the other pixels in each row.
Before CPU composition follows a blitter clear, drain queued work as well as
the active DMA. Waiting for the active blit alone does not establish that a
queued clear has completed.

Side-view staging borrows retired storage, never the active restoration backing.
Each update copies at most 48 cached rows and composes at most 16 dirty rows;
completed fresh runs also update the retained raster. This is read-only rendering
of already-created world state, not deferred generation or Create dispatch.
Staging cannot clear either visible buffer's repair debts or invoke their
pipelined publication copies. Commit only the prepared contiguous interval.
For upward preparation, bound the adjacent suffix to the paint budget so a
partial operation cannot claim an untouched gap. World changes still invalidate
staging before a camera switch can use it.

Prefetch only a column the player's resolved horizontal velocity approaches.
Do not infer movement from held input: wall collisions can zero velocity while
the key remains held. A stopped, dead or retreating player leaves completed
staging intact without preparing more rows. This is a rendering prediction,
not an entry rule; camera transitions still use shared simulation and fall back
to destination composition whenever the predicted interval is incomplete.

Retire and publish buffers independently: damage acknowledged in one display
must not clear the other's debt. A Copper rebuild does not itself invalidate
retained pixels. Fixed-capacity draw-list failure must prevent publication,
not silently omit actors.

## Terrain landing candidates

`AmigaLandingIndex` stores candidate occupancy in 128 columns of 512 bits
(8 KiB). Insertion sets one bit synchronously during Create; it cannot defer
visibility until the next frame or pattern cell. Queries search at or below the
requested row and return row+1, or zero when no candidate exists.

Occupancy is not breakable lifetime state. The collision resolver must still
reject inactive breakables and continue to the next candidate. Keep room-build
and live-insertion eligibility unchanged, including their distinct solid/landing
flags. The bitmap test compares every intermediate insertion against the old
nearest-floor table; live terrain tests cover generation and collision behavior.

## Retained ammo HUD contract

Compose damaged HUD strips in the retained Fast-RAM image and publish only
differing words independently in each plane. Do not clear visible Chip RAM
before repainting the same content. The palette is `01=white`, `10=red`,
`11=opaque black`; toggling only the first plane cannot represent every
white/black transition. Preserve genuine rim movement during recharge jiggle.

Cache divider geometry by capacity and recharge offset, but retain the general
source-equivalent path for unsupported cache ranges and consumption values.
Derive constant rim spans from artwork rather than assumed dimensions. Damage
must include old/new label ink, pending wide-label backing repairs and exposed
edges. Scene reconstruction invalidates the retained image. Validate optimized
composition against the complete ordered draw sequence, including clipping,
capacity changes and every meter offset.

## Verification scope

Fresh scalar composition provides a retained-buffer oracle. Host tests do not
execute the hardware blitter or prove Copper timing; bounded native runs are
required for those paths. Synthetic camera/shop fixtures do not establish
natural traversal, original Windows raster fidelity or the 20 ms frame budget.

Use the reproducible gates in the Makefile and the procedures in
[profiling](amiga-profiling.md), [replays](replay-format.md) and
[natural routes](natural-route-verification.md).
