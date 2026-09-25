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
finite ends and all area transitions. Native verification is recorded with
the scene extraction below once completed. Overlay pointer scrolling and
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
