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
