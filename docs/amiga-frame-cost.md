# Amiga gameplay frame-cost audit

Where A500 gameplay time goes, measured on the default build at `dd0055da`
(2026-10-05), with every accepted optimization enabled. The previous audit, at
`b815af63`, is given alongside for comparison. Remaining work is ranked in
[open-work §5](open-work.md#5-meet-the-amiga-frame-budget); this document holds
the measurements and the approaches already ruled out.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO.
- Tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1800 gameplay updates. It covers
  natural descent with one damaging contact, view Y 39 to 1874, at most five
  allocated enemy records and final health 3. It has no shop, no dense
  encounter and no garden-to-well transition.
- FPS-only build `7af03c18…`: 3,253 PAL fields (previously 3,252), 27.67 FPS.
  The worst gap is 7 fields at frame 660 (previously 8 at 701).
- [Instruction traces](amiga-profiling.md#cycle-exact-instruction-traces):
  build `c5ae10e3…` captured 100 fields from each of frames 50, 250, 450,
  644, 690, 843, 900, 1100, 1300, 1500 and 1700. Frame 701 is complete in
  the 690 capture; starting at 843 makes 844 complete too. The first frame
  of each capture is partial and excluded. The older `b99c2f69` audit's
  frame 844 was partial, so its 179.7 ms undercounts the frame.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=3000 \
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
| 51–91 | entrance bowl, firing | 27.0 | 27.5 | 39.6 (64) | 41/41 | 27.0 |
| 251–303 | open descent | 20.6 | 17.6 | 46.5 (278) | 19/53 | 21.4 |
| 451–470 | enemies in view | 60.8 | 53.6 | 101.9 (464) | 20/20 | 60.8 |
| 645–661 | damaging contact | 96.8 | 97.8 | 121.6 (660) | 17/17 | 96.7 |
| 691–700 | combat with gems and smoke | 85.3 | 82.8 | 103.4 (694) | 10/10 | 85.4 |
| 701 | pattern generation burst | 122.2 | | | 1/1 | 172.2 |
| 844–864 | generation burst with a time field | 50.0 | 46.7 | 96.6 (844) | 21/21 | —* |
| 901–920 | time fields present | 37.3 | 36.4 | 54.3 (907) | 20/20 | 37.3 |
| 1301–1338 | camera at the level bottom | 28.4 | 24.3 | 43.0 (1304) | 38/38 | 28.4 |

*The new window includes complete 844. The previous audit's window mean is
not used as a matched comparison; compare the separately measured complete
844 burst (120.7 ms) instead. Additional bottom windows
1101–1169, 1501–1567 and 1701–1737 average 28.5, 29.5 and 30.0 ms.

Menu scenes, from the earlier instrumented `profile.sh transition` run, peak at two
fields: splash 34.7 ms, garden 40.5 ms and style select 34.5 ms.

Generation bursts are substantially cheaper; combat, contact, time-field
and quiet frames are essentially unchanged. The 20 ms budget is not met.

## Cost by owner

`--causes` charges each sample to the innermost recognised owner on its
logical call stack; helpers are charged to their callers. Milliseconds per
frame:

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet 1301–1338 | Frame 645 | Frame 701 | Frame 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 8.4 | 6.2 | 21.4 | 28.2 | 13.5 | 12.3 | 31.0 | 27.1 | 19.4 |
| Actors, sprites and display | 3.7 | 2.4 | 8.9 | 22.0 | 6.5 | 2.1 | 15.9 | 22.0 | 7.5 |
| Generation logic | 0.4 | 1.2 | 1.1 | 1.7 | 0.7 | 0.6 | 1.4 | 40.1 | 36.4 |
| Runner bookkeeping | 6.1 | 2.7 | 12.9 | 10.2 | 4.5 | 5.1 | 12.9 | 10.7 | 11.5 |
| Software float emulation | 2.2 | 1.5 | 5.0 | 6.6 | 2.4 | 2.8 | 12.1 | 5.8 | 6.6 |
| Ammo HUD | 1.7 | 0.1 | 3.3 | 1.7 | 4.2 | 1.9 | 15.3 | 0.1 | 4.5 |
| Terrain rows and scrolling | 0.4 | 1.4 | 2.6 | 6.4 | 0.8 | 0.2 | 4.3 | 7.9 | 5.5 |
| Effect surface (currency/smoke) | 0.6 | 0.6 | 2.2 | 4.9 | 0.6 | 0.6 | 0.6 | 4.8 | 0.6 |
| Time fields, R-tree, frame loop | 3.4 | 4.6 | 3.3 | 3.6 | 4.2 | 2.8 | 7.7 | 3.9 | 4.6 |
| Total | 27.0 | 20.6 | 60.8 | 85.3 | 37.3 | 28.4 | 101.2 | 122.2 | 96.6 |

## Selected expensive frames

**Frame 701, pattern generation burst (122.2 ms; previously 172.2).**
`generateLiveTerrain` takes 42.95 ms. These costs nest; do not add them:

- `emitPreparedPattern` takes 38.46 ms, with 36.99 in row-program dispatch.
- Synchronous `emitEventCell` takes 20.31 ms, including 13.15 in
  `emitObject` and 7.88 in the ordinary generation callback.
- `createPrepared` takes 13.92 ms, including 6.93 ms self time publishing
  occupancy, landing candidates and source-ordered terrain records.
- `nextU32` takes 5.31 ms across the frame, including lookahead consumption;
  the expensive WELL computation is no longer in the burst.
- Time-field adoption costs 1.82 ms, below the separate-rewrite threshold.

The rest is ordinary combat work: actor draws 9.9 ms, currency/smoke 9.6,
row sync 6.1 and draw collection 5.7.

**Frame 844, generation burst with a time field (96.6 ms; previously 120.7).**

- Generation takes 40.74 ms: prepared-pattern dispatch 36.01, event cells
  25.98 and prepared terrain publication 6.87 (nested costs).
- 13 native `appendBreakables` take 3.94 ms (2,156 cycles each): 2.59 in
  slot/input/grid publication and 1.26 in chronological/layer metadata.
  Unused slot defaults are prepared at room boundaries, not on birth.
- `configureEnemy` takes 3.22 ms for one birth: 2.69 ms in ownership,
  including 1.66 in room-layer attachment. Neither ownership nor record
  initialization alone meets the 3 ms threshold for a separate rewrite.
- Draw collection takes 8.1 ms and `drawMainWorldSpan` 8.3.
- The prepared time field costs 2.4 ms to prepare and 2.0 ms to draw.

**Frame 645, damaging contact (101.2 ms; previously 101.4).**
`GameplayWorld::step` takes 52.5 ms; enemy contacts are about 10 ms of it and the
collision owner's attach lambdas about 15 ms. The ammo gauge takes 11.2 ms,
actor draws 11.0, draw collection 6.5, sprite assignment 4.7 and the health
HUD 4.0. `contactDamageNumber` costs 8.0 ms, of which
`RunnerPointDirection::evaluate` is 4.2 (binary64 arithmetic around the
0.9 ms table atan). Sprite assignment spends 2.8 ms building DMA words in
`buildHardwareSpritePart`.

## Typical frames

**Quiet, camera at the level bottom (28.4 ms; 1300–1338).**
- `GameplayWorld::step` takes 18.4 ms: walking enemies 2.8 (tile tests 1.9),
  the player 2.8, enemy alarms 1.2 and the collision owner's attach lambdas
  about 6.5 ms.
- Draw collection takes 2.7 ms, the ammo gauge 1.5 and sprite assignment 0.7.
- Integer division helpers take 1.7 ms of self time.

No function takes more than a tenth of the frame. Only removing the per-actor
walks can reach 20 ms, not tuning the functions in them.

**Combat with gems and smoke (85.3 ms; 691–700).**
- `GameplayWorld::step` takes 37.8 ms, currency collection 4.6 ms of it.
- Actor draws take 10.7 ms: damage-number text 5.5 (`AmigaDamageText::draw`,
  about nine CPU planar-word writes), CPU-drawn actors 3.2 (about two at
  10,000 cycles) and row damage 0.9.
- Currency/smoke composition takes 9.6 ms: three `stampPixel` calls at 14,038
  cycles each, 1.8 ms waiting on the blitter in `intersects`, and `clear` 0.9.
- Draw collection takes 6.6 ms, row sync 6.3 and effect-surface publication
  2.6.
- The 32-bit division, modulo and multiply helpers take 10.2 ms of self time.
  `GemMotion::q8`, `rounded` and `pixel` cause 4.4 ms of it, by dividing
  1/12800-pixel positions on every read; no other caller exceeds 1 ms.

**Enemies in view (60.8 ms; 450–470).** Game logic takes 21.5 ms and runner
bookkeeping 12.9.

**Time fields present (37.3 ms; 900–920).** Field preparation and drawing
take a few milliseconds. The rest of the frame is the ordinary game step plus
4.2 ms of gauge.

## Consequences for the budget

- Generation still costs 43 ms at 701 and 41 ms at 844, missing the 30 ms
  target. Synchronous event Creates and prepared publication are now the
  large remaining structures, not the removed cell interpreter or WELL loop.
- Without generation, frame 701 would still be an 80 ms combat frame:
  generation and combat must both fall.
- Combat costs 85 ms. Actor and effect composition is about 30 ms of it, and
  arithmetic helpers about 10 ms.
- The quiet frame is 28.4 ms. Its gap to 20 ms is the per-actor runner walks:
  collision-owner attach and dispatch, draw collection, alarms and animation.

## Rejected approaches

Each row was measured on the contact tape against its reference and removed.
Patches are local, ignored copies under `tmp/`; the commit named holds the
numbers. Do not retry a row without removing the reason in its last column.

| Area | Approach | Result | Why it lost | Commit, patch |
| --- | --- | --- | --- | --- |
| Effects | Offline interleaved alpha/color stamps plus exact mask-command overlap queries | Combat 77.2 → 75.7 ms; frame 649 93.3 → 95.4 ms | Exact queries and interleaved publication make the first affected frame slower | This commit, `effect-interleaved-commands-rejected.patch` |
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
