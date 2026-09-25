# Skeleton frame comparison gates

The pinned Windows runner uses the default double epsilon `0.00001`, not
exact ordering, for the finite real/integer comparisons used by the skeleton.
`enmSkeleton Step 0` has `cmp.i.v LT` after image_index/3 and `cmp.i.v GT`
after image_index/4 and image_index/6.

Reviewed runner evidence:

- `math_get_epsilon` is registered at `0x53fe56` and its wrapper at
  `0x53ccc0` returns the double at `0x8a6c28`. The file value is `0.00001`.
  No exported assembly calls `math_set_epsilon`.
- The finite real/integer branch at `0x5783d6` promotes the signed integer
  with `cvtdq2pd` and subtracts it from the real with `subsd`.
- The six comparison selectors dispatch through `0x5797e0`. LT goes to
  `0x578401`: negate epsilon and use `comisd`/`seta` for `-epsilon > delta`.
  GT goes to `0x578421`: `comisd`/`seta` for `delta > epsilon`.
  These are strict inequalities; values in the tolerance band satisfy neither.

`SkeletonDecisions` now performs this subtraction through the integer-only
binary64 implementation and compares its magnitude/sign with the original
epsilon bits. This is intentionally scoped to finite animation frames and
the three small integer gates, not a general VM comparison implementation.

Tests cover 129 consecutive binary32 values around each gate (387 boundary
cases), the existing 4,608 decision combinations, and repeated 0.3f sequence
advances: advance 20 is slightly above six but must not finish the throw;
advance 21 is past the gate. Normal and sanitizer runs pass.

This corrects a component awaiting live integration; it does not enable the
skeleton enemy. Activation/facing predicates supplied by the eventual owner
also need the original comparison semantics, rather than rounded coordinates.
