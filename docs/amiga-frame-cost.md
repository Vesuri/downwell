# Amiga gameplay frame-cost audit

Default A500 build at `7deed269` (2026-10-08), compared with the supplied
`36d8a0f2` audit. Remaining work is ranked in
[open-work §5](open-work.md#5-meet-the-amiga-frame-budget). This is a current
cost audit, not a claim that the 20 ms gameplay budget has been met.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO.
- Contact tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1,800 gameplay updates,
  view Y 39 to 1874, at most five allocated enemy records, final health 3.
  No shop, dense encounter or garden-to-well transition.
- FPS-only result recorded in `7deed269`: 2,792 PAL fields, 32.23 FPS,
  maximum five fields at frame 645. Supplied baseline: 2,950 fields,
  30.51 FPS, maximum six at 701. Quantized gaps are report-only.
- Natural two-level shop tape, also recorded in `7deed269`: 3,081 updates,
  5,669 fields, 27.17 FPS; maximum 34 fields at the room transition at 2411.
  Source-state/RNG and resident HUD verification pass. This transition is
  not a gameplay-frame budget measurement.
- Eleven 100-field instruction captures start at 50, 250, 450, 644, 690,
  843, 900, 1100, 1300, 1500 and 1700. First/final partial frames are excluded.
  Use the fixed ranges below, not every frame fitting in a capture.
- Trace ELF SHA-256:
  `5102b2995074cddd6ef69d5b88b3338b55820cf6c9c742a129734f35bbf1bf95`.
  Reports/folded stacks are in `tmp/prof/audit-7deed269/`; the ELF and capture
  log are in `tmp/trace/audit-7deed269/`. Raw captures are disposable.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=600 \
  TRACE_FRAMES=50,250,450,644,690,843,900,1100,1300,1500,1700 \
  TRACE_OUTPUT=$PWD/tmp/trace/audit-7deed269 bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/audit-7deed269 --frames=1301-1338 --causes --collapsed tmp/quiet.folded
python3 tools/amiga-trace-report.py tmp/trace/audit-7deed269 --frames=701 --causes --collapsed tmp/burst.folded
SKIP_TO_GAME=1 VERIFY_TIMEOUT_SECONDS=600 bash amiga/profile.sh shop
```

The effect census established in `516fae0b` counts 172 hardware-gem frames,
1,628 empty frames and zero raster fallback, smoke, debris, entry-cover or
overflow frames on this contact tape. “Combat with gems and smoke” in the old
audit was an incorrect coverage label. Mixed-effect performance remains unproven.

## Frame cost by scene

Totals include publication waiting, which can move between frames as computation
changes. One PAL field is 20 ms (142,102 cycles). Attribute redistributed waiting
separately; judge scene handoffs by combined latency and visible stalls.

| Tape frames | Scene | Mean ms | Median | Worst | Over 20 ms | Baseline mean |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 51–91 | entrance bowl, firing | 23.9 | 22.3 | 35.0 (64) | 28/41 | 23.9 |
| 251–303 | open descent | 20.6 | 17.9 | 46.5 (278) | 19/53 | 20.6 |
| 451–470 | enemies in view | 44.5 | 39.6 | 65.8 (463) | 20/20 | 53.6 |
| 645–661 | damaging contact | 60.2 | 54.0 | 89.6 (645) | 17/17 | 80.1 |
| 691–700 | combat with gems | 53.1 | 51.3 | 66.4 (694) | 10/10 | 67.3 |
| 701 | pattern generation burst | 91.1 | | | 1/1 | 104.2 |
| 844–864 | generation with a time field | 45.9 | 42.0 | 87.0 (844) | 21/21 | 45.7 |
| 901–920 | time fields present | 35.8 | 34.9 | 53.0 (907) | 20/20 | 35.7 |
| 1301–1338 | camera at level bottom | 26.5 | 22.9 | 38.3 (1304) | 38/38 | 26.3 |

Additional matched bottom windows 1101–1169, 1501–1567 and 1701–1737 average
26.9, 27.8 and 28.4 ms. Budget frames 645/646/660/701/844 measure
89.6/76.4/53.9/91.1/87.0 ms. These agree with the previous accepted change's
short captures to within 0.2 ms. Frame 488 measures 54.8 ms.

## Cost by owner

`--causes` charges samples to the innermost recognised function-name owner;
signature types do not determine ownership. Helpers are charged to their
callers. Values are rounded ms/frame; this is not a sum of inclusive functions.

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet | 645 | 646 | 660 | 701 | 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 9.1 | 6.9 | 21.3 | 23.1 | 14.2 | 13.0 | 33.8 | 30.1 | 24.7 | 23.9 | 25.5 |
| Actors, sprites and display | 3.3 | 2.4 | 4.0 | 9.6 | 6.3 | 2.1 | 12.2 | 8.5 | 10.3 | 10.4 | 7.3 |
| Generation logic | 0.2 | 1.1 | 0.4 | 0.3 | 0.4 | 0.4 | 0.4 | 0.4 | 0.2 | 33.6 | 24.3 |
| Runner bookkeeping | 4.2 | 1.8 | 7.2 | 6.6 | 3.0 | 3.7 | 7.9 | 7.6 | 4.6 | 7.3 | 10.7 |
| Software float emulation | 1.5 | 1.2 | 3.7 | 5.4 | 2.0 | 1.9 | 7.3 | 7.7 | 4.9 | 5.3 | 3.0 |
| HUD redraws | 1.7 | 0.1 | 3.4 | 1.7 | 4.3 | 1.9 | 19.3 | 13.7 | 5.5 | 0.1 | 4.5 |
| Terrain rows and scrolling | 0.5 | 1.4 | 1.3 | 3.0 | 0.8 | 0.2 | 4.6 | 4.2 | 0.7 | 7.0 | 6.4 |
| Effect surface | 0.6 | 0.6 | 0.5 | 0.2 | 0.6 | 0.6 | 0.6 | 0.6 | 0.3 | 0.2 | 0.6 |
| Time fields, R-tree, frame loop | 2.9 | 5.2 | 2.9 | 3.2 | 4.1 | 2.8 | 3.6 | 3.6 | 2.7 | 3.3 | 4.6 |
| Total | 23.9 | 20.6 | 44.5 | 53.1 | 35.8 | 26.5 | 89.6 | 76.4 | 53.9 | 91.1 | 87.0 |

## Remaining threshold-sized structures

Inclusive costs below nest: do not add parent and child rows.

| Structure | Frame 701 | Frame 844 |
| --- | ---: | ---: |
| Generation | 40.84 | 35.62 |
| Prepared-pattern dispatch | 35.23 | 29.92 |
| Event cells | 17.00 | 19.81 |
| Prepared terrain publication | 13.92 | 6.87 |
| Draw collection | 4.26 | 3.87 |

At 844, thirteen native breakable creations cost 3.77 ms (about 2,058 cycles
each). One enemy configuration costs 3.76 ms, including 3.23 ms in the
lifecycle publication callback. Its individual membership, native-table and
lifetime helpers are not independent threshold-sized targets. A further
generation attempt must remove shared ownership machinery as a whole, not
retune prepared row encodings, RNG helpers or constructors.

At 645, contact damage-number creation costs 5.29 ms. At 646, its first Step
costs 5.55 ms, including 3.39 ms in the certified contact-motion adapter;
2.39 ms is inside the decoder. The old 23.47 ms cosine evaluation no longer
runs on this admitted contact. The pair still costs about 10.84 ms, so the
remaining proposal must eliminate a whole computation rather than tune
sub-millisecond decoder/arithmetic helpers. Later Step costs about 2.14 ms.

At 660, counter/streak redraws cost 2.48/3.03 ms; at 488, 2.47/3.48 ms.
Their combined under-5-ms target is not met. Old 22-ms HUD text and 14-ms
raster currency costs are no longer a valid basis for the next change.
Ammo redraw is still 11.21 ms at 645 and 10.71 ms at 646; health is 3.98 ms
at 645. Those remain parked as specified by §5.

Combat draw collection costs 4.48 ms, including artwork/command consumers,
not merely the room traversal. Row synchronization averages 1.50 ms.
Hardware promotion removes the old raster-gem composition workload; further
mixed-effect work requires a tape that actually exercises the fallback.

Quiet `GameplayWorld::step` remains 17.30 ms. Walking-enemy motion and player
motion cost 2.78/2.77 ms individually. Draw collection is 1.92 ms, ammo redraw
1.48, captured alarm dispatch 1.09 and reconciliation 0.39. A quiet-frame
proposal needs a shared structure across callers; these figures do not
justify tuning unrelated small helpers.

The current audit therefore still identifies generation publication, combat
draw-command production and the contact/HUD computations as outstanding
structures. It does not establish dense-effect coverage or close §5. The
supplied audit and historical measurements remain available in Git.

## Rejected approaches

Each row was measured on the contact tape against its reference and removed.
Each patch, in [`rejected/`](rejected/), holds the removed code; the commit
named holds the numbers. Do not retry a row without removing the reason in its last column.

| Area | Approach | Result | Why it lost | Commit, patch |
| --- | --- | --- | --- | --- |
| Reentry reset | Clear occupancy only inside mutation-owned row extents | Reset 74.4 → 47.1 ms; room loading 46.6 → 65.6 ms; handoff 963.2 → 954.8 ms | Misses 40 ms target; extent maintenance consumes 19 ms and the following frame grows 57.0 → 65.4 ms; combined time unchanged | This row's introducing commit, [`terrain-reset-extents.patch`](rejected/terrain-reset-extents.patch) |
| Cold entry | Decode initial raster in bounded clean-style updates | Confirmation 639.5 → 635.1 ms; raster phase still 180.5 ms | Only 1,008 of 2,160 rows finish, so entry repeats the full copy; misses the 150 ms target and first gameplay rises 38.5 → 43.4 ms | This row's introducing commit, [`initial-raster-style-staging.patch`](rejected/initial-raster-style-staging.patch) |
| Alarms | Borrow enemy Step snapshot and merge typed alarm dispatch with remaining families | 645 93.9 → 92.3 ms; quiet 26.326 → 26.392 ms | Quiet alarm work rises 1.10 → 1.35 ms; borrowing adds capture/revision and merge work even with inactive counters, violating the window-mean gate | This row's introducing commit, [`enemy-borrowed-alarms.patch`](rejected/enemy-borrowed-alarms.patch) |
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
