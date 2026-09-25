# Animation field storage in the Windows runner

Read-only inspection of the pinned `Downwell.exe` (SHA-256
`340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085`)
establishes a narrowing boundary that VM assembly alone does not expose:

- `image_index` registration at `0x430d0c` binds setter `0x432170`.
  It converts the double with `cvtpd2ps` at `0x43219d` and stores a single
  float at instance offset `0x94`.
- `image_speed` registration at `0x430e0e` binds setter `0x432250`.
  `cvtpd2ps` at `0x43227d` narrows the double and the next instruction stores
  a single float at offset `0x98`.
- The speed getter `0x42dd90` loads that single at `0x42dd9f`, promotes it
  with `cvtps2pd`, and returns a VM double.

Thus custom `imgSp = 0.3` and builtin `image_speed = imgSp` are not the same
64-bit number when read back: the latter is the promoted binary32 0.3.
The custom variable remains the VM double. `verify-runner-image-fields.py`
guards the reviewed binary regions; it does not execute the original runner.

`RunnerImageValue` implements round-to-nearest-even narrowing and exact
promotion for normal finite values and signed zero, using 32-bit integer
operations. Subnormal/nonfinite/overflow inputs are explicitly unsupported
and leave the destination unchanged. Tests compare 100,000 normal values,
including tie neighbors and both signs, against host conversion. This uses
the normal nearest-even floating-point mode; changes to runner FP mode have
not been independently audited.

The new, not-yet-live skeleton timing component now narrows `imgSp` when Step
copies it to `image_speed`; subsequent custom-rate changes still leave that
Step's stored speed unchanged. Its 64 timing cases pass with the narrowed
expectations. This is not yet a complete animation implementation: advancement,
wrapping, Animation End events and sprite-type-specific getter behavior still
require recovery. Existing live actor animation remains unchanged; its rational
rate approximations must not be described as bit-exact runner animation.

## Legacy update path

The branch at `0x50ec00` selects `0x50ef0c` when global byte `0xb57f18`
is zero. The latter calls the index getter, adds stored single-precision speed
with x87 `fadd` and rounds back to single at `0x50ef19`. It then compares the
stored index with `image_number` (`0x468ee0`). For ordinary sprites this getter
returns the sprite's frame count. When index is below count, `0x50ef40` jumps
directly to return, including negative indices. On upper overflow it subtracts
count once, stores at `0x50efac`, then dispatches event type/subtype 7/7
(Animation End) via `0x444910`. There is no repeat-until-in-range loop.

`LegacyImageAnimation` transcribes that ordinary path with integer arithmetic,
including single-rounding after addition and subtraction. The caller receives
the event flag only after the new index is stored. Forty thousand scalar
comparisons cover positive/negative/zero speeds, five frame counts and speeds
exceeding a whole animation; normal and sanitizer runs pass. Skeleton timing
now exposes this advancement and checks 100 updates with interleaved time-stop.

Scope: the helper admits positive integer frame counts up to 65,535 and normal
finite/zero fields. It does not implement skeletal sprites, dynamic frame-count
rescaling, GMS2 playback-rate branches, drawing negative frame indices or the
phase in which the owner calls advancement. The source byte selecting this
path is assigned at `0x4979ec` from a loaded-field comparison against 2;
tracing that field all the way to the serialized version is still needed
before calling path selection independently proven for every resource.
Thus these are source-path tests, not original-runner execution or live actor
parity. The region hashes now also guard the branch and legacy update body.
