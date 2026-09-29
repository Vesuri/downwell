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

For signed 32-bit integer inputs, `assignInteger` directly normalizes the
magnitude and rounds discarded bits to nearest-even, including mantissa carry.
All such inputs are exactly representable in binary64, so this matches the
retained `assign(MotionBinary64::fromInteger(n))` path without its intermediate
construction. Zero produces positive zero; INT_MIN is handled with unsigned
magnitude arithmetic. Collision-owner integer publication, object default depth
and layer attachment depth use this path. Host tests compare 100,000 integers
with both the retained path and host float conversion; shared native fixtures
cover powers of two, ties, carries and the signed limits. Fractional inputs
continue through the existing general narrowing path. The shared lifecycle
adapters also use direct narrowing for explicit integer position/transform
writes and Q8 animation phase publication. The latter still narrows the integer
first, then divides by 256 through its exponent; its unsupported-phase guard,
missing-owner behavior and callback failure propagation are unchanged.

The new, not-yet-live skeleton timing component now narrows `imgSp` when Step
copies it to `image_speed`; subsequent custom-rate changes still leave that
Step's stored speed unchanged. Its 64 timing cases pass with the narrowed
expectations. This is not yet a complete animation implementation: advancement,
wrapping, Animation End events and sprite-type-specific getter behavior still
require recovery. Existing live actor animation remains unchanged; its rational
rate approximations must not be described as bit-exact runner animation.

## Live usual-gun projectile contract

`bulletStandard.Create` writes zero to `image_index` and double 0.3 to
`image_speed`, which the builtin setter narrows to `0x3e99999a`.
The selected machine-gun sprites 458/459 (`sprBulScatter2/1`) each contain six
consecutive unit keys and playback speed one frame per game frame; the pinned
metadata verifier checks these resources explicitly. `ProjectileAnimation`
therefore uses the selected unit-sequence timeline, not binary64 accumulation
or exact tenths. Its image and timeline coincide because this path has no
subsequent image-index writes. Both live adapters advance existing shots before
Begin Step; a shot born during Begin Step retains zero through its first Step
and Draw. Step collision/movement must not advance its image a second time.
This does not establish projectile collision-event ordering or registration
in the heterogeneous room animation traversal.

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

The traversal head is loaded from the room's active-list offset `0x80` at
`0x50db9d`, now checked explicitly by the verifier. It follows instance link
`0x14c` before calling animation, without testing the pending activation flag.
Registered shards consult `InstanceStore::animationMember`, which resolves the
lifetime generation and the maintained room list. This differs from the Step
and alarm active-flag check: pending deactivation does not remove animation
membership, and pending reactivation does not add it. Tests cover both requests,
their reconciliation, stale slot reuse and failed eligibility publication.
This is eligibility evidence, not proof of complete cross-family scheduling.

The stored-world live enemy animation pass now uses
`InstanceStore::visitAnimationChecked`, following the active room list's depth
order and stable equal-depth insertion order rather than compact storage order.
It publishes each image change using that lifetime's handle and stops at the
first failed publication. The visitor currently permits field updates only;
animation callbacks which create, remove or rebind memberships require separate
source-equivalent traversal handling. External effect families and the native
heterogeneous actor scheduler are not yet merged into this pass.

Membership must be reconciled before this pass begins. Original room entry
dispatches Room Start (event 7/4) at `0x5107f8`, then calls reconciliation at
`0x510813` and cleanup at `0x510820`; that range is pinned by the verifier.
The stored runtime publishes membership after its room-start sequence too.
Builder children are materialized at each emission callback before the builder
continues to the next cell, so phase-end reconciliation can demote dormant births
and a failed publication stops generation immediately. Deferring these births
until after End Step incorrectly leaves them on the next active animation list.
The shared builder still prepares supported scalar Create fields/RNG before
the publication callback; this does not establish original registration-before-
Create ordering for every field, child dependency or future object family.

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

## Integer sequence timeline

`SequenceTimeline` now implements the looping timeline arithmetic for unit
speed/direction multipliers. It first adds the stored speed and rounds to
binary32. Only a negative position or position at/above sequence length invokes
the source helper's two-remainder normalization, preserving its intermediate
binary32 addition of the length. It reports a boundary crossing after updating
the position; this flag is not itself a dispatched Animation End event.

Remainder is calculated from the 24-bit significand and exponent with bounded
32-bit shifts/subtractions, not repeated per-cycle wrapping or target floating
point. Supported lengths are positive integers up to 65,535, matching and
exceeding the recovered skeleton/bone lengths. Inputs are normal finite values
or signed zero; unsupported inputs fail without changing position or flags.
This is not a general implementation of all sequence modes or playback rates.

The test compares 100,000 full-exponent signed remainder/normalization cases
against host `fmod` with the runner's rounding boundaries, plus 32,000 timeline
updates across lengths 1/4/6/8 and positive, negative, zero and oversized speeds.
Signed-zero and atomic rejection cases are checked separately. Normal and
ASan/UBSan runs pass. The 68000 compile fixture has no undefined helper symbols;
this is compilation evidence, not a native execution or timing measurement.
Frame-track evaluation, index-write reconciliation, sprite changes and owner
integration remain outstanding, so playable builds are unchanged.

## Unit frame-track evaluation and write detection

For consecutive frame keys, `0x4f9014`–`0x4f906a` interpolates between the
current/next key. At the last key, `0x4f8faa`–`0x4f9011` interpolates toward
the current frame plus one over the key's length. With the verified keys
`time=i, length=1, frame=i`, both reduce to `i + (position-i)/1 * 1`.
For in-range normal binary32 positions this recovers position exactly; the
first key's final addition canonicalizes negative zero. `UnitSequenceFrames`
uses that checked specialization, not a general claim about arbitrary tracks.
It rejects negative/out-of-range timelines rather than assuming a wrap mode.

The ordinary sequence owner compares the GML-visible index against evaluated
track position at `0x50ec49`–`0x50ec8f`: subtract, round to binary32, take
absolute value, promote to double, then compare with **0.00001** at `0x8a6c28`.
Only a strictly larger difference enters index-write reconciliation. The
helper implements this detection; it does not yet implement the subsequent
inverse mapping or sprite-change semantics. This is a recovered tolerance,
not an epsilon introduced to hide a numerical mismatch.

50,804 host interpolation/threshold comparisons pass normally and under
ASan/UBSan, with a reverse-timeline-to-frame integration check. The 68000
fixture has no undefined helper symbols. Native execution and timing remain
unmeasured for this component. Frame evaluation can now follow timeline advance,
but live ownership and reconciliation still prevent enabling the new actor.

## Skeleton animation owner

The full pinned UTMT export now includes sequence tracks for all 741 sprites,
not just the initial eight actor/effect resources. Every track is a single
`GMSpriteFramesTrack` with consecutive keys `(time=i, length=1, value=i)`;
there are 3,152 keys in total, no nested tracks, messages or moments. Sequence
length equals frame count. IDs 125, 159 and 582 are empty; the positive-length
unit evaluator must not be invoked for them. IDs 399, 626, 738, 739 and 740 use
per-second playback; the other 736 use one frame per game frame. The verifier
checks each track and matches sequence playback metadata to sprite metadata.
This establishes the unit-key data domain for a shared sprite setter/evaluator,
not general arbitrary-track support, live animation parity or runtime FP mode.

`SkeletonAnimation` now joins the recovered timing state with a separate
sequence position and the idle/throw/hit sprites 208/207/205 (6/8/1 frames).
Its default sprite comes from object 120's serialized metadata. Advancement
belongs before Begin Step/alarms; ordinary Step still updates the stored speed
after that frame's advancement. This component is not yet registered in the
live instance pool.

Sprite setter registration `0x430cf6` calls through `0x433550` to `0x46bdf0`.
The latter returns immediately for the same ID. For a changed sprite it
truncates the existing image index and compares with the new frame count at
`0x46be59`. An out-of-range index resets the index to zero and, for the
unit frame track, resets sequence position to zero. Otherwise it preserves
the index. The normal-sprite branch of helper `0x46bff0` does not allocate
the special skeletal-sprite state.

`RunnerSpriteChange` shares the ordinary/unit-track reset decision with
`SkeletonAnimation` and `RunnerSpatialInstances::changeSprite`. The collision
owner installs the new ID, invalidates cached frame `0x180` to -1, applies index/
timeline resets, and marks bounds plus eligible dirty queues. Same-ID assignments
perform none of these operations. Missing/empty resources use frame count zero:
indices in `(-1,0)` truncate to zero and reset, but their sequence position is
retained because no positive-length track exists. A failed decision leaves the
sprite fields unchanged and invalidates the collision owner. The admitted domain
is normal/zero binary32 indices convertible to signed 32-bit integers and counts
through 65,535; unsupported FP conversions and special sprite state are not
silently approximated. Callers must resolve resource/frame metadata correctly.
Positive frame-count thresholds use the binary32 exponent and significand
directly. Negative indices and magnitudes below one cannot reach a positive
count; nonnegative indices at least 65,536 exceed every admitted count. Between
these limits, truncating the positive significand to an integer and comparing
with the count is exact. No floating-point threshold is constructed per setter.
`planReference` retains the integer-to-binary32 threshold conversion. Tests
compare below/at/above thresholds for all 65,535 positive counts against that
path and the independent binary64 conversion, plus arbitrary mantissas across
every exponent/sign, rejected inputs and same-ID early exits.
Tests combine this setter with object-property updates and queue publication;
production resource ownership and live collision installation remain unfinished.

`changeSpriteFromPack` resolves frame counts from the complete borrowed DWSC
table. The same-ID fast path does not consult metadata; a missing ID uses zero
frames, while an invalid pack fails and latches the owner before field mutation.
Artwork changes do not overwrite an explicit collision mask. Tests cover every
one of the 741 resources with six signed/fractional index cases, comparing reset
decisions against the independent DWSP frame counts, plus missing-ID and corrupt-
pack cases. DWSC residency and the complete production adapter remain open.

Skeleton VM events write only image index zero. The new component reconciles
that write at the next animation phase using the recovered difference
threshold. If a shorter sprite has left the old sequence position outside
the new track after an explicit zero write, it resolves that reset to zero
before advancing. The source inverse mapping at `0x50ed87`–`0x50ee2a` reduces
exactly to zero for this case. Arbitrary nonzero mismatches return failure;
they are not silently substituted with an identity inverse mapping.

32,000 host updates exercise interleaved resets, repeated and changed sprites,
rate changes and time-stop against a scalar sequence model. Normal and
sanitizer runs pass, and the combined 68000 fixture has no undefined helper
symbols. This does not establish live skeleton behavior: throw/activation/hit
presentation decisions, damage/death effects, owner lifecycle and rendering
still need connection. Native execution and timing of this component remain
unmeasured.

## Airborne player pose boundaries

`gml_GlobalScript_playerSpriteControl.asm` blocks 35–51 set the ordinary air
sprite's index from the sign of `ysp` and `floor(abs(ysp))`. Negative speeds
strictly above -2 select frame 1; exactly -2 and faster upward speeds select
frame 0. Nonnegative speeds select frames 2, 3, or 4 at the 1 and 2 boundaries.
The current shared fixed-point player applies this rule to its full signed
fifths-of-Q8 velocity, including the gravity remainder, not the truncated Q8
component. This corrects frame selection within that representation; original
binary64 motion and the exact sprite-control event phase remain separate work.
