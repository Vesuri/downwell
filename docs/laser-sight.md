# Laser Sight source audit and implementation boundary

Upgrade 12 now has shared machine-gun firing behavior and a ray rendered by
both backends. It is not yet a complete upgrade: other weapons, full collision
families and original GPU line coverage still need implementation/validation.

## Original evidence

UTMT 0.9.2.0, source data SHA-256
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`:

- `scrPlayerInit` VM initializes `shootLeg=-1`, then sets it to zero if
  `pugLasersight` is owned. This happens on player creation, not every shot.
- `scrPlayerEmitBullet` adds that offset to the muzzle and negates it. Zero
  therefore remains centred; ordinary shots alternate -1 and +1.
- `scrPlayerShootN` calls the emitter, then overwrites `myBullet.bdirRand=0`
  when the upgrade is owned. A previously emitted projectile is unaffected.
- `bulletStandard` still executes its unconditional first-Step
  `random_range(-bdirRand,bdirRand)` before computing velocity.

The zero-width range behavior was checked in the actual Windows runner,
`data/Downwell.exe`, SHA-256
`340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085`.
The `random_range` registration points to VA `0x53d840`. Its two bound-order
branches converge at `0x53d8a4`; the call at `0x53d8b4` goes to `0x438e60`
before multiplying by range width and adding the lower bound. Both paths in
that helper call the WELL routine at `0x438dc0` once. Equal bounds have no
early return. Therefore zero spread must still consume one random word.
`verify-machine-gun.py` pins the reviewed executable and the VM assignments.
No executable or recovered payload is altered or committed.

## Shared implementation and checks

`MachineGunEmitter` owns the instance-local muzzle alternation. Both adapters
reset it from inventory on player creation/next-room entry. Granting the
upgrade mid-room does not retroactively rerun that initializer. Trajectories
capture a `zeroSpread` flag at birth, retain the normal initialization draw,
and then use exact direction 270 and velocity (0,8).

Focused tests cover centred/reused emitters, RNG extremes, exactly one draw,
and reuse without leaking the override. The paired live combat fixture grants
the upgrade mid-room and compares complete projectile/RNG state. The paired
shop/level-clear fixture owns it across both room transitions and checks the
centred newly created emitters. The native weapon fixture also exercises four
centred zero-spread shots and their RNG counts; it is a focused CPU check, not
a native full-upgrade playthrough or rendered laser test.

## Remaining work

### Shared stepped-ray foundation

`LaserSightRay` now provides a platform-independent endpoint tracer. It
queries the starting point first, short-circuits wall before enemy queries,
advances in source-sized eight-pixel vectors, and tests the strict view-bottom
limit after advancement. Nonzero aim changes both components; player X speed
changes only the horizontal component. Q32-turn input uses the existing Q20
trajectory trig approximation, with positive vertical progress as a caller
precondition (the ordinary downward aim range).

`make test-laser-sight-ray` is included in `make test`. Tests cover initial
overlap, wall-before-enemy short-circuiting, stepped rather than exact-contact
endpoints, equality at the view bottom, fractional/negative coordinates,
opposite horizontal speeds, and aims -30/-15/0/15/30 with speeds -5/0/5.
An independent binary64 trig calculation bounds each tested component's error
below 0.001 pixels per step; the existing Q20 representation is not claimed
to reproduce the Windows runner's binary64 rounding exactly. Host sanitizer
tests pass. `verify-machine-gun.py` pins the reviewed playerDraw VM blocks
23–34, including collision parent IDs 57/83, flags, loop ordering and draw
anchors. Logs: `/tmp/downwell-ray-{test,vm,sanitized}.log`.

The tracer replaces cumulative queries with queries of each new segment only
for pure queries on a stable collision snapshot. Earlier empty segments then
cannot add a hit. Platform adapters must preserve that condition and the
original object-family selection; do not reuse a generic solid-terrain query
without auditing its membership. The current rectangle primitive uses the
port's half-open contact convention; original `collision_line` rounding and
mask behavior remain an explicit integration gate.

Both renderers now use this foundation. Two native captures match the host
as described below; original-GPU pixel fidelity and the native frame budget remain open.

### Collision-family audit

The serialized object ancestry confirms that `parentWall` includes the normal
wall (60), breakable box (92), crate (254), shop table (278), and shop case
(279). It **excludes** the thin platform (93). All five included objects have
`Solid=false`: that resource flag cannot select ray targets. The six currently
implemented area-one enemy types and the hostile shopkeeper (284) descend from
`subparentEnemy`. The verifier now checks the complete 23-member wall family,
these enemy memberships, thin-platform exclusion, and the selected default
collision rectangles directly against the pinned export.

Consequently `amigaResolveTerrainSegment` alone is not a complete ray wall
adapter: its indexed terrain coverage does not supply the crate and shop
instances. Those need live-instance queries, including removal/state changes.
The table's default relative rectangle is X=-30..29, Y=0..7; the case is
X=-9..8, Y=-9..8 (inclusive source margins). Neither is a generic 16x16 tile.
Object masks take precedence over visual sprites; instance overrides, scaling
and sprite changes still require inspection before using default rectangles.
One concrete override is now VM-pinned: `shopCase` Step, when destroyed and
not yet opened, switches `mask_index` to `noMask` (582). A ray adapter must not
keep using its resource-default case rectangle after that transition.
These metadata checks do not resolve the Windows runner's line-edge rounding
or constitute a rendered-ray verification.

`ShopWorld::rayWallIntersects` supplies a pure shared segment query for the
currently modeled shop children, in room coordinates. It reuses their live
rectangle visitor, excludes the counter/friendly keeper, respects invisible
wall retirement, and retains table/case masks after purchase or counter
retirement. Tests independently enumerate 6,000 point positions, trace rays
into a case and through the one-pixel gap to the table, and check fractional
edges and query side effects. Focused tests and ASan/UBSan pass. The shared
half-open rectangle convention is retained, not newly certified as Windows
line-edge behavior. Destructible cases are not yet represented in `ShopWorld`;
their mask transition must be added to the common visitor with that feature.
The host ray renderer calls this query through its runtime adapter.

### Live runtime endpoint adapters

Both `StartupRuntime` and `AmigaPortRuntime` now expose `traceLaserSight` with
the same explicit room-coordinate origin/view-bottom and Q32 angle inputs.
They query the implemented area-one walls, active breakables, shared shop
fixtures, and active area-one enemies. Native local coordinates are translated
without dropping the Q20 fractional part. Unlike bullet impact callbacks,
these queries do not damage, deactivate, reward, or advance actors. Enemy
membership is based on active instances, not a new health-based filter.

The existing 600-tick upgraded combat replay compares endpoints every 30 ticks
for straight, +15-degree and -15-degree rays with fractional origins. Both
adapters agree and leave RNG draw counts unchanged; the replay retains health
4, kills 3, currency 24 and best combo 1 on both targets. The ordinary Amiga
build passes its 68000 instruction and 64-bit-helper audits. Logs are
`/tmp/downwell-ray-adapters-{build,test,amiga}.log`.

The runtime convenience endpoints now use each player's actual Q8 position
and horizontal speed and the published camera bottom. Shared `fromQ8` retains
negative-coordinate fractions; `turnsFromQ8Degrees` truncates exactly to Q32
turns using bounded 32-bit operations. Tests compare all 184,321 Q8 values
from -360 through +360 degrees and signed 32-bit extremes to an independent
64-bit oracle; position conversion is reconstructed exactly as well. The
live replay also compares these actual-player endpoints (not just injected
origins). Focused and sanitizer tests pass, as does the ordinary native build.
Logs: `/tmp/downwell-ray-input-{build,test,amiga}.log`.

These endpoint APIs deliberately do not decide whether to draw. The renderers
handle ownership, goalStop, line rasterizing and painter order; noDraw/Cat and
actual player aim control are still missing. This covers currently modeled area-one
instances, not all 23 wall-family objects or the hostile shopkeeper. Crates,
other unimplemented wall types, destructive shop state and exact Windows
collision-line edges remain fidelity gaps, not implied support.

The original `playerDraw` places the ray outside the `!dFlash` player-sprite
branch, but inside `!goalStop && !noDraw`. Therefore presentation must not use
the existing damage-blink `playerVisible` gate for the ray. The current
convenience endpoints use zero aim, as does the implemented machine-gun
control path; they do not claim to implement the original aim controller.

### Host presentation

`Game::renderMain` now draws the owned ray after the player and before bullets,
independently of damage blinking and suppressed by goalStop. The unimplemented
Cat/noDraw interaction is not silently equated with the damage-blink flag.
`LaserSightLine` supplies a shared inclusive integer line rasterizer and the
source's `(x,y+8)` start anchor. Endpoint rounding uses nearest/even; this is an
explicit port presentation convention, not verified Windows D3D line coverage.
The existing renderer clip confines its red pixels to the game area.

The real host render tape compares owned/unowned output during hidden and
visible damage phases and checks no render-time RNG draws. Line tests cover
1,089 endpoint differences (all octants, point, horizontal and vertical), pixel
connectivity, endpoint inclusion, and negative half-pixel anchors. Focused
tests and line ASan/UBSan pass. Logs: `/tmp/downwell-laser-render-{build,test}.log`.

### Amiga presentation and sparse restoration

The Amiga actor list now contains a ray marker after the player. Hardware
allocation stops at that marker: earlier actors use the masked planar pass,
while later bullets/gems may still use hardware sprites. This keeps earlier
hardware sprites from incorrectly covering the planar ray. That extra BOB
work is a performance cost. The upgraded profiling tape remains slow (19.96
FPS at the latest measured checkpoint); see `amiga-profiling.md` for the
separate tracing, restoration and drawing phases and measured optimizations.

Each display buffer owns an `AmigaLaserSight` sparse overlay (3,552 bytes of
changed-byte storage). It saves only changed pixel bits, never edits the pristine
terrain backing, restores before terrain/actor painting, and draws at the
marker after draining pending blits. Ray pixels do not dirty entire terrain
rows. An identical ray with untouched underlying pixels performs zero writes;
already-red pixels are neither saved nor rewritten. Pending dirty rows and
overlapping BOB bounds invalidate retention. Full scene rebuilds discard old
overlay history rather than restoring stale pixels over a new scene.

Renderer tests cover both directions, clipping, point/horizontal/vertical
lines, ring origins 0/103/319, byte-exact restoration, newly changed underlying
terrain, unchanged-ray zero writes and already-red zero writes. The complete
renderer test and ASan/UBSan pass, including 22,834 existing offline-sprite
comparisons. The ordinary native build passes its 68000 audits.

`LASER_VERIFY=1` grants the upgrade after the initial landing in the native
scene fixture. `DOWNWELL_CAPTURE_LASER=1` does the same in the host capture.
Two 45-second-bounded A500 runs captured ticks 180 and 203; both compare with
**0 differing pixels out of 99,968** against SDL. Normal build settings were
restored afterward. `make verify-amiga-gameplay-laser-frame` reproduces the
first comparison and restores the ordinary build on success. Logs:
`/tmp/downwell-laser-native-check.log`, `/tmp/downwell-laser-recharge-native.log`,
`/tmp/downwell-ray-overlay-{build,test,sanitized}.log` and
`/tmp/downwell-laser-restored.log`. These are host/native parity captures,
not Windows GPU oracle captures or proof that frames fit in 20 ms.

`playerDraw` traces cumulative lines against `parentWall` and `subparentEnemy`,
starting at the player origin. Each iteration advances Y by
`lengthdir_y(8,270+aimAngle)` but X by
`lengthdir_x(8,270+aimAngle-xsp)`, then checks the view-bottom limit. It draws a
red line from `(x,y+8)` to the resulting endpoint. It is not simply a vertical
line to the closest wall, and rendering it must not mutate simulation RNG.

`bulletRanged` extends `decTimer` by `floor(decTimer/3)` at creation. Other
emission patterns can create several bullets while `myBullet` refers only to
the last one; their Laser Sight overrides must follow VM order rather than
blindly applying the machine-gun rule to all projectiles. Aim, ray rendering,
ranged lifetime, original event ordering and A500 frame costs remain open.
