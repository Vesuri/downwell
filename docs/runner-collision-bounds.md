# Windows collision-bound contract

Evidence is the pinned Windows executable, checked read-only by
`python3 tools/verify-runner-collision-bounds.py`. The verifier pins reviewed
machine code, not runtime results. Work priorities remain in [open-work.md](open-work.md).

## Entry points and storage

- `bbox_left` registration at `0x430cab` selects getter `0x42d500`;
  `bbox_right` selects `0x42d540`. Dirty bounds call `0x466010`.
- Instance offsets `0xe4/0xe8/0xec/0xf0` store left/top/right/bottom as binary32.
  Position is at `0xb0/0xb4`, scale at `0x9c/0xa0`, angle at `0xa4`.
- The bbox routine selects explicit mask index `0x144`, falling back to sprite
  index `0x84`. A missing resource collapses bounds to the instance position.
- `place_meeting` wrapper `0x52b1f0` converts supplied coordinates to binary32
  and calls `0x4fee30`; this is distinct from reading bbox getter values.

## Cache refresh and spatial publication

At `0x466045` the routine saves the old cached bounds before resolving the
mask/sprite. Failed lookup (`0x466077..0x4660b9`) copies position into both
endpoints, clears instance bit `0x200`, and forces the update argument to zero.
The common tail clears bounds-dirty bit `8`. It does **not** remove an existing
spatial entry or clear membership bit `0x8000` on this path.

For a resolved resource, `0x46669c` copies its precise-mask byte into instance
bit `0x200`. After clearing dirty bit `8`, a nonzero update argument invokes
`0x5020a0` with the saved old bounds. Recalculation with update zero changes the
cache/flags but leaves the index untouched. A later publication can therefore
require the index removal helper's full-root retry when the cached old bounds
no longer enclose its stored entry.

`RunnerSpatialBoundsUpdate::finishRefresh` models this finalization, not sprite
lookup or mask geometry. The reviewed ranges are pinned by
`tools/verify-runner-field-lookup.py`; component tests cover missing-resource
birth, retained old entries, restoration and update-disabled refresh.

## Unrotated ordinary-sprite branch

`0x46616d..0x466328` treats each axis in this order, narrowing every operation
to binary32 (margins are inclusive):

```
width = (high - low) + 1
near = ((low - origin) * scale) + position
far = (scale * width) + near
```

Compatibility mode rounds both endpoints to integers and converts them back
to binary32, then sorts them, then subtracts one from the far endpoint. In
fractional mode it sorts without rounding or subtracting. Zero scale can
therefore yield reversed inclusive endpoints in compatibility mode; this is
distinct from the spatial index's later endpoint normalization. Computing the
far edge independently from `(high + 1 - origin)` changes rounding and is not
equivalent.

`RunnerSpriteBounds::unrotated` implements this ordinary-sprite branch with
an explicit rounding callback. Its host tests use nearest-even and compare
16,384 cases against a separate floating-point instruction-order transcription,
including cases that distinguish reassociation. This does not establish the
original runtime's FP control word, nor cover rotated or skeletal resources.

## Rotated ordinary-sprite branch

`0x4663da..0x4664ab` subtracts sprite origins from inclusive margins, adds one
to the upper edges, and applies scale. It does not rotate the artwork's full
24×24 canvas when computing sprite 84's collision bounds: that sprite has
margin rectangle `(10,11)..(13,13)`, origin `(12,12)` and AxisAlignedRect mode.

`0x4664ab..0x4664f9` computes angle × float-pi / float-180 with a binary32
rounding after each operation. The result is promoted to binary64 for cosine
(`0x74b5f0`) and sine (`0x74b7b0`), then each result is narrowed to binary32.
This resembles the lengthdir wrappers but the subsequent bbox operations do
not apply lengthdir's tiny-fraction snapping.

For scaled local edges L/R/T/B and narrowed cosine C and sine S, the ordinary
branch sorts the separately rounded products and evaluates:

```
left   = (x + min(C*L, C*R)) + min(S*T, S*B)
right  = (x + max(C*L, C*R)) + max(S*T, S*B)
top    = (y + min(C*T, C*B)) - max(S*L, S*R)
bottom = (y + max(C*T, C*B)) - min(S*L, S*R)
```

Every multiply/add/subtract is binary32. Reassociation, double-only corner
rotation or translating already-rounded integer bounds is not equivalent.

The ordinary rotated branch sorts each pair of serialized margins before
subtracting the origin, adding one to its upper edge and applying scale.
Unlike the unrotated branch, compatibility conversion decrements the rounded
upper **integer** before converting it back to binary32. `RunnerSpriteBounds`
keeps these branches separate and accepts already-narrowed cosine/sine values.
The blood-contact adapter shares the geometry and retains half-open integer
output. General geometry tests cover mirrored scales and reversed margins;
the separate blood tests include its angle conversion and precise sampling.

## Compatibility branch and limits of this evidence

Byte `0x9360b0` changes the edge convention. If set, the bbox routine uses x87
`fistp` to convert each edge to integer and subtracts one from right/bottom.
If clear, internal bounds remain fractional and the public right/bottom
getters subtract one instead. Conversion depends on the runtime FP control word.

The `OPTN` chunk dispatch at `0x4985d2..0x4985e4` calls loader `0x47b9f0`
with the chunk payload. It recognizes the negative marker and version 2,
then `0x47bcc0..0x47bcd8` assigns the byte from bit `0x08000000` at payload
offset 8. The pinned data contains marker `0x80000000`, version 2 and flags
`0x28400214`: **compatibility mode is enabled at load**. The verifier checks
both this serialized value and the loader/dispatch machine code. This is a
static load-path conclusion, not a runtime FP-control-word measurement.

## Precise collision dispatch

Instance collision routine `0x464d30` adds one to inclusive upper bounds for
its broad overlap tests in this mode. If either sprite is precise, it calls
`0x4b8eb0`; the fractional-mode alternative is `0x4b96b0`. Both sprites' frame,
position, scale, angle and computed bounds are supplied. Frame selection is
truncated toward zero, then normalized to the sprite's frame count.

At `0x4b8fb8..0x4b8ff9`, the compatibility routine intersects the bounds and
converts the extrema with `cvttss2si`. Its loops include both endpoint pixels.
The unit-scale, near-zero-angle fast path rounds positions with x87 `fistp`
and indexes masks from origin plus pixel minus position. Other scales and
angles branch to `0x4b9227`, with rotated sampling at `0x4b93f6`. Goal fields
have scale 2, so their blood interactions cannot be justified by the unit-scale
fast path alone. Generalized integer rectangle/field tests prove their supplied
geometry only, not the rotated sampling of this latter path.

The scaled near-zero-angle path (`0x4b926a..0x4b93f6`) is distinct from the
rotated path even at angle zero. It computes each axis as
`((worldPixel - adjustedPosition) * reciprocalScale) + origin`, then truncates
with `cvttss2si` **before** testing canvas limits. Thus a sample in `(-1,0)`
becomes pixel zero and may hit. The rotated path tests the fractional coordinate
against zero first. `RunnerMaskTransform::prepareAxisAligned` preserves this
distinction; explicit negative-edge fixtures reject substituting the rotated
path with sine zero/cosine one.

At dispatch, all four scales are compared inclusively against binary32 versions
of `1 +/- epsilon`; both promoted angles must lie within double `+/- epsilon`.
The executable initializes epsilon at `0x8a6c28` to `1e-5`. This is an initial
data value, not proof of the value at every runtime call. The unit fast path's
position rounding and pair-wide dispatch are separate from the scaled sampler.

`RunnerMaskDispatch` models this pair-wide choice, with configurable epsilon
and the pinned initial value as its default. `TimeStopGoalContact` uses that
default for its unrotated ordinary/goal-field specialization: near-unit partners
round positions for sampling, while scale-2 goal fields retain scaled sampling.
Bounds are still calculated with the actual scales before choosing sampled
pixels. The specialization retains its existing nearest-even policy; this
does not independently establish the original runtime FP control word.

## Rotated sampling

`0x4b93f6..0x4b947a` computes sine/cosine of each **negated** angle with
the same float pi/division and float-result narrowing as bounds. Reciprocal
scales come from binary32 `1/scale`. Negative scales shift that sprite's
position by minus one before evaluating coordinates.

For each inclusive candidate world pixel, with `dx/dy` relative to that
adjusted position, the unit uses this order (round every operation to binary32):

```
sampleX = ((dy*S) + (dx*C)) * reciprocalScaleX + originX
sampleY = ((dy*C) - (dx*S)) * reciprocalScaleY + originY
```

Both coordinates must be nonnegative and strictly smaller than sprite width
and height, even before the optional precise-mask byte test. For that test,
`cvttss2si` truncates sample coordinates toward zero. Success requires both
sprites to pass. X is the outer scan loop and Y the inner loop. The sprite
flag at offset `0x8b` controls whether the mask byte is consulted; a false flag
does not bypass the canvas-bound checks.

`RunnerMaskTransform` shares this rotated-branch transform with the blood
sampler. It accepts the already-narrowed inverse sine/cosine, prepares reciprocal
scales and negative-scale position adjustments once, and can sample a borrowed
`SpriteMaskPack` view. Integer-only arithmetic is checked against separately
evaluated scalar binary32 expressions for 65,536 mirrored/fractional samples.
It rejects zero scales and unsupported nonfinite/subnormal arithmetic rather
than silently treating failure as a miss. Unit-scale arithmetic skips identity
multiplications/divisions; this does not select the runner's distinct near-zero
angle fast path. That dispatch, trig identity and original FP mode are not
established by the transform tests.

The loader at `0x4b7b45` maps serialized mode 1 to flag `0x89` (precise) and
mode 2 to `0x8a` (rotated-rectangle handling). Flag `0x8b` is different: it
indicates expanded mask bytes. `0x4b7e3f..0x4b7e61` retains serialized mask
count/data; `0x4bbbb0` lazily expands those bits and sets `0x8b` whenever stored
masks exist, regardless of the precise-mode flag. The collision routine calls
this expansion for both sprites. Sprite 84 has one stored mask, so its pixels
are tested when paired with precise sprite 616. The stored mask is exactly
columns 10..13, rows 11..13 inside its 24×24 canvas, not the whole canvas.

`tools/export-blood-mask.csx` exports the source mask with UTMT 0.9.2.0 using
`DOWNWELL_BLOOD_MASK_OUTPUT` and `DOWNWELL_DATA_WIN_SHA256`, following the goal
mask exporter convention. Keep output at `extracted/utmt-0.9.2.0/blood-mask.json`
and any CLI round-trip file under `generated/`, never `data/`.
`python3 tools/verify-blood-mask.py` checks the input identity and every mask bit.
`BloodMaskSample` uses the shared transform with unit scale and specializes this rectangle;
its prepared trig is independent of queried pixels. Active FP mode and signed
trig equivalence still need original-runtime verification.

`BloodCollisionBounds` specializes the earlier bbox arithmetic for sprite 84,
unit scales and nearest-even conversion, returning half-open integer edges.
Its scalar test isolates arithmetic from trig: it does not independently
prove Windows trig identity, FP mode, or the sampling routine described here.

`BloodGoalContact` covers both integral-centred goal fields (sprite 616, scale 2)
and ordinary fields (sprite 597, scale 1). It selects the sampling branch from
both sprites' scales/angles. `BloodMaskSample` then uses rounded integer
positions, unrotated fractional sampling, or the rotated transform as selected.
Bounds still use the actual blood angle, even when sampling dispatch treats it
as near zero. The stored blood rectangle's strictly positive edges make direct
interval tests equivalent to truncating coordinates and reading its verified
bits. These query adapters do not select instance identity or activation state;
the scheduler/registry must supply eligible fields.
