# Amiga gameplay frame-cost audit

Where A500 gameplay time goes, measured on the default build at `22df6961`
(2026-10-05). The preceding group-B audit, at `11186ab9`, is given alongside
for matched comparison. Remaining work is ranked in
[open-work §5](open-work.md#5-meet-the-amiga-frame-budget); this document holds
the current measurements and the approaches already ruled out.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO.
- Tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1800 gameplay updates. It covers
  natural descent with one damaging contact, view Y 39 to 1874, at most five
  allocated enemy records and final health 3. It has no shop, no dense
  encounter and no garden-to-well transition.
- FPS-only build `d344a24c…`: 2,980 PAL fields, 30.20 FPS, maximum five
  fields at frame 464. The preceding audit used 3,185 fields, 28.26 FPS
  and a six-field maximum at 488. Whole-field gaps are report-only.
- [Instruction traces](amiga-profiling.md#cycle-exact-instruction-traces):
  build `371dd4d9…` captured 100 fields from each of frames 50, 250, 450,
  644, 690, 843, 900, 1100, 1300, 1500 and 1700. Use the fixed frame ranges
  below, not every frame that fits in each capture. A capture's first frame
  is partial and excluded; 701 and 844 are complete.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=900 \
  TRACE_FRAMES=50,250,450,644,690,843,900,1100,1300,1500,1700 \
  TRACE_OUTPUT=$PWD/tmp/trace/audit bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/audit --frames=1301-1338 --causes --collapsed tmp/quiet.folded
python3 tools/amiga-trace-report.py tmp/trace/audit --frames=701 --causes --collapsed tmp/burst.folded
```

## Frame cost by scene

The gameplay step overlaps the previous frame's publication wait. Most traced
frames contain little idle time, but publication waits can move between quiet
frames as computation gets faster. These totals include that wait; do not
confuse its redistribution with a computation regression. One PAL field is
20 ms (142,102 cycles).

| Tape frames | Scene | Mean ms | Median | Worst | Over 20 ms | Previous mean |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 51–91 | entrance bowl, firing | 23.9 | 24.4 | 34.9 (64) | 28/41 | 26.9 |
| 251–303 | open descent | 20.6 | 18.7 | 44.5 (278) | 19/53 | 20.6 |
| 451–470 | enemies in view | 55.3 | 48.7 | 93.5 (464) | 20/20 | 59.0 |
| 645–661 | damaging contact | 82.1 | 79.7 | 106.3 (660) | 17/17 | 86.1 |
| 691–700 | combat with gems and smoke | 67.4 | 65.4 | 80.5 (694) | 10/10 | 71.3 |
| 701 | pattern generation burst | 106.7 | | | 1/1 | 109.0 |
| 844–864 | generation burst with a time field | 46.9 | 43.1 | 92.1 (844) | 21/21 | 49.8 |
| 901–920 | time fields present | 35.4 | 34.6 | 52.5 (907) | 20/20 | 37.3 |
| 1301–1338 | camera at the level bottom | 26.3 | 23.6 | 38.1 (1304) | 38/38 | 28.4 |

Additional matched bottom windows 1101–1169, 1501–1567 and 1701–1737 average
26.7, 27.4 and 28.1 ms. Runner overhead fell in quiet and combat windows;
the generation builder itself remains essentially unchanged. The 20 ms budget is
not met. Menu scenes, from the earlier instrumented `profile.sh transition`
run, peak at two fields: splash 34.7 ms, garden 40.5 ms and style select 34.5 ms.

## Cost by owner

`--causes` charges each sample to the innermost recognised owner on its
logical call stack; helpers are charged to their callers. Milliseconds per
frame, rounded; this table is not an additive list of inclusive functions:

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet 1301–1338 | Frame 645 | Frame 701 | Frame 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 8.4 | 6.4 | 20.5 | 21.4 | 13.5 | 12.4 | 31.3 | 21.1 | 19.4 |
| Actors, sprites and display | 3.3 | 2.3 | 8.0 | 15.5 | 6.2 | 2.0 | 12.0 | 16.3 | 7.3 |
| Generation logic | 0.4 | 1.2 | 1.1 | 1.7 | 0.7 | 0.6 | 1.4 | 40.1 | 36.5 |
| Runner bookkeeping | 4.1 | 1.5 | 10.1 | 7.3 | 2.9 | 3.4 | 9.7 | 8.2 | 9.3 |
| Software float emulation | 2.0 | 1.5 | 4.4 | 6.0 | 2.3 | 2.4 | 11.9 | 5.7 | 4.9 |
| Ammo HUD | 1.7 | 0.1 | 3.3 | 1.7 | 4.2 | 1.9 | 15.3 | 0.1 | 4.5 |
| Terrain rows and scrolling | 0.3 | 1.4 | 2.5 | 5.3 | 0.8 | 0.2 | 4.1 | 6.9 | 5.2 |
| Effect surface (currency/smoke) | 0.6 | 0.6 | 2.3 | 5.1 | 0.6 | 0.6 | 0.6 | 5.0 | 0.6 |
| Time fields, R-tree, frame loop | 3.0 | 5.7 | 3.1 | 3.4 | 4.2 | 2.9 | 7.6 | 3.5 | 4.5 |
| Total | 23.9 | 20.6 | 55.3 | 67.4 | 35.4 | 26.3 | 94.0 | 106.7 | 92.1 |

## Selected expensive frames

**Frame 701, pattern generation burst (106.7 ms; previously 109.0).**
`generateLiveTerrain` takes 42.81 ms. These costs nest; do not add them:

- `emitPreparedPattern`: 38.38 ms.
- Synchronous `emitEventCell`: 20.15 ms.
- `createPrepared`: 13.92 ms, publishing occupancy, landing candidates
  and source-ordered terrain records.
- `nextU32`: 5.31 ms across the frame, including lookahead consumption.

The rest includes actor draws 2.66 ms, sprite assignment 2.21, currency/smoke
9.60, row sync 5.21 and draw collection 4.72. Removing all generation would
still leave about 64 ms of work in this frame.

**Frame 844, generation burst with a time field (92.1 ms; previously 96.3).**

- Generation: 41.05 ms; prepared-pattern dispatch 36.38, event cells 26.35
  and prepared terrain publication 6.87 (nested costs).
- Thirteen native `appendBreakables`: 3.94 ms. They retain their family
  ownership and source-ordered publication; no new generic ownership work.
- One `configureEnemy`: 3.59 ms.
- Draw collection: 4.08 ms; `drawMainWorldSpan`: 8.04, including 6.44 in
  terrain painting.

**Frame 645, damaging contact (94.0 ms; previously 97.9).**
`GameplayWorld::step` takes 48.77 ms. The ammo redraw costs 11.28 ms,
actor draws 8.34, draw collection 5.75, sprite assignment 4.57 and the health
redraw 4.22. `contactDamageNumber` costs 7.97 ms, of which
`RunnerPointDirection::evaluate` is 4.20. Sprite conversion remains offline.

## Typical frames

**Quiet, camera at the level bottom (26.3 ms; 1301–1338).**
- `GameplayWorld::step`: 17.19 ms; walking enemies 2.78 (tile tests 1.93),
  player motion 2.82 and enemy alarms 1.19.
- Draw collection: 1.97 ms; ammo redraw 1.48; sprite assignment 0.68.

The remaining simulation cost is distributed. Individual quiet motion and
traversal owners are below 3 ms; this is not a justification for tuning their
division helpers or callback instructions. The under-22-ms target is unmet.

**Combat with gems and smoke (67.4 ms; 691–700).**
- `GameplayWorld::step`: 31.20 ms.
- Actor draws: 2.36 ms; sprite assignment: 2.42 ms.
- Currency/smoke composition: 9.72 ms, including 5.93 in `stampPixel`.
- Draw collection: 5.03 ms; row sync 4.32; surface publication 2.72.
- The remaining Draw cost includes consumers that resolve artwork and
  construct backend commands, not just the retained layer traversal. Removing
  another room walk alone would not remove those consumers.

**Per-frame runner walks**, inclusive time in milliseconds:

| Walk | Quiet | Combat | 645 |
| --- | ---: | ---: | ---: |
| Animation (complete room callback) | 0.88 | 2.46 | 2.60 |
| `reconcilePhase` (activation/depth) | 0.39 | 0.39 | 0.39 |
| Captured alarm dispatch | 1.02 | 2.07 | 4.39 |
| Draw collection | 1.97 | 5.03 | 5.75 |

**Enemies in view (55.3 ms; 451–470).** Game logic takes 20.5 ms and runner
bookkeeping 10.1. **Time fields present (35.4 ms; 901–920)** are dominated by
ordinary simulation and the 4.2 ms HUD owner, not field outlines themselves.

## Consequences for the budget

- Generation still costs 43 ms at 701 and 41 ms at 844, missing the 30 ms
  target. Synchronous event Creates and prepared publication remain large.
- Combat still costs 67 ms, despite cheaper actor rendering and runner walks.
  Currency/smoke composition remains a 9.7 ms structure.
- Quiet frames remain 26.3 ms. Runner bookkeeping fell from 5.1 to 3.4 ms,
  but game logic remains about 12.4 ms, plus software arithmetic and drawing.
- Prepared terrain publication still expands immutable pattern data into
  mutable occupancy, landing and platform representations. Its 13.92 ms at
  701 includes 3.05 ms in `beginPublication` (including RNG consumption) and
  1.36 ms appending platforms. Replacing redundant native terrain
  materialization is a distinct structural target; changing word encodings
  or copy helpers is not.
- The whole-tape average is 30.20 FPS, while the selected generation burst
  still takes 106.7 ms. Do not substitute average FPS or the quantized maximum
  gap for the 20 ms worst-frame goal.

## Rejected approaches

Each row was measured on the contact tape against its reference and removed.
Patches are local, ignored copies under `tmp/`; the commit named holds the
numbers. Do not retry a row without removing the reason in its last column.

| Area | Approach | Result | Why it lost | Commit, patch |
| --- | --- | --- | --- | --- |
| Terrain ownership | Immutable cumulative rows with copy-on-write mutation | 701 107.6 → 128.6 ms; 844 92.7 → 106.0 ms | Event Creates force full-row copies: 19.6 ms at 701 and 12.0 ms at 844, outweighing pointer publication | This row's introducing commit, `terrain-row-owner-rejected.patch` |
| Alarms | Prepared family groups after the generic sorted capture | Contact 97.8 → 94.9 ms; frame 845 54.8 → 57.1 ms | Preparation adds 2.52 ms after retaining registration capture and sorting; alarm work rises 13.43 → 15.49 ms | This row's introducing commit, `alarm-prepared-groups-rejected.patch` |
| Alarms | Direct per-resource family capture rows | Combat 70.4 → 69.4 ms; frame 650 69.9 → 75.0 ms | Rebuilding resource rows costs 6.60 ms; alarm work rises 8.82 → 13.26 ms despite removing the sort | This row's introducing commit, `alarm-direct-rows-rejected.patch` |
| Effects | Offline interleaved alpha/color stamps plus exact mask-command overlap queries | Combat 77.2 → 75.7 ms; frame 649 93.3 → 95.4 ms | Exact queries and interleaved publication make the first affected frame slower | 507837c9, `effect-interleaved-commands-rejected.patch` |
| Effects | Deferred producers published directly by the CPU | Combat 77.1 → 106.8 ms; frame 649 92.8 → 127.8 ms | Per-word source/mask extraction replaces efficient DMA; publication costs 36.6 ms | b9c574b2, `effect-direct-producers-rejected.patch` |
| Actor sprites | Backfill idle channels behind nonoverlapping planar actors | Combat 77.1 → 73.4 ms; frame 645 100.2 → 100.8 ms | Per-frame bounds, overlap and prepared-part resolution add 2.05 ms at 645, exceeding the 1.39 ms draw saving | db3bc780, `actor-occlusion-backfill-rejected.patch` |
| Gauge | Synchronous word blits from Chip layers | Recharge 24 → 17 ms | ~2,300 cycles per small blit, mostly waiting; shot frames slower | d0072b4b, `blitter-gauge.patch` |
| Gauge | Runtime constant-interval painter | Missed target | Interval bookkeeping costs what it saves | d0072b4b, `interval-gauge.patch` |
| Gauge | General interval rebuild per frame | Slower | Same; replaced by bounded damage ranges | c2b663b5, `ammo-intervals-v1.patch` |
| Gauge | Column sweep with ordered overlap solving | 645/844 +17/+12 ms | 31 ms of metadata solving on the 68000 | 997d62ac, `ammo-column-sweep.patch` |
| Gauge | Per-line uniform-middle publication | 645 and 844 slower | Still visits every line | 65d5c955, `ammo-uniform-rejected.patch` |
| Gauge | Eight-line region certificates | 645 +5.4 ms, 844 +4.3 ms | Classification adds 4.4 ms and removes no drawing | 3c47d140, `ammo-certificates-rejected.patch` |
| Gauge | Interior-only prepared segments | 645 −5.5 ms | Surrounding rim and label work remained | 4005f3ea, `ammo-segments-native-rejected.patch` |
| Gauge | Complete prepared gauge bank | Pixel-exact | 268–435 KiB | c5fa7ab5 (host probe only) |
| Gauge | Independent low-plane bank | All three worst frames slower | +20 KB, more transfers | 3db26669, `ammo-independent-plane-rejected.patch` |
| Gauge | Runtime prepared-run comparison | Recharge 12.6 → 14.3 ms | Run selection costs more than it saves | 6cfeb15c, `ammo-prepared-run-rejected.patch` |
| Gauge | Offline phrase-staged column, two tall blits | 645 −19 ms against the original compositor; shot HUD 4.7 ms; +39 KB | Rejected for a 0.31 ms move in frame 701, which is noise. Against today's factored default it saves only a few ms | 7452dfe0, `ammo-prepared-transfer-rejected.patch` |
| Fields | Direct pristine-terrain word traversal | Exit frames +5–15 ms | Two full traversals per frame | c086ccea, `field-direct-words-rejected.patch` |
| Fields | Vertical colour runs | Transfers 18.8 → 16.3 ms, frames unchanged | 37 KB for no frame gain | 9fde3a40, `field-runs-rejected.patch` |
| Fields | Whole-row restoration from offline spans | Frames unchanged | Copying rows and repainting moves the cost | d19ce3e8, `field-row-restoration-rejected.patch` |
| Fields | Chained adjacent-raster XOR packets | 32.2 → 39.3 ms | Packet traversal on the CPU | 3de03179, `field-delta-rejected.patch` |
| Fields | Prepared transfer-list batches | 32.4 → 40.0 ms | Queue scheduling overhead | e558b6a3, `field-batch-rejected.patch` |
| Terrain | All-phase CPU tile bank (80 KB) | 844 painting 26 → 15 ms | Superseded by direct run composition | 947e8004, `terrain-words-experiment.patch` |

## Measurement notes

- A capture's first frame is partial, because FS-UAE starts recording at the
  VBL after the marker. Start a capture at least one frame before the frame of
  interest.
- Frames whose call tree does not contain the changed code still move by up
  to about 0.5 ms between builds (memory layout, interrupt phase). Do not
  treat that as a regression.
- The FPS tape's worst gap is quantized to whole fields and moves by one field
  between builds. Report it, but judge worst frames by traced cycles.
- Small blits lose to the CPU: a synchronous word blit costs about 2,300
  cycles, mostly waiting. Use the blitter for tall strips only.
- Memory dumps (`profile.sh frames`) show what was drawn, not what the beam
  showed. DMA faults, such as the sprite-7 stripes fixed in `b815af63`, only
  appear in the per-field screenshots embedded in instruction traces.
