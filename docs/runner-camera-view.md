# Published views versus camera instances

The followed instance `camMain` (object 64) is not the camera view cache.
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
Rendering still follows the
final camera position, preserving immediate visual collision snaps and native
ring-scroll deltas. This is not a complete camera manager: explicit camera
setters, dirty-mode transitions, initial room publication and remaining
script-visible view consumers need phase-aware integration. A paired replay
does not establish those cases by itself.
