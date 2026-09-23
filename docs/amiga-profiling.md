# Amiga performance instrumentation

## Gameplay result: 600 frames without a missed PAL field

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
