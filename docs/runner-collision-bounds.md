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
`BloodMaskSample` specializes the unit-scale inverse transform and this rectangle;
its prepared trig is independent of queried pixels. Active FP mode and signed
trig equivalence still need original-runtime verification.

`BloodCollisionBounds` specializes the earlier bbox arithmetic for sprite 84,
unit scales and nearest-even conversion, returning half-open integer edges.
Its scalar test isolates arithmetic from trig: it does not independently
prove Windows trig identity, FP mode, or the sampling routine described here.
