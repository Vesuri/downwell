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

## Reproducible probe and limits

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

The probe is hand-written numeric instrumentation, not an execution of the
Windows binary. Rosetta's implementation of x87 and a finite sample cannot
prove every original CPU result. It provides a comparison gate for developing
the portable implementation, not permission to silently substitute host atan2
or approximate CORDIC in the Amiga build. Whole-game fidelity still requires
original-runner encounter evidence as specified in [remaining work](open-work.md).
