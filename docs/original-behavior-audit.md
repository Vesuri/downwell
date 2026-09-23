# Original-game fidelity audit

Both ports must reproduce the Windows game. SDL is a development comparison,
not an oracle. Cross-target replay proves execution consistency for its tape,
not equivalence with Windows. Historical SDL-parity notes do not supersede this.

## Default player horizontal movement

Inspected export: `extracted/utmt-0.9.2.0/Assembly/`.
`gml_GlobalScript_scrPlayerInit.asm` lines 313–355 initializes acceleration
0.2, ground maximum 2, air maximum 2.5, ground friction 0.4 and air friction
0.1. `gml_GlobalScript_scrPlMovement.asm` applies a reversal impulse of 2
when grounded versus 1 airborne, and explicitly zeros xsp when both direction
buttons are held. Its right-input branch runs last, setting facing right.

Both ports now distinguish these states. Native movement previously used the
airborne maximum, friction and reversal impulse, and treated both buttons as
neutral friction. Focused native tests assert the recovered ground maximum,
friction, reversal and immediate simultaneous-key stop independently of SDL.
Acceleration/friction still use rounded Q8 increments: this correction is not
a claim of exact integration or complete movement fidelity.

The corrected 64-frame native replay digest is `b112ca1f7174a6e1`. A changed
digest is expected when fixing movement; the source-backed assertions, not
the digest itself, establish why the behavior was changed.
The bounded 45-second A500 cross-target verifier passed all 64 frames with
that digest. Native player and port-runtime tests pass; the live entrance
comparison passes with the current in-progress collision changes present.

## Remaining confirmed gaps

- Player collision uses a landing resolver and corner probes rather than
  `scrCheckCollisionWith`'s rounded/ceil swept mask tests, including diagonal
  resolution and ceiling response. Fire/support ordering also needs review.
- Bat steering substitutes a coarse heading table and cross-product heuristic
  for the VM's point_direction, random_range and lengthdir operations. Hanging
  and hit-stun states are incomplete.
- Frog Step resets alarm 1 to 60 on the first grounded frame (`landed`), handles
  ceiling collisions, hit-stun and view gating. Native behavior is incomplete.
- Enemy/player contact uses a common proximity rectangle rather than recovered
  masks. A matching SDL result would not validate that approximation.
- Exact animation rates and sprite resource cycles are useful corrections but
  do not establish animation scheduling or all hit-state presentation.

The extended combat comparison remains a failing gate, not proof of fidelity.

## Fractional downward contact

Both `scrPlayerPlatformCollision` and `scrCheckCollisionWith` assembly compute
the vertical probe as round(yy) + ceil(abs(ysp))*sign(ysp). The SDL top-contact
helper now rounds yy, and applies to the shared top edge of walls/boxes as well
as thin platforms. At the observed (308.8,1984.2) floor contact, the old SDL
adaptation missed support and fired instead of jumping. Both ports now start
the extended tape grounded at y=1984. This fixes this contact case, not the
full two-axis/mask collision semantics listed above.

The startup test's first side room is now pattern 6 rather than 3. Generation
count 4, rows 95, one side room and zero shops remain pinned. Pattern 6 in
scrRightPat1 contains a J torch marker in addition to v,O,_,@,{,}; tests now
expect that torch and seven special markers. Pattern selection here is a
regression baseline for this adapted simulation, not a Windows reference trace.
The previous incidental kill before that checkpoint no longer occurs; an
explicit bounded 1800-step alternating-movement/fire continuation preserves
the mandatory kill/reward coverage (one kill, three gems, health 4 observed).

The extended SDL/native combat gate still fails: first >3-pixel player-Y
difference at tick 593, first kill mismatch at 601. Final health 3/3, kills
2/1 and currency 22/4. Do not widen tolerances to conceal this difference.

## Empty-ammo gunboot braking

The tick-593 divergence above was traced to the empty-charge branch, not a
floor collision: SDL reduced falling speed at tick 590 while Amiga did not.
`gml_GlobalScript_scrPlayerShootN.asm` lines 357–395 sets noAmmoYsp=3,
clamps a faster downward ysp, sets alarm 2 to 16 and enables shotDelay.
Amiga now implements that branch without creating a bullet or spending ammo.
Focused tests drain all eight rounds, verify the three-pixel fall displacement,
and verify that holding fire waits sixteen updates before braking again.
Smoke, EMPTY text and sound remain separate presentation omissions.
Full host tests pass. The bounded A500 canonical replay passes with unchanged
digest `b112ca1f7174a6e1`, and both 68000 audits are clean. That short replay
does not cover ammo depletion; the focused host test supplies that coverage.

After this correction the first >3-pixel Y difference moves to tick 666.
The kill mismatch at tick 601 remains even with player positions within a
pixel: native (172,2096), SDL (171.1,2096.7). Final health 3/3, kills 1/1 and
currency 12/4 still fail the extended combat gate. This supersedes the earlier
combat endpoint, not the outstanding mask/ordering concerns.

## Bullet muzzle coordinates

`scrPlayerInit` sets the default shootLeg to -1. `scrPlayerEmitBullet` adds it
to the supplied muzzle X and negates it on each emission. Amiga previously
always used -1; it now alternates -1/+1 for actual default-gun shots.
`scrPlayerShootN` captures instance x and y+4 before the Begin Step integrates
xx/yy. Amiga now captures rounded pre-movement coordinates instead of using
the post-movement position. SDL now uses the instance coordinates rather than
unrounded precise coordinates, with player x/y rounding using the same
ties-to-even helper as enemies.

Independent native tests check both alternating origins and a moving airborne
shot's pre-movement rounded origin followed by its eight-pixel bullet Step.
The canonical native digest changes to `bfbbd762b9b62a59`.
Full host tests and the bounded A500 replay pass with that digest; the 68000
instruction/helper audits are clean. The playable SDL build was rebuilt too.
These are default-gun corrections, not implementation of other weapon types.

The extended comparison still fails: SDL kills the later snake at tick 600
while native has not. The snake positions differ by one pixel (198/199 world
X), relevant to a bullet at the collision boundary. The inactive native bat
versus active SDL bat also needs investigation. Final health 2/3, kills 2/1,
currency 6/6: matching total currency does not establish matching combat.
