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
coordinates from the birth position. Nevertheless, its room animation walk
still validates each terrain lifetime and captures previous coordinates in
room order. Activation and retirement must remain observable, including the
source rule that an inactive element terminates its layer's Draw walk.

These facts do not justify skipping all terrain updates or treating terrain
presentation as permanently immutable. Before eliminating the repeated
snapshot operation, audit external `with`/dynamic setters and every coordinate
publication path, including temporary collision queries. Before omitting marker
animation, establish the selected runner's one-frame sequence semantics rather
than assuming invisibility makes its state irrelevant. Wall End Step behavior
has a separate VM comparison in `verify-wall-end-step`; the local call whitelist
here is not a proof of its callees' side effects or their ordering.
