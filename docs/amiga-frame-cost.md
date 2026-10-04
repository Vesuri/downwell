# Amiga gameplay frame-cost audit

This audit records where native A500 gameplay time goes at revision
`b99c2f69` (2026-10-04), and why. It replaces the 2026-09-29 audit at
`b89ff875` (5,837 fields on the same tape; see that revision of this file).
The work it implies is queued in
[section 5 of the work queue](open-work.md#5-meet-the-amiga-frame-budget).
Do not update this file as that work progresses. Record later measurements in
the commits that make them, and repeat the audit with the same method when the
representation changes. The figures are emulated-machine measurements of one
deterministic tape, not guarantees for other scenes.

Measurement correction: the original report counted each capture's first
frame as complete. Recording starts at the next VBL, so that frame may omit
simulation work. The corrected report excludes both boundary frames. The
historical aggregates below therefore include partial first frames, and the
frame-844 instruction total is a lower bound, not a complete-frame baseline.
The FPS-only field counts are unaffected. For new comparisons capture from
644, 699 and **843**, and compare interior frames 645, 701 and 844; do not
compare a complete frame against one of the old first-frame fragments.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO.
- Tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1800 gameplay updates. It covers
  natural descent with one damaging contact, view Y 39 to 1874, at most five
  allocated enemy records and final health 3.
- FPS-only build `93110cb8…`: 3,821 PAL fields, 23.55 FPS, 42.5 ms per frame
  on average. The worst gap is frame 645 at 9 fields. Frames 701 and 844
  also take 9 fields.
- [Instruction traces](amiga-profiling.md#cycle-exact-instruction-traces):
  build `90588bfd…` captured 100 fields from each of frames 50, 250, 450,
  644, 690, 844, 900, 1100, 1300, 1500 and 1700 (524 complete frames). A
  second build captured frame 701, starting at frame 699. The trace builds
  differ only in their compiled frame list.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=300 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=3000 \
  TRACE_FRAMES=50,250,450,644,690,844,900,1100,1300,1500,1700 bash amiga/profile.sh trace
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=30 TRACE_FRAMES=699 \
  TRACE_OUTPUT=$PWD/tmp/trace/run701 VERIFY_TIMEOUT_SECONDS=900 bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/run --frames=1300-1338 --causes
```

## Frame cost by scene

The gameplay step overlaps the previous frame's publication wait, so traced
frames contain almost no idle time. One PAL field is 20 ms (142,102 cycles).

| Tape frames | Scene | Mean ms | Median | Worst | Over 20 ms |
| --- | --- | ---: | ---: | ---: | ---: |
| 50–91 | entrance bowl, firing | 29.9 | 28.1 | 51.0 (84) | 42/42 |
| 250–303 | open descent | 23.0 | 17.4 | 75.0 (278) | 22/54 |
| 450–470 | enemies in view | 64.3 | 51.3 | 117.9 (464) | 21/21 |
| 644–661 | damaging contact | 104.2 | 101.4 | 179.5 (645) | 18/18 |
| 690–700 | combat with gems and smoke | 88.8 | 84.8 | 116.8 (694) | 11/11 |
| 701 | pattern generation burst | 184.5 | | | 1/1 |
| 844–864 | generation burst with a time field | 89.4 | 83.8 | 179.7 (844) | 21/21 |
| 900–920 | time fields present | 66.6 | 63.9 | 101.6 (907) | 21/21 |
| 1100–1737 | camera at the level bottom | 32.5 | 25.5 | 69.7 (1544) | 220/223 |

## Cost by owner

`--causes` charges each sample to the innermost recognised owner on its
logical call stack; helpers are charged to their callers. Milliseconds per
frame:

| Owner | Entrance | Descent | Enemies | Combat | Fields | Quiet 1300–1338 | Frame 645 | Frame 701 | Frame 844 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Game logic | 8.4 | 5.9 | 20.7 | 26.9 | 13.1 | 12.0 | 29.6 | 31.2 | 20.0 |
| Actors, sprites and display | 4.5 | 4.9 | 10.8 | 25.4 | 4.8 | 3.0 | 49.3 | 21.7 | 28.2 |
| Generation logic | 0.4 | 1.3 | 1.0 | 1.6 | 0.5 | 0.5 | 1.3 | 82.4 | 51.5 |
| Runner bookkeeping | 6.0 | 2.7 | 13.2 | 9.9 | 5.4 | 5.7 | 13.2 | 22.8 | 27.4 |
| Software float emulation | 2.2 | 1.6 | 4.8 | 6.5 | 2.5 | 2.6 | 46.1 | 9.2 | 7.1 |
| Ammo HUD | 3.6 | 0.1 | 5.5 | 3.3 | 7.5 | 4.0 | 30.5 | 0.1 | 12.5 |
| Time-field outlines | 0.1 | 0.1 | 0.1 | 0.1 | 28.2 | 0.1 | 0.1 | 0.1 | 23.4 |
| Terrain rows and scrolling | 0.4 | 1.3 | 2.7 | 6.6 | 1.1 | 0.2 | 4.1 | 8.0 | 5.5 |
| Effect surface (currency/smoke) | 0.6 | 0.6 | 2.2 | 5.2 | 0.6 | 0.6 | 0.6 | 5.1 | 0.6 |
| R-tree upkeep | 0.3 | 0.1 | 0.5 | 0.3 | 0.2 | 0.2 | 0.3 | 0.7 | 1.3 |
| Frame loop and other | 3.6 | 4.3 | 2.7 | 3.1 | 2.8 | 2.6 | 4.3 | 3.1 | 2.3 |
| Total | 30.0 | 23.0 | 64.3 | 88.8 | 66.6 | 31.5 | 179.4 | 184.5 | 179.7 |

The owner patterns are coarse: health-number drawing counts as actors and
display, and the per-actor runner walks partly count as game logic. The
anatomies below use the call trees.

## The three worst frames

Each takes 9 fields, for a different reason.

**Frame 645, damaging contact (179.5 ms).**
- 41.8 ms in `RunSession::contactDamageNumber`, almost all of it one
  `RunnerPointDirection::evaluate`. That call's `portableAtan` costs 246,892
  cycles (34.8 ms): 14 `divideBinary64` calls (10,265 cycles each) and 14
  `multiplyBinary64` calls in the software series.
- 29.9 ms in `redrawAmmo`: the stomp recharges ammo, so the whole gauge is
  redrawn while it jiggles.
- 25.7 ms in the health HUD. `drawHealthNumberLayers` draws each digit as
  layered CPU assets (14.1 ms), and `redrawHealth` takes 11.7 ms. Each of the
  23 `amigaRendererDrawAsset` calls costs 9,000 cycles.
- 11.1 ms in `AmigaGameplayActors::draw`, 5.9 ms of it damage text, plus
  9.1 ms of terrain stage preparation.

**Frame 701, pattern generation burst (184.5 ms).** `generateLiveTerrain`
takes 100.3 ms:
- Object creation: 62 `AmigaGameplayGeneration::create` calls cost 33.3 ms in
  total. Six `appendBreakables` take 11.0 ms and seven generic
  `GameplayRoomLayers::birth` calls 6.7 ms. Time-field adoption takes 3.1 ms.
- Run creation: 23 `createRun` calls cost 22.0 ms, including 7.1 ms of
  `publishSpan` and 7.1 ms of RNG advance.
- RNG: `GameMakerRandom::advance` outside runs takes 10.8 ms, 81 `nextU32`
  calls 8.0 ms and 39 `cavernEnemyChoice` calls 5.5 ms. That makes about
  26 ms of Well512 work in total. These draws are part of the original
  interleaving and must stay synchronous.

The rest of the frame is ordinary combat work: draw collection 10.7, actor
draws 10.2 and currency/smoke 9.6 ms.

**Frame 844, generation burst with a time field (179.7 ms).**
- Generation takes 77.5 ms. Thirteen `appendBreakables` cost 23.1 ms (12,636
  cycles each), and 15 generic layer births inside them 12.0 ms.
- `AmigaTerrainPainter::paint` paints the new rows with 28 CPU
  `amigaRendererDrawAsset` tile draws, 26.1 ms in total.
- Time-field preparation takes 18.5 ms and its drawing 4.8 ms.
- The ammo gauge takes 14.3 ms.

## Typical frames

**Quiet, camera at the level bottom (31.5 ms; 1300–1338).** Nothing exceeds
14% of the frame.
- `GameplayWorld::step` takes 18.5 ms. Of that: player 2.8 ms, walking
  enemies 2.7, the animation walk 2.1, alarm dispatch 1.2, activation
  reconciliation 1.1 and frog floor division 1.0. The remainder is spread
  across functions under 0.5 ms each.
- The ammo HUD takes 4.5 ms: row animation strips after shots, plus 1.2 ms
  of `AmmoHudDamage::update`. Draw collection takes 2.7 ms.
- Leaf instructions: memory operands 27%, branches 14%, multiply/divide/mem
  helpers 12% (3.7 ms), and `movem`, pushes and calls 13%.

The quiet frame has no hot spot. It is the cost of running the emulated
runner's per-actor walks and redrawing the gauge; it can only reach 20 ms
by removing those structures, not by tuning the functions in them.

**Combat with gems and smoke (88.8 ms; 690–700).**
- `GameplayWorld::step` takes 34.6 ms: currency 4.1, animation walk 4.1,
  bullets 3.5, player 3.3, snails 2.5 and many smaller parts.
- `AmigaGameplayActors::draw` takes 10.6 ms. Damage text is 5.5 ms of it,
  and 5.5 CPU asset draws cost 7,400 cycles each.
- Currency/smoke composition takes 9.7 ms for three stamps
  (`stampPixel` 13,872 cycles each).
- Terrain ring sync takes 6.4 ms (3.9 `Bitmap::copy` calls), draw collection
  6.3, stage preparation 4.1, the ammo gauge 3.8 and effect-surface
  publication 2.5.

**Enemies in view (64.3 ms; 450–470).** `GameplayWorld::step` takes 36.2 ms.
Captured alarm dispatch is 5.0 ms of it, the animation walk 4.0 and the
player 4.4. Ammo 5.0, draw collection 4.3 and currency/smoke 3.1 ms make up
most of the rest.

**Time fields present (66.6 ms; 900–920).** `AmigaFieldOverlay::prepare`
takes 22.5 ms per frame, 12.7 ms of it `amigaFieldWords`. Field drawing
takes 5.6 ms and the ammo gauge 7.2 ms.

## Ammo gauge experiments

Whole-gauge replacements measured on this tape and discarded include:
- Synchronous word blits from Chip layers cut recharge frames from 24 to
  17 ms. But each blit cost about 2,300 cycles, mostly waiting, so shot
  frames became slower and frame 844 took ten fields.
- A fused interior pass that writes each distinct line value once costs
  about 130 cycles per line in its store loop, the same as the existing
  passes.
- A retained final-column interval sweep, based on `e37042c6`, avoids the
  intermediate fill/stamp/publish passes and compares final intervals. It
  passes the HUD pixel oracle, but resolving source overlaps and comparing
  intervals costs 31.0 ms in frame 645. Total HUD time rises from 25.0 to
  42.4 ms; frame 645 rises from 115.0 to 132.2 ms and frame 844 from 145.9
  to 158.3 ms. The implementation is rejected, not a remaining default-off
  optimization. Evidence: `tmp/trace/ammo-column.IT7CIu`,
  `build/ammo-column-cost.log`; recoverable source:
  `tmp/ammo-column-sweep.patch`. The accepted comparison is
  `tmp/trace/ammo-blocks-sparse.VMxRkg`. Both use starts 644/699/843 with
  30 PAL fields per capture and the contact profile. Fewer pixel writes do
  not help when per-frame interval construction costs more than the removed
  rendering passes.

On this machine the gauge's cost is the number of lines rewritten, not the
way each line is composed.

## Consequences for the budget

- **Worst frames.** Each is roughly 180 ms. The largest removable parts are:
  - one software atan (35 ms);
  - layered health digits (26 ms);
  - whole-gauge recharge redraws (14–30 ms);
  - generic breakable/layer births (11–23 ms);
  - CPU terrain tile painting (26 ms);
  - time-field preparation (18 ms).

  Removing all of them still leaves the generation bursts at about 100 ms,
  their RNG draws and pattern emission.
- **Combat.** Combat is dominated by the per-actor simulation walks and CPU
  composition of actors, damage text, currency and smoke.
- **Quiet.** Quiet frames need about 11 ms removed. That equals the per-frame
  runner walks (animation, alarms, reconciliation, draw collection; about
  7 ms) plus the gauge's row strips (4.5 ms).

These projections are derived from measured component costs; each change
needs its own timing tape.

## Large-field traversal experiment

The contact tape does not measure large exit/start circles. Reproduce the
level-exit window with the prepared-small-field bundle from commit `bcca5bd4`,
but use `SKIP_LEVEL=1 PROFILE_CONTACT=0 TRACE_FRAMES=2,10,30 TRACE_FIELDS=30`
with `bash amiga/profile.sh trace`. Exclude partial capture frames as usual.
The baseline trace is `tmp/trace/field-large-baseline.PKnIdd`; its executable
text hash is `ed583f6ebda63c30d687500b5795d2eb8b74cfd1a41962be2f6fd862d036566e`.

In complete frames 3–36, large-field preparation costs roughly 56–72 ms and
drawing 11–17 ms when the raster changes. The prepared-row traversal and its
callback account for about 38 ms per sampled frame; saved-word construction,
coverage reconciliation, underlay capture and publication add more. Even
unchanged-raster dirty-row inspection takes 4.2–4.5 ms in frames 33, 36 and 37.

Simply removing the saved-word banks is insufficient. A tested direct-word
experiment restored each old mask from pristine terrain, then painted each
new circle in source order using `amigaFieldWords` again. Its two traversals
each cost about 40–48 ms. Frame 4 changed from 88.3 to 102.8 ms; frame 6 from
146.5 to 151.4 ms. It passed pixel tests but was withdrawn, not enabled.
The experiment patch is retained locally as `tmp/field-direct-words-rejected.patch`;
its trace is `tmp/trace/field-large-direct.uOVDeH` (text hash
`3e98ff2171517abd3389061699b302d506cccaf429082a03a0393fb925351e61`).
These two builds differ in code: use the large phase-cost differences as
architectural evidence, not sub-millisecond attribution.

Do not replace that traversal with another per-frame expansion or a second
generic pass. Removing its work requires a prepared representation that can
be consumed directly. An all-16-shift expansion of the existing 742 row
patterns would need 175,986 bytes of word payload, 23,744 bytes of phase
indices and 46,892 bytes of row offsets before other metadata; that naive
representation also needs a memory-design decision, not blind adoption.
