# Amiga gameplay frame-cost audit

## Runnable 512 KiB regression baseline — 2026-10-10

`c64e8385`, cycle-exact A500/68000, PAL, 512 KiB Chip / 8 MiB Fast.
The complete contact tape takes 9,874 PAL fields for 1,800 updates (9.11 FPS),
with a maximum 43-field gap at frame 1689. Native coverage: ticks 0–1799,
view Y 39–2749 (maximum 2759), seven allocated enemy slots, two damage-number
births, final health 2 and no dead samples. Two immutable terrain preparation
operations take eight fields before takeover; the FPS verifier previously
expected one operation and has been corrected. This is not 50 FPS or a claim
of stock total-memory fit.

The old audit below is **not a matched gameplay workload**: source behavior
changes now let this same input controller descend farther. Do not subtract
old frame costs and call the difference overhead without checking coverage.

Seven 100-field captures start at 50, 450, 644, 700, 843, 1300 and 1688.
First/final partial frames are excluded. Means below use these exact complete
windows, not the number of frames that happens to fit after a later change.

| Frames | Mean ms | Worst ms (frame) |
| --- | ---: | ---: |
| 51–74 | 77.0 | 102.9 (54) |
| 451–464 | 134.9 | 372.8 (455) |
| 645–657 | 141.2 | 412.3 (649) |
| 701–713 | 134.9 | 173.7 (704) |
| 844–855 | 154.5 | 203.9 (854) |
| 1301–1314 | 131.5 | 178.3 (1314) |
| 1689–1696 | 227.9 | 873.0 (1689) |

Frame 1689 costs 873.0 ms by instruction trace, including 726.0 ms of terrain
generation: 132 birth calls, 462.6 ms in native terrain owner birth, 152.9 ms
in its spatial publication and 117.9 ms in room-layer birth (inclusive, not
additive). Actual terrain row painting is only a few milliseconds. Across
the 98 captured complete frames, actor End Step averages 32.0 ms, alarm dispatch
12.9 ms, and an effect-surface room traversal 6.7 ms. The latter runs even
without relevant effect producers. These are architectural work costs, not
evidence for tuning the blitter or compiler settings.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=360 \
  TRACE_FRAMES=50,450,644,700,843,1300,1688 TRACE_FIELDS=100 \
  TRACE_OUTPUT=$PWD/tmp/trace/baseline-c64e8385 bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/baseline-c64e8385 --causes
```

Trace ELF SHA-256:
`5de31a57fc74699c4a8b971244016241383a23ce0d100669ba3bf8559d5f7769`.
Compact reports and the FPS driver log are in `tmp/a500-baseline-before/`
(`full-trace-report.txt`, `worst1689-report.txt`, `full-contact-verified.log`).
The ELF and capture log are retained in `tmp/trace/baseline-c64e8385/`;
raw instruction captures are disposable after comparison.

### Current matched boundary — `2fcd7feb`

Four fresh 100-field captures start at 50, 675, 1300 and 1688 after the
scene-residency changes. The configuration remains 512 KiB Chip, 8 MiB Fast
and the emulator's default 512 KiB slow RAM, not stock total A500 memory.
Compare the same complete frame IDs, not every frame that fits in a capture:

| Frames | Mean ms | Worst ms (frame) |
| --- | ---: | ---: |
| 51–74 | 41.32 | 63.6 (54) |
| 676–683 | 151.69 | 605.1 (676) |
| 1301–1314 | 74.91 | 104.0 (1314) |
| 1689–1696 | 141.49 | 578.7 (1689) |

Quiet frame 51 is 32.5 ms; combat/field frame 1304 is 96.1 ms. Against the
accepted `768d4ce4` trace, frames 51/54/676/1304/1689 move by
0.0/+0.4/+0.3/0.0/−0.1 ms respectively: timing noise, not a speedup from
asset exclusion. Each capture restarts the same deterministic controller.
The last captured ticks are 98/689/1324/1707, view Y is 152/1504/2137/2598,
maximum enemy slots 0/5/6/7, and final health 4/3/3/3.
The complete 1,800-update contact tape takes 6,225 PAL fields (14.46 FPS),
maximum gap 35 fields at frame 1512. Full-run coverage remains ticks 0–1799,
view Y 39–2749 (maximum 2759), seven enemy slots, two damage-number births,
final health 2 and no dead samples. Log: `tmp/scene-local-baseline-fps.log`.

These are **inclusive function costs**, not additive totals. The owner/append
rows include scalar terrain births in the same frame as prepared segments:

| Boundary | Frame 676, ms | Frame 1689, ms |
| --- | ---: | ---: |
| Entire frame | 605.1 | 578.7 |
| `NativeTerrainLifetimes::appendConstructed` | 331.39 | 396.62 |
| Owner `nativeTerrainBirth`, inside append | 211.19 | 251.66 |
| Birth dirty-queue/family publication | 34.98 | 36.22 |
| Inherited layer publication | 27.60 | 33.68 |
| Registration publication | 22.56 | 26.93 |
| `createKnownIdentity`, after owner birth | 37.06 | 45.00 |
| Run identity/publication insertion | 36.80 | 43.18 |

Frames 676/1689 construct 111/132 terrain cells. Prepared construction alone
costs 372.50/441.61 ms. It already passes the born identity directly to Create;
there is no remaining ordinal lookup to remove. It still materializes and
publishes chronology, queues, membership and layer state separately for each
cell. Pixel row painting is not this cost. Per-cell registration redesign is
closed by the two rejected attempts below; a faster copy/flag helper would
also leave the construction pipeline intact.

At this baseline, `createPrepared` admits a callback-free terrain segment: source Create
observers and unsupported geometry use the scalar oracle. `constructRun`
still calls `appendConstructed` per cell. Its birth callback publishes default
bounds before the prepared Create plan dirties appearance. No game callback
or query observes the interior of this admitted segment, but index pressure,
IDs, reverse-prepend queue order and the completed failure prefix remain
observable. `publishRun` closes even a failed prefix. Whole-run construction
must preserve those boundaries, including reactivation and inactive layer
cutoffs; merely replacing physical attachment with per-cell logical lookups
does not eliminate the measured work.

The replacement boundary is `AmigaGameplayGeneration::createPrepared` →
`GameplayWorld::constructNativeTerrainRun` → the native lifetime/owner pair,
not the scalar registration container. A meaningful first run-construction
attempt targets at least 80 ms of each generation stall by removing repeated
layer-state materialization, constructor/Create staging and run-insertion
validation together. Registration and spatial journal order remain exact;
neither is an excuse for a new per-cell publication accumulator. Capacity or
unsupported resource/observer cases must retain the scalar failed-prefix
behavior, rather than changing a partial constructor failure into an atomic
batch rejection. The retained scalar ownership fixture is the oracle for
activation, depth moves, slot reuse, inactive cutoffs and teardown.

Reproduce with the contact options above and
`TRACE_FRAMES=50,675,1300,1688 TRACE_FIELDS=100`. Trace ELF SHA-256:
`8a758e4d5d3748600eaa2c500fd8e0d0c2cbdb7ea42df847be8b8bdd907c8189`.
Reports: `tmp/scene-local-baseline-report.txt` and
`tmp/scene-local-{676,1689}-report.txt`; retained ELF/capture metadata:
`tmp/trace/scene-local-baseline/`. Use `--frames=676 --causes --tree-min 1`
or `--frames=1689 --causes --tree-min 1` for the focused reports.

### Whole prepared terrain construction

The shipping prepared constructor now admits a complete source segment before
borrowing its existing dynamic layer and spatial journal. Default geometry and
Create results are prepared once per used terrain kind; chronological IDs,
registration links and journal entries retain their original per-cell order.
Run identity/publication insertion and retained-layer population are published
in spans. There is no generic first-cell constructor just to obtain a layer.
The spatial journal admits the complete envelope once and appends its ordered
geometry keys without repeating bounds admission/publication for every cell.
Unprepared families, missing layers, source observers, journal pressure and
unsupported geometry retain the scalar constructor. A terminal failure still
publishes only the completed prefix, not an atomic all-or-nothing batch.

Current four windows, with the cavern asset bank and single-admission journal:

| Frames | Mean ms | Worst ms (frame) |
| --- | ---: | ---: |
| 51–74 | 41.37 | 63.4 (54) |
| 676–683 | 134.09 | 465.9 (676) |
| 1301–1314 | 74.96 | 104.1 (1314) |
| 1689–1696 | 121.14 | 415.6 (1689) |

Against `2be39f73`, the generation stalls save 56.3/64.8 ms; no matched frame
is more than 0.3 ms slower. Independent 35-field captures confirm 465.9/415.8 ms.
Frame 51 remains 32.5 ms and frame 1304 is 96.2 ms. These are still
unacceptable gameplay stalls, not 50 FPS.
The 1,800-frame contact tape takes 6,250 PAL fields (14.40 FPS), versus 6,265
at `2be39f73`, with the same maximum gap of 34 fields at frame 1512. Coverage
remains ticks 0–1799, view Y 39–2749 (maximum 2759), seven enemy slots,
final health 2 and no dead samples. Gameplay art loads/prepares once.

Trace ELF SHA-256:
`05bebfd7e34eca667285774fbc607315e6b0e7e9551894c622644ee62962f3d4`.
Reports: `tmp/journal-span-fps-gdb.log`, `tmp/journal-span-report.txt` and
`tmp/journal-span-confirm-report.txt`. Reproduce using the preceding contact command
with `TRACE_FRAMES=50,675,1300,1688 TRACE_FIELDS=100`.

### Remaining early-game dispatch cost — `f4b4433a`

A fresh six-field capture starting at frame 50 confirms frame 51 at **32.56
ms** (frame 52: 32.9 ms). This is before enemy births: the capture ends at
tick 52, view Y 106, zero allocated enemy slots and health 4. It does not
establish later combat coverage. The historical comparable frame below was
19.2 ms; the early regression is therefore not resolved by the generation
or residency changes.

| Inclusive boundary, frame 51 | ms |
| --- | ---: |
| Entire gameplay Step | 22.21 |
| Actor End Step capture/dispatch/execution | 4.28 |
| Bullet Step and published-position update | 3.82 |
| Draw collection | 3.04 |
| Player movement | 2.70 |
| Room animation callback | 2.02 |
| Bullet position publication, inside Bullet Step | 1.81 |
| Dither Above Begin Step | 1.69 |
| Captured alarm dispatch | 1.48 |
| Bullet retirement, inside Actor End Step | 0.81 |

Parent/child rows overlap. In particular, another arithmetic or bullet
retirement helper change cannot remove the remaining dispatch cost. The
retained generic End Step adapter captures lifetimes before callbacks,
checks live eligibility afterwards and preserves player/box/actor order.
The player's event updates `playerHit`; out-of-view destruction can run
callbacks and reread published positions. Absence of enemies alone is not
a certificate that this phase is empty. Any phase-elision replacement must
account for these effects at the owning lifecycle boundary rather than
introducing another full-room proof scan.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512
VERIFY_TIMEOUT_SECONDS=90 STRIP=1 TRACE_FIELDS=6 TRACE_FRAMES=50
TRACE_OUTPUT=$PWD/tmp/trace/early-current bash amiga/profile.sh trace`, then
`python3 tools/amiga-trace-report.py tmp/trace/early-current --frames=51
--causes --tree-min 1`. Report: `tmp/early-current-frame51.txt`; driver log:
`tmp/early-current-trace.log`. Trace ELF SHA-256:
`c1c2d1c92f5ede0b1fb4f82b9c5a43e597896c86aeb04ef4b4826eee20438d45`.

## Historical pre-integration audit

### Early-game regression comparison

The saved `13e08ca2` trace ELF and `3e0a3080`'s implementation have identical
80 consecutive observable-state samples (ticks 0–79): player fixed-point
position/velocity, camera pixels, health, ammo, enemy count, bullet-slot extent
and RNG draw count. Both run with 512 KiB Chip, 8 MiB Fast and the default
512 KiB slow RAM. This is evidence for comparing early workload costs, not a
full state digest or proof that later gameplay is equivalent.

`amiga/profile_early_state.gdb` collects these samples from the per-frame
trace marker. With the matching saved trace ELF and stripped Hunk installed,
run `VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=60
GDB_SOURCE=profile_early_state.gdb READY_MARKER=DOWNWELL_PROFILE_READY bash
amiga/verify_startup.sh`. Preserve `.run/gdb-startup.log` after each run and
compare its `EARLY` lines. The debugger stops invalidate timing; instruction
traces from separate uninterrupted captures supply the costs below.

Frame 51 is **19.2 ms historically versus 56.1 ms currently**. The current
56.11 ms trace attributes 45.97 ms to gameplay Step, including these inclusive
costs (parent and child rows must not be added):

| Work | ms |
| --- | ---: |
| Actor End Step dispatch and execution | 18.73 |
| Bullet End Step view retirement, inside that dispatch | 12.15 |
| Alarm dispatch | 9.65 |
| Bullet Step effects and publication | 3.91 |
| Room animation callback | 2.74 |
| Player movement | 2.70 |

Bullet retirement repeatedly resolves the same published lifetime and
recomputes view thresholds using binary64 operations. Its source-ordered
destruction callbacks and subsequent rereads are intentional semantics;
removing them without proving the admitted callback behavior is not a valid
optimization. Alarm dispatch repeatedly traverses captured object groups,
including absent-event groups. These are measured early-game costs even
before any enemies have spawned, not consequences of the deeper later route.

Evidence: `tmp/early-baseline-{old,current}/state.log`, current frame report
`tmp/early-baseline-current/frame51.txt`, historical window report in
`tmp/prof/audit-13e08ca2/`. Historical ELF SHA-256 is
`26bb4f25411a7d2344342f50447ac567f286e03d26bb44cefc643ab7e06a6e8f`;
current is `3aa1e229376680c1ada7d5c084133fc0a5950f4062e4ab87aeafae534280eaac`.

### Full historical workload

Default A500 build at `13e08ca2` (2026-10-08), compared with the supplied
`36d8a0f2` audit. Remaining work is ranked in
[open-work §5](open-work.md#5-meet-the-amiga-frame-budget). This is a snapshot
cost audit, not a claim that the 20 ms gameplay budget has been met. Subsequent
matched measurements and post-group audits are recorded in their commits.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO.
- Contact tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1,800 gameplay updates,
  view Y 39 to 1874, at most five allocated enemy records, final health 3.
  No shop, dense encounter or garden-to-well transition.
- FPS-only result: 2,800 PAL fields, 32.14 FPS,
  maximum five fields at frame 701. Supplied baseline: 2,950 fields,
  30.51 FPS, maximum six at 701. Quantized gaps are report-only.
- Natural two-level shop tape: 3,081 updates,
  5,586 fields, 27.58 FPS; maximum 31 fields at the room transition at 2411.
  Source-state/RNG and resident HUD verification pass. This transition is
  not a gameplay-frame budget measurement.
- Eleven 100-field instruction captures start at 50, 250, 450, 644, 690,
  843, 900, 1100, 1300, 1500 and 1700. First/final partial frames are excluded.
  Use the fixed ranges below, not every frame fitting in a capture.
- Trace ELF SHA-256:
  `26bb4f25411a7d2344342f50447ac567f286e03d26bb44cefc643ab7e06a6e8f`.
  Reports/folded stacks are in `tmp/prof/audit-13e08ca2/`; the ELF and capture
  log are in `tmp/trace/audit-13e08ca2/`. Raw captures are disposable.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=600 \
  TRACE_FRAMES=50,250,450,644,690,843,900,1100,1300,1500,1700 \
  TRACE_OUTPUT=$PWD/tmp/trace/audit-13e08ca2 bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/audit-13e08ca2 --frames=1301-1338 --causes --collapsed tmp/quiet.folded
python3 tools/amiga-trace-report.py tmp/trace/audit-13e08ca2 --frames=701 --causes --collapsed tmp/burst.folded
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
| 51–91 | entrance bowl, firing | 24.4 | 24.5 | 34.3 (54) | 30/41 | 23.9 |
| 251–303 | open descent | 20.6 | 17.7 | 46.8 (278) | 19/53 | 20.6 |
| 451–470 | enemies in view | 44.5 | 39.9 | 65.4 (463) | 20/20 | 53.6 |
| 645–661 | damaging contact | 59.5 | 53.1 | 86.6 (645) | 17/17 | 80.1 |
| 691–700 | combat with gems | 53.0 | 51.5 | 65.6 (694) | 10/10 | 67.3 |
| 701 | pattern generation burst | 91.1 | | | 1/1 | 104.2 |
| 844–864 | generation with a time field | 46.0 | 42.4 | 86.6 (844) | 21/21 | 45.7 |
| 901–920 | time fields present | 36.1 | 35.2 | 53.3 (907) | 20/20 | 35.7 |
| 1301–1338 | camera at level bottom | 26.7 | 23.5 | 37.9 (1304) | 38/38 | 26.3 |

Additional matched bottom windows 1101–1169, 1501–1567 and 1701–1737 average
27.0, 27.9 and 28.4 ms. Budget frames 645/646/660/701/844 measure
86.6/76.5/51.9/91.1/86.6 ms. Frame 488 measures 53.6 ms.

## Cost by owner

`--causes` charges samples to the innermost recognised function-name owner;
signature types do not determine ownership. Helpers are charged to their
callers. Values are rounded ms/frame; this is not a sum of inclusive functions.

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet | 645 | 646 | 660 | 701 | 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 9.0 | 6.9 | 21.3 | 23.0 | 14.2 | 13.0 | 30.4 | 30.0 | 24.4 | 24.1 | 25.0 |
| Actors, sprites and display | 3.4 | 2.4 | 4.0 | 9.6 | 6.4 | 2.1 | 12.3 | 8.6 | 10.0 | 10.4 | 7.4 |
| Generation logic | 0.2 | 1.1 | 0.4 | 0.3 | 0.4 | 0.4 | 0.4 | 0.4 | 0.2 | 33.6 | 24.5 |
| Runner bookkeeping | 4.2 | 1.8 | 7.2 | 6.6 | 3.0 | 3.7 | 7.6 | 7.9 | 4.6 | 7.4 | 10.5 |
| Software float emulation | 1.5 | 1.2 | 3.7 | 5.5 | 2.0 | 1.9 | 7.4 | 7.6 | 4.9 | 5.3 | 3.0 |
| HUD redraws | 1.7 | 0.1 | 3.3 | 1.7 | 4.3 | 1.9 | 19.9 | 13.7 | 4.1 | 0.1 | 4.5 |
| Terrain rows and scrolling | 0.5 | 1.4 | 1.3 | 3.0 | 0.8 | 0.2 | 4.5 | 4.2 | 0.7 | 6.7 | 6.4 |
| Effect surface | 0.6 | 0.6 | 0.5 | 0.2 | 0.6 | 0.6 | 0.6 | 0.6 | 0.3 | 0.2 | 0.6 |
| Time fields, R-tree, frame loop | 3.3 | 5.1 | 2.9 | 3.2 | 4.4 | 2.9 | 3.5 | 3.5 | 2.8 | 3.3 | 4.5 |
| Total | 24.4 | 20.6 | 44.5 | 53.0 | 36.1 | 26.7 | 86.6 | 76.5 | 51.9 | 91.1 | 86.6 |

## Remaining threshold-sized structures

### Natural raster fallback baseline

Seed 8, encounter route 4 at `e5c6ed59`, selected through
`SKIP_TO_GAME=1 PROFILE_OVERFLOW=1`, supplies an additional workload; it is not
comparable to the contact tape's FPS. Its 1,800 updates take 4,059 PAL fields
(22.17 FPS), with a maximum seven-field gap at 1352. The source-backed host
census identifies shade subtraction in 130 frames and nine-channel overflow
at 1397–1403; it has no smoke/debris. Native full-state and fallback pixel
verification remain required before accepting a rendering change.

Two 60-field instruction captures start at 1302 and 1396. The parser validates
both captures; the following windows contain only complete frames:

| Window | Mean frame ms | Worst ms (frame) | Composition | Publication | Promotion attempt | Row sync |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1303–1312, shade | 78.8 | 91.5 (1304) | 8.13 | 3.88 | 3.07 | 2.41 |
| 1397–1403, nine gems | 82.6 | 90.4 (1400) | 19.66 | 8.56 | 4.10 | 5.24 |

These are inclusive costs; publication includes its blitter work. Nine-gem
composition contains nine `stampPixel` calls costing 14.24 ms together.
At this baseline promotion rejects the entire currency surface on channel exhaustion,
so all nine gems are composed and published through the raster. Keeping a
source-topmost sprite suffix and rasterizing only the remainder could remove
eight of those stamps (about 12.7 ms before added partitioning costs), plus
some publication. The baseline assignment layer treats the whole controller
surface as being above every actor: partial promotion must explicitly preserve
that boundary for ordinary actors while placing the promoted suffix above it.
Shade subtraction, opaque black and sprite priority remain authoritative.

Reproduce with the overflow trace command in [profiling](amiga-profiling.md),
then `amiga-trace-report.py --frames=1303-1312` and `--frames=1397-1403`.
ELF SHA-256: `5e0befc89c52e2005ce88d5468317cedbf3b4607b1eeacc7deddd120a8b1bc36`.
Compact reports and folded stacks: `tmp/overflow-{shade,channels}-report.txt`
and `tmp/overflow-{shade,channels}.folded`; capture log and ELF are under
`tmp/trace/overflow-e5c6ed59/`. Raw captures are disposable.

### Contact tape structures

Inclusive costs below nest: do not add parent and child rows.

| Structure | Frame 701 | Frame 844 |
| --- | ---: | ---: |
| Generation | 40.84 | 35.86 |
| Prepared-pattern dispatch | 35.23 | 29.92 |
| Event cells | 16.92 | 19.81 |
| Prepared terrain publication | 13.92 | 6.87 |
| Draw collection | 4.35 | 3.65 |

At 844, thirteen native breakable creations cost 3.77 ms (about 2,058 cycles
each). One enemy configuration costs 3.76 ms, including 3.23 ms in the
lifecycle publication callback. Its individual membership, native-table and
lifetime helpers are not independent threshold-sized targets. A further
generation attempt must remove shared ownership machinery as a whole, not
retune prepared row encodings, RNG helpers or constructors.

At 645, contact damage-number creation costs 5.29 ms. At 646, its first Step
costs 5.56 ms, including 3.31 ms in the certified contact-motion adapter;
2.31 ms is inside the decoder. The old 23.47 ms cosine evaluation no longer
runs on this admitted contact. The pair still costs about 10.85 ms, so the
remaining proposal must eliminate a whole computation rather than tune
sub-millisecond decoder/arithmetic helpers. Later Step costs about 2.14 ms.

Ammo redraw is still 11.29 ms at 645 and 10.71 ms at 646; health is 4.51 ms
at 645. Those remain parked as specified by §5.

Combat draw collection costs 4.48 ms, including artwork/command consumers,
not merely the room traversal. Row synchronization averages 1.50 ms.
Hardware promotion removes the old raster-gem composition workload; further
mixed-effect work requires a tape that actually exercises the fallback.

Quiet `GameplayWorld::step` remains 17.29 ms. Walking-enemy motion and player
motion cost 2.79/2.73 ms individually. Draw collection is 1.94 ms, ammo redraw
1.48, captured alarm dispatch 1.09 and reconciliation 0.39. A quiet-frame
proposal needs a shared structure across callers; these figures do not
justify tuning unrelated small helpers.

The current audit therefore still identifies generation publication, combat
draw-command production and the contact computations as outstanding
structures. It does not establish dense-effect coverage or close §5. The
supplied audit and historical measurements remain available in Git.

## Rejected approaches

Each row was measured on its affected workload against its reference and removed.
Each patch, in [`rejected/`](rejected/), holds the removed code; the commit
named holds the numbers. Do not retry a row without removing the reason in its last column.

| Area | Approach | Result | Why it lost | Commit, patch |
| --- | --- | --- | --- | --- |
| End Step capture | One filtered traversal of the existing actor projection instead of five registration-range traversals | Against 872e8229: matched window means 41.39/141.15/74.97/129.26 → 41.04/140.64/74.38/128.51 ms; 676 522.0 → 521.7, 1689 480.4 → 479.6 ms | Saves only 0.35–0.75 ms per window against a 3 ms target; retains the expensive event/state work. Host ordering and 18,000-frame ownership checks pass, but this is not the required structural saving | This row's introducing commit; removed, no retained implementation |
| Native terrain registration | Run-owned registry plus a separate per-cell span accumulator | 676 604.8 → 599.6 ms; 1689 578.8 → 572.2; +23,804 BSS bytes and +12,676 Hunk bytes | Misses the estimated 15–20 ms saving: 26.9 ms registration becomes about 7.2 ms of accumulation plus 11.6 ms of flushing, with remaining lookup costs. Construct and publish runs directly instead of adding another per-cell publication accumulator | This row's introducing commit, [`terrain-registration-spans.patch`](rejected/terrain-registration-spans.patch) |
| Native terrain registration | Borrow run identities/stamps, actor-only registry and exceptional reactivation tail | 676 604.8 → 636.5 ms; 1689 578.8 → 614.0 (independent confirmation 636.8/613.7); −27,876 BSS bytes, −13,684 stripped Hunk bytes | Repeated borrowed identity resolution through the per-cell registration API raises registration cost 26.9 → 50.7 ms; all four matched window means worsen. Second rejected registration attempt; no further variants | This row's introducing commit, [`terrain-registration-borrowed.patch`](rejected/terrain-registration-borrowed.patch) |
| Alarm dispatch | Retain captured object-group ends and alarm masks | 51 45.3 → 44.4 ms; 676 674.4 → 673.2; 1689 661.7 → 659.6 | Only 0.9 ms in the targeted early frame versus 3–4 ms expected, with roughly 1.5 KiB extra ordinary storage; retains the per-column/per-instance dispatch structure | This row's introducing commit, [`captured-alarm-groups.patch`](rejected/captured-alarm-groups.patch) |
| Native terrain layers | Run-owned logical layer state replacing physical per-cell elements | 676 747.4 → 776.6 ms (independent repeat 776.6); 1689 751.7 → 780.4 ms | Layer birth still costs 86.4 ms versus 95.4; per-cell state lookup/publication replaces much of the removed attachment cost, while larger publication records add work. Draw savings do not offset generation stalls | This row's introducing commit, [`native-terrain-logical-layers.patch`](rejected/native-terrain-logical-layers.patch) |
| Cold ammo prepass | Remove static gauge artwork/number before retained gauge publication | Artwork 6.45 → 0.11 ms; gauge publication 23.70 → 27.88 ms; combined handoff 300.56 → 300.40 ms | The prepass seeds final words, reducing later CPU writes; its removal redistributes work/waiting without a meaningful handoff saving | This row's introducing commit, [`initial-ammo-prepass.patch`](rejected/initial-ammo-prepass.patch) |
| Effects | Offline exact full-mask gem/shade overlap bits (13,356 bytes) | Shade promotion 3.04 → 2.24 ms; shade window 79.0 → 78.4 ms; nine-gem window 70.2 → 71.0 ms | Viewport-clipped gems retain most of the scan; only 0.6 ms total saving, while the expanded promotion path costs more in the nine-gem window | This row's introducing commit, [`shade-overlap-table.patch`](rejected/shade-overlap-table.patch) |
| Cold HUD backdrop | Host-compose margins, both borders and ammo art; replace HUD clearing and layered entry draws with one full planar publication | Handoff 361.011 → 361.013 ms; publication 39.096 ms; +27,264 ordinary-memory bytes | Full-plane publication consumes all the saved composition/clear time; no meaningful saving, not a transition-frame gate failure | This row's introducing commit, [`hud-offline-full-backdrop.patch`](rejected/hud-offline-full-backdrop.patch) |
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
