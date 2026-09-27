# Usual-gun motion arithmetic

The authority is object 261's `bulletStandard` Step assembly, not host libm
or the previous fixed-point trajectory. `verify-machine-gun.py` checks both
usual-gun profiles (speed 8, spread 3), Laser Sight's birth-time override,
initialization, muzzle Create and solid-query/movement order against the pinned
UTMT 0.9.2.0 export.

The first Step computes `bDir += random_range(-bdirRand,bdirRand)`. The reviewed
`random_range` wrapper consumes one word even for zero spread. Width 6, lower
bound -3 and addition to 270 are retained as separate binary64 operations.
`bDir` remains a custom real; `image_angle` is separately narrowed to binary32.

Both lengthdir wrappers narrow speed and degrees to binary32, multiply degrees
by float pi, divide by float 180, evaluate the recovered trigonometric program,
narrow its result, multiply by speed and apply the original tiny-fraction snap.
`verify-runner-smoke-geometry.py` pins those wrappers/constants;
`export-smoke-trig.py --check` pins the shared program. These names reflect the
first consumer, not different GameMaker functions. The admitted speed interval
extends through 8; the existing angle/radian restrictions are unchanged.

`MachineGunSourceMotion` preserves absolute-room binary32 x/y and binary64
xsp/ysp. `endpoint()` returns promoted x+xsp and y+ysp without narrowing for the
solid line query. `commitPosition()` models the later coordinate arithmetic,
including a binary32 write on every update. It is an arithmetic operation, not
a replacement for publishing the source's sequential X and Y mutations to the
spatial owner. Translating to the native local origin before narrowing is not
equivalent. Backend offsets must be applied after original coordinate storage.

This component is not yet the live trajectory owner: the live Q20/Q32 adapter
must be replaced together with its collision endpoints, angle consumers,
muzzle-effect creation and setter publication. Neither a common implementation
nor its scalar tests alone prove full encounter fidelity. Tests compare
integer-only arithmetic with scalar execution of the recovered trig program,
not a claim that a fresh Windows execution has been captured. The original
runtime FP-environment assumptions remain part of the open work queue.

Run `make verify-machine-gun-source-motion verify-smoke-direction
verify-smoke-trig`. The source-motion test covers birth narrowing, raw RNG
endpoints, ordinary/Laser Sight initialization, repeated coordinate writes,
unrounded query endpoints, negative and deep coordinates, unsupported inputs,
and the non-equivalence of rounding at the native local origin.
