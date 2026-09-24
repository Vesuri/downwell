# Amiga display contract

The native target follows the recovered Windows 4:3 presentation rather than
stretching the 160-pixel game surface.

## Geometry

- Hardware display window: 352x284 PAL lores pixels.
- Logical canvas: 380x284 pixels, retained for one-to-one porting.
- DMA backing width: 384 pixels (24 words, 48 bytes per plane per line).
- The display window exposes logical pixels 16..367.
- Native drawing coordinates still map directly to backing coordinates.
- The game well is 160x284 at visible x=110..269, matching the host reference.

The display setup must derive and assert its DIW and DDF values in one place.
The intended OCS/ECS 16-bit fetch is 22 words per line; it must not inherit an
AGA fetch mode from the operating system.

The 22-word fetch is positioned at DDFSTRT=$30 and DDFSTOP=$D8, the documented
rightmost hardware limit. Its matching 352-pixel window is HSTART=$71 through
HSTOP=$1D1 in the extended horizontal coordinates. Logical x=0 has beam-space
origin $61, so logical x=16 begins at the window's $71 left edge. Each
bitplane pointer begins two bytes into its 48-byte backing row and each modulo
is four, thereby omitting one 16-pixel word at both edges.

This fetch has an explicit sprite cost. The Hardware Reference Manual states
that moving DDFSTRT from the normal $38 to $30 disables sprite DMA channel 7.
Thus the 22-word layout preserves automatic DMA for channels 0..6. Preserving
all eight requires the 21-word, 336-pixel `$38..$D8` fetch; channel 7 cannot be
claimed as an automatic-DMA sprite while the 352-pixel fetch is active.

## Dual playfield ownership

Four lores bitplanes are split 2+2:

| Layer | Hardware planes | Palette | Ownership |
| --- | --- | --- | --- |
| PF1 | BPL1, BPL3 | COLOR00..03 | Game well |
| PF2 | BPL2, BPL4 | COLOR08..11 | HUD and side panels |

PF2's 384x284 bitplanes each occupy a contiguous 13,632-byte region. PF1 uses
two 384x320 circular bitplanes. The 22-word crop advances 44 bytes per fetched
line, so BPL1MOD and BPL2MOD are both four and each hardware pointer lands on
the same two-byte inset of the next 48-byte backing row.

PF2 has priority over PF1. Pen zero remains transparent on both layers, with
COLOR00 black, so the two layers can use white and red independently without
painting opaque black rectangles over one another.

The 36 rows hidden below PF1's 284-line viewport are terrain lookahead. The
renderer composes the next 16 world lines there as one block; vertical camera
movement then consumes those rows solely by advancing the circular bitplane
pointers. After 16 downward pixels it refills the next hidden block. Upward
movement and horizontal side-room changes invalidate and rebuild the lookahead
so stale terrain can never enter the display.

PF1 is cleared outside the central well. PF2 is cleared inside the well except
for intentional overlays. The tablet-border art belongs to PF2 and straddles
the well edges exactly as recovered: x=-2 and mirrored x=162 in 160-pixel
surface coordinates.

This is the hardware counterpart of the SDL renderer contract: room, menu,
style, enemy, and shop drawing is clipped to the centred 160x284 aperture;
the clip is released only for 4:3 HUD drawing.

## Sprites

Hardware sprites sit in front of both playfields. Initial channel allocation:

- player: one attached pair when four colours are insufficient, otherwise one
  channel;
- bullets, gems, and small effects: remaining channels 0..6 where their
  width, palette, and multiplexing lifetime fit;
- enemies: candidates only after the playfield implementation is correct.

Every unused channel points at an armed-safe null sprite on every frame. Sprite
priority is programmed explicitly together with PF2-over-PF1 priority; it must
not depend on an operating-system value left in BPLCON2.

## Vertical ring and buffering

PF1 is a single 320-line circular raster. The camera's integer vertical offset
selects its first physical row by changing the BPL1/BPL3 pointers. If the
284-line visible span crosses the end of the allocation, the copper waits until
just after the final data fetch of the preceding scanline and repoints only
BPL1/BPL3 to the ring base. A line-255 wait barrier is emitted when the split
crosses the copper's eight-bit vertical comparator.

Scrolling must never repaint the complete viewport. Before advancing the
published origin, the renderer clears and draws only the rows or complete
16-pixel block bands about to become visible at the bottom. The 36 hidden ring
rows provide the update lead. PF2 remains a fixed, single-buffered HUD raster;
dynamic counters and gauges update only their small dirty regions.

The scaffold exercises this contract continuously: every other PAL frame it
draws one hidden bottom row, advances the ring origin by one, rebuilds the
inactive copper list with the new top and wrap pointers, and publishes that
list for the following vertical blank. It never copies or scrolls the raster
contents. The row producer decodes the selected entrance fragment directly from
the recovered builder-pattern pack and emits its terrain symbols one scanline
at a time. Later gameplay state will choose and sequence fragments while
retaining the same publication path.

The first hardware milestone is a four-plane copper display which proves the
crop, palettes, priorities, null sprites, and central aperture before it tries
to render recovered assets.

## Recovered asset format

`tools/generate-amiga-hud.py` has deterministic `hud` and `game` profiles. It
converts the exact HUD resources used by the landscape path, plus the first
player, area-one tile, and startup resources, into `generated/amiga-hud.dwap`
and `generated/amiga-game.dwap`. The generated payloads remain uncommitted
copyrighted material. Each frame record retains its stable sprite and frame
IDs, origin, logical dimensions, word-aligned DMA width, payload offset, and
size.

Rows are stored as two interleaved planes. The four pens deliberately
distinguish transparency from black:

- 0: transparent
- 1: white
- 2: red
- 3: opaque black

That distinction lets the PF2 border and gauges occlude PF1 where the recovered
art contains black while leaving genuinely transparent atlas pixels open.

## Constant-pen masked blits

`Bitmap::patternWithMask` uses channel A for the fetched opacity mask and
channel B for the constant pen pattern. The blitter's first/last-word masks
affect A only; placing opacity on B leaves unaligned text edges unmasked.
Traversal follows the opacity shift, including descending left-shift copies.
The edge masks cover the complete fetch span, with a zero mask for an extra
spill word beyond the source rectangle. Immediate and queued blits use the
same controls and A-mask/B-pattern assignment.

The native clear-screen verifier exposed this defect as 27 differing HUD
bytes on its first frame (stray left-edge pixels and missing right-edge bits).
With the correction, the first 160 consecutive native frames, including the
upgrade choices, matched both planar buffers byte-for-byte against the scalar
renderer before the initial 45-second diagnostic deadline. This is rendering
backend evidence, not proof of original Windows rasterization semantics.
