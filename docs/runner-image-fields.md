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

## Build selection and phase follow-up

The loader reads GEN8 payload into EDI at `0x4977c8`; `0x4979e8` compares
payload offset `0x2c` against 2 and sets `0xb57f18`. The pinned data has 2
at that offset, so **this build enters the GMS2 branch**, not the zero-flag
legacy entry. The older helper is not by itself proof of the selected path.

`make verify-animation-metadata` exports all 741 sprite records with pinned
input hash, extractor version/commit and stable resource IDs. Skeleton sprites
205–208 and bone 460 are normal sprites, with frame counts 1/1/8/6/4 and
FramesPerGameFrame speed 1. There are five FramesPerSecond resources:
399, 626, 738, 739 and 740. However, the skeleton/bone resources also contain
V2 sequence data. Unit speed alone does not prove that the runner bypasses
sequence handling: resolving how that maps to the `0x50ec22` pointer branch
is still required. No playable actor is enabled based on the unit multiplier.

The owner calls the animation routine at `0x50dc25` in an instance traversal,
before the Begin Step type/subtype 3/1 setup at `0x50dc6b` and the alarm-phase
call at `0x50dce6`. The component test now advances before applying ordinary
Step's speed/time-stop writes, using the previous stored speed. This phase
finding is independent of which animation branch each sprite selects.

## Sequence loop recovery (supersedes the tentative legacy binding)

The export now includes full sequence metadata for sprites 205–208 and 460.
Their sequences have playback mode 1 (loop), unit per-game-frame speed, length
equal to frame count, no moments or broadcast messages, and one
`GMSpriteFramesTrack`. Every key has time equal to its frame index, length 1,
no stretch/disable flag and one channel selecting that frame. The metadata
verifier checks every key, not merely the presence of a sequence.

In the sequence branch, `0x50ee54` multiplies stored image speed by instance
direction and sequence playback rate, then adds the result to a separate
timeline position at offset `0x88`. `0x50f9e0` retains the previous position,
updates the timeline and calls the sequence bounds helper `0x4e27d0`.
For playback mode 1, `0x4e2855` tests both negative positions and upper
overflow; either calls `0x4bbd30` and raises the boundary flag. The latter
performs a remainder, adds the length with single-precision rounding, then
takes a second remainder. This differs from the legacy single-subtraction
path, especially for the bone's negative animation speed. The frame-track
evaluation at `0x4f8e60` is separate from that timeline update.

The skeleton's temporary `advanceImage` binding to the legacy helper has
therefore been removed. Its verified alarm and builtin-speed conversion
behavior remains intact. `LegacyImageAnimation` and its path tests are kept
as explicitly legacy evidence, not used to implement these sequences.
Next work is the integer timeline/remainder implementation, track evaluation,
and synchronization when GML writes `image_index` or changes sprites. No
runtime resource is enabled based on an assumed identity mapping.
