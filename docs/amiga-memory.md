# Amiga memory budget

The native target separates DMA-visible display storage from immutable game
data and simulation state. Only memory read directly by Agnus is allocated
with `MEMF_CHIP`; recovered packs remain in the executable's ordinary data
sections and can therefore reside in Fast RAM when available.

## Current Chip RAM ownership

| Owner | Bytes | Lifetime |
| --- | ---: | --- |
| PF1 384x320 circular raster, two planes | 30,720 | whole run |
| PF2 384x284 fixed HUD raster, two planes | 27,264 | whole run |
| Two 128-word copper lists | 512 | whole run |
| Two 32-line player sprite buffers | 272 | whole run |
| One 8-line gunboot bullet sprite buffer | 40 | whole run |
| Armed null sprite | 8 | whole run |
| **Total** | **58,816** | |

`AmigaMemory` owns these seven allocations as one unit. A partial allocation is
rolled back immediately, and all early-exit paths release the same ownership
object before the graphics library is closed. Gameplay data must not migrate
into Chip RAM merely because the renderer consumes it.

The current executable embeds approximately 330 KiB of recovered room,
object/event, and builder-pattern data. These packs are validated before
takeover and read in place; no endian-converted duplicate is allocated.

The CPU baseline is now a plain 68000 and the emulator defaults to an A500.
For the moment it uses 512 KiB of Chip RAM plus an 8 MiB Fast RAM expansion so
CPU work is not confused with ordinary-memory reduction. The table above shows
that the current DMA allocations fit comfortably in 512 KiB of Chip RAM; the
embedded packs and eventual runtime state determine the separate Fast/Slow RAM
requirement that still needs measuring and reducing.

Future sprite, sound, and music conversion must add explicit rows to this
budget. Animation art should stay packed in ordinary memory and be expanded
only into bounded sprite/blitter staging buffers. The PF1 ring is permanent:
vertical scrolling changes its bitplane origin and redraws revealed rows, not
the complete 284-line viewport and not a second full playfield.
