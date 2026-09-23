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
The current working tree also contains an uncommitted SDL downward-contact
correction; its full-suite side-room expectation failure must be investigated
before treating that change as verified.
