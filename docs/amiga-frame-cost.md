# Amiga gameplay frame-cost audit

## Complete initial fall against the 20 ms budget — `45e96a9c`, 2026-10-12

This was the open-work section 0 baseline before item A; the current state
is the [source-record terrain owner](#source-record-terrain-owner-section-0-a).
The sections after it are earlier measurements, kept for their evidence; do
not quote their numbers as the current state.

**Pace:** `bash amiga/profile.sh quiet-fall` gives **192 PAL fields for 164
updates (42.7 FPS)**, with a worst gap of 7 fields at update 163. 18 updates
miss at least one field: 1, 67, 72, 122 (3 fields), 136 (5), eleven updates
between 138 and 161 (2 fields each, 143 3), 163 (7) and 164. The landing witness passes: tick
164, y 262,144, health 4, ammo 8, 3 enemies, 1,371 RNG draws. Every update
within 20 ms would mean 164 fields.

**Traces:** cycle-exact traces of every update from 2 to 164, deterministic
A500 with 512 KiB Chip, 512 KiB slow and 8 MiB Fast. There are three
100-field captures because one capture holds at most 100 fields. Update 1 is
the scene-entry update and is excluded.

| Window | Updates | Work mean ms | Worst ms (update) | Over 20 ms | Excess over 20 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Authored entrance, 2–55 | 54 | 16.2 | 20.5 (19) | 1 | 0 |
| Quiet generation, 56–121 | 66 | 20.2 | 27.7 (56) | 32 | 115 |
| First burst, 122–135 | 14 | 22.5 | 55.0 (122) | 8 | 54 |
| Enemies, 136–162 | 27 | 31.4 | 94.3 (136) | 27 | 308 |
| Landing, 163–164 | 2 | 93.7 | 136.0 (163) | 2 | 147 |
| Whole fall, 2–164 | 163 | 21.8 | 136.0 (163) | 70 | 626 |

"Work" is the time from one frame mark to the next, minus the publication
wait (`AmigaPlatform::waitPublished`). Display buffering absorbs part of the
alternating over-budget updates in the quiet window, so the pace loses
fewer fields than the per-update excess suggests. The target is still every
update within 20 ms.

### What one update costs

**Quiet update without a new row (98): 15.6 ms.** No enemies and no
generation, only the player falling past the walls:

- **Step: 8.8 ms.** Player motion and landing 2.0, Step's own body 1.0,
  DitherAbove Begin Step 0.66, phase reconciliation 0.53, six per-update
  clears (`memset`) 0.50, RNG refill 0.44, End Step interior 0.35, copies
  0.29, an empty alarm phase 0.28, camera 0.50, Consumables 0.23, weapon
  level recomputed from the streak 0.20, enemy Step-order capture 0.24, shop
  End Step 0.15.
- **Outside Step: 6.8 ms.** Draw-list collection 1.73 (a room-actor walk of
  1.20), display row sync 1.23, frame loop body 1.39, sprite assignment 0.43
  and copper update 0.25.

**Row update in the quiet window (99): 24.6 ms.** The camera advances one
16-pixel row every second update, and generation and painting of the new
row happen in the same update. That adds:

- **Generation: 6.0 ms.** Native terrain records for one row segment 2.42
  (`constructNativeTerrainPrepared`, of which `constructTerrainSpan` is
  1.40), 20 RNG draws at about 510 cycles each 1.44, builder position
  publication 0.52 and raster invalidation 0.50.
- **Painting: 2.85 ms,** borrowed corridor rows.

Across the 12 row updates in 98–121, generation averages 5.24 ms and painting
2.84 ms per row.

**Generation bursts:**

| Update | Generation ms | Native terrain records ms (segments) | Event cells ms (count) | Builder position ms | Update total ms |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 56 | 10.9 | 7.35 (scalar fallback) | – | – | 27.7 |
| 122 | 39.3 | 21.90 (8) | 5.94 (4) | 2.28 | 55.0 |
| 136 | 75.3 | 32.34 (13) | 22.04 (15) | 3.87 | 94.3 |
| 163 | 97.0 | 32.60 (13) | 43.32 (32) | 4.17 | 136.0 |

- **Native records:** they cost about 16,000 cycles (2.2–2.3 ms) per
  prepared row segment, at a border-only row (99) and in every burst alike.
  The work is split over about ten structures that each record the same
  span:
  - identity table admission and birth;
  - room membership (`bindTerrainSpan`);
  - logical layer admission and order (`admitPreparedTerrainSpan`,
    `publishPreparedTerrainSpan`);
  - retained cells (`reserveRetainedCells`, `commitRetainedCells`);
  - the thin-platform retirement window and external registration;
  - per-cell flags and sprites.
- **Event cells at 163:** breakables 16.1 ms for 21 (5,458 cycles each),
  enemy adoption 11.4 ms, of which runner birth and binding is 8.5, the
  breakable random evaluation 5.1 ms and the enemy choice 3.8 ms.
- **Update 164**, the landing update, spends 39.9 ms in Step on the enemies
  generated at 163: their room effects 15.4 ms and alarms 8.3 ms.

**Ordinary updates with an enemy (137–162): 29.3 ms mean.** No generation
runs in this window, because the 136 burst generated these rows ahead of
the camera:

- **Step: 17.1 ms.** It is 8.8 ms on a quiet update; one crawler adds about
  8.3 ms. Its alarm dispatch is 3.0–4.5 ms, of which the crawler's own alarm
  is 2.45; the room effect (enemy Step) 3.05; native enemy animations 1.36;
  and contacts 0.25.
- **Painting: 8.9 ms per new row,** 13 rows, mostly live tile painting
  (`paintCached` 5.17). Updates 141 and 143 paint 16.6 and 17.9 ms.

### Reproduce

Build and environment are as in `quiet-fall`, using trace builds:

```sh
export DETERMINISTIC=1 STRIP=1 PROFILE_IDLE=1 PROFILE_FRAMES=600 SKIP_TO_GAME=1 \
  SKIP_INTRO=0 SKIP_LEVEL=0 PROFILE_CONTACT=0 PROFILE_SHOP=0 PROFILE_ENTRY=0 \
  PROFILE_LASER=0 PROFILE_OVERFLOW=0 PROFILE_NAVIGATE=0 VERIFY_CHIP_KB=512 \
  VERIFY_TIMEOUT_SECONDS=300 TRACE_FIELDS=100
for s in 1 60 120; do
  TRACE_FRAMES=$s TRACE_OUTPUT=$PWD/tmp/trace/fall-$s bash amiga/profile.sh trace
done
python3 tools/amiga-trace-report.py tmp/trace/fall-1 --frames=1-96 --causes > tmp/fall/report-1.txt
python3 tools/amiga-trace-report.py tmp/trace/fall-60 --frames=97-121 --causes > tmp/fall/report-60.txt
python3 tools/amiga-trace-report.py tmp/trace/fall-120 --frames=122-164 --causes > tmp/fall/report-120.txt
python3 tools/amiga-fall-budget.py tmp/fall/report-{1,60,120}.txt
```

- **Captures:** the three captures complete updates 1–97, 60–146 and
  120–178. Each run takes about five minutes and writes about 4 GB.
- **Per-update detail:** use `--frames=N --collapsed=FILE` for one update's
  folded stacks.
- **ELF SHA-256 of the trace builds** (each differs only in its trace
  marker):
  - start 1: `4d9ba62d412725ff7a48bb33a785eb8b027dc172662970ec8ca8540d17b1f99c`;
  - start 60: `ce94130b27b0498c439df5b529f10b164e1a9f774ee131bfaaf6f25f18542bec`;
  - start 120: `a76cedcf2e677dd220d280cc5bf651e76f8621692cb38948fa06a812773ee88d`.
- **Pace build executable SHA-256:**
  `173315759dd7be7ddc0decf0ac5c1b6162ecdefd23474330aa09f2808eef4625`.

### Terrain API-read census for section 0 A

Diagnostic counters on `bec4e288` plus the census instrumentation cover
updates 1–164; this is **not a timing measurement**. Reproduce with
`bash amiga/profile.sh terrain-reads`; validate the instrument with
`make verify-terrain-read-census`. The native capture checks the complete
landing witness and snapshots 165 cumulative samples without debugger stops
inside the fall. It restores the normal build on exit.

Two captures gave the same counts and executable SHA-256:
`c9c092860b3ddd41265981e02b5e2a5f9e386ab6ad9370bc72b4db330278a09e`.

| Instrumented API boundary | Outside construction | During construction |
| --- | ---: | ---: |
| Identity query / visited cell | 0 | 362 |
| Run publication query | 0 | 0 |
| Room presence query | 0 | 286 |
| Room activity query | 0 | 0 |
| Room animation skipped run | 28 | 0 |
| Room reconciliation / marked-cleanup run | 0 | 0 |
| Layer state query | 0 | 0 |
| Inactive layer cutoff query | 4 | 0 |
| Flags query | 0 | 2 |
| Sprite / retained wall-state query | 0 | 0 |
| Thin-platform retirement proof | 164 | 0 |
| External registration query / emitted member | 0 | 142 |
| Generic table slot query / admission check | 0 | 142 |

Construction attempted 63 prepared segments / 144 cells: 29/58 in updates
56–121, 8/30 at 122–135, 13/28 at 136–162 and 13/28 at 163–164. The scalar
fallback at 56 also contributes construction reads. There is no generated
terrain construction in updates 1–55. Twenty-six animation skips occur in
136–162 and two in 163–164. The four cutoff checks occur twice in the entrance
and once in each of those final windows.

These are nested API call/visit counts, **not disjoint memory-access counts**.
Construction scope includes admission, publication and synchronous Create;
it does not include unrelated later phases. Zero means this route did not use
the instrumented query, not that the game's corresponding semantics can be
removed. The native landing witness, 164 retirement-proof reads and nonzero
construction counts provide route controls; host fixtures check scope nesting,
disabled probes, capture truncation, counter resets and startup leakage.

The replacement must supply room activation and cleanup, inactive-layer
cutoffs, thin-platform retirement and all mutation queries from its segment
owner. In particular, animation currently reads the room structure only to
skip its terrain; that does not justify copying fragmented lifetime topology
into animation. This census does not change the timing baseline above or close
item A.

### Source-record terrain owner (section 0 A)

The default Amiga build now uses it; `REFERENCE_TERRAIN_RECORDS=1` keeps the
retained per-span owners as the oracle. Each prepared row segment becomes one
`TerrainBirthRecords` record supplying identity, room and layer chronology
and initial state; changed cells materialize separately. The record is
reserved and written in place, and `GameplayCollisionOwner` proves the
per-kind flags, Create plan and integer depth once per tile. After one
successful source span, the collision owner lends the record owner the
counters that span updates (`TerrainSourceLease`): identity chronology and
capture revisions, the terrain layer's retained population, the layer and
room clocks and the thin-platform retirement proof. Later segments of the
same tile are admitted once (`reserveLeased`), the backend publishes their
occupancy, landing candidates and tiles, and `publishLeased` installs the
record; no layer, table, room or window owner is entered. The lease observes
owner health, borrowed callback status, layer topology and the bounded
counters; rebinding revokes it.

The last step to the gates removed more unobserved or repeated work:

- **Corridor rows** discard `choose(2)` and 19 cell draws; they now advance
  the stream by 20 in one step (update 99's builder step 22,744 → 10,890
  cycles).
- **Layer topology** advances only when an existing layer is erased,
  relinked or changes depth. A breakable birth inserting its scenery layer
  after equal-depth peers no longer revokes the terrain lease, so the
  bursts no longer re-prove a source span each update (7,290 cycles at 122
  and at 136).
- **Builder newline rows** under a lent publication cannot fail and have no
  spatial consumer; they land at the next scalar Create (or the phase end)
  instead of every row (builder position at 122: 13,136 → 8,672 cycles; at
  136: 24,668 → 13,400).
- **Leased segments are admitted once,** around the backend's own
  publication, instead of by the lifetimes, the lease and the record in
  turn.
- **A prepared RNG word** is a ring read and a cursor step: the prepared
  count, history and consumer state index derive from the draw count, read
  only by `stateWord()` and `stateIndex()`. With the single admission this
  took 122 from 35.8 to 34.9 ms and 136 from 66.0 to 64.5.

**Complete fall** (three 100-field captures from updates 1, 60 and 120,
`tools/amiga-fall-budget.py`), against the
[baseline](#complete-initial-fall-against-the-20-ms-budget--45e96a9c-2026-10-12):

| Window | Work mean ms | Worst ms (update) | Over 20 ms | Excess ms | Baseline mean / worst / over / excess |
| --- | ---: | ---: | ---: | ---: | --- |
| Authored entrance, 2–55 | 16.4 | 20.5 (19) | 1 | 0 | 16.2 / 20.5 / 1 / 0 |
| Quiet generation, 56–121 | 18.7 | 24.5 (71) | 22 | 29 | 20.2 / 27.7 / 32 / 115 |
| First burst, 122–135 | 21.2 | 34.9 (122) | 8 | 35 | 22.5 / 55.0 / 8 / 54 |
| Enemies, 136–162 | 30.6 | 64.4 (136) | 27 | 288 | 31.4 / 94.3 / 27 / 308 |
| Landing, 163–164 | 77.8 | 104.3 (163) | 2 | 116 | 93.7 / 136.0 / 2 / 147 |
| Whole fall, 2–164 | 20.8 | 104.3 (163) | 60 | 468 | 21.8 / 136.0 / 70 / 626 |

16 of the quiet window's 22 updates over 20 ms both generate and paint a row
(20.6–21.7 ms). Three generate without painting (20.2–21.4 ms): 72 and 118
with a 3.8 ms display row sync, 56 with a 13.5 ms Step and 2.7 ms draw-list
collection. Their excess is C's row work and the fixed update cost (D). The other
three paint without generating; the worst, 69 and 71, paint the most
(`drawMainWorldSpan` 7.5 and 6.1 ms). Capture ELF SHA-256:
`796ee3bb60228069148c8b435a44a0f4c68068ee6bb88d92ab7888d6a4cf3490` (from 1),
`dd438086da39db36fcf1c5fe96e052a2067b2d1f5a8c80a20a1f280c4432542c` (60),
`c73099a1fdc5fe4b16a9db10597de75b4715e2f90c1db284a376459eb4efeced` (120).

**Per update**, ms of work against the item's gates:

| Update | Baseline `45e96a9c` | `a3bce28b` | Now | Gate |
| ---: | ---: | ---: | ---: | ---: |
| 99, one quiet row | 24.6 | 22.6 | 21.1 | 22.8 |
| 122, first burst | 55.0 | 37.4 | 34.9 | 35 |
| 136, enemy burst | 94.3 | 68.5 | 64.4 | 65 |
| 163, landing | 136.0 | 110.2 | 104.3 | – |

Earlier revisions of this table gave 99 as 23.1 ms at `a3bce28b`: that was
the marker-to-marker time, including 0.5 ms of publication wait. The 22.8 ms
gate was derived from it, so it is 0.5 ms looser than intended; 99's work is
under the stricter 22.3 ms too.

**Per segment the target is not met.** A non-empty leased segment costs
about 7,300 cycles at 122 and 7,100 for a two-cell corridor segment at 99,
against the 6,500 the item aimed for; the update gates pass because of the
removals above. Remaining per segment at 122 (cycles):

| Part | Cycles |
| --- | ---: |
| Backend occupancy, landing candidates, tiles and the segment's RNG advance | 2,140 |
| Lease publication (lent counters and record facts) | 1,140 |
| Backend admission, tile lookup, completion and header reads | 1,150 |
| Record commit | 1,030 |
| Lease and record admission | 940 |
| `reserveLeased` and `publishLeased` own | 800 |

Builder position costs about 1,450 cycles per row at 122 and 1,100 at 136
(target 1,500), including one 1,250-cycle publication acquire per update. A
single-row update pays 3,650 (99), so the target is not met there; the
per-update acquire of an unchanged publication is listed under D.

Behaviour on this tree: `bash amiga/profile.sh quiet-fall` gives **186
fields** with the source-record owner (18 misses, worst gap 5 at 163) and
**190** with the retained oracle (17 misses, worst gap 6 at 163), versus the
baseline's 192; both give the exact landing witness. All 165 `profile_quiet_landing.gdb` state
samples match in both arms. The 18,000-frame ownership replays pass in both
arms, as do the source-record (leased, fused and unleased runs with
identical counters after every segment), RNG lookahead (against the direct
WELL stream after every operation), layer topology, lent builder publication,
raster-band, read-census, builder and collision-owner fixtures. Each check
fails when its piece is broken: the replays when deferred builder rows never
land (frame 136) or a fused segment drops its thin-platform count (frame
1113); the builder corpus and RNG replay when a corridor row draws 19; the
RNG test when the derived history or state index is off; the layer test when
erasure does not advance the topology. No route reads the builder between
rows, so the landing before each scalar Create is conservative. The normal
stripped Hunk (SHA-256 `2b78ec67bbdad16030dfcd982dba62138a3d1bc99935708657e87e8e00caccea`) passes 512 KiB Chip startup.

### Native enemy updates (section 0 B)

The default Amiga build now uses them; `REFERENCE_ENEMY_UPDATES=1` keeps the
generic alarm walk, room animation walk, per-setter publication and
fifths-of-Q8 motion as the oracle. At `a9dd0478` one crawler cost 8.7 ms of
Step per update in 137–162 and the three enemies born at 163 cost 23.6 ms of
room effects and alarms at 164. Almost all of it was generic machinery around
a few terrain tests. The change removes it at each owner:

- **Alarm phase.** When the combat capture holds only the player, the
  captured walk becomes a direct pass per alarm index: the player's damage
  alarm, the native box prefix and the enemy roster, in that resource order.
  The roster pass resolves each enemy once and skips counters that cannot
  change; events above Alarm 1 have no stored counter (the hit states count
  them down) and are not dispatched. The box prefix runs only once some box
  has armed Alarm 0, which only `publishBoxAlarm0` does. Enemy births no
  longer invalidate the capture, whose first watched range now holds the
  player alone.
- **Room animation.** With only native enemies in the animation population,
  each room-active enemy advances its image directly instead of the room walk
  visiting every member; the advance takes one divide.
- **Motion.** The crawler, snail and walker scripts are templates over their
  coordinate math. `FixedCollisionMath` (one fifths-of-Q8 long) stays the
  oracle; the native records hold a pixel and its 1280ths
  (`PixelCollisionMath`), so rounding, the pixels a solid test needs and the
  Q8 record fields take no division. Solid tests read the terrain index's
  aligned cells (`SolidCellGrid`, from `amigaTerrainSolidCells`), each
  distinct cell once, instead of calling the resolver per corner.
- **Publication.** With no collision consumer admitted, nothing reads an
  enemy's mirrored image index or transforms, so the record owns them and
  its position (`useRecordAppearance`): those setters and the sprite
  mirror's bookkeeping are no-ops, an unchanged sprite is not published, and
  a change between two sprites keeps its prepared draw action (every enemy
  flag write republishes it).
- **Step order** comes from the alarm roster instead of a registered-range
  walk, and an already ordered snapshot is not sorted again.
- **Boxes.** The unbound box Step only destroys exhausted boxes; it runs once
  some box has been exhausted. Every health write reports to the world.

Tried and reverted, both slower on the 68000: taking the walker's and snail's
mask box once per Step (it moved `contactWorldBounds` out of line, +3.2k
cycles in the contacts at 164), and keeping the player's contact box across
the enemy loop (+0.9k at 164).

**Complete fall** (three 100-field captures from updates 1, 60 and 120),
against the [source-record owner](#source-record-terrain-owner-section-0-a):

| Window | Work mean ms | Worst ms (update) | Over 20 ms | Excess ms | `a9dd0478` mean / worst / over / excess |
| --- | ---: | ---: | ---: | ---: | --- |
| Authored entrance, 2–55 | 16.5 | 20.6 (19) | 1 | 1 | 16.4 / 20.5 / 1 / 0 |
| Quiet generation, 56–121 | 18.8 | 24.5 (71) | 22 | 31 | 18.7 / 24.5 / 22 / 29 |
| First burst, 122–135 | 20.2 | 35.0 (122) | 8 | 27 | 21.2 / 34.9 / 8 / 35 |
| Enemies, 136–162 | 24.3 | 64.1 (136) | 14 | 135 | 30.6 / 64.4 / 27 / 288 |
| Landing, 163–164 | 65.7 | 100.2 (163) | 2 | 91 | 77.8 / 104.3 / 2 / 116 |
| Whole fall, 2–164 | 19.6 | 100.2 (163) | 47 | 285 | 20.8 / 104.3 / 60 / 468 |

Updates 2–121 have no enemy and are unchanged: their work moves by +0.1 ms
on average and at most +0.4 ms (101); update 98's Step is 476 cycles longer
(Step's own code and the roster-built Step order with no enemies) and its
whole update 20 cycles. 123–135 gain 1.1–1.3 ms each: the box Step and the
box alarm prefix no longer visit the first burst's boxes (about 9k cycles at
`a9dd0478`'s 149). Capture ELF SHA-256:
`a15c3d2001f1cee132372dc5d8213db0aa8d6b04030a0e2f08a141231e3e1b03` (from 1),
`8fa991aba60ed8c9830038c0f5ea33712bd3a38add1240244c24322ed9314702` (60),
`2034afd8e67eff56e334b51e500bf3ba36fe1be3d9609af26f6866cef9e82617` (120).

**Per update**, Step ms (cycles):

| Update | `a9dd0478` | Now | Gate |
| ---: | ---: | ---: | ---: |
| 150, falling crawler | 18.5 (131,114) | 10.8 (76,800) | – |
| The crawler's own work at 150 | 8.7 (61,908) | 2.2 (15,424) | 2.5 |
| 137–162 mean | 17.3 | 10.8 (at most 11.5) | 11 |
| 164, landing with three enemies | 39.9 (283,746) | 19.6 (139,254) | 20 |

The crawler's own work is measured against update 98, a quiet update at
`a9dd0478` (Step 62,902 cycles): at 150 its Step is 8.4k cycles (5.5k of it
motion), the alarm phase 4.8k above an empty one, contacts 1.4k and its
animation 0.7k. Every update over 20 ms in 137–162 paints a row (5.2–17.9
ms, item C); the others take 17.7–19.4 ms of work. At 164 the three enemies'
Step is 38.0k cycles (109.5k), of which the turtle's terrain tests are 12.7k
and the snail's 10.8k; alarms 16.8k (57.7k), half of it the crawler's motion;
room animation 2.2k (16.3k). The player's 25.9k is unchanged.

Behaviour on this tree: `bash amiga/profile.sh quiet-fall` gives **176
fields** with native enemy updates (10 misses, worst gap 5 at 163) and **186**
with the reference updates (18 misses, as at `a9dd0478`); both give the exact
landing witness. The enemy witness
(`amiga/profile_quiet_enemies.gdb`, every enemy's record fields for 400
updates, 738 rows: a crawler falling and wall-following, a snail and a turtle)
is identical in the native arm, the reference arm and at `a9dd0478`. All 165
landing state samples match in both arms. On the host, the pixel math matches
`FixedCollisionMath` over ±40 pixels exhaustively and two million random
pairs, and the three scripts match their oracle instantiations on random cell
terrain; the cell grid matches `amigaResolveTerrainSolid` and
`amigaResolveTerrainCorners` on random indexes and live or destroyed boxes;
the live simulation moves every enemy alike through the grid and the
resolver; the collision-owner suite gives the same ownership witness in the
native, audited (the roster's Step order against the walk, every capture)
and reference arms, including a death and two armed births between alarm
phases; the cross-target replay passes. Each check fails when its piece is
broken: the math test on a lost tie rule, carry or fifth; the audit when the
roster drops an entry from the Step order; the death-and-birth fixture when
the roster ignores a birth. A new contact fixture covers a stomp that moves
the player before the next enemy. `amiga-scene-runtime-test` still stops at
its unrelated frame-0 builder checkpoint check, as it did at `2331458e`, so
its cross-adapter enemy comparison does not run; `replay-test`'s schema-49
digest failure is unchanged (same digest as `a9dd0478`). The normal stripped Hunk (SHA-256
`d2493cfc7d6c3d2adf7e6ddc105472431360e29e7622f0b593335bca25b102d2`) passes
512 KiB Chip startup.

## Fixed frame work candidate — section 0 D, 2026-10-18

Not accepted as the default; D's gates remain unmet. These measurements use
`REFERENCE_FRAME_WORK=0 REFERENCE_ROW_PAINTING=1`: counted destruction results,
empty-alarm admission, streak-dependent weapon selection and grouped display
row debts, with the original row painter. No expanded asset bank is involved.
The comparison is against the complete `07313c0e` fall above, not a mixed
row-painting candidate. The three-capture method and 512 KiB Chip configuration
are unchanged.

| Window | Baseline mean/worst ms | Candidate mean/worst ms | Candidate over 20 ms | Candidate excess ms |
| --- | ---: | ---: | ---: | ---: |
| Entrance 2–55 | 16.5 / 20.6 | 15.0 / 18.2 | 0 | 0 |
| Quiet generation 56–121 | 18.8 / 24.5 | 17.3 / 22.5 | 12 | 7 |
| First burst 122–135 | 20.2 / 35.0 | 18.6 / 33.6 | 5 | 16 |
| Enemies 136–162 | 24.3 / 64.1 | 23.3 / 62.5 | 14 | 125 |
| Landing 163–164 | 65.7 / 100.2 | 63.4 / 98.7 | 2 | 87 |
| Whole fall 2–164 | 19.6 / 100.2 | 18.2 / 98.7 | 33 | 235 |

Update 98 remains 14.3 ms (baseline 15.8), above D's 11.5 ms gate;
the entrance mean remains above its 12 ms gate. Full behavior acceptance and
the separate cadence run must precede any default change. All 165 native
`quiet-state` observations match with `REFERENCE_FRAME_WORK=0` and `=1`,
including tick 164, y 262144, health 4, ammo 8, three enemies and 1371 RNG
draws (`tmp/d-state-{0,1}-gdb.log`). These are selected state observations,
not full-state or pixel equivalence. Counted-result and
alarm host tests pass in both arms; the row-sync test passes 10,000
wrap/damage/pipeline cases with exact copied rows and untouched margins.

Reports: `tmp/d-whole-{1,60,120}-report.txt`; traces:
`tmp/trace/d-whole-{1,60,120}`. Trace ELF SHA-256 by start mark:

- 1: `16cea0ae63824177063bb15fc1116188adf2a1f2d44cea9dea51652c6b5ffa68`
- 60: `43a9ab06803e006b119edc3d33db717fc3cfa3ad8b73d1e853f57d04d5b3bc0d`
- 120: `51474f861372e977aca77ba474e8e0834d73b0d922f2e84950642affd48f2be0`

## Combined row-painting candidate — section 0 C/D, 2026-10-18

**Not accepted as the default.** Three 100-field captures with
`REFERENCE_FRAME_WORK=0 REFERENCE_ROW_PAINTING=0` cover all updates 2–164.
The ordinary-memory terrain bank remains **330,024 bytes**; no expanded
wall-and-tile bank was used. This candidate includes exact decoration-bound
admission and run-based native composition of the retained row references.
The short-span CPU display-copy experiment is reverted.

| Window | Baseline mean/worst ms | Candidate mean/worst ms | Candidate over 20 ms | Candidate excess ms |
| --- | ---: | ---: | ---: | ---: |
| Entrance 2–55 | 16.5 / 20.6 | 15.1 / 19.7 | 0 | 0 |
| Quiet generation 56–121 | 18.8 / 24.5 | 17.8 / 24.3 | 1 | 4 |
| First burst 122–135 | 20.2 / 35.0 | 18.5 / 33.8 | 2 | 14 |
| Enemies 136–162 | 24.3 / 64.1 | 21.5 / 61.4 | 11 | 53 |
| Landing 163–164 | 65.7 / 100.2 | 59.3 / 88.0 | 2 | 79 |
| Whole fall 2–164 | 19.6 / 100.2 | 18.1 / 88.0 | 16 | 150 |

This does not pass C or D. Excluding the generation burst at 136,
137–162 has mean **20.0 ms**, worst **23.2 at 162**, ten over-budget
updates and 11 ms excess. Against the isolated D candidate above, quiet
generation is worse (17.3/22.5 → 17.8/24.3 ms), despite fewer over-budget
updates. Do not accept on the overall reduction alone.

Update **70 independently repeats at 172,720 cycles / 24.3 ms** in both
the start-1 and start-60 captures. It spends 9.7 ms in Step and **7.64 ms
in `drawMainWorldSpan`**, including 5.25 ms in `preparePatternCache` and
3.71 ms in its scalar `paintCached` fallback. It is a cold visible-row
miss, not fixed dispatch overhead. Update 98 now carries preparation and
transfer work: **19.1 ms work**, including 1.4 ms preparation and 1.4 ms
transfer. It is not an isolated remeasurement of D's 14.3-ms quiet update.
The native composition change reduces update 162's transfer from 3.9 to
2.3 ms versus the decoration-only candidate, but does not finish the row
window.

Evidence: `tmp/c-compose-{1,60,120}-report.txt`,
`tmp/c-compose-70-recheck.txt`, `tmp/c-compose-98-report.txt`.
ELF SHA-256 by trace start:

- 1: `63b1a6259b873eb6637aafd5485c43ad5851d676062bb922d4dcd16abe9330c0`
- 60: `2bdf58386116b07bf01e66e5de45897aa646b21397a7601ef1bce79e676f6229`
- 120: `06538fcfaa975c049a330a6270759aa3c16e91b478117d8841c9760ff38529cf`

Separate instrumented native evidence (`tmp/c-compose-pixels-gdb.log`):
80 native composed rows match the portable pixel oracle, zero failures;
all 165 selected fall samples match `tmp/d-state-0-gdb.log`, and the landing
marker passes. This is not full-state or whole-frame pixel equivalence.
Host evidence: 18,000-frame terrain ownership replay; 10,000 display-row
debt cases; 10,000 composed-row comparisons; 4,800 raster transfer cases;
416 decoration-edge cases; 12,360 pattern/seam/mutation rows; and the
20,000-update backing/staging fixtures pass. Remaining acceptance gates
and default selection are unchanged. The normal build is restored after
both new captures and passes the 68000 instruction/helper audits.

## No-input initial-fall comparison

### Complete recurring quiet interval

Matched 75-field captures starting at mark 56 cover every update 57–121,
not just the middle sample. Historical `13e08ca2` totals 9,295,120 cycles;
accepted `edaaa369` totals 9,505,010: **20.13 versus 20.58 ms/update**,
including publication waits. Worst intervals are **27.4 versus 27.5 ms**,
both at update 69. This is a roughly 2.3% mean difference, not evidence of
a remaining broad quiet-frame collapse or a new 50 FPS guarantee.

The same captures cover update 122: **28.2 versus 54.9 ms**, with Step
**20.5 versus 48.0 ms**. Updates 122–126 total 783,070 versus 952,290 cycles
(22.04 versus 26.81 ms/update). This isolates a materially larger remaining
initial-fall regression at generation, after the recurring quiet interval.
An independent eight-field capture starting at mark 121, with **512 KiB Chip**,
confirms frame 122 at **390,230 cycles / 54.9 ms**. Nine prepared segments
cost 26.77 ms; native lifetime construction inside them costs 21.73 ms,
including 12.56 ms in span-owner publication. Room-span insertion is 2.38 ms
and layer publication 2.08 ms, nested inside that owner cost. These are not
independent savings to sum; no single small helper removes the regression.
The complete fall remains 181 versus 191 PAL fields; this audit does not
close the restoration goal.

Both arms use deterministic A500/68000, 1 MiB Chip, 512 KiB slow and 8 MiB
Fast for compatibility with the historical regenerated assets. Current normal
startup is separately verified at 512 KiB Chip; no shipping memory increase.
Frozen ELF hashes: historical
`d1d1c64d3690c89ac343545c979946cd917543825ada4834c18a12c324f2a115`, current
`f8b656c909c636b0607c7c5ef468959b748893e3deaf1c211a3f14a1ec4b004a`.
Reproduce the current target-memory arm with `bash amiga/profile.sh quiet-window`:
it builds the no-input profile, ignores 55 frame marks and captures 75 fields.
Its reports require every complete update in 57–121 and the separate update 122;
`frames.json` preserves exact cycles and the ELF hash for matched comparisons.
For the historical expanded-memory comparison, explicitly use
`VERIFY_CHIP_KB=1024` on both arms. The underlying reports accept
`--frames=122-126 --require-complete --causes` for the following window.
Each run is capped at 90 seconds. Reports: `tmp/quiet-full-window-{current,historical}.txt`
and `tmp/quiet-full-window-{current,historical}-next.txt`, with independent
target-memory report `tmp/quiet-full-window-122-recheck.txt`; raw traces are
disposable once the figures are committed.

### Historical contact comparison boundary

The unchanged `GameplayContactProfileInput` function does not guarantee the
same workload across `13e08ca2` and the current game code. Separate 1,800-sample
state-only runs match player/camera position, velocity, health, ammo, enemy and
bullet counts, RNG draw count, seed and index through sample 463. Sample 464
has two fewer RNG draws in the current build; the counters reconverge at 465.
The same transient occurs at 650, then player velocity diverges at 652 and
position at 653. These samples are not full entity/RNG-word digests.

An independent eight-field instruction capture from mark 462 identifies the
first difference: the historical update executes two `CurrencyStepSound::begin`
choices for newly born gems. The current Step cutoff defers them until the
next update; these draws are delayed, not lost. `b98178ee` introduced this
reward birth/Step correction; its source contract and tests are in
`runner-step-eligibility.md`. Do not undo it to reproduce the historical FPS.
The later motion divergence has not been isolated to this single change.

The early firing window 51–70 is before the observed divergence: historical
mean/worst **23.984/34.418 ms**, current **28.002/42.033 ms**. In contrast,
same-number comparisons at 676–681 and 1305–1314 do not establish regression
size: the player has reached different positions and encounters. The no-input
fall above remains the immediate restoration target.

Both contact comparison arms use deterministic A500/68000, 1 MiB Chip,
512 KiB slow and 8 MiB Fast; current 512 KiB Chip captures agree with its
1 MiB control to rounding. Trace flags are `SKIP_TO_GAME=1 PROFILE_CONTACT=1
PROFILE_AUTORUN=1 FPSCOUNT=1 TRACE_FRAMES=50,675,1304`, 35 fields per mark,
90-second run deadlines. Historical code was rebuilt against its generated
snapshot, not an archived historical executable. Frozen ELF hashes:
historical `da1e49f2f0712ab6ef0efa6583a194becf0f0f57e136ba960248a6fb9ac669ca`,
current `c76084cbed92616b2b48e46d0ee4b27b4d28b5a0a3b89ff8241c82dfa60ed613`.
Reports: `tmp/contact-historical-13e08-report.txt`,
`tmp/contact-current-1m-6817-report.txt`, `tmp/contact-divergence-{historical,current}.txt`;
state-only witnesses: `tmp/contact-state-{historical,current}.log`.
The state-only breakpoints are not used for timing.

### Authored-entrance seam

Corridor admission now ignores offscreen diagonals: only the vertical neighbours
of visible wall columns affect their frames. Rendered rows still require the
exact corridor shape and empty horizontal neighbours. Scalar pixels verify the
newly admitted diagonal cases and all 26 tile frames in the offscreen guard rows.

Matched marks 57–80 total **3,639,740 → 3,621,110 cycles**. Frame 73's terrain
painting falls **5.9 → 2.8 ms**, and its whole interval 28.5 → 25.4 ms; both
arms include the same 6.9 ms publication wait. A separate five-field capture
from mark 72 confirms **180,420 cycles / 25.4 ms**. The whole-window saving
is small because waiting redistributes. The full fall is still **191 fields**;
the two early missed fields at 67 and 72 remain. Do not claim restored cadence.
Marks 161–166 remain exactly 2,412,320 cycles, including frame 163 at 135.9 ms.
All 165 state samples match; normal 512 KiB Chip startup passes. Normal Hunk
is 1,606,200 bytes, with no new assets or allocation. Profile ELF SHA-256:
`f8b656c909c636b0607c7c5ef468959b748893e3deaf1c211a3f14a1ec4b004a`.
Normal Hunk SHA-256:
`4c279b9e5cb847fa6f6daf84f2099ee1faa04b003351635be086d2f7a18e030a`.
Reports: `tmp/quiet-corridor-{onset,seam-onset,seam-recheck,seam-stall}.txt`.
The onset capture starts at mark 56 and records thirty fields; the burst
capture uses the existing mark-160/twenty-field procedure.

### Borrowed corridor pixels

The complete fall independently repeats at **191 fields / 164 updates
(42.93 FPS)**, versus 195 before corridor borrowing and the historical 181.
Only two of the ten extra fields are now in updates 57–121. The cadence
improvement alone is below 3%; the removed-work trace is the stronger evidence.
Matched marks 91–110 total **3,078,280 → 2,805,770 cycles (21.66 → 19.75
ms/update)**. Corridor paint spans fall from 4.9–5.6 to 2.9–3.2 ms; across
the whole window, `drawMainWorldSpan` falls **2.67 → 1.48 ms/update**.
The live shape check costs 0.15 ms/update. This saves somewhat less paint
work than the initial estimate of about 3 ms per eligible frame; it removes
composition/cache writes as a whole, not a sequence of helper optimizations.
Remaining marker-interval savings include redistributed publication waits.

Marks 161–166 total 2,403,370 → 2,412,320 cycles (56.38 → 56.59 ms/update,
noise). The worst complete frame, 163, is **136.1 → 135.9 ms**; its observed
field gap moves from six to seven through phase alignment, not added work.
All 165 native state samples match. Scalar pixels match at all sixteen
corridor phases; borrowed-raster tests pass 324 mixed transfers plus 4,800
store/load/wrap/pipeline cases. The normal build starts with 512 KiB Chip;
the emulator still has 8 MiB Fast and 512 KiB slow RAM. No Chip allocation
is added. Normal Hunk: 1,606,228 bytes (+2,236), including 704 extra prepared
pixel bytes. Profile ELF SHA-256:
`3392076defcae614d8f52e2dee3200a0faa52a67bab21d358cf0ed2c3fa75326`.
Normal Hunk SHA-256:
`e8de74f3c4b5b594863a7d9e5ff83b7000f9a7d0132eb73480ee84b74e6bd562`.
Reports and witnesses: `tmp/quiet-corridor-*`, using mark-90/26-field and
mark-160/20-field captures. The original painter remains the unsupported-shape
fallback and the pixel oracle. The performance-restoration goal is still open.

### Shipping ownership specialization

The shipping external-collision contract now fixes native/scenery/box ownership
at compile time, excluding unreachable reference-owner lifecycle paths. The
constructor still rejects incompatible arguments; reference builds retain the
original ownership choices. Native field ownership remains runtime-selected.

The current fall repeats at **195 PAL fields / 164 updates (42.05 FPS)** versus
198 before specialization and the historical target of 181. This small cadence
change alone remains below the 3% noise allowance. Marks 91–110 total
3,172,050 → 3,078,280 cycles (22.32 → 21.66 ms/update), with Step 11.27 →
11.19 ms. The window's worst interval moves **29.0 → 31.1 ms**: frame 105 now
contains 9.6 ms of publication waiting; its work excluding that wait is about
21.5 ms, versus 21.4 before. Do not describe redistributed waiting as removed
simulation work. Marks 161–166 total 2,430,370 → 2,403,370 cycles; frame 163
is 136.8 → 136.1 ms. The goal remains unresolved.

Normal stripped Hunk: **1,769,920 → 1,603,992 bytes**. Matched profile ELF
sections attribute this mainly to `.text` (1,320,120 → 1,158,186 bytes), not
assets: `.rodata` grows 196 bytes, `.data` is unchanged and `.bss` shrinks four
bytes. No dynamic allocation or Chip-memory configuration changes. All 165
native state samples match; both ownership-contract configurations, the
18,000-frame host ownership replay and normal 512 KiB Chip startup pass.
Profile ELF SHA-256:
`53ab87bcb24d62e22446fcccc5db096e1d6ca124c29de18146ac1575515d949c`.
Normal Hunk SHA-256:
`d82ffb6c9250357cce10348d2e09b53847d041f812445fbf58270a9d0b5f103a`.
Reports and witnesses: `tmp/quiet-owner-specialization-*`; captures use the
same mark-90/26-field and mark-160/20-field windows. A separate five-field
capture starting at mark 104 checks frame 105's wait attribution.

### Phase-scoped controller publication

The pre-specialization no-input fall independently repeats at **198 PAL fields for 164
updates (41.41 FPS)** with 512 KiB Chip, versus 199 before this change and the
historical target of 181. The one-field cadence change alone is noise, not
evidence that the regression is resolved. The missed-field ledger attributes
seven extra fields to updates 57–121, out of seventeen over the whole fall.

DitherAbove now acquires its native position mirrors once per Begin Step,
retaining every source-ordered setter and its appearance dirtiness. Collision
consumers and ordinary callers keep the checked callback reference path.
Matched marks 91–110 measure controller work **1.20 → 0.88 ms/update** and
whole Step **11.64 → 11.27 ms/update**. Including redistributed VBlank waits,
the window is 3,130,780 → 3,172,050 cycles (22.03 → 22.32 ms/update); its worst
interval is 30.0 → 29.0 ms. This is a small removed-work improvement, not a
large cadence win. Leave this now-submillisecond controller alone.

Marks 161–166 total 2,442,890 → 2,430,370 cycles; frame 163 is 137.0 → 136.8 ms,
effectively unchanged. All 165 native state witnesses match. Host controller
fixtures, collision-owner integration and ten terrain replay tapes (18,000
frames) pass. The normal non-profile build passes 512 KiB Chip startup.
Normal Hunk: 1,769,920 bytes (+240); no allocation added. Profile ELF SHA-256:
`6eb1e3c26e4457fa488a68353bc50cfa10ad7905ef9b33cfb5f6f2f8cab017d2`.
Normal Hunk SHA-256:
`aad33042d3ceb75edb8d4fda7ad4ef1ffa5c51fbd9210dcaa5e373b281090f1f`.
Reproduce with the same mark-90/26-field and mark-160/20-field captures below.
Reports and cadence/state witnesses: `tmp/quiet-dither-phase-*`.

### Middle of the quiet fall: updates 91–110

The same constructor also explains much of the later no-input stall. A separate
ten-field capture at mark 135 of the shipping-specialization profile above
contains complete frame 136: **671,400 cycles / 94.5 ms**, versus 50.4 ms in
the historical fall. Current Step is 85.8 ms (historical 41.7 ms). Within that
Step, generation costs 75.4 ms; its prepared-terrain callbacks cost 40.7 ms,
including **31.8 ms of native lifetime construction**. Owner span publication
is 18.6 ms inside that construction. These are nested costs, not additive.
Draw collection is 3.5 ms and display row synchronization 1.2 ms. This is
not evidence of a renderer regression or a change in the recurring quiet
frame target. Report: `tmp/quiet-136.txt`; reproduce with the frozen profile
ELF above, ignore 134 marks, capture ten fields, report `--frames=136`.

The batching boundary matters: `AreaOneBuilder::emitPreparedPattern` invokes
source event Creates between prepared segments and publishes the builder's
position at each row advance. Those callbacks must see all earlier births
and none of the later ones. Initial-border Steps already publish both walls
in one segment before advancing the row. Deferring whole patterns or several
Steps is therefore not an equivalent replacement for the measured constructor.
A replacement must remove work within the existing admitted segment, or prove
and preserve each intervening observation and failure prefix.

Single-frame decomposition at `56945ecc`: a five-field capture starting at
mark 92 contains complete frame 93, **187,870 cycles / 26.44 ms**. The source
border publication calls `appendPreparedCells` once. Its relevant inclusive
costs are:

| Frame-93 work | ms |
| --- | ---: |
| Whole Step | 14.49 |
| Prepared terrain callback | 2.93 |
| Native lifetime construction, inside that callback | 2.43 |
| Collision-owner span publication, inside construction | 1.40 |
| Layer admission, inside span publication | 0.21 |
| Layer publication, inside span publication | 0.26 |
| Room membership, inside span publication | 0.25 |
| Fresh registry admission, inside span publication | 0.10 |
| Chronology publication, inside span publication | 0.09 |
| Retained identity-run insertion, outside span publication | 0.29 |

Do not add nested rows. Room membership is only about one tenth of the
construction cost: replacing that list alone cannot remove the regression.
The source confirms that the registry already publishes chronology as a span,
and no generic collision geometry/index is constructed on this external-backend
path. Any further architectural replacement must address the combined
publication transaction, not tune these submillisecond components separately.
No runtime change accompanies this capture. Reproduce using the frozen
`6eb1e3c2` profile ELF above, ignore 91 frame marks, capture five fields and
report `--frames=93 --tree-min=0 --focus constructTerrainSpan
--lines constructTerrainSpan`. Full breakdown: `tmp/quiet-birth-cost.txt`.

Matched deterministic 26-field captures beginning at mark 90 compare historical
`13e08ca2` with `bad8bd33`, using the frozen profile ELFs identified below.
Both comparison arms use 1 MiB Chip because the rebuilt historical asset pack
does not fit 512 KiB. A separate current 512 KiB capture confirms the same
cost: **3,130,780 cycles**, versus **3,130,800** at 1 MiB. Historical cost is
**2,806,770 cycles**. These are twenty complete marker intervals, including
publication waits, not twenty independent work timers.

| Inclusive ms/update, updates 91–110 | Historical | Current |
| --- | ---: | ---: |
| Whole marker interval | 19.75 | 22.03 |
| Gameplay Step | 9.14 | 11.65 |
| Generation callback, within Step | 1.49 | 2.56 |
| Player movement, within Step | 2.09 | 2.03 |
| Alarm phase, within Step | 0.91 | 0.29 |
| DitherAbove Begin Step, within Step | absent | 1.20 |
| Actor Draw collection | 1.30 | 1.74 |
| New terrain span painting/transfer | 2.68 | 2.67 |
| Display row synchronization | 1.22 | 1.21 |

Ordinary Step updates cost 7.8–8.0 ms historically versus 9.0–10.1 ms now.
The eight border-generation updates (93/95/97/99/102/104/106/109) cost
10.9–11.3 versus 14.7–15.7 ms. This is the recurring quiet-fall workload,
not the large generation burst at 163. Frame totals peak at 25.1 versus
30.0 ms, but the current peak at 105 includes 8.5 ms in publication waiting;
do not attribute that interval to terrain construction.

The prepared border callback costs 1.17 ms averaged across all twenty updates,
or **2.93 ms per invocation** (eight calls). Native lifetime construction
accounts for 0.97 ms/update (2.43 ms/call), including 0.56 ms/update in the
collision owner's span publication. Occupancy admission and `canAppend`
together account for only about 0.05 ms/update. Hoisting those checks cannot
remove the measured regression; the surviving ownership publication is the
relevant structure. Terrain drawing, restoration and player movement are not
the regressed work. Required DitherAbove behavior must remain.

The historical source explains this difference: `13e08ca2`'s
`AmigaGameplayGeneration::createPrepared` ends after occupancy/landing and
`tiles_.appendPrepared`; it does not construct native terrain lifetimes.
`NativeTerrainLifetimes.h` first appears in `f6f7d448`, with registration
chronology connected in `92bfda6e` and deferred room membership in `1eada70a`.
The current lifetime-construction cost (0.97 ms/update) accounts for about
91% of the generation callback increase (1.07 ms/update). This is added
semantic state, not evidence that the old terrain painter became slower.

Nor is the whole room copy unused. `eligibilityFlags` reads its reconciled
active state for externally handled walls; `reconcileActivation` changes
registration chronology and layers at the source phase boundary;
`cleanupDestroyed` merges marked terrain with native boxes in room order.
Layer state separately supplies inactive Draw cutoffs. An ownership
replacement must preserve these distinctions; deleting wall registration
or substituting an always-active lease is not equivalent. These consumers
rule out removing room binding without replacing its observable state.

Reproduce with the frozen quiet-fall build and memory configurations below:
ignore 89 hits of `downwellTraceFrameMark`, delete the breakpoint, capture
26 fields, then report `--frames=91-110 --causes`; `--focus createPrepared`
isolates border publication. Reports are `tmp/quiet-middle-{old,current,512}.txt`
and `tmp/quiet-middle-generation.txt`. All three emulators stopped on explicit
success within their 90-second deadlines. Raw captures were removed after
recording these numbers. The normal executable was not rebuilt or replaced.

### Complete fall through first landing

Pre-controller-publication result: **199 PAL fields for 164 updates (41.21 FPS)**,
independently repeated with deterministic emulation and 512 KiB Chip. The
historical target remains 181 fields (45.30 FPS), not a new 50 FPS requirement.
The current normal stripped build passes the 512 KiB Chip startup check; this
configuration still has 8 MiB Fast and 512 KiB slow RAM.

Empty prepared terrain segments now use their reference RNG-only executor,
without terrain admission or a lifetime/layer publication session. A bounded
state-only probe found seven empty segments among twenty in frame 163; their
draw counts were 8/2/1/3/4/2/3. They contain no Create callbacks. The measured
empty publication-session cost was about 3 ms in that frame, with additional
admission overhead. Non-empty terrain publication is unchanged.

Matched parent/current 512 KiB Chip captures: frame 163 **141.2 → 137.0 ms**,
Step 118.8 → 114.9 ms; marks 161–166 total **2,470,670 → 2,442,890 cycles**.
The empty `publishRun` path disappears from the profile. Marks 57–63 are
1,024,260 → 1,024,330 cycles, noise. The complete fall independently repeats
at 199 fields versus 200; this cadence difference alone is below the noise
threshold. The burst's removed work is the performance evidence, not the
one-field change. All 165 native state samples match, the 18,000-frame terrain
replay agrees, and builder fixtures verify complete RNG state at each Create,
failure prefixes and empty segments consuming draws without callbacks.
Normal Hunk: 1,769,680 bytes; no memory allocation added. Profile ELF SHA-256:
`84ed8c0c219340f6331448c1fd29e367a858f38f8cea5aff3223bd6eb95d0156`.
Normal Hunk SHA-256:
`297662d5ebc6d4d5899bd86b2c19d37e56d7948ad971afd86af3fef07930a3ab`.
Reports: `tmp/quiet-empty-terrain-{control-onset,control-stall,onset,stall}.txt`;
cadence: `tmp/quiet-empty-terrain-{cadence,repeat}.log`.

Previous empty-alarm-phase comparison (200 fields):

The captured alarm phase retains whether its sole event owner is the player.
When no external box/enemy alarms participate and the live player damage
counter is negative, no callback can arm another column: skip the empty phase.
Counter zero still runs, as do positive counters and captures with other event
owners. Birth/capture invalidation revokes the proof; scalar ownership retains
the full dispatcher. This is phase removal, not another alarm-group/roster
variant. A read-only state probe at the onset found objects 35/61/65, event
mask 2, player counter -1, and no player-controller/run-controller instances.

The estimated removable cost was 1.3 ms/update. Matched parent/current native
captures give alarm phase **1.57 → 0.29 ms** and marks 57–63 totals
**1,129,010 → 1,024,260 cycles** (22.70 → 20.59 ms/update including waits).
Worst interval is 26.5 → 25.2 ms. The complete fall repeats at 200 fields,
versus the parent's 205; cadence alone is below the 3% significance threshold.
The changed phase's traced cost establishes the saving. Landing marks 161–166
are 2,468,890 → 2,470,670 cycles, noise; frame 163 is 140.9 → 141.2 ms, with
unchanged 118.8 ms Step. The normal stripped Hunk is 1,770,056 bytes; no Chip
allocation is added. Negative/zero/positive counters and subsequent box birth
have explicit fixtures; the owner suite, 18,000-frame replay and all 165
native state samples agree. Profile ELF SHA-256:
`d59a937292632e4f75dc60ea5229935efd3a886be69f8b1aadc4f6999307f746`.
Normal Hunk SHA-256:
`935b2bdad3677a68145080b07388f7f20af757803211cb5f99fcf53c8b79200f`.
Reports: `tmp/quiet-empty-alarm-{control-onset,control-stall,onset,stall}.txt`;
cadence: `tmp/quiet-empty-alarm-{cadence,repeat}.log`.
The complete current ledger has 26 missed updates (no overflow). Updates
57–121 contribute eight extra fields versus zero historically, leaving this
interval responsible for eight of the remaining 19 extra full-fall fields.

Previous animation-phase comparison (205 fields):

The empty native animation phase now has a lifetime-maintained population
guard. Inactive and marked lifetimes remain counted until retirement; new
families conservatively participate. The reference walk remains available.
The estimated removable cost was 1.09 ms/update in the onset window. Matched
frames 57–63 cost 1,151,940 → 1,129,010 cycles (23.16 → 22.70 ms/update,
including waits); animation traversal itself falls from about 1.09 to 0.08 ms.
Some saved work becomes publication waiting, rather than elapsed-time savings.
The window's worst interval falls from 27.4 to 26.5 ms. Frames 161–166, where
enemies require the walk, cost 2,465,170 → 2,468,890 cycles (noise); frame 163
is 140.6 → 140.9 ms. Full-fall cadence changes 208 → 205 fields, below the FPS
noise threshold by itself; the traced eliminated traversal is the evidence.
All 165 selected state samples and the 18,000-frame ownership replay agree.
The population costs four ordinary bytes and no Chip allocation. Normal Hunk:
1,768,832 bytes, SHA-256
`f429730d9ac7b123617ba9a5e7bac26c5c417a14a1bcd1a511e81e392a5fa742`.
Profile ELF SHA-256:
`9867a0f8dec545b0324ff458d91d342cd9fbd9dcfe1e5a9a47c89efc1fbcbf32`.

Matched historical/current onset captures (marks 57–63, 12-field captures
starting at mark 56, deterministic, both 1 MiB Chip) isolate the remaining
regression. Historical ELF is the `13e08ca2` hash below; current is the hash
above. Selected totals are 1,003,630 versus 1,128,990 cycles. The current
512 KiB capture is 1,129,010 cycles: the expanded comparison is not hiding
a target-memory slowdown in this window.

| Inclusive cost, ms/update | Historical | Current |
| --- | ---: | ---: |
| Whole marker interval, including waits | 20.18 | 22.70 |
| Gameplay Step | 9.20 | 12.99 |
| Terrain generation callback, within Step | 1.60 | 2.74 |
| Player movement, within Step | 2.10 | 2.04 |
| Alarm phase, within Step | 0.91 | 1.57 |
| DitherAbove Begin Step, within Step | absent | 1.17 |
| Actor Draw collection | 1.30 | 1.76 |
| New-row painting/transfer | 1.89 | 1.84 |
| Display row restoration | 1.22 | 1.26 |

Do not sum nested rows or interpret the marker interval as work excluding
waits. Ordinary Step updates 57/59 are 7.8 → 10.3 ms; generation updates
58/61/63 are 11.0/11.2/10.9 → 16.8/16.0/15.9 ms. The prepared-pair
publication now costs 1.27 ms averaged over all seven updates, about 2.97 ms
per call (three calls). Native lifetime construction accounts for 1.06 ms
of that average, including 0.62 ms in the collision-owner span publication.
This identifies the surviving border-birth ownership boundary, not an
individual arithmetic/copy helper, as the next generation investigation.
The historical absence of source-required DitherAbove is not permission to
remove it. Scrolling/restore and player movement are not the observed cause.
Reports: `tmp/quiet-historical-onset-report.txt` and
`tmp/quiet-current-onset-1m-report.txt`; raw traces removed after commit.

Earlier RNG-preparation comparison (`e0e3f138`): **210 fields / 39.05 FPS**,
independently repeated. Idle-time preparation of the existing RNG ring reduces
frame 163 from **155.76 to 141.3 ms**. The historical 181-field result is still
not restored. The ring remains 4 KiB; no new asset or buffer is allocated.
One producer word is prepared only during an already-required publication
wait, with an eight-raster-line scheduling margin. Consumer draws, state and
the ordinary on-demand fallback are unchanged.

Matched 512 KiB Chip captures: frames 91–96 cost 1,056,390 → 1,056,210 cycles
(noise); frames 161–166 cost 2,572,340 → 2,468,170 cycles. Following frames
164–166 remain about 51.8/52.1/42.5 ms: the saving is not a stall deferred to
the next update. The full-fall one-field improvement alone is below the FPS
noise threshold; the accepted evidence is the traced worst-frame reduction.
Treat producer work inside the wait as CPU work, not as eliminated arithmetic.
All 165 sampled states match; the RNG oracle checks every output and all
consumer state words across variable idle budgets, wrap, copies and reseeding.
The restored normal 1,767,540-byte Hunk passes 512 KiB Chip startup.
Profile ELF SHA-256:
`56b1fea95865a0f17fb1e91b9fb03baca8c0c95911f4ff35eb5e3fce3ed921e4`.
Normal Hunk SHA-256:
`05de8b71e111b879bf4883eb9657c1de992fdd51d25833216b4cd90540ad9d62`.

Reproduce the current target-memory cadence from the repository root with:

```sh
bash amiga/profile.sh quiet-fall
```

This cleans/builds a stripped no-input profiling executable, defaults to
512 KiB Chip and explicitly enables FS-UAE deterministic mode. It stops at
first landing after 164 updates, prints its executable hash and
`QUIET_CADENCE`/`QUIET_LANDING`/`QUIET_MOTION`, and restores the normal stripped
build on exit. The emulator run has a 90-second deadline; build time is
separate. The raw debugger result remains in
`amiga/.run/profile-quiet-fall.log`. `VERIFY_CHIP_KB=1024` explicitly selects
the expanded-memory historical-comparison configuration; it is not the default.
This is a cadence measurement, not a pixel oracle or a goal-completion verdict.
The harness rejects more than 206 PAL fields: the repeated `f8113b6c` baseline
of 200 plus 3% timing allowance. This guards against a new regression; it does
not replace the historical 181-field restoration target. Tighten the ceiling
only after repeatable native evidence, never raise it merely to pass a run.
`tools/verify-quiet-fall.py` checks the complete printed landing witness
(position, velocity, camera, grounded state, health, ammo, enemy/bullet counts
and consumed RNG count), unique completion records and cadence accounting.
It rejects mismatched or incomplete measurements. Its optional `--max-fields`
argument applies an explicit ceiling after the correctness checks; without
that argument it judges only the witness. Neither mode proves full-state/pixel
equivalence. Its sabotage fixtures are
run by `make test-quiet-fall-witness`.

The earlier launcher left FS-UAE deterministic mode unset. An unchanged
`bf566363` profile executable subsequently produced 206 fields as well as the
recorded 208, with matching landing state. Do not treat this two-field
variation as a gain. With `DETERMINISTIC=1` explicitly supplied, two bounded
512 KiB Chip runs and a matched 1 MiB Chip current run reproduce 208 fields;
the historical 1 MiB Chip control reproduces 181. This does not establish a
zero noise floor for all runs.
Logs: `tmp/quiet-witness-{current,fixed-1,fixed-2,fixed-1m,old-fixed}-gdb.log`.
Use `DETERMINISTIC=1` for direct frozen-binary verification commands too.

The complete no-input fall is 164 updates. `13e08ca2` and `f5c7705f`
match all 165 pre-Step observations through tick 164 using
`amiga/profile_quiet_landing.gdb`. At landing: X=0, Y=1024 pixels,
vertical speed=0, grounded, health=4, ammo=8, enemies=3, RNG draws=1371.
These are selected observable states, not full-state or pixel equivalence.

Separate runs with **no debugger stops inside the fall** give:

| Complete-fall cadence | Historical `13e08ca2` | Current `f5c7705f` |
| --- | ---: | ---: |
| PAL fields for 164 updates | 181 | 222 |
| Elapsed emulated time | 3.62 s | 4.44 s |
| Mean interval, including waits and generation | 22.07 ms | 27.07 ms |
| Update rate | 45.30 FPS | 36.94 FPS |
| Worst field gap (frame 163, both builds) | 5 fields | 8 fields |

An independent repeat of each frozen build reproduces its field count,
worst gap and final observation exactly.

The short steady windows below do **not** establish restored performance
through landing. The full fall still takes 0.82 seconds longer. Worst gaps
are field-quantized, not instruction-traced work costs; do not attribute the
whole difference to frame 163 without tracing the rest of the fall.

Build both arms with `TRACE_FRAMES=165 PROFILE_FRAMES=600`, keeping the
no-input flags and symmetric memory configuration described below. Run
`profile_quiet_landing.gdb` separately for state, then
`profile_quiet_cadence.gdb` for timing. The latter stops only at trace-ready
frame 165 and requires 164 completed gameplay updates and a grounded player.
For either probe use `GDB_SOURCE=profile_quiet_<landing|cadence>.gdb
VERIFY_CHIP_KB=1024 VERIFY_TIMEOUT_SECONDS=90 ./verify_startup.sh`, with
`READY_MARKER=DOWNWELL_QUIET_LANDED` or
`READY_MARKER=DOWNWELL_QUIET_CADENCE_READY`, respectively.
Do not attempt to extend a short run by writing the frame limit through gdb:
the FS-UAE stub silently drops guest-memory writes. Rebuild with the limit;
the landing probe rejects limits below 600.

Complete-fall ELF SHA-256:

- Historical: `d1d1c64d3690c89ac343545c979946cd917543825ada4834c18a12c324f2a115`.
- Current: `8e16974d7fb84a9fa6954c547fa3b42ca573d350884fa46b91698a515b4b8ff5`.

### Short instruction-traced windows

The prepared initial-border pair now uses the existing certified span owner
instead of two scalar lifetime/layer births. Frame 93 is **34.03 → 29.91 ms**
(241,790 → 212,510 cycles); generation is 10.06 → 6.07 ms. The replacement's
prepared publication costs 2.93 ms, including 2.45 ms native construction.
It does not change the RNG draws preceding the pair or the following row
setter. The static 18-byte description has process lifetime because native
terrain ownership borrows its create sequence; callbacks/admission failures
retain the scalar executor.

The complete fall is now **211 PAL fields / 38.86 FPS**, independently
repeated, versus 222 / 36.94 before this change and 181 / 45.30 historically.
All 165 selected state observations still match. Frames 151–164 cost
3,951,952 → 3,946,902 cycles in matched captures; frame 163 remains 155.8 ms
and frame 164 changes 51.9 → 51.8 ms. These differences are noise, not a
claimed generation-stall saving. The 18,000-frame native ownership comparison,
incremental terrain tests and builder callback/failure-prefix checks pass.
The normal stripped Hunk is 1,767,696 bytes and passes 512 KiB Chip startup.
Candidate profile ELF SHA-256:
`d2829f9e5b8024ce2c0913326ff6ead2a0ae0b39e57dd671df9f4a0ef6e338f0`.
Normal Hunk SHA-256:
`0cef6f2a03d9f78aab9f9ee20c2bca23bdf58b1fbe7f0715c68f1b59fab984b6`.

The same frozen candidate also completes the **entire fall with 512 KiB
Chip**, using `VERIFY_CHIP_KB=512` with `profile_quiet_cadence.gdb`:
164 updates, 211 fields, worst gap 8 at frame 163; landing tick 164,
X=0/Y=262144 Q8, health 4, ammo 8, three enemies, 1,371 RNG draws.
This is gameplay evidence beyond the normal splash/menu startup gate.
It still uses 8 MiB Fast and 512 KiB slow RAM; it is not a stock-total-memory
fit claim. Its result matches the symmetric 1 MiB Chip comparison.

A separate 20-field capture from frame 150 isolates the last steady frames
**151–162**, excluding the frame-163 generation burst. Both builds provide
12 complete frames: historical 2,157,990 cycles (25.31 ms/frame), prepared
pair 2,471,990 (28.99 ms/frame). Inclusive matched costs are:

| Last steady fall, ms/frame | Historical | Prepared pair |
| --- | ---: | ---: |
| Gameplay Step | 13.30 | 17.98 |
| Alarms, within Step | 2.66 | 3.48 |
| Crawler update, across Step/Alarm | 1.72 | 1.71 |
| Player movement, within Step | 2.06 | 1.97 |
| DitherAbove Begin Step | absent | 1.17 |
| Terrain painting | 2.39 | 2.41 |
| Actor Draw collection | 1.79 | 2.24 |
| Scrolling-row synchronization | 1.18 | 1.16 |

Do not blame terrain painting or crawler collision arithmetic for this
remaining regression. The controller/dispatch/publication contracts need to
be separated from redundant bookkeeping; the absent historical DitherAbove
event is not removable source behavior. These inclusive rows overlap and
must not be summed as independent phases.

The first-landing stall was separately recaptured at **512 KiB Chip** from
frame 160 for 20 fields, using the prepared-pair ELF above. Complete frame
163 is **155.76 ms**, agreeing with the 1 MiB capture. Of this, generation
costs 115.85 ms and terrain painting 6.84 ms. Generation's principal
non-overlapping publication branches are:

| Frame 163 generation branch | Inclusive ms |
| --- | ---: |
| Prepared wall/platform segments | 47.26 |
| Breakable backend publication | 16.21 |
| Breakable Create evaluation | 11.90 |
| Other event-cell object emission (including enemy births) | 13.34 |

Within the 47.26 ms prepared branch, native construction is 35.99 ms and
its `constructTerrainSpan` owner is 19.05 ms. The latter's own instructions
are only 25,724 cycles (3.62 ms), spread across 46 source locations; it is
not one expensive arithmetic helper. Its nested layer publication, room
membership and admission cost 3.59, 3.35 and 3.15 ms, respectively. These
numbers locate costs, not justify reviving the rejected registration-owner
or fresh-cell-initialization variants. Breakable publication likewise
already uses `NativeFamilyBirth`; it is not the old generic box path.
No runtime change was made from this attribution alone.

Matched later-fall captures (same complete-fall ELFs above), starting at
frame 90 for 100 PAL fields, expose a recurring row-publication regression.
Both reports select **91–157**, 67 complete updates; the current capture
ends before the first landing and does not measure frame 163.

| Inclusive cost | Historical | Current |
| --- | ---: | ---: |
| Frames 91–157 mean, including wait | 22.73 ms | 29.35 ms |
| Frame 93 total | 23.83 ms | 34.03 ms |
| Frame 93 terrain generation | 3.36 ms | 10.06 ms |
| Frame 93 builder emitObject | 0.91 ms | 6.92 ms |
| Frame 93 terrain painting | 2.90 ms | 2.90 ms |
| Frame 93 actor Draw collection | 1.30 ms | 2.83 ms |

Totals for the matched window are 10,821,280 and 13,970,220 CPU cycles.
An independent 12-field current capture reproduces frame 93 exactly:
241,790 cycles. Current generation includes 5.93 ms in native birth,
4.53 ms in lifetime append, and 2.98 ms in collision-owner terrain birth;
these are **nested**, not additive costs. The latter includes 1.51 ms of
generic layer birth. The source builder's `generationCount_ < -1` branch
advances its RNG, then emits two wall Creates at X=160 and X=320 before
advancing the row. This path still uses separate scalar lifetime/layer
publication, unlike admitted prepared terrain segments. Source identity,
Create order and failure prefixes remain required; dropping wall ownership
is not an acceptable restoration. Any replacement must remove these two
scalar publications as a whole, not tune the nested lookup/copy helpers.

Reproduction: use the complete-fall build, break at
`downwellTraceFrameMark`, ignore its first 89 hits, continue, then delete
that breakpoint before `monitor profile 100 "" ".../trace-90.bin"`.
Report `--frames=91-157` and `--frames=93` with the matching ELF. There are
no debugger stops inside the capture. Use a separate 12-field capture from
the same boundary to confirm frame 93. The normal build is not replaced
by these frozen-binary captures.

### Where the remaining fall loses fields — `e0e3f138`

Active prepared terrain spans now preserve the actor Draw cache: the span
publisher admits an existing layer and appends only active, nonprojected
scenery. It cannot change actor order or insert an inactive cutoff. The full
layer revision still advances; activation, depth and retirement retain their
invalidation paths. The ordinary/reference Draw traversal remains available.

Against the frozen `e0e3f138` control, onset marks 57–63 change from
1,168,550 to 1,151,890 cycles (**23.50 → 23.16 ms/frame**). Draw collection on
affected marks 58/61/63 drops from 2.8/3.0/2.8 to 1.7/1.7/1.7 ms, meeting the
roughly 1 ms affected-update estimate. Mark 59 grows 1 ms entirely in the
publication-wait path; the window's worst interval improves 28.5 → 27.4 ms.
The landing control/candidate window 161–166 changes 2,468,150 → 2,465,150
cycles, with frame 163 at 141.3 → 140.6 ms and other changes within 0.3 ms.
Do not claim that small landing movement as a structural burst improvement.

Two complete 512 KiB Chip candidate runs give **208 fields / 39.423 FPS** for
164 updates, versus the parent's 210 fields and historical 181. The two-field
aggregate difference alone is below the FPS noise threshold; the accepted
evidence is eliminated Draw-order reconstruction on affected updates. All
165 selected state samples match. Native/reference ownership agrees across
18,000 frames including actor Draw order. The focused fixture checks stable
actor projection through active span birth, invalidation and cutoff on
deactivation, reactivation, retirement and re-entry. Normal startup passes
with a 1,767,312-byte stripped Hunk and unchanged 512 KiB Chip setting.

Candidate profile ELF SHA-256:
`934d6f24c542e87320adea6f6494c47c3dd40748ff463aa70c0d69c065798476`.
Normal Hunk SHA-256:
`25a62fdb67cfe1b8f8601b2d2248c4c30e4241153d67e4ff2c0fac083038c67a`.
Reports: `tmp/quiet-projection-{control-onset,onset,control-stall,stall}-report.txt`;
cadence/state: `tmp/quiet-projection-{fall-gdb,repeat-gdb,state}.log`.
Capture onset at mark 56 for 12 fields and landing at mark 160 for 20 fields,
using the same frozen build/flags and bounded verification command below.
Raw traces are deleted after these measurements are committed.

The cadence probe now prints the existing missed-field ledger after the run,
without adding any timed instrumentation. Historical `13e08ca2` records 13
updates taking more than one field; current `e0e3f138` records 36. The current
ledger holds only the first 32, through update 157: its final four entries are
not available, and must not be inferred from this listing.

Updates **57–121** account for **17 extra fields** in current versus none in
historical. This is 340 ms of the full fall's 580 ms excess (210 versus 181
fields), before the larger bursts at 122, 136 and 163. The current missed
updates in this interval are 57, 63, 67, 71, 74, 79, 83, 87, 91, 95, 99,
103, 107, 111, 115, 118 and 121. The expanded-memory control and target-memory
current run have identical totals and saved entries. Historical requires
1 MiB Chip with the rebuilt assets; current was checked with both 512 KiB
and 1 MiB Chip, keeping the other machine settings unchanged.

A 12-field instruction capture beginning at mark 56 checks the onset on
current 512 KiB Chip. Complete marks 57–63 average **23.50 ms**, with 18.7–28.5
ms marker intervals. Gameplay Step averages 14.05 ms, including 2.79 ms in
builder dispatch, 2.05 ms in player movement, 1.55 ms in alarms, 1.17 ms in
DitherAbove and 1.09 ms in animation dispatch. Draw collection averages
2.22 ms; row transfer/drawing 1.84 ms; restoration 1.18 ms. These are inclusive
boundaries, not additive leaf costs. Small initial-border generation is
already running here: this is not a generation-free window. The ledger's
field count and marker-to-marker CPU interval have different boundaries;
do not identify the expensive handler from a ledger entry alone.

This prioritizes the recurring 57–121 workload over further isolated
first-landing burst work. It does not establish a new performance improvement
or justify changing individual sub-millisecond helpers.

Reproduce the ledger with `profile_quiet_cadence.gdb` and the complete-fall
build above. For the onset trace, ignore 55 hits of `downwellTraceFrameMark`,
delete the breakpoint, then capture 12 fields and report `--frames=57-63`.
Current ELF SHA-256:
`56b1fea95865a0f17fb1e91b9fb03baca8c0c95911f4ff35eb5e3fce3ed921e4`.
Reports are `tmp/quiet-misses-{old,current,current-1m}-gdb.log` and
`tmp/quiet-onset-report.txt`; raw trace removed after recording these figures.

The immediate regression target is the quiet fall, not the contact tape or
the separate 50 FPS goal. Rebuilt `13e08ca2` and `06d1a583` match all 160
pre-Step observations (ticks 0–159): player position/velocity/grounded state,
camera, health, ammo, enemy count, bullet extent and RNG draw count. No buttons
are pressed. This is observable-state agreement, not a full replay digest;
the player has not landed by the last observation.

Separate uninterrupted instruction captures give these matched-frame means:

| Work, ms/frame | Historical | Before pixel controller | Current |
| --- | ---: | ---: | ---: |
| Frames 11–20, including publication wait | 20.35 | 22.03 | 20.68 |
| Frames 11–20, excluding that wait | 15.78 | 21.53 | 18.51 |
| Frames 51–55, including publication wait | 20.64 | 23.69 | 20.48 |
| Frames 51–55, excluding that wait | 17.12 | 23.01 | 19.97 |
| Gameplay Step, frames 51–55 | 7.43 | 12.00 | 11.08 |
| Player movement, inside Step | 2.10 | 2.04 | 2.04 |
| Alarm phase, inside Step | 0.91 | 1.55 | 1.55 |
| Actor Draw collection | 1.30 | 1.75 | 1.75 |

Worst marker-to-marker times are 26.1 → 29.2 ms for 11–20 and
23.4 → 25.0 ms for 51–55. These include variable display waiting; they are
not worst work-only costs. Frame 56 generates terrain and is deliberately
excluded from the steady-fall window. Before the pixel adapter, DitherAbove
Begin Step alone costs 2.08 ms in 51–55. Source-required controller behavior cannot
simply be removed: the structural question is the added phase traversal,
state publication and ownership work. Player physics and terrain row copying
do not explain the observed increase.

The pixel controller retains integral Y at publication boundaries and consumes
the already-integral gameplay camera. Fractional/large imported Y falls back
to the source translation. Ordered setters and camera-Y read timing are
unchanged. Its inclusive cost is 2.08 → 1.15 ms in frames 51–55. All 160
observations still match, and the native/scalar/snapshot ownership witnesses
agree. This restores about 0.9 ms of work, not the historical baseline.

The current column also skips controller composition when all producer owners
are empty, and currency-promotion admission when there are no currency slots.
This removes the empty surface preparation, shade-admission traversal, view
setup and publication: 22.10 → 19.97 ms work in 51–55, 20.67 → 18.51 ms in
11–20. Presence is conservative (including pending retirement and off-screen
records); shade alone only subtracts pixels. Existing row damage restores old
effect pixels before the gate, and a later nonempty pass clears/rebuilds the
retained surface before reading it. These savings are not a claim that the
historical quiet baseline or the whole goal has been restored.

With this gate, worst marker intervals are 26.3 ms in 11–20 and 22.6 ms in
51–55. The early maximum includes 3.57 ms publication waiting in frame 19
(22.76 ms work); an independent 12-field recapture reproduces all ten frame
totals exactly, 1,469,450 cycles. The normal 1,767,508-byte stripped executable
passes the 512 KiB Chip startup gate. Producer coverage and 1,024 alternating
buffer/wrap/retirement frames check that omitted publication leaves no trails.

Before the empty-producer gate, worst marker-to-marker times are 25.1 ms for 11–20 and 25.8 ms
for 51–55. The latter rises from 25.0 ms because frame 54's publication wait
moves from 3.34 to 5.83 ms: that frame's work falls from 20.54 to 20.00 ms.
Independent 12-field re-captures of control and candidate reproduce all five
frame totals exactly (841,440 versus 826,700 cycles). Do not describe the
longer wait as a computation regression or omit it from reported cadence.
The normal candidate passes the 512 KiB Chip startup check; its stripped
Hunk is 1,766,656 bytes. No new assets or Chip buffers are allocated.

Source check: `13e08ca2` has no authored `ditherAbove_` Begin Step in
`GameplayWorld::step`. Object 61 was introduced by `16ccf0ff` and subsequently
connected to room admission. Thus its pre-adapter 2.08 ms is not all redundant
bookkeeping, nor does matching the 160 player-state observations prove full
old/new behavior equivalence. It is **not** entry-wipe object 73. Assembly
`gml_Object_objDitherAbove_Step_1.asm` requires camera X publication, a single
16-pixel Y advance when the camera gap reaches 272 (208 in low-spec), Y
rounding, and ending destruction. Wall contacts use this published position.
The reference test is `make verify-dither-above`; it checks the pinned source,
ordered writes and failure prefixes. Removing the controller, delaying its
published position until Draw, or turning its single increment into catch-up
would invalidate the restoration. Its roughly 0.76 ms of position setters
is included in the 2.08 ms, not an additional saving. Any reduced native
representation must retain the whole controller's behavior and be measured
against the current implementation as well as the historical target.

Both captures use A500/68000, PAL cycle exact, **1 MiB Chip**, 512 KiB slow,
8 MiB Fast. The rebuilt historical executable fails allocation with 512 KiB
Chip using the current generated asset snapshot; 1 MiB is used symmetrically
for this comparison only. It does not replace the production 512 KiB startup
gate. The old build uses its own field-column generator (the generated C++
layout changed); its only gameplay edit disables automatic left/right/fire
input. Its launcher/environment use the current shared tools. This is a
rebuild comparison, not the original historical binary's absolute timing.

Current flags: `SKIP_TO_GAME=1 PROFILE_AUTORUN=1 PROFILE_IDLE=1 FPSCOUNT=1
TRACE_FRAMES=10,50,90 PROFILE_FRAMES=160 STRIP=1`. Historical flags omit
`PROFILE_IDLE` and `STRIP`, with the no-button harness edit instead.
Use `profile_quiet_state.gdb` in a separate state-only run. Trace with
`profile_trace.gdb`, three captures of 12 PAL fields, and report precisely
`--frames=11-20` or `--frames=51-55`. Do not time the state-breakpoint run.

Measured ELF SHA-256:

- Historical: `784563b8b4e74de6f00c66b5feb34a47d4b1ef1186137922a0ac0fc1f29f9be9`.
- Current: `3c2847e4781903b7a7db5f1c18313f3219476830679c39c9050485ea96391982`.
- Pixel controller: `bb2721d775e78242376cc4beb788bd222d4bf8bf29095f6eb92f68f3c182314f`.
- Empty-producer gate: `40ef410c89f409b5bb7cd211533763c74e4ad563cc3a91f81096f89f9e654e94`.

The normal stripped executable was restored byte-identically after capture
(`c8bf6211b9ad2c965fe46f27c1e9268e75ac4e710e51ad9f0d824a90f9f4fa28`).
After the pixel-controller run the restored normal Hunk hash is
`01ec13a2fec8e0125290cd4e1a92058750aad633d3f4f2457cfc4421f6334a2b`.
After the empty-producer gate the normal Hunk hash is
`cc67e7b42c20c4d373b1b0a2b983681aabb5275bb648dcdb9ce34d08e2de1c24`.

## Regression comparison baseline

The fixed comparison baseline is game revision `e8588c07`, A500/68000 with 512 KiB Chip, 512 KiB slow and
8 MiB Fast. Exact complete-frame cycles, partial-frame exclusions, trace hashes
and the matching ELF hash are committed in
[`amiga-frame-baseline.json`](amiga-frame-baseline.json). This is a measured
baseline, not a claim that the regression or 20 ms budget is resolved.

| Matched frames | Mean ms | Worst ms |
| --- | ---: | ---: |
| 51–70 | 33.4059 | 46.5187 |
| 676–681 | 101.0014 | 238.9664 |
| 1047–1055 | 64.6839 | 91.9438 |
| 1305–1314 | 60.3150 | 79.0418 |
| 1512–1521 | 60.6326 | 139.5814 |
| 1535–1545 | 55.4820 | 72.2706 |
| 1689–1694 | 89.5376 | 221.2552 |

Frame 51 alone is 26.4 ms; the early tape includes firing, not idle input.
Capture duration is **not** a comparison window: a faster build fits more
updates into the same number of PAL fields. The ranges above select exactly
the same complete frames in both revisions. The last capture does not fully
contain frame 1695, so it must not be included. Do not average rounded values
copied from the Markdown report when exact cycles are available.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512 \
  VERIFY_TIMEOUT_SECONDS=180 STRIP=1 \
  TRACE_FRAMES=50,675,1046,1304,1511,1534,1688 TRACE_FIELDS=35 \
  TRACE_OUTPUT="$PWD/tmp/trace/candidate" bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/candidate --causes \
  --frame-summary tmp/candidate-frames.json > tmp/candidate-report.txt
python3 tools/amiga-frame-compare.py docs/amiga-frame-baseline.json \
  tmp/candidate-frames.json \
  --frames 51-70,676-681,1047-1055,1305-1314,1512-1521,1535-1545,1689-1694
```

The comparison rejects missing/partial frames instead of silently reducing
coverage. If a slower candidate fails to cover a range, recapture a longer
window. Overlapping complete-frame captures are rejected rather than summed
into an apparent longer frame. Workload and replay equivalence remain separate
requirements; matching frame numbers alone does not prove either. The trace
report's `--frames` selector now restricts its frame table as well as attribution.
Reference report/ELF: `tmp/baseline-exact-windows-report.txt` and
`tmp/trace/baseline-exact-windows/Downwell.elf`.

## Phase attribution at the current baseline

### Remaining generation structure with phase-owned builder publication

The 40-field contact capture measures frame 676 at 226.95 ms, with
152.98 ms in generation. Inclusive costs (do not sum nested entries):

| Work | ms |
| --- | ---: |
| Prepared terrain emitter | 91.10 |
| Native terrain construction | 73.95 |
| Append prepared cells | 65.29 |
| Span ownership/publication | 39.75 |
| Room-span membership, inside publication | 7.82 |
| Layer-span publication, inside publication | 6.63 |
| Layer admission, inside publication | 5.49 |
| Fresh-table admission, inside publication | 3.31 |
| Builder-position publication across row advances | 5.10 |
| Terrain row synchronization, outside generation | 2.8 |

Builder-position publication still includes 3.08 ms narrowing source coordinates.
The external-collision scene lends its stable coordinate mirror for the enclosing
generation phase, validating ownership on the first actual row publication.
Every row writes both the controller record and public mirror immediately;
nested Create reads (including retained record pointers) and failed-birth
prefixes are not deferred. Retirement while that storage is borrowed fails
before mutation. Full collision service and unleased generation keep the
lifecycle callback reference. Do not reopen this boundary as a narrowing or
setter-helper optimization.

Against the preceding accepted contact trace (ELF `b0f1606ff97a089dc15f6e8e8540ba4329fe9c0bd4f74ad152a38289298e2fac`),
generation frames 676/1512/1689 change from 232.22/136.29/214.57 to
226.95/132.08/207.64 ms. The largest increase in any matched frame across
54–69, 676–681, 1305–1314, 1512–1521 and 1689–1694 is 0.35 ms.
An independent first capture measured frame 676 at 226.85 ms.
The no-input generation guard (163–174) has mean/worst 55.74/158.89 ms,
versus the committed guard's 56.08/163.06 ms. These are work-time reductions,
not a cadence win: the 1,800-update contact tape takes 4,955 PAL fields
(18.16 FPS), versus 4,952 (18.17 FPS), with the same 12-field maximum gap.

Span construction is already batched. Its source-line attribution has no
single large loop body that justifies another local rewrite; the helper costs
above are not all removable work. Keep the rejected registry and per-cell
encoding approaches closed.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512 STRIP=1
VERIFY_TIMEOUT_SECONDS=180 TRACE_FRAMES=53,675,1304,1511,1688 TRACE_FIELDS=40
TRACE_OUTPUT="$PWD/tmp/trace/builder-phase-final" bash amiga/profile.sh trace`,
then report `--frames 676 --causes`. Reports:
`tmp/builder-phase-final-676.txt`, `tmp/builder-phase-compare.txt` and
`tmp/builder-phase-idle-compare.txt`; contact trace ELF SHA-256:
`28c8c65ff785cb09acf10f14866ab65f129213ce9acc8cacf7667122a59ed65c`.

### Firing versus retirement boundaries

A bounded recheck at `f1b720e7` separates two early contact-tape spikes before
any enemies are allocated. These are different structures, not one repeated
HUD cost. Values are inclusive milliseconds; nested rows must not be added.

| Boundary | Frame 54 (shot birth) | Frame 60 (retirement) |
| --- | ---: | ---: |
| Whole frame | 46.40 | 44.69 |
| Gameplay Step | 29.76 | 30.01 |
| Projectile Step, all live bullets | 7.74 | 4.25 |
| Actor End Step, including admission | 0.94 | 12.99 |
| Bullet End Step, inside actor End Step | — | 7.50 |
| Generic projectile retirement, inside bullet End Step | — | 6.28 |
| Actor draw collection | 5.27 | 4.4 |

Frame 54 spends 3.16 ms in one birth and 3.19 ms in room animation; its
ammo redraw is 3.57 ms. Native birth itself is 2.34 ms, so removing only that
adapter cannot recover the entire shot spike. The projectile initialization
trigonometry is 1.76 ms, below the structural work threshold.

Frame 60 leaves `tryNativeActorEndStepInterior` and enters the complete
captured dispatcher (11.84 ms). Generic projectile retirement reads published
Y four times, performs source binary64 threshold arithmetic/comparisons, and
publishes destruction. The original no-op proof does not cover this boundary.
The native phase now additionally admits definitely-outside bullets when the
destruction function and owner match the pinned native binding. It captures
those handles in registration order before player End Step, then marks them
without the generic event evaluator. This is valid only while every box,
thin-platform and ordinary enemy retirement is proven a no-op and no other
End Step family is present. The native mark cannot move actors or create new
ones; repeated source destroy requests already become no-ops after its first
successful mark. Cleanup remains deferred. Ambiguous floating-point boundaries,
custom bindings and unsupported phases retain the complete callback adapter.
The no-op-only selector and source evaluator remain reference paths. No new
per-instance storage or task-stack snapshot is introduced.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512 STRIP=1
VERIFY_TIMEOUT_SECONDS=120 TRACE_FRAMES=53 TRACE_FIELDS=24
TRACE_OUTPUT="$PWD/tmp/trace/shot-phase" bash amiga/profile.sh trace`.
Run the reporter separately with `--frames 54` and `--frames 60 --causes`.
One window avoids overlapping capture markers. Reports are
`tmp/shot-phase-{54,60}.txt`; trace ELF SHA-256:
`f3c1583bd48bf842f5ace001d9e282e7e11d767f4b612f404d8999d20f040ed0`.
The normal stripped build is restored after the bounded run. This audit
identifies remaining work; it is not a performance improvement.

The independent no-input generation gate uses
[`amiga-idle-frame-baseline.json`](amiga-idle-frame-baseline.json), captured at
`27ff7482`. Do not compare these frame numbers with the contact tape. Reproduce
with `PROFILE_IDLE=1 SKIP_TO_GAME=1 PROFILE_CONTACT=0 PROFILE_FRAMES=600
VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=120 STRIP=1 TRACE_FRAMES=162
TRACE_FIELDS=35 TRACE_OUTPUT=$PWD/tmp/trace/idle-check bash amiga/profile.sh trace`.
Export `--frame-summary` with the trace reporter and compare frames `163-174`
using `tools/amiga-frame-compare.py`; partial frames 162 and 175 are excluded.
Keep both the no-input gate and the contact windows when evaluating generation.

A two-window recheck at `05a36ba3` isolates early frame 51 and combat frame
1305. This is diagnostic evidence, not an optimization or a new baseline.
Both frames reproduce the committed baseline within 0.5 ms.

| Inclusive owner | Frame 51 ms | Frame 1305 ms |
| --- | ---: | ---: |
| Whole frame | 26.35 | 61.51 |
| GameplayWorld Step | 16.15 | 43.46 |
| Room actor Draw collection | 3.02 | 4.41 |
| Enemy alarms, including captured dispatch | 1.55 | 3.24 |

In frame 51, bullet Step costs 2.95 ms, player Step 2.07 ms, room
animation 2.02 ms and DitherAbove Begin Step 1.67 ms. These are nested in
GameplayWorld Step, not additional frame costs. Bullet position publication
is 0.95 ms inside bullet Step; the source projectile computation is 1.22 ms.
This does not support an isolated setter or arithmetic-helper optimization.

Frame 1305 spends 16.63 ms in enemy Step, including walking motion/collision
5.91 ms and bat behavior 4.06 ms. Source-line attribution puts the box-prefix
scan/eligibility checks at roughly 2 ms, below the structural work threshold.
Moreover, `BoxDestructionScript::runRetained` calls `alarmStop(0)` even when
health is positive. A health-only pending-destruction queue cannot replace
this phase without retaining that behavior and its ordered callbacks.

Reproduction: use the baseline trace command above with
`TRACE_FRAMES=50,1304`, `TRACE_FIELDS=35` and
`TRACE_OUTPUT="$PWD/tmp/trace/phase-owner-audit"`. Run the report with
`--frames 51` or `--frames 1305`, plus
`--lines GameplayWorld::step --focus GameplayWorld::step --tree-min 0.15`.
The line selector is a substring selector: its output also includes
`stepEnemyRoomEffect` and other matching names; do not mistake their lines
for the outer Step's self cost. Reports are
`tmp/phase-owner-{51,1305}.txt`; matching retained ELF SHA-256 is
`0cf51e6fc452c443bb065fee48348da8ff55ab450df158932f6d27541d04f85f`.
Raw captures are disposable after recording the numbers. The normal stripped
build was restored after this bounded run (1,928,980 bytes).

## Bitmap-selected room cleanup

At `11f8ca10`, two retiring thin platforms make Draw cleanup cost 38.15 ms
in frame 1047 and 19.8 ms in frame 1535. The cause is expanding a marked
terrain run into individual cells, including every unmarked cell. Publication
splitting itself costs only 0.89 ms in frame 1047 and was deliberately left
unchanged. The estimated removable traversal cost was about 23 ms.

Cleanup now selects marked cells from the existing owner bitmap, bounded by
the current room run. It retains active-before-inactive ordering, saves the
next room position before each callback, and re-reads membership afterwards.
There is no new queue, storage or publication pass. The old cell traversal is
retained as `visitMarkedCleanupOrderedReference` for differential tests.
Frame 1047 cleanup falls to 13.89 ms. A fresh baseline/candidate capture gives
116.3 → 91.9 ms for frame 1047 and 81.7 → 72.3 ms for frame 1535.

Matched complete-frame ranges against the prior seven-window contact report
(extra frames now fitting into a 35-field capture are excluded):

| Frames | Mean before → after, ms | Worst before → after, ms |
| ---: | ---: | ---: |
| 51–70 | 33.580 → 33.405 | 46.5 → 46.5 |
| 676–681 | 101.033 → 101.017 | 239.5 → 239.0 |
| 1047–1055 | 67.278 → 64.700 | 116.4 → 91.9 |
| 1305–1314 | 60.310 → 60.320 | 78.8 → 79.0 |
| 1512–1521 | 61.170 → 60.720 | 139.3 → 139.6 |
| 1535–1545 | 56.355 → 55.445 | 82.0 → 72.3 |
| 1689–1695 | 85.629 → 85.500 | 221.2 → 221.3 |

Full contact tape: 5,010 → 4,996 PAL fields over 1,800 updates, 17.96 →
18.01 FPS; maximum gap remains 12 fields at generation frame 676. This removes
retirement scanning stalls, not the remaining generation or simulation costs.
Host tests cover sparse marks across word boundaries, callback-added marks,
interior removals, inactive suffixes and failure prefixes; the full terrain
oracle passes ten tapes / 18,000 frames / up to 928 terrain records.

Reproduce with the contact commands below using trace directories
`tmp/trace/retirement-attribution` (1046,1534) and `tmp/trace/sparse-cleanup`
(50,675,1046,1304,1511,1534,1688), 35 fields each. Reports:
`tmp/retirement-{1047,1535}.txt`, `tmp/sparse-cleanup-report.txt`,
`tmp/sparse-cleanup-1047.txt`, `tmp/sparse-cleanup-{host,fps}.log`.
Baseline/candidate trace ELF SHA-256:
`e98978b516ccd9f72748f5a53cdcc62c4d930e4d40c7f2d3079ab661b3f7453d` /
`3cf60a1c7d1b6a259827daeffc0368bff23394de91d3f2b70a6d8aabd98d3614`.
Normal STRIP=1 startup passes on 512 KiB Chip (plus 512 KiB slow and 8 MiB
Fast), retaining 126,840 free Chip bytes and the unchanged 904,628-byte owner;
reference allocations remain absent. Executable: 1,999,680 bytes. Evidence:
`tmp/sparse-cleanup-memory{,-gdb}.log`; normal ELF SHA-256
`f875a0dce7ea4a43f1662c4dfa46495cdf9468dd976dec0576fd019ac76bc9f5`.

## Direct admitted terrain transactions

Retained, callback-free spans in the external collision backend now publish
through one complete transaction, without entering the generic per-cell run
session. Native allocation preflights fresh slots and lifetime capacity; the
owner admits metadata, layer and room capacity before publication. No callback
observes room publication before native identities are installed. Unsupported
cases retain the previous session/scalar path; partial failures retain their
completed lifetime and publication prefix. This does not remove native identity,
layer, room, activation or platform registration ownership.

The initial estimate included about 13 ms of session-related work in frame 676.
Required metadata admission remains, so the actual frame saving is 5.4 ms;
native prepared construction measures 78.79 ms. Against `fa2be1ac`:

| First frame | Mean before → after, ms | Worst before → after, ms |
| ---: | ---: | ---: |
| 51 | 33.575 → 33.580 | 46.8 → 46.5 |
| 676 | 107.440 → 106.320 | 244.9 → 239.5 |
| 1047 | 67.333 → 67.278 | 116.4 → 116.4 |
| 1305 | 60.320 → 60.310 | 79.0 → 78.8 |
| 1512 | 61.400 → 61.170 | 141.5 → 139.3 |
| 1535 | 56.373 → 56.355 | 82.0 → 82.0 |
| 1689 | 86.543 → 85.629 | 227.3 → 221.2 |

No-input frame 163 improves 169.4 → 167.5 ms. Full contact tape: 5,010 PAL
fields / 17.96 FPS versus 5,013 / 17.95, maximum gap 12 at frame 676.
This shortens generation stalls, not ordinary gameplay. Reproduce with the
contact/idle commands below, using `tmp/trace/direct-span-contact` and
`tmp/trace/direct-span-idle`. Reports: `tmp/direct-span-report.txt`,
`tmp/direct-span-676.txt`, `tmp/direct-span-idle-report.txt`,
`tmp/direct-span-fps.log`. Contact/idle trace ELF hashes:
`4a552e69dad590442c3b5979f50ad097d1c3b457c54c52268d4bfa1900f8c4ee` /
`3312a3c2d789fd9dcedaf135ec72c36d26b79b5aa2dba0a10e3666a829def886`.

Direct/session/scalar transaction tests cover clocks, fields, failed prefixes,
unsupported fallback and session entry/exit counts; 18,000-frame ownership and
collision-owner activation/re-entry tests pass (`tmp/direct-span-host.log`).
Normal STRIP=1 startup retains 126,840 free Chip bytes and absent reference
allocations (`tmp/direct-span-memory.log`). The stripped executable is
2,000,800 bytes (+3,720); normal ELF hash
`d11c45abda3e70e2f2430b5299af9e1c49a7b200f0f2bffb9c3627395610b2af`.
The terrain binding adds one function pointer, not per-cell storage.

## Native corner-query publication

Walking/bat motion now submits its four corner samples to the native occupancy
backend together. Duplicate cell lookups are eliminated within that query;
there is no retained cache or additional terrain owner. The point-callback
oracle retains its order and short-circuit behavior. Source movement and event
order are unchanged. This targets the 4.10 ms walking tile-query and 2.14 ms
bat collision boundaries above, not their small arithmetic helpers.

Matched contact windows against `cca9e817` (same code as the later audit):

| First frame | Mean before → after, ms | Worst before → after, ms |
| ---: | ---: | ---: |
| 51 | 33.575 → 33.575 | 46.5 → 46.8 |
| 676 | 107.900 → 107.440 | 245.6 → 244.9 |
| 1047 | 68.989 → 67.333 | 118.0 → 116.4 |
| 1305 | 62.290 → 60.320 | 80.8 → 79.0 |
| 1512 | 61.850 → 61.400 | 142.2 → 141.5 |
| 1535 | 56.709 → 56.373 | 82.2 → 82.0 |
| 1689 | 87.000 → 86.543 | 227.9 → 227.3 |

No-input generation frame 163 is 169.4 ms (169.5 before). The uninterrupted
1,800-update contact tape takes 5,013 PAL fields / 17.95 FPS (5,074 / 17.74
before); maximum gap is 12 fields at 676, not the acceptance gate. This is
a modest combat saving, not a restored 20 ms baseline. Reproduction uses the
span-publication commands below with output directories
`tmp/trace/corner-query-contact` and `tmp/trace/corner-query-idle`.
Reports: `tmp/corner-query-report.txt`, `tmp/corner-query-idle-report.txt`,
`tmp/corner-query-fps.log`. Contact/idle ELF hashes:
`453ede1ff23e0d3e402ae73905e9347fb901f7cc405b02ef3afef50342149061` /
`0ed3e9e427782d2fb5db3c9f73877ef4c40f34d526a25c4ad9cf813b6451bff6`.

Tests compare over 40,000 corner queries, bounds, live-box destruction and new
wall visibility, fallback callback order, and 600 live frames of checkpoints,
trajectories, destruction counts and RNG against point queries. Bat tests and
the existing 18,000-frame ownership replay also pass. Two stale scalar-wall
fixtures now explicitly allocate their oracle backing. Logs:
`tmp/corner-query-host.log`, `tmp/corner-query-final-host.log`.
Normal stripped startup still leaves 126,840 Chip bytes free, with no excluded
reference allocations; owner size remains 904,628 bytes. Executable size is
1,997,080 bytes (+1,028); normal ELF hash
`1b2ae9fadf7c9ee3ef773017c5b443290f496ad743afd8cb9e508cd1762016de`.
Startup evidence: `tmp/corner-query-memory.log`.

## Combat simulation attribution at `f3f5d753`

A bounded 20-field contact capture at marker 1304 reproduces frame 1305 at
63.26 ms (449,460 cycles). The five complete frames 1305–1309 measure
63.3, 64.8, 61.9, 58.4 and 59.8 ms. Coverage ends at tick 1309, view Y 2122,
six enemy slots and health 3; configuration is 512 KiB Chip / 8 MiB Fast.
This is attribution, not an optimization.

| Inclusive owner, frame 1305 | ms |
| --- | ---: |
| Gameplay simulation | 45.44 |
| Enemy Step (inside simulation) | 18.46 |
| Walking movement (inside enemy Step) | 7.31 |
| Walking tile queries (inside walking movement) | 4.10 |
| Bat behavior (inside enemy Step) | 4.40 |
| Bat solid collision resolution (inside bat behavior) | 2.14 |
| Bullet Step (inside simulation) | 4.86 |
| Room animation (inside simulation) | 3.34 |
| Alarm dispatch boundary (inside simulation) | 3.16 |
| Draw collection (outside simulation) | 4.33 |
| Ammo redraw (outside simulation) | 3.72 |
| Hardware sprite assignment (outside simulation) | 3.45 |

Parent and child rows must not be added. Movement repeatedly samples solid
points through the platform callback, including terrain and breakables. A
whole native occupancy-query boundary is a candidate for removing duplicated
point work; changes must retain collision decisions, source movement order and
the portable callback oracle. These numbers do not justify individual setter,
division-helper or alarm variants. `buildHardwareSpritePart` costs 2.45 ms
inside sprite assignment but its native branch copies offline DMA words and
updates headers; runtime pixel conversion is excluded on Amiga.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512
VERIFY_TIMEOUT_SECONDS=180 STRIP=1 TRACE_FRAMES=1304 TRACE_FIELDS=20
TRACE_OUTPUT=$PWD/tmp/trace/combat-owner-audit bash amiga/profile.sh trace`,
then `python3 tools/amiga-trace-report.py tmp/trace/combat-owner-audit
--frames=1305 --top=100 --tree-min=0.5 --causes`.
Report: `tmp/combat-owner-1305.txt`; driver: `tmp/combat-owner-audit.log`.
Trace ELF SHA-256:
`8df3f28913c1b5ba2c0513b75a81ec411e2fc20f2067d0a3561075675f46dc36`.
The profiling script restored normal `STRIP=1` and passed the native audits.

## Scene-admitted span publication

The external collision backend no longer maintains generic wall registration:
no admitted consumer reads it. Native lifetime, chronology, activation and
room/layer ownership remain unchanged. Object 93 retains its existing indexed
End Step registration; full-service scenes retain every registration. Generic
whole-scene dispatch and terrain population consumers fail closed rather than
silently observing a partial registry. No replacement registry is allocated.

Callback-free spans now publish birth-clock and retirement-window notices once,
rather than once per cell. The scalar/full-service path remains the oracle;
chronology, saturated revisions and watched-range invalidations are identical.
Platform registrations still run in source order. No additional storage is used.
The preceding audit measured the replaced notices at 13.76 ms in frame 676;
the estimate was a 10–12 ms saving. Native construction measures 83.81 ms
instead of 98.97 ms, and span construction 31.71 instead of 46.79 ms.

Matched contact timings against `7b3a56b7`, 512 KiB Chip / 8 MiB Fast:

| Frame / work | Before ms | After ms |
| --- | ---: | ---: |
| 51, early gameplay | 26.4 | 26.4 |
| 676, generation | 260.6 | 245.6 |
| 1047, thin-platform retirement | 118.0 | 118.0 |
| 1305, combat | 63.3 | 63.1 |
| 1512, generation | 146.2 | 142.2 |
| 1515, gameplay | 72.9 | 72.6 |
| 1535, thin-platform retirement | 82.5 | 82.2 |
| 1689, generation | 245.2 | 227.9 |

The complete 1,800-update tape takes 5,074 PAL fields, **17.74 FPS**
(previously 5,079 / 17.72 FPS). This reduces generation stalls, not the ordinary
frame-budget shortfall. The maximum gap is 13 fields at frame 676; it is not
the acceptance gate. Ten host tapes / 18,000 frames compare native terrain
lifetimes and pre-cleanup retirement marks with full-service ownership.
Cross-target replay passes. Targeted activation/re-entry tests retain inactive
platforms and retire them on reactivation; undeclared consumers are rejected.
All seven matched window means and maxima are unchanged or lower. Scalar/span
notification tests cover both watched ranges, empty ranges, invalid requests,
order overflow, revision saturation, unsupported coordinates and stale proofs.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512
VERIFY_TIMEOUT_SECONDS=180 STRIP=1 TRACE_FRAMES=50,675,1046,1304,1511,1534,1688
TRACE_FIELDS=35 TRACE_OUTPUT=$PWD/tmp/trace/span-notices-contact
bash amiga/profile.sh trace`, then `python3 tools/amiga-trace-report.py
tmp/trace/span-notices-contact --causes`. Run the same settings with
`bash amiga/profile.sh fps` for the uninterrupted tape.
Reports: `tmp/span-notices-report.txt`, `tmp/span-notices-676.txt`,
`tmp/span-notices-fps.log`, `tmp/span-notices-replay.log`,
`tmp/span-notices-host.log`, `tmp/span-notices-boundaries.log`.
Matched baseline: `tmp/wall-admission-report.txt`.
Trace ELF SHA-256:
`26173e5272b14edf4b443bf508cf0fd16dd52cbcde15418de5476bd71db57ce8`.

No-input generation frame 163 improves **172.8 → 169.5 ms**; reproduce with
`PROFILE_IDLE=1 SKIP_TO_GAME=1 PROFILE_CONTACT=0 PROFILE_FRAMES=600
VERIFY_CHIP_KB=512 VERIFY_TIMEOUT_SECONDS=120 STRIP=1 TRACE_FRAMES=162
TRACE_FIELDS=35 TRACE_OUTPUT=$PWD/tmp/trace/span-notices-idle
bash amiga/profile.sh trace`. Report: `tmp/span-notices-idle-report.txt`;
ELF SHA-256 `c9d82b4b9846d473a334858aee22abce3699bbcd4c4695a2c380c8293e5b82ce`.
Matched baseline: `tmp/wall-admission-idle-report.txt`.
Normal `STRIP=1` startup passes `verify_memory.gdb` with 126,840 bytes
Chip remaining, no gameplay preparation or excluded reference allocations.
The restored normal executable is 1,996,052 bytes (+108); its ELF SHA-256 is
`6c47f6cf20f79c2d5f29747ccb91902bde0c636743fc46654a750793fd61696a`.
The owner remains 904,628 bytes; no storage is added by span notifications.
Startup evidence: `tmp/span-notices-memory.log` and
`tmp/span-notices-memory-gdb.log`.

## Remaining prepared-publication boundary at `55674919`

A fresh 20-PAL-field capture beginning at contact marker 675 reproduces
frame 676 at **260.29 ms** (1,849,420 cycles). Coverage reaches tick 676,
view Y 1493, five enemy slots and health 3, with 512 KiB Chip and 8 MiB Fast.
This is attribution, not an optimization or restored frame-budget claim.

| Inclusive boundary in frame 676 | ms | Relationship |
| --- | ---: | --- |
| Entire gameplay Step | 230.93 | Contains generation and ordinary simulation |
| Live generation | 185.47 | Parent of the following generation work |
| Prepared terrain publication, 33 calls | 116.27 | Includes native ownership and backend occupancy |
| Native prepared construction | 98.97 | Inside prepared publication |
| Prepared cell lifetime insertion | 75.92 | Inside native construction |
| Collision-owner span construction | 46.79 | Inside lifetime insertion |
| External birth chronology | 9.45 | Inside span construction |
| Layer span publication / admission | 7.49 / 5.62 | Inside span construction |
| Retirement-window birth notices | 4.31 | Inside span construction |
| Fresh external-slot admission | 3.70 | Inside span construction |
| Identity/run insertion | 8.15 | Inside lifetime insertion, outside span construction |
| End-of-span room publication | 8.14 | Inside native construction, outside lifetime insertion |
| Event cells, 34 calls | 46.95 | Separate from prepared publication |
| Breakable births, 17 calls | 14.25 | Inside event cells |
| Builder row advancement | 11.48 | Separate from prepared publication and event cells |
| Builder position publication | 10.90 | Inside row advancement |
| Draw collection | 7.77 | Outside gameplay Step |

Do not sum parent and child rows. These costs do not justify another
registration encoding, individual setter rewrite, or sub-millisecond Draw
helper experiment. The accepted all-window report attributes 3.87 ms
to Draw collection, including actual command production. A replacement of prepared publication must cover
the complete lifetime/layer/room transaction, preserve event-Create barriers
and successful failure prefixes, and remove work rather than defer a second
per-cell reconstruction. RNG consumption remains interleaved with event Creates.

The source boundary is `AmigaGameplayGeneration::createPrepared` →
`NativeTerrainLifetimes::constructPrepared/publishRun` →
`GameplayCollisionOwner::constructTerrainSpan`, followed by
`flushTerrainRoomSpan`. Lifetime allocation admits contiguous free slots;
the collision owner independently admits fresh table/room slots and layer
capacity; the native run is then installed and room membership flushed.
The wall-registration removal did not remove those other owners. A whole
transaction replacement must preserve both completed prefixes on failure
and visibility before the next event Create. Deferring all publication until
the end of a pattern or frame does not meet that contract. Builder position
publication is separate source-visible state, not part of this saving estimate.

Reproduce with `SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_CHIP_KB=512
VERIFY_TIMEOUT_SECONDS=180 STRIP=1 TRACE_FRAMES=675 TRACE_FIELDS=20
TRACE_OUTPUT=$PWD/tmp/trace/admitted-generation-audit bash amiga/profile.sh trace`,
then `python3 tools/amiga-trace-report.py tmp/trace/admitted-generation-audit
--frames=676 --top=140 --tree-min=0.5 --causes`.
Report: `tmp/admitted-generation-676.txt`; driver: `tmp/admitted-generation-audit.log`.
Trace ELF SHA-256:
`08c4000468ac04dcb0800a96ed054926551a5d674c1aa1d7190ae949beebe60e`.

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

### Historical natural raster fallback baseline

The figures below belong to the historical seed-8/route-4 trajectory. After
the source Step-order correction, that route no longer supplies this coverage.
The current source-checked workload is seed 40, route 21, documented in
[profiling](amiga-profiling.md). Its frame numbers and timings must not be
compared directly with this table or used to imply a rendering speedup.

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

Reproducing this table requires revision `e5c6ed59` and its overflow trace
command, then `amiga-trace-report.py --frames=1303-1312` and `--frames=1397-1403`.
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
| Initial-fall §0 D, short display-row CPU publication | At the existing sync point, drain earlier blits and copy initialized spans of at most 16 rows with the CPU; retain blits for larger spans | Against the unchanged-bank composition candidate: 122–135 mean/worst **18.5/33.8 → 19.2/34.5 ms**; 137–162 **20.0/23.2 → 20.5/23.4**, over-budget count **10 → 15**, excess **11 → 20 ms**. Ordinary sync rises from **1.4 to about 2.1 ms**. Reference/candidate ELF SHA-256 `06538fcfaa975c049a330a6270759aa3c16e91b478117d8841c9760ff38529cf` / `79add51ab3874d2e04e0919d56a8b71b54cea52208b01a2c29ef830cd3a890cf`; reports `tmp/c-compose-120-report.txt`, `tmp/d-short-sync-120-report.txt`. | Removing queued setup does not offset CPU copying and serialization for these 176-pixel spans. Both host paths pass 10,000 wrap/damage/pipeline cases, but affected windows regress. Reverted; no strip-size or copy-loop sweep. | This row's introducing commit; experiment reverted |
| Initial-fall §0 C, deadline-only paint-ahead | Move optional Chip transfers after required rendering, reduce transfers to eight rows, and admit both transfer and plan units only below 17 ms of measured work | Against the direct-row-reference candidate, updates 137–162 mean **20.1 → 20.2 ms**, worst **24.6 → 25.6**, excess **16 → 32 ms**, nine over-budget updates in both. Update 145 now performs **8.3 ms** of required row painting. ELF `d458c4541fc22c971589383c9fab40109b1606a9ef5f255cfd8d4ebb68a592b0`; report `tmp/c-budgeted-120-report.txt`. | The stricter deadline leaves cold rows until exposure: it defers work rather than removing it. Scene/raster/pixel tests pass, but the affected window regresses. Reverted; do not sweep deadline or strip-size variants without first reducing preparation/transfer work. | This row's introducing commit; scheduling experiment reverted |
| Initial-fall §0 D, camera projection | Retain rounded camera coordinates keyed by the exact position-plus-shake input | Against the counted-frame candidate: update 98 **14.3 → 14.2 ms**; updates 98–121 mean **17.0 → 16.9**, worst **20.3 → 20.2**, one update over 20 ms in both. Reference/candidate ELF SHA-256: `48e1bcac4daab89c2159058ef186fd27858e8e7037ebb4f6b9a857a4b8b5921c` / `f6aff38aaa22800b9afe0011015d5397e1b2a4d42e50d6dbdffcadad3c84b830`. | Within 0.5-ms noise; does not justify retained state. Host runtime output matches the reference; mutation, copy, shake, reset and sanitizer tests pass. No cache/helper variants. | This row's introducing commit; [`retained-camera-projection.patch`](rejected/retained-camera-projection.patch) |
| Initial-fall §0 A, unified segment records | One record owns identity, common flags/sprite/wall state, room state and retained layer state; mutations split records. Actor-only room nodes, thin registration derived from segment kind bits, range admission instead of per-cell admission. | Three 100-field captures cover 2–164 without gaps. Candidate window mean/worst/over-20/excess: entrance **16.2/20.2/1/0**, quiet **20.3/35.8/32/119**, burst **22.3/51.8/8/51**, enemies **32.5/92.7/27/338**, landing **94.7/136.6/2/149** (ms where applicable). Whole fall **22.0/136.6/70/658**, versus baseline **21.8/136.0/70/626**. Update 99 is **24.5 ms**; 122 **51.8**, 136 **92.7**, 163 **136.6**. Update 56 independently repeats at **254,320 cycles / 35.79 ms**, versus baseline 27.7 ms. `appendPreparedCells` alone still costs **17.39 ms for eight segments at 122 / 15,443 cycles per segment**, before builder-position publication (2.05 ms) and invalidation. | Consolidating state does not remove the generic publication work that creates that state. It misses the 2,500-cycle/segment and 4-ms/burst limits, the quiet-row/excess limits and both burst gates; the scalar fallback regresses by 8.1 ms. Rejected on work, not field-count redistribution or the small landing variation. The cadence run gives **198 fields / 164 updates**, worst gap seven at update 1; the landing witness passes (tick 164, y 262,144, health 4, ammo 8, three enemies, 1,371 RNG draws). The 18,000-frame replay and sanitizer fixtures for holes, reuse, mutation, failed prefixes and pending-birth room publication pass; these are not performance wins. Removed as a whole, not a basis for descriptor-packing, accessor or copy-helper tuning. Trace ELF SHA-256s for starts 1/60/120: `8f4fe073aca5b984aac2a008238b74ae772ed3c3ffa9eea67ac3299a090de362`, `76386a7725d71149dbea4398338d8b8890c3d83c12a22fe3d1cd033b6c20d39c`, `9d1ca7076e97be9be1697e811147c4a7794c449b298e493b7df7711c8343227b`; short recheck `d0d02dd6b62ff00c4ab3d99147a207da111e12f3fb44078542c3a887dc99d212`. | This row's introducing commit, [`terrain-segment-owner.patch`](rejected/terrain-segment-owner.patch) |
| Initial-fall multi-row identity owner, global lookup | Host-generated two-byte column/row geometry; one native descriptor and owner transaction per event-free group | 122 54.923 → 51.106 ms (363,110 cycles, repeated); 136 94.5 → 96.950 ms; 163 135.9 → 141.214 ms; fall 191 → 192 fields | Per-row identity construction is actually removed, but repeated global group lookup costs 5.28/6.29 ms at 136/163. Adds 21,540 ordinary asset bytes. Superseded by the source-ordered traversal attempt below, not shipped. ELF `504de9758532b988de36826cf3487d9dc967609ac6e9be54678e2ca0bd318cd0`; reports `tmp/trace/pattern-owner-candidate-{122,136,stall}/report.txt` | This row's introducing commit; final experiment in [`terrain-pattern-owner.patch`](rejected/terrain-pattern-owner.patch) |
| Initial-fall multi-row identity owner, source-ordered traversal | One forward group cursor per selected pattern, eliminating repeated global searches; same authoritative multi-row constructor | 122 54.923 → 50.476 ms; 136 94.5 → 93.650 ms; 163 independently repeats at 965,350 → 973,600 cycles (135.867 → 137.028 ms). Quiet 57–121 20.571/26.879 ms mean/worst; full fall remains 191 fields | The first-spike saving does not restore fall cadence, and the worst frame is 1.161 ms slower. The 21,540-byte ordinary pack increase and normal executable growth 1,607,312 → 1,617,300 bytes are not justified by this outcome. 18,000 replay frames, 165 native state samples, real external-owner comparisons, 1,251 geometry comparisons, source failure/RNG tests and sanitizer gates agree. Both normal builds pass 512 KiB Chip startup. Removed as a whole; no group-layout, cursor, admission-loop or row-decoding variants. ELF `5c75758bb7c2d5a7a4b78cd6a87e77d341a7de82cc6926ac721eb1a4c6a5f87c`; reports `tmp/trace/pattern-owner-cursor-candidate-{122,136,stall}/report.txt`, independent `tmp/trace/pattern-owner-163-{control,candidate}/report.txt`, quiet `tmp/trace/quiet-window.RNCTzQ/quiet.txt`; logs `tmp/pattern-owner-cursor-{cadence,landing}.log` | This row's introducing commit, [`terrain-pattern-owner.patch`](rejected/terrain-pattern-owner.patch) |
| Initial-fall sparse room owner | Run-owned room keys; actor-only animation links; only selected activation/destruction entries ordered; fresh native reservation replaces per-cell second-owner admission | Frame 122 54.923 → 53.913 ms (383,060 cycles, independent repeat); 136 94.5 → 93.910 ms (667,240 cycles); 163 135.9 → 135.8 ms; complete fall 191 → 192 fields (noise). Quiet 57–121 mean/worst 20.566/26.499 ms | Avoids the shared-descriptor fragmentation failure, but still misses a substantial construction saving: at 136 native construction 31.83 → 30.96 ms; room bind 3.33 → 1.40 ms is partly offset by the larger publication and state transfers. 803 sparse lifecycle comparisons and native reservation-prefix/layer-split/clear tests pass under sanitizers; all 165 native fall samples match. Removed as a whole; no room-state accessor, metadata packing or sparse-list variants. ELF `dd2aa3bca4fe8f3cbc12c6b16e7c59ca04480124c1f925aef00a2ac2cd4d5057`; reports `tmp/trace/quiet-sparse-room-{122,136,stall}/report.txt`, `tmp/trace/quiet-window.Y0QPaK/quiet.txt`; logs `tmp/quiet-sparse-room-{cadence,landing}.log` | This row's introducing commit, [`terrain-sparse-room.patch`](rejected/terrain-sparse-room.patch) |
| Initial-fall shared terrain owner | Room links and marks embedded in lifetime/layer descriptors, sharing the slot locator; independent actor nodes and intrusive split/release repair | Frame 122 54.923 → 56.331 ms (400,240 cycles, independent eight-field repeat); 136 94.5 → 96.403 ms; full fall 191 → 284 fields. Complete 57–121 mean stays 20.6 ms, but 161/162 rise from 33.0/25.5 to 98.5/91.0 ms. All 165 native state samples match; mixed-state sanitizer differential passes | Room insertion falls 2.38 → 1.82 ms, but native construction rises 21.73 → 22.87 ms. Lifetime/layer fragmentation now propagates to room skip traversal: animation cursor costs 64.85 ms/update in 161–162. The 20-field landing trace ends inside frame 163: no complete 163 time is claimed. Removed as a whole, not a basis for accessor or skip-projection tuning. ELF `ebe86055698b1af51b7b3e2a96f10648a572a3d474046268fa6dc996dc1fac26`; reports `tmp/trace/quiet-shared-room-{122,136}/report.txt`, `tmp/trace/quiet-shared-room-stall/complete-prefix.txt`, `tmp/trace/quiet-window.sjAMTt/quiet.txt`; cadence/state logs `tmp/quiet-shared-room-{cadence,landing}.log` | This row's introducing commit, [`terrain-shared-room.patch`](rejected/terrain-shared-room.patch) |
| Initial-fall terrain state | Run-owned initial flags/sprite/wall state, materialized only on cell mutation | Frame 122 54.92 → 54.29 ms (390,230 → 385,760 cycles, independently repeated); native construction 21.73 → 20.85 ms; complete quiet 57–121 unchanged at 20.6 ms | Removes initialization but not ownership publication; misses the estimated 3–4 ms ceiling and adds run defaults, mutation bits and read branches. Scalar/eager-reference mutation, cache, layer-split and reuse tests pass; all 165 native fall samples match. Removed as a whole; no lazy-state/helper variants. Candidate ELF `e58f6bd0a6dd6d6d4a2f8750df41ab175b33f6457cf8852ea2bb732fac47dee4`; reports `tmp/trace/quiet-run-defaults-{control,candidate}/report.txt` and `tmp/trace/quiet-window.5osbVl/` | This row's introducing commit, [`terrain-run-defaults.patch`](rejected/terrain-run-defaults.patch) |
| Initial-fall terrain construction | Cross-row ownership transactions between source event cells, with proved borrowed builder-position publication | Fall 191 → 194 fields; 122 54.9 → 57.1 ms; 136 94.5 → 107.0 ms (independent repeat 760,400 cycles); 163 135.9 → 150.7 ms; quiet 57–121 stays 20.6 ms | Grouping does not remove per-row identity/state construction. At 136, original construction 31.75 ms becomes 21.57 ms residual per-row plus 10.51 ms grouped construction; planning, admission and scratch initialization add cost. Misses the estimated 3–6 ms saving at 122. Full builder/failure, native mutation and 18,000-frame replay tests pass; all 165 native state samples match. Reverted as a whole, not a basis for scratch/loop variants. ELF `d51d7721234b9ca271878d3c018b29b51078bbb6620a2cb926e9f6374b7cd632`; reports `tmp/quiet-terrain-group-{quiet,122,136,stall,recheck}.txt` | This row's introducing commit, [`terrain-cross-row-group.patch`](rejected/terrain-cross-row-group.patch) |
| Quiet-fall terrain publication | Cache immutable birth recipe and borrow existing layer counters/clock, retaining per-instance logical state | Fall 191 → 189 fields; marks 91–110 essentially unchanged, 2,805,770 → 2,808,060 cycles; frame 136 94.5 → 91.9 ms; frame 163 135.9 → 138.2 ms | Removes about 0.5 ms per small border update, not the estimated 8–10 ms in frame 136. Repeated ownership publication remains; reduced idle RNG production shifts work into the landing burst (independent recapture: 982,264 cycles). No useful middle-window gain; reverted rather than tuning the borrowed helper. All 165 state samples match and 18,000-frame ownership tests pass. Reports `tmp/quiet-terrain-publication-{middle,136,stall,recheck}.txt`; frozen ELF `tmp/quiet-terrain-publication/Downwell.elf` | This row's introducing commit, [`terrain-publication-borrow.patch`](rejected/terrain-publication-borrow.patch) |
| Quiet-fall RNG preparation | Remove mandatory four-word Step refill; rely on bounded idle service and exact demand fallback | Marks 91–110: 3,078,280 → 2,967,540 cycles; fall unchanged at 195 fields; frame 163 **136.1 → 162.4 ms** | Idle capacity does not replace amortized production. Independent frame-163 recapture confirms 1,153,980 cycles and 25.19 ms in 94 demand-refill calls. Reverted; do not sweep producer budgets. Host full-state/output tests pass, including cache exhaustion with zero frame budgets. Control ELF `53ab87bc`, candidate `4cd35476`; deterministic 512 KiB Chip; reports `tmp/quiet-idle-producer-*` | This row's introducing commit, [`quiet-idle-producer.patch`](rejected/quiet-idle-producer.patch) |
| Quiet-fall room publication | Coalesce consecutive active terrain room spans; realize links before order/activation/cleanup observers | Updates 91–110: 3,130,780 → 3,130,530 cycles; 121–138: 3,543,980 → 3,545,080; fall 199 → 198 fields (noise) | State-only probe proves 56 deferred memberships at update 120, but publication bookkeeping saves no meaningful work. Added membership checks/realization offset insertion savings. Room differentials and 18,000-frame ownership replay pass; do not retry suffix-coalescing variants. Control ELF `84ed8c0c`, candidate `3d0d3a53`; deterministic 512 KiB Chip; reports `tmp/quiet-room-{control-middle,middle,control-flush,flush}.txt`; raw traces removed | This row's introducing commit, [`quiet-room-coalescing.patch`](rejected/quiet-room-coalescing.patch) |
| Fresh terrain state | Prepare default flags, sprite and wall state before room births; skip matching initialization on untouched slots, retaining platform-specific flag writes | Frame 676 226.85 → 224.04 ms (1,611,770 → 1,591,820 cycles); constructor self 10.83 → 8.94 ms; scalar/reuse/failure-prefix and 18,000-frame ownership checks pass | Only 2.81 ms removed versus the 3 ms floor and roughly 6 ms estimated ceiling. Kind-dependent flags and freshness bookkeeping retain much of the work. Removed before broader timing/startup-cost validation; no initialization/helper variants | This row's introducing commit; [`fresh-terrain-state.patch`](rejected/fresh-terrain-state.patch); `tmp/terrain-preinit-mixed-report.txt`, ELF `c0a9f079d74dfd863dbe7785d4f87cb39f50dc3a9823b87589e6b561ad1a340a` |
| Pattern admission | Host-validated local pack plus compiled size/CRC32, replacing the runtime structural validator | Pattern lease excluding IO 1426.183 → 1344.975 ms; complete cold handoff plus first gameplay 9357.425 → 9277.429 ms | The integrity scan retains almost all admission cost; about 0.9% combined-transition reduction does not justify weaker corruption-only admission and extra machinery. Removed; no CRC/table/helper variants | This row's introducing commit; [`certified-pattern-admission.patch`](rejected/certified-pattern-admission.patch) |
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

## Prepared terrain: scene-entry versus live publication

The existing preparation boundary is important when pricing another initial-fall
change. These are source contracts, not additional measured savings:

| Input or effect | Existing owner | May be moved to scene entry? |
| --- | --- | --- |
| Cell kinds/columns, RNG draw counts, occupancy words | `AreaOneBuilder::PreparedTerrain` / builder pack | Already prepared offline. |
| Area-specific wall/platform Create effects | `PreparedTerrainCreate::prepare` | Already prepared; ordinary plans require zero RNG and zero image speed. |
| Generic spatial publication branch | `GameplayCollisionOwner::collisionConsumersAdmitted_` | Already compile-time false in `DOWNWELL_EXTERNAL_COLLISION_BACKEND`; it is not a runtime generic collision service to remove. |
| RNG advancement and special-cell counter | `PreparedTerrain::beginPublication` / `completePublication` | No: these bracket the source segment's live publication and its failure boundary. |
| Slot/generation, descriptor and room position | `NativeTerrainLifetimes` / `TerrainOwnershipRuns` / room owner | No unconditional reservation: retirement, reuse and interleaved actor births affect them. A hidden descriptor reservation also excludes structural mutations until commit. |
| Global birth clock, layer population and order | `NativeInstanceTable::externalBirthSpan` / `GameplayRoomLayers::publishPreparedTerrainSpan` | Must be published at the live boundary; interleaved births and layer retirement consume/change this state. |

Consequently, preparing the current constructor's fixed inputs earlier is not a
new architectural saving. The 21.73 ms update-122 construction measurement is
inclusive: its 12.56 ms `constructTerrainSpan` region already includes room
binding (2.38 ms), layer publication (2.08 ms) and layer admission (1.83 ms).
Do not add those children to the parent or price removal of the compiled-out
generic spatial branch. A replacement must reduce live ownership/publication
work while preserving the listed boundaries; moving it into ordinary falling
updates is not elimination.

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
