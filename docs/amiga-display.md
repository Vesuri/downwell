# Amiga display contract

The native target follows the recovered Windows 4:3 presentation rather than
stretching the 160-pixel game surface.

## Geometry

- Visible canvas: 380x284 PAL lores pixels.
- DMA backing width: 384 pixels (24 words, 48 bytes per plane per line).
- The display window crops two fetched pixels from each horizontal edge.
- Native visible coordinates therefore map to backing coordinates with a
  two-pixel bias.
- The game well is 160x284 at visible x=110..269, matching the host reference.

The display setup must derive and assert its DIW and DDF values in one place.
The intended OCS/ECS 16-bit fetch is 24 words per line; it must not inherit an
AGA fetch mode from the operating system.

## Dual playfield ownership

Four lores bitplanes are split 2+2:

| Layer | Hardware planes | Palette | Ownership |
| --- | --- | --- | --- |
| PF1 | BPL1, BPL3 | COLOR00..03 | Game well |
| PF2 | BPL2, BPL4 | COLOR08..11 | HUD and side panels |

PF2 has priority over PF1. Pen zero remains transparent on both layers, with
COLOR00 black, so the two layers can use white and red independently without
painting opaque black rectangles over one another.

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
- bullets, gems, and small effects: remaining single channels where their
  width, palette, and multiplexing lifetime fit;
- enemies: candidates only after the playfield implementation is correct.

Every unused channel points at an armed-safe null sprite on every frame. Sprite
priority is programmed explicitly together with PF2-over-PF1 priority; it must
not depend on an operating-system value left in BPLCON2.

## Buffering

PF1 is double buffered because the scrolling game changes continuously. PF2 is
single buffered initially because most of the HUD is static; dynamic counters
and gauges update only their small dirty regions. If tearing measurements show
that this is insufficient, PF2 can be double buffered without changing the
core rendering boundary.

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
