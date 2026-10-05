# Amiga gameplay frame-cost audit

Where A500 gameplay time goes, measured on the default build at `11186ab9`
(2026-10-05). The preceding group-A audit, at `dd0055da`, is given alongside
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
- FPS-only build `d181e864…`: 3,185 PAL fields, 28.26 FPS, maximum six
  fields at frame 488. The preceding audit used 3,253 fields, 27.67 FPS
  and a seven-field maximum at 660. Whole-field gaps are report-only.
- [Instruction traces](amiga-profiling.md#cycle-exact-instruction-traces):
  build `1b7cc0d8…` captured 100 fields from each of frames 50, 250, 450,
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

The gameplay step overlaps the previous frame's publication wait, so traced
frames contain almost no idle time. One PAL field is 20 ms (142,102 cycles).

| Tape frames | Scene | Mean ms | Median | Worst | Over 20 ms | Previous mean |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 51–91 | entrance bowl, firing | 26.9 | 28.0 | 39.1 (54) | 41/41 | 27.0 |
| 251–303 | open descent | 20.6 | 17.5 | 46.5 (278) | 19/53 | 20.6 |
| 451–470 | enemies in view | 59.0 | 53.4 | 99.5 (464) | 20/20 | 60.8 |
| 645–661 | damaging contact | 86.1 | 85.7 | 109.3 (660) | 17/17 | 96.8 |
| 691–700 | combat with gems and smoke | 71.3 | 68.4 | 87.5 (694) | 10/10 | 85.3 |
| 701 | pattern generation burst | 109.0 | | | 1/1 | 122.2 |
| 844–864 | generation burst with a time field | 49.8 | 46.7 | 96.3 (844) | 21/21 | 50.0 |
| 901–920 | time fields present | 37.3 | 36.4 | 54.2 (907) | 20/20 | 37.3 |
| 1301–1338 | camera at the level bottom | 28.4 | 24.6 | 42.4 (1304) | 38/38 | 28.4 |

Additional matched bottom windows 1101–1169, 1501–1567 and 1701–1737 average
28.5, 29.5 and 30.1 ms. Combat and contact costs fell; quiet frames and the
generation builder itself remain essentially unchanged. The 20 ms budget is
not met. Menu scenes, from the earlier instrumented `profile.sh transition`
run, peak at two fields: splash 34.7 ms, garden 40.5 ms and style select 34.5 ms.

## Cost by owner

`--causes` charges each sample to the innermost recognised owner on its
logical call stack; helpers are charged to their callers. Milliseconds per
frame, rounded; this table is not an additive list of inclusive functions:

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet 1301–1338 | Frame 645 | Frame 701 | Frame 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 8.5 | 6.3 | 20.6 | 21.6 | 13.5 | 12.3 | 31.8 | 20.8 | 19.4 |
| Actors, sprites and display | 3.3 | 2.3 | 8.0 | 15.4 | 6.3 | 2.0 | 12.0 | 16.0 | 7.3 |
| Generation logic | 0.4 | 1.2 | 1.1 | 1.7 | 0.7 | 0.6 | 1.4 | 40.1 | 36.5 |
| Runner bookkeeping | 6.1 | 2.7 | 13.0 | 10.2 | 4.5 | 5.1 | 12.9 | 10.7 | 11.7 |
| Software float emulation | 2.2 | 1.5 | 5.0 | 6.7 | 2.4 | 2.8 | 12.2 | 5.8 | 6.5 |
| Ammo HUD | 1.7 | 0.1 | 3.4 | 1.7 | 4.3 | 1.9 | 15.3 | 0.1 | 4.5 |
| Terrain rows and scrolling | 0.3 | 1.4 | 2.5 | 5.3 | 0.8 | 0.2 | 4.0 | 7.0 | 5.3 |
| Effect surface (currency/smoke) | 0.6 | 0.6 | 2.3 | 5.1 | 0.6 | 0.6 | 0.6 | 4.9 | 0.6 |
| Time fields, R-tree, frame loop | 3.6 | 4.6 | 3.3 | 3.6 | 4.3 | 3.0 | 7.7 | 3.7 | 4.6 |
| Total | 26.9 | 20.6 | 59.0 | 71.3 | 37.3 | 28.4 | 97.9 | 109.0 | 96.3 |

## Selected expensive frames

**Frame 701, pattern generation burst (109.0 ms; previously 122.2).**
`generateLiveTerrain` takes 42.97 ms. These costs nest; do not add them:

- `emitPreparedPattern`: 38.38 ms.
- Synchronous `emitEventCell`: 20.23 ms.
- `createPrepared`: 13.92 ms, publishing occupancy, landing candidates
  and source-ordered terrain records.
- `nextU32`: 5.31 ms across the frame, including lookahead consumption.

The rest includes actor draws 2.35 ms, sprite assignment 2.21, currency/smoke
9.26, row sync 5.21 and draw collection 4.78. Removing all generation would
still leave about 66 ms of work in this frame.

**Frame 844, generation burst with a time field (96.3 ms; previously 96.6).**

- Generation: 40.92 ms; prepared-pattern dispatch 36.24, event cells 26.22
  and prepared terrain publication 6.87 (nested costs).
- Thirteen native `appendBreakables`: 3.94 ms. They retain their family
  ownership and source-ordered publication; no new generic ownership work.
- One `configureEnemy`: 3.37 ms.
- Draw collection: 7.89 ms; `drawMainWorldSpan`: 8.11.
- Prepared time-field operations: 4.57 ms together.

**Frame 645, damaging contact (97.9 ms; previously 101.2).**
`GameplayWorld::step` takes 52.69 ms. The ammo redraw costs 11.21 ms,
actor draws 8.27, draw collection 5.92, sprite assignment 4.56 and the health
redraw 4.05. `contactDamageNumber` costs 7.97 ms, of which
`RunnerPointDirection::evaluate` is 4.20. A cold damage-label composition
costs 3.33 ms within its 5.10 ms cache/draw path. Prepared sprite payload
updates cost 3.27 ms; conversion remains offline.

## Typical frames

**Quiet, camera at the level bottom (28.4 ms; 1301–1338).**
- `GameplayWorld::step`: 18.57 ms; walking enemies 2.84 (tile tests 1.93),
  player motion 2.77 and enemy alarms 1.25.
- Draw collection: 2.67 ms; ammo redraw 1.48; sprite assignment 0.68.
- Repeated collision-owner validation: 1.37 ms across its callers.

The cost is distributed across per-actor walks and their callbacks; the next
structural target is those walks, not their individual small helpers.

**Combat with gems and smoke (71.3 ms; 691–700).**
- `GameplayWorld::step`: 33.90 ms.
- Actor draws: 2.37 ms, including 1.69 in damage-label cache/draw. No software
  actor asset draws in this window. Sprite assignment takes 2.42 ms, with
  1.15 in prepared payload updates.
- Currency/smoke composition: 9.69 ms, including 6.02 in `stampPixel`.
- Draw collection: 6.06 ms; row sync 4.29; surface publication 2.71.
- Integer division, modulo and multiply helpers: 4.56 ms of self time.
  Gem-position refresh is 0.28 ms; do not reopen that arithmetic.

**Per-frame runner walks**, inclusive time in milliseconds:

| Walk | Quiet | Combat | 645 |
| --- | ---: | ---: | ---: |
| Animation (`visitAnimationRetainedChecked`) | 1.10 | 3.82 | 4.45 |
| `reconcilePhase` (activation/depth) | 1.24 | 1.24 | 1.24 |
| Captured alarm dispatch | 1.01 | 2.14 | 4.61 |
| Draw collection | 2.67 | 6.06 | 5.92 |

**Enemies in view (59.0 ms; 451–470).** Game logic takes 20.6 ms and runner
bookkeeping 13.0. **Time fields present (37.3 ms; 901–920)** remain essentially
unchanged; ordinary simulation and the 4.3 ms HUD owner dominate over field
outlines themselves.

## Consequences for the budget

- Generation still costs 43 ms at 701 and 41 ms at 844, missing the 30 ms
  target. Synchronous event Creates and prepared publication remain large.
- Combat still costs 71 ms, despite cheaper actor rendering and gem state.
  Currency/smoke composition remains a 9.7 ms structure.
- Quiet frames remain 28.4 ms. Per-frame runner walks are the next ranked
  target; group B did not remove that work.
- Whole-tape FPS improved much less than the combat window. Do not substitute
  its average, or its quantized maximum gap, for the 20 ms worst-frame goal.

## Rejected approaches

Each row was measured on the contact tape against its reference and removed.
Patches are local, ignored copies under `tmp/`; the commit named holds the
numbers. Do not retry a row without removing the reason in its last column.

| Area | Approach | Result | Why it lost | Commit, patch |
| --- | --- | --- | --- | --- |
| Alarms | Prepared family groups after the generic sorted capture | Contact 97.8 → 94.9 ms; frame 845 54.8 → 57.1 ms | Preparation adds 2.52 ms after retaining registration capture and sorting; alarm work rises 13.43 → 15.49 ms | This row's introducing commit, `alarm-prepared-groups-rejected.patch` |
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
