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
