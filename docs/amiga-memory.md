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
| Resident game-art pack (directory, planar art, garden sprites) | 277,320 |
| **Total** | **485,792** |

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

The eight-frame spin animation adds 6,912 resident bytes, including directory
entries and offline mirrored art. The bounded A500 allocation check passes
with this pack. Four offline frog damage frames and their mirrored/DMA variants
add 2,880 resident bytes; the bounded 512 KiB A500 allocation check also passes
with those assets. The renderer test caps resident game art at 278,000 bytes. Run
`make verify-amiga-memory` for a bounded target allocation/initialization
check. Startup diagnostics expose the failed check, initial Chip free/largest
block, requested asset bytes and remaining Chip memory after allocations.

## Ordinary memory

The executable is tested with the ordinary 4 KiB AmigaDOS task stack. An
800-byte automatic menu-Copper verification buffer previously enlarged the
main frame enough that the gameplay verification build corrupted an asset
cache immediately below the task stack, eventually producing a bus error in
HUD drawing. The identical binary passed with a diagnostic 16 KiB stack.
That scratch buffer and long-lived scene state now use static storage instead;
scene construction still happens explicitly at application entry.

The 600-tick cache verifier now checks a CPU-initialized 256-byte canary at the
bottom of the task stack. Its normal-stack run passes, including 101 terrain
composition comparisons. This is bounded route coverage, not a proof of the
maximum stack use of every scene or future feature. `VERIFY_STACK_BYTES` can
override the verifier's stack for diagnosis; its default remains 4096. The
emulator debugger did not reliably install memory canaries itself, so the
verification executable initializes them before platform/scene construction.

Serialized room/object/builder packs, prepared gameplay DMA source words,
simulation state and the three-column terrain raster cache can reside in Fast
RAM. Only DMA-read data belongs in Chip RAM. The three-column full-level cache
and collision indices still need a separate final memory-reduction pass.
Audio will require an explicit additional budget; the current headroom must
not be assumed sufficient for unmeasured additions.
