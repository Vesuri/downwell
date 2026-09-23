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
| Menu foreground/background caches and entry shaft | 74,496 | whole run |
| Two 160-word copper lists | 640 | whole run |
| Two 32-line player sprite buffers | 272 | whole run |
| Twelve 32-line bullet sprite buffers | 1,632 | whole run |
| Two 32-line enemy sprite buffers | 272 | whole run |
| Fourteen 32-line menu-title sprite buffers | 1,904 | whole run |
| Armed null sprite | 8 | whole run |
| Shared splash/style screen cache | 62,480 | whole run |
| Blitter-readable recovered game-art pack | 181,316 | whole run |
| **Total** | **381,004** | |

`AmigaMemory` owns these allocations as one unit. A partial allocation is
rolled back immediately, and all early-exit paths release the same ownership
object before the graphics library is closed. Gameplay data must not migrate
into Chip RAM merely because the renderer consumes it.

The splash and style-select screens are mutually exclusive. They reuse one
bounded cache: during startup it holds five precomposed 176x284 splash frames;
after the splash it holds the two style-select frame buffers and its repeating
border. Splash transitions therefore change copper bitplane pointers without
drawing into the visible display.

The current executable embeds approximately 330 KiB of recovered room,
object/event, and builder-pattern data. These packs are validated before
takeover and read in place; no endian-converted duplicate is allocated.

The CPU baseline is now a plain 68000 and the emulator defaults to an A500.
For the moment it uses 512 KiB of Chip RAM plus an 8 MiB Fast RAM expansion so
CPU work is not confused with ordinary-memory reduction. The table above shows
that the current DMA allocations fit comfortably in 512 KiB of Chip RAM; the
embedded packs and eventual runtime state determine the separate Fast/Slow RAM
requirement that still needs measuring and reducing.

Future sound and music conversion must add explicit rows to this budget. The
recovered planar art currently has to be copied to Chip RAM because it is used
directly as a blitter source; reducing that 181,316-byte residency remains an
important A500 memory task. Animation art should otherwise stay packed and be
expanded only into bounded sprite/blitter staging buffers. The PF1 ring is
permanent: vertical scrolling changes its bitplane origin and redraws revealed
rows, not the complete 284-line viewport and not a second full playfield.
