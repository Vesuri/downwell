# Published views versus camera instances

## Resource initialization and room entry

The selected runner's room-resource reader resolves the serialized view table
at `0x48b6e1` and passes each view record to `0x454880` at `0x48b7c3`.
That constructor converts integer view X/Y (record offsets 4/8) to binary32
camera cache fields `+0x184/+0x188`. Width/height come from offsets 12/16.
It also copies follow settings and builds projection matrices; it does not
derive the initial cached coordinates from a followed instance.

The new-room branch at `0x50fdf9` obtains the room resource and calls its copy
routine `0x487fa0`. Inside that copy, `0x4881d7` clones each source camera
through `0x4543f0`. The clone explicitly copies cached X/Y at
`0x45441e–0x454432`, before storing the new camera ID in the destination view.
Thus creating `camMain` is not itself evidence that the cached view has followed
it. In particular, entry-dither Create must not substitute the actor's Y minus
half the viewport height for `camera_get_view_y`.

The verifier pins these resource-construction and cloning paths, including
the individual coordinate loads/stores. This is static evidence, not a complete
room-entry execution trace: intervening script camera setters, persistent-room
reentry and the first follow publication still need integration/verification.
Do not generalize the new-room copy path to all room transitions.

Native room-pack format 4 retains all eight serialized view records per room,
including disabled slots, signed coordinates, port rectangles and follow
settings. `RoomPack::roomView` decodes them without adding camera objects or
changing the rendering view. The rmMain resource starts at (0,0), with a
160-by-284 view and parent-camera object 63 as its follow target. This metadata
is the input to room-entry publication, not a replacement for the subsequent
phase-aware follow/setter behavior. The additional pack storage is 11,200 bytes
(25 rooms, eight 56-byte records); the decoder does not allocate a view array.

`RoomEntryViews` narrows these integers to binary32 without host floating-point
conversion and invalidates its readable state on a failed load. StartupRuntime
loads it in room preparation before authored instance births. The production
native GameplayWorld receives a borrowed room pack and loads rmMain's cache
before player birth, independently of its followed-camera initialization. It
retains that pack across in-place run recreation. Invalid resources fail before
publishing a player. Resource-free fixtures can omit the pack; the terrain
handoff still loads the cache for those callers. The supplied pack is immutable
for the world and its run resets. The terrain handoff reuses the constructor's
initial-view snapshot only for that exact pack and rmMain; a different pack or room takes
the original decoder path, including invalidation on failure. No snapshot is
carried across world reconstruction or substituted for live camera state.
`REFERENCE_ROOM_VIEWS=1` retains the duplicate handoff decode for native timing
and replay comparison. `make verify-room-views` compares full ownership witnesses
against that retained path, including alternate resources and failure prefixes.
This does not establish full cross-family room birth ordering. Tests observe
the cache from actual
player-birth callbacks, including retry, and terrain callbacks. These are
initial snapshots, explicitly not current-frame view getters; connecting them
to entry-effect creation and the complete camera publication lifecycle remains
necessary. Each world retains eight small view records, not camera matrices.

## Frame publication

The followed instance `camMain` (object 64) is not the camera view cache.

The stored main-camera adapter publishes its resolved position to the instance's
binary32 fields and resource-backed query owner during Create, ordinary Step
and the side-locker collision correction. Pixel caches remain for existing
render consumers, but entry-effect readers use the precise fields. Rejected
owner publication leaves the record's accepted position unchanged and fails the
world. Tests check fractional positions at the actual entry-effect read and
failure at each of those publication sites. This synchronizes the implemented
camera-motion adapter; it does not validate all of that adapter's Q12 easing
against the original VM or replace the separate published-view phase rules.
Its Step precedes `objBuilder` (65), but writing its x/y does not itself
publish new values for `camera_get_view_y`.

Evidence in the untouched Windows runner, pinned by
`python3 tools/verify-runner-camera-view.py`:

- `camera_get_view_y` at `0x455e90` reads binary32 camera field `+0x188`
  at `0x455edb`. It does not perform following while reading.
- The follow routine `0x453ec0` publishes x/y at `0x4541ee/0x454200`.
  Its projection update clears camera dirty flag `+0x1e1` at `0x453b96`.
- The frame calls `0x50a1d0` after ordinary Step/engine motion, before
  Collision (`0x50decc`) and End Step (`0x50def5`). However, its follow call
  at `0x50a6de` is conditional on that dirty flag. Normal instance movement
  does not mean a dirty camera. Treating this as an unconditional publication
  would incorrectly expose this frame's target position to End Step.
- Normal Draw (`0x50da30`) calls `0x491960`, then `0x495fb0`, whose enabled-view
  path follows a valid camera at `0x496032` without that dirty-flag condition.
  Thus the normal followed view is refreshed before rendering, after collision
  snaps and End Step.

VM `gml_GlobalScript_scrBuilder` reads `__view_get` YView and height.
`gml_GlobalScript___view_get` routes YView through `camera_get_view_y`.
`gml_Object_camMain_Step_0` writes rounded instance x/y, not camera view setters.
The builder therefore reads the preceding Draw's published view during normal
gameplay. Using camMain's new position just because its object ID sorts earlier
advances generation and RNG consumption prematurely.

`Shop` End Step likewise reads cached YView for its -320/+3000 retirement
edges. `objTransDither` ordinary Step advances y by eight only while it is
strictly below cached YView + height. At equality it must stay still, even if
camMain is already moving to a lower view. These two VM entries are pinned too.

The current adapters snapshot that normal published view at update entry for
builder, ordinary/End Step lifetime checks, shop retirement, exit-dither
movement, ordinary projectile out-of-view checks and frog/walker/crawler
visibility. `scrBulletOov` uses cached YView - 100 and YView + height + 32;
`scrInView` uses cached X/Y/width/height with the caller's margins. Native
relative coordinates subtract the fixed (240,176) world origin, not a new
camera position. Crawler alarms retain their earlier phase's view too.
A horizontal crossing regression places a frog outside the old shaft view
and inside the upcoming side view: integration starts only next update.
The stored world's scoped time-shard scheduler also reads this snapshot.
`fxTimeShard` calls `outOfView(0,320)` against its published binary32 y before
writing rounded private yy back to y; unlike `scrBulletOov`, this helper does
not add view height. Equality at either edge survives. The live registration
test compares equality with a quarter-pixel outside during natural descent,
including the destroy mark and subsequent cleanup. This validates the existing
host scheduler, not production time-stop ownership on both platforms.
Rendering still follows the
final camera position, preserving immediate visual collision snaps and native
ring-scroll deltas. This is not a complete camera manager: explicit camera
setters, dirty-mode transitions, initial room publication and remaining
script-visible view consumers need phase-aware integration. A paired replay
does not establish those cases by itself.
