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
