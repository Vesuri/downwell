# Level-ending profile, 2026-09-25

Measured from `SKIP_LEVEL=1`, using FS-UAE's A500 configuration (68000,
512 KiB Chip, 8 MiB Fast), PAL, `-O3`. These are emulated beam/VBlank times,
not host wall time or measurements on physical hardware. Warp changes how
quickly the experiment runs, not the PAL-field denominator.

Reproduce with:

```sh
SKIP_LEVEL=1 VERIFY_TIMEOUT_SECONDS=60 bash amiga/profile.sh probes
SKIP_LEVEL=1 VERIFY_TIMEOUT_SECONDS=60 bash amiga/profile.sh fps
```

Each run stops at 480 measured updates, with a 60-second wall-clock deadline.
The initial gameplay setup frame is excluded by the existing profiler; clear
screen entry and subsequent next-level setup are included. The tape waits for
the normal exit, idles in selection, presses right every 16 updates during
clear updates 160–239, then holds fire from update 240 to depart. The ordinary
startup build is restored, including `SKIP_LEVEL=0`.

The detailed probes and frame-count-only build are separate experiments.
Never describe probe-build throughput as shipping FPS. The section label
"idle" means no input: headings, icons and camera easing still animate.

## Measurements

The tables immediately below are the original baseline. See **Retained
upgrade layers** at the end for the subsequently integrated hardware-scrolled
content path and its remaining costs.

Frame-count-only results (first complete 480-update capture):

| Section | Updates | PAL fields | FPS / stall |
|---|---:|---:|---:|
| Exit field and wipe | 96 | 901 | 5.33 FPS |
| Clear entry | 1 | 27 | 540 ms |
| Reveal | 60 | 179 | 16.76 FPS |
| Selection without input | 99 | 501 | 9.88 FPS |
| Selection with right input | 80 | 349 | 11.46 FPS |
| Confirm/departure | 122 | 365 | 16.71 FPS |
| Next-level setup and first updates | 22 | 139 | worst gap 118 fields / 2.36 s |

Detailed probe-build average milliseconds per idle-selection update:

| Work (exclusive phases) | ms |
|---|---:|
| Border validation/signature scan | 27.33 |
| Border pixel drawing | 0.00 |
| Overlay validation/damage/cache bookkeeping | 16.68 |
| Overlay clear and redraw | 31.12 |
| Draw-list/layout work | 15.74 |
| Trigonometric sampling | 2.60 |
| Shared clear simulation | 6.17 |
| Hardware sprite handling | 0.91 |
| Copper patches | 0.53 |
| Final blitter drain | 0.14 |
| Publication | 0.15 |

There are also loop/input costs and VBlank waiting. Nested phases are excluded
from their parents: do not add the earlier unsplit border/overlay totals to
these values. Blitter helpers may wait inside drawing, so the final drain is
not total blitter waiting. No clock clamps occurred in the detailed capture.

Before the clear screen, exit-field preparation averages **130.33 ms** and
its final word drawing **13.27 ms**. Geometry work also remains in the older
sprite phase; the whole pre-clear cost is not assigned solely to these two
brackets. Clear entry's retirement/asset preparation is about **300 ms**;
first-buffer border painting about **147 ms**. Introducing offers causes a
one-time bank replacement of about **125 ms**. These are instrumented costs,
not isolated benchmarks or promised savings.

## Current implementation

### First follow-up: repeating-border scan

The retained circular border now checks only newly exposed rows when both
frames lie strictly inside the same repeating area's strip. Repainting the
same snapshot examines zero rows. Finite ends, changing areas and transition
seams retain the general signature path. This adds no buffer or allocation.

The same detailed A500 tape measures idle border work at **2.64 ms**, versus
**27.33 ms** above; navigation is **2.36 ms**, versus **27.33 ms**. Border
painting remains zero during those sections. These are probe-build costs,
not a new shipping-FPS claim. The baseline table above is intentionally kept.
Log: `/tmp/downwell-clear-strip-profile.log`, detailed output in
`amiga/.run/profile-probes.log` (replaced by subsequent profile runs).
The expanded 3,840-frame host oracle includes reverse/large camera jumps,
finite ends and all area transitions. Native verification is recorded in
`scene-architecture.md`: two clears and third-level reentry pass. Overlay pointer scrolling and
shared style/upgrade presentation are still unfinished.

- Source-derived clear state, camera/player easing and carousel logic run in
  the shared core. `HudReal` preserves binary64-style arithmetic with integer
  operations; coordinates are then narrowed using the shared raster policy.
- PF1 contains borders in a double-buffered 320-row circular buffer. Copper
  pointers/wrap handle scrolling. However, each buffer's cache examines all
  284 visible rows every update to rediscover which signatures changed. In
  the idle/navigation samples no border paint is needed, but this scan alone
  exceeds the frame budget.
- PF2 contains titles, upgrade icons and descriptions. Each back buffer keeps
  the preceding draw commands and bounds. Changed commands contribute old/new
  rectangles; overlaps are merged, those rectangles are cleared, and every
  intersecting command is drawn again in source order. Unchanged overlapping
  art can therefore be redrawn too.
- Each heading is seven colored/shadow passes; each description is five
  outline/foreground passes. Shadow offsets, icon bobbing and carousel motion
  keep generating damage even without user input. The renderer uses CPU word
  drawing for small eligible assets and masked blits for larger ones.
- Artwork is already planar. Entry/offer changes select records and copy
  payloads into a Chip bank; this is not chunky-to-planar conversion. Current
  bank construction scans directories and copies bytes at runtime.
- The player uses offline hardware sprite artwork. The clear path still calls
  the build/copy wrapper each update rather than retaining a resolved asset;
  its measured cost is small compared with border/overlay work.
- The field preceding completion rebuilds sparse coverage from three
  24-segment outlines when geometry changes, restores departed pixels, and
  writes changed words. Avoiding unchanged framebuffer writes has not made
  its per-pixel coverage construction cheap.

## Proposed order — not implemented by this profiling change

1. Eliminate whole-viewport border scans in the common repeating case. Advance
   the ring pointer; handle exposed rows and actual area seams explicitly.
   Consider a prefilled repeating strip where valid. Preserve later-area seam
   behavior rather than optimizing only level 1.
2. Cache/composite complete heading shadows and outlined descriptions. Retain
   static regions; restore only actual moving damage. Key caches by rendered
   geometry so the original animation timing is unchanged. Do not replace
   animated source headings with static text to improve a benchmark.
3. Replace repeated layout/trig work with proven equivalent retained samples
   or tables where possible. Keep source-backed state and raster comparisons;
   do not silently change easing or rounding to cheaper approximate rules.
4. Give the exit-field outlines a word/span-oriented or precomputed geometry
   path, preserving source RNG consumption even when raster output is cached.
5. Prepare banks/initial border strips outside visible critical transitions;
   use resolved directories and aligned bulk copies. Measure Chip/Fast memory
   tradeoffs before reserving more buffers.

Do not begin with Copper tricks: current Copper updates are roughly half a
millisecond, while software cache/layout work costs tens of milliseconds.
Hardware overlay scrolling could remove the common camera translation, but
cannot alone represent independent heading shadows, bobbing and carousel
movement. Decide the retained-art strategy before changing playfield roles.

Regression checks after adding timing brackets: 360 alternating overlay
frames and 2,880 circular-border frames match fresh rendering. Native builds
retain the 68000 instruction/helper audits. This profile is not a Windows
presentation-equivalence proof or evidence that the broader parity goal is done.

## Retained upgrade layers

The live upgrade scene now retains finite world-space border and content
canvases. Camera/tile translation patches Copper pointers, with independent
icon/shadow animation using damage rectangles. See `scene-architecture.md`
for geometry, memory ownership and native pixel verification. The older
320-row ring and viewport-overlay description above is the baseline path,
now retained only as the clear-scene verification oracle.

The same bounded 480-update detailed profile reports these idle averages:

| Work | ms |
|---|---:|
| Overlay validation, damage and painting combined | 24.43 |
| Draw-list/layout | 15.78 |
| Trig sampling | 2.57 |
| Shared clear simulation | 6.62 |
| Border render/cache | 4.96 |
| Sprites | 0.82 |
| Copper patches | 0.54 |

The overlay total replaces the baseline's 16.68 + 31.12 ms phases; this new
renderer does not split bookkeeping from painting yet. Border timing includes
bank invalidation spikes up to 165 ms; steady navigation averages 3.34 ms.
Border command construction remains in the residual phase, so the cache
figure is not its complete CPU cost. Instrumentation reported zero clock clamps.

This is not a 20 ms result. Remaining work includes avoiding identical border
repainting after offer-bank replacements, reducing animated heading/outline
composition, and proving cheaper equivalents for repeated source-derived
layout calculations. Do not remove original animation to satisfy the budget.
Detailed log: `/tmp/downwell-retained-probes.log`.

The separate frame-count-only run (`/tmp/downwell-retained-fps.log`) measured
20.98 FPS reveal, 14.73 FPS idle selection, 13.70 FPS navigation and 22.93 FPS
departure. Clear entry took 14 fields (280 ms). Next-level setup still reached
a 119-field gap (2.38 s); that path was not optimized here. Both profile runs
completed within their 60-second hard limits and restored ordinary startup.

### Equivalent border-bank retention

Keeping the existing border canvases across offer/inventory-bank replacement
removes their rebuild spikes. In a subsequent complete detailed 480-update
capture, idle border work averaged 3.33 ms and peaked at 3.56 ms (previously
4.96 / 165.30 ms). Departure border work averaged 3.38 ms and peaked at
3.66 ms (previously 5.99 / 162.49 ms). Initial scene painting is still needed.
These are border-phase times, not complete-frame times or shipping FPS.

Idle overlay work remains 24.43 ms and layout 15.78 ms. Reconstructing and
comparing unchanged border commands is also still work to remove. Thus this
change eliminates intermittent redundant painting, not the entire frame-budget
problem. The native 432-frame pixel comparison and host/sanitizer replacement
tests pass; see `scene-architecture.md` for the equivalence constraint.

The first 60-second capture reached its report at the deadline but did not
detach successfully. A repeat completed cleanly with a 120-second hard limit,
zero clock clamps and normal-build restoration. Complete log:
`/tmp/downwell-border-rebind-profile-v2.log`.

### Cached border geometry

The following bounded 480-update detailed capture adds the per-display-slot
area/seam cache. Idle border work falls from 3.33 to 0.19 ms (maximum 0.25 ms).
The surrounding residual phase falls from 5.30 to 1.76 ms because it no longer
constructs 76 border commands per frame. Together this removes approximately
6.7 ms of work per selection update. Navigation border work is also 0.19 ms.

Idle content rendering remains 24.47 ms, layout 15.77 ms, simulation 6.62 ms
and trig 2.60 ms. This is still not a 20 ms frame. Detailed-probe throughput
must not be presented as shipping FPS. The run completed within its 120-second
hard deadline with zero clock clamps and normal-build restoration. Log:
`/tmp/downwell-border-geometry-profile.log`.

### Offline fixed text outlines

The live path now draws each upgrade name/description as one host-composed
masked image, retaining frame-0 masks and the five-pass presenter as an oracle.
All 71 variants and four clipping positions match; 360 animated host frames
and 432 native completion frames also match. Red heading animation is unchanged.
See `scene-architecture.md` for the bytecode basis and memory bound.

The final 480-update detailed capture measures idle content rendering at
21.37 ms (previously 24.47), and navigation at 21.68 ms (previously 25.34).
Content-phase maxima fall from 61.77 / 70.75 ms to 42.88 / 46.24 ms.
Layout still takes 15.44 / 17.39 ms. This does not satisfy the 20 ms budget.

Keeping both oracle masks and outlined variants in the selected bank increases
one-time preparation work. The first implementation peaked at 278.61 ms;
validating the source directory once and merging requested keys reduces it to
167.98 ms. This remains higher than the approximately 101 ms pre-outline bank
preparation, so bank staging/copying is still unfinished optimization work.
No additional Chip allocation is used; every possible selected bank fits the
existing reservation. Do not confuse the preparation tradeoff with steady
frame cost or present detailed-probe throughput as shipping FPS.

The final native regression passes two clears, 192 exit-field frames, 60
third-level updates and the stack guard. The final profile completes within
its 120-second hard limit, reports zero clock clamps, and restores normal
startup. Logs: `/tmp/downwell-outlined-merge-native-gdb.log` and
`/tmp/downwell-outlined-merge-profile.log`.

### Native bank bulk copy

Replacing the bank's indexed byte-copy loops with standard `memcpy` selects
the existing runtime's aligned 68000 longword loop. The same detailed profile
reduces peak offer-bank preparation from 167.98 to 45.30 ms. This is a
transition-cost improvement, not a 123 ms per-frame saving. Initial border
painting and the animated content/layout costs are unchanged and still need
work; the 20 ms frame target is not achieved.

All alignment/guard/source-preservation tests pass, including ASan/UBSan.
The bounded native comparison again passes 432 completion frames and both
reentries. The 480-update profile completed within its 120-second hard limit,
reported zero clock clamps, and restored normal startup. Evidence:
`/tmp/downwell-bank-copy-native-gdb.log` and
`/tmp/downwell-bank-copy-profile.log`.

### Exact heading coordinates

The heading specialization removes repeated general coordinate arithmetic,
while retaining the existing Q30 sampler and binary32 rounding policy. See
`scene-architecture.md` for its range proof and independent boundary tests.
The same 480-update detailed profile now measures:

| Phase | Idle average | Navigation average |
| --- | ---: | ---: |
| Layout (excluding nested trig) | 10.28 ms | 13.23 ms |
| Heading/trig preparation | 1.67 ms | 1.67 ms |
| Content rendering | 21.35 ms | 21.65 ms |
| Shared completion step | 6.62 ms | 9.39 ms |

Layout was previously 15.44 / 17.39 ms, with roughly 2.60 ms in trig.
This saves approximately 6 ms during idle updates without changing command
pixels. Content rendering remains the largest steady cost, and the complete
frame still exceeds 20 ms. Detailed-probe throughput is not shipping FPS.
Initial border painting remains about 164 ms, also unfinished work.

The native pixel/stack regression and sanitizer checks pass. The profile
finishes within its 120-second deadline, reports zero clock clamps, stops the
owned emulator and restores all startup skips to zero. Evidence:
`/tmp/downwell-heading-native-gdb.log`, `/tmp/downwell-heading-profile.log`,
`/tmp/downwell-heading-final-host.log`, and
`/tmp/downwell-heading-sanitized.log`.

### Immediately overwritten heading passes

The retained selection renderer now omits a masked write when the next
command completely replaces the same resource/frame at the same position.
It retains the original commands for validation and damage calculation.
This reduces average content rendering from 21.35 to 17.79 ms at idle and
from 21.65 to 19.36 ms during navigation. Layout and simulation are unchanged;
the complete frame still exceeds 20 ms. Content maxima are 42.93 / 43.76 ms,
so the improvement is not a worst-case frame-time guarantee.

The same 480-update profile finishes within its 120-second limit with zero
clock clamps; normal startup is restored. Native two-clear pixel comparisons,
the stack guard, 1,800 host heading/pen/clipping cases and ASan/UBSan pass.
Logs: `/tmp/downwell-overwritten-profile.log`,
`/tmp/downwell-overwritten-native-gdb.log`, and
`/tmp/downwell-overwritten-sanitized.log`.
