# Retained ammo HUD: correctness and performance checkpoint

The HUD is not cleared in visible Chip RAM between drawing operations.
`AmigaAmmoHud` retains an off-screen planar image in Fast RAM, recomposes
damaged strips there, and publishes only differing words in each plane.
An unchanged word is not written, even when another word on the same row
changes. This prevents a stationary red rim disappearing during a redraw.

The existing HUD palette remains 01=white, 10=red, 11=opaque black. Merely
toggling the first plane cannot implement every white/black transition.
The two planes are compared/published independently instead. The original
recharge jiggle can genuinely change rim pixels; those changes are preserved.

## Work retained or avoided

### Rejected high-plane-only background experiment (2026-09-25)

A guarded interior path verified that every row/divider overlay retained
the low-plane bits, then wrote only the high plane when restoring black or
white background spans. This removed redundant low-plane background writes,
but still required the subsequent row/divider composition passes.

The retained-HUD oracle, 22,834 sprite comparisons and sanitized renderer
passed. A bounded native firing capture matched SDL (0/99,968 differing
pixels), and the native audits passed. Nevertheless the same FPSCOUNT tape
took 816 PAL fields / 600 updates (36.76 FPS), versus 812 fields (36.95 FPS)
beforehand, with the same five-field maximum gap. There is no demonstrated
net performance improvement, so the code and helper were removed. This does
not disprove single-plane updates; it rejects this particular extra dispatch
path. The next composition experiment should eliminate overlapping passes,
not just one store within them.

Evidence: `/tmp/downwell-hud-high-{test,sanitized,fps,frame}.log`. Native
runs used a 45-second timeout and started directly in gameplay.

### Retained implementation

Another rejected experiment detected homogeneous opaque eight-row overlaps
inside a damaged interior strip. When all contributing stamps had the same
family/frame and their combined mask covered all 12 pixels, it copied their
cached final words directly, skipping both background and row draws. Mixed
strips retained the original path. The scan excluded the special bottom
stamp and checked all contributing rows and the original y<232 restriction.

This was pixel-correct (complete renderer oracle, sanitizers, native firing
capture with 0/99,968 differing pixels; native audits clean), but the bounded
600-update FPSCOUNT tape took **824 fields / 36.41 FPS**, maximum gap five,
versus the 812-field baseline. The implementation was removed. The additional
homogeneity scan is a suspected cost, not independently measured by this
whole-frame comparison. Do not repeat per-draw uniformity detection without
measuring that overhead or reusing already-computed run information.
Logs: `/tmp/downwell-hud-opaque-{test,sanitized,fps,frame}.log`.

A third experiment retained the three published gauge columns in 3,408 bytes
of Fast RAM, comparing new words with that cache rather than reading Chip
RAM. Scene invalidation refreshed the cache, wide-label fallback refreshed
it after publication, and each changed plane still wrote independently.
Direct tests covered all column spans and changed-plane combinations,
including zero writes on repeated publication. The full renderer oracle,
sanitizers and native firing captures passed (0/99,968 differing pixels).

The generic paired-plane loop measured **820 fields / 36.59 FPS**; a
specialized one-column pointer-walking loop measured **825 / 36.36 FPS**.
Both used the same 600-update, 45-second-bounded tape and retained a five-field
maximum gap. Neither improved the 812-field / 36.95 FPS baseline, so both
were removed, including the extra cache allocation. These whole-frame
measurements do not isolate the cost of Chip-RAM reads or prove a general
claim about caching. Evidence prefixes: `/tmp/downwell-hud-published-` and
`/tmp/downwell-hud-published-narrow-` (`test`, `sanitized`, `fps`, `frame`).

- Original binary64 divider-loop geometry is cached by capacity for the
  current one-charge weapon, including all four recharge offsets. Visibility
  still depends on ammo. Capacities above 64 and other consumption values
  retain the general source-equivalent path; they are not truncated.
- Fill geometry is recomputed only when ammo/capacity changes.
- Row damage compares displayed subimages, so state 3 to state 4 (both frame
  3) does not dirty the gauge. Unchanged strips and frame edges are retained.
- Consecutive identical row stamps are combined in original draw order.
  Eight overlapping source rows become one native word per destination row.
  These are copies/compositions of already offline-planar assets, not a
  hardware-sprite pixel conversion.
- Static underlay/rim columns are cached. Their black/transparent interior
  is discovered from the actual artwork, allowing ordinary shots to avoid
  processing the stationary red sides altogether.
- Small masked sprites and rectangles have portable word-oriented CPU paths.
  Larger on-screen operations still use the existing framework blitter.
- Cache preparation occurs with the initial level picture, not on the first
  shot. Scene reconstruction invalidates the retained HUD. Other HUD regions
  do not overlap the retained gauge/number bounds.

The native `ammoHud` object occupies 41,110 bytes of ordinary BSS/Fast RAM in
the reviewed build, including a 27,264-byte working planar image. It adds no
Chip-RAM buffers and does not change copper programs or hardware scrolling.

## Verification

- Cached divider command sequences match the original binary64 loops across
  capacities 1..64, every ammo count, all four offsets, and fallback cases.
- Native-width integer construction/truncation is checked against binary64,
  including 100,000 32-bit integer samples.
- Shifted/clipped small CPU sprites and rectangle clears match the established
  byte-span renderer for all 16 horizontal bit offsets.
- Retained HUD pixels match the **pre-optimization full draw sequence** over
  firing/recharge sequences at capacities 1, 8, 15, 64 and 127, including all
  their ammo counts and three-digit labels. Repeating an unchanged state
  requires zero visible-word writes.
- Bounded native firing and recharge captures both match SDL with **0 of
  99,968 differing pixels**. These prove the implemented presentation, not
  an independently captured Windows GPU raster.
- The complete host suite, AddressSanitizer/UndefinedBehaviorSanitizer renderer
  run, bounded cross-target replay and both 68000 audits pass. The weapon
  replay remains `0db388a5b1c2a40a`, coverage `3fff`; rendering changes do not
  change the simulation or RNG stream.

## Performance is still an open gate

Measured with `SKIP_TO_GAME=1`, A500/68000, the same 600-update autorun tape,
45-second host timeout, and phase probes. Initial level composition is
excluded by the existing profiler. All runs clean up their emulator and
restore a normal build.

Before this work, the HUD phase total was 45,056 ms over the tape: **75.09 ms
per game update**, or approximately **324 ms per actual HUD update** (139).
The final retained-renderer phase totals (27..30) are 1,742.886 + 746.342 +
596.085 + 1,566.149 = 4,651.462 ms: **7.75 ms per game update**, or **33.46 ms
per actual HUD update**. Instrumented fields fell from 3,258 to 1,226.

This is roughly a tenfold HUD improvement, **not completion of the 20 ms
frame-budget requirement**. Recharge/full-strip cases, label composition and
publication still have substantial cost. Do not resume feature expansion on
the basis of these averages or describe firing performance as solved.
The separate FPSCOUNT-only run completed 600 updates in 1,040 PAL fields:
**28.85 FPS**, maximum gap six fields. This is the current shipping-style
checkpoint for this tape, not 50 FPS.

Evidence logs for this checkpoint are `/tmp/downwell-hud-final-profile.log`,
`/tmp/downwell-hud-ammo-native.log`, `/tmp/downwell-hud-recharge-native.log`,
and `/tmp/downwell-hud-complete-suite.log`. The phase report now separates
base/fill, row artwork, divider geometry/draws and rim/label/publication.

## Follow-up: less label and divider work

The next increment crops offline-composed ammo digits to their actual ink,
preserving their anchors and resource ID. The HUD pack shrinks by 2,280 bytes
to 12,356 bytes and records `hud-v2` provenance. Label damage uses the union
of old/new ink bounds, except when a previous wide transparent backing needs
restoring. Repeated unchanged requests must not discard that pending repair.

Divider visibility is now classified once per ammo/capacity change, not once
per damaged strip. Exact power-of-two ratios and integral fill rectangles
avoid emulated binary64 arithmetic without changing its results. General
capacities retain the original rounding path. Tests include 100,000 exact
power-of-two ratio comparisons as well as the existing exhaustive geometry
and retained-pixel comparisons.

The same bounded 600-update tape reports HUD phases totaling
1,653.688 + 667.598 + 468.339 + 1,070.559 = 3,860.184 ms:
**6.43 ms per game update**, approximately **27.77 ms per actual HUD update**.
That is another 17% reduction, still not within the 20 ms frame budget.
The instrumented run uses 1,188 fields; the separate FPSCOUNT run uses
989 fields (**30.33 FPS**, maximum gap six fields). No copper/scroll changes
or new Chip-RAM allocation are involved.

Logs: `/tmp/downwell-hud-integer-profile.log` and
`/tmp/downwell-hud-integer-fps.log`. Clearing/compositing changed regions in
the retained working image is still a cost to reduce; unchanged visible
words and stationary red sides are not cleared and redrawn.

The complete host suite, sanitized renderer, both native pixel captures
(zero differences), cross-target replay and 68000 audits pass again. The
normal non-skip executable is restored after verification. The retained HUD
object now occupies 41,178 bytes, 68 bytes more than the previous checkpoint.
Verification logs use the `/tmp/downwell-hud-integer-` prefix with
`suite.log`, `sanitized.log`, `ammo.log`, `recharge.log` and `replay.log`.

## Single-pass interior background

Interior damage no longer clears the entire strip to black before painting
its white fill. It writes final background words in at most three spans:
black, white interior with black side pixels, black. The palette is unchanged
(white `01`, black `11`), so the low plane has the same value in all three
spans. Each row of each plane is written once before animated overlays, and
the red rim remains outside this path. This removes redundant Fast-RAM work
as well as retaining changed-only publication to Chip RAM.

The same 600-update probe tape drops from 1,188 to 1,179 fields. Base/fill
time falls from 1,653.688 to 1,513.069 ms; total HUD phases become 3,719.592 ms
(6.20 ms per game update). This is incremental progress, not a 20 ms pass.
The portable masked-column helper is tested for padding/adjacent-plane
preservation and invalid bounds. The full retained-HUD reconstruction oracle
and sanitized renderer pass. Logs use `/tmp/downwell-hud-fused-`.
The separate FPSCOUNT run takes 980 fields for 600 updates (**30.61 FPS**,
maximum gap six fields), versus 989 fields before this change.
Native firing and recharge captures again have zero differing pixels out of
99,968 against SDL. Both 68000 audits pass, and the capture workflow restores
the ordinary non-skip executable. No shared simulation code changed.

## Rejected experiment: per-frame artwork-difference damage

A 100-byte table of exact changed scanline bounds for each pair of gauge-row
frames reduced row/divider/label drawing slightly, but computing and looking
up the row identities raised base cost. On the same bounded 600-update probe
tape, fields increased from 1,179 to 1,185. HUD phase totals were 1,603.903 +
647.138 + 459.389 + 1,053.790 = 3,764.220 ms, versus 3,719.592 ms beforehand.
The implementation was removed; do not reintroduce it on the assumption that
smaller damage bounds automatically mean lower 68000 cost.

The experiment leaves stronger tests: every state pair 0..4 is isolated at
row indices 0, 1, 91, 92 and 183 against the complete reconstruction oracle,
covering the special bottom sprite and both sides of the fill boundary.
The preceding wide-number backing transition is settled separately so it
cannot confound the row-only comparison. Evidence is in
`/tmp/downwell-hud-rowdamage-profile.log`; the restored implementation passes
`/tmp/downwell-hud-row-transitions.log`. Future work should target compositing
and publication costs, not add another per-row damage lookup.

## Retain constant sides during gauge jiggle

Previously any change to the meter's vertical offset disabled retained sides
for the whole strip. Cache preparation now finds the longest vertically
constant span of the **complete** background and rim columns, with black
interior and transparent rim in the animated-row column. The intersection
of this span at the old and new offsets has identical side pixels; only its
interior needs recomposition. Caps, labels, newly exposed edges and pending
wide-label backing repairs keep the full path. Nothing is inferred from
assumed artwork dimensions, and no simulation/palette/scroll behavior changes.

On the same 600-update probe tape, fields fall from 1,179 to 1,147. HUD phases
are 1,163.464 + 689.702 + 504.856 + 698.458 = 3,056.480 ms, down 17.8%:
5.09 ms per game update, or approximately 21.99 ms per actual HUD update.
This still does not establish a 20 ms game frame. There is no new buffer or
per-row damage table. Tests compare every pair of the four meter offsets at
each ammo count 0..8 against complete reconstruction, alongside the existing
multi-capacity/label and isolated-row tests. Sanitizers pass. Evidence logs
use the `/tmp/downwell-hud-jiggle-` prefix.
The separate FPSCOUNT-only measurement is 600 updates in 940 fields:
**31.91 FPS**, maximum gap six fields, versus 980 fields before this change.
Native shot and recharge captures both remain pixel-identical to SDL
(0/99,968 differences), and both 68000 audits pass. The verification workflow
restores the ordinary non-skip build after the bounded emulator runs.

## Constant middle of repeated row stamps

Once a descending run overlaps all eight source rows, its composed middle is
the same pair of masked words on every scanline. The CPU renderer now writes
that span directly from the existing overlap cache rather than repeating
source-row clamps and three table lookups per line. Only the two varying ends
use the general run compositor. Original source-over order is unchanged; no
new asset conversion, cache allocation, blit or assembly path is introduced.

On the same 600-update probe tape, the row phase falls from 689.702 to
489.309 ms (29%). Total HUD phases are 1,165.461 + 489.309 + 480.537 +
698.932 = 2,834.239 ms, or 4.72 ms per game update. Instrumented fields fall
from 1,147 to 1,137; this still is not a 20 ms frame-budget pass.

Tests compare runs of 0, 1, 7, 8, 9, 17 and 184 stamps for every row-art frame,
at top/middle/bottom positions and horizontal/vertical clipping, against
individual ordered asset draws. The retained-HUD suite and renderer
sanitizers also pass. Logs use `/tmp/downwell-hud-run-`.
FPSCOUNT-only measurement: 600 updates in 928 PAL fields (**32.33 FPS**,
maximum gap six fields), versus 940 fields before this change.
Native firing and recharge captures remain pixel-identical (0/99,968
differences against SDL). Both 68000 audits pass, and the ordinary non-skip
executable is restored. Shared game logic and deterministic state are unchanged.

## Skip settled animation scans

The shared `AmmoGaugeRows` now remembers when a scan found no state changes.
Until the filled-row boundary changes, all rows are already at their targets
and the next call can return false without revisiting 184 bytes. A changed
boundary resumes the original per-row stepping immediately. Initial state,
3-to-3.8 invisible animation steps, change notifications, and copy/reset
behavior are retained; this is not a lower animation update rate. Production
code advances row state only through `step()`; renderers only observe it.

20,000 updates compare frames and change flags with the original binary64
animation over long idle intervals, rapid reversals and capacity changes.
On the 600-update native probe tape, HUD state/frame-flag time falls from
791.261 to 334.860 ms (1.319 to 0.558 ms per update). With the newly separated
runtime probes enabled, fields fall from 1,166 to 1,134. These instrumented
figures are not shipping FPS. Logs use `/tmp/downwell-hud-idle-`.
The separate FPSCOUNT-only tape uses 904 PAL fields for 600 updates:
**33.19 FPS**, maximum gap six fields (previously 928 fields / 32.33 FPS).
The full host suite and bounded native cross-target replay pass, with weapon
digest `0db388a5b1c2a40a` / coverage `3fff` unchanged. Both 68000 audits pass;
the replay workflow restores the normal non-skip executable. This does not
resolve the separately measured generation spike or the 20 ms frame gate.
