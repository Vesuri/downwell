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
The independent full-frame style renderer and its eleven prepared artwork
wrappers exist only in `SCENE_VERIFY` builds. Normal scene preparation must not
construct verification-only state; playable selection uses `AmigaSelectionLayer`.

## Display lifecycle

### Transient artwork leases

Shop, death-results and upgrade artwork are files, not shipping executable
payloads. `AmigaTransientBank` grants a view only to the declared owner. New
contracts need explicit byte budgets in `generate-amiga-transient-banks.py`;
the generator rejects growth beyond them and records input hashes/tool version.
The shipping link audit rejects accidentally re-embedded transient banks.
Verification builds retain linked shop/results bytes only to compare the
file-loaded copies. The upgrade pixel oracle reads the loaded original pack;
it still independently composes the expected display, without a second 169 KiB
executable copy. The generated manifest pins the source file's SHA-256.

Style selection and ordinary gameplay must not prepare death-results art.
Shop and results load directly into their mutually exclusive Chip bank, without
an ordinary-memory source copy. Repeated frames with the same lease perform no
loading or validation. Failed replacement publishes neither the old nor a
partially read new bank. Shop closure repairs its old footprint before releasing
the lease. Cover DMA invalidates the overlay lease before borrowing its memory.

Upgrade selection allocates its ordinary-memory source only on entry. Selected
records are copied to the scene's Chip bank; the source remains available for
offer/inventory changes and is freed after scene retirement. Exiting the program
also frees it. Acquire failures return code 20, never an empty successful scene.
The link audit, lease tests, entry profiler and clear/restart verifiers enforce
these boundaries. These contracts do not yet partition the gameplay resource,
room, collision-mask or SDL texture catalogues; that scope remains in the queue.

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
begins a preparation plan without writing that cache. Starting with the second
garden frame, after the preceding garden list has replaced splash/gameplay DMA,
service one drained block operation or tile pair per frame. Garden scrolling
uses separate menu planes and leaves the preparation destination untouched.
Only that direct garden-to-style transition may preserve the border
cache, and only with the same asset-bank addresses and a valid prior
preparation plan. Gameplay reuses the storage, so other entries rebuild it.
Both content canvases and both borrowed Copper slots are invalidated even when
the border is retained. Cache retention never implies display-buffer retention.
Partial preparation is never renderable. Direct-to-style entry finishes the
same plan synchronously before publication; an early transition resumes a
pending plan instead of restarting it. Failure latches until a new preparation.
This schedules only immutable pixels, never game state, RNG or input decisions.

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

## Canonical level-clear motion

`LevelClearMotion` owns the same source player/camera state on both targets.
Create sets player Y to zero and camera Y to 142. Alarm 4 exposes choices
before Step 60, changing the player target/divisor to 302/10 and camera target
to 340. The recurrence remains the source subtraction, division, then addition.

The offline `generate-clear-motion` program records the first 59 updates and
the choice sequence through its binary64 fixed point. It compares the portable
arithmetic with independent host binary64 at every generated step. The table
contains full states, never rounded pixels; the generated manifest records
input hashes and extraction provenance. Before each table advance the owner
checks all state inputs. Altered inputs, noncanonical choice timing and all
departure updates permanently fall back to the retained arithmetic for that
motion instance. No RNG, event dispatch, carousel input or rendering is moved
ahead of its original phase. `REFERENCE_CLEAR_MOTION=1` selects arithmetic
throughout. The table resides in ordinary executable memory, not Chip RAM.

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

## Read-only actor ordering

`GameplayWorld::visitRoomActors` remains the live layer traversal. Command
collectors that do not change lifetimes, eligibility, sprites or depth may use
`visitRoomActorsReadOnly`. Its owner retains ordered actor handles and depths;
positions, animation frames and Draw side effects are not cached.

Native ownership maintains a `RunnerPreparedDrawList` at mutations. Birth and
retirement splice individual nodes in authoritative layer-element order;
activation reconciliation repositions the affected node. Serviced depth
changes relink from the source layer topology, including layer merges and their
inactive-tail ordering. Registration, exclusions and sprite changes publish
prepared Draw eligibility. Inactive nodes remain as layer cutoffs; scenery-only
population leases and unborn serialized placeholders are not drawable nodes.

Each actor token names a prepared family and generation handle, not a borrowed
record pointer. Enemy compaction therefore cannot invalidate it. Collectors
reuse the flat actor/depth array until this prepared list changes. A refill
walks prepared nodes without resolving lifetimes, object metadata or sprites,
and invokes callbacks as it fills, preserving successful failure prefixes.
Layer depths use their mutation-time binary32 encoding. No source sorting by
numeric resource ID, handle or depth alone substitutes for element order.

The original cached layer traversal is retained by `REFERENCE_DRAW_LIST=1`;
registered ownership also keeps that path, and scalar ownership always uses
the live traversal. Saturated revision counters disable flat-array reuse.
Visitors that mutate ownership must use the live path, not the read-only
projection. Retry preserves both callback bindings and resets the projection.

The flat actor array occupies 3,376 bytes plus bookkeeping. Prepared links and
identities add about 29 KiB in ordinary memory, with no Chip RAM allocation.
Native verification compares kind, generation handle and depth against live
Draw on every tested frame, without consuming Draw RNG.

## Verification scope

Fresh scalar composition provides a retained-buffer oracle. Host tests do not
execute the hardware blitter or prove Copper timing; bounded native runs are
required for those paths. Synthetic camera/shop fixtures do not establish
natural traversal, original Windows raster fidelity or the 20 ms frame budget.

Use the reproducible gates in the Makefile and the procedures in
[profiling](amiga-profiling.md), [replays](replay-format.md) and
[natural routes](natural-route-verification.md).
