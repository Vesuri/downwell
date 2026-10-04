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

### Uniform-middle publication experiment

At `5a96d1f1`, frame 645 still spends 25.26 ms in the ammo HUD. An experiment
coalesced consecutive equal animation states, composed their non-overlapping
middles directly from prepared words, and left the existing renderer to paint
the edges. This removes intermediate fill/stamp passes without constructing
general interval lists, but still compares and publishes each middle line.

It failed the worst-frame gate: frame 645 changed from 115.110 to 116.250 ms,
701 from 177.772 to 177.675 ms, and 844 from 127.130 to 131.728 ms. Across 18
common complete frames, the mean changed from 78.356 to 79.357 ms. The new
constant-column publisher alone cost 8.20 ms in frame 645. Skipping intermediate
composition is insufficient when final per-line publication remains expensive.

Evidence: `tmp/trace/ammo-uniform-mode0`, `tmp/trace/ammo-uniform-mode1`,
`build/ammo-uniform-comparison.log`, and `build/ammo-uniform-645-cost.log`.
The captures use the contact tape, starts 644/699/843, 30 PAL fields each,
and the prepared backing-5 field bundle. The rejected source is retained only
in `tmp/ammo-uniform-rejected.patch`; it is not a default-off implementation.
Its `AMMO_UNIFORM_RUNS=0/1` selector produced different text hashes
(`d92e1637acce480aa3a9d977c23f26db0c54d76a3e05fe298187e42939210aa2` and
`1cc10c85d690c0f2d8a192bddfe914a2231a596e4cf5ab61559c77fafac0cccd`).
Thus this is not a text-identical timing comparison; use nonzero selector
values for both arms of future retained-path comparisons to avoid moving a
zero-initialized selector between BSS and initialized data. The measured
publisher cost and regression do not justify retaining this implementation.

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

## Prepared large-field columns

The opt-in `PREPARED_FIELDS=1 PREPARED_FIELDS_BACKING=2` path replaces runtime
row expansion, sparse coverage-map construction and saved-word reconciliation
with offline column packets. It restores old masks from the pristine terrain
ring and consumes prepared colour words in source order. Its ordinary-memory
bank is 73,024 bytes, deduplicated across radius phases, with no per-X-shift
expansion or additional Chip allocation. The portable raster remains the pixel
oracle; this measurement does not establish Windows GPU raster equivalence.

Reproduce the matched level-exit comparison with the same bundle described
above and `TRACE_FRAMES=2,30 TRACE_FIELDS=30`; use
`PREPARED_FIELDS_BACKING=1` for the sparse-large reference and `=2` for columns.
The captures are `tmp/trace/field-columns-reference.GgLmuV` and
`tmp/trace/field-columns-candidate.XIRhwg`; both executable text hashes are
`7c56e06215ce1aa1a5951d24c9c284563c2430780fb237b59b92587fac72052b`.
The earlier 2/10/30 markers overlap with the faster renderer: use separated
windows, not an incomplete capture set.

| Complete level-exit frame | Sparse large (ms) | Columns (ms) |
| ---: | ---: | ---: |
| 3 | 90.17 | 34.86 |
| 4 | 88.13 | 35.57 |
| 5 | 106.68 | 39.81 |
| 6 | 146.46 | 88.39 |
| 31 | 102.12 | 39.40 |
| 33 (unchanged raster) | 19.19 | 19.46 |
| 36 (unchanged raster) | 21.22 | 18.73 |

Changing-raster preparation falls from roughly 56–71 ms to 8–10 ms, but
drawing still costs 12–13 ms. The 48.5 ms `selectExitMaskBank` operation in
frame 6 is unchanged. Row-footprint inspection falls from about 4.3 ms to
1.4 ms, but whole unchanged-raster frames can still vary with other row work
and display synchronization; frame 33 is not a whole-frame improvement.
These remaining costs still miss the 5 ms field and 20 ms whole-frame targets.

The matching contact windows 644/699/843 use
`tmp/trace/field-columns-contact-reference.BjQUfk` and
`tmp/trace/field-columns-contact-candidate.FeBaHF`, with shared text hash
`9531cba7be3e8e446338f670b5eeae6e94e3213f194f638ee739efbf57a89f72`.
Frames 645/701/844 cost 816,590/1,262,490/901,690 cycles in the reference and
816,570/1,262,450/901,280 in the column mode. They do not regress, but those
sub-millisecond differences are not a combat performance win.

## Large-field representation limits

`make analyze-field-layouts` counts geometry work and uncompressed bitmap-bank
storage using the generated portable rasters; it is not a native timing model.
For independent, uniformly distributed Q8 radius pairs, excluding identical
rasters, retaining identical whole columns eliminates only 3.4–4.2% of the
word visits at circle X positions 110, 190 and 269. Word deltas still require
42.4–47.4% as many destination-word visits as the current restore-plus-draw
passes, before dirty rows, mixed-circle ordering, clipping and traversal cost.
These are full-vertical-extent geometric counts, not observed contact-tape
frequencies or predictions of equivalent cycle savings.

A vertical-run experiment deduplicated translated colour sequences and moved
vertical clipping, wrap splitting and interior row-footprint marking out of
the per-word loop. It added 36,916 ordinary-memory bytes, with no new Chip
allocation. Exhaustive radius/shift pixels and the existing mixed-field host
tests passed, but the native result did not justify keeping it. The paired
2/30 level-exit captures are `tmp/trace/field-runs-reference.h7eIC5` and
`tmp/trace/field-runs-candidate.dQP6hY`; both text hashes are
`7a4fbf07f8e3c1b37bf5af7b604de56517edd9daa41c31ded7112ce13c91a8f3`.
Across selected complete frames 3–36, field transfers fall only from 18.83 to
16.34 ms, while complete frames average 39.46 versus 39.57 ms. Frame 6 changes
from 88.42 to 88.66 ms and frame 33 from 19.59 to 22.22 ms; presentation waiting
absorbs the modest CPU saving. The experiment was removed. Its recoverable
patch is `tmp/field-runs-rejected.patch`; it is not another production option.

The bitmap counts also rule out blindly expanding every large circle into a
full masked sprite. Across the 21 distinct rasters, single full-height images
need 594,084 bytes. Bands of 16 lines reduce that to 87,752 bytes but require
32 rectangles per unclipped circle on average; 32-line bands need 146,360
bytes and 16 rectangles. Translation-deduplicated one-word-wide fragments
need 51,204 bytes of colour/mask pixels and about 33 rectangles per full
circle. Counts exclude additional placement/index metadata, runtime clipping,
and DMA setup costs; view clipping can reject many rectangles. A DMA design
needs an explicit resident-bank or staging budget and native blit measurements,
not a claim that these host counts already achieve the field budget.

## Large-field row-restoration experiment

Replacing masked restoration with existing terrain-row DMA did not improve
complete-frame pacing enough to retain. The experiment retired old field
ownership transactionally, scheduled at most four physical spans covering at
most 80 rows, synchronized the pristine terrain, and repainted fields before
actors. Tall side arcs and larger/more fragmented footprints retained masked
restoration. It added no Chip allocation.

The first version scanned physical row footprints and queried existing row
debt, costing 4.0–4.7 ms just to schedule restoration. Preparing exact clipped
footprints offline reduced that scheduling to about 1.5 ms. Deduplicating
16-position X blocks required 8,106 ordinary-memory bytes for all 512 radius
phases and centres -21 through 401. Host checks covered 219,648 X/radius
footprints, 32,768 clipping/wrap raster cases and transactional row restoration;
the composition tests also passed address/undefined-behavior sanitizers.
The earlier scanning version passed the native full-buffer fixture, but the
offline-footprint variant was rejected on timing before repeating native pixel
and contact/FPS acceptance gates.

The final matched level-exit traces are
`tmp/trace/field-row-spans-reference.NXx44p` (masked restoration) and
`tmp/trace/field-row-spans-candidate.zoHkQ8` (prepared spans plus terrain rows).
Both executable text hashes are
`42c8e996bec15f9d2374410823c49059cf1cf0bf5863faf25bad3d869960ddee`.
Use the prepared-column bundle and 2/30 markers described above; the discarded
experiment selected its candidate with `PREPARED_FIELDS_BACKING=3` and its
reference with `=2`. That candidate option is no longer in the build.

| Complete level-exit frame | Masked restore (ms) | Terrain rows (ms) |
| ---: | ---: | ---: |
| 3 | 34.93 | 32.82 |
| 4 | 35.78 | 33.72 |
| 5 | 39.48 | 41.27 |
| 6 | 88.40 | 88.20 |
| 33 | 19.72 | 21.76 |
| 36 | 18.97 | 20.91 |

Selected complete frames 3–36 total 5,326,870 versus 5,325,730 cycles: about
39.46 versus 39.45 ms per frame, effectively unchanged. New terrain copies
cost roughly 4–6 ms on changing-field frames; drawing remains 12–13 ms.
Average presentation waiting rises from 2.09 to 4.10 ms in the selected window.
Moving restoration into the row owner alone therefore does not solve the field
budget. The source and generated-table changes were removed; the recoverable
patch is `tmp/field-row-restoration-rejected.patch`. Do not revive this as
another production option or count its host correctness checks as a speedup.

## Resident large-field bands

`PREPARED_FIELDS=1 PREPARED_FIELDS_BACKING=3` uses resident 176-pixel top/bottom
arc images for circle centres 182–198. Other placements keep the column path.
The images contain white coverage and total coverage; the framework Bitmap
copy path derives red through the complemented-source masked blitter minterm.
Images are generated offline and copied into Chip once on gameplay entry, not
converted, copied or expanded for each field. Mixed-circle source order,
mask-only restoration from pristine terrain, clipping and physical ring wraps
remain unchanged. See the [memory contract](amiga-memory.md): the bank costs
71,456 Chip bytes, reusing the unused scene-bank tail before extending it.

The paired level-exit traces use the prepared-column bundle with
`TRACE_FRAMES=2,60 TRACE_FIELDS=30`, `SKIP_LEVEL=1 PROFILE_CONTACT=0`, and
`PREPARED_FIELDS_BACKING=2` versus `=3`. Captures:
`tmp/trace/field-bands-mode2.ppMp7b` and
`tmp/trace/field-bands-mode3.Na6tNt`. Both text hashes are
`1187ebd6ca2a38747923a25d33dcbd6f0ac52b7c3140f378674a0bc024fd2ba9`.
Use the reporter's paired-field DMA-stall accounting; rejecting those boundary
records or dropping the stalled cycles would invalidate the comparison.

| Complete level-exit frame | Columns (ms) | Resident bands (ms) |
| ---: | ---: | ---: |
| 3 | 34.84 | 25.78 |
| 4 | 35.72 | 32.33 |
| 5 | 39.67 | 26.59 |
| 6 | 88.42 | 82.54 |
| 11 | 37.92 | 38.88 |
| 70 | 39.53 | 28.06 |

For the intersection of fully captured frames (3–15 and 61–74, 27 frames),
mean complete-frame cost falls from 41.13 to 33.01 ms. Frame 11 illustrates
that presentation-phase changes can still make individual non-gate frames
slower. Preparation plus drawing remains about 11–16 ms, not the 5 ms target;
the 48.5 ms exit-mask selection in frame 6 is still present.

Matched contact captures are
`tmp/trace/field-bands-contact-trace-2.xgHHKj` and
`tmp/trace/field-bands-contact-trace-3.kZsfsH`, with shared text hash
`e3fb235789fc864e3ea0bad25f0dd8672890178dba5dfa59d3d242b1a027f307`.
Frames 645, 701 and 844 are cycle-identical between modes at
817,250 / 1,263,190 / 901,780 cycles. The field improvement is not a claim of
faster combat or completion of the whole-frame budget.

The marker-free FPS runs (`TRACE_FRAMES=` and `PROFILE_CONTACT=1`, same
bundle and modes) both take 3,390 PAL fields for 1,800 updates, with a
nine-field maximum at frame 701. Their shared text hash is
`159659ca9fb27b7d4d04af414f2bdcef7d1cdf29da34c3959c5828b106f4cac4`.
Logs are `build/field-bands-pure-fps-2.log` and
`build/field-bands-pure-fps-3.log`. The earlier 3,391-field runs included
inert trace markers and are not the marker-free FPS baseline.

The final native pixel verifier compares 1,979 full buffers through 1,800
contact updates, plus 512 terrain-mask phases and 131 mixed/shifted/wrapped
composites, with zero failures. Executable hash:
`5d331a14d0be6a896020026be32673a79d33dd2fb09df8410833b1a84d81095b`;
text hash `5c1c64ce12c0e6a10b3f29bcd993b0ef86309172208671ca8a15e5e5075873a5`.
It measures 11,520 Chip bytes free after allocation. Host raster tests cover
all 8,704 admitted radius/position combinations, clipping and restoration,
and pass address/undefined-behavior sanitizers. These pixel oracles are the
shared portable raster, not evidence of original Windows GPU coverage.

The cross-CPU replay also passes with the same optimization bundle and
`REPLAY_VERIFY=1`, using `build/port-replay-reference-table-atan` as the host
oracle (matching `TABLE_CONTACT_ATAN=1`). The tested native executable hash is
`cb6c4c7205721c0553a05d8564b34dc16f9b91c75ea61c8ddaf388c04fbaa30e`;
the command/result log is `build/field-bands-cross-replay.log`. This checks
shared state and RNG, separately from the native DMA pixel verifier.

### Resident-band transfer preparation

The first scheduling experiment queued transfers with blitter priority off,
then enabled priority only while draining. It lost: the 28 common complete
frames in `tmp/trace/field-batch-mode3.ZDRnaH` and
`tmp/trace/field-batch-mode4.4MyLJ9` rose from 35.003 to 40.092 ms.
Both use text hash
`1e8ed1484a17122905e5613bdc6abe4cee174c5e744fcbf946e78c3895f75b92`.
That source is removed; `tmp/field-batch-rejected.patch` preserves the rejected
experiment, not a supported option.

The subsequent prepared-geometry experiment keeps the accepted early-priority
policy. It removes repeated Bitmap construction and horizontal mask, shift,
word-count and modulo calculation from band painting/restoration. The native
audit attributed roughly 4 ms to this repeated setup. `BitmapCopyPlan`
prepares 17 painting and 17 restoration geometries when binding the band bank;
only vertical placement and height remain variable at submission. The generic
Bitmap implementation remains compiled as the comparison.

Here `PREPARED_FIELDS_BACKING=3` selects the generic resident-band path and
`=4` selects prepared geometry, not the rejected scheduling experiment.
The paired level-exit captures are `tmp/trace/field-plan-mode3-reference` and
`tmp/trace/field-plan-mode4-candidate`, using the same optimization bundle,
`SKIP_LEVEL=1 PROFILE_CONTACT=0 TRACE_FRAMES=2,60 TRACE_FIELDS=30`.
Their text hash is
`be7125e90247702794c28757145f23ea82a78ff6e3b1dbbc4b2e578a3f4a8395`.
Across the 30 common complete frames, mean cost falls from 36.240 to
31.576 ms. Frame 6 falls from 75.40 to 73.29 ms; individual presentation-phase
frames can get slower (frame 4: 27.61 to 33.19 ms). This is not a claim of
meeting either the whole-frame or the under-5-ms field budget.

Native pixel checks pass 1,979 full-buffer comparisons, 512 terrain-mask
phases and 131 overlapping/shifted/wrapped composites. The verifier executable
hash is `4f493ead5336dec2f51d8649a54ebc1f1b8270a662dcad264c51ccaad3c8dd44`;
its text hash is
`60d56ab9359609e2dff4b6d9d394a5dc4c25858af4dda0c79c5fec49bfbd55fe`.
`make verify-bitmap-copy-plan` independently models the blitter channels and
checks 340 paint/restore/shift/height/complement cases, including untouched
edges; address and undefined-behavior sanitizer checks also pass.

Matched contact captures `tmp/trace/field-plan-contact-reference` and
`tmp/trace/field-plan-contact-candidate` have the same text hash
`68ea21a0ae84dd4e5c1a8132c38692c3f3211592eb2c1fc4dbdd26d5fec20b32`.
All 20 common complete frames are cycle-identical. Mandatory frames
645 / 701 / 844 remain 817,260 / 1,263,090 / 901,750 cycles respectively.

Marker-free contact FPS runs (`TRACE_FRAMES=`) take 3,390 PAL fields for the
generic path and 3,389 for prepared geometry, over 1,800 updates; both retain
the nine-field maximum at frame 701. This one-field tape difference is not
a meaningful combat improvement. Logs are `build/field-plan-fps-3.log` and
`build/field-plan-fps-4.log`, with common text hash
`eff94c9c8379314a436b587d9ffe4ab1e5765fed33b15a944289cd1c12625113`.

Cross-target replay passes with the same optimization bundle,
`REPLAY_VERIFY=1 TABLE_CONTACT_ATAN=1`, and the matching
`build/port-replay-reference-table-atan` host oracle. Executable hash:
`9638a9db16593b41a0b357b1dc3d0ee550c8ec79fd92f05c0ef10e1c762f443e`.
The result is in `build/field-plan-cross-replay.log`; the normal build is
restored with clean 68000 instruction/helper audits. Prepared geometry remains
opt-in pending the full scene coverage and frame-budget gates.

### Rejected adjacent-raster XOR packets

A differential experiment replaced full old-mask restoration and new painting
when one stationary large circle owned an unchanged terrain footprint. The
identity per plane was exact:

`target ^= oldColour ^ newColour ^ (terrain & (oldCoverage ^ newCoverage))`.

Offline adjacent-raster packets could compose reversibly in either direction.
This avoided storing every old/new raster pair: the 21 frames have 226,180
changed words across all directed pairs (2,261,800 bytes at ten bytes per
command, before directories), versus 3,282 words / 32,820 bytes for adjacent
forward edges reused in reverse. The 21-entry offset directory added 42 bytes
of ordinary memory and no Chip allocation. Reproduce the representation audit
with `c++ -std=c++17 -O2 tools/audit-field-deltas.cpp -o build/audit-field-deltas`
and `build/audit-field-deltas`. The visible-word counts are conservative whole
word bounds before partial viewport-edge masks.

The candidate admitted at most four adjacent edges, required unchanged X/Y
and ring origin, and rejected dirty old/new arc rows or multiple fields.
Host checks passed 24,038 reversible placements and 3,400 ownership lifecycle
frames (3,196 admitted updates), alongside existing mixed-size, dirty-row and
double-buffer cases and address/undefined-behavior sanitizers. Those checks
did not establish speed.

Paired native captures `tmp/trace/field-delta-mode4` and
`tmp/trace/field-delta-mode5` used the resident prepared-transfer bundle with
`SKIP_LEVEL=1 PROFILE_CONTACT=0 TRACE_FRAMES=2,60 TRACE_FIELDS=30`.
Their common text hash is
`8ea4f7cad773183c13cf46a4d03e671df0f12665f14db88a9946d2c1654317de`.
The 29 common complete frames worsened from **32.175 to 39.314 ms**.
Frame 61 rose from 33.01 to 68.17 ms; frame 70 from 27.14 to 66.92 ms.
The differential routine itself averaged 5.72 ms over all captured frames,
including frames that did not use it. Dirty-footprint checks added work too.
Eliminating full painting did not compensate for chained per-word CPU updates.

The candidate, generator extension and experimental tests are removed; the
recoverable patch is `tmp/field-delta-rejected.patch`. Native pixel/contact/FPS
acceptance gates were not pursued after this timing failure. Do not revive
this as a renderer option, hide it behind a narrower current-tape special
case, or count its host checks as an A6 performance gain.

### Rejected prepared transfer-list batch

This experiment prepared hardware controls at bank binding, described all
field transfers before DMA, and executed the list synchronously through the
framework. It removed per-transfer generic queue construction and IRQ-state
writes, while retaining the same coverage/pixel buffers, masks and blit sizes.
The estimate targeted about 3–4 ms of repeated submission/setup in the native
audit; DMA waiting itself was not assumed removable.

Native pixel verification passed 1,979 full-buffer comparisons, 512 mask phases
and 131 overlap/wrap composites; free Chip remained 11,512 bytes. Verifier
executable hash: `6ec1c784c47aa561d135582414777002df10d587c411a93e3f10ac94c4295c98`;
text hash `50378251b7d35bd3ad8e09ab2110ab04636aafe3d04778048481db98625ac47e`.
Host job decoding and sanitizer checks also passed all 340 transfer cases.

Paired captures `tmp/trace/field-jobs-mode4` and
`tmp/trace/field-jobs-mode5` use `SKIP_LEVEL=1 PROFILE_CONTACT=0`,
`TRACE_FRAMES=2,60 TRACE_FIELDS=30`, and the same prepared-field optimization
bundle as above. Their shared text hash is
`c583b959f1263793554d2af127bd6fc0b945da426da88e740f4ff58fa25a5031`.
The 28 common complete frames worsen from **32.360 to 40.040 ms**.
The candidate spends 8.62 ms/frame in `blitterWait` and 9.09 ms/frame in
presentation waiting; field preparation plus drawing averages about 14.2 ms.
Prebuilding the list did not eliminate DMA occupancy or improve complete-frame
pacing. The implementation is removed, with a recoverable ignored patch at
`tmp/field-jobs-rejected.patch`. Contact/FPS/replay acceptance gates were not
pursued after the timing failure.

`tools/audit-field-band-tiles.cpp` measures another possible tradeoff without
changing runtime code. Build with
`c++ -std=c++17 -O2 tools/audit-field-band-tiles.cpp -o build/audit-field-band-tiles`
and run `build/audit-field-band-tiles`. It bounds subrectangles over the existing
resident images, so no new Chip image bank is needed. Counts cover 42 arc
images and all 17 admitted horizontal phases, before vertical clipping/wrap:

| Horizontal pieces | Fetched words | Transfers | Words saved per extra transfer |
| ---: | ---: | ---: | ---: |
| Existing whole arc | 303,688 | 714 | — |
| 2 | 323,226 | 1,428 | -27.36 |
| 3 | 222,818 | 2,142 | 56.63 |
| 4 | 190,530 | 2,856 | 52.83 |
| 6 | 153,800 | 4,242 | 42.49 |
| 11 | 126,588 | 7,770 | 25.10 |

These are geometry counts, not a timing win. Edge-word fetches make a two-piece
split worse even before setup. Finer splits reduce transparent DMA but sharply
increase transfer count. They must not be implemented on the assumption that
less fetched area alone makes them faster.

### Fused terrain/large-field composition experiment

`PREPARED_FIELDS_BACKING=5` combines pristine terrain and the new field in one
blitter pass. It replaces the old restore-then-paint pair inside new arc
rectangles; only departing old-mask strips are retired separately. The estimate
targets the previous 4–7 ms restoration pass, not sub-millisecond submission
helpers. It uses the existing resident bank and compiled mode-4 comparison,
without additional Chip allocations. The rectangle-ownership contract is in
[Amiga memory](amiga-memory.md); overlapping fields, moved centres/origins and
retained laser pixels retain the masked path.

Host geometry checks cover 34,816 stationary old/new combinations, retiring
6,635 of 1,086,268 old rectangle rows. The lifecycle suite includes 2,040
ownership frames (1,649 fused compositions), dirty terrain, denied ownership,
mixed shapes, 128 pending replacements and fallback transitions. Both suites pass ASan/UBSan, and the
renderer suite checks pending versus retained laser ownership.

The bounded native `fields-clear` run with `PROFILE_LASER=1` completed 480
gameplay updates with 4,090 whole-buffer checks, 583 fused transfers and zero
pixel failures. The stricter report separates **510 fixture transfers** across
two scene-bank bindings from **73 actual exit transfers**. It subtracts 255
transfers per complete 437-composite fixture and requires at least 20 real exit
transfers. Log: `build/field-composite-clear-laser-final.log`. Native executable:
`5f34bd7e04a1b94e2a02521f12443208a482838e9859354985aef06eb2a0d538`;
text: `ec5f1c895f35977b0f95032b24b9abaa512aaf159e2885e4c3fcc19958051a39`.

Matched marker traces `tmp/trace/field-composite-final-mode4` and
`tmp/trace/field-composite-final-mode5` use the optimization bundle above,
`SKIP_LEVEL=1 PROFILE_CONTACT=0 PROFILE_LASER=0`,
`TRACE_FRAMES=2,60 TRACE_FIELDS=30`, and shared text hash
`407938d9242ee2fd4972233a7b9cc2b91d6730b8ee322602c2e1a9b0da167357`.
Across 37 common complete frames the mean improves **31.441 → 28.553 ms**.
The worst captured frame, frame 6, remains **73.784 ms** in both modes;
`selectExitMaskBank` accounts for 48.5 ms of it. Individual wait-phase-shifted
frames can worsen (frame 73: 27.114 → 34.315 ms), so this mean is not a
worst-frame or 20 ms success claim. Stationary fused preparation generally
takes 1.1–2.6 ms, with drawing still around 5.3–7 ms.

The mandatory contact captures (`tmp/trace/field-composite-contact-mode4` and
`tmp/trace/field-composite-contact-mode5`, `SKIP_LEVEL=0 PROFILE_CONTACT=1`,
`TRACE_FRAMES=644,699,843 TRACE_FIELDS=30`) have shared text hash
`d93fd99d8f5427600a8f6d41584dd3d70be41b82994549841993a5362975f511`.
All 19 common complete frames are cycle-identical, including frame 645
(817,270 cycles / 115.026 ms), 701 (1,260,870 / 177.460 ms), and 844
(905,010 / 127.375 ms). This is a no-regression result, not a combat gain:
that workload does not admit stationary large-field fusion.

Marker-free contact tapes in both modes complete 1,800 updates in **3,392 PAL
fields** (26.53 FPS), with the same worst gap of nine fields at frame 701.
Logs are `build/field-composite-fps-mode4.log` and
`build/field-composite-fps-mode5.log`; the shared text hash is
`8a87bdab95b0724327aeb7ad7f7f951f58d6b4eb79f851a97ca27b279127f945`.

Cross-target replay passes against `build/port-replay-reference-table-atan`,
including the per-frame trace and component digests. Native replay executable:
`6a9f439cbf1413f016832b5615e38d2e04f56b5d1059e3ad8770358411fcd290`;
log: `build/field-composite-cross-replay.log`. The matching compact-angle host
oracle is intentional; no shared simulation code changed in this experiment.
The normal playable executable was restored, with clean 68000 instruction and
64-bit helper audits.

Retain this measured pass elimination as an opt-in improvement, not completion
of A6 or permission to enable it by default. The under-5-ms field target and
broader placement/coverage gates remain open.
