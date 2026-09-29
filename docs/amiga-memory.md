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
| Resident game-art pack (directory, planar art, garden sprites) | 273,184 |
| **Total** | **481,656** |

A temporary 16-byte startup copper list is also allocated during initialization.
Immutable planar/mask payloads may share byte-identical word-aligned subregions
and matching tails. Every record still describes one contiguous stream with its
original length, origin and pixels. Writable garden DMA records are appended
separately and must not overlap any other record. Gameplay DMA source records
remain in the ordinary-memory tail. This is offline layout only: the target
does not decompress or rebuild assets.

Gameplay reuses the retired splash/style cache for its two display rings and
garden memory for side-view staging and the HUD pack. These are not additional
allocations. Scrolling changes copper bitplane pointers; the private terrain
backing restores moving-object damage and supplies newly revealed rows.

### Results-pack space without another Chip allocation

During gameplay the 76,800-byte garden allocation is partitioned by
`GameplaySceneStorage`: bytes 0..30,719 remain the full staging ring (which
can rotate into a displayed slot); bytes 62,860..76,799 hold the 13,940-byte
HUD pack, relocated from the entry-shaft base. This leaves a contiguous
32,140-byte middle gap, enough for the separate 25,820-byte results pack.
The HUD start is rounded down to a four-byte boundary for future odd sizes;
overlapping/oversized partitions fail validation. Results packing tests enforce
the current gap budget, and renderer tests cover the arithmetic boundaries.

The results pack is now linked as ordinary read-only executable data and copied
once into that gap during gameplay entry, after intro DMA is retired. The loader
checks the planar pack's span against the reserved capacity and returns failure
code 20 if it does not fit or validate. It is not drawn yet. This does not
increase Chip allocation or destroy an unused-looking
intro cache that is actually serving as live gameplay memory. The splash/style
allocation remains the two display rings and cannot be reused for results art.

The 600-tick cache verifier also compares every loaded results byte and every
relocated HUD byte against the linked immutable sources after buffer rotations
and side-room staging probes. It rejects absent, incomplete or corrupted art.
The executable grows by the packed results payload in ordinary memory; this is
not a claim of zero additional total memory use.

Validation: host suite, bounded A500 600-tick cache oracle and active gameplay
frame comparison pass after relocation. The native capture retains its existing
checksum; no presentation baseline was changed. Normal target build restored,
with clean 68000 instruction and software-helper audits.

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

Immutable payload placement is an offline operation. The packer removes duplicate
streams and defers word-aligned substrings. It compares a greedy growing-tail
layout with disjoint paths selected by descending suffix/prefix overlap;
cycles are rejected and the smaller actually packed result wins. Containment
can also span joins between earlier immutable streams. Ties use stable ranks
from descending stream size and lexical byte order, independent of input
iteration order. This is a heuristic, not a minimum-size guarantee. The
`.packing.json` sidecar records layout version, extractor and input/output hashes.
The directory remains sorted by resource/frame ID; each complete planar
and mask stream remains contiguous. No target decompression, extra copies or
runtime asset conversion are introduced. Writable DMA records bypass sharing
and retain separate storage. `tests/planar-packing-test.py` compares directory
metadata and every payload against an unshared pack and checks writable-record
isolation. The renderer's residency gate remains 278,000 bytes; passing it is
not proof of whole-program or peak-transition memory fit.

The eight-frame spin animation adds 6,912 resident bytes, including directory
entries and offline mirrored art. The bounded A500 allocation check passes
with this pack. Four offline frog damage frames and their mirrored/DMA variants
add 2,880 resident bytes; the bounded 512 KiB A500 allocation check also passes
with those assets. The renderer test caps resident game art at 278,000 bytes. Run
`make verify-amiga-memory` for a bounded target allocation/initialization
check. Startup diagnostics expose the failed check, initial Chip free/largest
block, requested asset bytes and remaining Chip memory after allocations.

## Ordinary memory

Room, object-registration and layer-element links store internal pool slots,
not resource IDs or instance generations. `SlotIndex` uses 16-bit links when
the capacity **including the end sentinel** fits in 65535, and 32-bit links
otherwise. Layer ownership uses the layer capacity independently of the
element capacity. Layer counts use the element width so a full pool still fits.
Room membership already requires capacity below 65534 for its marked-list
sentinels, so its forward/backward links are always 16-bit. Public indices,
numeric IDs, generation counters and ordering rules are unchanged. This is
ordinary owner storage only; it neither reduces DMA allocations nor establishes
a stock-A500 memory requirement.

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
