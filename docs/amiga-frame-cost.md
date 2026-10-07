# Amiga gameplay frame-cost audit

Where A500 gameplay time goes, measured on the default build at `36d8a0f2`
(2026-10-06). The preceding audit, at `22df6961`, is given alongside for
matched comparison. Remaining work is ranked in
[open-work §5](open-work.md#5-meet-the-amiga-frame-budget); this document holds
the current measurements and the approaches already ruled out.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO.
- Tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1800 gameplay updates. It covers
  natural descent with one damaging contact, view Y 39 to 1874, at most five
  allocated enemy records and final health 3. It has no shop, no dense
  encounter and no garden-to-well transition.
- FPS-only build: 2,950 PAL fields, 30.51 FPS, maximum six fields at frame
  701. The preceding audit used 2,980 fields, 30.20 FPS and a five-field
  maximum at 464. Whole-field gaps are report-only.
- [Instruction traces](amiga-profiling.md#cycle-exact-instruction-traces):
  build `e9bcce72…` captured 100 fields from each of frames 50, 250, 450,
  644, 690, 843, 900, 1100, 1300, 1500 and 1700. Use the fixed frame ranges
  below, not every frame that fits in each capture. A capture's first frame
  is partial and excluded; 701 and 844 are complete. Frame 488, the first gem
  pickup, lies outside these captures; it measured 95.5 ms in `36d8a0f2`.

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
| 51–91 | entrance bowl, firing | 23.9 | 24.3 | 34.9 (64) | 28/41 | 23.9 |
| 251–303 | open descent | 20.6 | 18.7 | 44.3 (278) | 19/53 | 20.6 |
| 451–470 | enemies in view | 53.6 | 44.1 | 84.6 (464) | 20/20 | 55.3 |
| 645–661 | damaging contact | 80.1 | 79.4 | 100.6 (646) | 17/17 | 82.1 |
| 691–700 | combat with gems and smoke | 67.3 | 65.2 | 80.2 (694) | 10/10 | 67.4 |
| 701 | pattern generation burst | 104.2 | | | 1/1 | 106.7 |
| 844–864 | generation burst with a time field | 45.7 | 41.8 | 87.9 (844) | 21/21 | 46.9 |
| 901–920 | time fields present | 35.7 | 34.6 | 52.2 (907) | 20/20 | 35.4 |
| 1301–1338 | camera at the level bottom | 26.3 | 23.2 | 38.5 (1304) | 38/38 | 26.3 |

Additional matched bottom windows 1101–1169, 1501–1567 and 1701–1737 average
26.7, 27.5 and 28.1 ms. The worst traced frames are 701 (104.2 ms), 646
(100.6), 645 (93.6), 660 (90.2) and 844 (87.9). The 20 ms budget is not met.
Menu scenes, from the earlier instrumented `profile.sh transition` run, peak
at two fields: splash 34.7 ms, garden 40.5 ms and style select 34.5 ms.

## Cost by owner

`--causes` charges each sample to the innermost recognised owner on its
logical call stack; helpers are charged to their callers. Owners match
function names, not signature types. Milliseconds per frame, rounded; this
table is not an additive list of inclusive functions:

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet 1301–1338 | 645 | 646 | 660 | 701 | 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 8.9 | 6.8 | 21.7 | 23.4 | 14.3 | 13.1 | 34.4 | 32.0 | 25.2 | 23.6 | 24.1 |
| Actors, sprites and display | 3.2 | 2.2 | 7.8 | 15.3 | 6.2 | 1.9 | 11.9 | 8.3 | 16.3 | 16.4 | 7.2 |
| Generation logic | 0.3 | 1.1 | 0.4 | 0.3 | 0.4 | 0.4 | 0.4 | 0.4 | 0.2 | 35.2 | 28.0 |
| Runner bookkeeping | 4.5 | 1.5 | 9.0 | 7.5 | 3.0 | 3.6 | 9.5 | 9.5 | 5.1 | 8.2 | 10.9 |
| Software float emulation | 1.4 | 1.1 | 3.7 | 5.3 | 1.9 | 1.9 | 10.1 | 28.8 | 4.8 | 5.2 | 2.9 |
| HUD redraws | 1.7 | 0.1 | 3.3 | 1.7 | 4.2 | 1.9 | 19.3 | 14.0 | 22.2 | 0.1 | 4.5 |
| Terrain rows and scrolling | 0.3 | 1.3 | 2.3 | 4.6 | 0.8 | 0.2 | 4.0 | 3.7 | 3.0 | 6.4 | 5.1 |
| Effect surface (currency/smoke) | 0.6 | 0.6 | 2.5 | 5.8 | 0.6 | 0.6 | 0.6 | 0.6 | 10.5 | 5.6 | 0.6 |
| Time fields, R-tree, frame loop | 3.0 | 5.8 | 3.0 | 3.4 | 4.2 | 2.8 | 3.5 | 3.4 | 3.0 | 3.6 | 4.6 |
| Total | 23.9 | 20.6 | 53.6 | 67.3 | 35.7 | 26.3 | 93.6 | 100.6 | 90.2 | 104.2 | 87.9 |

HUD redraws cover the ammo gauge, health, gem counter and streak. Before
`eb272433` the software-float owner also absorbed draw collection whose
signatures mention runner float types; the previous audit's float rows are
overstated by up to about 5 ms in combat.

## Selected expensive frames

**Frame 701, pattern generation burst (104.2 ms; previously 106.7).**
`generateLiveTerrain` takes 40.79 ms. These costs nest; do not add them:

- `emitPreparedPattern`: 36.30 ms.
- Synchronous `emitEventCell`: 18.07 ms.
- `createPrepared`: 14.00 ms, publishing occupancy, landing candidates
  and source-ordered terrain records; `beginPublication` is 3.05 of it.
- `nextU32`: 5.31 ms across the frame, including lookahead consumption.

The rest includes actor draws 2.35 ms, sprite assignment 2.21, currency/smoke
9.62, row sync 5.21 and draw collection 4.87. Removing all generation would
still leave about 63 ms of work in this frame.

**Frame 646, first damage-number step (100.6 ms; new).** The frame after
contact. `GameplayWorld::step` takes 67.31 ms, of which the player damage
number's first step is 28.25. Its `lengthdir_x` evaluates the recovered SSE
cosine program once in software binary64: 166,748 cycles (23.47 ms), made of
26 multiplies at 3,461 cycles and 33 additions at 2,185. The ammo redraw is
11.0 ms. While the number lives, each later frame still spends about 2.1 ms
in its binary64 motion and damping.

**Frame 645, damaging contact (93.6 ms; previously 94.0).**
`GameplayWorld::step` takes 48.46 ms. The ammo redraw costs 11.21 ms,
actor draws 8.26, draw collection 5.98, sprite assignment 4.56 and the health
redraw 4.05. `contactDamageNumber` costs 7.97 ms, of which
`RunnerPointDirection::evaluate` is 4.20. Contact therefore costs about
36 ms of damage-number arithmetic across 645 and 646.

**Frame 660, gem counter and streak change (90.2 ms; previously 106.3).**
The gem counter redraw is 12.88 ms (digit composition on first use 5.34,
publication 4.97) and the streak 9.64 ms (publication 5.20, coverage
derivation about 3.9). Currency/smoke composition is 13.88 ms, including
7.61 in `stampPixel`; effect publication 5.68 and row sync 4.10.

**Frame 844, generation burst with a time field (87.9 ms; previously 92.1).**

- Generation: 36.93 ms; prepared-pattern dispatch 32.28, event cells 22.17
  and prepared terrain publication 6.87 (nested costs).
- Thirteen native breakable creations: 3.63 ms, 1,984 cycles each.
- One `configureEnemy`: 3.59 ms.
- Draw collection: 4.25 ms; `drawMainWorldSpan`: 7.97, including 6.37 in
  cached terrain painting.

## Typical frames

**Quiet, camera at the level bottom (26.3 ms; 1301–1338).**
- `GameplayWorld::step`: 17.30 ms; walking enemies 2.79, player motion 2.82
  and enemy alarms 1.29.
- Draw collection: 1.98 ms; ammo redraw 1.47; sprite assignment 0.68.

The remaining simulation cost is distributed. Individual quiet motion and
traversal owners are below 3 ms; this is not a justification for tuning their
division helpers or callback instructions. The under-22-ms target is unmet.

**Combat with gems and smoke (67.3 ms; 691–700).**
- `GameplayWorld::step`: 31.06 ms; the live damage number's motion 2.1.
- Actor draws: 2.36 ms; sprite assignment: 2.42 ms.
- Currency/smoke composition: 9.74 ms, including 6.00 in `stampPixel`.
- Draw collection: 5.00 ms; row sync 4.29; surface publication 2.71.
- The remaining Draw cost includes consumers that resolve artwork and
  construct backend commands, not just the retained layer traversal. Removing
  another room walk alone would not remove those consumers.

**Per-frame runner walks**, inclusive time in milliseconds:

| Walk | Quiet | Combat | 645 |
| --- | ---: | ---: | ---: |
| Animation (complete room callback) | 0.87 | 2.43 | 2.68 |
| `reconcilePhase` (activation/depth) | 0.40 | 0.39 | 0.39 |
| Captured alarm dispatch, including the box prefix | 1.10 | 1.87 | 3.97 |
| Draw collection | 1.98 | 5.00 | 5.98 |

**Enemies in view (53.6 ms; 451–470).** Game logic takes 21.7 ms and runner
bookkeeping 9.0. **Time fields present (35.7 ms; 901–920)** are dominated by
ordinary simulation and the 4.2 ms HUD owner, not field outlines themselves.

## Consequences for the budget

- Generation still costs 41 ms at 701 and 37 ms at 844, missing the 30 ms
  target. Synchronous event Creates and prepared publication remain large.
- A damaging contact now costs two expensive frames: the contact itself and
  the damage number's first step, about 36 ms of binary64 direction and
  cosine arithmetic between them.
- Combat still costs 67 ms. Currency/smoke composition remains a 9.7 ms
  structure; draw collection and row sync are 5.0 and 4.3 ms.
- The gem counter and streak no longer draw glyph by glyph, but a change of
  either still costs about 22 ms, mostly in compiled-C publication loops.
- Quiet frames remain 26.3 ms; game logic is about 13 ms, plus runner
  bookkeeping, software arithmetic and drawing.
- The whole-tape average is 30.51 FPS, while the worst traced frame still
  takes 104.2 ms. Do not substitute average FPS or the quantized maximum gap
  for the 20 ms worst-frame goal.

## Rejected approaches

Each row was measured on the contact tape against its reference and removed.
Each patch, in [`rejected/`](rejected/), holds the removed code; the commit
named holds the numbers. Do not retry a row without removing the reason in its last column.

| Area | Approach | Result | Why it lost | Commit, patch |
| --- | --- | --- | --- | --- |
| Cold entry | Decode initial raster in bounded clean-style updates | Confirmation 639.5 → 635.1 ms; raster phase still 180.5 ms | Only 1,008 of 2,160 rows finish, so entry repeats the full copy; misses the 150 ms target and first gameplay rises 38.5 → 43.4 ms | This row's introducing commit, [`initial-raster-style-staging.patch`](rejected/initial-raster-style-staging.patch) |
| Alarms | Borrow enemy Step snapshot and merge typed alarm dispatch with remaining families | 645 93.9 → 92.3 ms; quiet 26.326 → 26.392 ms | Quiet alarm work rises 1.10 → 1.35 ms; borrowing adds capture/revision and merge work even with inactive counters, violating the window-mean gate | This row's introducing commit, [`enemy-borrowed-alarms.patch`](rejected/enemy-borrowed-alarms.patch) |
| Alarms | Mutation-owned enemy roster with pinned copy-on-write phase order | 645 dispatch 3.97 → 2.33 ms; frame 844 87.7 → 88.3 ms, repeated | Birth maintenance adds 0.25 ms and generation-frame dispatch adds 0.25 ms; exceeds the budget-frame regression gate | This row's introducing commit, [`enemy-alarm-roster-rejected.patch`](rejected/enemy-alarm-roster-rejected.patch) |
| HUD text | Retained final-word comparison and digit-band-only publication | 660 90.5 → 87.1 ms; counter/streak still 20.02 ms | Fast-memory comparison/cache maintenance leaves publication at 7.43 ms; misses the 5 ms saving target | This row's introducing commit, [`hud-retained-delta.patch`](rejected/hud-retained-delta.patch) |
| Display restoration | Ordered disjoint row-debt ranges replacing the row map | Combat 67.3 → 66.0 ms; 886 34.1 → 34.8 ms | Sync saves 1.29 ms, below the 1.5 ms target; time-field dirty queries rise 1.0 → 1.5 ms and regress an affected frame | This row's introducing commit, [`row-debts-rejected.patch`](rejected/row-debts-rejected.patch) |
| Display restoration | Shared word coverage for row debts and cached field/ray footprints | Restoration/debt 5.16 → 4.52 ms; combat 63.14 → 62.57 ms; field window 45.32 → 45.61 ms | Saves only 0.64 ms against a 1.5 ms target; footprint maintenance costs 0.29 ms per field frame while queries save 0.04 ms | This row's introducing commit, [`row-coverage-rejected.patch`](rejected/row-coverage-rejected.patch) |
| Effects | Offline shifted CPU word stamps into the retained raster | Combat 67.3 → 66.9 ms; 649 85.3 → 87.3 ms | CPU word painting costs 2.82 ms in combat and requires extra DMA fences; misses the 3 ms target and regresses first-effect composition | This row's introducing commit, [`effect-word-stamps-rejected.patch`](rejected/effect-word-stamps-rejected.patch) |
| Terrain ownership | Immutable prefixes with per-cell overrides, no row copies | 701 105.9 → 111.2 ms; 844 88.8 → 96.0 ms; tape 30.19 → 29.61 FPS | Override lookup/merging outweighs removed materialization; terrain drawing at 844 rises 8.0 → 9.6 ms | This row's introducing commit, [`terrain-sparse-owner-rejected.patch`](rejected/terrain-sparse-owner-rejected.patch) |
| Effects | One three-plane stamp submission with delta register packets | Combat 67.3 → 66.5 ms; frame 649 85.35 → 87.01 ms | Sparse first-effect frame pays for queued successors: effect queue dispatch 0.25 → 2.04 ms, outweighing shared geometry | This row's introducing commit, [`effect-stamp-batch-rejected.patch`](rejected/effect-stamp-batch-rejected.patch) |
| Terrain ownership | Immutable cumulative rows with copy-on-write mutation | 701 107.6 → 128.6 ms; 844 92.7 → 106.0 ms | Event Creates force full-row copies: 19.6 ms at 701 and 12.0 ms at 844, outweighing pointer publication | This row's introducing commit, [`terrain-row-owner-rejected.patch`](rejected/terrain-row-owner-rejected.patch) |
| Alarms | Prepared family groups after the generic sorted capture | Contact 97.8 → 94.9 ms; frame 845 54.8 → 57.1 ms | Preparation adds 2.52 ms after retaining registration capture and sorting; alarm work rises 13.43 → 15.49 ms | This row's introducing commit, [`alarm-prepared-groups-rejected.patch`](rejected/alarm-prepared-groups-rejected.patch) |
| Alarms | Direct per-resource family capture rows | Combat 70.4 → 69.4 ms; frame 650 69.9 → 75.0 ms | Rebuilding resource rows costs 6.60 ms; alarm work rises 8.82 → 13.26 ms despite removing the sort | This row's introducing commit, [`alarm-direct-rows-rejected.patch`](rejected/alarm-direct-rows-rejected.patch) |
| Effects | Offline interleaved alpha/color stamps plus exact mask-command overlap queries | Combat 77.2 → 75.7 ms; frame 649 93.3 → 95.4 ms | Exact queries and interleaved publication make the first affected frame slower | 507837c9, [`effect-interleaved-commands-rejected.patch`](rejected/effect-interleaved-commands-rejected.patch) |
| Effects | Deferred producers published directly by the CPU | Combat 77.1 → 106.8 ms; frame 649 92.8 → 127.8 ms | Per-word source/mask extraction replaces efficient DMA; publication costs 36.6 ms | b9c574b2, [`effect-direct-producers-rejected.patch`](rejected/effect-direct-producers-rejected.patch) |
| Actor sprites | Backfill idle channels behind nonoverlapping planar actors | Combat 77.1 → 73.4 ms; frame 645 100.2 → 100.8 ms | Per-frame bounds, overlap and prepared-part resolution add 2.05 ms at 645, exceeding the 1.39 ms draw saving; superseded by prepared bindings in `11186ab9` | db3bc780, [`actor-occlusion-backfill-rejected.patch`](rejected/actor-occlusion-backfill-rejected.patch) |
| Gauge | Synchronous word blits from Chip layers | Recharge 24 → 17 ms | ~2,300 cycles per small blit, mostly waiting; shot frames slower | d0072b4b, [`blitter-gauge.patch`](rejected/blitter-gauge.patch) |
| Gauge | Runtime constant-interval painter | Missed target | Interval bookkeeping costs what it saves | d0072b4b, [`interval-gauge.patch`](rejected/interval-gauge.patch) |
| Gauge | General interval rebuild per frame | Slower | Same; replaced by bounded damage ranges | c2b663b5, [`ammo-intervals-v1.patch`](rejected/ammo-intervals-v1.patch) |
| Gauge | Column sweep with ordered overlap solving | 645/844 +17/+12 ms | 31 ms of metadata solving on the 68000 | 997d62ac, [`ammo-column-sweep.patch`](rejected/ammo-column-sweep.patch) |
| Gauge | Per-line uniform-middle publication | 645 and 844 slower | Still visits every line | 65d5c955, [`ammo-uniform-rejected.patch`](rejected/ammo-uniform-rejected.patch) |
| Gauge | Eight-line region certificates | 645 +5.4 ms, 844 +4.3 ms | Classification adds 4.4 ms and removes no drawing | 3c47d140, [`ammo-certificates-rejected.patch`](rejected/ammo-certificates-rejected.patch) |
| Gauge | Interior-only prepared segments | 645 −5.5 ms | Surrounding rim and label work remained | 4005f3ea, [`ammo-segments-native-rejected.patch`](rejected/ammo-segments-native-rejected.patch) |
| Gauge | Complete prepared gauge bank | Pixel-exact | 268–435 KiB | c5fa7ab5 (host probe only) |
| Gauge | Independent low-plane bank | All three worst frames slower | +20 KB, more transfers | 3db26669, [`ammo-independent-plane-rejected.patch`](rejected/ammo-independent-plane-rejected.patch) |
| Gauge | Runtime prepared-run comparison | Recharge 12.6 → 14.3 ms | Run selection costs more than it saves | 6cfeb15c, [`ammo-prepared-run-rejected.patch`](rejected/ammo-prepared-run-rejected.patch) |
| Gauge | Offline phrase-staged column, two tall blits | 645 −19 ms against the original compositor; shot HUD 4.7 ms; +39 KB | Rejected for a 0.31 ms move in frame 701, which is noise. Against today's factored default it saves only a few ms | 7452dfe0, [`ammo-prepared-transfer-rejected.patch`](rejected/ammo-prepared-transfer-rejected.patch) |
| Fields | Direct pristine-terrain word traversal | Exit frames +5–15 ms | Two full traversals per frame | c086ccea, [`field-direct-words-rejected.patch`](rejected/field-direct-words-rejected.patch) |
| Fields | Vertical colour runs | Transfers 18.8 → 16.3 ms, frames unchanged | 37 KB for no frame gain | 9fde3a40, [`field-runs-rejected.patch`](rejected/field-runs-rejected.patch) |
| Fields | Whole-row restoration from offline spans | Frames unchanged | Copying rows and repainting moves the cost | d19ce3e8, [`field-row-restoration-rejected.patch`](rejected/field-row-restoration-rejected.patch) |
| Fields | Chained adjacent-raster XOR packets | 32.2 → 39.3 ms | Packet traversal on the CPU | 3de03179, [`field-delta-rejected.patch`](rejected/field-delta-rejected.patch) |
| Fields | Prepared transfer-list batches | 32.4 → 40.0 ms | Queue scheduling overhead | e558b6a3, [`field-batch-rejected.patch`](rejected/field-batch-rejected.patch) |
| Terrain | All-phase CPU tile bank (80 KB) | 844 painting 26 → 15 ms | Superseded by direct run composition | 947e8004, [`terrain-words-experiment.patch`](rejected/terrain-words-experiment.patch) |

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
