# Amiga gameplay frame-cost audit

Where A500 gameplay time goes, measured on the default build at `b815af63`
(2026-10-04), with every accepted optimization enabled. The previous audit, at
`b99c2f69`, is given alongside for comparison. Remaining work is ranked in
[open-work §5](open-work.md#5-meet-the-amiga-frame-budget); this document holds
the measurements and the approaches already ruled out.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO.
- Tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1800 gameplay updates. It covers
  natural descent with one damaging contact, view Y 39 to 1874, at most five
  allocated enemy records and final health 3. It has no shop, no dense
  encounter and no garden-to-well transition.
- FPS-only build `b8eb0341…`: 3,252 PAL fields (previously 3,821), 27.7 FPS.
  The worst gap is 8 fields at frame 701 (previously 9 at 645).
- [Instruction traces](amiga-profiling.md#cycle-exact-instruction-traces):
  build `d2a0d2c3…` captured 100 fields from each of frames 50, 250, 450,
  644, 690, 844, 900, 1100, 1300, 1500 and 1700 (602 complete frames).
  Frames 701 and 844 come from separate 30-field captures starting at 699 and
  843, because a capture's first frame is partial and is excluded. The
  previous audit's frame 844 was such a partial frame, so its 179.7 ms
  undercounts the frame.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=3000 \
  TRACE_FRAMES=50,250,450,644,690,844,900,1100,1300,1500,1700 \
  TRACE_OUTPUT=$PWD/tmp/trace/audit bash amiga/profile.sh trace
for start in 699 843; do
  SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=30 TRACE_FRAMES=$start \
    TRACE_OUTPUT=$PWD/tmp/trace/audit-$start VERIFY_TIMEOUT_SECONDS=900 \
    bash amiga/profile.sh trace
done
python3 tools/amiga-trace-report.py tmp/trace/audit --frames=1300-1338 --causes
python3 tools/amiga-trace-report.py tmp/trace/audit-699 --frames=701 --causes
```

## Frame cost by scene

The gameplay step overlaps the previous frame's publication wait, so traced
frames contain almost no idle time. One PAL field is 20 ms (142,102 cycles).

| Tape frames | Scene | Mean ms | Median | Worst | Over 20 ms | Previous mean |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 50–91 | entrance bowl, firing | 27.0 | 27.5 | 39.3 (54) | 41/41 | 29.9 |
| 250–303 | open descent | 21.4 | 18.0 | 69.4 (278) | 19/53 | 23.0 |
| 450–470 | enemies in view | 60.8 | 53.5 | 101.6 (464) | 20/20 | 64.3 |
| 644–661 | damaging contact | 96.7 | 97.6 | 121.4 (660) | 17/17 | 104.2 |
| 690–700 | combat with gems and smoke | 85.4 | 82.9 | 103.4 (694) | 10/10 | 88.8 |
| 701 | pattern generation burst | 172.2 | | | 1/1 | 184.5 |
| 844–864 | generation burst with a time field | 50.8 | 46.6 | 120.7 (844) | 21/21 | 89.4 |
| 900–920 | time fields present | 37.3 | 36.2 | 54.2 (907) | 20/20 | 66.6 |
| 1100–1737 | camera at the level bottom | 29.1 | 25.0 | 56.8 (1564) | 240/240 | 32.5 |

The contact frame (645) fell from 179.5 to 101.4 ms, and time-field frames
are about 30 ms cheaper. The generation burst barely moved, and combat and
quiet frames improved by only about 3 ms: none of the accepted work targeted
them.

## Cost by owner

`--causes` charges each sample to the innermost recognised owner on its
logical call stack; helpers are charged to their callers. Milliseconds per
frame:

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet 1300–1338 | Frame 645 | Frame 701 | Frame 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 8.4 | 6.3 | 21.5 | 28.3 | 13.4 | 12.3 | 30.7 | 31.5 | 22.6 |
| Actors, sprites and display | 3.7 | 2.4 | 8.9 | 22.0 | 6.5 | 2.1 | 15.9 | 21.9 | 7.5 |
| Generation logic | 0.3 | 1.3 | 1.0 | 1.6 | 0.5 | 0.5 | 1.3 | 82.6 | 51.6 |
| Runner bookkeeping | 6.1 | 2.7 | 12.9 | 10.2 | 4.5 | 5.1 | 12.9 | 13.8 | 17.3 |
| Software float emulation | 2.2 | 1.4 | 5.0 | 6.7 | 2.4 | 2.8 | 12.3 | 5.8 | 6.5 |
| Ammo HUD | 1.7 | 0.1 | 3.3 | 1.8 | 4.2 | 1.9 | 15.3 | 0.1 | 4.4 |
| Terrain rows and scrolling | 0.4 | 1.4 | 2.6 | 6.4 | 0.8 | 0.2 | 4.5 | 7.8 | 5.3 |
| Effect surface (currency/smoke) | 0.6 | 0.6 | 2.2 | 4.8 | 0.6 | 0.6 | 0.6 | 4.9 | 0.6 |
| Time fields, R-tree, frame loop | 3.5 | 5.2 | 3.4 | 3.6 | 4.4 | 3.0 | 8.0 | 3.8 | 5.0 |
| Total | 27.0 | 21.3 | 60.8 | 85.4 | 37.3 | 28.4 | 101.4 | 172.2 | 120.7 |

## The three worst frames

**Frame 701, pattern generation burst (172.2 ms, 8 fields).**
`generateLiveTerrain` takes 93.3 ms, almost all of it in
`AreaOneBuilder::emitPatternMode`. These costs nest; do not add them:
- 62 `AmigaAreaOneTerrainBuilder::collect` calls take 26.4 ms. Inside them,
  `AmigaGameplayGeneration::create` takes 22.4 ms (2,568 cycles each).
- 23 `collectRun` calls take 22.9 ms, of which `createRun` is 22.1 ms
  (6,828 cycles each). That includes 7.2 ms of
  `AmigaTerrainRunIndex::publishSpan` and 7.1 ms of `GameMakerRandom::advance`.
- 11 `AreaOneBuilder::emitObject` calls take 17.0 ms.
- Well512 work: 81 `nextU32` calls take 8.0 ms, `advance` 7.1 ms and 39
  `cavernEnemyChoice` calls 5.5 ms. The draws are part of the original
  interleaving and must stay synchronous.

The rest is ordinary combat work: actor draws 9.9 ms, currency/smoke 9.6,
row sync 6.2 and draw collection 5.6.

**Frame 844, generation burst with a time field (120.7 ms, 6 fields).**
- Generation takes 65.5 ms: 44 `create` calls cost 27.6 ms (4,460 cycles
  each), 14 `emitObject` calls 30.1 ms, and 13 `appendBreakables` 11.0 ms
  (6,034 cycles each, against a 2,000-cycle target).
- Draw collection takes 8.0 ms, `drawMainWorldSpan` 8.0 and terrain painting
  6.4.
- The prepared time field costs 2.4 ms to prepare and 2.0 ms to draw.

**Frame 645, damaging contact (101.4 ms, 5 fields).**
`GameplayWorld::step` takes 52.2 ms; enemy contacts are 10.1 ms of it and the
collision owner's attach lambdas about 15 ms. The ammo gauge takes 11.2 ms,
actor draws 11.2, draw collection 6.6, sprite assignment 4.7 and the health
HUD 4.0. Contact direction now costs under 1 ms.

## Typical frames

**Quiet, camera at the level bottom (28.4 ms; 1300–1338).**
- `GameplayWorld::step` takes 18.4 ms: walking enemies 2.8 (tile tests 1.9),
  the player 2.8, enemy alarms 1.2 and the collision owner's attach lambdas
  about 6.5 ms.
- Draw collection takes 2.7 ms, the ammo gauge 1.5 and sprite assignment 0.7.
- Integer division helpers take 1.7 ms of self time.

No function takes more than a tenth of the frame. Only removing the per-actor
walks can reach 20 ms, not tuning the functions in them.

**Combat with gems and smoke (85.4 ms; 690–700).**
- `GameplayWorld::step` takes 37.8 ms, currency collection 4.6 ms of it.
- Actor draws take 10.7 ms, currency/smoke composition 9.6 (three
  `stampPixel` calls at 14,038 cycles each), draw collection 6.6, row sync 6.3
  and effect-surface publication 2.6.
- The 32-bit division, modulo and multiply helpers take 10.2 ms of self time.
  `GemMotion::q8`, `rounded` and `pixel` cause about 4 ms of it, by
  converting fixed-point positions on every access.

**Enemies in view (60.8 ms; 450–470).** Game logic takes 21.5 ms and runner
bookkeeping 12.9.

**Time fields present (37.3 ms; 900–920).** Field preparation and drawing
take a few milliseconds. The rest of the frame is the ordinary game step plus
4.2 ms of gauge.

## Consequences for the budget

- Generation dominates both remaining burst frames: 93 ms at 701 and 66 ms at
  844. The targets are object creation, run creation with span publication,
  and pattern emission; the Well512 draws are fixed.
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
