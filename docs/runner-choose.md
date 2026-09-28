# Windows choose: one word, not irandom

## Evidence

Read-only inspection of the original `data/Downwell.exe`, SHA-256
`340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085`,
resolves the `choose` name at VA `0x8139d8`. The registration at `0x53fab5`
supplies function `0x53b860` to the registration routine.

The wrapper tests argument count for **zero**, not one, at `0x53b87b`.
Every nonempty call invokes `0x438da0` at `0x53b885`, passing argument count.
That helper calls WELL at `0x438da1`, retains the unsigned result, computes
the absolute divisor and executes unsigned division at `0x438db7`; the return
value is its remainder. There is no singleton early return. The wrapper then
converts this index through floating point, adds the zero constant at
`0x7fcc88`, calls its rounding helper and clamps to count-1. For the small
lists used here those integer indices are exactly representable.

Thus `choose(a)` consumes one word, and `choose(a,b,...)` selects
`nextU32() % count`. It is **not** the two-word `irandom(count-1)` algorithm.
Identical-valued choices still consume a word.

`python3 tools/verify-runner-choose.py` pins the executable and reviewed
registration/wrapper/helper regions without committing recovered code bytes.
This hash check guards the reviewed evidence; it is not independent execution
of the original runner or a general claim about other GameMaker versions.

## Shared primitive and outstanding migration

Current follow-up: `resultRandomizeUg` block 5, `enmBat_Create` heading and
`Scattered16_Create` frame selection explicitly call `irandom` in the VM.
These remaining integer-range sites must not be migrated to `choose`.
The later-cavern spawn correction is now implemented separately (see
`original-behavior-audit.md`); historical migration fingerprints below predate
that correction. This audit does not establish completeness of omitted events.

`GameMakerRandom::chooseIndexSmall` now implements this behavior for nonempty
lists of up to 65,535 entries. The RNG unit test covers all those counts over
32 seeds, comparing selection, draw count, index and all sixteen state words.
A singleton case also pins the next word of the existing WELL test vector.

### Bounded remainder arithmetic

`SmallUnsignedRemainder` keeps `% divisor` as its portable oracle. Its 68000
twin uses the remainder returned by `DIVU.W`, reducing the high word before
appending the low word. Every divide has a high word smaller than the nonzero
16-bit divisor, so the quotient fits without overflow. Integer ranges reuse
that reduced remainder across successive words; divisor 65,536 takes the
power-of-two path. Power-of-two choices still consume their one WELL word,
including singleton choices; integer ranges still consume both words.

The native replay gate compares both forms over every valid divisor, including
maximum dividends and maximum legal reduced high words. RNG and ordered Create
tests separately check stream consumption; arithmetic equivalence alone does
not verify that a VM call site selected the correct RNG operation.

**Migration is partial.** The shared area-one builder and
`BuilderRewardChoice` now use `chooseIndexSmall` where their implemented VM
branches call `choose`. Other sites mix genuine `irandom`
with translated `choose`, so globally replacing `irandomSmall` is incorrect.
Audit each call against VM instructions and correct both adapters together.
Generated fixtures/checkpoints must then be rebuilt from the corrected stream,
and natural host/native routes reverified; do not retain old hashes by
artificially burning extra random words.

This finding supersedes the unresolved singleton question in area-two recovery.
It also limits previous parity evidence: matching two translations with the
same RNG mistake does not prove original-runner fidelity. Correcting existing
choices takes priority over enabling catacomb content.

## First migration: shared cavern generation

Audited against `scrLevelPat1`, `scrBuilder`, `builderCavern` and the relevant
Create-event VM call sites. Both SDL and Amiga consume this shared builder.

| Corrected choice | Argument count |
| --- | ---: |
| Side-room count | 3 |
| Mirroring, including otherwise empty prelude rows | 2 |
| Shop chance, normal/style 1 | 2 / 7 |
| Optional wall/box cells | 2 |
| Scattered decoration / mixed enemy symbol | 5 / 4 |
| Reward chunk/crate / low-health override | 2 / 6 |
| Box break sound / normal frame / gem frame | 4 / 2 / 2 |
| Turtle walking, snail direction, crawler orientation, decoration flip | 2 |
| Gun-module type / low-health overwrite | 2 / 7 |

The missing GunModule low-health overwrite draw is now retained after its
initial choice. Pattern indices, room-position ranges, gem chance, actor
alarms, scatter frame and module-number rejection remain genuine `irandom`
calls. The general builder still has other known incomplete behavior (such as
later-level cavern spawn rules); this migration does not claim to fix all of it.

Validation:

- 128 independent mixed-choice/range setup cases compare all RNG state;
  61,440 reward decisions compare against raw-word modulo choices.
- All 64 complete generation traversals and the visible/incremental builder
  tests pass, normally and under address/undefined-behavior sanitizers.
- New regression fingerprint: `1fd36f0408435e8b`. The superseded
  `ffcdfc23dc638be7` used the wrong RNG algorithm. Neither is a Windows capture.
- Ordinary SDL and Amiga builds pass, including the 68000 instruction and
  64-bit-helper audits (`/tmp/downwell-builder-choose-{sdl,amiga}.log`).
- The natural host gate passes seed 99 at 1,684 gameplay updates, 211 upgrade
  updates and 120 re-entry updates. It then **fails** the seed-4 decoded-entry
  checkpoint assertion; that shortcut still pins the pre-correction stream.
  Evidence: `/tmp/downwell-builder-choose-natural.log`. Do not report this
  gate, the native seed-4 route or old route-sweep results as current passes.

Next audit: startup/garden choices, shared gem/player/impact choices, enemy
alarm callbacks and other gameplay calls. Regenerate checkpoints after those
corrections, keeping genuine integer-range calls intact. There are no extra
draws added to manufacture compatibility with the old stream.

## Second migration: garden and title events

The SDL startup handler and native `AmigaMenu` preparation now use one-word
choices for `objTree` (six arguments) and `objGrass` sprite selection (four).
The VM retains genuine `irandom` for `groundPlayerSet`'s sleeping pose,
`groundTrees` frame and grass frame; `soundLand`/`soundFootstep` sample lookup
uses `irandom_range`. Those calls were deliberately not replaced.

Title Alarm 0 and Step 0 now use `TitleParticleRandom` for their currently
unrendered effects' stream consumption. The VM's right-to-left argument order
is retained: burst movement speed, animation speed, sprite choice, y, x;
rising particles first choose whether to emit, then animation speed, sprite,
y, x. `parentMovingFx` Create contains no additional random draws. This does
not implement the missing particle visuals or prove native title-event parity.

Checks:

- Native garden reconstruction matches SDL and an independent raw-word VM
  call sequence for each randomized object, plus all sixteen final RNG words.
  Menu Create consumption is **77 words**, replacing the old 92-word result.
- Title tests assert argument-call order and both conditional outcomes;
  128 seeds each check a twenty-particle burst and 100 rising-particle Steps
  against a raw-word oracle. RNG tests pass normally and with sanitizers.
- The broad startup test reaches the gameplay entrance but **fails** its
  stale fragment-four assertion: current output is fragment 2, unmirrored,
  six fragment rows / 35 total rows, 92 new instances and 89 fragment draws.
  Log: `/tmp/downwell-startup-choose-test.log`. Downstream geometry-specific
  assertions are not yet rerun; do not report the full startup suite as passing.
- Both ordinary builds pass, including Amiga 68000 instruction/helper audits
  (`/tmp/downwell-startup-choose-{amiga,sdl}.log`). No emulator was launched.

For the active RNG audit and checkpoint work, see [the work queue](open-work.md).

## Third migration: shared gems and immune bullet impacts

`objCurrency` Create VM chooses signed image speed from four entries and its
heading from two already-evaluated integer ranges. `GemAnimation` and
`GemHoming` now use single-word choices. Both `irandom_range` heading candidates
still execute first, in reverse argument order, consuming two words each.
The implemented creation prefix (velocity ranges, animation, heading) therefore
uses eight words rather than ten. This does not claim completeness of the
source's subsequent terrain-embedded spawn handling or other area branches.

`scrTypicalBulletDamage`'s immune sound selection now uses one-word
`choose(30,31)` in the shared `StandardBulletImpact`. The integrated native
runtime test correctly expects two words for a first-step shot absorbed by a
turtle: one for spread and one for ricochet. The machine-gun verification fake
and focused fake RNGs expose `chooseIndexSmall`, preventing an accidental
return to the integer-range interface.

Independent real-RNG tests cover 1,024 gem creations (values, heading order and
complete RNG state) and 1,024 immune impacts (sound, stream and no damage or
reward). RNG tests pass normally and under address/undefined-behavior
sanitizers; gem-motion, enemy-contact and machine-gun trajectory tests pass.
Both ordinary builds pass, with clean 68000 instruction/helper audits
(`/tmp/downwell-gem-choose-{amiga,sdl}.log`). No emulator was launched.

Gem CPU fixture version 6 has fingerprint `c57fd381461e0fdd`, coverage 1023.
It includes the one-word first-Step bounce-sound choice and retained
`allSet`/`bounceSnd`/`mySound` fields. Version 5 (`e526fc84c6bf6315`) corrected
Create choices but omitted that Step draw; the earlier `f175194dfba46820`
encoded the two-word chooser bug. These are versioned CPU-equivalence fixtures,
not independent Windows gameplay captures. Full natural-route RNG ordering,
player/reward callbacks and enemy alarm choices remain subject to the live
call-site audit in `open-work.md`.

## Fourth migration: shop callbacks

`scrShopAssignText` VM selects one of three farewell strings with `choose`.
`Shop_Step_0` likewise uses `choose(1,2,3)` for its merchant-response branches.
Both were incorrectly routed through two-word integer draws. Shop creation now
requires a separate farewell-choice callback; both adapters supply
`chooseIndexSmall(3)`. Shared player/shop interaction supplies
`1 + chooseIndexSmall(3)` for the response. Inventory rejection sampling remains
on `irandom_range`, with the farewell choice invoked synchronously afterward.

The remaining callbacks inspected here are genuine range calls: shopkeeper
Step/Alarm 1 use `irandom_range(20,60)` and ground-turtle Alarm 1 uses
`irandom_range(120,300)`. They remain unchanged. Frog alarm behavior does not
draw RNG; the existing bat and snail motion/hit paths use real-valued random
draws rather than `choose`. This does not establish completeness of missing
effect/sound/event behavior elsewhere.

Tests distinguish the stock and farewell callbacks and retain exhaustion's
no-draw contract. Another 2,048 real-RNG shop creations (128 seeds, two health
states, eight slots) compare stock, farewell and all RNG words against explicit
source-order range/rejection/choice operations. RNG sanitizer tests, shop-world
tests and the host-compiled Amiga runtime suite pass. Both ordinary builds pass,
including the 68000 instruction/helper audits. Evidence:
`/tmp/downwell-shop-choose-runtime.log` and
`/tmp/downwell-shop-choose-{amiga,sdl}.log`.

Next: regenerate decoded startup checkpoints and rerun affected end-to-end
routes against the corrected stream, fixing input tapes or actual behavior
where they fail rather than substituting the former RNG sequence.

## Regenerated startup checkpoints and natural routes

Decoded `StartupRuntime::skipToGameForTesting` now yields:

| Seed | Words consumed | Side-room rows |
| --- | ---: | --- |
| 2 (ordinary native entry) | 331 | 8, 3 |
| 4 (native natural-route fixture) | 325 | 8, 4 |

The counts include the placed start-field radius draw before first-level
self-destruction. Compare both shortcuts against decoded startup for draw
count, state index, all sixteen state words and the pre-builder entry state.
These are cached startup snapshots, not compensating draws added to gameplay.

`verify-natural-exit` compares real-input routes through generated exits,
upgrade selection and next-level entry. The seed-4 controller also visits a
generated shop, tests insufficient funds without changing money or stock, and
returns to the shaft. `amiga/verify_natural.gdb` pins the corresponding target
timings, presentation counters and stack guard; regenerate its expected counts
from the paired host route whenever source-backed RNG changes affect terrain.
Do not weaken the underlying encounter or result checks to retain old tapes.

The startup fixture's geometry and route counts are implementation regressions,
not independently captured Windows trajectories. Host sanitizer checks do not
prove target stack safety. Emulator checks with expanded Fast RAM do not prove
stock-A500 memory fit; those requirements remain in `open-work.md`.
