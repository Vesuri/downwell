# Remaining work

This is the single work queue for the Downwell port. Keep only unfinished work
here; remove items when completed. Completed changes belong in the git log.
Linked technical documents explain source behavior and implementation constraints,
not separate priority queues.

Both SDL and Amiga must behave like the original Windows Steam game. VM assembly,
serialized resources and original-runner evidence are the authority; port-to-port
agreement alone is insufficient. Target Amiga 500/68000. Let gameplay run slower
on PAL; do not add 60 Hz catch-up.

## Priority and scope

The goal, set by the user on 2026-10-23, is a **world 1 vertical slice**: the
first world of Downwell, from boot to the end of level 1-3, fully implemented
and feature complete in the normal Amiga executable. Work it in this order:

1. gameplay and visuals (W1);
2. sound effects (W2; music is deferred to WM);
3. performance (W3, which absorbs section 0 and the world-1 part of section 5).

Restructure the code alongside (WR) so the Amiga build compiles in only what
world 1 uses, while keeping already-implemented later-world code available.
Only after W is complete, continue with what world 2 needs: the skeleton
encounter tasks (section 1), the catacombs (section 2) and the remaining
content (section 4).

Sections 1, 2 and 4 are parked. Sections 0 and 5 are W3's performance work.
Sections 3 and 6–8 still apply, but only where they touch world 1.

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
  binds the shipping collision owner and encounter session: without it,
  same-object enemies step in activation-slot order instead of registration
  order and the native replay drifts.
  `tools/verify-world-one-native.py` replays the tape on the normal build;
  `DOWNWELL_DENSE_CHECK=from,to` checks every tick in a window.
- **One item at a time,** in the listed order unless a dependency forces
  otherwise. Attach any newly discovered prerequisite to an existing item.
- **Performance during W1/W2:** no 20 ms gate yet, but do not add per-frame
  work for features that are not present (no empty scans), and keep normal
  1 MiB Chip startup and the `quiet-fall` landing witness passing. Note any
  large new cost in the commit for W3.
- **Restructuring** steps (WR) must be behaviour-neutral and pass the existing
  gates; do them when they make the next W item easier, not as a separate
  sweep.

### W1. Gameplay and visuals

Current state of the normal Amiga build: boot, garden, style select,
area-1 generation and scrolling, movement, stomping and shooting, the health,
ammo and gem HUD, damage numbers, six builder-placed enemy families (snake
111, ground turtle 112, frog 113, bat 116, snail 117, crawler 119) plus the
level spawner 78 with its white jelly 115 and red ball 118 (normal-mode
channels, steering motion, stomp and bullet deaths, eye art), box bursts (smoke,
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
After the 1-3 clear the run reaches a deliberate end-of-slice screen at
CATACOMB-1 without loading area 2. Items keep their numbers as they close.
Known approximations: overlapping `moduleText` records never push each other
down (Collision with `moduleText`), and `moduleText`, `itemNotif`, "EMPTY!" and
the `levelNumber` letters assume top-left text alignment although the HUD
leaves `fa_middle` set; "EMPTY!" reuses the `itemNotif` record for
`fxRisingText`, skips its Collision with `fxRisingText` and is born after the
whole player Step. `levelNumber` is drawn on the fixed HUD layer: above the world and the hardware sprites,
and hidden while the death flash covers the playfield. The merchant's speech
(`fxRisingText` from `scrRisingTextShop`) uses the same alignment as "EMPTY!"
and rises in Q16 like `itemNotif` rather than binary64. The Shop Step's
messages, sounds and BGM gain are produced but silent until W2, and its
`shopInView` and `keeperAngry` flags drive only sounds. The criminal route
cannot happen in the pinned build: only a shop case's `destroyed` flag sets
`global.criminal`, and nothing writes it (`original-behavior-audit.md`).
The pause menus drop the PC-only DISPLAY, LANGUAGE, Quick Start and key
binding screens, and the BGM and volume options are stored but silent (volume
until W2, BGM until WM). PlayMenu's HARD MODE label is static (no whomp). The save is written at
static moments (results opening, pause close and its actions, style confirm,
slice end and quit), not on every `ini_write_real`; the slice end counts as a
run end for the records. The clear screen shows the style-0 poses for every
style, and the first run after boot keeps the builder checkpoint taken before
style select, patched with the chosen style and hard mode.

9. **Upgrade effects.** Every upgrade the clear screen can offer in world 1
   (IDs 0–22 except the disabled 2, 19 and 21) must work in play, not only be
   selectable, including the balloon/decoy target that steering enemies
   chase (they now always chase the player), box chain reactions (upgrade
   5) and exploding knees on scenery and boxes (upgrade 0). W1.4 left Hot
   Casing (upgrade 8, damaging casings), Reverse Engineering (upgrade 1,
   `pugEngineer` on gun pickups) and Laser Sight's beam against the new guns'
   spread (its `bdirRand` and `decTimer` changes are in) to this item, and
   W1.5 left Time Field Damage (`pugTime`: `scrTypicalDamage` from the time
   fields), and W1.6 left Rest in Pieces (`pugRip`: corpses that explode).
   Reference: [upgrade coverage](upgrade-coverage.md).
10. **Hard-mode enemies.** Hard mode itself works (PlayMenu toggle, the
    cavern builder's extra enemies, doubled shop prices, "-H" titles, the
    records' +100), but its area-1 spawner sets (`hardSet1`, already in
    `SpawnerStep::createCaverns`) need two enemies the port lacks, so
    `GameplayWorld` still creates the normal sets: `enmEyefish` 114 (sprites
    289/290/291, ehp 15, bullet-immune, stompable, a horizontal patrol at
    `movesp` 0.5 that turns at walls; its Create never arms Alarm 1, so it
    never fires `pooferBullet`) and `neonRedBall` 173 (the red ball's steering
    with ehp 40, maxsp 1.5, knock-back on hit, `sprNeonRedBall` and the
    `emitMovingFx` 704 trail, from above). Pass `run_.hardMode` to
    `createCaverns` once both exist. Also still unseen natively: the results'
    unlock notice (implemented; route variant 445 dies in 1-3 on the host but
    is still alive in 1-2 natively at its last input, so a dying route that
    crosses a goal still has to be found and captured).
11. **Low-health flicker.** At 1 HP `playerDraw` replaces the player, on 7 of
    every 52 Draws, by two `shaderPlayerRed` copies (white and red texels red,
    black transparent) jittered by `irandom_range(-2,2)` and
    `irandom_range(-1,1)`. The draws feed the world RNG, so adding it reshuffles
    the route.
12. **Walking enemy wedged in a solid hangs the compact world.** Found with the
    W2.3 RNG stream: currency tape `--overflow` with startup seed 40 and
    encounter route 0 (`SCAN_ROUTE`-style override of `amigaOverflowRoute`).
    Around post-Step frame 831 a grounded `enmSnake`-family walker (object 111)
    at offset (-34, 1706.6) is suddenly inside a solid band from y-20 to y,
    with nothing below. `WalkingEnemyMotion::step` then takes its
    `place_meeting(x, y + ysp)` branch, and the "count down to the floor" loop
    never ends. The source has the same loop shape, so the original relies on
    walkers never overlapping a solid's underside; find what writes the solid
    over the walker (incremental generation or a box/cell update) before
    adding any guard. A hang on a natural route is a ship blocker.

### W2. Sound effects

Scope set by the user on 2026-11-09: world 1's **sound effects** on Paula.
**Music is deferred** (see WM): every song is Ogg Vorbis and will have to be
recomposed for a tracker later. Sounds that are needlessly long may be left
out. Quality is an experiment, with Chip RAM kept in check. Work the items in
order; each sound group closes only when it is audible on a natural route in
the normal build. User decisions (2026-11-09):
- **Chip:** spend it generously and get in as many sounds as fit, instead of
  setting a small budget.
- **No streaming:** nothing streams from Fast RAM. A long sound is cut to fit
  Chip, or dropped.
- **Panning:** four hard-panned voices, as Amiga players expect. Some of them
  will later go to music.

**Inventory (W2.1):** [sound inventory](sound-inventory.md), generated by
`tools/sound-inventory.py` (`--check` verifies it), lists every sound world 1
reaches, with its size at 8,363 Hz after the -30 dB crop, scenes, callers
and a verdict. The tool takes a closure from the world-1 rooms over object
events, scripts and creations, keeping only the builder symbols in the
area-1 patterns, and names every cut with its reason.
- **What world 1 reaches:**
  - 114 objects reach 181 sounds. 164 of them are kept or cut.
  - The rest are dropped:
    - music (WM);
    - water and oxygen sounds;
    - the wood and metal box breaks of areas 2 and 4;
    - the PC-only menus;
    - `sfxNull`.
- **Sizes of the short sounds:**

  | Scene | Short sounds |
  |---|---:|
  | Gameplay | 284 KiB |
  | Shop | 40 KiB |
  | Front end | 33 KiB |
  | Results | 23 KiB |
  | Clear | 11 KiB |
  | Pause | 6 KiB |

  The garden also needs the player's footstep, landing and jump sounds.
- **Long sounds, to be cut:**
  - the seven time-field winds 349–355 (5.7–8.4 s each, 411 KiB);
  - `amb1` 191 (an 86 s loop, 706 KiB);
  - `sfxFalling` 186 (an 8 s loop);
  - `sfxTitleAppear` 188 (3.6 s);
  - the jetpack loop 343 (2 s).
- **Original playback rules:**
  - GameMaker runs 32 channels (`audio_channel_num(32)`).
  - `soundPlay` carries a priority and a cut-previous flag.
  - `soundPlayOL` adds a category (footsteps, gunshot, gunwall, merchant,
    enemymove, music, waterEnemy, waterThings, UI, boss) and cuts the
    category's previous sound. Its `enemymove` and `waterEnemy` categories
    skip sounds from offscreen callers.
  - Gain is the asset volume × `global.gameVolume` × `audio_master_gain`.
  - Pitch changes: low ammo +0.8, the death and results progress bars, and
    `objPlayer_n` Alarm 8.
- **Audio state can gate gameplay RNG:**
  - `soundLand` draws its sample (`irandom_range`) only while
    `sfxGunRecharge` 27 is not playing.
  - `goalTimeField`'s Collision and the clear screen's `Result` Step draw a
    wind (`choose` of seven) only while `global.voidWind` is not playing.
  - The port must therefore know when the original sample would still be
    playing, as its source duration in 60 Hz ticks, independent of any
    Amiga cut.
  - The other audio reads (`bgmDuck`, `gunshotStop`, `levelBeginCue`,
    `sfxGemBoostLow`) only affect audio.
- **RNG audit:** the compact `GameplayWorld` (the Amiga's shipping path) is
  missing five world-1 sound draws, listed in the inventory:
  - `soundFootstep`;
  - `soundLand`;
  - `soundBullet` (a bullet on an unbreakable wall);
  - the goal-field wind;
  - the clear screen's wind.

  The Exploding Knees and decoy-balloon draws belong to W1.9. The other
  world-1 draws are consumed.
- **What exists today:**
  - `AmigaWindAudio` plays one mono sound through two 4,096-byte Chip
    fragments, with VBlank and audio IRQs and a master gain.
  - `SoundPlayScript`, `TimeFieldWind`, `BulletWallSound` and the
    `RunSession` hooks drive the registered owners.
  - The compact world's hooks return true and are marked `// Audio is W2`.
- **Memory:** measured on 2026-11-20 with the normal 1 MiB build in level
  1-1 (outside a shop): 476,576 bytes of Chip are free, 476,520 of them in
  one block, so about 465 KiB. Startup leaves 651,136.

2. **Conversion and the quality experiment.** Decided on 2026-11-20:
   every class at **8,363 Hz with a -24 dB crop**; the seven winds as
   1.5 s swells, the void ambience as a 2 s loop, `sfxFalling` a 1 s loop,
   the jetpack a 0.5 s loop and the title sting 2.5 s. The run bank
   (gameplay, shop, pause) is 417 KiB of the 465 KiB free, leaving about
   47 KiB for the mixer, W3 and later art; all kept sounds come to 491 KiB.
   The bank comes from `tools/sfx-bank.py --bank` (`make amiga-sfx-bank`)
   with `tools/sfx-bank-policy.json`; `--preview` rebuilds the listening
   page. Paula's minimum period (124) bounds pitch × rate: at 8,363 Hz the
   guns' 1.8 and the progress-bar fill's 2.2 fit.
3. **Core sound requests.**
   - **Done:** the gameplay change (the sound RNG draws, `SoundVoices` and
     the player's hop, grace jump, wall kick and tiny jump). Its quiet-fall
     recovery is in W3.
   - **Route logs:** the host logs every sound request (tick, ID, priority,
     loop, cut, category, gain, pitch) on the natural routes: the world-one
     tape, the menus driver, a death route and a hard-mode route. The
     inventory gains each sound's observed count and peak concurrency.
   - **Request ring:** a fixed `SoundRequests` ring in `src/core`, filled at
     the original call sites in source order. The compact world, shops, run
     session, pause menu, clear screen, results and front end all write to
     it. The `// Audio is W2` stubs become real requests and the missing
     sites are added.
   - **Core bookkeeping:** `global.OLchannel`, cut-previous and offscreen
     suppression live in core. The gameplay-visible `isPlaying` reads (the
     recharge sound and `global.voidWind`) use the source-duration model.
     The audio-only ones may use the backend's voice state.
   - **No further gameplay change:** after the RNG fix, the host tapes stay
     byte-identical, and host tests check the request logs of the routes.
   - **SDL:** it may play the logged requests as a reference. This is
     optional.
4. **Paula mixer.** It is also the first player of the W2.2 bank: listen
   to the chosen set in FS-UAE before wiring.
   - **Generalise `AmigaWindAudio`** into four voices, with commands
     published by the main loop and applied in the VBlank, and no allocation
     in IRQs.
   - **Voice stealing:** take a free voice; otherwise steal the lowest
     priority (the oldest on ties) when the new sound's priority is at least
     as high; otherwise drop the sound.
   - **Playback:** one-shots end on a silent word and loops repeat their
     segment. The period comes from rate × pitch; the volume is asset ×
     `gameVolume` × master, scaled to 0–64.
   - **Panning:** each voice is hard-panned (Paula's 0 and 3 left, 1 and 2
     right). Alternate sides between consecutive starts where that is free.
   - **No streaming:** every sample is Chip-resident. Any long sound that is
     kept (winds, ambience, the clear-screen whoosh) is cut to a short loop
     or segment.
   - **Music later:** let WM reserve some of the four voices.
5. **Scene-local banks.**
   - **Gameplay bank:** loaded with the cavern banks and kept across Retry,
     like the plane atlas. Pause sounds live in it.
   - **Front-end bank:** title, garden, style select and menus.
   - **Clear and results sounds:** in the gameplay bank or in those scenes'
     transient banks, whichever the inventory favours.
   - **Gates:** measure free and largest Chip before and after
     (`verify-amiga-memory`, `verify-amiga-startup`), and keep the 1 MiB
     startup gate.
6. **Wire world 1 by groups.** Each group is audible on a natural route and
   checked natively: GDB reads the voice table at the ticks the host log
   names. The groups:
   1. player, guns (`bStatSound`, empty, recharge, low ammo), bullet hits,
      stomps and gems, on the world-one route;
   2. enemies, rock and box breaks, torches and gem chunks;
   3. shops, the keeper's voice, module pickups (105/106) and the
      `moduleText` letters (334), on the W1.8 shop route;
   4. side rooms (void enter and exit 184/185, the ambience if kept),
      time-stop and the winds;
   5. combo, upgrade sounds (jetpack loop, rocket jump, gem shoot), the
      clear screen, death, results, unlock notices, menus, palette, title
      and the level-begin cue;
   6. the master gain and the stored volume option.

**Gates for every step:**
- Quiet-fall stays ≤ 197 fields: idle audio costs nothing but Paula DMA, with
  no per-update scans.
- The world-one native replay still ends `END 2 1 8275 1 265269` (slice
  variant 119).
- 1 MiB startup passes.
- W3 costs are noted: IRQ and VBlank time per start, Chip and Fast bytes.

### WM. Music (deferred)

The songs (`bgmOverworld` 193, `bgmArea1` 194, `bgmShopCave` 192 for the
shop radio, and `bgmSecret` 200 as the void-room ambience variant) are Ogg
Vorbis and need recomposing as tracker modules. That is not part of the world
1 slice yet. Until then the BGM calls, `bgmDuck`, the BGM pause and resume
around `pTimeStop`, the level-begin music start and the BGM option keep their
state but stay silent. W2's mixer must let a module player reserve voices
later.

### W3. Performance

After W1 and W2: every gameplay update of world 1 within 20 ms on the A500.
Start with the initial fall (section 0: D, then C, then E), then the rest of
world 1: dense combat, shops, side rooms and time-stop, and scene handoffs
judged by transition latency (section 5). Re-measure first; numbers recorded
before W1 and W2 will be stale. The parked candidates in section 0 (C and D
selectors) and in 1.01 (the scene collision backend) can be reused here.
New costs from W1: the spawner step runs every update (quiet-fall rose from
178 to 181 fields), and each live jelly or red ball calls `portableAtan` and
binary64 trig every update. W1.4: every shot now draws RNG (spread, speed,
image speed, three casing draws), the compact bullet store holds 32 shots,
the laser casts its ray every update in 8-pixel steps against enemies,
walls and shootables, and up to 96 laser trails are BOBs. Gun art adds
17,762 bytes to the cavern CPU bank (now 246,710) and 8,334 Chip bytes to its
plane atlas (now 117,744); the prepared actor-frame table grew from 512 to 640
entries. The spread guns' and aimed shots' art uses the nearest offline
quarter turn without the usual gun's per-angle pixel proof. W1.5: every
update tests the player and every live enemy against a y-sorted field index
(a mask query only within reach of a field), and casings, debris, moving
effects and TorchFires against `TimeStopBound` while stopped; up to 20 (store 32) binary64
time shards each run a solid query, and the compact no-loop store doubled to
32 for gem-high SHOTGUN bursts. Sprite 91 adds 1,170 Chip bytes to the cavern
plane atlas (now 118,914). `fxTimeChange` slows presentation on purpose (two
fields per update at room speed 30, plus the 30 ms sleep); judge it as
transition latency, not as frame cost. The quiet-fall witness went from 190
to 193 fields: the steady-state Begin Step, the enemy field pass and the
gem, box and fire gates. W1.6: each death now runs blood, smoke, flash and an
`objDeadBody` (up to 16 corpses, each a TimeStopBound and two mask-599 wall
queries per update, drawn on the controller effect surface), every damaging
hit spawns an impact, a bullet or casing at a wall steps up to 101 tenths
of its velocity to place its puff, and the player's dust and the empty-gun
smoke add no-loop and smoke records. `momentDelay` (30 for one update) and
player damage (40 for 40 updates) slow presentation on purpose; judge them
like `fxTimeChange`. The corpse art, spark, dust and "!" add 2,460 Chip bytes
to the cavern plane atlas (now 121,374). The quiet-fall witness went from
193 to 195 fields. W1.7: the player's pose is evaluated once per update at
the Draw boundary and shared by the draw list and Alarm 7's nine-record
afterimage ring; while gem high, up to nine extra BOBs are drawn, each with
two world RNG draws; the streak block (a 14-word, 22-row HUD stage that also erases
its previous pixels by mask) is recomposed and published on every update
that changes it, which is every update while gem
high (cap and motion frames) or while the bar line moves; `currencyText`
redraws the currency while it counts. Combo, reward and title art, the
afterimage pages and the upgrade icons add 34,968 bytes to the cavern CPU
bank (now 288,058) and 21,960 Chip bytes to its plane atlas (now 143,334);
the prepared actor-frame table grew from 640 to 1,024 entries. The
`levelNumber` title publishes to the HUD layer only when a letter appears or
moves (a third of its updates; its baked letters cost one per reveal step).
The quiet-fall witness went from 195 to 197 fields (the per-update HUD state
and pose sharing; the title starts at the landing). W1.8: each shop's speech
is a rising-text record (stepped with the notices, gated by a live count), up
to ten glyph BOBs; the four new glyphs add 310 bytes to the cavern CPU bank
(now 288,368) and 160 Chip bytes to its plane atlas (now 143,494). A full
no-loop or moving effect store now drops the new effect instead of failing
the game; the store sizes (32 and 48) are unchanged. W1.10: the per-style
player art adds 78,466 bytes to the cavern CPU bank (now 366,834) and 30,510
Chip bytes to its plane atlas (now 174,004), and the actor-frame table grew
from 1,024 to 1,408 entries (cavern 1,274, reference game 1,298). The pause
menus compose in a 27,264-byte scratch copy of the HUD layer (any memory,
allocated only while paused) with the CPU renderer and copy the changed rows,
so animating screens never show a cleared well; the pause bank is 8,818
bytes and the results bank grew to 28,672 for the unlock notice's glyphs.
The Timer publishes its 16 HUD rows on every update while shown and costs
nothing while off.

W2.3's gameplay change misses the quiet-fall ceiling: 205 fields against
197 (HEAD 194). The landing witness is unchanged, so this is pure cost, about
0.4–0.7 ms per update by an instruction trace against the previous build:
- About 700 cycles in `PlayerMotion::step`: the hard-land state, the two
  alarms, `airstatus`, the wall-pose test and the call's larger frame.
- About 2,000 cycles in `GameplayWorld::step`: the `SoundVoices` clock, the
  recharge starts and the goal-field wind gate, plus diffuse growth across
  unrelated lines.
- Compiling the world-side calls out still gives 200, and builds that differ
  only in layout vary by about ±3 fields.
- Already done: `SoundVoices` keeps live voices packed (an idle world scans
  nothing), and the wind skips its contact test while `global.voidWind`
  plays.
- Recover the gate before W3's 20 ms work, and move the player's rare-state
  bookkeeping behind a single test.

The wall-kick pose (`sprPlayerWall`, frame 0 with mirrored, DMA and
afterimage forms) takes the cavern CPU bank to 368,798 bytes (ceiling raised
to 361 KiB) and its plane atlas to 175,094 Chip bytes. The reference game
planes' ceiling is now 177 KiB.

### WR. Restructure for world 1

- Remove the `#if` tests of `REFERENCE_*` macros that nothing defines
  (`REFERENCE_ACTOR_BOUNDS_TRIG`, `REFERENCE_COLLECTION_TERRAIN_READS`,
  `REFERENCE_COLLISION_VISUAL`, `REFERENCE_JOURNAL_GEOMETRY_DECODE`,
  `REFERENCE_SOURCE_{ADMISSION,CLEANUP,ROW}_SCAN`,
  `REFERENCE_TERRAIN_ELIGIBILITY`, `REFERENCE_TYPED_TERRAIN_CLEANUP`) and the
  `TERRAIN_RUN_STORE` stub, after confirming each is dead.
- Add one world-1 manifest (areas, rooms, families, upgrades, banks) that
  drives the existing admission points (`AmigaGameplayAssetAdmission`,
  `RUNTIME_ROOMS`, `CAVERN_ROOTS`, family storage), plus a map-file gate beside
  `audit` that fails when a family outside the manifest links code.
- Park later-world code in its own directory, kept building through host
  tests: the skeleton and bone families, catacomb builder pieces, and the
  scene collision backend with its candidate `.mk` files. Cut their include
  edges from `GameplayWorld.h` and `GameplayCollisionOwner.h`.
- Move `*_VERIFY` and oracle code out of shipping sources
  (`DownwellAmiga.cpp` has 279 conditionals) into verify-only objects, keeping
  the GDB hook surface stable; group the 108 Amiga build knobs into
  shipping, verify, profile and reference files.
- Replace the stale `src/platform/amiga/README.md` with the current
  structure and feature status.

## 0. Restore the A500 initial fall: every update within 20 ms

**Part of W3 (performance); paused until W1 and W2 are done.** The target is the **no-input initial fall from the top of the well to
the first platform**, updates 1–164. The goal is that every update fits one
PAL field, 20 ms of work, so the fall takes 164 fields. This replaces the
earlier target of matching the historical 181 fields; reaching that is a
milestone on the way, not completion.

The current state is the complete trace of the
[native enemy updates](amiga-frame-cost.md#native-enemy-updates-section-0-b)
(formerly item B), on top of the
[source-record terrain owner](amiga-frame-cost.md#source-record-terrain-owner-section-0-a)
(formerly item A). It runs 176 fields for 164 updates, with a worst update of
100 ms. "Work" excludes the publication wait. The per-owner costs quoted in
C–E come from the earlier
[`45e96a9c` trace](amiga-frame-cost.md#complete-initial-fall-against-the-20-ms-budget--45e96a9c-2026-10-12)
(192 fields, worst 136 ms); re-measure before relying on a number.

| Window | Work mean ms | Worst ms (update) | Updates over 20 ms | Excess ms |
| --- | ---: | ---: | ---: | ---: |
| Authored entrance, 2–55 | 16.5 | 20.6 (19) | 1 | 1 |
| Quiet generation, 56–121 | 18.8 | 24.5 (71) | 22 | 31 |
| First burst, 122–135 | 20.2 | 35.0 (122) | 8 | 27 |
| Enemies, 136–162 | 24.3 | 64.1 (136) | 14 | 135 |
| Landing, 163–164 | 65.7 | 100.2 (163) | 2 | 91 |

This is the shipping default. Unaccepted C and D candidates exist behind
`REFERENCE_ROW_PAINTING=0` and `REFERENCE_FRAME_WORK=0` (both default `1`);
their measurements are under C and D below. Three owners account for the
remaining excess. Every update over 20 ms in 137–162 paints a row (C). The
items below remove them in order of how much over-budget work each one
removes. Each item names:

- the measured work it removes;
- its target;
- the structural change;
- its gate.

The estimates are from the trace and are not additive guarantees; re-measure
after each item.

### How to work this section

- **Measure the whole fall with the three-capture method** in the audit.
  `tools/amiga-fall-budget.py` prints the window table above.
- **Gate on all of these together:**
  - the window table: work mean, worst update, number over 20 ms and excess;
  - the specific updates each item names;
  - the `quiet-fall` pace and its landing witness.
- **Do not reject a change on pace alone** when it removes the measured work.
  Display buffering absorbs and redistributes alternating overruns. A
  better mean likewise does not excuse a worse window worst update.
- **Before writing code,** state which measured work the change removes,
  using this profile. Then compare the same windows after the change.
- **Behaviour gates:**
  - all 165 native state samples;
  - the landing witness (tick 164, y 262,144, health 4, ammo 8, 3 enemies,
    1,371 RNG draws);
  - the 18,000-frame terrain replay;
  - mutation and failure fixtures for whatever the item touches;
  - normal startup at 1 MiB Chip;
  - unchanged replay behaviour.

  Preserve source event order, RNG order, instance identities, DitherAbove
  behaviour and hardware scrolling. Keep the portable implementation as the
  oracle (`REFERENCE_*` selector) until the native path is accepted, then make
  the native path the default.
- **Re-check any surprising per-update number** with a short capture of that
  update before committing it.
- **Clean up after each item:** delete your own traces once their numbers are
  in a commit. Stop every owned emulator before handoff.

### C. Row painting and the two-update row period

**Deferred; not accepted as the default.** The preserved C/D candidate is
selected with `REFERENCE_ROW_PAINTING=0 REFERENCE_FRAME_WORK=0`; both remain
`1` by default. It uses prepared pattern rows, ordinary-memory preparation,
paint-ahead and reduced fixed-frame work, without the rejected larger asset
bank. Behavior/pixel checks have passed, but the timing gate below has not.
The existing late-fall capture still has four updates over 20 ms; exact
fence-excluded accounting gives 18.9 ms mean and 20.6 ms worst (139).
The full-fall, cadence and default-acceptance gates must be rerun on the final
candidate before closing C. Measurements and test evidence are in
[the audit](amiga-frame-cost.md#row-preparation-and-empty-family-dispatch--provisional).

- **Removes:**
  - corridor row painting, 2.85 ms per row in the quiet window;
  - cave row painting, 8.9 ms per row on average in 137–162 (`paintCached`
    5.17), with 16.6 and 17.9 ms at 141 and 143;
  - breakables drawn through the generic masked asset path (2.5 ms at 141);
  - raster invalidation of each new row's band, per-pixel-row dirty bytes
    for the painter (0.39 ms per row).

  In the quiet window, generation and painting land in the same update of
  each two-update row period (15.6 versus 24.6 ms).
- **Target:**
  - at most 4 ms to paint a row, and at most 6 ms in the worst update;
  - row work split so neither update of a period carries both generation
    and painting.
- **Approach:**
  - **Paint ahead:** paint the next row into the hidden scroll margin in the
    update without a new row. Its terrain is generated before it is visible.
  - **Prepared pattern rows:** extend the borrowed-corridor method to the
    builder's patterns. Use offline per-pattern row images, with live fix-ups
    only at segment seams, autotile neighbours and breakables. The live
    painter remains the oracle and the fallback.
- **Gate:**
  - no row update over 20 ms in 122–135 or 137–162;
  - scalar pixel comparison across all pattern rows and seams.

### D. Fixed per-update cost

**Next when section 0 resumes; not accepted as the default.** The candidate
`REFERENCE_FRAME_WORK=0` (counted destruction results, empty-alarm admission,
streak-dependent weapon selection, grouped display-row debts) takes the whole
fall from 19.6/100.2 to 18.2/98.7 ms mean/worst and 47 to 33 updates over
20 ms, but update 98 is still 14.3 ms and the entrance mean above 12 ms. All
165 state samples match; full behaviour acceptance and the cadence run are
still to do. See [the audit](amiga-frame-cost.md#fixed-frame-work-candidate--section-0-d-2026-10-18).
The quiet-update breakdown below is from `45e96a9c`; re-measure it on the
candidate before choosing the next removal.

- **Removes part of the 15.6 ms a quiet update costs with nothing to do:**
  - Step 8.8 ms;
  - draw-list collection 1.73 ms (a room-actor walk of 1.20);
  - display row sync 1.23 ms;
  - frame loop body 1.39 ms;
  - the rest.

  Step's parts include:
  - six per-update clears, 0.50 ms;
  - phase reconciliation 0.53 ms;
  - End Step interior 0.35 ms;
  - an empty alarm phase 0.28 ms;
  - enemy Step-order capture 0.24 ms;
  - weapon level recomputed every update 0.20 ms;
  - camera 0.50 ms;
  - DitherAbove 0.66–1.09 ms;
  - RNG refill 0.44 ms;
  - the builder's position publication, re-acquired every generation update
    although it is unchanged for the builder's lifetime, 0.18 ms.
- **Target:** a quiet update at or under 11 ms. This leaves room for a row's
  work and for one enemy.
- **Approach:** do work only when its inputs change. Not instruction tuning:
  - keep a retained draw list, updated at birth, death and movement, instead
    of a walk every update;
  - recompute derived values when their inputs change: the weapon level when
    the streak changes, the camera when the player moves;
  - replace per-update clears with dirty tracking;
  - skip phases with no participants entirely: alarms, Step-order capture,
    End Step interior, shop End Step.

  List every item that runs on a quiet update. Keep each one only where a
  source event requires it.
- **Gate:**
  - update 98 type at or under 11.5 ms;
  - the entrance window mean at or under 12 ms;
  - unchanged state samples.

### E. Stage generation bursts ahead of their trigger

- **What remains:** with items A and B the bursts at 122, 136 and 163 still
  cost 35.0, 64.1 and 100.2 ms of work, mostly builder and
  event-cell work (event cells about 6 and 22 ms at 122 and 136). At
  `45e96a9c` event cells at 163 were 43.3 ms:
  - breakables 16.1 ms for 21, 5,458 cycles each;
  - enemy adoption 11.4 ms, of which runner birth and binding is 8.5;
  - breakable random evaluation 5.1 ms;
  - enemy choice 3.8 ms.
- **Target:** 122, 136 and 163 each at or under 20 ms.
- **Approach:**
  1. **Cheaper event cells:** breakables publish as records with the
     source-record segment method (`TerrainBirthRecords` and a lent-counter
     lease, as terrain does), and enemy adoption joins the roster and
     record-owned appearance of the
     [native enemy updates](amiga-frame-cost.md#native-enemy-updates-section-0-b)
     instead of the generic runner birth and binding.
  2. **Stage the builder's next pattern** in slack time on earlier updates,
     once its trigger row is known: pattern choice, consumed RNG values, cell
     bytes and breakable outputs. Key the staged result on:
     - RNG draw count and state index;
     - builder state;
     - the next instance identity;
     - the occupied rows.
  3. **At the trigger update,** verify the key and commit the staged records
     in source order. Advance the RNG by the staged draw count, and run live
     only the Create events that observe the world, such as enemy Create.
  4. **On a key mismatch** (another RNG consumer or birth intervened),
     discard the staged result and run the normal path, which stays the
     oracle.
- **Depends on C and D,** which provide the slack. Do E last.
- **Gate:**
  - the three bursts at or under 20 ms;
  - a fixture that forces a key mismatch;
  - unchanged state samples and replay.

### Closed approaches (do not retry)

Measured and rejected during the earlier restoration attempts. The evidence
is in the audit and the git log.

- Shared intrusive room and lifetime descriptors, accessor tuning and
  another room-animation skip projection.
- The sparse-change room owner, room-state packing and sparse-list variants.
- Initial-state, recipe-cache, cross-row grouping and observed-room
  reconstruction variants.
- The persistent multi-row identity replacement (group formats, cursors,
  admission loops).
- Hoisted metadata or layer admission caches.
- Standalone registration-owner replacements and registry encodings.
- Fresh-cell default preinitialisation and copy-loop variants.
- Immutable-row and copy-on-write spatial encodings.

The source-record owner that completed the former item A replaced all the
per-span structures together, behind one record per segment; do not
reintroduce per-span terrain structures on the generated path.

### Other regression work (after the fall)

- Enforce scene/level-local residency and execution before adding content:
  1. Replace remaining resident
     gameplay tables/room catalogues and SDL collision masks with explicit
     scene/level dependencies. New native mask consumers
     must use those dependencies, not link the full reference mask catalogue.
     Include reachable spawns, persistent
     objects, nested Create/Draw dependencies and transition overlap; fail on
     undeclared resources rather than falling back to all assets. Apply the
     same dependency declarations to SDL texture/audio residency. Prove peak
     ordinary/Chip memory and unload/re-entry with growing unrelated content.
  2. Make phase dispatch depend on live, eligible family membership and scene
     ownership, not the catalogue of implemented families. Preserve event/RNG
     order, synchronous births, activation and captured-phase cutoffs. Assert
     inactive systems do no work; a later family must not add empty per-frame
     scans to world 1. Identify each replaced measured runner walk before
     changing it; do not simply add another full validation pass.
     Replace work at the owning phase/lifecycle boundary, not individual
     publications. Do not sweep enemy-appearance setter or validation-helper
     variants (see the rejected-approaches table).
     Use the current actor-event audit; do not start another capture/traversal
     or individual setter optimization without a structural removal.
  3. Add regression fixtures that add unrelated resources/families and verify
     unchanged current-scene residency and work counts, alongside replay and
     bounded native timing gates. Transition-only preparation must not run in
     gameplay frames. Check failure, Retry and all scene handoffs.
- After the initial fall, use the matched early contact prefix to investigate
  firing: updates 51–70 measure 23.98/34.42 ms historical mean/worst versus
  28.00/42.03 ms current. Do not use later historical frame numbers as equal
  workloads: reward Step eligibility changes RNG timing from sample 464 and
  player motion diverges at 652 (see `amiga-frame-cost.md`). Establish equivalent
  workloads before comparing later combat, generation or field windows.
  Old section 5 numbers are not a current baseline.
- Remove the measured regression at its architectural cause. Compare the same
  workload and behavior checks before/after, preserving hardware scrolling,
  offline assets and native terrain ownership. Do not resume micro-optimization
  sweeps, duplicate simulation passes or reference-only performance modes.
  For generation, profile the remaining work inside and around admitted
  whole-segment construction; it must not reintroduce generic cell constructors
  or per-cell retained-layer publication. Standalone
  registration-owner replacements are closed after two measured rejections;
  do not retry registry encodings or tune their lookup helpers. Preserve inactive
  end-of-layer cutoffs, activation, depth order and retirement; an always-active
  scenery lease alone cannot represent these semantics. Gate on current frames
  676, 1512 and 1689, plus no-input generation frame 163, as well as
  ordinary gameplay; use the current measurements in `amiga-frame-cost.md`.
  Separate the measured RNG refill cost from publication; do not count nested
  timings twice or replace these owners with arithmetic-helper tuning.
  Fresh-cell default preinitialization is closed after its measured attempt;
  do not retry initialization or copy-loop variants (see rejected approaches).
  This is not a revival of rejected
  immutable-row/copy-on-write spatial encodings. Preserve bounded run membership
  rather than replacing it with another per-cell logical lookup/publication
  path (see the rejected-approaches table). Judge the next structural change
  against the remaining generation cost and memory footprint, not an isolated
  copy-helper improvement.
- Reduce executable and static-memory growth at its owning structures: audit
  linked collision/query machinery, embedded resource/mask banks and full-height
  terrain caches. Account separately for Hunk code/data/relocations, zero-filled
  BSS and dynamic Chip allocations. Loading assets from files reduces executable
  size only; prove residency savings separately. Preserve the portable/reference
  oracle without requiring all of its machinery in the shipping native binary.
  Do not restart rejected per-cell registration encodings. Future query-heavy
  scenes must budget the full on-demand tree allocation from the memory
  contract; its absence in the current contact workload does not prove those
  scenes fit. Continue removing unrelated resident resources and code without
  moving the same always-resident bytes into startup allocations.
- Close this section only with every initial-fall update within 20 ms (items
  C–E), a normally runnable build, explained/resolved recent regressions and
  meaningful startup failure codes. Retain any 20 ms shortfall outside the
  initial fall explicitly in section 5; do not claim 50 FPS or stock total-memory fit without evidence. Restore a normal
  non-profile build and stop every owned emulator run before handoff.

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

#### 1.18 Validate the integrated A500 workload and memory
**Depends on:** 1.15–1.16.
**Work:** Run bounded native natural-encounter captures, including dense smoke,
debris, gems, fields, shops and Retry. Measure actual per-owner costs, worst
gameplay frames, Chip/Fast peaks and stack/capacity margins. Confirm 68000-only
output and meaningful allocation failures. Use offline assets and retained
hardware scrolling; no runtime sprite conversion or silent RAM expansion.
**Done when:** Reproducible measurements and coverage are recorded in the commit;
newly exposed costs and fit failures have concrete entries in §5/§7. Do not claim
20 ms or stock-A500 fit unless the measurements prove them. Finishing §1's
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

## 5. Meet the Amiga frame budget

Target <=20 ms frames during active gameplay and smooth style/upgrade navigation.
Scene handoffs are not gameplay frames: judge them by total transition latency
and visible stalls, not a 20 ms per-frame limit. Average gameplay FPS alone is
not the criterion. Use the current regression baseline and exact comparison
windows in the [frame-cost audit](amiga-frame-cost.md), not its historical
`13e08ca2` section. Source Step-order changes altered the contact tape's
trajectory; frame 701 is no longer its largest terrain burst. Finish section 0
before resuming the historical ranked items below, re-estimating their costs
against the live workload. Extend coverage to quiet gameplay and scene handoffs
rather than treating the captured generation and contact
windows as a complete game audit. Measure the cold gameplay handoff separately
with `profile.sh entry`.

### How to work this section

An earlier round spent most of its commits on one parked item (the ammo
gauge) and left ten opt-in flags behind. Do not repeat that.

- Work the items below in order. Each names the structure to remove, its
  measured cost and a target. Implement the whole structural change in one
  attempt; do not sweep variants, modes or parameters.
- Do not touch code that costs under 1 ms in the windows it runs in. That
  includes loop rewrites, type/register changes, unrolling, inline assembly,
  compiler flags and helper substitutions. If a profile shows nothing of 3 ms
  or more in the target, the item is done: move on.
- Before starting, write in the commit message the work removed and the
  expected saving from the numbers here. Afterwards trace only the affected
  windows (start a capture one frame early: 644, 659, 699, 843) and run the
  FPS tape. Repeat the full audit only after each lettered group lands.
- Gate on the frames the budget is about, not on every frame. Use the current
  audit's frame numbers and coverage. Accept when
  the targeted cost falls, no measured budget frame or window
  worst frame gets slower by more than 0.5 ms, and no window mean the change
  runs in gets worse. Another single frame may rise by up to about 1 ms when
  the change saves several times that elsewhere; report it in the commit.
  Untouched frames move by up to about 0.5 ms between builds; that is noise.
  The tape's worst gap is quantized to whole fields: report it, never gate on
  it. Do not reject an 11 ms saving for a 0.6 ms move in a 70 ms frame.
- For scene handoffs, measure the complete transition through the first
  interactive frame, including preparation charged to earlier frames. Small
  differences such as 8 ms or one or two PAL fields are not rejection grounds
  by themselves. Accept a substantial overall improvement even when VBlank
  waiting shifts into the following frame. Preserve smooth ongoing animation
  and gameplay; distinguish added computation from redistributed waiting.
  Saving estimates and attempt limits guide prioritization, not automatic
  rejection of useful changes. Reassess candidates blocked solely by an
  obsolete timing gate without starting another parameter/variant sweep.
- Quote frame numbers from the report's per-frame table for the frame you
  name, and re-check a surprising number with a short capture of that frame
  before committing it. The `36d8a0f2` message gives 660 as 84.5 ms; the
  frame measures 90.2 ms (a saving of about 16 ms, not 22).
- An accepted change is the default in the same commit. Keep the replaced path
  only as a reference selector for oracles. Do not add experiment flags, mode
  numbers or parallel candidate implementations to the tree.
- Two rejected attempts at an item end it: record one row each in the audit's
  [rejected approaches](amiga-frame-cost.md#rejected-approaches) table (commit,
  and the removed code as a patch in `docs/rejected/`), then take the next
  item. Measurements belong in commit messages. Do not add narrative or
  per-experiment results to this file or the audit.
- Verify each change with the full RNG state, numeric instance IDs, creation
  and event order, positions and draw order on deterministic replays
  (`make verify-cross-replay`, `make verify-terrain-ownership`, the per-frame
  ownership digests, and `profile.sh overflow-state` for that encounter), plus
  the native pixel oracles (`profile.sh terrain`,
  `profile.sh fields`, `profile.sh ammo`, native frame dumps against the
  reference selector) when presentation changes.
- Delete your own traces under `tmp/trace/` once their numbers are in a commit;
  each 100-field capture is about 500 MB. For several queries on one capture,
  export `--collapsed` stacks once instead of re-running `--focus`.

### A. Generation bursts (frames 701 and 844)

`generateLiveTerrain` costs 40.84 ms at 701 and 35.86 ms at 844. A whole
builder pattern is emitted synchronously, as in the original; the draws and
creations must stay in that order. Further changes must remove remaining
publication or Create machinery, not reorder those effects.

**Remaining budget shortfall, not another encoding sweep.** Generation at
701 still exceeds the 30 ms target. Event cells take 16.92 ms and prepared
terrain publication 13.92 ms (nested within generation). At 844 these are
19.81 and 6.87 ms. The next generation attempt must eliminate a named
remaining structure, not retune the prepared row program or its helpers.

   - Further actor/time-field publication work must replace shared ownership
     as a whole; individual lifetime helpers are below the work threshold.
     Do not retune the generated-box command path. One enemy configuration
     at 844 costs 3.76 ms, including 3.23 ms in lifecycle publication.
     Establish the removable structure and saving
     before changing it; charge mutation costs in the affected-frame gate.
     Do not tune `nextU32`, constructors or small choice helpers individually.
   - Fixed-terrain publication remains a shortfall, but both immutable-row
     owner attempts are closed (see the rejection table). Do not retry
     copy-on-write rows, sparse overrides, prepared row encodings, copy loops
     or individual publication helpers. This owner item has exhausted its
     two attempts; proceed to the other shared ownership/composition work.
     No deferred source-visible Create or later terrain exposure is permitted.
   - Preserve source-ordered terrain segments separated by event cells: never
     expose later terrain to an earlier Create. Retain the scalar reference
     for registered terrain, instance IDs, and capacity/failure prefixes.
   - Keep coincident-wall counts, landing candidates, platform record order,
     dirty rows, RNG interleaving and the optional pillar tail exact.
   - Recheck full-state/ownership replay and native terrain pixel oracles.
     `REFERENCE_PATTERNS=1` selects the retained cell interpreter.
   - Native breakables cost about 2,058 cycles per creation at 844 (thirteen, 3.77
     ms), still above 1,500. Do not tune individual writes or copy loops for
     this threshold. Keep `REFERENCE_BREAKABLES=1` as the oracle, including
     room re-entry and failure prefixes. See the
     [allocation contract](terrain-update-contract.md#native-breakable-allocation).

### B. Contact damage number (frames 645 and 646)

The remaining contact-pair damage-number work is 10.85 ms, above its
10 ms target. At 645, creation costs 5.29 ms. The retained post-difference
direction calculation is below 3 ms. At 646, first Step costs 5.56 ms, including
3.31 ms in the certified motion decoder and wrappers. Subsequent motion and
damping cost about 2.1 ms per frame; leave their individual helpers alone.
Preserve bit-identical direction, speed and positions and keep frame 646
under 80 ms while removing remaining threshold-sized computations.

- Keep the [finite-domain certificate](runner-point-direction.md#lossless-contact-motion-representation)
  and portable cosine oracle. `REFERENCE_CONTACT_MOTION=1` selects the retained
  first-Step program. Mutated, arbitrary and fallback directions must not be
  silently admitted to the certified domain.
- A further change must remove a named remaining computation across the
  contact adapter/first Step, not tune sub-millisecond decoder or arithmetic
  helpers. Do not substitute trigonometric identities or reduced precision.
- A faster generic binary64 multiply is a helper substitution, not this item.
- Keep exact Q8 coordinate admission and the full-wrapper fallback
  (`REFERENCE_CONTACT_COORDINATES=1`). Do not tune the sub-millisecond
  coordinate adapter or the now-under-3-ms direction calculation.

### C. Combat composition shortfall

Remaining effect work is the raster fallback for mixed producers, actual
shade subtraction and entry cover. Measure those
cases separately from eligible gem-only frames before another compositor
attempt. Use the effect-coverage census described in the profiling procedure
to establish that the chosen window actually exercises each proposed path;
producer-phase timings alone do not prove smoke/debris or raster coverage.
Use the seed-87 route-34 tape for shade subtraction and channel overflow. Preserve
the unshaded actor/BOB pass, topmost unshaded hardware suffix, shaded raster prefix, and
controller/ordinary-actor boundary when changing
the retained raster. Do not tune sub-millisecond stamp helpers. Mixed producers and entry cover still
need their own native workload and pixel evidence.
Current combat draw collection costs 4.48 ms and row synchronization 1.50 ms.
The supplied baseline's raster preparation/publication costs no longer describe
this hardware-gem-only workload; do not use them to justify fallback changes.
Review the complete producer/compositor/consumer path, preserving source
painter order, opaque-black replacement and shade subtraction. The two
rejected effect approaches remain closed: a new proposal must remove their
exact-query or CPU source/mask-extraction cost, not repeat either layout.
The delta-register stamp batch is also closed: it shifts work into queued
dispatch and regresses the first affected frame. Do not sweep its scheduling.
Offline shifted CPU stamps into the retained raster are closed too; do not
retune their word interpreter, layout or CPU/DMA selection.
Row-debt representation has exhausted its two attempts: disjoint ranges and
shared word coverage with cached field/laser footprints are closed. Do not
retune their range layout, bit scans or footprint maintenance. Proceed to the
remaining producer/compositor/consumer structures, not another debt encoding.
Keep per-producer/presentation state and final pixels as oracles. Do not
reopen small sprite assignment or gem-position helpers.
Keep `REFERENCE_EFFECT_COMPOSITION=1` as the separate-plane oracle. Further
composition work must remove a threshold-sized structure, not tune stamp
lookup, row geometry or the sub-millisecond bitmap setup. Preserve the
retained alpha raster's exact queries and uncommon-mask fallback.

### E. Quiet-frame shortfall

Pursue shared quiet-frame work, not another enemy alarm-order variant. Do not
retune roster insertion, pinning, snapshot caching, merge loops or negative
counter dispatch. The rejected borrowed Step snapshot and generic temporary
alarm-array layouts remain closed. Any remaining effect-alarm proposal must
first meet the work threshold in a workload that actually exercises it.

Target: quiet frames under 22 ms; current mean is 26.7 ms. Player and walking
enemy motion cost about 5.6 ms together, but each is under 3 ms in this window.
Leave their individual helpers alone. A quiet-frame proposal needs a shared
work structure to remove, measured across its callers, not a collection of
unrelated sub-millisecond edits.

### F. Parked until A–E are done

- Ammo gauge: 11.29 ms at 645 and 10.71 ms at 646, 1.5–4 ms in typical frames. Every
  runtime classifier is ruled out (rejected-approaches table). If reopened,
  start from the staged-transfer patch listed there, measured against the
  factored default.
- Health HUD at 645 (4.51 ms); terrain painting at 844 (6.41 ms against 5 ms).

### G. Keep native ownership sufficient

- Native ownership rejects every collision query and wall End Step. When an
  original query or wall event enters native gameplay, answer it from native
  state (the grid for terrain selectors, family records for actors) with the
  same first-hit identity and order, verified against the registered
  reference. Do not re-enable per-instance spatial publication for it.
- Actors at the wall depth (100) are rejected, because an inactive wall ends
  that layer's Draw. Supporting one needs the layer's wall-order cutoff.

### H. Coverage

Add a dense-encounter tape before claiming the budget. Include the natural
two-level horizontal shop tape (`profile.sh shop`), not only the contact
tape, in subsequent audits. Repeat the audit after each lettered group lands.
Particle-heavy natural coverage depends on the destruction/effect integration
in §1: manually emitted smoke/debris fixtures or a bullet-only census do not
prove that coverage. Keep the input-only seeded search's source/runtime parity
checks enabled when selecting a tape.

### I. Other scenes

The earlier style-reveal-only tape peaks at two fields (splash 34.7 ms,
garden 40.5 ms, style
34.5 ms, instrumented). Reduce upgrade-selection layout/content and transition
preparation costs, keeping static layers and pointer-driven movement. Reduce
animated overlay and offer-bank preparation stalls.
Measure asset staging and Chip/Fast residency separately from steady-state navigation.
Preserve the original heading, icon and carousel animation.

Reduce the overall cold gameplay handoff exposed by `profile.sh entry`.
Measure the last style frame and first gameplay frame together under the
transition gate above; do not discard either. Remaining synchronous work includes
initial terrain raster and backing/HUD
setup. Move immutable preparation out of confirmation without overwriting
the garden/style display owners or introducing another long frame earlier.
Do not move RNG draws or source-visible Create effects ahead of confirmation.
Do not retry bounded clean-style raster preparation or sweep row budgets,
scheduling, dictionary gather loops or encodings. Proceed to remaining
backing/HUD setup and room publication structures; do not count relocated
copy work as eliminated work.

Reduce remaining mutable room-layer reset/publication at level reentry,
measured with the natural shop tape (`PROBES=1 bash amiga/profile.sh shop`).
Keep immutable metadata admission and the no-survivor proof outside repeated
placeholder searches. A further attempt must remove a measured structure,
not tune reset loops. Preserve serialized placeholders, source IDs, background
initialization order, failure prefixes and later actor attachment; retain
the generic layer lifecycle as the oracle (`REFERENCE_ROOM_PREPARE=1`).
Mutation-owned occupancy row extents are rejected: tracking during room
publication consumes most of the removed reset work. Do not tune their range
updates or partial-clear loops; pursue a different ownership structure.

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

- Measure Chip/Fast RAM, task stack, asset residency and peak transition usage.
  Establish the A500 memory requirement; expanded-emulator runs do not prove
  stock-machine fit. Size the shop speech overlap reserve against repeated
  messages within the nested parent Step, not just one replacement per parent.
- Budget further Chip residency against the measured 20,272-byte cavern
  minimum free space (126,840 bytes before gameplay); remeasure after pack
  growth and through scene transitions. The cavern DMA bank is 109,410 bytes;
  the verification-only full union remains 115,806 bytes. Reconcile gameplay-family
  residency before admitting additional encounter artwork on a stock-memory machine.
  Keep the gate unchanged until residency is reduced or a new whole-program
  allocation measurement justifies a different budget.
  Keep the 68000 CPU target; do not silently increase emulator Chip RAM
  to make memory checks pass.
- The effect stamps are still linked into the
  executable, so ordinary memory holds them beside any Chip copy. Give them
  scene/level dependency contracts and bounded file residency; merely moving
  every bank to a permanently loaded sidecar does not reduce memory.
- Exercise allocation/capacity failures and scene teardown/re-entry. Preserve
  meaningful nonzero exit codes for startup failures.
- Propagate terminal failures through the remaining direct actor, reward and
  collision loops: stop later work in the same frame without rolling back
  completed effects. Checked event dispatch alone does not cover these loops.
- Maintain reproducible offline assets and 68000 instruction/helper checks
  across supported build configurations.

## 8. Broaden end-to-end verification

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
- Prepare release packaging, compatibility tests and installation documentation
  for the measured CPU/memory requirements, without bundling copyrighted inputs.

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
