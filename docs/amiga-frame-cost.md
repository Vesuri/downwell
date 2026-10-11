# Amiga gameplay frame-cost audit

## Scene-admitted wall ownership

The external collision backend no longer maintains generic wall registration:
no admitted consumer reads it. Native lifetime, chronology, activation and
room/layer ownership remain unchanged. Object 93 retains its existing indexed
End Step registration; full-service scenes retain every registration. Generic
whole-scene dispatch and terrain population consumers fail closed rather than
silently observing a partial registry. No replacement registry is allocated.

Matched contact timings against `759fb458`, 512 KiB Chip / 8 MiB Fast:

| Frame / work | Before ms | After ms |
| --- | ---: | ---: |
| 51, early gameplay | 26.4 | 26.4 |
| 676, generation | 287.1 | 260.6 |
| 1047, thin-platform retirement | 118.2 | 118.0 |
| 1305, combat | 63.3 | 63.3 |
| 1512, generation | 153.7 | 146.2 |
| 1515, gameplay | 72.8 | 72.9 |
| 1535, thin-platform retirement | 82.4 | 82.5 |
| 1689, generation | 275.5 | 245.2 |

The complete 1,800-update tape takes 5,079 PAL fields, **17.72 FPS**
(previously 5,083 / 17.71 FPS). This removes generation stalls, not the ordinary
frame-budget shortfall. The maximum gap is 13 fields at frame 676; it is not
the acceptance gate. Ten host tapes / 18,000 frames compare native terrain
lifetimes and pre-cleanup retirement marks with full-service ownership.
Cross-target replay passes. Targeted activation/re-entry tests retain inactive
platforms and retire them on reactivation; undeclared consumers are rejected.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512
VERIFY_TIMEOUT_SECONDS=180 STRIP=1 TRACE_FRAMES=50,675,1046,1304,1511,1534,1688
TRACE_FIELDS=35 TRACE_OUTPUT=$PWD/tmp/trace/wall-admission-contact
bash amiga/profile.sh trace`, then `python3 tools/amiga-trace-report.py
tmp/trace/wall-admission-contact --causes`. Run the same settings with
`bash amiga/profile.sh fps` for the uninterrupted tape.
Reports: `tmp/wall-admission-report.txt`, `tmp/wall-admission-fps.log`,
`tmp/wall-admission-replay.log`, `tmp/wall-admission-host.log`.
Trace ELF SHA-256:
`e422323a4777996700adf399606d40c28758c8f01acf01e4bb329d95c7894ff1`.

No-input generation frame 163 measures **172.8 ms**; reproduce with
`PROFILE_IDLE=1 SKIP_TO_GAME=1 PROFILE_CONTACT=0 PROFILE_FRAMES=600
VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=120 STRIP=1 TRACE_FRAMES=162
TRACE_FIELDS=35 TRACE_OUTPUT=$PWD/tmp/trace/wall-admission-idle
bash amiga/profile.sh trace`. Report: `tmp/wall-admission-idle-report.txt`;
ELF SHA-256 `39559513fdfe457ff827ba3d7fb03be33eb4127d733f853ca94fc0d2b7b45b0d`.
This is an absolute measurement, not a matched-current-baseline saving.
Normal `STRIP=1` startup passes `verify_memory.gdb` with 126,840 bytes
Chip remaining, no gameplay preparation or excluded reference allocations.
The restored normal executable is 1,995,944 bytes; its ELF SHA-256 is
`4cd50f7ef0e6492073cc1b72bfce71e5502e699b0402c777c2c6279bd6731b5e`.
Startup evidence: `tmp/wall-admission-memory.log` and
`tmp/wall-admission-memory-gdb.log`.

## Prepared-publication boundary at `759fb458`

A fresh 35-PAL-field capture beginning at contact marker 675 reproduces
frame 676 at **286.81 ms** (2,037,790 cycles). Its following complete frames
677–680 take 76.2, 67.3, 75.6 and 74.9 ms. Coverage reaches tick 680,
view Y 1496, five enemy slots and health 3, with 512 KiB Chip and 8 MiB Fast.
This is attribution, not an optimization or restored frame-budget claim.

| Inclusive boundary in frame 676 | ms | Relationship |
| --- | ---: | --- |
| Entire gameplay Step | 257.45 | Contains generation and ordinary simulation |
| Live generation | 211.74 | Parent of the following generation work |
| Prepared terrain publication, 33 calls | 141.81 | Includes native ownership and backend occupancy |
| Native prepared construction | 124.35 | Inside prepared publication |
| Prepared cell lifetime insertion | 101.46 | Inside native construction |
| Collision-owner span construction | 72.42 | Inside lifetime insertion |
| External-object registration | 21.95 | Inside span construction; existing rejected lane |
| External birth chronology | 9.45 | Inside span construction |
| Layer span publication / admission | 7.49 / 5.62 | Inside span construction |
| Identity/run insertion | 8.23 | Inside lifetime insertion, outside span construction |
| End-of-span room publication | 8.14 | Inside native construction, outside lifetime insertion |
| Event cells, 34 calls | 48.00 | Separate from prepared publication |
| Breakable births, 17 calls | 14.33 | Inside event cells |
| Builder row advancement | 11.40 | Separate from prepared publication and event cells |
| Builder position publication | 10.90 | Inside row advancement |
| Draw collection | 7.8 | Outside gameplay Step |

Do not sum parent and child rows. These costs do not justify another
registration encoding, individual setter rewrite, or sub-millisecond Draw
helper experiment. The current all-window report attributes about 4.12 ms
to Draw collection, including actual command production; its prepared-order
traversal is only 1.01 ms. A replacement of prepared publication must cover
the complete lifetime/layer/room transaction, preserve event-Create barriers
and successful failure prefixes, and remove work rather than defer a second
per-cell reconstruction. RNG consumption remains interleaved with event Creates.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512
VERIFY_TIMEOUT_SECONDS=180 STRIP=1 TRACE_FRAMES=675 TRACE_FIELDS=35
TRACE_OUTPUT=$PWD/tmp/trace/generation-owner-audit bash amiga/profile.sh trace`,
then `python3 tools/amiga-trace-report.py tmp/trace/generation-owner-audit
--frames=676 --top=140 --tree-min=0.15 --causes`.
Report: `tmp/generation-owner-676.txt`; driver: `tmp/generation-owner-audit.log`.
Trace ELF SHA-256:
`ded490a6892d5b12839df26d4cd7055138e3e392d273e5c50fb26ea3f5ea0f7a`.

## External collision-backend execution boundary

The guarded native backend omits generic actor/terrain collision geometry,
index membership, collision-queue maintenance and unobserved box publication
mirrors; authoritative native records retain position and artwork. It retains
family preparation, registration and event order. The source/full-service path
remains available to collision-enabled scenes. This is not another queue encoding.

| Budget frame | Retained maintenance, ms | External backend, ms |
| --- | ---: | ---: |
| Contact 51 | 31.8 | 31.2 |
| Contact 676 | 396.9 | 331.6 |
| Contact 1512 | 201.5 | 175.4 |
| Contact 1689 | 386.9 | 320.1 |
| No-input generation 163 | 245.63 | 208.9 |

Matched contact-window means (starting 51/676/1512/1689, respectively) are
38.029→37.024, 184.233→161.800, 75.312→71.237 and 148.425→130.725 ms.
Matched worst frames are 55.6→52.6, 396.9→331.6, 201.5→175.4 and
386.9→320.1 ms. These measurements do not establish a 20 ms baseline.
Baseline reports: `tmp/no-query-contact-report.txt`, `tmp/span-envelope-report.txt`.
Candidate reports: `tmp/external-terrain-contact-report.txt`,
`tmp/external-terrain-idle-report.txt`. Contact/idle trace ELF hashes:
`4fd579fc30f59f30bc3c38c595ce8c20063deef4ec7590bac5aa9f4aa4a0ac60` /
`dcf6c25bd9859454bd2462b2d2f58fe0c048be2891eb1ddacab1f1070c4f1ec9`.
Reproduce with the contact command below using
`TRACE_FRAMES=50,675,1511,1688 TRACE_FIELDS=35`, and the no-input generation
command using `TRACE_FRAMES=162 TRACE_FIELDS=35`.

## Early contact execution gate

At `1330206e`, a fresh 35-field capture beginning at marker 50 reproduces
frame 51 at **31.23 ms**. It contains live bullets but no
enemies; capture coverage ends at tick 68, view Y 120, health 4. It is not
an idle frame. The normal stripped executable is restored after the capture.

| Separate owner in frame 51 | Inclusive ms |
| --- | ---: |
| Actor End Step dispatch/execution | 4.07 |
| Bullet Step, including position publication | 3.34 |
| Draw collection | 3.10 |
| Player movement/update | 2.54 |
| Room animation | 2.01 |
| Entry dither Begin Step | 1.69 |
| Captured alarm dispatch | 1.29 |

These are not all empty-family scans. Bullet position publication accounts
for 1.34 ms *inside* Bullet Step; Draw's registered traversal includes actual
sprite projection. Player position publication is only 0.62 ms, nested within
player movement. Do not reopen alarm or sub-millisecond traversal variants on
the strength of older, pre-repair numbers. This capture still exceeds the
historical 19.2 ms frame; it does not establish restored performance.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512 \
  VERIFY_TIMEOUT_SECONDS=180 STRIP=1 TRACE_FRAMES=50,1300 TRACE_FIELDS=35 \
  TRACE_OUTPUT=$PWD/tmp/trace/published-state-1330206e bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/published-state-1330206e \
  --frames=51 --causes --tree-min=0.15 --top=140
```

Do not combine the contact tape with `PROFILE_FRAMES`: that option selects
ordinary-gameplay profiling and the Makefile rejects the combination.
Reports: `tmp/published-state-51.txt`, `tmp/published-state-1305.txt` (select
`--frames=1305` for the latter). Trace ELF SHA-256:
`a0bbbda9e7ffe9762dc8d3529317b6d1e9355c31dc43182cc63f206ffda1fe2e`.

The second capture reaches tick 1309, view Y 2122, six enemy slots and health
3. Frame 1305 is **68.17 ms**: enemy Step 19.34 ms, bullet Step 5.28 ms,
draw collection 4.33 ms, actor End Step 4.05 ms, player movement 3.22 ms and
room animation 3.33 ms. These are separate owners, not removable overhead.
Walking-enemy tile meeting costs 4.10 ms inside enemy Step; its point callbacks
include 1.77 ms of native terrain checks and 0.97 ms of breakable lookup.
Enemy integer-position publication is 1.82 ms, also nested within enemy Step.

Published actor state is not wholly collision-only: `endStepBullet` reads its
binary32 Y for source retirement and the admitted interior test. Sprite-change
handling reads the previous sprite and image index and may reset timeline state.
`withPlayerTimeStop` also has an explicit published player-position reader.
No blanket actor-state deletion or sub-millisecond setter optimization is
justified by this census. The consumer boundary must distinguish those readers
from the generic collision geometry already excluded by scene admission.

## No-input workload and naming

The early contact-tape windows called "quiet" below are low-contact **firing**
windows: they contain live bullets and projectile updates. They are not an
idle baseline. The separate no-input route uses ordinary startup/generation,
with no actor, health, collision or RNG injection. It includes the initial
fall; after landing the enemies continue their source behavior.

At `778b7fc4`, three 35-field captures cover early falling, generation and
settled gameplay, using 512 KiB Chip and 8 MiB Fast on the cycle-exact PAL
A500. They end at ticks 72/170/313, view Y 415/1076/1082, with 0/3/3 enemy
slots and health 4. Input comes exclusively from the no-input controller.
These captures are not a new full-tape FPS measurement.

| Complete frames | Mean ms | Worst ms |
| --- | ---: | ---: |
| 51–72 | 30.9 | 41.4 |
| 301–313 | 47.2 | 49.2 |

Frame 301 is 45.89 ms: enemy Step 14.42 ms, captured alarm dispatch 3.36 ms,
player motion 3.73 ms, actor End Step dispatch 2.35 ms, and draw collection
2.98 ms. These are separate owners; none is an improvement claim. The enemy
Step cost includes movement/collision behavior as well as publication, not
just irrelevant-family scanning. The extra registered effect-surface traversal
is 0.53 ms: below the work threshold. Do not remove offscreen source Steps merely
because their pixels are invisible, or replace this measured generation target
with sub-millisecond empty-family/helper changes.

```sh
VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=120 STRIP=1 bash amiga/profile.sh idle
PROFILE_IDLE=1 SKIP_TO_GAME=1 PROFILE_CONTACT=0 PROFILE_FRAMES=600 \
  VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=120 STRIP=1 \
  TRACE_FRAMES=50,162,300 TRACE_FIELDS=35 TRACE_OUTPUT=$PWD/tmp/trace/scene-dispatch-778b7fc4 \
  bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/scene-dispatch-778b7fc4 --frames=301 --causes --tree-min=0.5
```

`PROFILE_IDLE` is a diagnostic input route only, never a shipping or optimized
logic mode. The build rejects other input tapes combined with it; the FPS
verifier requires no input, bullets, damage or death. Scripts restore the normal
build. Saved reports: `tmp/scene-dispatch-idle51.txt` and
`tmp/scene-dispatch-idle301.txt`. Matching trace ELF SHA-256:
`7d3e033bc0653c9bd7137eac7ba92b6d3df7ac98d1b090b1ea2c55eb851495af`.

## No-input generation gate

Current check at `16979319`: frame 163 is **245.63 ms**, 1,745,260 cycles
(0.3 ms from the preceding no-query admission capture, within noise).
`constructTerrainSpan` is 43.12 ms inclusive, 6.17 ms self. Its 26
`PreparedTerrainBirthBounds::publication` calls total 4.76 ms, including
necessary key, precision and domain checks as well as the unused historical
envelope. Removing only the envelope has not established the 10 ms worst-frame
work threshold; no implementation variant was attempted. Cached collision
state has direct readers outside index traversal (see the memory contract),
so no-query admission alone cannot remove that state or its publication.

Reproduce with the command below, `TRACE_OUTPUT=tmp/trace/span-envelope-16979319`,
then report `--frames=163 --top=140 --lines=constructTerrainSpan --tree-min=0.2`.
Report: `tmp/span-envelope-report.txt`; matching ELF SHA-256:
`7d2e3f9e5542d675f3da193a58c9f6546e3bbb2874ba5c63da1214195f9c295d`.

The same `778b7fc4` capture measures frame 163 at **256.91 ms** (1,825,390
cycles). This is instruction time, not a CPU estimate from a presentation gap.
It agrees with the preceding sparse-cutoff measurement (256.8 ms) within noise.

| Owner in frame 163 | Inclusive ms |
| --- | ---: |
| Live generation | 208.09 |
| Prepared terrain admission (within generation) | 98.63 |
| Native terrain construction (within admission) | 87.28 |
| Terrain collision-owner span publication (within construction) | 47.24 |
| Breakable construction (within generation) | 42.78 |
| Breakable native birth publication (within construction) | 26.54 |
| Enemy configuration (within generation) | 18.74 |
| Main-world row painting | 8.93 |

Nested costs must not be summed. The burst occurs without player input,
bullets or damage; capture coverage ends at tick 170, view Y 1076, three
enemy slots and health 4. It adds an idle-generation gate alongside the
contact tape's 676/1512/1689, rather than attributing all stalls to firing.

```sh
PROFILE_IDLE=1 SKIP_TO_GAME=1 PROFILE_CONTACT=0 PROFILE_FRAMES=600 \
  VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=120 STRIP=1 \
  TRACE_FRAMES=162 TRACE_FIELDS=35 \
  TRACE_OUTPUT=$PWD/tmp/trace/scene-dispatch-generation bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/scene-dispatch-generation \
  --frames=163 --causes --tree-min 2
```

Report: `tmp/scene-dispatch-generation163.txt`; matching ELF is the three-window
capture identified above.
The bounded script restores the normal build. Raw traces are disposable.

## Retained End Step capture — 2026-10-11

At `5373be79`, a fresh 30-field capture starting at 1300 measures frame 1305
at 74.7 ms, including **8.09 ms** in actor End Step. The same snapshot is
rebuilt and sorted even when its membership has not changed. Retaining the
player/non-box actor snapshot behind the native capture revision reduces
End Step to **4.13 ms** and the complete frame to **71.0 ms**. Eligibility and
lifetime reads during execution remain live; box prefixes are not cached.

| Matched frames | Mean before / after, ms | Worst before / after, ms |
| --- | ---: | ---: |
| 51–74 | 39.642 / 38.117 | 56.6 / 56.8 |
| 676–683 | 131.363 / 127.875 | 446.8 / 446.9 |
| 1301–1314 | 73.436 / 70.114 | 97.2 / 96.9 |
| 1512–1519 | 81.850 / 80.388 | 223.6 / 223.6 |
| 1689–1696 | 117.725 / 115.413 | 404.3 / 404.1 |

No matched frame increases by more than 0.3 ms. Frames that rebuild their
snapshot, including quiet frame 51 (32.1 ms), do not acquire the reuse saving.
This eliminates repeated capture work; it does not make event execution or
the remaining gameplay frames fit in 20 ms.

The complete 1,800-update contact tape takes **5,724 PAL fields (15.72 FPS)**,
versus 5,922 (15.20 FPS) at `5373be79`; the largest gap remains 22 fields at
676. Coverage remains ticks 0–1799, view Y 39–2749 (maximum 2759), seven enemy
slots, two damage-number births, final health 2 and no dead samples. The CPU
art bank still loads/prepares once (221,282 bytes), the gameplay raster lease
is 1,081,344 bytes, and no upgrade/results/shop banks load. Full-tape evidence:
`tmp/endstep-capture-fps-gdb.log`.

Reproduce with the contact trace command below and
`TRACE_FRAMES=50,675,1300,1511,1688 TRACE_FIELDS=100`. Reports:
`tmp/endstep-capture-report.txt`, `tmp/endstep-capture-before-1305.txt` and
`tmp/endstep-capture-1305.txt`. Focused baseline ELF SHA-256:
`2f621bc3713a5c09fb6d4aacb017a1b6b74a641bdc17dfd90770c2aaa8ee15b3`;
candidate ELF SHA-256:
`f4831dabcf2541643721d9dfffa1415a1e7a23d78c6484554836c22b055149ff`.
The three ownership fixtures retain full reference capture and verify repeated
dispatch, callback destruction/replacement, slot reuse, Retry and deferred
cleanup. Ten terrain tapes cover 18,000 frames. A warmed snapshot's callback
cannot admit its newly created actor into the same phase.
The 64-frame cross-target replay and normal startup with 512 KiB Chip also
pass; startup leaves 126,840 Chip bytes free. Evidence logs:
`tmp/endstep-capture-{host,replay,startup,startup-gdb}.log`. All owned emulator
runs are bounded; the normal non-profile `STRIP=1` build is restored.

## Activation-stall audit — 2026-10-11

The contact tape's 33-field gap at frame 1512 was not covered by the four
recent generation/combat windows. A fresh 60-field capture starting at 1511
on `1335f367` measures **658.3 ms** for that frame. Room activation
reconciliation costs **440.05 ms**, including 720 eligibility reads
(327.45 ms). Unchanged terrain repeatedly resolves its identity and flags
because a room activation request is pending. Rendering is not this stall.

The changed-slot projection retains the two room passes and publication order
but excludes unchanged eligibility reads. Frame 1512 is **223.6 ms**, with
**4.50 ms** inside selected room reconciliation. An independent 35-field
capture confirms **223.8 ms**. The same controller and 512 KiB Chip / 8 MiB
Fast / 512 KiB slow configuration are used throughout. No source event,
generation call or publication is deferred to obtain the reduction.

| Matched frames | Mean before / after, ms | Worst before / after, ms |
| --- | ---: | ---: |
| 51–74 | 39.629 / 39.642 | 56.6 / 56.6 |
| 676–683 | 131.350 / 131.363 | 447.1 / 446.8 |
| 1301–1314 | 73.421 / 73.436 | 97.2 / 97.2 |
| 1512–1519 | 136.225 / 81.850 | 658.3 / 223.6 |
| 1689–1696 | 117.700 / 117.725 | 404.4 / 404.3 |

No frame in the four previously measured windows increases by more than 0.3 ms. The quiet frame
51 remains 32.1 ms; combat/field frame 1304 remains 89.3 ms. Removing this
stall does not restore the 20 ms gameplay budget.

The full 1,800-update tape takes **5,922 PAL fields (15.20 FPS)**, versus
6,127 fields (14.69 FPS) at `1335f367`. Its largest gap is now 22 fields at
676, rather than 33 at 1512. Coverage is unchanged: ticks 0–1799, view Y
39–2749 (maximum 2759), seven enemy slots, two damage-number births, final
health 2 and no dead samples. The CPU art bank loads/prepares exactly once
(221,282 bytes); no upgrade/results/shop banks are loaded. The gameplay-only
terrain raster lease is 1,081,344 ordinary-memory bytes. FPS evidence:
`tmp/activation-selection-fps-gdb.log`.

Reproduce with the contact trace command below, selecting
`TRACE_FRAMES=50,675,1300,1511,1688 TRACE_FIELDS=100`; independent confirmation
uses `TRACE_FRAMES=1511 TRACE_FIELDS=35`. Reports:
`tmp/worst-1512-report.txt`, `tmp/activation-selection-report.txt`,
`tmp/activation-selection-1512.txt`, `tmp/activation-confirm-report.txt`.
Baseline ELF SHA-256:
`432a9f9f84fe90f80f159423cadb47050ea208a6496578adb84f162822adee22`;
candidate ELF SHA-256:
`9aecfd582a4ee547d8de254a60533861637cc3d2184e07e71843c2a008f7b816`.
The scalar ownership fixture retains full room reconciliation; the run-room
fixture checks ordered publication and failure prefixes, including a 32-to-1024
unrelated-entry growth case that must retain exactly one eligibility read.
Three shared-owner variants, ten terrain tapes (18,000 frames), the 64-frame
cross-target replay and normal 512 KiB Chip startup pass. The restored normal
build leaves 126,840 Chip bytes free. Logs use the
`tmp/activation-selection-{host,replay,startup,startup-gdb}.log` prefix.

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

Current four windows, with the cavern asset bank, single-admission journal,
sparse executable Draw chain and native prepared sprite geometry:

| Frames | Mean ms | Worst ms (frame) |
| --- | ---: | ---: |
| 51–74 | 40.11 | 57.1 (54) |
| 676–683 | 132.30 | 451.1 (676) |
| 1301–1314 | 74.10 | 97.9 (1314) |
| 1689–1696 | 118.85 | 409.2 (1689) |

Draw collection at frame 54 is 5.6 ms; rebuilding actor order no longer walks
non-drawing placeholders/scenery. Native bounds refresh now shares the
reference owner's prepared immutable shapes. Against `c4228228`, frame 676
falls from 460.3 to 451.1 ms, independently confirmed by a 35-field capture.
Its breakable birth observer spends 40.85 ms in bounds refresh rather than
49.07 ms: prepared translation costs 10.30 ms instead of 17.90 ms rebuilding
the shape. Spatial publication still costs 21.43 ms and is not bypassed.
No matched frame is more than 0.3 ms slower. Frame 51 remains 32.6 ms; frame
1304 is 90.0 ms. These are still unacceptable gameplay stalls, not 50 FPS.
The full 1,800-update tape takes 6,176 PAL fields (14.57 FPS), versus 6,179
at `c4228228`; its maximum gap remains 33 fields at 1512. PAL quantization
largely hides the focused computation saving in the aggregate FPS figure.
Coverage
remains ticks 0–1799, view Y 39–2749 (maximum 2759), seven enemy slots,
final health 2 and no dead samples. Gameplay art loads/prepares once.

Trace ELF SHA-256:
`d1819a83dd71901581e664b2109e50d7207c7a5590a08504f769ece5ecbb4677`.
Reports: `tmp/native-geometry-fps-gdb.log`, `tmp/native-geometry-report.txt` and
`tmp/native-geometry-confirm-report.txt`. Reproduce using the preceding contact command
with `TRACE_FRAMES=50,675,1300,1688 TRACE_FIELDS=100`.

### Remaining generation ownership costs — `c4228228`

Independent 35-field captures starting at 675/1688 measure frames 676/1689
at 460.29/410.0 ms. Inclusive boundaries (not additive):

| Boundary | Frame 676, ms | Frame 1689, ms |
| --- | ---: | ---: |
| Prepared terrain owner span | 133.03 | 150.08 |
| Terrain identity/run insertion (`appendSpan`) | 25.33 | 30.89 |
| External-object registration | 22.64 | 26.93 |
| Dirty-queue marking, all callers | 21.70 | 21.96 |

Frame 676 also has a separate breakable-birth cost: `createBreakable` takes
73.75 ms, with 65.47 ms in `NativeFamilyBirth::publish`. Its birth observer
spends 49.07 ms in `refreshNativeActor`, including 17.90 ms recomputing unit
sprite bounds and 21.06 ms in spatial publication. This is not the prepared
terrain constructor or terrain rendering. Frame 1689 has no corresponding
breakable-birth subtree; it must not be credited with this prospective saving.

The production caller is `boxAllocation_.observeBirth`, after `birthPosition`
and `nativeAppearance`, before spatial/layer publication completes. Reusing
immutable shape geometry there would not justify caching eligibility, tree
membership or callback results. The generic bounds refresh remains necessary
for unsupported positions or appearance and later mutations.

Reproduce with the contact command above, `TRACE_FIELDS=35` and
`TRACE_FRAMES=675,1688`. Reports: `tmp/generation-current-{676,1689}.txt`;
driver log: `tmp/generation-current-trace.log`. Trace ELF SHA-256:
`838723d93bbe446201d652fe0a9f467328932cc341e4a0c47747f7760766af1d`.

### Remaining actor-event costs — `d771b7bf`

Two eight-field captures starting at 50 and 1304 measure frame 51 at
**30.37 ms** and frame 1305 at **66.44 ms**. The first ends at tick 53,
view Y 107, zero enemy slots and health 4. The second ends at tick 1305,
view Y 2120, six enemy slots and health 3. The historical comparable early
frame below was 19.2 ms. Generation/residency changes have not resolved that
regression; these short windows do not establish whole-game performance.

| Inclusive boundary, frame 51 | ms |
| --- | ---: |
| Entire gameplay Step | 20.25 |
| Actor End Step capture/dispatch/execution | 4.17 |
| Bullet Step and published-position update | 2.94 |
| Draw collection | 3.0 |
| Player movement | 2.07 |
| Room animation callback | 2.02 |
| Bullet position publication, inside Bullet Step | 0.94 |
| Dither Above Begin Step | 1.66 |
| Captured alarm dispatch | 1.29 |
| Bullet retirement, inside Actor End Step | 0.83 |

Parent/child rows overlap. In particular, another arithmetic or bullet
retirement helper change cannot remove the remaining dispatch cost. The
retained generic End Step adapter captures lifetimes before callbacks,
checks live eligibility afterwards and preserves player/box/actor order.
The player's event updates `playerHit`; out-of-view destruction can run
callbacks and reread published positions. Absence of enemies alone is not
a certificate that this phase is empty. Any phase-elision replacement must
account for these effects at the owning lifecycle boundary rather than
introducing another full-room proof scan. The historical `GameplayWorld::step`
used direct enemy/box retirement and a drone adapter, not this mixed actor
phase. Its lower time is not permission to restore that older event ordering.

Frame 1305 spends 18.39 ms in enemy Step, 4.87 ms in bullet Step and 4.11 ms
in actor End Step. Within enemy Step, transform publication is 1.56 ms,
image-index publication 1.14 ms, and walker sprite publication 0.61 ms.
Together those disjoint appearance publications are 3.31 ms; this is an
upper bound on removable work, not a predicted saving. Walking collision
checks (4.10 ms) and bat collision resolution (2.14 ms) are separate, actual
gameplay work and must not be charged to that appearance estimate.

`Binding::transform`, `imageIndex`, `mask` and `sprite` still maintain the
generic `NativePosition::sprite` state even when collision consumers are
not admitted. Direct `nativePublishedSprite` callers are currently tests;
generic collision refresh/collection consumes it internally. However sprite
writes also update table sprite membership and prepared Draw admission, and
`RunnerSpriteChange::plan` uses the prior image index to implement reset
semantics. Removing the entire appearance mirror therefore requires separating
those source-visible effects, not merely nulling the callbacks or dropping
setters. The full-service reference and unsupported-read guards must survive.
This evidence does not justify tuning any individual sub-millisecond helper.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512
VERIFY_TIMEOUT_SECONDS=120 STRIP=1 TRACE_FIELDS=8 TRACE_FRAMES=50,1304
TRACE_OUTPUT=$PWD/tmp/trace/current-event-cost bash amiga/profile.sh trace`, then
`python3 tools/amiga-trace-report.py tmp/trace/current-event-cost --frames=51
--causes --tree-min 0.3` (select `--frames=1305` for the later frame).
Reports: `tmp/current-event-cost-{51,1305}.txt`; driver log:
`tmp/current-event-cost.log`. Trace ELF SHA-256:
`e98c02aec4d148dd18fb258e62605b9188483bafd1c729fd56818cd4578e8bbb`.

## Historical pre-integration audit

### Matched early-frame attribution

An eight-PAL-field re-capture of the preserved `13e08ca2` ELF measures
frame 51 at 19.45 ms (the original longer capture reported 19.2 ms).
Compare with the `d771b7bf` frame-51 capture above, not the obsolete
56.1 ms post-integration snapshot below:

| Same boundary in frame 51 | `13e08ca2`, ms | `d771b7bf`, ms |
| --- | ---: | ---: |
| Entire frame | 19.45 | 30.37 |
| Gameplay Step | 11.18 | 20.25 |
| Player movement | 2.17 | 2.07 |
| Machine-gun projectile helper, excluding outer publication | 1.26 | 1.22 |
| Bullet coordinate publication | 0.34 | 0.94 |
| Room animation callback | 1.34 | 2.02 |
| Captured alarm dispatch | 0.69 | 1.29 |
| Draw collection | 2.19 | 3.00 |

Rows are inclusive and must not be summed. Ordinary movement, terrain
segment tests and projectile motion have not acquired the missing 11 ms.
The later mixed End Step callback costs 4.91 ms, including 4.17 ms in its
capture/dispatch/execution boundary. Registered Dither Above Begin Step costs
1.66 ms. Neither had an equivalent callback in the old frame; this does **not**
make their entire cost removable. The old implementation retired enemies and
boxes directly and handled projectile retirement in Step. Commits `40921412`,
`660f9be6`, `3c0d265b`, `c9cbba2e` and `69c390e6` establish the later mixed
ordering, thin-platform boundary, post-collision projectile retirement and
player-hit End Step. Reverting that ordering is not a performance fix.

Any native End Step replacement must own a complete captured phase: player
35 before boxes 92, then the thin-platform boundary and ordered actors; live
eligibility and destruction callbacks still apply, while callback-born actors
wait for the next capture. Replacing generic lifetime resolution requires
validating borrowed typed records across retirement/reuse and activation, not
removing those semantics or merely shortening the existing capture loops.

Historical re-capture: `tmp/old-early-recheck-report.txt`,
`tmp/old-early-recheck-gdb.log`, ELF in `tmp/trace/old-early-recheck/` with
SHA-256 `26bb4f25411a7d2344342f50447ac567f286e03d26bb44cefc643ab7e06a6e8f`.
The debugger stopped at `downwellTraceReady(50)`, recorded eight uninterrupted
PAL fields using `monitor profile 8`, and the report selected `--frames=51`.
Both normal binaries were restored immediately afterward. The existing
80-sample state comparison below bounds workload comparability; it is not a
full pixel/event-equivalence claim.

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
| Scene terrain admission | Omit all generic terrain registration; retire thin platforms through the active room walk | Generation 676/1512/1689 287.1/153.7/275.5 → 260.0/145.6/243.6 ms, but phase-7 retirement 1047/1535 118.2/82.4 → 178.8/151.5 ms; 18,000-frame ownership and cross-target replay pass | Removing the live platform consumer's membership replaces its small indexed walk with a 60–70 ms room scan. Retain object 93 registration; only wall registration is unconsumed | This row's introducing commit; `tmp/terrain-retirement2-{baseline,candidate}-report.txt`, candidate ELF `8c181023896588bbe3ea0257e3693b8f3e010d4c1820029115f236dd7d3438f6`; no retained implementation |
| Enemy appearance publication | Record-owned image/transform state, retaining lifetime validation and sprite/Draw membership | Frame 1305 66.44 → 66.18 ms; 51 30.37 → 30.7; generation 676/1512/1689 302.1/160.8/283.9 → 301.8/160.5/283.6 ms; 18,000-frame ownership and full-service collision tests pass | Validation still costs 2.23 ms inside the replaced 3.31 ms publication boundary; whole-frame change is within noise. Removed; no setter/validation-helper variants | This row's introducing commit; `tmp/record-appearance-report.txt`, `tmp/record-appearance-1305.txt`; no retained implementation |
| Terrain Draw cutoff | Preserve cached cutoff across active scalar/prepared-span births | Against add6c322 with the same tree lease: idle generation 163 265.5 → 265.4 ms (1,886,300 → 1,885,440 cycles); cutoff traversal remains 7.93 ms | Other invalidation paths still require the complete retained-run scan. Misses the estimated 8 ms saving; logical ordering, 32/2048-entry admission tests and 18,000-frame terrain replay pass but do not establish a performance win. Removed; do not sweep invalidation variants instead of replacing the full-run observation | This row's introducing commit, [`active-birth-cutoff-cache.patch`](rejected/active-birth-cutoff-cache.patch) |
| Native dirty queues | Last-mark serials until first observation, then reconstruct once and retain the original linked queue | Against 092fc0ee with the same tree lease: contact 676 416.0 → 419.2 ms, 1512 209.3 → 210.2, 1689 403.4 → 406.1; idle 163 265.5 → 267.0; 1,800 updates 5,568 → 5,565 PAL fields; +47,584 ordinary bytes | Misses the estimated 10 ms burst saving: mark bookkeeping rises from about 17 ms to 20 ms in 676. Slight tape/quiet-frame variation does not offset worse affected generation frames or extra storage. Ordering/overflow/callback tests and 18,000-frame terrain replay pass. No further dirty-queue variants after this and the fresh-span rejection | This row's introducing commit, [`deferred-dirty-queue.patch`](rejected/deferred-dirty-queue.patch) |
| Prepared terrain queues | Admit fresh segments once and publish reverse-birth dirty-queue links as a batch | Against a9408e8b: 676 451.1 → 452.2 ms; 1689 409.2 → 409.3; matched window means 40.108/132.300/74.100/118.850 → 40.100/132.475/74.071/118.863 ms | Admission costs 6.41 ms and link publication 8.58 ms in frame 676; segment bookkeeping consumes the remaining per-cell mark saving. Host queue and 18,000-frame terrain checks pass, but there is no end-to-end improvement | This row's introducing commit; removed, no retained implementation |
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
