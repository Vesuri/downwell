# Gameplay performance checkpoint after scene extraction

Measured from `27da653`, A500 / 512 KB Chip / 8 MB Fast, `-O3`, PAL.
Both runs start with `SKIP_TO_GAME=1`, use the existing 600-update deterministic
left/right/fire profile, and have a 120-second emulator deadline. The profile
script restores all startup skips to zero afterward. This fixture does not
cover every encounter, shop transition, upgrade or later level.

## Low-overhead result

600 updates take 815 PAL fields: **36.81 FPS**, maximum gap **five fields**.
The 20 ms requirement is not met. No staged horizontal camera transfers occur
in this tape, so it cannot measure that optimization. Only the first 32 missed
deadlines are listed by the existing logger; the list is not a complete count.

Evidence: `/tmp/downwell-gameplay-current-fps.log`.

## Detailed probes — not shipping FPS

The instrumented run takes 997 fields. Its worst complete frame is sample 258,
140.392 ms including 9.535 ms waiting. Exclusive phase accounting attributes
68.875 ms to generation, 23.164 ms to object registration and 25.178 ms to
terrain indexing: **117.217 ms combined**. These are repeated brackets within
one frame, not the single-bracket maxima printed in the main profile table.
Do not compare this 140 ms figure directly with the low-overhead five-field gap.

Selected average exclusive costs over all 600 samples:

| Work | ms/update |
| --- | ---: |
| Waiting | 8.494 |
| Dirty display-row copies | 0.528 |
| Terrain walls, tiles, decorations, assets and wall-frame lookup | 1.936 |
| Actor collection, channel selection and hardware-sprite update | 2.297 |
| Ammo phases 27–30 and nested detail phases 37–39 | 4.101 |
| Generation plus registration and terrain indexing | 0.847 |

The small generation average hides its burst cost. Terrain copying alone is
not the cause of the largest observed stall. This does not prove that every
copy or repaint is optimal. The ordinary fixture has no Laser Sight activity.
Collision counters report indexed routes only (2,258 solid, 600 landing,
464 segment queries / 719 cells), with no fallback calls. Hardware sprites
record 523 header-only updates and 454 offline-payload copies (15,486 words);
these are copies of exported data, not runtime pixel conversion.

Evidence: `/tmp/downwell-gameplay-current-probes.log`. Probe overhead is
material; low-overhead FPS must remain the throughput measure.

## Next performance work

1. Break down generation CPU work and optimize the largest costs while
   preserving exact RNG consumption and synchronous Create ordering. The
   `GameplayWorld::createGeneratedInstance` handoff explicitly requires shop
   and exit Create effects before the next pattern-cell random draw. Moving
   generation earlier or spreading it over frames is not behavior-neutral.
2. Revisit the 4.10 ms average ammo-path cost, distinguishing geometry,
   damage scanning and actual changed-word publication. Retain the existing
   static rim and incremental update rules.
3. Re-run the same low-overhead tape after each change, plus correctness
   gates. Add separate fixtures for horizontal shop transitions and dense
   encounters; this tape alone cannot prove a worst-case 20 ms budget.

## Rejected experiment: bounded RNG remainder

A four-word reduction of the 63-bit bounded random value was tested as a
replacement for the three general 32-bit remainder operations. The portable
candidate matched 1,048,576 independent 64-bit-modulo cases across all 65,536
divisors; existing RNG tests preserved results, draw counts and following
stream words. Its 68000 twin used four quotient-bounded `DIVU.W` instructions.
Cross-target replay passed, as did builder fixtures (ordered emission digest
`ffcdfc23dc638be7`) and sanitizer checks of the portable reduction.

It did **not** improve the unchanged gameplay tape: 600 updates / 816 fields,
36.76 FPS, five-field maximum gap, versus the 815-field / 36.81 FPS baseline.
The instrumented worst-frame generation/registration/index sum changed from
117.217 to 116.718 ms, too small to establish a useful frame-time benefit.
The production RNG and build files were restored; no extra division assembly
or unused helper was retained. This experiment does not justify changing
RNG/Create timing or splitting generation across simulation frames.

Evidence: `/tmp/downwell-small-modulo-host.log`,
`/tmp/downwell-small-modulo-rng.log`, `/tmp/downwell-small-modulo-sanitized.log`,
`/tmp/downwell-small-modulo-builder.log`, `/tmp/downwell-small-modulo-replay.log`,
`/tmp/downwell-small-modulo-probes.log`, `/tmp/downwell-small-modulo-fps.log`.
The builder log includes an initial incorrect fixture invocation followed by
the passing invocation with the required builder-pack argument.

Next investigate command initialization/creation and terrain indexing in the
generation burst; the bounded remainder candidate is not the main bottleneck.

## Generation detail instrumentation

Added optional phases 67–74 for command setup, command initialization, terrain
cell insertion, landing-index updates, tile-list insertion, and breakable,
enemy and shop registration. They compile away outside `PROBES`; generation
ordering and RNG consumption are unchanged.

The same bounded 600-update tape completed with valid exclusive accounting,
zero clock clamps and unchanged collision/sprite counters. The worst frame is
still sample 258. It contains no breakable, enemy or shop registration; the
burst is terrain-command generation. Detailed exclusive brackets there are:
command setup 37.970 ms, initialization 18.040 ms, terrain cells 13.098 ms,
landing index 15.948 ms, tile list 10.092 ms, plus remaining generation
51.501 ms, registration 23.249 ms and index coordination 44.049 ms.

These are **not production operation costs**. The many fine-grained phase
switches inflate the worst frame from the older 140.392 ms to 236.404 ms;
the detailed run takes 1,017 fields. In particular the tile-list probe includes
early rejection of non-tile commands. Do not infer that each reported bracket
is removable work, or treat this run as a shipping slowdown. The actionable
narrowing is that the largest burst does not create actors or shops. Further
measurement should use coarse isolated/batched probes or a matched empty-probe
control before choosing an optimization. Shipping throughput remains the
low-overhead baseline above, not a new 20 ms result.

Evidence: `/tmp/downwell-generation-detail-probes-v2.log`. Host builder and
incremental terrain tests pass with ordered emission digest
`ffcdfc23dc638be7` (`/tmp/downwell-generation-detail-host.log`). The initial
instrumented build exposed a missing explicit `Profile.h` include in
`GameplayWorld.h`; that was fixed before the successful run. Normal startup
was restored and both 68000 audits passed.

## Fidelity scope

SDL and Amiga both currently stop advancing after cavern level 3 because
later-area builders are absent. The matching limitation is not an Amiga-only
regression and is not original-game completeness. The Windows data/VM remains
the authority; neither this profile nor cross-port agreement changes that.
