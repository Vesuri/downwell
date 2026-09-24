# Laser Sight source audit and implementation boundary

Upgrade 12 now has shared machine-gun firing behavior. It is not yet a complete
upgrade: the visible ray and other weapon types remain unimplemented.

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
