# Amiga performance instrumentation

## Current checkpoint (2026-09-25)

The historical 50 FPS result below predates subsequent fidelity work and is
not the current performance claim. The retained ammo HUD fixes visible clears
and reduces its measured work substantially, but the current 600-update
firing tape runs at 36.41 FPS. See [the HUD checkpoint](amiga-ammo-hud-performance.md)
for exact scope, phase timings, remaining budget work and pixel verification.

### Separate runtime costs (2026-09-25)

The old `player` phase also covered camera and procedural generation, while
`gems` covered retirement and HUD state after the gem loop. Four additional
probe-only phases now separate these costs; existing phase IDs are stable.
The GDB report derives its loop bound from the actual counter-array size.

| Phase | Work | Total / 600 updates | Mean / update | Maximum bracket |
| --- | --- | ---: | ---: | ---: |
| 31 | Camera update | 288.107 ms | 0.480 ms | 0.717 ms |
| 32 | Generation callback | 484.155 ms | 0.807 ms | 102.096 ms |
| 33 | Exit/retirement/results lifecycle | 308.130 ms | 0.514 ms | 0.941 ms |
| 34 | HUD animation and frame flags | 791.261 ms | 1.319 ms | 2.096 ms |

Evidence: `/tmp/downwell-runtime-phases-profile.log`, bounded 45-second
`SKIP_TO_GAME=1` probe run with the same 600-update tape. These are
instrumented timings, not a new shipping-FPS measurement. The generation
spike and recurring HUD-state cost deserve separate investigation. In
particular, phase 34 includes the 184-row animation loop even when settled;
the measurement does not yet isolate that loop from the other frame flags.
All new probes compile out of ordinary and FPSCOUNT-only builds. Host runtime
fixture digests remain unchanged (`7e82f060aa3fcbf9`, `0db388a5b1c2a40a`).

### Exact power-of-two integer RNG ranges

`irandomSmall` now reduces power-of-two divisors by masking the low word,
after consuming both original WELL512 words. This is algebraically identical
to the recovered positive-63-bit remainder, including bounds zero and 65535;
it does not substitute another RNG or skip draws. Tests compare every bound
0..65535 with `irandom`, including the following stream word and draw count.

The same 600-update probe tape reduces total generation time from 482.565 to
463.278 ms, with instrumented fields falling from 1,134 to 1,127. The maximum
generation bracket remains about 102 ms (102.373 ms in this run), so this
optimization **does not fix the generation spike**. The remaining spike
needs finer attribution rather than assuming RNG arithmetic dominates it.
Evidence logs use `/tmp/downwell-rng-power2-`.
The FPSCOUNT tape takes 903 PAL fields for 600 updates (33.22 FPS, maximum
gap six fields), versus 904 fields previously: essentially unchanged, not a
meaningful frame-rate fix by itself.
The full host suite, bounded native cross-target replay and both 68000
audits pass. All replay digests remain unchanged; the verification workflow
restores the ordinary non-skip executable.

### Generation callback attribution

Probe phases 35 (generated-object registration) and 36 (terrain index/tile
append) split callback work out of phase 32. Before further RNG changes, the
same 600-update tape reports 432.597 ms remaining in the builder, 50.429 ms
in registration and 56.472 ms in index/tile append (203 generated callbacks).
Most measured generation work therefore remains outside those callbacks.
Evidence: `/tmp/downwell-generation-detail-profile.log`.

Nested probes split one long callback into many exclusive brackets. Its old
102 ms maximum cannot be compared with the new 9.514 ms phase-32 bracket as
though the stall were fixed. The extra probes also add overhead: this run
uses 1,136 fields and has a 159.643 ms maximum whole frame. Use uninstrumented
FPSCOUNT for shipping performance; all new callbacks compile out otherwise.

### Three-remainder small-range RNG reduction

The non-power-of-two path now reduces the high word, appends the upper 16
bits of the low word and reduces again, then appends the remaining 16 bits
and reduces once more. Each intermediate fits uint32 because the remainder
is below a divisor of at most 65536. This replaces six remainder operations
and a multiply with three remainders, preserving both stream draws exactly.

With the same detailed probes, builder-exclusive time falls from 432.597 to
424.684 ms over 600 updates. Fields are 1,135 versus 1,136; the whole-frame
maximum remains 159.589 ms with probes. This is a small arithmetic saving,
not a solution to the generation stall. Evidence uses
`/tmp/downwell-rng-word-`; all supported bounds are tested against the
original positive-63-bit remainder and subsequent RNG stream word.
FPSCOUNT reports 904 fields for 600 updates (33.19 FPS, maximum gap six),
versus 903 previously: effectively unchanged, not a frame-rate improvement.
The full host suite and bounded native replay pass with unchanged digests.
Both 68000 audits pass, and the ordinary non-skip executable is restored.

### Read pattern rows once

Area-one generation now requests a validated immutable row view rather than
decoding the same little-endian row record and checking its ranges for every
cell. Per-row widths (including the anomalous shop row) and cell order remain
unchanged. The view borrows the pack storage and introduces no allocation,
format change, RNG draw or scheduling change. It validates both row and cell
ranges, including caller-supplied pattern metadata, before exposing a pointer.

All 737 rows / 8,435 cells are compared with individual cell reads; invalid
rows, cell ranges and widths are rejected. The pack sanitizer and deterministic
builder tests pass. With the same detailed native probes, builder-exclusive
time falls from 424.684 to 398.312 ms over 600 updates (6.2%). Maximum whole
frame falls from 159.589 to 146.233 ms, still far outside the 20 ms budget.
Instrumented fields are 1,136 versus 1,135, so do not infer a general frame-rate
gain from that maximum alone. Evidence uses `/tmp/downwell-builder-rows-`.
FPSCOUNT-only result: 901 fields for 600 updates (33.30 FPS, maximum gap
five fields), versus 904 fields and a six-field maximum before this change.
The complete host suite, bounded native replay, and both 68000 audits pass.
All replay digests are unchanged, and the normal non-skip executable is restored.

### Bound traversal to occupied bullet/gem slots

The native runtime retains an occupied-end index for each pool. Births still
use the shared stable compaction in creation order, with the original full
capacity available. Only inactive tails are trimmed after stepping; holes
remain untouched until the next birth. Slots beyond the end are always
inactive. Simulation, counting and actor presentation can omit this unused
tail without changing event order, birth limits, RNG or sprite allocation.
Level transitions reset both bounds along with their pools.

Tests compare bounded counts with full-capacity scans over 1,200 updates and
assert that no live slot is omitted; reward births and complete collection
also exercise gem-bound expansion and contraction. Runtime fixture digests
are unchanged. In the same 600-update probe tape, actor presentation costs
1,512.311 ms (2.521 ms/update), gem stepping 58.330 ms (0.097 ms/update), and
the instrumented tape uses 1,096 fields versus 1,136 previously. This is
still not a 20 ms frame-budget pass. Evidence uses `/tmp/downwell-pool-span-`.
The FPSCOUNT-only tape uses 858 PAL fields for 600 updates (**34.97 FPS**,
maximum gap five fields), down from 901 fields / 33.30 FPS.
The full host suite and bounded native replay pass with unchanged digests.
The native gameplay shot capture has 0/99,968 differing pixels against SDL.
Both 68000 audits pass; the ordinary non-skip executable is restored.

### Collision route counters

Probe-only counters distinguish indexed solid/landing lookups from slow
serialized-terrain fallbacks, and count breakable full scans. They reset with
the excluded first gameplay setup frame and are printed by `profile.gdb`.
They compile out of normal and FPSCOUNT builds.

The 600-update tape reports **10,725 indexed solid queries, 600 indexed
landing queries, zero solid/landing fallbacks, and zero breakable fallbacks**.
Thus the measured collision phase is not evidence that the spatial index is
being lost. Frequent timing brackets also add overhead; do not equate this
instrumented phase directly with shipping cost. Evidence:
`/tmp/downwell-collision-routes-profile.log`. This diagnostic change makes
no new FPS claim and does not alter collision behavior.

### Restore actor damage only in the touched display

BOBs previously marked their rows dirty in both display buffers, although
only the inactive buffer received the drawing. Actor damage now marks only
that buffer; changes to the terrain backing still mark both. OR-ing the
damage bits preserves outstanding terrain or actor damage in either buffer.
This avoids requesting restoration solely because an actor was drawn in the
other display. It does not yet retain unchanged BOB compositions.

The bounded native forced-BOB scalar oracle and 600-tick terrain/cache oracle
both pass, as do the 68000 instruction/helper audits. The FPSCOUNT tape takes
860 PAL fields for 600 updates: **34.88 FPS**, maximum gap five fields. This
is essentially unchanged from 858 fields / 34.97 FPS; no frame-rate gain is
claimed. Evidence: `/tmp/downwell-bob-damage-{verify,cache,fps}.log`. Emulator
runs are limited to 45 seconds; the ordinary non-skip build is restored.

A separate experiment caching actor bounds between collection and BOB
drawing was rejected after measuring 866 fields / 34.64 FPS. Its extra actor
record fields are not retained. Evidence: `/tmp/downwell-bob-bounds-fps.log`.

### Retain composed divider artwork

`AmigaAmmoHud` now caches the ordered divider pixels as a single native-word
column when ammo or capacity changes. Row-animation strips reuse that column;
the four meter jiggle positions translate it vertically. Capacities above 64,
unsupported stamps and forced full draws retain the old path. This adds
1,136 bytes of planar words plus two cache keys in Fast RAM, no Chip buffer.

The divider probe total falls from 481.339 to **358.377 ms / 600 updates**
(0.802 to 0.597 ms/update, about 26% less), with the maximum bracket falling
from 10.527 to 5.147 ms. Other HUD phases remain essentially unchanged. The
shipping-style tape is **859 PAL fields / 600 updates = 34.92 FPS**, max gap
five fields: no meaningful overall FPS improvement over 34.88 FPS. The
20 ms requirement remains unmet.

Expanded host tests compare every capacity 1..64, every ammo count and all
four offsets against the ordered full-render oracle, in addition to existing
animation, fallback and unchanged-write tests. These also pass ASan/UBSan.
The bounded native shot capture differs from SDL by 0/99,968 pixels, and both
68000 audits pass. Logs: `/tmp/downwell-divider-cache-{test,sanitized,frame,
fps,profile,restore}.log`. Native runs are limited to 45 seconds and the
ordinary non-skip executable is restored. These tests preserve the existing
source-derived rendering; they do not independently prove Windows fidelity.

### Separate HUD damage detection and publication

New probe-only phases isolate work formerly charged to base/fill and
rim/label. The same bounded 600-update tape reports:

| Phase | Calls | Total ms | ms/call | Maximum bracket ms |
| --- | ---: | ---: | ---: | ---: |
| 37: damage detection | 139 | 639.735 | 4.602 | 6.148 |
| 38: changed fill geometry | 41 | 16.532 | 0.403 | 0.421 |
| 39: changed-word publication | 324 | 459.612 | 1.419 | 6.074 |

Thus rebuilding fill geometry is not the primary remaining base-phase cost.
Damage detection scans all 184 animation states and copies the previous row
state; it is the next optimization target. Publication still matters, but a
previous shadow-buffer experiment did not improve it. These are exclusive
instrumented brackets, including probe overhead, not shipping CPU timings.
Instrumented fields are 1,112; no new shipping-FPS claim is made.

The expanded host renderer suite and both native 68000 audits pass. The
45-second bounded run cleaned up and the ordinary non-skip executable was
restored. Logs: `/tmp/downwell-hud-detail-{profile,test,restore}.log`.
All three added timing scopes compile out without `PROBES=1`.

### Bound damage by changed endpoints

The HUD still requests the same single bounding strip, but now finds the
first and last changed animation frames without inspecting the interior.
Fill-family changes are a contiguous interval whose bounds are included
once, rather than compared at every row. States 3 and 4 continue to share
frame 3; rows hidden by the original y<232 test remain excluded.

The same probe tape reduces phase 37 from 639.735 to **365.368 ms** over 139
HUD updates (43% less; 2.628 ms/update). The FPSCOUNT tape takes **847 fields
for 600 updates = 35.42 FPS**, versus 859 / 34.92 FPS before; maximum gap is
still five fields. This does not meet the 20 ms budget or remove generation
spikes.

12,000 dense/sparse state and fill-boundary cases compare exact top/bottom
bounds against the previous full scan. Existing exhaustive capacity/ammo/
jiggle pixel comparisons and ASan/UBSan also pass. Evidence logs use
`/tmp/downwell-damage-endpoints-` with `test`, `sanitized`, `profile`, `fps`
and `frame` suffixes. The native shot capture matches SDL with 0/99,968
differing pixels, both 68000 audits pass, and the ordinary non-skip build is
restored. Every emulator run is bounded to 45 seconds.

### Publish narrow HUD strips without the general width loop

Changed-only one-word strips now walk both planes together, avoiding the
general per-row width loop. Each plane is still compared independently and
unchanged words are never written. Wider rectangles and unconditional copies
retain the general implementation; no additional retained buffer is used.

Phase 39 falls from 459.644 to **325.872 ms / 600 updates** (29% less), with
the maximum bracket falling from 6.017 to 3.280 ms. The lightweight tape is
**840 PAL fields / 600 updates = 35.71 FPS**, versus 847 / 35.42 FPS before.
Maximum gap remains five fields; this is not completion of the 20 ms gate.

Direct tests cover all 24 columns and all combinations of modified planes,
check exact write counts, preserve outside rows/columns, and require zero
writes on repetition. Full retained-HUD pixel comparisons and ASan/UBSan pass.
Evidence logs use `/tmp/downwell-word-publish-` with `test`, `sanitized`,
`profile`, `fps` and `frame` suffixes. The native shot capture matches SDL
with 0/99,968 differing pixels, both 68000 audits pass, and the ordinary
non-skip executable is restored. Emulator runs are limited to 45 seconds.

### Exact equal-denominator projectile comparisons

The shared continuous projectile/rectangle test compares numerators directly
when the positive denominators match. In particular, an untouched initial
0/1..1/1 interval no longer needs wide cross-products. Unequal denominators
retain the original calculation; strict edge handling, collision order and
RNG consumption are unchanged.

100,000 fraction cases match host 64-bit cross-products, and 490,000 segment
intersections match the independent slab oracle. The latter also pass
ASan/UBSan. Runtime fixtures and bounded native cross-target replay retain all
four digests, including weapon digest `0db388a5b1c2a40a`, coverage `3fff`.
Both 68000 audits pass and the ordinary executable is restored.

FPSCOUNT reports **839 fields / 600 updates = 35.76 FPS**, max gap five,
versus 840 / 35.71 previously: essentially unchanged, not a meaningful FPS
gain. This small arithmetic simplification does not resolve the frame budget
or generation stall. Logs: `/tmp/downwell-fraction-compare-{test,sanitized,
fps,replay}.log`; emulator runs are bounded to 45 seconds.

### Rejected projectile rejection experiments

Two follow-ups passed the independent intersection oracle, sanitizers,
runtime fixtures and bounded native cross-target replay, but were not kept:

- A preliminary two-axis bounding rejection duplicated the existing clipping
  checks and slowed the tape to 858 fields / 34.97 FPS (baseline 839 / 35.76).
- Doing the existing y clipping before x took 838 fields / 35.80 FPS, only one
  field different from baseline. This is not sufficient evidence to favor
  that order for the game generally.

The production intersection code is unchanged. Additional regressions cover
positive/negative vertical separation despite horizontal overlap, zero-width
and reversed rectangles. The restored host test passes all 490,000 slab
oracle cases; the ordinary non-skip native build and 68000 audits pass.
Logs use `/tmp/downwell-segment-reject-`, `/tmp/downwell-segment-yfirst-` and
`/tmp/downwell-segment-experiments-` prefixes. Runs were bounded to 45 seconds.
Further collision work needs attribution within actual hit/miss queries,
not additional unmeasured broad-phase checks.

### Query bullet terrain by indexed cells

The native terrain backend now supplies an optional segment resolver. Bullet
terrain collision visits the overlapped 16x16 indexed cells and intersects
their half-open rectangles once each, rather than resolving every pixel in
the segment's bounding rectangle. This is exactly the union of the previous
pixel rectangles. Active breakables still use the runtime's current state;
enemy and breakable impact order before the wall query is unchanged.

Backends without a segment resolver retain the pixel-query path. A terrain
context without an index also falls back to pixel queries. Callback rebinding
preserves the optional resolver, while next-level reset clears it. Both native
game-entry sites install it; no additional terrain storage is allocated.

86,700 fractional segments compare indexed/pixel results across cell edges,
corners and active/inactive breakables. A 600-tick live-generated firing and
movement run compares checkpoints, rolling digests, destroyed-block counts
and full RNG state on every tick with the pixel-query runtime. Both tests
also pass ASan/UBSan. Native shot pixels match SDL (0/99,968 differences), and
the bounded cross-target replay retains all four digests.

The lightweight tape improves from 839 to **824 PAL fields / 600 updates =
36.41 FPS** (previously 35.76). Maximum gap remains five fields: the 20 ms
budget and generation stall are still open. Evidence logs use
`/tmp/downwell-segment-index-` with `test`, `live-test`, `sanitized`, `fps`,
`frame`, `replay` and `suite` suffixes. The complete host suite and both 68000
audits pass; the ordinary non-skip executable is restored. Native runs are
bounded to 45 seconds.

### Refreshed indexed-query profile

Probe-only counters now distinguish segment queries, visited cells and the
segment fallback. The 600-update tape reports **464 indexed segments, 719
visited cells, zero segment fallbacks**, alongside 5,923 indexed solid-point
queries, 600 indexed landing queries and no point/landing/breakable fallbacks.
The segment backend is being used, not silently falling back to pixel walks.

Selected exclusive instrumented costs (ms per game update): player 3.807,
actor presentation 2.512, terrain collision 1.869, enemy movement 1.575,
scroll 1.388, bullet work outside terrain queries 0.941. HUD phases
27..30,37..39 total about 3.87 ms/update. Maximum instrumented frame is still
147.551 ms; the generation spike remains unresolved.

There are fewer nested timing brackets now that bullets no longer call the
point resolver repeatedly. Do not interpret the instrumented timing reduction
as pure shipping CPU savings: probe overhead changed too. The authoritative
FPSCOUNT measurement remains 36.41 FPS. The new counters compile out of
ordinary builds; no new frame-rate gain is claimed here.

Evidence: `/tmp/downwell-indexed-query-profile.log`; the bounded run cleaned
up, restored the ordinary non-skip build, and passed both 68000 audits.
Inspection identifies repeated indexed breakable candidates and a repeated
selected-hit intersection as a next optimization candidate, not yet changed.

### Avoid repeated indexed breakable candidates

Bullet breakable selection now visits distinct endpoint cells rather than
always querying all four endpoint combinations. When both endpoints lie in
one cell, only that cell is queried; corner-crossing segments still consider
all four cells. Selection still chooses the earliest-created intersecting
active block. The selected indexed hit is not intersected a second time;
the general full-scan fallback retains its intersection test.

Eight seeded 120-tick firing/movement comparisons run the indexed path against
the full-scan fallback with identical nearby geometry and both creation
orders. They verify checkpoints, rolling digests, RNG draw counts, destroyed
block notification order and each block's active state; every run destroys
blocks. Runtime/live-terrain tests, runtime ASan/UBSan and bounded native
cross-target replay pass. All replay digests remain unchanged, both 68000
audits pass, and the ordinary non-skip executable is restored.

The FPSCOUNT tape remains **824 PAL fields / 600 updates = 36.41 FPS**, max
gap five. No measurable FPS improvement is claimed. Logs use
`/tmp/downwell-breakable-cells-` with `test`, `regression`, `sanitized`, `fps`
and `replay` suffixes. Native runs are bounded to 45 seconds.

### Split player movement from shop and setup work

Probe-only phases 40 and 41 separate the call to `player_.step` and the shop/
keeper sequence from phase 16. The current 600-update tape reports:

| Phase | Total ms | ms/update |
| --- | ---: | ---: |
| 40: movement, excluding nested terrain queries | 1,325.580 | 2.209 |
| 41: shops and keepers | 242.634 | 0.404 |
| 16: remaining setup/player work | 897.329 | 1.496 |

Movement has 4,865 timing brackets because nested terrain queries split the
parent scope. Its 1.017 ms maximum bracket is **not** a maximum whole movement
call. All values include instrumentation overhead. This is diagnostic only;
the shipping baseline remains 36.41 FPS, and the maximum instrumented whole
frame remains 147.067 ms.

Runtime fixture digests and both native 68000 audits pass. The bounded run
cleaned up and restored the ordinary non-skip build. Logs use
`/tmp/downwell-player-detail-{profile,test,restore}.log`.
Inspection identifies repeated four-corner player-mask queries as a possible
backend batching target. Any batching must preserve those corner semantics,
not substitute full rectangle overlap or change source movement order.

## Historical gameplay result: 600 frames without a missed PAL field

The 2026-09-23 optimized `SKIP_TO_GAME=1` run completed **600 frames in
600 PAL fields: 50.00 FPS, maximum gap one field**. This includes the
160-pixel side-room camera change. It is evidence for this deterministic
window, not a guarantee for every encounter or later level.

Configuration: A500/68000, PAL, 512 KB Chip RAM and 8 MB Fast RAM; `-O3 -flto`;
`FPSCOUNT=1 PROFILE_AUTORUN=1`; 45-second host timeout. Sprite priority,
22-word data fetch and the copper-reset circular scroll remain unchanged.
No game-speed compensation is applied on PAL.

The major changes are:

- Spatial terrain/landing and breakable indices replace repeated serialized
  resource searches. Host tests compare these with the original queries.
- Gameplay checksums remain in host/replay/scene-verification builds, but are
  not recalculated every frame in the playable build.
- HUD digits and mirrored scenery are composed offline. HUD assets are moved
  to retired intro Chip RAM, so the framework blitter can actually draw them.
- The static level raster is composed once at level entry. New rows transfer
  into the private terrain ring; unchanged rows survive camera reversals.
  Destroyed blocks invalidate the affected cached rows, including hidden rows.
- Side-room views are prepared incrementally in retired garden memory on
  frames without a new terrain strip. On a camera change that ring becomes
  the inactive display ring. Its restoration-copy DMA overlaps sprite setup.
- Copper programs retain their layout and patch only pointers/wrap waits.
  The two display rings synchronize only dirty, word-aligned gameplay spans.

Memory trade-off: the Fast-RAM raster reserves 1,081,344 bytes plus 24,576
dirty flags (about 1.06 MiB combined). The terrain/landing index uses 192 KiB;
the breakable index uses 32 KiB. Staging reuses 30,720 bytes of retired garden
Chip RAM; no new Chip allocation was added. This does **not** claim stock
A500 memory compatibility. Level-entry composition is excluded from the
steady-state frame window.

`make verify-amiga-gameplay-cache` checks 600 simulation ticks, compares
cached transfers with fresh terrain composition (146 comparisons in the
recorded run), checks staged view pixels and compares display rings with the
static backing. The 68000 MOVEM row-copy twin is included in these checks.
Host renderer tests independently compare mirrored scenery and all ten
precomposed ammo digits against their original drawing operations, in addition
to the 12,348 offline hardware-sprite comparisons.
The normal-intro grounded and active gameplay captures retain identical
decoded display pixels to the pre-optimization captures. Their raw capture
hashes change because the retained hidden ring rows change. Cross-target
replay also passes (`64 95d0aaea 1be7256f`).

The progression was 0.78 FPS initially, 31 FPS after indexing, approximately
40 FPS after retained-row fixes, then 49.92 FPS with one missed side-room
deadline. Overlapping that restoration DMA with sprite setup removed the
remaining miss. Longer/different deterministic scenarios should be added
before treating this as a general worst-case frame-budget guarantee.

Run from the repository root:

```
bash amiga/profile.sh fps
bash amiga/profile.sh probes
```

Each command clean-builds its own configuration, drives the same deterministic
garden input (40 idle frames, then right), and freezes after 180 completed
garden iterations. GDB stops only once, after that window; it never samples
with conditional breakpoints inside the measurement. The emulator has a
60-second host safety timeout, overridable with `VERIFY_TIMEOUT_SECONDS`.
Timeout and success both clean up the emulator; the wrapper restores a normal
build on exit. Reports remain in `amiga/.run/profile-fps.log` and
`amiga/.run/profile-probes.log` (ignored).

## Two distinct modes

Gameplay: `SKIP_TO_GAME=1 VERIFY_TIMEOUT_SECONDS=45 bash amiga/profile.sh probes`
(or `fps`). This measures 600 iterations after discarding the initial gameplay
composition frame. Deterministic input alternates direction every 120 ticks
and fires on ticks 0 and 3 modulo 16. Phase 4 means HUD updates for gameplay;
phase 6 includes terrain row transfers/staging. Missed deadlines report their
frame number, field count, scroll delta and horizontal camera change.
This is not representative of every level/encounter.

First gameplay sample after offline sprite conversion (30 frames, PROBES):
simulation 1028.250 ms/frame, terrain 224.516 (max bracket 3437.227), HUD
14.691, sprites 7.245, copper construction 2.927, publication 0.147.
These instrumented timings exclude the one-time scene build and are not a
shipping FPS measurement. No clock clamps were recorded.

Gameplay sprite bounds, slicing, mirroring and planar-to-sprite interleaving
are generated on the host. The copy-only payload remains in Fast RAM; per-DMA
channel caches copy prepared words only when the variant/vertical DMA range
changes. Position-only updates touch the header. Horizontal viewport edges
use PF2 priority and opaque-black HUD margins, not sprite pixel clipping.
The original pixel converter remains a host test oracle; target builds never
fall back to runtime conversion. Tests cover 12,348 variant/position cases.

## Historical baseline: retained gameplay terrain and measured bottleneck

Gameplay now retains one private 384x320, two-plane terrain ring (30,720
bytes). Two equally sized display rings reuse 61,440 bytes of the finished
intro/style cache, without increasing Chip RAM allocation. Both display rings
track dirty physical rows independently. Blitter copies synchronize only dirty
row runs into the inactive ring; the copper still scrolls by bitplane pointers
and resets those pointers at ring wrap. Hardware sprites do not touch either
terrain bitmap and therefore need no background restoration. This backing is
also the restoration source for future software-drawn moving objects.

New bottom strips are drawn into the private backing, never the visible ring.
Upward scrolling draws a new top strip in one pass. Side-room changes rebuild
the viewport off-screen in one pass, not 284 separate line scans. Destroyed
blocks redraw their clipped 16x16 region, not 16 entire rows. Unchanged terrain
is not redrawn for ordinary player/sprite motion. The initial grounded capture
checks all display-ring bytes against the backing (including a ring wrap).

The expanded 30-frame PROBES sample separates terrain collision (phase 14)
and dirty-row copies (phase 15):

| Work | Average ms/frame |
| --- | ---: |
| Terrain collision queries (302 calls) | 1008.434 |
| Remaining simulation | 22.126 |
| Terrain strip drawing | 224.669 |
| HUD updates | 14.802 |
| Prepared hardware sprites | 7.261 |
| Dirty-row copies | 3.319 |
| Copper construction | 3.120 |

Separate low-overhead FPSCOUNT run: 30 frames / 1,931 PAL fields = **0.78
FPS**, worst gap 241 fields. This confirms that the dominant costs are real,
not just the detailed-probe overhead. Both runs used the same deterministic
gameplay inputs and 45-second host safety limits. The wrapper restored the
normal `SKIP_TO_GAME=1` executable afterwards.

Buffering prevents visible partial updates; it does **not** solve the dominant
CPU bottleneck. `amigaResolveTerrainSolid` walks serialized room instances for
each query, and both collision resolvers traverse pattern rows/cells repeatedly.
Drawing likewise searches room instances and repeatedly resolves wall neighbours.
The next optimization should be a predecoded spatial terrain/collision index,
checked against the current resolvers as an oracle—not further sprite masking
or full-screen redraws. The later active-enemy capture exceeded its 45-second
host limit; the verifier stopped the emulator rather than leaving it running.

`FPSCOUNT=1` counts completed loop iterations and elapsed real VERTB fields,
plus the worst per-iteration field gap, per scene. The garden publishes one
frame per loop. FPS is `50 * completed garden frames / elapsed PAL fields`.
This is an emulated-machine measurement, not emulator host throughput. The
profiling launcher uses A500 CPU settings and the existing 512 KB Chip/8 MB
Fast configuration; this is not a claim about stock-A500 memory requirements.

`PROBES=1` adds exclusive phase totals, call counts, maximum bracket lengths
and maximum whole-loop time. These numbers include instrumentation overhead;
do not quote their frame rate as production FPS. They follow the Revs
beam-epoch scheme: VERTB advances an epoch by `313 * 256` ticks, and a stable
VPOSR/VHPOSR sample supplies the position within the field. Retry interrupted
samples and vertical-high-bit transitions; clamp backward samples in the
beam-wrap/interrupt-latency interval and count those clamps. Conversion is
nominal PAL time: `ticks * 20 / 80128` milliseconds. It is a beam timing
estimate, not an exact CPU-cycle counter, and includes interrupts/DMA stalls.

Phases are other, VBlank waiting, input, simulation, title updates, sprite
selection/position, scrolling calculations, copper updates and publication.
Phase totals are exclusive, not nested. `max_bracket_ms` is a single bracket,
not a whole frame's aggregate when a phase is visited more than once. The
RAII frame boundary closes measurements on `continue` paths as well. Small
loop-boundary/profiler bookkeeping outside that boundary is not in the phase
totals. Timing counters freeze at the completed-frame boundary, while the
underlying VBI clock may continue; the report uses the frozen scene counters.

Garden, style-select and gameplay stages have detailed brackets. Style phases cover
background-copy submission, actors, text, final blitter drain and the CPU
mirroring fallback separately. Gameplay separates collision queries and
backing-to-display copies; splash has wait, input/simulation and an
undifferentiated remainder. Startup asset loading
before the main loop is not yet bracketed. Neither instrumentation mode is
enabled in normal builds. Verification builds are rejected in combination
with profiling: their reference rendering must not pollute performance data.

## First measurement

The detailed 180-frame garden run after introducing these helpers reported
180 PAL fields, no clock clamps, approximately 16.306 ms/frame waiting and
3.504 ms/frame in the remaining measured phases. Largest average work phases:
sprite selection/header updates 0.782 ms, simulation 0.653 ms, copper 0.499 ms,
scroll calculations 0.490 ms. These are instrumented results, not budgets or
guarantees for every game scene. Title changes and first-use copper construction
produce spikes that the maximum columns retain.

The separate lightweight run reported 180 frames in 180 PAL fields: 50.00 FPS,
with a maximum one-field iteration gap. This establishes the sampled garden
window only, not the whole game's frame rate.

## Style-select measurement (2026-09-23)

`SKIP_INTRO=1` selects a 180-frame style-select window instead of the garden.
`PROFILE_NAVIGATE=1` adds alternating left/right key presses every 16 frames
after entrance easing completes; direction reverses every 80 loop frames.
It uses normal edge-triggered input and never selects a style. For example:

```
SKIP_INTRO=1 VERIFY_TIMEOUT_SECONDS=45 bash amiga/profile.sh fps
SKIP_INTRO=1 PROFILE_NAVIGATE=1 VERIFY_TIMEOUT_SECONDS=45 bash amiga/profile.sh fps
SKIP_INTRO=1 PROFILE_NAVIGATE=1 VERIFY_TIMEOUT_SECONDS=45 bash amiga/profile.sh probes
```

Measured after `bd88ef7`, using the same A500/512 KB Chip/8 MB Fast setup:

| Lightweight test | Frames | PAL fields | FPS | Worst field gap |
| --- | ---: | ---: | ---: | ---: |
| Entrance + idle usual style | 180 | 180 | 50.00 | 1 |
| Entrance + left/right navigation | 180 | 281 | 32.03 | 2 |

The navigation run missed one refresh on 101 of 180 iterations. Detailed
instrumentation is substantially more intrusive near the 20 ms threshold;
its FPS must not replace the lightweight result above. The final navigation
probe run reported these average exclusive brackets across all 180 frames:

| Bracket | ms/frame |
| --- | ---: |
| Actors, excluding CPU mirror | 10.271 |
| CPU-mirrored left arrow | 3.796 |
| Text | 5.523 |
| Final blitter drain | 1.374 |
| Background-copy submission | 0.820 |
| Simulation | 0.468 |
| Copper updates | 0.378 |
| Publication | 0.154 |
| Input + other | 1.064 |

The mirror ran 101 times: 683.232 ms total, **6.765 ms per invocation**,
maximum bracket 7.014 ms. Its nested pixel loop still flips the 16x16 arrow
on the 68000 and reads/writes Chip RAM. This is distinct from the right
arrow's corrected Bitmap masked-blit path. The higher actor cost also
includes additional visible/clipped carousel icons. The phase brackets
include CPU setup and concurrent DMA contention, not isolated blitter times.
In particular, most of the background copy executes after its submission
bracket; the final drain measures only what remains queued at the end.

Style selection still copies both complete 176x284 planes (12,496 bytes),
looks up/clips/draws carousel icons and redraws headings every frame. The
black transition mask is immutable and pointer-scrolled; it is not the
expensive part. Recommended next changes, not implemented by this profiling
work: host-generate the mirrored arrow and use Bitmap for both directions;
cache settled headings/description; reduce full-frame background restoration
and redraw to the changing regions or a hardware-scrolled border layer.
Re-measure navigation with FPSCOUNT after each change. The idle 50 FPS
sample does not establish sufficient headroom for those moving states.

## Style optimizations and hardware scrolling

The same 180-frame left/right test after the optimizations reports
**180 PAL fields, 50.00 FPS, maximum gap one field** (previously 281 fields,
32.03 FPS). This result includes entry and navigation, with the ordinary
A500/512 KB Chip/8 MB Fast profiling configuration; it is not a guarantee
for unrelated scenes or all hardware configurations.

Changes: offline-mirrored left arrow; offline-trimmed bobs with unchanged
anchors; prepared Bitmap descriptors; offline static text panels; per-buffer
dirty rectangles and actor-state caching. Most importantly, PF2 now scrolls
the immutable border by DMA pointers, while PF1 retains the static content.
The temporary fifth plane supplies an opaque-black high bit to front-priority
PF1 during transitions, so both playfields are hidden without repainting.
Only four planes remain enabled once the reveal clears.

The diagnostic full-redraw oracle checked 105 consecutive frames with zero
mismatches. The original and trimmed asset pixels/anchors, mirrored arrow,
captured headings, and fifth-plane mask palette/priority were also checked.
All emulator runs were limited to 45 seconds and cleaned up. The playable
`SKIP_INTRO=1` build was restored after profiling.
