# Amiga gameplay frame-cost audit

This audit records where native A500 gameplay time went at revision
`b89ff875`, and why. The work it implies is queued in
[section 5 of the work queue](open-work.md#5-meet-the-amiga-frame-budget).
Do not update this file as that work progresses. Record later measurements in
the commits that make them, and repeat the audit with the same method when the
representation changes. The figures are emulated-machine measurements of one
deterministic tape, not guarantees for other scenes.

## Configuration and reproduction

- FS-UAE A500: cycle-exact 68000, 512 KiB Chip, 8 MiB Fast, PAL, Kickstart
  3.1; `-O3` with LTO. The working tree also contained an uncommitted
  startup-only resident Chip-pack copy, which does not run per frame.
- Tape: `SKIP_TO_GAME=1 PROFILE_CONTACT=1`, 1800 gameplay updates. It covers
  natural descent with one damaging contact, view Y 39 to 1874, at most five
  allocated enemy records and final health 3.
- FPS-only build `c4e86313…`: 5837 PAL fields, 15.42 FPS, 64.9 ms per frame
  on average. Frame 701 is worst, at 47 fields.
- PROBES build `45539519…` (instrumented, 7985 fields): counters below. Its
  worst frame is also 701; zero clock clamps.
- [Instruction traces](amiga-profiling.md#cycle-exact-instruction-traces):
  build `37f2091f…` captured 100 fields from each of frames 50, 250, 450, 690,
  900, 1100, 1300, 1500 and 1700. That is 306 complete frames. Build
  `43cc995e…` captured frame 701. The trace builds differ only in their
  compiled frame list.

```sh
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=180 bash amiga/profile.sh fps
SKIP_TO_GAME=1 PROFILE_CONTACT=1 VERIFY_TIMEOUT_SECONDS=900 bash amiga/profile.sh probes
SKIP_TO_GAME=1 PROFILE_CONTACT=1 TRACE_FIELDS=100 VERIFY_TIMEOUT_SECONDS=1500 \
  TRACE_FRAMES=50,250,450,690,900,1100,1300,1500,1700 bash amiga/profile.sh trace
python3 tools/amiga-trace-report.py tmp/trace/run --frames=1300-1338 --causes
```

## Frame cost by scene

Traced frames include the spin to the next vertical blank. One PAL field is
20 ms (142,102 cycles).

| Tape frames | Scene | Mean ms | Median | Worst | Over 20 ms |
| --- | --- | ---: | ---: | ---: | ---: |
| 50–91 | entrance bowl, firing | 47.3 | 42.4 | 72.8 | 42/42 |
| 250–303 | open descent | 36.7 | 30.1 | 354.8 (278) | 37/54 |
| 450–470 | enemies in view | 93.0 | 72.5 | 155.5 | 21/21 |
| 690–700 | combat with gems and smoke | 149.3 | 146.7 | 176.7 | 11/11 |
| 701 | pattern generation burst | 928.4 | | | 1/1 |
| 900–920 | time fields present | 93.1 | 93.0 | 127.2 | 21/21 |
| 1100–1737 | camera at the level bottom | 50.3 | 43.9 | 76.0 | 157/157 |

Frame 278 is a smaller generation burst (332 ms in `GameplayWorld::step`).

## Cost by owner

`--causes` charges each sample to the innermost recognised owner on its
logical call stack. Multiply/divide and `mem*` helpers are charged to their
callers. Runner bookkeeping means the emulated GameMaker runner state:
lifetime registries, room active/inactive lists, layer element lists,
object-membership lists, spatial dirty queues, and bounds caches, together
with their publication and validation. Milliseconds per frame:

| Owner | Quiet 1300–1338 | Descent 250–300 | Combat 690–699 | Fields 900–920 | Frame 701 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Runner bookkeeping | 14.1 | 11.0 | 21.6 | 16.1 | 561.8 |
| R-tree upkeep | 1.4 | 3.6 | 2.2 | 6.1 | 106.2 |
| Generation logic (builder, RNG) | 0.5 | 2.1 | 1.4 | 0.4 | 143.5 |
| Game logic | 12.6 | 5.9 | 24.9 | 13.7 | 28.8 |
| Software float emulation | 2.3 | 1.4 | 11.0 | 2.4 | 15.0 |
| Effect surface (currency/smoke) | 0.2 | 0.2 | 35.3 | 0.2 | 34.4 |
| Actors, sprites and display | 2.9 | 4.5 | 31.1 | 4.6 | 28.0 |
| Time-field outlines | 0.1 | 0.1 | 0.1 | 33.0 | 0.2 |
| Ammo HUD | 4.8 | 0.1 | 4.4 | 7.5 | 0.1 |
| Terrain rows and scrolling | 0.2 | 1.7 | 6.9 | 1.4 | 7.2 |
| Frame loop and other | 2.7 | 2.7 | 3.3 | 2.8 | 3.2 |
| Wait for vertical blank | 7.9 | 3.3 | 7.4 | 4.8 | 0.0 |
| Total | 49.6 | 36.4 | 149.6 | 93.1 | 928.4 |

## Findings

### Runner state is maintained whether or not anything reads it

The PROBES tape made no spatial-tree reads: box, line, height and node-count
counts are all zero across 1800 updates. It made 505 first publications and
no re-publications. The journal queued 624 operations and serviced 861, the
latter including work queued before the window, with no pressure drains or
eager fallbacks. The tree still costs 1.4–6.1 ms per frame in the windows
above and 106 ms in frame 701.

Terrain collision is answered by the native terrain grid: `solid_indexed`
21,600 and `landing_indexed` 1,800, with every fallback at zero. The retained
painter draws terrain, and the room-actor Draw visitor skips terrain elements.
Animation snapshots copied terrain 240 times in 1800 updates, against 26,174
actor copies. Yet every generated block still registers a lifetime, room-list
node, layer element, object membership, dirty-queue entry, bounds cache entry
and journal record.

Frame 701 has 145 births. Its 138 terrain appends cost 27,724 cycles each
(538 ms). Per birth, object-membership attachment costs 6,578 cycles, layer
birth 6,277, journal insertion 2,777 and room binding 2,600. Terrain Create
properties cost 2,526 per block. `generateLiveTerrain` takes 800.7 ms of the
928.4 ms frame, and runner bookkeeping plus the R-tree take 72% of the frame.

### Traversals pay per-element indirection, not per-element work

In the quiet window the room animation walk costs about 5.1 ms per frame.
Enemy and bullet animation and image publication account for only 0.9 ms of
that. The rest goes to handle decoding, family `contains` resolution,
previous-position capture and list traversal: about 2,000 cycles for each of
roughly 15 visited actors. Actor Draw collection costs 36,000 cycles
(5.1 ms), about 4 ms of it in the layer walk; in combat it reaches 10 ms.
Captured alarm dispatch costs 15,600 cycles (2.2 ms) for at most five
enemies. Activation reconciliation adds about 0.7 ms. Position, image and
transform publication through the lifecycle bindings adds about 2 ms.
`GameplayCollisionOwner::valid()` costs 582 cycles and runs six times per
frame. `MotionBinary64::fromInteger` costs 553 cycles and
`RunnerImageValue::assignInteger` 503.

### Rendering composes on the CPU

In the combat window the currency/smoke effect surface costs 35.3 ms per
frame. Composing its 160×284 three-plane CPU surface (clear, stamp, subtract
and visibility) takes 209,337 cycles, and publishing it to bitplanes a further
56,603. `AmigaGameplayActors::draw` takes 143,753 cycles: 15 CPU asset draws
at 8,451 cycles each. With time fields present, outline preparation costs
195,588 cycles per frame, or 33 ms including drawing. An ammo HUD redraw costs
77,859 cycles (11 ms) in quiet frames and 133,395 (19 ms) in the field window.
While firing, about 40% of frames redraw it, up to 30 ms in one frame. During
descent the CPU paints newly exposed terrain rows for about 9–14 ms every third
frame.

### Arithmetic

Multiply/divide and `mem*` helpers take 9.2% of all traced cycles and 13.3%
in combat. Quiet frames execute about 40 32-bit divides and time-field frames
about 160; `__divsi3` costs 571 cycles and `__udivsi3` 407. Software binary64
and binary32 emulation takes 2.3 ms per quiet frame and 11 ms per combat
frame. `GameMakerRandom::nextU32` costs 766 cycles. Frame 701 makes 329
calls (35.5 ms), plus 179 calls to `cavernEnemyChoice` at 1,092 cycles
inclusive. Frame 701's 143.5 ms of generation logic comprises RNG 35.5 ms,
wall-contact appends 16.8, pattern emission 15.2 and per-instance creation
adapters about 25.

### Memory traffic, not call count

Leaf-instruction classes, as a percentage of cycles:

| Class | All frames | Quiet | Combat | Frame 701 |
| --- | ---: | ---: | ---: | ---: |
| Other memory operands | 23.5 | 23.0 | 17.1 | 30.3 |
| Stack-frame operands | 8.9 | 7.0 | 18.2 | 15.8 |
| Pushes | 3.9 | 4.0 | 3.8 | 5.3 |
| `movem` | 3.7 | 4.0 | 2.9 | 3.6 |
| Calls and returns | 2.0 | 2.1 | 1.8 | 2.6 |
| Helpers | 9.2 | 7.9 | 13.3 | 5.0 |
| Branches | 12.6 | 13.0 | 12.2 | 10.6 |
| Register-only | 24.4 | 23.0 | 25.7 | 26.7 |
| Vertical-blank wait | 11.6 | 15.9 | 4.9 | 0.0 |

Data-memory instructions account for 40% of all cycles and 55% in frame 701.
Calls do not: quiet frames execute about 254 calls (164 direct, 90 indirect)
and combat frames about 880 (596 and 282). LTO inlines nearly every lambda
and template. Calls, returns and register saves cost about 3.0 ms per quiet
frame and 7.0 ms per combat frame. The traffic comes from the data model. A
single operation reaches several parallel tables through handles and revalidates
at each layer, and large inlined generic bodies spill their locals. Lambdas
cost extra only where they are type-erased (`void*` context plus function
pointer) inside per-instance loops.

## Consequences for the budget

The quiet and descent windows spend about 15 ms per frame on bookkeeping and
R-tree upkeep that no query observes. Removing it is necessary but does not
reach 20 ms; game logic, rendering and arithmetic must shrink too. Combat is
dominated by CPU composition, and time-field frames by outline preparation.

The worst frame is a synchronous generation burst. Registration and the tree
take about 650 of its 928 ms. Without them it is about 275 ms: 144 ms of
generation logic plus the frame's combat work. Cheaper rendering and RNG
would bring it to roughly 80 ms, leaving RNG draws and pattern emission as
the floor. Moving those draws to another frame would change RNG interleaving.
These projections are derived from measured component costs; each change
needs its own timing tape.
