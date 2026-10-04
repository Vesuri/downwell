# Terrain update constraints

Authority: the immutable Steam `data.win`, SHA-256
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`,
exported by UndertaleModTool 0.9.2.0. `make verify-terrain-create`
pins the local event inventories, ancestry, Create properties, explicit wall
End Step stores/calls and Draw calls. It does not prove whole-program immobility.

| Object | Local behavior relevant to avoiding repeated work |
| --- | --- |
| 60 `objWall_n` | Create stops animation. End Step can change tile frames, create grass, convert to tiles, deactivate or destroy the instance. Explicit stores do not change coordinates. Draw adds corner highlights. |
| 93 `objThinPlatform` | Create selects frame 23 and stops animation. End Step calls `scrOutofview`; it is not an empty event. |
| 99 `Wall` | Empty Create and End Step override the parent's events. Invisible sprite 46 has one frame, but this object does **not** inherit the parent's `image_speed=0` assignment. |

All reachable PreCreate events in these inheritance chains are empty or call
`event_inherited` only. There are no local Begin Step, ordinary Step, alarm or
collision events in these chains. This limits local script-driven movement;
it does not rule out external instance writes or implicit runner updates.

The shared terrain store currently assigns coordinates at birth and exposes
no subsequent position setter. Collision-owner attachment initializes previous
coordinates from the birth position. Its room animation projection may omit
terrain's identical snapshot copies, but does not assume external writes are
impossible: every spatial-owner coordinate publication marks a conservative
terrain-range dirty flag. A dirty pass uses the full room-order walk. A write
during animation switches the remainder to the full walk at the raw successor
and also requests a full next pass, preserving already-visited snapshots.
Reactivation forces a refresh, since an inactive instance can retain an older
snapshot. The live owner prepares projected links at room entry. Active-list
insertion computes the new slot's successor and, for a projected actor, repairs
only its preceding contiguous omitted run. Removing a projected actor redirects
that run to the next projected slot; removing omitted terrain changes no cached
target. Activation uses these same removal/insertion operations. No new source
event is skipped, and the raw room links remain authoritative. The caller-owned
rebuild-on-mutation projection remains available as a reference; copied owners
and changed omitted ranges rebuild before reuse. Actor animation
and terrain End Step are not omitted by this mechanism. Activation and
retirement remain observable, including the source rule that an inactive
element terminates its layer's Draw walk.

`terrainPlaceMeeting` is an important exception to stationary collision state:
`RunnerSpatialInstances::place` temporarily moves the caller for its query.
`RunnerPlaceQuery` restores through the position setter, preserving dirty-queue
side effects; only its spatial branch also restores cached bounds. Neither
move changes `previous`. The spatial-owner tests retain a distinct earlier
snapshot during both list and spatial queries, including fractional query
coordinates. The gameplay-owner tests additionally exercise repeated queries
from all three terrain object types and verify authoritative coordinates and
previous coordinates after each query. These tests cover successful restoration,
not every failure path or external original-game writer.

These facts do not justify skipping all terrain updates or treating terrain
presentation as permanently immutable. Spatial coordinates are private to
`RunnerSpatialInstances`: attachment initializes both current and previous;
subsequent writes, including temporary query movement/restoration, use its
observed `move` publication. Future external `with`/dynamic setters must use
that same publication boundary, not write around the observer. Tests cover
dirty range fallback, writes before/after omitted slots, inactive promotion,
slot reuse and temporary queries. Before omitting marker
animation, establish the selected runner's one-frame sequence semantics rather
than assuming invisibility makes its state irrelevant. Wall End Step behavior
has a separate VM comparison in `verify-wall-end-step`; the local call whitelist
here is not a proof of its callees' side effects or their ordering.

## Retained wall row selection

`GameplayTerrainInstances::visitWallRows` is a conservative presentation
accelerator, not collision lookup or a new Draw order. Groups of 32 consecutive
birth-order records retain their minimum/maximum center Y. Creation prepares
the group before exposing the new record to the birth callback. Painting skips
groups outside the requested interval expanded by the maximum supported wall
radius (16 pixels), then applies the existing per-sprite row and visibility
checks in unchanged order. Coincident walls remain separate records.

Retirement preserves stable compaction. Only records crossing a group boundary
expand the destination range; obsolete extremes may remain, producing harmless
extra candidates. Appending into an empty group resets its range. Clear rebuilds
the remaining suffix even on terminal partial failure. Coordinates are immutable
in this store: any future position setter must maintain these ranges too.

Unknown wall art disables group rejection until a complete clear, preserving
the original uncullable fallback. Signed coordinate extremes use ordered
unsigned differences rather than overflowing expanded endpoints. Traversal
callbacks must not mutate the store. The range table costs 512 bytes for 2048
records; it allocates no Chip RAM and changes neither raster invalidation nor
circular bitplane scrolling.

Scene-verification builds compare the selected sequence with an independent
full-record scan before drawing; normal and profiling builds omit that oracle.

## Batched native wall composition

`TERRAIN_BATCH=1` selects an experimental masked-strip backend. It leaves the
registered wall painter compiled and unchanged. When native terrain has no
per-wall records, clipped occupancy rows supply the same above/right/below/left
frame bits as `amigaIndexedWall`. Each row carries a one-cell horizontal halo;
the previous/current/next row windows share their occupancy reads. A static
assertion pins the occupancy flags used by that projection.

The renderer binds the 26 original sprite-51 frames once, checking their 16×16
geometry, (8,8) origin and planar layout. Sources stay in the immutable ordinary
memory game pack. No shifted asset bank or runtime pixel conversion is involved.
The batch places those words on a world-aligned scratch lattice. Coverage is the
union of the two colour planes: pen 3 remains opaque black, not transparent.
Coincident tiles retain painter order, and repeating the last identical tile is
idempotent. Unsupported frames, off-lattice placements, mirrored art and other
sprites flush the pending strip before using the generic renderer.

The scratch uses 44,304 bytes of the scene bank that is otherwise idle during
gameplay: three 416×284 planes for colour and coverage. It is never a displayed
buffer. The existing Bitmap masked-copy path shifts and clips the combined strip
into the private terrain ring; physical wrapping splits the transfer. Each
transfer drains before its source can be reused. Returning to garden/style
reloads the front bank through the existing scene-owner transition. No additional
Chip allocation or change to static backing/display ownership is required.

`make test-amiga-terrain-batch` checks ordered overlapping tiles, the transfer
coordinate contract, both render targets, clipping and circular wrapping, and
full occupancy windows at map boundaries against the retained scalar painter.
`bash amiga/profile.sh terrain` is the separate native pixel oracle (use the
same gameplay/tape environment as the timing run). It compares every actual
blitter repaint with a generic CPU repaint from the same prior full-ring bytes,
stopping immediately on a mismatch. Its reference repaint and buffer comparison
are verification work; never use that build's frame times as performance results.

`TERRAIN_DIRECT=1` enables a separate experimental transaction for dirty cached
well/side-room spans. It composes sprite-51 artwork directly into the existing
native-word Fast-RAM rows, retaining the same ordered wall and tile traversal.
It neither creates a coverage surface nor reads the result back from Chip RAM.
Only a successful transaction publishes the completed rows through the existing
cache-to-backing transfer and clears their dirty flags. It allocates no additional
production storage. Circular display scrolling and both display-buffer debts
remain owned by the existing row/raster classes.

The direct attempt admits the ordinary 160-pixel viewport and 16×16 unmirrored
sprite-51 frames. Other artwork aborts the transaction without modifying Chip
buffers; the caller then recomposes the entire dirty span with the retained
painter. Black wall-background rectangles precede all artwork and are redundant
in the newly cleared cache span. Transparent pixels and opaque palette-black
pixels remain distinct. The host tests compare native row words against the
scalar compositor and test failed transactions followed by successful retries.
With `TERRAIN_BATCH_VERIFY=1`, successful native direct spans additionally compare
against the original painter; the existing full-ring oracle covers batch fallback.

The direct experiment binds a 4,992-byte host-generated bank for phase six:
26 frames × 16 rows × three 32-bit words (coverage, low colour, high colour).
It is immutable ordinary-memory data, linked only for `TERRAIN_DIRECT=1`.
The manifest pins the original source, extraction version, input pack and
generator hashes. Interior two-word placements use these prepared words;
other alignments and edge placements retain the portable shifting path.
The host oracle checks all 26 frames independently against the original
planar artwork, in addition to ordered terrain/viewport tests.

Side-view staging uses the same transaction. `prepareStage` distinguishes
`stageInBacking` (copy finished Chip rows into the cache) from `stageInCache`
(publish finished cache rows into retired staging storage). It must not read
stale staging data over a directly composed cache span. Both producer kinds
share the 16-row repaint budget, dirty-run partitioning and physical wrap;
neither acknowledges live display-buffer debts. A failed producer leaves its
run dirty and does not publish it.
