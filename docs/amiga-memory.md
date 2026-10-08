# Amiga memory budget

The CPU target is an A500/68000. The current emulator has 512 KiB Chip RAM and
8 MiB Fast RAM; the latter remains a development configuration, not a final
memory requirement.

## CPU-only HUD artwork

The immutable counter/streak pair-column tables occupy 49,368 bytes in
ordinary read-only memory (14,520 currency; 34,848 streak), not Chip RAM.
They replace 8,160 bytes of individual digit-layer tables in the default
renderer: the artwork increase is 41,208 bytes. No additional mutable cache,
framebuffer or staging allocation is required. The generator certifies the
two-byte decomposition rather than storing a 279,438-byte three-digit bank.
`REFERENCE_HUD_COMPOSITION=1` links the retained individual-layer tables
instead of the pair columns.

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
| Resident gameplay art pack (directory, planar art) | 115,326 |
| Effect surface | 20,448 |
| Offline interleaved smoke/gem stamps | 5,320 |
| Shared scene bank (boot or front art; gameplay terrain scratch and field images) | 146,818 |
| **Total** | **496,386** |

`make verify-amiga-memory` reports 6,192 bytes of Chip RAM remaining after
these allocations.
The interleaved stamps reuse no live scene storage. Before that bank, the
same allocation set left 11,512 bytes free. The resident large-field arc
images cost 56,376 of the scene
bank's bytes; before they shipped, 67,888 bytes were free.

## Scene banks

Chip RAM holds only the current scene's art. `generate-amiga-hud.py --profile
game` encodes the former single game pack and splits its records, unchanged,
into three banks:

- `amiga-game.dwap`, gameplay art, copied into Chip once at startup and
  resident throughout.
- `amiga-boot.dwap` (`Downwell.boot`), the publisher logo, controls, credits
  text and dither fade. Startup composes it into the splash cache, then
  discards it.
- `amiga-front.dwap` (`Downwell.front`), garden scenery, the aligned title,
  garden hardware sprites and style select. The menu prepares style select
  while the garden is shown, so the two share one bank. Garden scenery keeps
  only the frames the seed-2 menu creation selects; `amiga-menu-test` checks
  every draw-list frame against this bank.

The front bank puts its complete directory and all live garden/style payloads
in a 64,936-byte prefix. Within it, all style-readable resources and the
complete directory occupy the first 37,030 bytes. Only one-shot garden
scenery follows the live prefix;
`AmigaGardenScene::prepare` copies it into the scrolling canvases before any
staging starts. Writable sprite DMA headers never alias another payload.
The generator emits `DOWNWELL_FRONT_LIVE_BYTES`, and the target statically
checks that it ends before byte 75,360. After the first garden frame retires
the previous scene, each garden update stages at most 256 words of immutable
large-field arcs at that offset. No live front artwork or canvas is modified.
This borrows existing Chip storage, rather than allocating another bank.
After garden DMA retires, clean-content style updates stage at most 256 words
of small-field images into `[44304,73568)`. The generated
`DOWNWELL_FRONT_STYLE_BYTES` bound keeps every style source below that range;
payloads cannot alias across these lifetime boundaries. The complete front
file is 96,814 bytes. Its retired garden payload is not read again before a
front-bank reload.

Style carousel DMA uses fourteen offline records (eight player frames, four
independently positioned locks and two arrows). Each holds two separate
136-byte streams, one per presentation buffer: 3,808 payload bytes plus 336
directory bytes. Their writable position headers cannot be interned. All
streams stay below the retained style-prefix bound, including while field
and results artwork is staged elsewhere in the shared bank.

The two scene banks are files beside the executable. Each load is a single
`Open`/`Read`/`Close` of the whole file into the shared Chip region, through
`AmigaPlatform::loadFile`. During the load the OS gets back its VERTB
servers, blitter interrupt and multitasking; the display stays owned and the
copper is re-pointed afterwards. Leaving gameplay for the menu or style select
reloads the front bank before either scene is prepared. Splash scenes run only
at boot. During gameplay no front/boot bank occupies the region.

During gameplay the shared scene bank holds no boot or front art. The gameplay
renderer partitions it:

| Range | Owner |
| --- | --- |
| `[0,44304)` | Terrain batch scratch: two colour planes and a coverage plane, 416×284 |
| `[44304,73568)` | Resident small time-field images (29,264 bytes) |
| `[73568,75360)` | Two 896-byte masked-underlay buffers for the saved-background reference |
| `[75360,146816)` | Resident 176-pixel large-field arc images (71,456 bytes): white coverage and total coverage; the blitter derives red as `mask & ~white` |
| `[146816,146818)` | Readable zero word for the shifted blitter's extra fetch |

Retire scene DMA before binding. Small images, arc images and their immutable
blit plans remain resident through level-clear/upgrade and the next level;
those scenes do not overwrite their ranges. Each entry still resets both
overlays and rebinds their backing/terrain pointers. Invalidate residency
before any boot/front bank load, including a failed load. Entry completes any
unfinished staged image upload (including skipped/short garden or style
paths), then binds the immutable plans; partially uploaded images are never
exposed to DMA. Upload completion and bound-bank residency are separate:
preloading small images must not skip first-entry large-field plan setup.
`REFERENCE_FIELD_BANK=1` retains synchronous entry uploads without garden or
style staging. Terrain scratch writes are confined to the prefix and
do not invalidate the field banks. No extra Chip allocation is required.
The results, clear-scene and field-reentry fixtures cover that path. Small
fields restore their masks from the pristine terrain ring
(`AmigaMemory::game()`); a stationary resident-band circle composes with
pristine terrain as blitter channel C instead of restoring and repainting.
Other placements use the large-field column bank: 73,024 bytes of immutable
ordinary memory (512 radius indices, 21 frame maps, 214 column offsets and
8,852 row records). Direct terrain composition uses a 4,992-byte phase-six
word bank in ordinary memory, and `TERRAIN_BATCH_VERIFY=1` adds
ordinary-memory reference rings only in verification builds.

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
can rotate into a displayed slot); bytes 64,540..76,797 hold the 12,258-byte
HUD pack, relocated from the entry-shaft base. This leaves a contiguous
33,820-byte middle gap, enough for the separate 24,010-byte results pack.
The HUD start is rounded down to a four-byte boundary for future odd sizes;
overlapping/oversized partitions fail validation. Results packing tests enforce
the current gap budget, and renderer tests cover the arithmetic boundaries.

After style selection has retired garden DMA, content-clean style updates
stage at most 256 HUD bytes into that high-end partition. A completed upload
remains resident through gameplay and upgrade screens, which reserve the same
high-end region. Garden reuse and fresh style entry invalidate its certificate.
Short/skipped selection without a resident bank uses the complete
synchronous copy. No partial pack becomes active, and no extra Chip allocation
is needed. `REFERENCE_ENTRY_ASSETS=1` retains the synchronous path;
`profile.sh entry` compares the resident bytes with the offline pack.

The results pack is linked as ordinary read-only executable data. Content-clean
style updates stage at most 256 words into the middle gap after garden DMA has
retired. Gameplay entry consumes the completed certificate, or copies the whole
pack synchronously if selection was skipped or staging was incomplete. Fresh
style entry and consumption invalidate this certificate: unlike the HUD,
results artwork shares storage with shop artwork and transition masks.
No incomplete upload is exposed to the renderer. The loader
checks the planar pack's span against the reserved capacity and returns failure
code 20 if it does not fit or validate. This does not
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
isolation. The renderer's residency gate is 120,000 bytes; passing it is not
proof of whole-program or peak-transition memory fit.

The eight-frame spin animation adds 6,912 resident bytes, including directory
entries and offline mirrored art. The bounded A500 allocation check passes
with this pack. Four offline frog damage frames and their mirrored/DMA variants
add 2,880 resident bytes; the bounded 512 KiB A500 allocation check also passes
with those assets. The renderer test caps resident game art at 120,000 bytes. Run
`make verify-amiga-memory` for a bounded target allocation/initialization
check. Startup diagnostics expose the failed check, initial Chip free/largest
block, requested asset bytes and remaining Chip memory after allocations.

## Ordinary memory

`AmigaActorFrames` owns 45,138 bytes of static ordinary-memory render bindings
on the 68000: canvas geometry, immutable sprite-part pointers and a bounded
sparse frame index. It is prepared once from the linked game bank, before
gameplay. Actor draw records carry pointers into that bank. It adds no Chip
allocation; the existing sixteen channel buffers still own DMA control words.
This is not a second copy of sprite pixels.

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
The retained room-terrain admission plan uses 12,312 bytes of ordinary BSS
(256 decoded resource records plus identity/count/extent metadata), no Chip
RAM. Its borrowed room pack must remain immutable; it owns no live instances.
The initial-room terrain artwork is an 11,548-byte immutable Fast-RAM bank:
720 rows in each of three columns, referencing 164 unique 44-byte rows.
Pristine raster rows borrow these immutable dictionary words instead of
expanding the whole bank at entry. A bit in each existing dirty byte identifies
borrowed storage; repainting replaces complete rows in the existing mutable
raster. Invalidation alone does not change pixel ownership. Transfers gather
borrowed rows into the existing Chip backing/display rings, retaining pipeline
and wrap semantics. No Chip allocation or second bitmap is added. The decoder
borrows the bank for the executable's lifetime and rejects malformed sizes,
row references and unaligned native storage before publishing it.
`REFERENCE_INITIAL_RASTER_STORAGE=1` retains full initial materialization.
Audio will require an explicit additional budget; the current headroom must
not be assumed sufficient for unmeasured additions.
