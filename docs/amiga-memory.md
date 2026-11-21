# Amiga memory budget

The target is an unexpanded A1200: 2 MiB Chip and no Fast RAM, so the
executable's hunks, its BSS and every allocation share one pool. Only DMA
data asks for `MEMF_CHIP`; everything else asks for `MEMF_PUBLIC` and uses
Fast when present. The verification machine is still an A500 with 1 MiB
Chip and 8 MiB Fast at `$200000` (`amiga/machine.sh`: `MACHINE=a500`, the
default until the game fits, and `MACHINE=a1200`), which proves the
placement. Exec labels any slow RAM at `$C00000` `MEMF_FAST` as well,
although its bus timing differs.

## Target budget

Measured 2026-11-21 on the shipping build (`-Os` with hot paths at `-O2`,
terrain raster ring, no source terrain records, no sparse field reference).
An unexpanded A1200 booted to a shell has 1,963,176 bytes free (largest
block 1,954,856; `make -C amiga free-memory-probe`).

| Part | Bytes | Largest items |
| --- | ---: | --- |
| Loaded code and data | ~1,062,000 | `.text` 667,336, `.rodata` 321,068 (factored ammo tables about 100 KB), `.data` 73,970 |
| BSS | 727,816 | `portRuntimeStorage` (`GameplayWorld`) 221,388, `nativeTerrainStorage` 145,888, `terrainIndex` 76,814, `actorFrames` 46,932, `ammoHud` 42,404, `clearStorage` 32,198, `blitterQueueBuffer` 24,002 |
| Public allocations (peak) | 676,672 | cavern CPU bank + entry mask 399,902, terrain raster 135,168, field columns 70,816, builder patterns 70,779 |
| Chip allocations (gameplay) | ~960,000 | run sound bank 410,262, cavern plane atlas 175,094, effect surface + shared scene bank (front art, then gameplay field bands) 167,266, menu planes 76,800, scene cache 66,704, game ring 30,720, HUD 27,264 |
| Total | ~3,427,000 | about 1.46 MB over the target |

The art and sound banks (run sounds, cavern CPU art and entry mask, cavern
planes, menu planes) are about 1.07 MB; the rest, about 2.36 MB, is already
more than the machine has. The menu planes are reused during gameplay (HUD,
results, clear storage) and the run sound bank holds only sounds world 1 can
play (14 of its 145 are not heard on the slice routes: upgrades, pause menu,
explosions). Fitting needs both: smaller code, BSS and buffers, and smaller banks.

`amiga/probe_allocations.gdb` lists every exec `AllocMem`/`FreeMem` with its
caller; `DOWNWELL_MEMORY_SAMPLE=1` on `tools/verify-world-one-native.py`
reports the route's low-water marks; `tools/amiga-size-report.py` attributes
sections to symbols and classes. The startup requirement
(`downwellChipRequirement`, `downwellPublicRequirement`) comes from the
route's peak use.

## Native terrain identities

Shipping and profiling builds declare `DOWNWELL_EXTERNAL_COLLISION_BACKEND`
from their scene ownership contract. Generic collision consumers are then
compile-time unavailable, rather than runtime-selected branches retained
through lifecycle callbacks. Constructing a scalar/reference owner, disabling
native boxes, or supplying generic collision backing fails this contract;
unsupported collision collection still fails and latches owner failure.
Explicit full-reference scene/field/entry fixtures retain the complete owner.
The native results/reentry fixture uses the shipping restriction. The host
tests compile both contracts and check the build-selection matrix. This is
not a gameplay optimization toggle or permission to omit source events.
It does not yet remove every spatial routine from the link: some are still
reachable through other shared-owner paths. Runtime external-backend admission
and the scene's no-query guards remain mandatory before births.

The scalar terrain oracle borrows an explicit `GameplayTerrainInstances::Storage`
bank for its records, birth-order slots and row ranges. `ReferenceStorage`
provides that bank for reference and native-terrain comparison owners. Native
and native-scenery owners retain only the empty scalar service, not its arrays;
their terrain is owned by `nativeTerrainStorage` below. There is no allocation
fallback. An undeclared scalar birth fails, while empty lookup/clear remain
safe. Bind fresh backing once and keep it alive throughout the borrower's
use; clear preserves lifetime generations for Retry rather than rebinding.
Startup/contact guards require absent backing, full-service reference
fixtures require it present, and a host size assertion prevents embedding
the arrays in the scalar service again. This is ordinary-memory residency,
not Chip-memory savings or a new terrain registry encoding.
The 68000 bank is 152,064 bytes; replacing the embedded arrays with three
four-byte pointers removes 152,052 bytes from the native owner. The small
scalar service itself is 150 bytes. Full-service reference totals grow only
by those pointers, not by a second copy of the bank.

Inactive retained-layer membership has a one-bit-per-slot projection plus a
count (260 bytes for 2,048 slots on 68000). It owns no layer, order or lifetime
records: cutoff queries read those from the authoritative run owner. Successful
publication, layer-state changes, retirement and clear maintain the projection;
pending/unpublished births are excluded. With no inactive entries, Draw cutoff
queries return without walking terrain. The retained-run traversal remains the
test oracle for earliest-key and interval queries.

`nativeTerrainStorage` reserves 145,888 bytes of ordinary BSS on the 68000
(412,184 with the source records that only collision-owner builds bind),
outside Chip RAM and the task stack. It retains at most 2,048 terrain
lifetimes in birth order. Adjacent generated cells share identity/geometry
descriptors; coincident and authored walls remain distinct. The bounded
descriptor array reserves worst-case singleton capacity, so compression
reduces live descriptors rather than the allocation size. Per-cell state is
one flags byte plus the run locator, not a duplicate generic runner record.
Prepared builder segments may retain one descriptor for their complete mixed
wall/platform identity stream, borrowing immutable bytes from the builder pack.
The caller must explicitly promise that the pack outlives these lifetimes;
temporary streams keep the copying constructor. Normal Amiga generation uses
the executable-resident builder pack. A future scene-local pack must clear all
borrowing terrain before replacement or unloading. Publication mutations split
the descriptor while advancing its borrowed stream, preserving per-cell order,
holes, generations and source coordinates. Authored IDs and borrowed stream
pointers share descriptor storage; the 68000 allocation does not grow.
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

Full-service native actor publication retains position, sprite/mask IDs, binary32
scale/angle/image index, timeline/cached frame, cached bounds and spatial/appearance flags per
non-terrain collision slot in ordinary memory. Box slots belong to the optional
collision bank, not the always-resident owner. Terrain slots are excluded from
both arrays; their coordinates remain run-owned. This storage does not allocate
Chip RAM or another spatial index:
native publication borrows the otherwise unused reference owner's journal/tree
allocation, without attaching generic lifetime/property records there.
Collision-enabled native scenes explicitly supply `NativeCollisionStorage`:
two dirty queues (each two 16-bit links per shared slot plus counters), one
unsigned slot per shared identity for animated-mask retention, and prepared
sprite/terrain collision geometry, and 512 box publication records. This is borrowed
ordinary-memory backing, not part of every owner, not Chip memory and not an
allocation fallback. The caller must keep it alive until after its owner.
Missing backing fails on collision-service use. External-backend admission
rejects a supplied bank rather than retaining an unnecessary scene dependency.
Terrain geometry and lifetime identities remain in their run owner.
On the 68000 this bank is 126,862 bytes, including 30,720 bytes of box records
and 48,570 bytes of prepared geometry (26,676 bytes of native-endian sprite
descriptors plus 21,894 bytes of prepared bounds). Full-service reference owners carry the same geometry in their
explicit `ReferenceStorage`. Guarded external-backend owners neither reserve
nor prepare it; they retain only a four-byte geometry pointer. Required sprite
metadata, object defaults and terrain Create plans remain available. Startup
and contact-profile verifiers assert null bank and geometry pointers, and the
preparation-count gate distinguishes terrain Create plans from optional
collision birth bounds. Query-enabled fixtures verify the full-service path.
The native-endian descriptor cache lives in this same explicit bank, not in
the always-resident owner. External scenes read required frame counts directly
from the immutable serialized pack. Adding unrelated descriptors cannot grow
their native cache or cause a full-catalogue decode; the growth fixture adds
128 descriptors and exercises existing births without admitting geometry.
The startup and contact verifiers also require a null prepared-descriptor
pointer. This does not yet partition the serialized sprite metadata itself.
Moving box records into the explicit bank
removes 30,720 bytes from an external-backend scene; full-service native totals
are unchanged because that scene admits the bank. This does not reduce Chip
usage or remove the remaining actor-position array.

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

The shipping HUD source is `Downwell.hud`, with an explicit 16 KiB maximum
in the transient-bank manifest. Gameplay entry reads it directly into the
reserved Chip HUD-art region after the previous display has retired; there is
no always-resident CPU source or temporary ordinary-memory upload allocation.
Startup/style do not load it. Level handoffs retain that region, while garden
and fresh style ownership invalidate its binding before reuse. Read or format
failure returns 20 before admission. Shipping linkage rejects `amiga_hud_pack`;
verification builds retain independent linked bytes and compare the actual
file-loaded region across re-entry. `REFERENCE_ENTRY_ASSETS=1` retains the
old linked-copy oracle. Prepared HUD renderers still validate their source
records on first binding; file loading alone does not certify their artwork.

A historical `STRIP=1` size audit with admitted room payloads, scene-loaded gameplay art
and large-field pixels, and without reference-only collision masks is
**2,008,404 bytes**, SHA-256
`9a6ec6bd2e583d27e1a1305b169be2e8a21c9c9e208bb92e8bbaa291a3c21d31`.
The matching ELF section sizes distinguish payload from runtime allocation:

| Section | Bytes | Stored in the executable? |
| --- | ---: | --- |
| Machine code (`.text`) | 1,393,032 | Yes |
| Read-only data (`.rodata`) | 532,732 | Yes |
| Initialized writable data (`.data`) | 34,194 | Yes |
| Initializer pointer | 4 | Yes |
| Zero-filled storage (`.bss`) | 2,256,840 | No; allocated at load |

Initialized payload totals 1,959,962 bytes. Hunk relocation, padding and container
records account for the remaining 48,442 bytes. Stripping cannot remove
the initialized payload. ELF debug sections are not that payload either.

Linked serialized banks account for 312,657 of the 532,732 read-only bytes:

| Linked bank group | Bytes |
| --- | ---: |
| HUD source | 13,986 |
| Effect stamps | 5,320 |
| Entry, exit and selection masks | 76,464 |
| Admitted room pack | 26,468 |
| Object metadata | 47,136 |
| Sprite collision metadata | 26,700 |
| Builder patterns | 70,779 |
| Prepared terrain and initial terrain | 16,540 |
| Small time-field images | 29,264 |

The remaining read-only bytes include generated tables, constants and strings.
The 71,456-byte large-field pixel stream is read from `Downwell.fields`
directly into the gameplay Chip bank; there is no linked
CPU upload copy. A linkage guard rejects its return. This is distinct
from the 70,816-byte CPU column-rendering representation in `Downwell.columns`;
those are not interchangeable arrays. The latter has an ordinary-memory
gameplay lease (`MEMF_FAST`, no Chip fallback), loaded and checksum-validated
before renderer admission. Front-end scenes retain neither payload. Gameplay
retirement invalidates the column binding before freeing the lease. Re-entry
reads it again; an absent binding rejects drawing, never reads a file mid-frame.
The native link audit rejects `fieldColumnWords` as well as `fieldBandPixels`.
The host keeps both original arrays as serialization oracles; file-loaded CPU
commands are also checked against independent raster pixels.

Reproduce the section and bank measurements with `m68k-amiga-elf-objdump -h`
and `m68k-amiga-elf-objdump -t -C` on `amiga/out/Downwell.elf`, after sourcing
`amiga/env.sh`. Serialized banks use end-minus-start symbol addresses; their
symbols have zero size. Function-size sums must deduplicate aliased addresses
and must not include debug-section symbols.
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

Collision storage uses scene-dependent backing. The derived R-tree
is separately leased on first use: its 1,236,578-byte allocation includes a
1,204,752-byte node pool. Replacing embedded backing with the 18-byte lease
removes 1,236,560 resident bytes when no tree consumer runs. The ordered
mutation journal is a separate optional 90,112-byte allocation, not another
copy of the tree. Its 16-byte lease replaces the embedded bank. Native
allocation requests `MEMF_FAST`, with no Chip fallback.
At the journal split, the native owner occupied 1,247,790 bytes, down from 1,337,886:
90,096 fewer always-resident ordinary bytes. This is a BSS/residency reduction,
not a claim that 90 KiB disappeared from the executable's on-disk payload.

Normal Amiga gameplay declares that it has no spatial-index query consumers
before constructing the player or room. Its implemented collision adapters use
the terrain grid and family owners. The index-only declaration suppresses the unused
mutation journal and historical spatial envelope; it retains unique identity
membership and every synchronous flag, dirty-queue and layer publication.
Box/line queries and tree-size observations fail terminally under this contract:
there is no reconstruction from current positions or silent fallback. Clear
preserves the declaration. A new query consumer must choose the full index
before scene construction, not switch it on after losing birth history.
No journal bank is acquired under this declaration. Its provider is bound
before births but is invoked only before the first queued operation in a
query-enabled scene. Failed acquisition invalidates the journal before that
operation publishes membership or invokes a prepared-span producer.

This declaration excludes index traversal, not all collision-state readers.
`GameplayCollisionOwner::collisionPointCandidate` and `collisionOverlap` read
`nativeCollisionVisual` directly and can refresh cached bounds without first
querying the tree. Direct collision-event adapters also read published actor
coordinates through that view. Do not infer permission to remove cached bounds,
appearance flags or dirty ordering from an absent R-tree query alone; those
would require a separately proven consumer contract.

Normal native gameplay additionally calls `declareExternalCollisionBackend()`
before births. This fails closed on direct cached-geometry reads, collision-pair
collection, dirty-pass execution and native object-query preparation, including
paths which bypass the index. Unexpected consumers invalidate the owner; they
must not silently return an empty collision result. Full collision services
remain the default for reference owners and explicit query fixtures. The guard
suppresses generic actor/terrain collision geometry, index membership and
spatial/collision queue maintenance.
External-backend admission requires native box ownership. Breakable births
retain authoritative integer coordinates, artwork and family preparation,
without constructing unused binary32 position/appearance mirrors or generic
cached geometry. The original integer-offset overflow admission is retained.
Direct box mirror reads fail terminally, as do scenes attempting this admission
without native boxes. Full-service births retain the original publication path
and its caller-supplied records. Actor registration, activation, layers, live
sprite setters and native terrain lifetime publication remain synchronous.
The external backend admits generic terrain registration only for object 93:
its pinned End Step uses the existing indexed registration-order walk.
Walls have no dispatched wall event or generic terrain selector in this scene,
so their generic registration is not maintained at birth, retirement or
reactivation. Their native lifetime, chronology, room/layer membership and
activation flags remain authoritative. This is consumer admission, not another
terrain registration encoding or a replacement list. Full-service scenes keep
the complete registry. Generic whole-scene Step/alarm dispatch is unavailable
under external admission; terrain population queries fail without replacing
the output with an empty count. Diagnostic registration walks describe only
the admitted registry, while ownership replay also compares native terrain
identities, chronology and destruction marks before cleanup.

An admitted prepared span publishes its birth chronology and stationary
retirement certificate as one callback-free transaction. Every slot is first
checked fresh, lifetime/order overflow and layer capacity are admitted, and
the wall/platform Create plans must have no extra observer. The resulting
birth clock and saturated binding/capture revisions equal the per-cell path;
the local Draw revision is untouched. Same-row platform bounds are included
once and the certificate retains the final lifetime ceiling. Object 93 still
registers in original order. Registration cannot allocate or exhaust a separate
pool for these fresh slots. Unsupported spans retain scalar publication and
its failure prefix; no event Create is delayed past another segment. The
full-service path retains the scalar notices as the oracle. This adds no
per-cell storage, new registry or scene allocation.

`make verify-amiga-results-restart` uses native-scenery ownership and external
collision admission, not the full-service owner selected by the other scene
oracles. It injects results readiness and then exercises production confirmation,
Retry, style-selection and surface-return paths, ending after 200 gameplay
updates. The debugger requires absent scalar control/terrain, geometry,
collision backing, spatial tree and journal after re-entry. Existing counters
also verify HUD contents and CPU/DMA art retention, release and reload across
the four gameplay entries. Source-mask querying remains in the separate scene
and field fixtures; its unused probe counter is not a native admission check.
The results target bounds the emulator run and restores a normal stripped
executable through an exit trap, including on verification failure. This
instrumented re-entry fixture is not a frame-time benchmark.

Player, enemy and bullet position setters retain lifetime/ownership validation,
but do not maintain the excluded backend's coordinate mirror. Player/enemy
records own their integer positions; bullet `sourceMotion` owns the published
binary32 coordinates, including Collision-phase assignments read by End Step.
Birth still validates coordinate admission. Direct published-position reads for
these families fail closed under the external-backend declaration; a new
consumer must explicitly admit the full backend before births. Full-service
owners retain the mirrored path as the comparison implementation.
Prepared terrain spans publish the same admitted registration/layer failure prefix
without geometry keys, bounds envelopes or index publication. Scalar terrain
births use the same backend boundary; the full collision-enabled path retains
the original geometry/publication behavior. The contact verifier requires an
empty generic index even while native terrain and actors are alive.
Diagnostic dirty-queue traversal is unavailable under this declaration rather
than pretending that an unmaintained queue represents source state. Collision
metadata bits are private to the excluded backend; eligibility/order bits remain
live. Queue/mask-retention backing is absent for this scene, including startup
and Retry; the full-service path requires caller-supplied storage. The ordinary
actor-position array remains a separate retained structure. Journal backing is
absent under this declaration, checked at startup and after the contact tape.
A passing bounded replay
establishes coverage only for that workload, not every future scene or consumer.

Query-enabled owners, including the native original-mask verification path,
retain the complete ordered journal in the leased bank. Reset releases both
journal and tree backing; the providers survive to support re-entry. The
platform provider scopes also release memory on early exits from the
placement-created owner. The host's default inline bank remains the oracle;
lease tests compare ordered results, pressure, prepared spans and failure
prefixes against it. Validity checks, zero-budget service and
empty resets do not allocate. Queries, nonempty explicit service, journal pressure and eager numeric fallback
can materialize the tree synchronously, preserving historical insertion/removal
order. Allocation failure is terminal until clear; clear releases backing and
permits a fresh acquisition. The host retains the embedded tree as the
reference implementation.

The bounded 1,800-update contact workload has no allocated tree, with zero
pending journal operations and zero generic index population. The native original-mask fixture
does allocate on query. This is a residency saving, not a smaller materialized
tree or a performance claim; query-heavy scenes still require the full backing.

Reproduce with the bounded contact `profile.sh fps` command in the frame-cost
audit. Its `Spatial storage` line reads fields directly through the debugger:
calling `nodeCount()` or `height()` would violate a no-query declaration, or
drain a query-enabled journal and invalidate this observation. The ELF layout
can be inspected without running the game:
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
126,840 Chip bytes free. At the audited build above, initialized payload plus
BSS is 4,216,802 bytes before gameplay raster and dynamic graphics allocations.
Adding the raster and CPU field-column leases brings this to 5,368,962
ordinary-memory bytes. A small
Hunk file therefore would not, on its own, prove an A500 memory fit.
Moving data to sidecar files changes file size, not residency, unless its
lifetime or representation also changes. The shared DMA plane atlas below
addresses Chip residency; it does not remove the original CPU-side pack.
The room projection excludes unreachable source rooms, rather than moving
their bytes into another resident allocation. Its supported-room union remains
resident; see [the room contract](room-pack.md#admitted-runtime-projection).

### Builder pattern lifetime

The 70,779-byte cavern builder pack is `Downwell.patterns`, not linked into
the executable. The reviewed ceiling is 72 KiB, declared independently of
export size in `generate-amiga-transient-banks.py`. Its generated manifest
records input hashes and the bank's owner as `terrain-lifetimes`.

Normal publisher/garden/style startup has neither the allocation nor a parsed
view. First gameplay entry allocates ordinary Fast memory (no Chip fallback),
reads and validates the pack before room generation. Missing/invalid input
fails entry with return code 20. The pack object stays at a stable address so
generation and rendering borrowers retain their existing references.

Native terrain runs borrow immutable prepared-cell bytes from this pack.
Consequently this lease is **not** released with the graphics catalogue at
upgrade entry: it must survive as long as any such terrain identity survives.
Retry reuses the lease. Returning to style selection or the garden releases
it once native terrain is empty, invalidating the parsed view before freeing
its bytes. Re-entry validates a new lease. Whole-program teardown frees the
allocation after the last use. Peak gameplay payload is unchanged; startup
residency and executable size are reduced, not the live terrain footprint.

Guards: normal `verify-amiga-memory` rejects any startup pattern load;
`verify-amiga-results-restart` checks three loads/two releases across Retry,
style and surface destinations; the contact tape checks one load and zero
releases. `make verify-amiga-pattern-failure` hides only the verifier's mounted
copy, checks the actual 68000 return value is 20, and restores the normal
stripped build. The link audit rejects reintroduction of `amiga_builder_pack`.

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
arrays and `RunnerSpatialInstances::ControlStorage`. The control bank owns the
scalar lifetime high-water marks, registration membership and both dirty queues.
It is not a member of `GameplayCollisionOwner`. Reference and
terrain-only reference construction require that backing explicitly; native
construction rejects it. The normal Amiga build reserves no such arrays.
Reference scene/field/entry verification builds construct them before the owner,
outside the task stack. Host fixtures hold them until after the owner dies.

`RunnerBorrowedRecords` does not allocate on first use. An absent or out-of-range
read/write latches terminal failure and immediately notifies the owner; no
additional per-frame validation scan is needed. The small failed-write sentinel
is not backing storage and can never make a failed owner valid again. Record
capacity changes do not change the borrower's size. These rules remove the
two scalar record arrays. The separate control-bank lease removes another
138,612 ordinary bytes on the 68000: its 138,616-byte bank is replaced by a
four-byte pointer. `GameplayCollisionOwner` is 904,628 bytes; its shared spatial
owner is 664 bytes. The index and its on-demand journal/tree retain their
existing ownership and are not part of the scalar bank.

Native construction explicitly declares external spatial ownership instead
of reading an always-empty scalar membership table before each index operation.
This declaration and scalar-bank binding are mutually exclusive. There is no
lazy allocation or fallback bank. Actual scalar consumers, including direct
lifetime and queue-count accessors, latch failure when backing is absent.
Reference-state inspection through `GameplayCollisionOwner::state` still returns
no scalar record for native ownership; native published-state APIs retain their
separate guards. Clear preserves the supplied bank's generation high-water
marks and is safe for an index-only owner without a bank. It does not switch
backend admission or release backing whose lifetimes may still be observed.
Startup/contact checks assert absence; full reference Results/Retry checks
assert presence. This changes ordinary residency, not Chip usage or the
registration representation.

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
| Gameplay-leased DMA plane atlas (cavern; full pixel oracle 124,146) | 118,914 |
| Effect surface | 20,448 |
| Offline interleaved smoke/gem stamps | 0 additional (4,932 in cache tail) |
| Shared scene bank (boot or front art; gameplay terrain scratch and field images) | 146,818 |
| **Gameplay total, before allocation alignment** | **494,654** |
| **Front-end total, without gameplay DMA atlas** | **375,740** |

W1.4's gun art (modules, letters, casing, laser and spread shots, the laser
muzzle's 270 page, the cropped pickup effect, notice icon and glyphs) added
8,334 Chip plane bytes and 17,762 cavern CPU bytes; against the 502,592 free
bytes measured at startup this leaves about 9 KiB of gameplay Chip headroom.
W1.5's time-shard sprite 91 (six frames) adds 1,170 plane bytes, leaving
about 8 KiB on that 512 KiB machine. On the 1 MiB target, normal startup
leaves 651,128 Chip bytes free, so gameplay keeps about 520 KiB in reserve.
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

The exit and upgrade-selection masks are scene-owned sidecars, not permanent
executable payloads. `Downwell.exitmask` (31,104 bytes) loads directly into the
existing, retired overlay Chip bank before exit-cover publication; no extra
allocation is made. `Downwell.selectionmask` (14,256 bytes) shares the ordinary
upgrade source allocation, is copied to the upgrade DMA layout on entry, and
is freed with that source on departure. Reentry loads a fresh source once.
Missing/short reads fail before publishing the cover/scene. No file I/O or
mask preparation belongs to ordinary contact gameplay. The exit byte oracle
alone retains its linked reference; production link audits reject both masks.
`Downwell.entrymask` (31,104 bytes) occupies the tail of the ordinary-memory
gameplay art lease. It loads once on acquisition and retires with that lease;
Retry retains it, while returning from style/surface reacquires it. The
gameplay CPU effect compositor borrows it beyond entry-cover DMA lifetime,
so detaching the hardware wipe must not retire this source. The shared Chip
overlay still receives its existing copy when needed. No extra Chip allocation
or per-frame file I/O is introduced. Only `ENTRY_WIPE_VERIFY=1` links a byte
oracle; the production link audit rejects it. Missing/short files fail before
binding the compositor or publishing the lease, returning code 20. Startup's
memory verifier requires zero entry-mask loads and resident bytes.

These leases remove 76,464 permanent payload bytes outside their scenes.
Selection temporarily reacquires its 14,256 bytes and gameplay its 31,104
entry-mask bytes in ordinary memory; none changes the reserved Chip layout.
Fixed mask budgets and hashes are in
the generated transient-bank manifest. The assembly option fingerprint must
include the exact verifier definitions, not just whether any VERIFY flag is
present, so switching verifiers cannot retain linked oracle assets.

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

- `amiga-game.dwap` (`Downwell.game`), 260,150 bytes of CPU-only reference art,
  and `amiga-game.planes` (`Downwell.planes`), 124,146 bytes of DMA reference
  art. Only explicit replay/plane pixel oracles may load these catalogues.
  The normal link audit rejects their load paths in production code.
- `amiga-cavern.dwap` (`Downwell.cavern`), 246,710 CPU bytes, paired with
  `amiga-cavern.planes` (`Downwell.cavernplanes`), 118,914 DMA bytes, replaces
  that union during cavern gameplay. `CAVERN_ROOTS` is independent of future
  union additions. The exporter checks fixed 248 KiB CPU/117 KiB DMA ceilings
  and retained payloads against the full pack. Scene retirement drains
  DMA and invalidates every borrowed CPU/DMA view before release;
  upgrade entry releases the lease and next-level entry reacquires it.
  Full-catalogue replay
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

The upgrade source is 185,152 bytes, including 15,600 bytes of player DMA
records for the six living poses returned by `PlayerMotion::spriteId(false)`.
These use the same offline encoder as gameplay, with byte-for-byte checks;
no planar player copies are retained. `UPGRADE_PLAYER_IDS` is independent of
future gameplay catalogue growth. The reviewed source ceiling is 184 KiB.
Shops/results retain their stricter planar-only validation; upgrade validates
the full directory including CPU-only sprite streams.

After retiring all gameplay DMA, upgrade entry invalidates borrowed actor,
effect, terrain-batch and HUD views and frees the 228,948-byte CPU catalogue
and 109,410-byte Chip plane allocation. It also releases terrain backing and
CPU field columns. Selection never reads `activeGamePack`. Next-level entry
retires selection DMA, releases its source, then reloads/rebinds gameplay.
Relative to retaining cavern art, this saves 205,682 ordinary bytes (measured
before W1.4's gun art) and 117,744 Chip bytes during selection; gameplay residency is unchanged. Fixed shared
scene storage and other embedded tables are not included in these savings.
The native clear fixture requires zero gameplay leases during both selections
and three loads/two releases on return to the third level. Host admission tests
also grow the unrelated gameplay catalogue without growing selection.

The front bank puts its complete directory and all live garden/style payloads
in a 64,936-byte prefix. Within it, all style-readable resources and the
complete directory occupy the first 37,030 bytes. Only one-shot garden
scenery follows the live prefix;
`AmigaGardenScene::prepare` copies it into the scrolling canvases before any
staging starts. Writable sprite DMA headers never alias another payload.
Large-field arcs are not prepared in the garden. After retiring front-end DMA,
gameplay entry reads `Downwell.fields` directly into the existing range at
75,360. The generated size must fit its reviewed 71,456-byte budget. A rolling
word checksum verifies the offline big-endian stream before DMA pointers are
published, and a zero guard word follows it. A missing, short or corrupt read
fails entry with return code 20; there is no full-catalogue or linked fallback.
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
unfinished small-image upload (including skipped/short garden or style paths),
loads the large-image file if absent, then binds the immutable plans; partially
loaded images are never exposed to DMA. Upload completion and residency are separate:
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

The hardware-sprite copy cache is valid only within an immutable source-bank
and destination-buffer lifetime. Gameplay and upgrade source release invalidate
it before memory can be freed/reused; equal addresses do not imply equal asset
contents after re-entry. Unchanged live artwork retains header-only updates.
Any new owner that reuses source or destination storage must call
`amigaRendererReleaseHardwareSpriteCopies` at that boundary. The host native-cache
fixture covers same-address bank replacement, destination reuse and copy counts.

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

Registered Shop parents and children borrow an explicit
`GameplayShopInstances::Storage` bank. The current native frontend uses the
compatibility Shop path and admits no such bank; normal startup and the contact
profile enforce a null backing pointer. This does not disable compatibility
shops or install the unfinished registered-Shop session. Its eventual scene
owner must bind empty backing before registered creation, retain it through all
parent/child callbacks, and retire every record before withdrawing it. Missing
backing rejects parent and child births before source-ID allocation or
publication. Unload/re-entry preserves lifetime generations; live or nonempty
replacement backing is rejected. Reference fixtures explicitly supply their
bank. The store itself contains no capacity-sized array, so increasing this
unadmitted family's capacity cannot grow ordinary scene residency.
The 96-record bank occupies 20,928 ordinary bytes on the 68000. The native
owner saves 20,924 bytes net of its borrowed pointer; no Chip allocation is
changed. This is a residency change, not a gameplay frame-time improvement.

Skeleton and bone records borrow an explicit
`GameplayCollisionOwner::SkeletonEncounterStorage` bank. Cavern startup and
contact-profile guards require both pointers to be null; there is no implicit
allocation on a birth. An unadmitted birth fails before family-local lifetime
allocation, RNG consumption or publication. Stored/reference registrations
explicitly own their backing and retain the same record capacity and behavior.
The bank must outlive its borrowers. Binding or withdrawing backing is allowed
only with no live records and no executing event; supplied backing must be
empty. Retirement followed by withdrawal and re-entry preserves generation
counters, so old handles cannot address replacement actors. These are ordinary
records, not sprite pixels or Chip allocations. Moving them out of the native
owner removes 19,128 bytes on the 68000, net of two borrowed pointers. This does
not install the still-unfinished registered skeleton encounter in production
or partition every other family's storage.

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
