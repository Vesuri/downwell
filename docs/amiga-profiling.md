# Amiga performance instrumentation

## Current checkpoint (2026-09-25)

### Rejected indexed Laser Sight breakable lookup

An experiment reused the existing breakable-cell index to test at most four
cells for each eight-pixel ray segment, keeping the linear fallback for
unaligned/duplicate layouts. It passed 3,213 paired endpoint comparisons over
fractional origins, both index boundaries, +/-30-degree aims and signed
horizontal speeds, and the live upgraded combat replay retained its prior
state. Nevertheless the same upgraded FPSCOUNT tape measured **1,968 fields /
600 updates = 15.24 FPS**, versus **1,896 / 15.82 FPS** before the change;
maximum gap remained seven fields. No speedup was demonstrated, so the
production change was removed. The availability/layout-invariance test remains.
This result does not isolate the reason for the regression; fewer candidate
tests alone did not justify retaining it.

Logs: `/tmp/downwell-laser-index-{build,test,replay,fps}.log` and
`/tmp/downwell-laser-index-restored-{test,build}.log`. The ordinary build is
restored. Ray overlay writing/restoration and the extra planar-actor path
remain higher-priority performance targets than this rejected lookup change.

### Laser Sight cost (not a 20 ms success)

`PROFILE_LASER=1 SKIP_TO_GAME=1 VERIFY_TIMEOUT_SECONDS=45 bash amiga/profile.sh fps`
grants the real upgrade on entering gameplay in the profiling autorun. It does
not enable scene verification; `LASER_VERIFY` remains prohibited with probes.
The upgrade changes firing behavior too, so this is an upgraded gameplay tape,
not a rendering-only A/B experiment against the unupgraded tape. The script
prints the fixture setting and executable hash and now explicitly restores
non-skip, non-fixture settings even when those flags were inherited from the
environment.

The first 600-update upgraded run took **2,731 PAL fields / 10.98 FPS**, maximum
gap nine fields. The separate instrumented run attributed 7.115 ms/update to
endpoint tracing (phase 45), 27.762 ms to retention/restoration (46), and
24.600 ms to ray drawing plus its preceding blitter drain (47). Nested terrain
queries are charged to phase 14, not 45; phase 47 is not pure CPU raster time.
These probe timings are diagnostic, not shipping frame times.

Replacing per-pixel modulo with bounded subtraction in the two overlay row
paths reduced the upgraded run to **2,057 fields / 14.58 FPS**, maximum gap
eight. All 320 ring origins times 284 visible rows are exhaustively checked
against modulo (90,880 cases); existing sparse restoration and sprite tests
pass. This improvement does not resolve the frame-budget failure.

Checking retention eligibility before scanning dirty rows/overlapping actors
further reduces the run to **1,896 fields / 15.82 FPS**, maximum gap seven.
Moved rays or changed ring origins must be restored regardless of overlap,
so inspecting that overlap first was wasted work. Unchanged-ray retention is
still checked against pending terrain damage and actor bounds. Logs:
`/tmp/downwell-laser-retain-{test,fps}.log`.

The optimized native capture still matches SDL with **0/99,968 differing
pixels**; renderer ASan/UBSan passes. The capture workflow restores the normal
build and its 68000 instruction/helper audits pass. Evidence:
`/tmp/downwell-laser-optimized-{frame,sanitized}.log`.

Logs: `/tmp/downwell-laser-{fps,probes}.log` and
`/tmp/downwell-laser-row-{test,fps}.log`. The ordinary build retains no upgrade
injection; phases 45–47 compile out without probes.

### Mutually exclusive builder symbol dispatch

Enemy-symbol selection and terrain-symbol selection now use separate
switches. The unconditional per-cell WELL512 draw, decoration Create order,
conditional random choices, chunk/crate alternation, special-object counts
and synchronous callbacks retain their original positions. This is another
shared-core dispatch change, not a changed generation algorithm.

A new host regression digest covers every emitted Create field (including
float bit patterns and all twelve alarms), command order, RNG draw count,
state index and all sixteen WELL words across 64 seeded level traversals.
The pre-change implementation from `c1f968f` and the new code independently
produce **ffcdfc23dc638be7**, now pinned by the test. This is a refactoring
oracle, not an independent Windows gameplay capture.

Worst instrumented frame 258 falls from **145.317 to 141.875 ms**; builder
phase 32 falls from **72.897 to 69.172 ms**. Registration/index phases remain
22.379 / 25.577 ms. FPSCOUNT measures **811 fields / 600 updates = 36.99 FPS**,
versus 815 before, with maximum gap still five. The gain is modest and the
generation stall remains far outside 20 ms; probe times include overhead.

Builder/live terrain tests, the pinned digest under sanitizers, native replay
and both 68000 audits pass. The ordinary non-skip build was restored. Logs:
`/tmp/downwell-symbol-dispatch-{test,profile,fps,replay}.log`,
`/tmp/downwell-symbol-{before,golden,sanitized,final-test}.log`. Native runs
used the same 45-second bound.

### Mutually exclusive builder Create dispatch

`AreaOneBuilder::emitObject` now dispatches its distinct object IDs with a
switch instead of a sequence of independent equality checks. All initializers,
RNG calls, default command construction and synchronous observer ordering are
unchanged. Static terrain takes the default path without testing every
special Create handler. This is shared core code, not Amiga-only behavior.

Against the preceding resolved-sprite build, worst instrumented frame 258
falls from **149.695 to 145.317 ms**; exclusive builder phase 32 falls from
**77.458 to 72.897 ms** (about 5.9%). Registration/index phases measure
22.537 / 25.373 ms. This reduces the sampled generation stall but does not
remove it. These values include probes and are not shipping frame times.

The separate FPSCOUNT tape takes **815 fields / 600 updates = 36.81 FPS**,
maximum gap five, versus 813 immediately before. There is no average-FPS
improvement claim; the measured benefit is the smaller generation spike.
Builder fixtures, live incremental terrain tests, builder sanitizers and
bounded cross-target replay pass; replay digests are unchanged. Both native
audits pass, and the ordinary non-skip executable is restored. Logs use
`/tmp/downwell-builder-dispatch-` (`test`, `sanitized`, `fps`, `profile`,
`replay`, `restored`). All emulator runs have a 45-second bound.

### Actor presentation breakdown

#### Resolve the selected hardware part once

Channel selection now obtains a short-lived `AmigaResolvedSpritePart`, using
its part count to allocate channels and passing its directory record directly
to the drawing path. Part zero no longer performs a second lookup after
allocation; additional parts resolve individually. The descriptor is local
to that actor's allocation/draw sequence, so no new persistent cache or bank
invalidation policy is needed. The existing lookup API and resolved API share
the same drawing implementation; source-pixel conversion remains host-only.

All 22,834 offline sprite/edge comparisons also exercise the descriptor path
and verify its channel requirements, including flips. Missing offline data
fails without touching the destination. Renderer sanitizer tests, native
firing capture (0/99,968 differing pixels), forced-BOB oracle and native audits
pass. The ordinary non-skip executable is restored.

FPSCOUNT measured **813 PAL fields / 600 updates = 36.90 FPS**, maximum gap
five, within the existing 812–813 baseline. Instrumented actor phases 42/43/44
total 744.701 / 545.650 / 481.216 ms (previously 674.022 / 508.925 / 600.478).
The update phase is smaller but collection/selection increased; the combined
cost is effectively unchanged. This is a verified removal of a repeated
lookup, **not a demonstrated frame-rate gain**. Header-only/copy counters
remain exactly 523 / 454, with 15,486 payload words. Logs use
`/tmp/downwell-resolved-` (`test`, `sanitized`, `fps`, `frame`, `profile`,
`bobs`, `restored`); all emulator runs have a 45-second bound.

#### Rejected immutable-directory cache shortcuts

An explicit immutable-bank lifetime API allowed linked gameplay artwork to
reuse validated directory records without checking its header and directory
bounds on every cache hit. Mutable scene banks kept the existing validation;
the setter invalidated records even for same-address bank replacement.
The first variant reused the existing 64-slot cache; a second added an
independent 64-pointer cache so mutable lookups could not evict its entries.

Both passed renderer oracle/lifetime tests and bounded native firing captures
(0/99,968 differing pixels). The first also passed the sanitized renderer.
The 600-update FPSCOUNT tape measured **814 fields / 36.86 FPS** for the
shared cache and **821 / 36.54 FPS** for the separate cache, both maximum gap
five. Neither demonstrated improvement over the repeated 812–813 baseline.
Both implementations and their lifetime API were removed; the ordinary build
was restored. Logs use `/tmp/downwell-immutable-` and
`/tmp/downwell-immutable-separated-` (`test`, `fps`, `frame`; `sanitized` for
the first variant). This rejects extra lookup-cache paths, not the untested
larger change of passing one resolved actor descriptor through all stages.

Additional PROBES-only phases separate gameplay actor collection (42),
hardware-channel selection (43), and each hardware-sprite update (44).
Counters 8–10 distinguish header-only updates from copying already-offline
sprite payloads and count copied 16-bit image words (excluding control words
and terminators). All new probes/counters compile out of shipping/FPSCOUNT.

The bounded 600-update `-O3` gameplay tape reports:

| Phase | Calls | Total | Mean per game update |
| --- | ---: | ---: | ---: |
| Actor collection | 600 | 674.022 ms | 1.123 ms |
| Channel selection, excluding nested updates | 1577 | 508.925 ms | 0.848 ms |
| Hardware-sprite update | 977 | 600.478 ms | 1.001 ms |

523 updates changed only the position header; 454 copied payloads totaling
15,486 words (30,972 bytes). All 977 update calls are accounted for by those
two counters. These counts do not imply runtime pixel-to-planar conversion:
the copied data is host-prepared DMA artwork. The whole update phase includes
lookup, validation, positioning, cache checks and copying, so it does not
isolate the copy loop's time. Collection plus selection is a larger measured
target than the update phase; next investigate resolving reusable actor
metadata once rather than repeating directory work across those stages.

Evidence: `/tmp/downwell-actor-profile.log`. The added brackets introduce
overhead (1,006 instrumented fields); do not compare their sum directly with
the older unsplit phase as a runtime regression. The renderer oracle passed
22,834 offline sprite comparisons, the ordinary executable was restored, and
both native audits passed. No shipping performance improvement is claimed.

### Compiler optimization comparison

The current 600-update A500 gameplay tape favors the existing `-O3` setting:

| Optimization | PAL fields | FPS | Maximum gap |
| --- | ---: | ---: | ---: |
| `-O3` (two baseline repeats) | 812–813 | 36.90–36.95 | 5 fields |
| `-O2` | 901 | 33.30 | 6 fields |
| `-Os` | 1158 | 25.91 | 8 fields |

Only the optimization level changed; LTO, 68000/soft-float target flags and
the deterministic autorun tape stayed the same. Both experimental builds
passed the instruction/helper audits and completed the bounded gameplay
profile, but they were not subjected to full replay/pixel acceptance because
neither was retained as the default. Logs: `/tmp/downwell-opt-{o2,os}-fps.log`.
The ordinary non-skip `-O3` executable was restored and audited afterward.

Use `AMIGA_OPTIMIZATION=-O2` or `AMIGA_OPTIMIZATION=-Os` with
`SKIP_TO_GAME=1 VERIFY_TIMEOUT_SECONDS=45 bash amiga/profile.sh fps` to repeat
the comparison. The Makefile accepts only `-O2`, `-O3`, or `-Os`, defaults to
`-O3`, and includes the selected value in the object-rebuild options stamp.
The profiler records the selected optimization and tested executable SHA-256
before launching UAE. This is a reproducibility control, not a performance
or feature-parity improvement.

The historical 50 FPS result below predates subsequent fidelity work and is
not the current performance claim. The retained ammo HUD fixes visible clears
and reduces its measured work substantially, but the current 600-update
firing tape runs at 36.99 FPS. See [the HUD checkpoint](amiga-ammo-hud-performance.md)
for exact scope, phase timings, remaining budget work and pixel verification.

### Exact WELL512 shift on 68000

The target compiler emitted a variable-count 28-bit left shift for each
WELL512 draw. The native implementation now rotates right four bits and
masks with `0xf0000000`; this preserves exactly the low nibble shifted into
the high nibble. The host retains the portable shift expression as its
oracle. Disassembly confirms `ror.l #4` and `andi.l`, both 68000 instructions.
Draw order, state layout and draw counts are unchanged.

The bounded 600-update FPSCOUNT tape takes **812 PAL fields / 36.95 FPS**,
versus 819 fields / 36.63 FPS beforehand; maximum gap remains five fields.
The instrumented worst frame is still 258, at 149.453 ms versus 150.267 ms.
Its builder/registration/index phases take 77.054 / 22.539 / 25.624 ms.
This is a small measured improvement, not a solution to the generation stall
or the 20 ms frame budget.

Portable RNG and incremental generation tests, bounded cross-target replay,
and both native instruction/helper audits pass. Logs:
`/tmp/downwell-rng-shift-{test,profile,fps,replay}.log`. Replay verification
restored the ordinary non-skip executable. Runs used `SKIP_TO_GAME=1` where
applicable and a 45-second emulator limit.

### Separate runtime costs (2026-09-25)

#### Rejected direct DIVU range-reduction variants

Two exact replacements for the non-power-of-two `irandomSmall` modulus
reduced the positive 63-bit value four words at a time. Every partial
dividend was below divisor*65536, so each 68000 DIVU quotient fit a word;
the existing mask path handled divisor 65536. Both original RNG draws
remained in place. One variant extracted each remainder; the other kept it
in the high word between DIVU instructions and replaced only the quotient
word with the next input word.

Host tests covering all bounds 0..65535 and native cross-target replays
passed; both native audits passed. FPSCOUNT measured **818 fields / 36.67
FPS** and **816 / 36.76 FPS** respectively for 600 updates (maximum gap five),
versus the repeated unchanged baseline of 812–813 fields. The first variant's
worst instrumented frame measured 149.074 ms, builder phase 76.815 ms versus
the earlier 77.054 ms. This small phase reduction did not establish an
overall improvement. Both implementations were removed.

Evidence prefixes: `/tmp/downwell-divu-` (`test`, `fps`, `replay`, `profile`)
and `/tmp/downwell-divu-packed-` (`test`, `fps`, `replay`). Emulator runs were
bounded to 45 seconds and the ordinary build restored. Do not infer a net
win from avoiding generic remainder helpers without measuring the complete
compiled runtime.

#### Baseline repeatability and rejected landing-entry-width experiment

Two fresh runs of the unchanged implementation measured 812 and 813 PAL
fields for 600 updates (36.95 and 36.90 FPS), both with maximum gap five.
These runs used the same bounded FPSCOUNT autorun workflow and clean builds;
one field of observed variation means tiny single-run differences should
not be treated as precise CPU-time savings. Logs:
`/tmp/downwell-baseline-repeat-{one,two}.log`.

A 16-bit cached row+1 threshold in the upward landing-index propagation loop
passed live terrain tests, sanitizers and cross-target replay. The slowest
instrumented frame measured 148.575 ms; its index phase measured 24.769 ms
(previously 149.453 / 25.624 ms). However the separate FPSCOUNT run took
816 fields / 36.76 FPS, with maximum gap five. This did not improve the
whole-frame target, so the implementation was removed. Do not describe the
smaller exclusive phase sample as an overall speedup. Logs use
`/tmp/downwell-index-width-{test,sanitized,profile,fps,replay}.log`.

The retained regression test inserts 128 out-of-order one-way platforms into
four columns, repeats each insertion, and verifies all 512 nearest-landing
entries in all four columns after every insertion. The ordinary non-skip
build was restored after the baseline repeats; both 68000 audits passed.

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

### Batch the player's four mask corners

Shared `PlayerMotion` accepts an optional pure four-corner mask resolver;
without one, it invokes the same four point callbacks in the same order as
before. Horizontal substeps and upward collision probes use that common
helper, without changing coordinates, quantization or motion order.

The native terrain callback visits distinct indexed cells occupied by those
corners. It does not replace the corners with full rectangle overlap. Without
an index it uses the old four-point query. The runtime wrapper includes the
same four shop-solid points, preserves the callback when rebound after copies,
and clears it for next-level reconstruction. Both native entry paths install
the callback. No extra bitmap or terrain cache is allocated.

19,683 positions compare the batched result against point queries, covering
every cell alignment and active/inactive breakables. The existing 600-tick
live-generated side-by-side test now compares both player and bullet batching
with the point-query runtime. Checkpoints and RNG state remain identical.
These checks also pass ASan/UBSan, and player/runtime fixtures and bounded
native cross-target replay retain their digests.

The lightweight tape takes **819 PAL fields / 600 updates = 36.63 FPS**,
versus 824 / 36.41 FPS before. Maximum gap remains five fields; the overall
frame budget remains open. Evidence uses `/tmp/downwell-player-mask-` logs
(`test`, `sanitized`, `fps`, `replay`, `frame`, `suite`). The complete host
suite passes, native shot pixels match SDL (0/99,968 differences), and both
68000 audits pass. The ordinary non-skip executable is restored. Emulator
runs are bounded to 45 seconds.

### Attribute the slowest complete frame

Probe builds now retain exclusive phase totals for the slowest complete
frame, not just the largest individual bracket. The report verifies that
these integer tick totals sum exactly to the frame duration. Initial gameplay
setup is excluded as before. Frame accumulators are reset before the starting
timestamp, and snapshot copies occur after the ending timestamp. All added
state/work is compiled out without `PROBES=1`.

The 600-update tape's slowest frame is **258**, measuring **150.267 ms** with
instrumentation. Its largest components are:

| Component | ms in frame 258 |
| --- | ---: |
| Builder work excluding nested registration/indexing | 77.885 |
| Generated-object registration | 22.836 |
| Terrain index/tile insertion | 25.318 |
| Vblank waiting | 11.159 |
| Player movement excluding terrain queries | 2.045 |

Generation's three exclusive components total **126.039 ms**. No ammo HUD
phase ran in this frame. This identifies the long stall separately from the
recurring HUD costs: fixing ordinary frames alone cannot eliminate it.
Preserve original Create ordering and RNG consumption when optimizing or
preparing generation work; deferring live effects changes behavior.

The native accounting check and both 68000 audits pass. The 45-second bounded
run cleaned up and restored the ordinary executable. Logs:
`/tmp/downwell-worst-frame-{profile,restore}.log`. The added probes introduce
overhead, so 150.267 ms is not an uninstrumented frame-time claim. The latest
FPSCOUNT baseline remains 36.63 FPS; this diagnostic makes no speedup claim.

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
