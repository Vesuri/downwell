# Amiga memory budget

The CPU target is an A500/68000. The current emulator has 512 KiB Chip RAM,
512 KiB slow RAM at `$C00000`, and 8 MiB Fast RAM at `$200000`. Exec labels
both non-Chip regions `MEMF_FAST`; they do not have the same bus timing.
This remains a development configuration, not a final memory requirement.

## Native terrain identities

`nativeTerrainStorage` reserves 145,534 bytes of ordinary BSS on the 68000,
outside Chip RAM and the task stack. It retains at most 2,048 terrain
lifetimes in birth order. Adjacent generated cells share identity/geometry
descriptors; coincident and authored walls remain distinct. The bounded
descriptor array reserves worst-case singleton capacity, so compression
reduces live descriptors rather than the allocation size. Per-cell state is
one flags byte plus the run locator, not a duplicate generic runner record.
Each run also retains a shared native registration-clock base. An intervening
actor birth prevents adjacent walls from merging across that chronology gap.
The existing per-cell flags byte also packs constructor spatial membership,
precise-mask and bounds-dirty bits. Another bit distinguishes constructor
from post-Create cached geometry. Bounds are derived from the run's object,
coordinates and cached resource stage for publication and retirement; no
per-cell rectangle array is allocated.
A 16-bit sprite value per cell retains the admitted normal wall/platform
Create result; the flags byte marks Create completion. Platform image index
23 and stopped animation follow from that typed Create state, without generic
animation records. The remaining flags bit marks authored room-script
application after Create. Its initial pillar value is derived from the
creation-code ID already retained in the run; no additional per-cell storage
is required for those scripts. Live wall End Step fields occupy one packed
32-bit word per slot (8,192 bytes total), preserving the exact admitted domains
of code 986. One transient pointer and handle expose current event mutations
to synchronous callbacks; the packed state retains the failure prefix after
an event returns. No generic property record or duplicate geometry is added.
The lifetime binding also forwards collision-cache and presentation notices;
these callbacks add eight bytes on the target, not per-cell storage. A further
four-byte callback publishes activation changes to the shared collision owner.
Two store-wide flags track retained presentation and unbounded artwork.
The painter visits intersecting identity runs after wall-state mutations;
untouched scenes retain the indexed batch compositor. No draw-record array is
retained: each selected wall supplies a temporary value to the painter.
Room teardown preserves the generation counter, preventing old handles from
addressing replacement walls. Generated numeric VM IDs are not inferred from
these family-local handles.

Native actor publication retains position, sprite/mask IDs, binary32
scale/angle/image index, timeline/cached frame, cached bounds and spatial/appearance flags per
non-terrain collision slot inside the ordinary-memory collision owner.
Terrain slots are excluded from that array; their coordinates remain
run-owned. This storage does not allocate Chip RAM or another spatial index:
native publication borrows the otherwise unused reference owner's journal/tree
allocation, without attaching generic lifetime/property records there.
The native spatial/collision dirty queues each retain two 16-bit links per
shared collision slot and two counters. These are ordering links only;
terrain geometry and lifetime identities remain in the run owner. Both
queues are ordinary memory, with no per-frame allocation.
Collision collection also reserves one unsigned slot per shared collision
identity in the owner for animated-mask retention. This scratch lives in
ordinary memory rather than the Amiga task stack; it holds ordering keys,
not copied terrain or actor records.

## Measuring transient Chip residency

`amiga/chip_memory_watch.gdb` is sourced by the startup-memory, complete-scene
and Results/Retry verifiers after connecting and before running the executable.
It watches Exec's Chip `MemHeader.mh_Free`, recording every change and the
minimum free-byte value, including allocation/free pairs inside DOS file I/O.
This measures residency after the executable has loaded; it does not measure
the loader's earlier transient footprint. The watch adds no shipping code or
allocation, but debugger stops mean these runs are not frame-time measurements.

At each `AmigaPlatform::loadFile` entry and the final verification marker,
`downwell-chip-snapshot` walks the free list, validates its ordering and total,
and reports the largest block. Largest-block figures are checkpoint samples,
not a claimed continuous minimum; the free-byte low-water mark is continuous
while the watch is installed. The script rejects multiple Chip regions rather
than silently measuring only one. SDK field offsets are compile-time checked
in `framework/AmigaMemory.cpp` because unused type metadata need not survive
in the companion ELF. Use explicit `VERIFY_CHIP_KB=512` when running these gates.

Scene verification shares one 54,528-byte ordinary-memory scratch arena between
the actor, field-overlay, terrain-row and Results-HUD pixel oracles. An RAII
lease traps on overlapping use; the two simultaneous HUD images occupy separate
halves. Each oracle still initializes its own complete expected image. The arena
is allocated with `MEMF_FAST` after loading and freed on every return from the
diagnostic run; allocation failure returns 20. This avoids forcing all test
scratch into the executable's large load-time allocation and lets Exec use an
existing secondary non-Chip region. It is absent from normal builds. Do not
interpret restoring this diagnostic's loader fit as a shipping memory reduction.

## Executable size versus runtime memory

The normal `STRIP=1` build with admitted room payloads, scene-loaded gameplay art
and without reference-only collision masks is **2,141,020 bytes**, SHA-256
`e28bd170d36ac30c3515fef90eb7d0c4319d9595a286b77851be2f9fa89a70c1`.
The matching ELF section sizes distinguish payload from runtime allocation:

| Section | Bytes | Stored in the executable? |
| --- | ---: | --- |
| Machine code (`.text`) | 1,383,684 | Yes |
| Read-only data (`.rodata`) | 674,952 | Yes |
| Initialized writable data (`.data`) | 34,194 | Yes |
| Initializer pointer | 4 | Yes |
| Zero-filled storage (`.bss`) | 3,491,648 | No; allocated at load |

Initialized payload totals 2,092,834 bytes. Hunk relocation, padding and container
records account for the remaining 48,186 bytes. Stripping cannot remove
the initialized payload. ELF debug sections are not that payload either.
The sparse executable Draw chain adds 8,568 ordinary BSS bytes and 2,072
Hunk bytes against `f4b4433a`; it adds no Chip allocation. Normal 512 KiB Chip
startup still leaves 126,840 bytes free (`tmp/sparse-draw-startup-gdb.log`).
Native prepared bounds reuse the existing geometry table and add no BSS or
Chip allocation; the same startup free space is verified in
`tmp/native-geometry-startup-gdb.log`.
The activation change set adds 496 ordinary BSS bytes, no Chip allocation.
Its selected reconciliation increases the linked Hunk by 15,256 bytes against
`1335f367`; this is a frame-cost improvement, not a binary-size reduction.
Normal startup still leaves 126,840 Chip bytes free, with no gameplay art or
terrain raster resident (`tmp/activation-selection-startup-gdb.log`).
Retained End Step capture reuses the existing snapshot storage; its player
handle/revision/validity add 12 ordinary BSS bytes. The Hunk grows 2,852 bytes
against `5373be79`. Startup Chip usage remains unchanged
(`tmp/endstep-capture-startup-gdb.log`).

The pre-repair owner audit attributes roughly 551 KB to
`GameplayCollisionOwner` and its template instantiations, 260 KB to
`GameplayWorld`, and 64 KB to spatial-tree machinery. This is linked code
size, not inclusive call cost. The runtime room pack is 26,468 bytes and the gameplay
pack 234,718 bytes. Other scene/resource banks and precomputed tables make up
the rest. The 511,580-byte raw collision-mask catalogue is linked only when
`DOWNWELL_SCENE_VERIFY` supplies its actual consumer: the precise terrain-mask
oracle. Shipping code has no mask-byte reader and neither links nor validates
that catalogue. The normal linkage audit rejects its symbols. This does not
remove collision bounds metadata or change the shipping collision algorithm.
Future live precise-mask events need scene-owned mask dependencies, not a
restoration of the complete reference catalogue to every executable.

The largest BSS owner is collision storage (2,620,546 bytes in the current
native ELF). Of this, the scheduled spatial owner is 1,465,932 bytes and its
journal/index container is 1,327,222 bytes. The derived R-tree alone reserves
1,236,578 bytes (including a 1,204,752-byte node pool); the ordered mutation
journal is a separate 90,112 bytes. These are nested sizes, not additive.

The bounded 1,800-update contact run at `03e29381` leaves the tree at its
single empty root (`used=live=1`), while the journal has 1,156 pending
operations and population 824, still deferred. No level reset occurs during
this workload. This demonstrates dormant tree backing in this run, not
permission to remove the journal or the query implementation. Queries,
explicit service, journal pressure and eager numeric fallback can require
materialization; any lease must preserve synchronous failure and historical
insertion/removal order. It must also distinguish front-end, gameplay and
reference-oracle lifetimes. No tree residency saving is implemented yet.

Reproduce with the bounded contact `profile.sh fps` command in the frame-cost
audit. Its `Spatial storage` line reads fields directly through the debugger:
calling `nodeCount()` or `height()` would drain the journal and invalidate this
observation. The ELF layout can be inspected without running the game:
`m68k-amiga-elf-gdb -batch amiga/out/Downwell.elf -ex 'set max-value-size unlimited' -ex 'ptype /o ((downwell::GameplayCollisionOwner*)0)->owner_.index_'`.

Terrain raster
pixels now require a separate 1,081,344-byte ordinary-memory gameplay lease;
only row validity, shop revisions and binding state remain in the static owner.
The lease is absent during normal startup and front-end scenes, and is released
before upgrade selection, then reacquired for gameplay. The allocation is
`MEMF_FAST | MEMF_CLEAR`: untouched clean rows preserve the former zero-filled
BSS contract. No clearing or allocation occurs in ordinary gameplay frames.
The lease keeps 1,081,344 pixel bytes out of startup allocation, but peak
gameplay memory is essentially unchanged: this is scene residency, not a
smaller raster representation.
`tmp/raster-lease-startup-gdb.log` verifies no raster residency at startup and
126,840 Chip bytes free. Initialized payload plus BSS exceeds 5 MB even before
the gameplay raster and dynamic graphics allocations. A small
Hunk file therefore would not, on its own, prove an A500 memory fit.
Moving data to sidecar files changes file size, not residency, unless its
lifetime or representation also changes. The shared DMA plane atlas below
addresses Chip residency; it does not remove the original CPU-side pack.
The room projection excludes unreachable source rooms, rather than moving
their bytes into another resident allocation. Its supported-room union remains
resident; see [the room contract](room-pack.md#admitted-runtime-projection).

Reproduce section sizes with `m68k-amiga-elf-objdump -h Downwell.elf`, and
linked symbol sizes with `m68k-amiga-elf-objdump -t Downwell.elf`. Use the
matching stripped Hunk for file-size comparisons; the debugger ELF intentionally
retains symbols. Build with `make -C amiga STRIP=1` for the stripped Hunk.

## CPU-only HUD artwork

The immutable counter/streak pair-column tables occupy 49,368 bytes in
ordinary read-only memory (14,520 currency; 34,848 streak), not Chip RAM.
They replace 8,160 bytes of individual digit-layer tables in the default
renderer: the artwork increase is 41,208 bytes. No additional mutable cache,
framebuffer or staging allocation is required. The generator certifies the
two-byte decomposition rather than storing a 279,438-byte three-digit bank.
`REFERENCE_HUD_COMPOSITION=1` links the retained individual-layer tables
instead of the pair columns.

## Scalar reference storage admission

`GameplayCollisionOwner::ReferenceStorage` owns the scalar spatial and appearance
arrays. It is not a member of `GameplayCollisionOwner`. Reference and
terrain-only reference construction require that backing explicitly; native
construction rejects it. The normal Amiga build reserves no such arrays.
Reference scene/field/entry verification builds construct them before the owner,
outside the task stack. Host fixtures hold them until after the owner dies.

`RunnerBorrowedRecords` does not allocate on first use. An absent or out-of-range
read/write latches terminal failure and immediately notifies the owner; no
additional per-frame validation scan is needed. The small failed-write sentinel
is not backing storage and can never make a failed owner valid again. Record
capacity changes do not change the borrower's size. These rules remove the
two scalar record arrays only: the shared journal/index and other reference
ownership machinery still have their existing owners.

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
| Gameplay-leased DMA plane atlas (cavern; full pixel oracle 112,970) | 106,574 |
| Effect surface | 20,448 |
| Offline interleaved smoke/gem stamps | 0 additional (4,932 in cache tail) |
| Shared scene bank (boot or front art; gameplay terrain scratch and field images) | 146,818 |
| **Gameplay total, before allocation alignment** | **482,314** |
| **Front-end total, without gameplay DMA atlas** | **375,740** |

The directory sizes depend on the generated asset set; remeasure actual free
and largest Chip blocks when it changes. The original contiguous gameplay
DWAP stays in ordinary memory for CPU rendering, effects, metadata and sprite
stream selection. `Downwell.cavernplanes` has a separate Chip allocation acquired at
gameplay entry, not during ordinary startup. Retry retains the allocation;
returning to style select or the surface retires its blits and cached Bitmap
views before freeing it. Re-entry allocates, loads and validates it again;
failure returns 20 rather than publishing a partial binding. Gameplay-only
replay and plane pixel-oracle builds explicitly acquire it before their tests.
Its immutable colour and mask planes are interned independently
offline, including aligned substreams. Blitter source views bind those three
addresses; no mask generation, conversion or per-frame upload takes place.
`make test-amiga-plane-bank` compares every source view byte-for-byte and
checks that malformed directories cannot publish out-of-bank DMA pointers.
The 388-byte effect-stamp header/directory stays in ordinary memory. Its 4,932
DMA bytes occupy the retired selection cache at offset 61,440, immediately
after the two 30,720-byte gameplay rings and within the 66,704-byte allocation.
Startup fixtures initialize this payload; every gameplay entry reloads it
after draining the blitter because intervening scenes can overwrite the cache.
Neither the per-frame stamp lookup nor the pixel streams change. The resident
large-field arc images cost 71,456 of the scene bank's bytes. Headroom must be
remeasured when any pack or other Chip owner grows.

## Scene banks

Initial gameplay room construction is admitted at first gameplay entry, not
publisher, garden or direct-style startup. Its ordered room births, prepared
terrain index and generation bindings use the same initializer once; later
Retry/level reloads retain their existing reset path. The publisher/style and
full-scene verifiers check this execution boundary. This does not yet defer
the world/owner constructors or release their reserved ordinary-memory storage.

Boot/front art shares a scene bank. Both gameplay art banks are absent during
ordinary front-end scenes, including later returns. CPU-side actor/effect
preparation occurs only when a gameplay lease is acquired. The gameplay bank
uses an explicitly admitted cavern subset in area 1. It excludes later-area
skeleton/bone and wood/default debris art, including mirror/DMA derivatives.
Other areas are not admitted by the production loader. They need explicit
CPU/DMA dependencies before area progression is enabled; normal progression
currently reaches only the three cavern levels. The old area-2–5 fallback to
the gameplay union is forbidden. Other embedded gameplay tables remain outside
this lease, so this is not complete scene-local residency.
`generate-amiga-hud.py --profile game`
splits its records, unchanged, into the following banks:

- `amiga-game.dwap` (`Downwell.game`), 234,718 bytes of CPU-only reference art,
  and `amiga-game.planes` (`Downwell.planes`), 112,970 bytes of DMA reference
  art. Only explicit replay/plane pixel oracles may load these catalogues.
  The normal link audit rejects their load paths in production code.
- `amiga-cavern.dwap` (`Downwell.cavern`), 221,282 CPU bytes, paired with
  `amiga-cavern.planes` (`Downwell.cavernplanes`), 106,574 DMA bytes, replaces
  that union during cavern gameplay. `CAVERN_ROOTS` is independent of future
  union additions. The exporter checks fixed 224 KiB CPU/105 KiB DMA ceilings
  and retained payloads against the full pack. Scene retirement drains
  DMA and invalidates every borrowed CPU/DMA view before release;
  same-bank Retry/upgrade handoffs retain the lease. Full-catalogue replay
  and plane pixel oracles explicitly retain the full reference bank.
- `amiga-boot.dwap` (`Downwell.boot`), the publisher logo, controls, credits
  text and dither fade. Startup composes it into the splash cache, then
  discards it.
- `amiga-front.dwap` (`Downwell.front`), garden scenery, the aligned title,
  garden hardware sprites and style select. The menu prepares style select
  while the garden is shown, so the two share one bank. Garden scenery keeps
  only the frames the seed-2 menu creation selects; `amiga-menu-test` checks
  every draw-list frame against this bank.

Shop and death-results art is loaded directly from `Downwell.shop` and
`Downwell.results` into their mutually exclusive Chip region, only on demand.
Neither is prepared during style selection or ordinary gameplay entry.
`Downwell.clear` has an ordinary-memory source allocation only during upgrade
selection, released on departure. These three payloads are not linked into
shipping executables. The generated transient-bank contracts have explicit
growth budgets; their source hashes are in `amiga-transient-banks.json`.
Retained pixel buffers have independent lifetimes: releasing source art must
not erase a still-visible scene, and repurposing DMA storage requires retirement.

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

Front-end work is admitted by scene entry, not by executable startup. The
publisher/credits/controls route loads and composes only the boot bank. The
front bank and immutable garden planes are prepared on garden entry, before
their first presentation. Direct style entry loads front art without painting
the garden; direct gameplay entry loads neither bank and composes neither
splash nor garden pixels. The blank startup copper disables bitplane DMA when
there is no splash cache. Shared backing allocations still exist because
gameplay reuses them; skipping preparation does not claim to free that memory.
Native startup, full-scene and contact-tape verifiers check these load and
preparation counts, including no repeated front-end work during gameplay.
`make verify-amiga-style-admission` checks the direct-style route through 64
updates and restores the normal build. `make verify-amiga-scenes` checks the
complete route and splash pixel hashes; the contact FPS tape checks direct
gameplay admission. Use `VERIFY_CHIP_KB=512` for the native memory gate.

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
sparse frame index. It is prepared once per gameplay art lease, before
gameplay. Actor draw records carry pointers into that bank; lookup is disabled
before the bank is released and re-entry rebuilds the bindings. It adds no Chip
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
