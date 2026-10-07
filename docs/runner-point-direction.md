# Original point-direction arithmetic

`tools/verify-runner-point-direction.py` pins the executable hash, registration,
wrapper, real-argument conversion, finite atan dispatch and constants.

The wrapper at `0x53d490` narrows all four arguments to binary32 before
subtracting, then narrows both differences. For zero dx it returns 270 for
positive dy, 90 for negative dy, and zero for coincident points. Otherwise it
calls `0x74bec0`, narrows the returned double to float, multiplies by float 180,
divides by float pi, and either negates a nonpositive angle or subtracts a
positive angle from float 360. Each float operation is a rounding boundary.

## Finite atan path

`0x74bec0` supplies the atan2 dispatch table at `0x8795c0` and enters the x87
wrapper at `0x770dc0`. The classifier at `0x7710d5` saves/sets the control word,
classifies each operand with `FXAM`, and selects a handler through the table.
Positive and negative normal values both map to class zero in the lookup at
`0x8795ae`. A normal/normal pair therefore selects `0x7712e0`.

The table's mode byte is not five, so this path sets control word `0x133f`:
extended precision, nearest rounding, masked exceptions. The finite handler
takes both magnitudes, executes `FPATAN`, subtracts from the x87 pi constant
when x is negative, then negates when y is negative. The result is eventually
stored as binary64 before the GameMaker wrapper narrows it to binary32.

This is not the SSE polynomial used for lengthdir's sine and cosine, and the
existing gameplay CORDIC steering approximation is not its numerical oracle.
Zero/nonfinite classification paths require separate treatment.

## Live contact input domain

`make trace-contact-direction-domain` runs the contact and laser/combat host
fixtures with a test-only observer. It prints the Q8 player coordinates,
integer caller coordinates, world origin, and binary32 dx/dy actually passed
to atan after the wrapper's argument and difference narrowing. The observer
is absent from shipping builds. Vertical/coincident differences bypass atan
and therefore produce no observation. Paired host/native-reference simulation
can report the same contact twice; the log is not an encounter count.

Do not restrict a replacement to quarter-pixel differences. The Q8 motion
adapter's horizontal acceleration is 51/256 (`PlayerControls.h`), while
vertical integration carries fifths of Q8 between updates (`PlayerMotion.cpp`).
Neither imposes a quarter-pixel lattice. The contact at tape frame 645 has
dx bits `4125d000` and dy bits `40e66000`: 10.36328125 and 7.19921875 pixels.
These are observations of the current shared motion adapter, not proof of the
original runner's complete reachable domain.

Even the illustrative square [-32,+32] on both axes at Q8 precision has
268,468,225 input pairs: a four-byte result per pair requires 1,073,872,900
bytes before symmetry/compression. The earlier quarter-pixel probe does not
justify a smaller complete lookup. Nor do the tapes prove a maximum contact
radius for every supported caller. Any bounded table must test its admission
domain after wrapper narrowing and retain the original path outside it.

The x87 probe includes all seven distinct observed nonvertical contact/laser
input pairs and 4,096 additional independently varying Q8 offsets, besides
its existing grid. These are regression cases, not an exhaustive rounding
proof. The presentation-only approximation below deliberately does not preserve
floating-point rounding bits; the user explicitly chose meaningful visual
precision for damage-number direction. This exception does not authorize
approximating collision, damage, RNG, or other gameplay decisions.

## Compact presentation direction

### Exact contact-coordinate admission

`ContactCoordinates` removes the world-origin addition/subtraction round trip
only when all four absolute contact coordinates lie in [-65536,65536] on the
Q8 lattice. Every such coordinate has an integer numerator of magnitude at
most 2^24, so the wrapper's binary32 argument narrowing changes no bits.
Subtracting two admitted coordinates is exact in binary64. The difference
has a signed numerator bounded by 2^25; narrowing that integer and scaling
by 2^-8 therefore reproduces the wrapper's binary32 difference, including
nearest-even ties. All values are normal or zero, so scaling introduces no
underflow/overflow boundary. Admission sums use int64, including caller plus
origin, before any narrowing. A rejected admission publishes nothing.

The exact absolute player coordinates still feed synchronous damage-number
creation. Only the already-narrowed differences enter `fromDifferences`, which
retains the axis cases, atan selection and every subsequent degree rounding
boundary. This does not extend the table-atan domain or its motion certificate.
`REFERENCE_CONTACT_COORDINATES=1` selects the full coordinate wrapper on both
builds. `make verify-contact-coordinates` checks boundary, fractional, signed
and independently varied origins against that wrapper.

Both builds use `TableAtan.h` for contact damage-number direction only.
`DOWNWELL_PORTABLE_CONTACT_ATAN` (Amiga: `TABLE_CONTACT_ATAN=0`) selects the
retained calculation, which also remains the fallback for inputs outside the
table adapter's admission domain. `build/contact-direction-table-test`
compares this adapter against a host running the retained calculation.

`tools/generate-atan-table.py` generates 257 unsigned Q16-radian samples of
atan on [0,1], using Decimal arithmetic. The table occupies 514 bytes, plus
two quadrant constants. Its manifest records the generator hash and precision;
it contains mathematical constants, not extracted game assets. Magnitude
ordering reduces atan2 to this interval. Integer division provides a Q24
ratio; eight bits select a segment and sixteen interpolate its endpoints.
The adapter admits normal finite binary32 inputs with exponent differences
at most eight. Axis/unsupported cases use the retained implementation.

The conservative error in Q16 radians is endpoint quantization <=0.5,
interpolation truncation <1, interpolation curvature <=0.125 (using
|atan''|<1 and spacing 1/256), ratio truncation <1/256, and at most two
quadrant constants <=1 in total. Thus error is <2.629 Q16 units, or
0.00230 degrees. Conversion through the original binary32 degree wrapper
adds less than 0.0001 degree, leaving the total below 1/256 degree.
There is no fallback merely to reproduce floating-point rounding bits.

`make probe-table-atan-x86-macos` checks a million finite input pairs against
the recovered x87 sequence, the complete contact-wrapper grids and observed
contacts, and the damage-number motion for 60 ticks. Direction has a 1/256
degree tolerance; horizontal text motion has a 1/64 pixel tolerance; vertical
motion and lifetime remain exact. Tests do not promise identical raster pixels
at every quantization boundary. The original direction affects only the
number's initial horizontal speed, not health or player movement.

## Reproducible probe and limits

### Damage-number lookup domain

The object-221 Step bytecode calls `lengthdir_x(2, enemydir)` only while
`xsp == 999`. `scrDmgCalcFxPlayer` assigns its second argument to `enemydir`
after synchronous Create. Neither instruction stream rounds that argument to
an integer angle. The current contact adapter supplies the result of
`RunnerPointDirection`; arbitrary fractional directions also remain legal for
the effect's standalone creation API.

For the current table-atan admission path, the intermediate signed angle is
Q16 radians. An exhaustive **superset**, not a tape-derived whitelist, is
every signed integer magnitude from zero through the generated Q16 pi
constant, 205887. Passing those 411776 inputs through the retained binary32
degree wrapper produces 411776 distinct direction encodings (signed zero is
significant). Passing each through the retained degree-to-radian wrapper,
portable recovered cosine program and `lengthdir_x` component produces 389145
distinct binary32 speed encodings. A flat speed table alone would consume
1647104 bytes, before keys or lookup metadata. Deduplicating speeds does not
make this a small table.

`make probe-contact-motion-domain` reproduces these counts using the portable
integer arithmetic, without host trigonometry or floating-point identities.
It also checks that each returned speed is exactly promoted binary32. The
ordered direction/speed digest is `87bbbbf9`; the probe exports no asset bank.
Inputs outside table-atan admission, including its axis/fallback cases, are
not covered by this finite superset and must retain the portable path unless
separately proven. A table indexed by rounded integer degrees would change
the current direction, velocity and subsequent position writes.

### Lossless contact-motion representation

`tools/generate-contact-motion.cpp` encodes the finite superset above as 403
four-coefficient predictors plus a sorted exact-correction map. Predictor
coefficients are fitted from evaluations of the retained recovered program,
not host `cos`. They are only a compression mechanism: the generator evaluates
all 411776 inputs with both the retained program and the final integer decoder,
and rejects any differing binary64 speed. Predictor error is never accepted as
a new precision policy. The current bank has 132 corrections and occupies
13952 bytes in ordinary memory, with no Chip allocation.

`make generated/contact-motion.inc` generates the ignored bank and a manifest
with original input, generator and arithmetic-source hashes. The decoder uses
68000-compatible integer limbs and the original component narrowing/snapping.
`make verify-contact-motion` additionally checks 2070 complete 60-tick
trajectories and the mutation/fallback guards.

The contact adapter certifies a direction only when `tableAtanCandidate`
succeeds. The effect keeps that admission certificate separately from its VM
variables; a changed direction or a new argument assignment invalidates it.
Decoding still occurs at the sentinel-controlled first Step, never during
Create. Non-contact births, axes and portable-atan fallbacks retain the original
program. `REFERENCE_CONTACT_MOTION=1` on either build (or the matching
`DOWNWELL_REFERENCE_CONTACT_MOTION` define) forces that program for comparisons.

`PortableAtan.h` is a mathematical candidate, not x87 emulation. It reduces
the ratio to [0,1], transforms ratios >=1/2 around pi/4, evaluates an alternating
series with integer-only binary64 operations, and applies quadrant corrections.
`DivideBinary64.h` performs nearest-even normal division with two 32-bit limbs.
The candidate's binary64 result is not promised to match FPATAN bit-for-bit;
the required observable comparison here is its narrowed binary32 result.
The probe compares that result directly for its first 10,000 finite inputs.
`make verify-portable-atan` additionally checks arithmetic, quadrants, signed
zeros and broad float exponent ranges against a host reference. Neither test
constitutes exhaustive proof or a native frame-budget measurement.

`make probe-runner-atan-x86-macos` runs a finite one-million-input probe with
the recovered x87 operation sequence and compares its narrowed result with
host libm. It requires an x86-capable macOS execution environment (Rosetta on
Apple Silicon). The ordinary build and tests do not require that environment.
It also compares complete `RunnerPointDirection` results for contact offsets
from -32 through +32 on both axes, at world origin (240,176), with player
fractions 0, 1/4, 1/2 and 3/4. This includes the wrapper's argument narrowing
and degree conversion, not just its atan dependency. It does not validate
the subsequent lengthdir impulse, player integration or collision scheduling.

The probe is hand-written numeric instrumentation, not an execution of the
Windows binary. Rosetta's implementation of x87 and a finite sample cannot
prove every original CPU result. It provides a comparison gate for developing
the portable implementation, not permission to silently substitute host atan2
or approximate CORDIC in the Amiga build. Whole-game fidelity still requires
original-runner encounter evidence as specified in [remaining work](open-work.md).
