# Depth-driven enemy spawner

Authority: object 78 (`Spawner`) and its serialized events in the untouched
Steam `data.win`, decoded with UndertaleModTool 0.9.2.0, commit
`03c7048438347799e0717b6e57a9358d000472e0`. Input SHA-256:
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`.
`tools/verify-runner-instance-ids.py` pins the reviewed assembly hashes.
This is a source contract, not evidence of live implementation.

## Ownership and initialization

`scrBuilder` creates 78 at `(self.x - 100, self.y)` before selecting its first
ordinary main pattern. It is not an enemy or terrain tile. Metadata has no
parent, sprite 702, depth zero and ordinary Step code 1048. Create code 1047
calls `spawnerInit`.

Initialization assigns depthMeasure=0, totalDepthCount=500, falldepthMem=0,
enemyMaxCount=10, fromAbove=0 and script `active`=0. The last field is **not**
runner deactivation: the object must continue receiving Step. Normal areas
1, 2, 4, 3, 5 dispatch to spawnerSet1, 2, 3, 4, 5 respectively; hard mode uses
hardSet1–4 for the first four and still spawnerSet5 for area 5.

For normal area one, enemyMaxCount becomes 20. Channel rows below are
`initial countdown / reset base / random amplitude / min count / max count / above`.
Enemy lists contain repetitions deliberately; selecting a singleton still
calls `irandom(0)`.

| Level | Channel | Row | Enemy selection list |
|---|---|---|---|
| 1 | 0 | 160 / 160 / 32 / 1 / 1 / 0 | 115 |
| 2 | 0 | 160 / 240 / 120 / 1 / 1 / 0 | 115 |
| 2 | 1 | 2000 / 360 / 32 / 1 / 1 / 0 | 118, 116, 116, 116, 116, 116 |
| 3 | 0 | 160 / 320 / 64 / 1 / 1 / 0 | 115, 115 |
| 3 | 1 | 800 / 560 / 100 / 1 / 1 / 0 | 116, 116 |
| 3 | 2 | 1300 / 800 / 100 / 1 / 1 / 0 | 118, 118, 118 |

115 is enmWhiteJelly; 118 is enmRedBall. These are not interchangeable with
the builder's existing enemy profiles.

## Ordinary Step order

1. If script active is false, activate when global.ply > self.y and area != 5.
   Continue into the active body in the same event.
2. Assign depthMeasure = global.falldepth - falldepthMem, then update
   falldepthMem, even when death/endReached/enemy-count guards prevent spawning.
3. Short-circuit on death, builder 65's endReached, then
   instance_number(83) >= enemyMaxCount. Do not substitute visible enemies or
   a family-local store count for the selected runner's instance count.
4. Walk channels in ascending index. Subtract depthMeasure from countdown.
   If countdown <= 0, reset to base + irandom(amplitude) * choose(-1, 1), in
   that RNG evaluation order; then draw spawnAmt with irandom_range(min,max).
   There is one reset per channel per event, not a catch-up loop.
5. For every attempted child, select its object with irandom(enemyMax[i]) and
   publish self.mask_index = object_get_mask(selected object) before querying.
   Caller mask publication cannot be replaced by a point/occupancy probe.
6. Select X = 160 + 16*irandom_range(2,8). For below-screen channels, compute
   snappyViewh = (ceil((viewY + viewH)/16) + 1)*16, then
   Y = snappyViewh + 16*irandom_range(1,5). Above-screen channels use
   snappyViewy = round(viewY/16)*16 and subtract the randomized Y offset.
7. Set wl=0. Short-circuit place_meeting against parentWall 57, then
   subparentEnemy 83, then (below only) tile_layer_find(100,x,y).
   If obstructed, redraw coordinates and increment wl. Below-screen retries
   use X range 1..9; above-screen retries retain 2..8. Stop when wl > 32.
8. After the loop, repeat the wall query **before** testing wl < 32. A position
   found with wl=32 is not spawned. Do not cache this final query or omit it
   when retry exhaustion is already known.
9. On success, synchronously create the selected enemy with the shared numeric
   ID allocator and its complete Create effects/RNG; only then enter the
   returned instance and call instance_deactivate_object(self). Continue the
   next repetition/channel after those effects. Do not batch or reserve IDs.

The retained variables i, spawnAmt, spawningEnemy, randxGrid, randyGrid,
snappyViewh/snappyViewy, wl and mask_index are caller state. Query/callback
failure must preserve the executed prefix rather than pretending the birth
completed. Original `falldepth` and selected view values are inputs; camera
offset or rounded player displacement is not an established substitute.
