# Remaining work

This is the single work queue for the Downwell port. Keep only unfinished work
here; remove items when completed. Completed changes belong in the git log.
Linked technical documents explain source behavior and implementation constraints,
not separate priority queues.

Both SDL and Amiga must behave like the original Windows Steam game. VM assembly,
serialized resources and original-runner evidence are the authority; port-to-port
agreement alone is insufficient. Let gameplay run slower on PAL; do not add
60 Hz catch-up.

**Target machine (set 2026-11-21, replacing the A500):** an Amiga 1200
with 4 MB Fast RAM (68EC020 at 14 MHz, AGA, 2 MB Chip, 4 MB Fast;
`MACHINE=a1200-4m`, the gates' default). The unexpanded A1200 was the
first choice the same day, but the game needs about 3.4 MB. The game is
built for the 68020 (`AMIGA_CPU=68020`, since 2026-11-24; AmigaStart.c stays
68000 code and refuses a 68000 or 68010). The game must start on any Amiga
with a 68020 or later, enough Chip memory for its DMA data and enough public
memory (Chip or Fast) for everything else, and refuse with a clear message
otherwise.

## Priority and scope

The goal, set by the user on 2026-10-23, is a **world 1 vertical slice**: the
first world of Downwell, from boot to the end of level 1-3, fully implemented
and feature complete in the normal Amiga executable. Work it in this order:

1. gameplay and visuals (W1);
2. sound effects (W2, done; see [Amiga sound effects](amiga-audio.md); music
   is deferred to WM);
3. performance (W3).

The shipping Amiga executable holds only what world 1 uses (the
world-one profile and its link gate, `make -C amiga audit`) and fits the
target; keep it as small as possible while W3 makes it as fast as possible.
Already-implemented later-world code stays available to owner builds and the
host tests.
Only after W is complete, continue with what world 2 needs: the skeleton
encounter tasks (section 1), the catacombs (section 2) and the remaining
content (section 4).

Sections 1, 2, 4 and 5 are parked. Sections 3 and 6–8 still apply, but only
where they touch world 1.

## W. World 1 vertical slice on the Amiga

World 1 is area 1: the garden and style select, levels 1-1 to 1-3 with their
shops and side rooms, the clear and upgrade screens, death, results and Retry,
up to the hand-off to area 2. In the original only `builderCatacomb` creates
skeletons, so the skeleton encounter (section 1) is not part of it.

### How to work W

- **Bar:** behaviour matches the original game (VM assembly, serialized
  resources and the source audits in `docs/`), checked by deterministic
  input-only routes and native state or pixel checks. The existing compact
  Amiga implementations (`GameplayWorld`, `ShopWorld`, `RunSession` and
  friends) are acceptable. The registered source-exact owners, SDL parity and
  original-runner evidence are not required to close a W item. Keep the SDL
  build compiling and its tests passing; it remains a development reference.
- **Visible:** an item is done only when a player sees it work in the normal
  Amiga build on a natural route. Record the route (seed, input, build flags)
  and the native check in the commit. Helper-only or test-only commits do not
  close an item. The host route search (`DOWNWELL_VERIFY_WORLD_ONE_SLICE`)
  binds the collision owner and encounter session; the shipping build runs
  without them and must reproduce the same routes (`make
  test-world-one-ownership` compares witnesses, sounds and draw lists on
  all eight slice routes; `COLLISION_OWNER=1` builds the owned Amiga
  reference). `tools/verify-world-one-native.py` replays the tape on the normal build;
  `DOWNWELL_DENSE_CHECK=from,to` checks every tick in a window.
- **One item at a time,** in the listed order unless a dependency forces
  otherwise. Attach any newly discovered prerequisite to an existing item.
- **Performance during W1/W2:** no 20 ms gate yet, but do not add per-frame
  work for features that are not present (no empty scans), and keep normal
  startup and the `quiet-fall` landing witness passing. Normal startup means
  the target machine. Note any
  large new cost in the commit for W3.
- **Restructuring** must be behaviour-neutral and pass the existing gates;
  verifier code goes into its `DownwellAmiga*Verify.inc` fragment, not
  inline in `DownwellAmiga.cpp`.

### W1. Gameplay and visuals

Current state of the normal Amiga build: boot, garden, style select,
area-1 generation and scrolling, movement, stomping and shooting, the health,
ammo and gem HUD, damage numbers, six builder-placed enemy families (snake
111, ground turtle 112, frog 113, bat 116, snail 117, crawler 119) plus the
level spawner 78 with its white jelly 115 and red ball 118 (steering motion,
stomp and bullet deaths, eye art) and, in hard mode's channels, the eyefish
114 and the neon red ball 173, box bursts (smoke,
flash, impact, `breakablesDebris` 370, rewards), `Scattered16` 289, `gemChunk`
293, torches 295/296, the steering enemies' splash, smoke and flash,
`GunModule` 109 (battery and heart pickups, `moduleText` 373 and `itemNotif`
374) with the seven `bStatInitLevel1` guns, their shots, muzzles, laser
trails and casings, side rooms with `pTimeStop` (start, ordinary and goal
fields, `fxTimeChange` 211 room speed, time shards 238, frozen enemies,
currency, casings, debris, moving effects and TorchFire, field kills), the
damage chains (`objDeadBody` 263 corpses, blood, death smoke and flash, bullet
impacts and the immune spark, bullet, casing and laser wall hits, jump,
turn and landing dust, the empty-gun smoke and "EMPTY!", and the
`momentDelay` and player-damage `room_speed` changes), the combo and
controller HUD (the combo counter over the player, `comboRewardText` 383, the
`levelNumber` 372 title at first landing or kill, the streak bar with " N"
and its icon, GEM HIGH with the gem-high afterimages, the counting gem total
and the owned-upgrade icons), shops (stock and prices by
area, style and membership, the prompt sign, the merchant's animation and
thankful, confused and goodbye reactions, funded, too-poor and sold-out
purchases, Leave with the farewell, and the merchant's speech), level exit,
upgrade selection, level transitions, death, results and Retry all work, as
do the pause menu (Escape or P) with its confirm, PALETTE, STATS and OPTIONS
screens, all five styles with their rules, unlocks and the results' unlock
notice, hard mode's toggle, builder, prices and titles, the Timer option and
`save.ini` (style, hard mode, palette, timer, records and lifetime gems).
Each level opens with the objEnterDither (73) entry wipe. After the 1-3 clear the run reaches a deliberate end-of-slice screen at
CATACOMB-1 without loading area 2. Items keep their numbers as they close.
Known approximations: overlapping `moduleText` records never push each other
down (Collision with `moduleText`), and `moduleText`, `itemNotif`, "EMPTY!" and
the `levelNumber` letters assume top-left text alignment although the HUD
leaves `fa_middle` set; "EMPTY!" reuses the `itemNotif` record for
`fxRisingText`, skips its Collision with `fxRisingText` and is born after the
whole player Step. The spread guns' and aimed shots' art uses the nearest
offline quarter turn without the usual gun's per-angle pixel proof.
`levelNumber` is drawn on the fixed HUD layer: above the world and the hardware sprites,
and hidden while the death flash covers the playfield. The merchant's speech
(`fxRisingText` from `scrRisingTextShop`) uses the same alignment as "EMPTY!"
and rises in Q16 like `itemNotif` rather than binary64. The Shop Step's
BGM gain is produced but silent until WM, and its `shopInView` and
`keeperAngry` flags drive only sounds. The criminal route
cannot happen in the pinned build: only a shop case's `destroyed` flag sets
`global.criminal`, and nothing writes it (`original-behavior-audit.md`).
The pause menus drop the PC-only DISPLAY, LANGUAGE, Quick Start and key
binding screens, and the BGM option is stored but silent until WM. PlayMenu's HARD MODE label is static (no whomp). The save is written at
static moments (results opening, pause close and its actions, style confirm,
slice end and quit), not on every `ini_write_real`; the slice end counts as a
run end for the records. The clear screen shows the style-0 poses for every
style, and the first run after boot keeps the builder checkpoint taken before
style select, patched with the chosen style and hard mode. That first world
is the pinned checkpoint whatever the garden route: in the original the
garden's draws and duration shape it, but the Amiga front end draws no game
RNG, and the host reference keeps the pre-W1.14 walk straight into the well
(`useCaptureGarden`) only as the checkpoints' capture path.
Every world-1 upgrade works in play, each on an `upgrade-<ids>-v<variant>`
route (Makefile `UPGRADE_ROUTES`; [upgrade coverage](upgrade-coverage.md)).

15. **Jumping against a wall can be cancelled.** `PlayerMotion` tests a
    horizontal move at the truncated pixel, so pushing into a wall lets x
    creep to .5-.9 of the last free pixel. The upward probe rounds as
    `place_meeting` does, meets the wall, and the jump's `ysp` becomes the
    ceiling response (zero) in the same Step. In the original, `scrWallCol`'s
    rounded test stops x below .5, so the jump goes ahead. W1.14 gives the
    garden the rounded horizontal test (`configureRoundedHorizontal`);
    gameplay keeps truncation, because changing it moves every gameplay
    route. Confirm on the Windows build (jump while holding into a well wall
    after a slow approach), then switch gameplay and search the routes again.
16. **Garden gun.** No room check stops `scrPlayerShootN` in `rmMenu`, and
    the garden's `objGrass` is a `parentShootableThing`, so an airborne
    press that is neither a wall kick nor the hard-landing re-jump fires
    the gun in the original. The port holds such a press back until fire is
    released (`GardenMotion`). Confirm on the Windows build, then bring the
    garden bullets, the gunshot sounds, the recharge on landing and the
    grass reaction into the front end.
17. **Disappearing gems leave white outlines.** A gem about to vanish skips
    its Draw every other update (`dflash`, `objCurrency` Step and Draw). On
    those updates the Amiga shows the gems as white outlines with black
    interiors; `sprGemM`'s four frames are all red-filled and gem high draws
    no outlines. Also seen: partly drawn gems beside `EMPTY!`. Seen with a
    `gemChunk`'s ten gems on the floor of a side room during gem high.
    Check first whether it predates the uncommitted 5 C row-restore queue.
18. **Gameplay display checks fail.** The six emulator pixel checks
    (`verify-amiga-gameplay-frame`, `-active-frame`, `-laser-frame`,
    `-gemhigh-frame`, `-ammo-frame`, `-recharge-frame`) now run on
    `a1200-fast`, like the terrain cache check: their verification builds
    carry about 2.8 MB of reference state the 4 MB target cannot hold. Their
    native raster and display-ring oracles pass. What fails:
    - the comparison with the SDL host's frame (landing 324 differing
      pixels, recharge 548, active 951, ammo 3,603, laser 3,605; the landing
      frame's pinned checksum too). The host reference lacks world-1
      features the Amiga has: the `levelNumber` title, landing dust and
      particles, the landing pose, enemy positions, and the W1.4 shot (the
      laser frame's camera x is 161 on the Amiga, 160 on the host). The
      differences are the same with the polled blitter queue, so they
      predate `d1b5dbda`;
    - gem-high never reaches its capture (a visible gem-high shot after
      the fixture's combo of 8 at landing), within 45 s or 240 s; the gdb
      "target is running" message is the timeout's interrupt.
    Decide what these checks compare against (native frames reviewed and
    pinned, or a host reference brought up to the world-1 features), then
    find why the gem-high fixture no longer reaches gem high.
19. **The compact bullets can run out.** `birthBullet` fails the whole
    gameplay step when all `gameplayCompactBulletCapacity` (32) slots are
    live: the world-one slice at `DOWNWELL_SLICE_VARIANT=154` stops at 1-3
    tick 8671 firing the shotgun (sprite 473) after picking upgrade 5. The
    original has no such limit. Find the live-bullet bound per gun, style
    and upgrade (pellets per shot, lifetime, fire rate) and size the pool
    from it, or retire the oldest bullet as an explicit, documented
    approximation.
21. **The world-one route driver fails without an end tick.** `CENSUS=1
    ROUTE=world-one bash amiga/profile.sh route` stops at its first
    sound-voice sample (`verify-world-one-native.py` `sampleVoices`: the
    `VOICES` line is missing), before gameplay, at `6095f6cf` and with
    either ring size; with `ROUTE_END_TICK` it skips the sampling and
    passes. Find why the voice format no longer prints, so the route runs to
    the hand-off with its witnesses and sounds checked.

Reported from a play session on the Amiga (27.11.2026; the user's screen
recording `Screen Recording 2026-11-27 at 15.05.49.mov`, times in it):

22. **A white rectangle after a T heart (0:40).** Picking up a heart with
    a T, a white rectangle rises from it before the TRIPLE text appears.
    Check the original's pickup (the T marker's and the text's draws).
23. **Stuck inside a wall without a jump (1:56).** The player can end up
    inside a wall and then cannot jump. Probably item 15's truncated
    horizontal test; check that first.
24. **The laser sight may cost too much (2:19).** Play felt slower with the
    laser sight, though the host was under load. Measure the laser route
    (`upgrade-12-v155`, W3.14) before changing anything.
25. **The shop selector gem is cut off at the top (2:44).**
26. **Garbage or torn frames over the shop's item preview (2:47).** Either
    stale data is drawn over the preview or it takes long enough to show
    torn frames; look for unnecessary clears.
27. **The GAME OVER fade (4:22).** The dither-pattern fade misbehaves and
    leaves the bottom of the screen stale.
28. **The results screen flickers (4:25–4:43).** The statistics flicker
    (static items apparently redrawn), the PROGRESS indicator flickers when
    redrawn and again at 1000 for an unlock, and moving between RETRY,
    STYLE SELECT and SURFACE flickers and draws garbage.
29. **SURFACE shows a corrupted garden (4:48),** like a wrong modulo.

### WM. Music (deferred)

The songs (`bgmOverworld` 193, `bgmArea1` 194, `bgmShopCave` 192 for the
shop radio, and `bgmSecret` 200 as the void-room ambience variant) are Ogg
Vorbis and need recomposing as tracker modules. That is not part of the world
1 slice yet. Until then the BGM calls, `bgmDuck`, the BGM pause and resume
around `pTimeStop`, the level-begin music start and the BGM option keep their
state but stay silent. The Paula mixer ([Amiga sound effects](amiga-audio.md))
must let a module player reserve voices later.

### W3. Performance

The target: every gameplay update of world 1 within 20 ms of work on the
A1200 with 4 MB Fast RAM (`MACHINE=a1200-4m`). Code, BSS and CPU-side data
run from Fast. Only DMA data shares the Chip bus with display, blitter and
audio DMA, so CPU writes into Chip (row painting, HUD and plane publication)
gain far less from the faster CPU than computation does. Scene handoffs are
judged by overall transition latency and visible stalls, not a per-update
limit. The updates the original slows on purpose are transitions, not frame
cost: `fxTimeChange` (two fields per update at room speed 30, plus a 30 ms
sleep), `momentDelay` (30 for one update) and player damage (40 for 40
updates).

The [frame-cost audit](amiga-frame-cost.md) was measured on the A500 and
predates most of W1 and W2: relative evidence only. Use the target's numbers
below.

**Working rules**

- Before writing code, state the removed work and the expected saving from
  the profile, in the commit message.
- Compare the same deterministic workload on the target, including its worst
  update. Trace only the affected updates, starting a capture one update
  early, and re-check a surprising number with a short capture before
  quoting it. Use the shortest workload that shows the owner.
- No tuning of code under 1 ms in the updates it runs in (loop rewrites,
  types, unrolling, inline assembly, compiler flags, helper substitutions).
  Take an owner only with at least 3 ms in a budget update. Both limits
  hold until the algorithmic items are done; hand tuning of the hot spots
  is then W3.22 (decided 27.11.2026).
- Accept when the targeted cost falls, no worst update gets slower by more
  than 0.5 ms and no mean the change runs in gets worse; another update may
  rise by about 1 ms when the change saves several times that elsewhere
  (report it). Untouched updates move by up to 0.5 ms between builds.
- An accepted change is the default in its commit, with the replaced path
  kept only as a `REFERENCE_*` oracle. No experiment flags, mode numbers or
  parallel candidates in the tree.
- Two rejected attempts end an item: one row each in the audit's
  [rejected approaches](amiga-frame-cost.md#rejected-approaches) table, the
  removed code as a patch in `docs/rejected/`. Closed approaches stay closed
  (below).
- Verify with the full RNG state, instance IDs, creation and event order,
  positions and draw order on deterministic replays (`make
  verify-cross-replay`, `make verify-terrain-ownership`, `make
  test-world-one-ownership`), plus the native pixel oracles when
  presentation changes.
- Delete your own traces once their numbers are in a commit (a 100-field
  capture is about 500 MB).

**Baseline on the target**

The initial fall (updates 1–164, no input) presents in 164 fields; display
buffering absorbs its two overruns, 136 (21.2 ms, 18.9 of it in
`GameplayWorld::step`) and 163 (28.7 ms, 23.5 in the step). Quiet updates
cost 5–6 ms, the row-painting updates about 11 ms. `profile.sh quiet-fall`
fails on any more than 164 fields; judge smaller changes by work cycles from
`profile.sh quiet-window` (`--work-summary`, baseline
`docs/amiga-quiet-work-baseline-a1200-4m.json`), with `quiet-state`
(`docs/amiga-quiet-states.txt`) guarding behaviour.

The normal run: `CENSUS=1 CENSUS_UPDATES=8192 ROUTE=world-one
ROUTE_END_TICK=8100 bash amiga/profile.sh route` (the
[census](amiga-profiling.md#per-update-work-census) of the host route
`world-one`: boot, the garden, normal mode 1-1 to 1-3 with shops, side
rooms, time-stop and the clear and upgrade screens, stopped at tick 8100
just before the hand-off to area 2; W1.21 covers running it to the end).
Measured at `6095f6cf` on 26.11.2026: 8,963 updates, the ring holding the
last 8,192 (the first 771, all front end, overwritten). Frames count
gameplay updates from the start of the run.

| Updates | Count | Work mean ms | Worst ms (frame) | Over 20 ms | Excess s |
| --- | ---: | ---: | ---: | ---: | ---: |
| Gameplay 1-1 | 2,226 | 21.6 | 57.7 (1569) | 1,069 | 9.7 |
| Gameplay 1-2 | 2,692 | 33.4 | 83.9 (3662) | 2,428 | 36.5 |
| Gameplay 1-3 | 1,536 | 42.1 | 338.5 (6847) | 1,501 | 34.0 |
| Side-room camera | 816 | 47.6 | 437.4 (5796) | 776 | 22.7 |
| Room speed below 60 | 405 | 44.5 | 313.5 (6692) | 405 | 9.9 |
| Clear and upgrade overlays | 424 | 5.1 | 107.8 (5974) | 9 | 0.2 |
| Front end (last 87) | 87 | 1.3 | 14.7 | 0 | 0 |

All gameplay: 6,454 updates, mean 31.4 ms, median 30.6 ms, 23% within
20 ms, 55% at 20–40 ms; its mean span is 33.5 ms, about 30 updates a
second. Handoffs: style select to 1-1 1,406 ms, 1-1 to 1-2 1,116 ms, 1-2
to 1-3 1,091 ms (splash and menu fell outside the ring).

The gameplay floor still rises within each level. The cheapest update of
each 200-update stretch goes from 4 ms to 12–29 ms through 1-1, from 16 ms
to 20–34 ms through 1-2, and from 17 ms to 24–42 ms through 1-3; the medians
go from 7 to 22–37, 20 to 31–47 and 23 to 33–58 ms. Each level change
resets it.

**Owners (W3.7, 27.11.2026).** The floor does not rise from accumulated
objects: each level's later stretches carry more per-update owners at
once (gem high from mid 1-1 onwards, corpses, jellies and red balls,
torches, time shards in side rooms). `CENSUS_PARTS=1` with the normal-run
census above splits the gameplay class (mean 31.9 ms, excess 82.5 s) by
loop stage, mean ms / share of excess: step 13.2 / 34.2 s, BOBs 5.2 /
14.1, effect composition 3.0 / 8.5, HUD 3.0 / 7.9 (almost all the gem
streak block), collect 2.0 / 5.6, overlays 1.3 / 3.2, rows 1.0 / 2.1,
assign 0.8 / 2.1; the side-room camera and slowdown classes rank the
same. Eight route traces at typical over-budget updates (800, 2019, 3442,
4431, 5228, 6649, 7236, 7528) give, per update: `GameplayWorld::step`
15.0 ms (steering 1.8: jelly steer, red ball, steering draw's binary64;
enemy room effects 1.7: bats, walkers, frogs; time shards 1.6, nearly all
their `sParentSolid` meetings; player 1.1; gem-high afterimages 0.9, all
RNG draws; enemy alarms 0.8; corpses 0.7; currency collection 0.4; a long
tail under 0.3), BOBs 6.3 (the CPU word compositor 2.7 at about 3,100
cycles a BOB, 35 BOBs at the 1-3 combo reward), composition 3.9 (corpses
on the surface 2.0 at 5228, shade subtraction), the streak block 2.9
(4.4 whenever gem high shows), collect 2.4 (torch fire 0.4). Compiling
`AmigaRenderer.c` at -O2 did not help (BOBs at 7236 13.3 → 15.4 ms).

**Items**

10. **W3.10 The fall's two overruns, 136 and 163.** Both are generation
    bursts inside `GameplayWorld::step` (the builder's next pattern and its
    event cells). First make event cells cheaper: breakables as source
    records under a lent-counter lease, enemy adoption through the native
    roster. Then stage the builder's next pattern in slack time, verify its
    key at the trigger update and commit it there. Gate: both at or under
    20 ms, a fixture that forces a key mismatch, unchanged state samples and
    replay.
12. **W3.12 Gameplay owners in census order.** Where an update's time goes
    now (spray route, the first side room and 690–699):
    - `GameplayWorld::step`, 16.1 ms at spray 690–699, spread thin: the
      per-enemy step 2.0 ms (bat, snail and crawler behaviour), jelly
      steering 2.0, debris 1.2, the ranged bullet 1.1, casings 0.9, alarms
      0.9, corpses 0.8, smoke against walls 0.7, the player 0.7, steering
      contacts 0.6. Debris, jelly and corpse steps are mostly their terrain
      queries.
    - Effect composition, 8–9 ms where smoke exists: about 19 masked
      interleaved stamps at about 5,000 cycles each, then the clear, shade
      subtraction and the publish copy (on constant fire 560–569: smoke
      stamps 4.1 ms of blitter, gem stamps 1.2, shade 0.5, the clear 1.6,
      the publish copy 3.9). The CPU still waits on the blitter
      (`isBlitterBusy`, `processBlitterQueue`); find the drains and the work
      queued ahead of them, then choose between fewer smoke stamps, a cheaper
      publish and CPU work moved between queueing and the first read. Needs
      a design pass. Keep `REFERENCE_EFFECT_COMPOSITION=1` and the final
      pixels as oracles, and the retained alpha raster's exact queries.
    - Actor drawing, about 9 ms in the side room (CPU-drawn assets 5.1 ms,
      bordered glyphs 1.6 ms), the time-field overlay about 3 ms, terrain
      rows 2.4 ms, collection 1.7 ms.
    - Time shards (5.25 ms in the side room): one swept-region check for
      empty cells could answer all three `sParentSolid` meetings at once.
    - The work W1 added, to measure when its workload is profiled: Hot
      Casing against every active enemy; Heart Balloon's string climb (up to
      21 point tests, five binary64 divisions) and enemy test; the blast and
      drone steps and the unowned enemy step order's insertion pass; the
      laser's ray and up to 96 trail BOBs; every shot's RNG draws; the
      eyefish's wall tests and the neon ball's enemy test; gem high's
      afterimages, streak block, `currencyText` and the Timer's HUD rows;
      up to 16 corpses with their `TimeStopBound` and wall queries; the 1-HP
      flicker; shop speech glyphs.

    **Fixed point instead of binary64 (decided 2026-11-25).** Replace
    core's software binary64 owner by owner with `FixedPoint.h` (Q16.16,
    table trig); bit-exact VM agreement is given up (about 1e-4 px per
    lengthdir, 0.002 degrees per point_direction). Each owner keeps its VM
    form as a `...Reference` oracle compared in `make test-fixed-point`,
    converts its stored state (positions and speeds as Q16) with its
    producers and readers together (converting only at a boundary does not
    pay), and re-searches any sound route whose autopilot no longer survives
    (`DOWNWELL_SLICE_VARIANT`). About 0.8 ms of binary64 is left per spray
    update, no caller over 0.1 ms: casings' `fromInteger`, smoke
    wall-contact scaling, the spawner countdown, mask bounds' binary32
    narrowing, steering and corpse pixel publication, flash-ball animation.
    The rest goes too (decided 27.11.2026): W3.20.
13. **W3.13 Scene handoffs,** judged by total latency through the first
    interactive frame and by visible stalls: gameplay entry (1,406 ms),
    level transitions (1,091–1,116 ms) and shop re-entry, the clear and
    upgrade screens, results and Retry, and the pause menus. Measure the
    last style frame and the first gameplay frame together
    (`profile.sh entry`); the remaining synchronous work includes the
    initial terrain raster and backing and HUD setup. Move immutable
    preparation out of confirmation without moving RNG draws or
    source-visible Create effects ahead of it. Level re-entry: reduce the
    mutable room-layer reset and publication (`PROBES=1 bash
    amiga/profile.sh shop`), keeping `REFERENCE_ROOM_PREPARE=1` as the
    oracle. Relocated copy work is not eliminated work.
14. **W3.14 The other workloads,** each a separate census once the normal
    run meets its target:
    - hard mode (`ROUTE=hard-mode-death`, to its death in 1-3 and the
      results), last measured before the fixed-point work; re-measure;
    - the upgrade routes that add per-update work (`UPGRADE_ROUTES`: Hot
      Casing `upgrade-8-v17`, Heart Balloon `upgrade-15-v2`, Gunpowder and
      Drone `upgrade-5-9-v53`, Rocket Jump `upgrade-0-18-v58`, the laser
      `upgrade-12-v155`);
    - side rooms: the spray route reaches the first side room's time-stop
      field (875–1009); still to cover are a torch room and gem high with a
      `gemChunk`'s gems on screen. Room speed 30 makes 25 updates per second
      correct there. The effect surface keeps one word span per row, so
      torches on both walls at one height publish and clear the full width
      between them; measure that before proposing two spans per row;
    - a dense-encounter tape, the contact tape (`profile.sh fps`) and the
      shop tape (`profile.sh shop`). The last two wait for the host
      reference's W1.4 shot (section 3): `verify-contact-profile` fails at
      frame 13, `generate-shop-profile` at parity frame 3 (camera X 159
      against 160) and the overflow witness at frame 7, group 1.
16. **W3.16 AGA fetch mode, as an optional fast path.** The game must keep
    working on ECS, so this is a second display setup chosen at start when
    the chipset is AGA (`GfxBase` `ChipRevBits0`), with today's FMODE 0
    display the default and the fallback. The framework's `setPlayfield`
    has an FMODE 3 branch, but `AmigaHardware::hasAGAChipSet` is never set
    and the game drives its own copper lists, so nothing uses it. The
    effect composition keeps the blitter busy (W3.12); 64-bit bitplane
    fetches use a quarter of the bus slots and leave the rest to the blitter
    and CPU Chip access. Keep 16-pixel sprites (FMODE bits 2–3 zero) so the
    offline sprite streams stay valid. Bitplane pointers and modulos must be
    64-bit aligned, so the 352-pixel fetch with its two-byte inset becomes a
    384-pixel fetch cropped by DIW; the 48-byte rows keep the PF1 ring
    aligned. First question: can a fetch window that meets FMODE 3's DDF
    rounding keep all eight sprite channels (today DDFSTRT `$38` keeps
    channel 7)? Work it out from the AGA documentation, then show every
    channel at both display edges in the emulator; if a channel is lost,
    stop. Then check that the profiling emulator times AGA DMA
    cycle-exactly, and compare constant fire and the normal run (mean,
    fields, worst update, blitter waits). Keep it only if it is faster and
    the ECS path still passes the display checks on an ECS machine preset.
20. **W3.20 No binary64 in core (decided 27.11.2026).** Convert every
    remaining software binary64 owner to `FixedPoint.h`, whatever its
    cost. Left (28.11.2026, 8 kept `MotionBinary64` symbols): the damage
    numbers (`PlayerDamageNumber`: the contact birth's direction, the
    step's lengthdir and 0.8 damping, the Amiga draw list's text
    position) with the player damage that feeds them through
    `stepGameplayPlayer`, then the primitives only they use
    (`SmokeDirection::component`, `RunnerPointDirection::fromDifferences`,
    `TimeShardBirth`'s vertical, `compareRunnerReal`, `divideBinary64`). Where
    the owned reference or StartupRuntime shares a type, the shipping world
    gets its own and binary64 stays only at a boundary the world-one build
    does not link (decided 27.11.2026). The motion may change slightly; keep each VM form as its
    `...Reference` oracle in `make test-fixed-point` and re-search the
    routes whose autopilot no longer survives. Done when the shipping
    executable links no binary64 arithmetic (no `MotionBinary64` or
    `MultiplyBinary64` callers, no hooks) and the 68020 hooks are removed.
    Inventory (27.11.2026, shipping ELF): 92 functions take or return
    `MotionBinary64`, about 2,850 source lines in 331 files (the SDL
    reference owners included). Audio is Q16 (`AudioLevel`). The SDL
    host's reference (`StartupRuntime`, the `Stored*` owners) is deleted,
    not converted (decided 27.11.2026); the stages convert the shipping
    owners and the tests that share them. In order:
    1. effect drawing and placement: `EffectSurfacePlacement`,
       `EffectSpriteDraw`, the smoke, debris, corpse and level-number
       draws, entry dithers and dither-above, `CompactRoomEffects`,
       flash balls, torch fire, `TimeStopEffects`, the sequence timelines
       and `RunnerImageValue` assignment, the draw list;
    2. enemies: `SteeringOps` and `RunnerPointDirection` /
       `RunnerDirectionValue`, the red ball and its `CheckCollisionWithScript`,
       the white jelly step, compact blood and immune sparks, the
       machine-gun shot's source motion, projectile publication;
    3. the player: `stepGameplayPlayer`, `PlayerSolidCollisionBinary64`,
       `PlayerHorizontalBinary64`, `RunnerMoveSnap`, the mask bounds
       (`RunnerSpriteBounds`, `CurrencyCollisionBounds`), box and goal
       contact (`PlayerBoxContact`, `RunSession::playerGoalContact`,
       `TimeStopGoalContact`), the shop world;
    4. remove `MotionBinary64` from the shipping link, the binary64 hooks
       and `Binary64Hooks*`. binary32 (`RunnerImageValue`) is a separate
       follow-up once nothing promotes it to binary64.
21. **W3.21 Structural options (approved 27.11.2026, not a priority).**
    Resting corpses kept outside the rebuilt effect surface (2.0 ms of
    composition at 5228), and a third gem-high streak design that visits
    only the words that change (the caps and the motion sprite).
22. **W3.22 Hand tuning (approved 27.11.2026),** once the algorithmic
    items are done: every hot spot, the working rules' 1 ms and 3 ms limits
    lifted. First the BOB word compositor (`drawAssetNormal`, about 3,100
    cycles a BOB, 6.3 ms at the pooled over-budget updates).
18. **W3.18 Re-audit and lock in.** Repeat the census on the target, with
    every active-gameplay update within 20 ms. Commit the results to the
    audit, and turn the cadences into regression ceilings: the quiet fall
    at 164 fields, and a ceiling for the world-one route. Check that the
    shipping footprint still fits the target.
19. **W3.19 Close W3:** remove this section, moving its working rules and
    closed approaches to section 5.

**Closed approaches (do not retry)**

Measured and rejected; the evidence is in the audit, `docs/rejected/` and
the git log.

- Terrain: shared intrusive room and lifetime descriptors, accessor tuning,
  another room-animation skip projection; the sparse-change room owner,
  room-state packing and sparse lists; initial-state, recipe-cache,
  cross-row grouping and observed-room reconstruction; the persistent
  multi-row identity replacement; hoisted metadata or layer admission
  caches; standalone registration-owner replacements and registry
  encodings; fresh-cell default preinitialisation and copy loops;
  immutable-row and copy-on-write spatial encodings. The source-record
  owner replaced the per-span structures behind one record per segment; do
  not reintroduce per-span terrain structures on the generated path.
- Row painting: the prepared pattern rows and paint-ahead (former section 0
  C) and the fixed-frame-work candidate (former D) were retired on
  26.11.2026: the target already met their goals, and on the routes the
  rows candidate saved about 1% of fields while making 27–48 updates
  1–3 ms worse.
- Effect composition: deferring per-stamp row extents into merged bands;
  the two rejected effect layouts (a new proposal must remove their
  exact-query or CPU source/mask-extraction cost); the delta-register stamp
  batch; offline shifted CPU stamps into the retained raster; both
  row-debt representations (disjoint ranges, shared word coverage with
  cached footprints); small sprite assignment and gem-position helpers.
- Quiet frames: enemy alarm-order variants, roster insertion, pinning,
  snapshot caching, merge loops, negative counter dispatch, the borrowed
  Step snapshot and generic temporary alarm-array layouts.
- HUD: the gem-high streak block's cached base with a delta publish and
  its direct per-word publish (W3.7, two attempts; a third must visit
  only the words that change).
- Drawing: converting to fixed point only at the drawFx placement
  boundary (cost as much as it saved); the two-word asset path (0.07 ms).
- The ammo gauge's runtime classifiers; bounded clean-style raster
  preparation and row-budget, scheduling, gather-loop or encoding sweeps
  for the gameplay entry; mutation-owned occupancy row extents for level
  re-entry.

## 1. Finish live skeleton and bone integration

**Parked: world 2.** In the original only `builderCatacomb` creates skeletons.
Resume after the world 1 slice (W). Task 1.01's scene collision candidate is
parked with its measurements; W2 and W3 may reuse its wind audio and backend.

### Scope, completion and working rules

The deliverable is a naturally generated skeleton encounter in the normal SDL
and Amiga executables: throw, bone/wall/player contacts, shooting, stomp,
time-stop, death, effects, rewards, retirement and Retry. Both must follow the
original game, not merely match each other. No test-only allocator, forced actor
birth, always-successful callback or manually emitted particle proves this.

The remaining work is split below into 18 bounded tasks. These are integration
deliverables, not 18 small patches. Keep their IDs stable until closed. Each
task must end with its production callers connected, the replaced path excluded,
and evidence covering both targets. Existing helpers should be reused unless
the live integration demonstrates a specific defect.

Important current boundaries to remove:
- `StoredEncounterSession` still uses the narrower `StoredDitherCollisionPass`;
  the registered skeleton and Shop event environments must be installed there
  around normal update/render as their family integrations are admitted.
- `DownwellAmiga.cpp` calls the ordinary `port.step(packedInput)`. The registered
  skeleton Step/contact entry points are exercised by tests, not installed here.
- `Game::render` has registered Shop Draw dispatch, but requires a borrowed
  environment. Exposing that hook is not installing a working frontend.
- Component fixtures in `EnemyDeathWorldsTest`, `RegisteredShopRuntimeTest`
  and `GameplayCollisionOwnerTest` establish parts of the contracts, not the
  natural encounter described above.

**How to work this section**
1. Start with 1.01 and 1.02, then complete the Shop vertical slice (1.03).
   Proceed through the dependency order below; do not spend another sequence
   of turns adding isolated Shop helpers without installing their callers.
2. Before editing, name the production caller, the legacy path it replaces,
   the missing inputs, and the end-to-end case that will prove the change.
   If those cannot be identified, investigate that specific boundary first.
3. Keep one task active. A newly discovered prerequisite must be attached to
   one of these tasks with the failing caller/test that requires it. Do not
   recursively expand the whole game or turn an optional later-area branch
   into a prerequisite for every earlier integration.
4. Integrate a complete vertical slice where possible; commit cohesive changes.
   A helper/test-only commit is allowed as preparation but does not close a task.
   Remove completed tasks; put results and timings in commits, not this queue.
5. At each milestone, report the normal route that now works, the remaining
   failing boundary, and the relevant test/capture. Count closed deliverables,
   not commits, source lines or isolated green fixtures.
6. Keep unsupported paths explicit. Do not enable a partial registered system
   beside the legacy system and count their combined output as source parity.
   Do not restore generic per-cell terrain ownership to make integration easy.
7. Preserve the whole scope below. Original-runtime evidence that cannot be
   obtained locally stays explicitly unverified (1.17); it must not prevent
   unrelated production integration, nor be silently replaced by SDL agreement.

### Milestone I — real session ownership and a complete Shop

#### 1.01 Install a production encounter session
**Scope (prerequisite brought forward, authorized):** install the minimum
native ordered query/dispatch admission that dither and wind need, from
1.05/1.07. Keep terrain identity in runs, preserve observable source event
ordering, and own collision/audio leases only in the scenes that use them. The
broader families and verification gates of 1.05/1.07 stay there.

**Authorized freedoms:**
- *Backend representation:* the production backend may use a different
  internal representation and need not reproduce identical tree or
  dirty-list history. Prove equivalent observable event order, RNG/audio
  ownership, destruction notifications and failure behavior against the
  retained full runner oracle; matching final state alone is insufficient.
  Keep exact selected-pair order, dirty-list retention and spatial history in
  that oracle.
- *Wall-cleanup order:* independent dither wall-cleanup notifications may be
  reordered. Preserve the exact set of walls removed, their removal update,
  pixels and gameplay state. A callback failure must stop before
  Draw/publication, but its partial wall-cleanup order may differ. This does
  not relax wind contact order, RNG/audio ownership or any other event family.

**Status:** the shipping default keeps the external collision backend. The
collision-enabled candidate (`scene_collision_candidate.mk`) completes the
fall with the exact landing state (tick 164, x 0, y 262144, camera y 1024,
grounded, health 4, ammo 8, three enemies, 1371 RNG draws) but takes **927 PAL
fields** for 164 updates (8.85 FPS; update 1 takes 202 fields). Generation
update **136** costs **840.8 ms**, 351.6 ms of it tree journal service; always
include the complete update 136 in performance gates, not just 137–141. The
[collection-read audit](amiga-frame-cost.md#encounter-candidate-combined-terrain-collection-read)
and the audit's encounter-candidate sections record the measurements so far.
Successful allocation and matching landing state are not a production
performance gate. Do not disable the external-backend guard without replacing
it with an explicit, verified scene admission, and first separate query
admission from the existing native terrain-span and record-backed actor fast
paths. Remove the remaining terrain-bound reconstruction and source
tree-maintenance work through a behavior-equivalent backend; do not substitute
small-window averages for the gate.
**Remaining integration gates:**

1. Remove the collision-admission performance regression above with a shared,
   explicit scene-specific backend. First establish the ordering rule for each
   admitted event family from the source and oracle, including overlapping wind
   fields, wall destruction, pending room changes and failing callbacks. Compare
   ordered wind/audio transcripts and intermediate state, not only final digests.
   For independent dither cleanup compare per-update wall sets and pixels;
   exercise failing callbacks without requiring the oracle's partial wall order.
   Keep the complete source collector compiled as the reference. A narrower
   production collector is permitted only after proving the omitted work has
   no observable effect, including incoming events and failure behavior.
   The [query census](amiga-frame-cost.md#collision-query-census-empty-result-traversal)
   identifies 42 empty queries out of 43 in update 136; traversal costs 69.5 ms
   separately from journal service. A global non-terrain history envelope was
   measured and rejected (see the audit's rejected-approaches table); do not
   tune envelope/check-layout variants. Any further exclusion needs an
   independently priced proof, including incoming events and cached bounds.
   Empty observed results alone are not a proof. Internal catch-up, generation
   stamps and unobserved pairs may differ. Preserve callback/failure prefixes
   except for the explicitly authorized independent wall-cleanup reordering.
   Add negative tests for dropped walls, late cleanup, reordered wind contacts
   and publication after failure.
2. Promote the scene-local native backend from `scene_collision_candidate.mk`
   to the shipping build only after that gate passes, compared against the
   reference owner, with the complete initial fall measured. Preserve the production
   `AmigaEncounterResources` binding of collision history, masks,
   `GameplayEncounterEnvironment` and `AmigaWindAudio`; do not substitute probe
   assets or successful no-op audio callbacks. Keep SDL and Amiga on the shared
   [session boundary](scene-architecture.md#encounter-session-boundary) and
   [admitted-event service contract](scene-architecture.md#admitted-environment-reads-and-writes).
   Any additional admitted consumer needs its own resource/read/write audit
   and real settings, input, language/font, tile/view and audio bindings.
3. Re-run final-build host gates: `test-encounter-session`,
   `test-gameplay-encounter-session`, `test-gameplay-dither-pass`,
   `build/gameplay-scene-collision-contract-test` and
   `build/native-collision-source-span-test` (execute both binaries).
   These must rebuild when shared headers change; use
   `test-host-check-dependencies` to guard against stale verification binaries.
4. Re-run final-build native gates: normal 1 MiB Chip startup, shortcuts,
   `verify-amiga-encounter-clear`, `verify-amiga-encounter-reentry`,
   `verify-amiga-encounter-update-failure` and `verify-amiga-encounter-failure`.
   Exercise actual production leases and callbacks, not only isolated device
   fixtures. Confirm release on scene/run exit and return 20 without further
   Draw/publication after an injected required callback failure. The lifecycle
   fixtures do not establish naturally generated encounters; those remain in
   their later numbered tasks.

**Done when:** Normal startup/shortcut, update, Draw, level transition and Retry
all construct/use/retire the session on both platforms. A deliberate callback
failure stops the frame and returns the meaningful failure code; an unbound
required handler is never silently skipped. No new game rules live in backend
adapters. This task provides lifetime/binding infrastructure, not closure of
the individual event families below.

#### 1.02 Complete the source creation stream and persistent ownership
**Depends on:** 1.01.
**Work:** Feed one source numeric-ID allocator through actual startup and
rmMain creation, independently of family-local generation handles. Include
serialized walls/controllers, persistent object 37, player-created camera,
depth meter, entry-dither and conditional companions, Shop children and spawner
78. Preserve synchronous Create, RNG interleaving, failures and
Create-before-deactivate. Complete stored companion events/Draw as well as
native ownership; an ID-only placeholder is insufficient. Publish activation,
rebinding and destruction through the same lifecycle. Use the
[main-room birth audit](original-behavior-audit.md#main-room-controller-births-and-numeric-ids).

**Done when:** Full startup and SKIP_TO_GAME reach equivalent source room state
with matching birth/ID/RNG transcripts through the first generated pattern,
including invisible children. Both normal frontends supply this allocator to
later effect callers. Retry/re-entry has no stale handles, reused live IDs or
missing persistent controller. Family behavior is checked in its task below.

#### 1.03 Finish a naturally encountered Shop end-to-end
**Depends on:** 1.01–1.02.
**Work:** Install registered Shop creation, cached language, nested per-parent
`with (Shop)` Step captures, shared input, synchronous purchases/child setters,
radio/view reads, child phases and ordered Room End. Bind parent and rising-text
Draw to real glyph/sprite primitives in both mixed Draw paths. Preserve retained
Create-time anchors, Draw mutations, negative cursor subimages and text state.
Supply pending activation/language-unpause behavior where reached. Exclude all
registered parents/children from compact compatibility passes. Resolve the
area-three keeper hat's blue pixels through an explicit native palette/art
decision, not silent quantization. Size overlapping speech lifetimes against
nested parent Steps, coordinated with §7.

**Done when:** The input-only natural shop route reaches, opens, navigates,
buys, displays price/description/speech, closes and leaves a Shop on both
executables. Stock, health/ammo, money, RNG, IDs, audio and child retirement
agree with source contracts; counter/menu/prompt draw once at their source
boundaries. Include funded/insufficient/sold-out, criminal, multiple-parent,
language-change and area-three presentation cases. Existing
`test-registered-shop-runtime`, `test-shop-parent-draw` and Game render tests
remain regression gates, not substitutes for this route.
Reference: [Shop contract](shop-port.md).

### Milestone II — complete live generation, queries and scheduling

#### 1.04 Admit the real spawner and its children
**Depends on:** 1.01–1.02.
**Work:** Connect `SpawnerStep` / stored and native spawner owners to natural
generation and mixed Step. Supply actual fall-depth, enemy counts, mask queries,
settings and synchronous child creation/deactivation. Install white-jelly 115,
red-ball 118 and object-220 moving effects, including sprite-704 emissions;
do not reinterpret them as sprite-84 blood.
**Done when:** A seeded natural route exercises spawner thresholds and both
child families; source birth/RNG/order, activation, motion, Draw and retirement
are checked on both platforms. No spawner or emission exists only as an ID.
Reference: [spawner contract](spawner-source-contract.md).

#### 1.05 Publish terrain and actor identity to live queries
**Depends on:** 1.02; test with families admitted by 1.03–1.04.
**Work:** Extend stored clock-query ownership to gameplay identity queries.
Connect native terrain identity runs and actor-family records to source-selected
spatial queries without generic per-cell records. Publish movement, mask/image,
activation, rebinding and retirement in source order; implement the selected
tree traversal and mixed spatial flush/post-pass retention. Complete terrain
animation/image updates, retained wall End Step and synchronous limboShard 379
births. Preserve coincident walls and first-hit identity, not just occupancy.
**Done when:** Normal-world identity-bearing queries select the same source ID
as the registered reference under overlapping/moving/destroyed candidates.
Include a field that determines shard direction, coincident walls, actor rebinding
and area-four wall End Step. Full ownership and terrain replay gates pass.

#### 1.06 Replace projectile approximations at their live setters
**Depends on:** 1.02, 1.05.
**Work:** Replace usual-gun Q20/Q32 live trajectory adapters with the recovered
setter/trig path, retaining absolute room coordinates and wrapper rounding.
Publish speed/direction/position mutations in order; retain muzzle births,
RNG, collisions and destruction timing. Connect non-precise line selection.
**Done when:** Both normal shooting paths use the same source-backed motion and
selection rules. A replay covers fractional/negative coordinates, wall/enemy
contacts, range expiry and multiple candidates, including source IDs and effect
birth order. Display-coordinate agreement alone does not pass.
Reference: [projectile contract](projectile-motion.md).

#### 1.07 Install the single ordered encounter dispatch boundary
**Depends on:** 1.05–1.06.
**Work:** Replace the narrower frontend pass with one sealed candidate stream.
Install dispatch routing for skeleton/bone/player/bullet, smoke/blood/wall and entry/exit-field pairs,
including reverse handlers and the authored dither 61/wall pair. Supply real
settings/ending state. Use `dispatchSkeletonEncounterCollision` and the stored
registered encounter admission with the real player factory/damage script.
Do not run extra per-family collision passes behind it.
**Done when:** Simultaneous contacts, reverse events, callback destruction and
wall retirement produce one source-ordered event transcript in both live worlds.
Every admitted pair has a handler or an explicit unsupported failure, never
a placeholder. Collision ordering is independent of storage/Draw order.
At this boundary, unimplemented families must fail explicitly; 1.09–1.13 own
their concrete handlers and 1.15 must admit the whole encounter without those
failures. Closing this dispatch task does not close those family tasks.

#### 1.08 Finish mixed phase and painter publication
**Depends on:** 1.02, 1.05; install each family as its task lands.
**Work:** Complete mixed Begin Step, animation, alarms, ordinary Step, collision,
End Step and cleanup without legacy duplicate passes. Publish deferred depth
changes, pending activation and Draw-time structural mutations. Use selected
GMS2 layer/element traversal, including inactive-instance termination and the
box's -60000 override. Merge terrain, fields and effects at original boundaries;
keep room animation order separate from Draw. Preserve entry-dither view reads
and effect-surface Draw.
**Done when:** A mixed production encounter records phase and painter order
including same-frame birth, destruction, activation and depth changes. Both
backends match the recovered ordering contract, and pixel tests cover overlap
rather than isolated sprites.
References: [phase eligibility](runner-step-eligibility.md),
[alarms](runner-alarms.md), [animation](runner-image-fields.md).

### Milestone III — time-stop and complete damage/death chains

#### 1.09 Connect time-stop, fields and shards
**Depends on:** 1.01–1.02, 1.05, 1.08.
**Work:** Install player Begin Step checkStop/pTimeStop and parent-field queries.
Connect startTimeField 371, ordinary/exit fields and wind contacts to live settings,
audio and scheduling. Admit Timeout object 82 from real damage, plus time-shard
and time-change births through the source allocator. Preserve fractional/mirrored
contacts, moving shards during paused gravity/animation, independent alarm
cooldowns, transition effects and RNG. Goal-stop/shop locks are not pTimeStop.
**Done when:** Natural entry, overlapping fields, exit and damage-triggered
Timeout run on both frontends with correct freeze/resume, shards, sound guards,
RNG and cross-family Draw. Neither backend bypasses the real stop state.
Reference: [time-stop identity](time-stop-collision.md).

#### 1.10 Connect actual player damage and knockback
**Depends on:** 1.06–1.09.
**Work:** Bind shared damage scripts at natural enemy/bone contacts. Preserve
immunity checks, player writes, hit effects, controller/audio side effects and
their order. Replace non-coincident fixed-point knockback with the source
point-direction/lengthdir path through subsequent motion. Resolve live damage
text formatting/placement, including fractional values if reachable.
**Done when:** Bone/enemy hit, simultaneous hit, immunity, stomp and fatal-hit
routes produce checked state/effect transcripts in both frontends. Measure native
contact cost and compare source-backed direction/placement, not merely two ports.
Original runtime uncertainty remains separately tracked in 1.17.

#### 1.11 Complete natural destruction and reward creation
**Depends on:** 1.02, 1.05, 1.08–1.10.
**Work:** Install registered Scattered16 289 Step/contact/Draw, including bullets,
forced-jump player writes/full recharge and Exploding Knees. Replace removal-only
box destruction with ordered sound, smoke, flash, impact, chain reaction, debris
and reward creation. Use source IDs and published debris state, not atomic legacy
registration. Bind actual tile/view/settings/audio. Connect registered currency
creation, homing, collection and Draw; establish unnormalized angle/trig behavior.
**Done when:** Naturally shot/stomped scenery and chained boxes emit the complete
effect/reward sequence on both builds with checked RNG, IDs, money and retirement.
Include debris time-stop and capacity failure. No manually spawned effect fixture
is used to claim the natural chain complete.

#### 1.12 Finish blood, corpse, blast and flash lifecycles
**Depends on:** 1.02, 1.05, 1.08–1.11.
**Work:** Install registered blood/smoke/flash/corpse/expiry/blast owners around
natural callers, removing legacy flash-pool and equivalent bypasses. Supply live
mask/tile/time-stop/Knife and Fork state. Preserve synchronous child Create,
rotated-mask contacts, particle order, corpse consumption and expiry before
rewards/retirement. Keep death/low-spec direct Draw at its original depth.
**Done when:** Natural death and consumption routes exercise complete effect
chains on both targets; overlapping currency/smoke shade holes and retained
restoration match pixel oracles, and simultaneous wall contacts retire correctly.
Reference: [skeleton effect contracts](skeleton-effect-ownership.md),
[effect surfaces](effect-surfaces.md).

#### 1.13 Connect death bookkeeping, first-land and controller HUD
**Depends on:** 1.02, 1.08–1.12.
**Work:** Route natural enemy-death callers through shared run-state bookkeeping:
actual toggleGuncut, achNoKill, momentDelay and levelBeginCue audio/creation.
Install firstLand/BGM and levelNumber 372 with real language/font/surface Draw.
Complete controller HUD streak geometry, Gemhigh, timer flashing, leading spaces,
icon centering and pause/time-stop guards in source Draw order. Use fully owned
controller identities, not an assumed singleton standing in for any population.
**Done when:** Natural first landing, kills, streak change/reset and paused/unpaused
HUD behavior run through the same shared path on both targets with source IDs,
audio order and pixel checks. No successful placeholder callbacks remain.

#### 1.14 Complete Result handoff and Retry
**Depends on:** 1.02, 1.08, 1.10, 1.13.
**Work:** Dispatch ordered Room End, load destination layers and rebind persistent
object 37. Admit native exit wipe to mixed destination alarms and finish stored
Result alarm storage/activation publication. Supply the actual shared room-speed
global; PAL pacing must not overwrite it with 50. Implement fired controller
events in the recovered Result contract; no old combat pass behind menus and no
batching all controller alarms ahead of Result alarms.
**Done when:** Natural death reaches rmResult, waits, accepts input and retries
twice on both builds with correct persistent state, alarm order, teardown and
source allocator behavior. Include pending alarms at the handoff.
Reference: [Result contract](original-behavior-audit.md#result-room-population-and-persistent-alarms).

### Milestone IV — natural skeleton encounter and final evidence

#### 1.15 Enable the registered skeleton and bone route
**Depends on:** 1.01–1.14.
**Work:** Install `withRegisteredSkeletonEvents` and native
`stepRegisteredSkeleton` in normal production scheduling, with real allocator,
tile, query, audio, settings, stop and reward environments. Route naturally
generated skeletons and synchronous bone births to these owners. Remove the
replaced encounter branches, not unrelated area-two content.
**Done when:** A deterministic input-only route reaches a naturally generated
skeleton, witnesses a throw and wall/player bone contact, shoots and stomps
skeletons, collects rewards, observes death effects and out-of-view retirement,
then dies/retries. It runs on both executables without a fixture-only birth.
Capacity failure stops later work while preserving the completed prefix.

#### 1.16 Add and run end-to-end encounter verification
**Depends on:** 1.15.
**Work:** Record route/seed/build/input hashes and coverage counters for the
encounter above. Compare full per-frame state, RNG, source IDs, phase/contact/
painter order and lifetimes, not only a final digest. Capture both renderers
during overlapping effects. Run the relevant component regressions,
`make verify-cross-replay` and `make verify-terrain-ownership`; ensure their
actual coverage includes the new route or extend it.
**Done when:** The normal executables pass the same declared route, failures
identify the first divergent frame, and coverage proves every required event
ran. Repeat after Retry. A passing old cavern tape does not close this task.

#### 1.17 Close original-runner evidence gaps
**Can proceed independently; required for full fidelity sign-off.**
**Work:** Resolve selected collision-family/same-object traversal, precise and
rotated-mask sampling, runtime FP rounding/control state and uncertain GPU
raster behavior against the original runner. Keep assembly/resource evidence
separate from inferred runtime behavior. Obtain original encounter/overlap
captures for comparison, including damage direction and currency/smoke shade.
The user has no Windows environment; first establish an available way to execute
the original or obtain trusted captures. Do not assume installing Wine or renting
a machine is authorized, or that compatibility-layer output proves Windows FP.

**Done when:** Each uncertainty has original-runtime evidence or a sufficient
source/binary proof covering the actual path. Missing x87/MXCSR runtime evidence
must remain explicitly unverified; host/native agreement cannot close it.
References: [collision bounds](runner-collision-bounds.md),
[FP startup evidence](runner-fp-startup.md). This external-evidence gap does not
block implementing or testing the other tasks locally.

#### 1.18 Validate the integrated workload and memory on the target
**Depends on:** 1.15–1.16.
**Work:** Run bounded native natural-encounter captures, including dense smoke,
debris, gems, fields, shops and Retry. Measure actual per-owner costs, worst
gameplay frames, Chip/Fast peaks and stack/capacity margins. Confirm 68020-only
output outside AmigaStart.c and meaningful allocation failures. Use offline assets and retained
hardware scrolling; no runtime sprite conversion or silent RAM expansion.
**Done when:** Reproducible measurements and coverage are recorded in the commit;
newly exposed costs and fit failures have concrete entries in §5/§7. Do not claim
20 ms or fit on the target unless the measurements prove them. Finishing §1's
integration does not erase those separate performance/memory requirements.

## 2. Make catacombs playable

**Parked: world 2.** Resume after the world 1 slice (W).

- Connect recovered patterns and symbol dispatch to both live generators,
  including synchronous actor creation and builder state.
- Implement skulls, spiders and remaining catacomb hazards: source-specific
  motion, collisions, alarms, animation and artwork.
- Implement area transition/entry/exit and verify natural cavern-to-catacomb
  progression. Do not substitute cavern actors or patterns.
- Declare and budget catacomb CPU/DMA asset dependencies before enabling its
  production admission. Include source spawns, effects, persistent objects and
  transition overlap; verify unload/re-entry. Do the same for each later area;
  never restore the full-catalogue fallback.

Reference: [area-two recovery](area-two-recovery.md).

## 3. Close original-game fidelity gaps

- Audit remaining random-choice call sites and full RNG state through creation,
  effects and rewards. Rebuild checkpoints after source-backed corrections.
- Audit remaining event ordering, fractional collisions, activation boundaries,
  time-stop/feedback, special camera modes and overlapping camera lockers.
  Finish remaining phase-aware published-view reads, including camera dirty
  setters, initial room publication and native time-effect queries; see
  [runner camera view](runner-camera-view.md).
- Replace approximate live animation where original float/sequence behavior
  changes observable frames or event timing. Cover relevant sprite/index writes;
  a legacy animation helper is not the selected GMS2 sequence path.
  For currency, integrate registered active-room membership, pending activation
  semantics and time-stop speed writes; see the currency animation audit.
  The standalone pre-Step animation pass does not close these ownership gaps.
- Run the SDL host on `GameplayWorld` and delete the host reference
  (`StartupRuntime` and the `Stored*` owners) rather than bringing it up to
  the native world 1 (decided 27.11.2026; the clean-up after W3.20). The
  reference's gaps, which the tests below stop at: its player shot is still the pre-W1.4
  `MachineGunTrajectory`: no `bulletRanged` Create draws (`random_range` for
  imgSp and bSpeed), muzzle effect, gun shake or `bulletCasing` (walls, Hot
  Casing); `GameplayWorld::firePlayerGun` is the native model. And it starts
  no time stop at the goal unless the caller drives `stepWithPlayerTimeStop`,
  which the SDL platform does not, so enemies inside the goal field freeze
  instead of shattering into gems. `amiga-scene-runtime-test` stops its
  decoded-scene parity at the first landing and its shop parity after the
  exit latch's grants because of these; extend both once the reference
  matches (they ran through the shots, results and later rooms before).
  `make verify-contact-profile` stops at gameplay frame 13 for the same
  reason (the tape fires): camera Y 68 against the reference's 69, player
  194 against 194.199. `make generate-shop-profile` stops at parity frame 3
  (camera X 159 against 160), and `profile.sh overflow-state` at frame 7
  (witness group 1); check that they share this cause.
- Complete side-room reward objects and omitted title/particle effects.
- Verify fractional raster positions, draw order, effect surfaces, shaders and
  alternate palettes against original-game captures.
- Verify Laser Sight's original collision-line rounding, mask behavior and GPU
  rasterization; host/native agreement alone does not establish these.
- Fill resource/manifest gaps needed by new features: masks, glyph/kerning data,
  room layers and extension dependencies. Preserve IDs and input/tool hashes;
  check decompiled code against VM instructions.

References: [behavior audit](original-behavior-audit.md),
[random choice](runner-choose.md), [activation](activation-fidelity.md),
[side cameras](side-camera-lockers.md).

## 4. Complete remaining game content

**Parked except where W needs it.** World-1 guns, upgrades, music and sound
are W items; the rest waits for the later worlds.

- Remaining areas, enemies, hazards, bosses and ending flow.
- Remaining weapons, styles, upgrades and their interactions; hard-mode rules.
- Complete shared projectile state and lifecycle before dependent upgrades:
  damage, direction, speed, range, penetration, artwork, creation RNG and
  collision order. Use it for Popping Gems, Gunpowder Blocks, Drone, Hot Casing
  and weapon-specific Laser Sight behavior rather than backend-specific rules.
- Complete blast/corpse lifecycles for Blast Module, Rocket Jump, Rest in Pieces
  and Knife and Fork; gun-module hit/reshuffle behavior for Reverse Engineering;
  companion/time-field lifecycles for Heart Balloon, Drone and Timeout; and
  Safety Jetpack input, fuel, reset and thrust.
- Complete upgrade pickup/grant/HUD effects and validate each upgrade through
  a live deterministic encounter, original-game evidence and bounded native
  execution. Inventory or offer support alone does not close an upgrade.
- Music and sound on both platforms, including Amiga conversion, mixing,
  timing and memory residency.
- Persistence, unlocks, achievements where applicable, settings and complete
  restart/continue behavior.

Implement shared behavior and connect both backends. A feature absent from both
ports does not count as parity with the original.

Reference: [upgrade IDs and source entry points](upgrade-coverage.md).

## 5. Meet the Amiga frame budget beyond world 1

**Parked: world 2.** World 1's performance work is W3, with its working
rules, target and closed approaches; apply them to each later area. The
earlier A500 items (generation bursts at 701 and 844, the contact damage
number, combat composition, quiet frames, the ammo gauge and the other
scenes) were folded into W3 or retired with their measurements in the
[frame-cost audit](amiga-frame-cost.md).

- **Keep native ownership sufficient.** Native ownership rejects every
  collision query and wall End Step. When an original query or wall event
  enters native gameplay, answer it from native state (the grid for terrain
  selectors, family records for actors) with the same first-hit identity
  and order, verified against the registered reference. Do not re-enable
  per-instance spatial publication for it. Actors at the wall depth (100)
  are rejected, because an inactive wall ends that layer's Draw; supporting
  one needs the layer's wall-order cutoff.
- **Coverage.** Each later area needs a dense-encounter tape and its own
  census before claiming the budget. Particle-heavy coverage depends on the
  destruction and effect integration in section 1; manually emitted
  fixtures or a bullet-only census do not prove it.

Reference: [profiling procedure](amiga-profiling.md).

## 6. Finish scene and backend separation

- Move remaining cache-refresh/display coordination and transition orchestration
  out of `DownwellAmiga.cpp` into scene/backend classes. Consolidate SDL startup
  orchestration too.
- Keep publisher, credits, controls, garden, style selection, gameplay and upgrade
  selection independently owned; share style/upgrade presentation where useful.
  Keep game rules in `src/core`, not duplicated in platform backends.
- Consolidate remaining world scheduling and state adapters so both platforms
  invoke the same simulation. Remove replaced rule branches rather than
  retaining alternative platform-specific implementations.

Reference: [scene architecture](scene-architecture.md).

## 7. Establish memory fit and failure handling

- Measure the task stack and peak transition usage (the gameplay footprint
  and the startup requirement are in `docs/amiga-memory.md`);
  expanded-emulator runs do not prove stock-machine fit. Size the shop speech overlap reserve against repeated
  messages within the nested parent Step, not just one replacement per parent.
- Chip residency: the route driver checks peak Chip and public use against
  the declared startup requirements (`downwellChipRequirement`, about 1 MB
  of Chip and 2.5 MB of further memory). The A500-era free-Chip budgets in
  `docs/amiga-memory.md` no longer apply. Reconcile gameplay-family
  residency before admitting more encounter artwork, and keep the 68020
  target; do not raise emulator memory to make a check pass.
- Enforce scene- and level-local residency before adding content: replace
  the remaining resident gameplay tables, room catalogues and SDL collision
  masks with explicit scene and level dependencies (reachable spawns,
  persistent objects, nested Create and Draw dependencies, transition
  overlap), and fail on an undeclared resource rather than falling back to
  all assets. Prove peak memory and unload and re-entry.
- Dispatch phases from live, eligible family membership, not the catalogue
  of implemented families, keeping event and RNG order: a later family must
  add no empty per-update scans to world 1. Add fixtures with unrelated
  resources and families that show unchanged residency and work counts.
- Reduce executable and static memory at their owning structures (linked
  query machinery, embedded banks, full-height terrain caches), accounting
  Hunk code, data and relocations, BSS and Chip allocations separately.
  Moving resident bytes into startup allocations is not a saving.
- The effect stamps are still linked into the
  executable, so ordinary memory holds them beside any Chip copy. Give them
  scene/level dependency contracts and bounded file residency; merely moving
  every bank to a permanently loaded sidecar does not reduce memory.
- Exercise allocation/capacity failures and scene teardown/re-entry. Preserve
  meaningful nonzero exit codes for startup failures.
- Propagate terminal failures through the remaining direct actor, reward and
  collision loops: stop later work in the same frame without rolling back
  completed effects. Checked event dispatch alone does not cover these loops.
- Maintain reproducible offline assets and instruction/helper checks
  (68000 for AmigaStart.c, 68020 elsewhere) across supported build
  configurations.

## 8. Broaden end-to-end verification

- Restore `make verify-amiga-gameplay-bobs` (the blitter-against-CPU effect
  pixel oracle and overflow actors). At 7c49136c it never reaches
  `downwellGameplayBobsReady`: no failure breakpoint fires and the emulator
  runs past 100,000 fields (480 s budget). Its script now sets
  `remotetimeout`; the hang is in the game run, not the connection.
- Restore `verify_gameplay_camera.gdb`: at 2711c5b4 it never reaches
  `downwellGameplayLongRunReady` (120 s and 900 s budgets), like the BOB
  verify above. Its script now sets `remotetimeout`.
- Restore the damage-number shared-world capacity-failure/reset regression:
  `StartupRuntime::reset` must recover a session after number capacity is
  exhausted. The committed reference also fails this assertion; do not weaken
  the failure/reset expectation to make the test pass.
- Isolate the unexplained native main-replay digest mismatch seen without
  per-frame tracing. Spatial/effect subtests matched; adding per-frame capture
  made two subsequent builds pass. Passing traced builds do not establish the
  cause or prove the mismatch fixed. Preserve a failing executable and symbols
  before restoration, then distinguish build-sensitive behavior from fixture
  or execution-state differences.
- Cover naturally funded purchases, all shop/side-room layouts and camera
  transitions, dense combat, uncommon upgrade combinations and repeated runs.
- Compare new behavior through deterministic host/native replays, including
  complete RNG and owned-instance state.
- Compare fresh terrain/HUD composition with retained buffers on natural routes;
  synthetic geometry alone does not establish rendering fidelity.
- Compare original Windows behavior/captures with both ports through startup,
  selection, gameplay, death, upgrades and later-area progression.
- Run all relevant regression gates before claiming parity. Component tests,
  CPU replays and screenshots each prove only their respective scope.
- Release compatibility: 0.1 was tested on an emulated A1200 with 4 MB Fast
  and a real A1200 with a 68040. Test an ECS machine with a 68020 and Fast
  RAM, and the minimum memory, before claiming them in the ReadMe.

## Closing work

Remove an item only when its shared behavior, both backend integrations and
relevant verification are complete; W items follow their own bar (Amiga
first, see How to work W). Record implementation and test results in
the cohesive commit, not a second status document or a completed-items section.
Keep format specifications, source audits and reproducible test instructions in
the linked technical references; keep priorities and remaining work here.

Use `SKIP_TO_GAME=1` for gameplay, `SKIP_LEVEL=1` for completion and `SKIP_INTRO=1`
for style tests. Test full startup when scene/display ownership changes.
Time-limit emulator runs, stop owned sessions and restore normal builds.
Never modify `data/` or commit copyrighted exports/generated payloads.
