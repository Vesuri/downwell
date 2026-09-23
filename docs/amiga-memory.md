# Amiga memory budget

The CPU target is an A500/68000. The current emulator has 512 KiB Chip RAM and
8 MiB Fast RAM; the latter remains a development configuration, not a final
memory requirement.

## Current Chip RAM ownership

| Owner | Bytes |
| --- | ---: |
| Private PF1 384×320 terrain backing, two planes | 30,720 |
| PF2 384×284 HUD, two planes | 27,264 |
| Garden foreground/background and entry shaft | 76,800 |
| Six 400-word copper lists | 4,800 |
| Two player/channel-zero sprite buffers | 272 |
| Twelve regular actor sprite buffers | 1,632 |
| Two channel-seven sprite buffers | 272 |
| Null sprite | 8 |
| Shared splash/style cache | 66,704 |
| Resident game-art pack (directory, planar art, garden sprites) | 267,528 |
| **Total** | **476,000** |

A temporary 16-byte startup copper list is also allocated during initialization.
Gameplay reuses the retired splash/style cache for its two display rings and
garden memory for side-view staging and the HUD pack. These are not additional
allocations. Scrolling changes copper bitplane pointers; the private terrain
backing restores moving-object damage and supplies newly revealed rows.

## September 23 startup failure and correction

Additional offline actor variants increased asset residency to 302,664 bytes,
making the total 511,136 bytes before the temporary copper list. On the tested
A500, only 502,584 Chip bytes were free (largest block 502,120) at that point.
Startup check 4, the AmigaMemory allocation, therefore failed and returned to
AmigaOS. This was a genuine Chip RAM shortage, not an executable-launch issue.

The asset generator now shares byte-identical immutable planar payloads while
preserving every directory entry and resource/frame ID. This saves 35,136
bytes. Writable garden sprite DMA records are explicitly excluded from sharing;
inactive position headers must never alias the displayed sprite.

The renderer test caps resident game art at 270,000 bytes. Run
`make verify-amiga-memory` for a bounded target allocation/initialization
check. Startup diagnostics expose the failed check, initial Chip free/largest
block, requested asset bytes and remaining Chip memory after allocations.

## Ordinary memory

Serialized room/object/builder packs, prepared gameplay DMA source words,
simulation state and the three-column terrain raster cache can reside in Fast
RAM. Only DMA-read data belongs in Chip RAM. The three-column full-level cache
and collision indices still need a separate final memory-reduction pass.
Audio will require an explicit additional budget; the current headroom must
not be assumed sufficient for unmeasured additions.
