# Builtin direction storage and setter

Read-only inspection of the Windows executable pinned by
`tools/verify-runner-direction-value.py` establishes:

- Registration `0x430b76` binds setter `0x431db0` and getter `0x42d850`.
- The setter narrows the VM real to binary32 at `0x431dd9` **before** calling
  `0x46b3d0`. That routine promotes the float, calls the CRT `fmod` dispatcher
  with 360.0, and stores the result as binary32 at instance offset `0xc8`.
- Negative remainders receive a binary32 addition of 360. Negative zero does
  not take that branch. A sufficiently small negative input can therefore
  produce exactly 360.0f; there is no second modulo after this addition.
- The getter promotes the stored binary32 value back to a VM real.
- The setter then calls `0x466910`, which recomputes engine horizontal and
  vertical speeds from direction and builtin speed (`0xcc`). It narrows angle
  arithmetic and trig outputs, snaps components near integers, sets dirty bit
  8 and calls `0x501e50`. This is not equivalent to assigning a heading alone.

The CRT normal finite dispatch selects `0x74beb0`: swap operands, repeat
`FPREM` while C2 is set, then discard the divisor. The period is exactly 360.

`RunnerDirectionValue::normalize` covers only the **numeric stored value**,
under the project's nearest-even binary32 assumption. It first narrows using
`RunnerImageValue`; for magnitudes at least 360 it computes the exact remainder
of the float's integer significand and power of two. Smaller magnitudes retain
their original bits. Negative adjustment then narrows the sum to binary32.
Signed zero is preserved. Unsupported numeric inputs fail without publishing.

The test compares finite binary32 patterns and directed binary64 narrowing
cases against host float conversion, `fmod` and float addition. This is not
Windows runtime FP-mode verification, nor a complete direction setter: live
owners must additionally publish the engine-component and dirty-state effects.
In particular, white jelly custom `xsp`/`ysp` are not those engine components.
