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

The option-loading code at `0x47bcc0..0x47bcd8` assigns this byte from bit
`0x08000000` of the structure word at offset 8. Selecting the deployed branch
requires tracing that structure to serialized options or observing the runner;
the instruction hash alone does not prove the byte's runtime value. Likewise,
the subsequent precise-mask collision routine must establish how fractional
bounds become sampled pixel intervals. Do not wire blood collisions using an
assumed edge convention merely because an integer field-mask helper passes.
