# Gameplay performance checkpoint after scene extraction

## HUD remeasurement and rejected grouped damage scan

At `1199a31`, a fresh coarse profile still attributes about 4.10 ms/update
to ammo phases (including measurement overhead): base 0.769, rows 0.823,
dividers 0.646, rim/label 0.515, damage 0.770, fill geometry 0.029 and
word publication 0.551. Only 138 damage updates occur in the 600-frame tape;
this is not evidence of an unconditional full HUD redraw every frame.
The largest instrumented frame remains generation-dominated: 51.864 ms
generation, 23.253 ms registration and 23.676 ms indexing. Log:
`/tmp/downwell-hud-current-probes.log`.

A four-byte damage scan normalized states 3/4 to their identical visible
frame, skipping equal groups while retaining scalar endpoint handling.
It passed 12,000 scalar damage comparisons, full HUD reconstruction tests,
22,834 asset comparisons and ASan/UBSan. Damage time fell from 462.128 to
412.156 ms over the instrumented tape, but shipping throughput worsened
from 825 fields to 827; an equality-first refinement took 826. All retain
the four-field worst gap. The grouped implementation was removed: no
shipping optimization is claimed. The extra unchanged/invisible-state test
cases remain. Logs: `/tmp/downwell-hud-blockscan-host.log`,
`/tmp/downwell-hud-blockscan-sanitized.log`,
`/tmp/downwell-hud-blockscan-probes.log`,
`/tmp/downwell-hud-blockscan-fps.log` and
`/tmp/downwell-hud-blockscan-fast-fps.log`.

The next HUD target should be reducing composition passes across a damaged
strip, not another micro-optimization of the endpoint scan. Background/fill,
animated row runs, dividers and final word publication currently make
separate passes even though their final pixels occupy the same narrow column.
Any fused path must preserve ordered masking and the full scalar pixel oracle.

## Decode only the matching builder pattern

`BuilderPatternPack::findPattern` now scans the two serialized identifiers
without decoding every preceding record's dimensions and row/cell ranges.
Only the matching record is decoded. Failed lookups preserve the previous
output behavior; invalid packs return before any record access. There is no
new allocation, cache, asset format or RNG/Create scheduling change.

At `4c624c0`, the matched baseline takes 827 PAL fields for 600 updates
(36.28 FPS); the candidate takes 825 (36.36 FPS). Both have a four-field
maximum. This small difference does not resolve the 20 ms target or establish
a meaningful reduction in the worst generation stall. Logs:
`/tmp/downwell-pattern-lookup-baseline.log` and
`/tmp/downwell-pattern-lookup-candidate.log`.

Tests compare every output field with the old full-enumeration lookup across
all recovered patterns, missing keys, null and damaged packs. ASan/UBSan pass;
64 generated levels retain ordered emission/RNG digest `ffcdfc23dc638be7`.
Both host natural-exit routes, upgrade/re-entry and the generated-shop route
also pass. Logs: `/tmp/downwell-pattern-lookup-host.log`,
`/tmp/downwell-pattern-lookup-sanitized.log` and
`/tmp/downwell-pattern-lookup-natural.log`.

The bounded native route also passes: exit at update 1,477, 211 upgrade
frames, 135 exit-field frames, 49 shop-HUD comparisons and 43 return updates,
with zero failures and an intact stack guard. The normal Amiga build is
restored and both 68000 audits pass; SDL is rebuilt. Evidence:
`/tmp/downwell-pattern-lookup-native.log` and
`/tmp/downwell-pattern-lookup-native-gdb.log`.

## Skip invisible ammo-row transitions

`AmmoGaugeRows` still advances every original internal state and retains its
existing `step()` change result. A separate presentation flag records sprite
family or integer-frame changes. States 3 and 4 both display frame 3, so their
transition alone no longer sets `GameplayFrame::ammoChanged`. Ammo, capacity
and HUD motion changes still independently invalidate the gauge. No divider,
fill, rim, animation timing or palette behavior is changed.

Twenty thousand updates match the original fractional animation's state-change
and visual-change results. A 1,000-update single-digit firing/idle test omits
unchanged composition and remains byte-identical to full redraws; existing
capacity/divider and 22,834 sprite comparisons also pass. The new comparison
starts with a clean HUD history: appending it after the multi-digit tests gave
the scalar oracle an extra narrow clearing pass on an unchanged frame.
Logs: `/tmp/downwell-ammo-visible-rows.log`,
`/tmp/downwell-ammo-visible-render.log`, `/tmp/downwell-ammo-visible-host.log`.

The bounded native tape takes 827 fields, **36.28 FPS**, with the same
four-field maximum. The one-field difference from the matched 828-field
baseline is small; this is removal of a redundant redraw trigger, not a
meaningful resolution of the frame budget. Cross-target replay retains all
four expected simulation digests. Logs: `/tmp/downwell-ammo-visible-fps.log`
and `/tmp/downwell-ammo-visible-replay.log`.

## Current matched measurement after natural-route integration

At `7144495`, the unchanged ordinary gameplay tape takes **828 PAL fields,
36.23 FPS, maximum four-field gap** over 600 updates. This supersedes the
822-field current-throughput claim below; the earlier measurements remain
historical evidence for their particular revisions. Log:
`/tmp/downwell-rng-shift15-baseline.log`. The profile restores ordinary startup.

A candidate `SWAP; ROR.L #1; ANDI.L #$ffff8000` replacing the RNG's
15-bit left shift produced exactly the same 828-field / 36.23-FPS result and
four-field maximum (`/tmp/downwell-rng-shift15-fps.log`). Although the expression
is bit-equivalent, it yielded no measured end-to-end benefit. It was removed;
the original portable shift and existing 28-bit-shift implementation remain.
The host RNG tests pass. No new native RNG-equivalence claim is made for the
discarded candidate. This experiment does not address the remaining 20 ms
budget shortfall, and its unchanged result is not an optimization win.

## Current remeasurement after backing/staging ownership

At `3c5108e`, the same 600-update A500/PAL tape takes **822 fields, 36.50 FPS**,
with a five-field maximum gap. This supersedes the older 815-field result
below as the current measured baseline; it is not a claimed improvement.
Camera-lock behavior and scene/cache ownership have changed since that older
baseline. There are still zero staged column swaps in this particular tape.
Log: `/tmp/downwell-current-cache-fps.log`.

Matched instrumented runs isolate the inner generation probes' perturbation:

| Instrumentation | Fields | Worst frame (sample 258) |
| --- | ---: | ---: |
| Including inner generation phases 67–74 | 1034 | 237.513 ms |
| Without inner generation phases | 1017 | 141.681 ms |

Both have zero clock clamps and identical indexed-collision/terrain-segment
and hardware-sprite counters. The difference is 95.832 ms in that worst frame;
it is measurement overhead/code-layout perturbation, not a shipping speedup.
The coarse frame attributes 69.329 ms to generation, 23.391 ms to registration
and 25.502 ms to indexing (118.222 ms combined). The ammo phases sum to about
4.101 ms/update over the tape. All phase measurements still include probe
overhead; low-overhead FPS remains the throughput measure.

Inner probes are now explicitly opt-in with `GENERATION_DETAIL=1` and compile
away otherwise. Existing outer phases remain enabled by `PROBES=1`. Use
`GENERATION_DETAIL=1 SKIP_TO_GAME=1 VERIFY_TIMEOUT_SECONDS=120 bash amiga/profile.sh probes`
to recover the detailed view. The default probes command now takes the coarse
view and prints the selected mode. The script explicitly resets the detail
option when restoring the ordinary build.

Logs: `/tmp/downwell-current-cache-probes.log` (inner detail before the toggle)
and `/tmp/downwell-current-cache-coarse.log`. Normal startup is restored with
both 68000 audits clean. Host tests verify disabled, coarse and detailed macro
behavior, including return to the enclosing phase
(`/tmp/downwell-generation-probe-toggle.log`). Production generation order,
RNG consumption and painting are unchanged by this instrumentation switch.
Next optimize generation with these coarse brackets as the timing comparison;
do not treat the heavily instrumented inner brackets as removable game cost.

## Plain terrain command initialization (after `aa2e2b0`)

The shared pattern emitter now initializes one default wall/platform command
per pattern, updates only object/x/y, and invokes each callback synchronously.
Objects 60 and 93 previously rebuilt the entire actor-sized payload for every
cell despite using identical defaults. Other objects keep their existing
Create initialization. The temporary remains local to each pattern invocation;
there is no shared mutable singleton, delayed callback or RNG rescheduling.

The same low-overhead tape takes 825 fields (36.36 FPS), versus 822 (36.50 FPS)
before the change. **Average throughput did not improve.** The maximum gap
falls from five fields to four. The matched coarse worst frame (sample 258)
falls from 141.681 to 130.998 ms; its generation bracket falls from 69.329 to
58.858 ms, while registration/indexing remain approximately unchanged at
23.400/25.410 ms. The change is retained for reduced generation-burst cost,
not as a claim of 50 FPS or improved average throughput. Instrumented timing
remains perturbed by probes; the four-field low-overhead gap still misses 20 ms.

Evidence: `/tmp/downwell-terrain-command-fps.log` and
`/tmp/downwell-terrain-command-coarse.log`. Sixty-four seeded levels retain
the complete ordered Create-field/RNG digest `ffcdfc23dc638be7`; explicit tests
also compare emitted walls/platforms against freshly initialized commands.
The builder passes ASan/UBSan (`/tmp/downwell-terrain-command-sanitized.log`).
Incremental terrain tests and paired SDL/Amiga scene tests pass, including
both next-level re-entries (`/tmp/downwell-terrain-command-live.log`). These
prove preservation of the modeled generator, not a new Windows-runner audit.

The bounded native camera gate passes 600 updates, seven transitions, six
staged swaps/one forced fallback, 119 fresh terrain comparisons and 14 display
checks with zero mismatches and an intact stack guard. Four camera locks and
three releases remain covered. Log: `/tmp/downwell-terrain-command-native.log`;
details saved as `/tmp/downwell-terrain-command-native-gdb.log`. Normal startup
is restored and both 68000 audits pass.

## Single validated terrain index (after `0ee08af`)

Terrain insertion now validates coordinate bounds before subtraction, computes
the cell index once, and reuses it for the nearest-landing update. Landing
values use their actual 16-bit range (1..512), with a guarded unsigned cell
stride so the loop cannot wrap past the first row. Existing code generation
already used a pointer stride; this is not presented as removal of a multiply
per row. Invalid aligned extreme coordinates now reject before potentially
overflowing signed arithmetic. No new memory, callbacks or timing semantics
are introduced.

The same low-overhead tape improves from 825 to 822 fields (36.36 to 36.50
FPS), retaining the four-field worst gap. Treat this as a modest measured
difference, not a 50 FPS result. The coarse worst frame falls from 130.998 to
129.107 ms. Its index bracket falls from 25.410 to 23.554 ms, while generation
and registration remain around 58.840 and 23.505 ms. Logs:
`/tmp/downwell-index-once-fps.log` and `/tmp/downwell-index-once-coarse.log`.

Incremental terrain tests pass, including full recomputation after each
generation event, shuffled/duplicate insertions, boundary cells and live
indexed-versus-reference collision tapes (`/tmp/downwell-index-once-host.log`).
The expanded tests also pass ASan/UBSan, including extreme coordinates
(`/tmp/downwell-index-once-sanitized.log`).

Paired SDL/Amiga scenes retain both next-level re-entries
(`/tmp/downwell-index-once-scenes.log`). The bounded native camera gate passes
600 updates, seven transitions, 119 terrain comparisons and 14 alternating
display checks with zero mismatches and an intact stack guard. The six staged
swaps, forced fallback, four locks and three releases remain covered. Logs:
`/tmp/downwell-index-once-native.log` and
`/tmp/downwell-index-once-native-gdb.log`. Normal startup is restored and both
68000 audits pass. Natural full-level coverage and the 20 ms target remain open.

## Earlier extraction checkpoint

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
