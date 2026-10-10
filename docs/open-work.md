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

Immediate priority: restore the runnable, measured A500 baseline in section 0.
Do not add further section 1 functionality while this regression is unresolved.
After that, finish live skeleton encounters (section 1), following its numbered
integration tasks: production ownership, generation/query/dispatch, time-stop
and damage chains, then natural encounter verification. The A500 frame-budget work in
section 5 remains open; use its measured architectural approach rather than
tuning small functions or compiler flags. Then make catacombs playable
(section 2), followed by the remaining content (section 4).
Apply the fidelity, performance, architecture, memory and
verification requirements in sections 3 and 5–8 to each integration; they are
not deferred cleanup phases.

Supporting helpers alone do not close an item. Completion requires the shared
behavior, both backend integrations and relevant original-game verification.

## 0. Restore a runnable, measured A500 baseline

- Measure transient peak allocations and free/largest Chip blocks during bank
  loads at explicit 512 KiB Chip / 8 MiB Fast, 68000 settings. Keep startup,
  gameplay and results/Retry residency checks as regression gates; successful
  handoffs and the initial allocation balance do not measure transient peaks.
  Do not silently raise verification memory or discard source behavior.
- Extend the bounded SKIP_TO_GAME contact audit with a comparable pre-regression
  workload; distinguish behavior/input-path changes from increased per-frame work.
  Report quiet, firing/combat, generation and field windows with worst-frame
  costs and coverage. Old section 5 numbers are not a current baseline.
- Remove the measured regression at its architectural cause. Compare the same
  workload and behavior checks before/after, preserving hardware scrolling,
  offline assets and native terrain ownership. Do not resume micro-optimization
  sweeps, duplicate simulation passes or reference-only performance modes.
  For generation, replace per-cell room/layer attachment in the native terrain
  path with run-owned chronology and layer membership. Preserve inactive
  end-of-layer cutoffs, activation, depth order and retirement; an always-active
  scenery lease alone cannot represent these semantics. Gate on current frames
  676 and 1689 as well as ordinary gameplay. This is not a revival of rejected
  immutable-row/copy-on-write spatial encodings. Publish bounded run membership
  once rather than replacing physical attachment with another per-cell logical
  lookup/publication path (see the rejected-approaches table).
- Reduce executable and static-memory growth at its owning structures: audit
  linked collision/query machinery, embedded resource/mask banks and full-height
  terrain caches. Account separately for Hunk code/data/relocations, zero-filled
  BSS and dynamic Chip allocations. Loading assets from files reduces executable
  size only; prove residency savings separately. Preserve the portable/reference
  oracle without requiring all of its machinery in the shipping native binary.
- Close this goal only with a normally runnable build, repeatable measured
  baseline, explained/resolved recent regressions and meaningful startup failure
  codes. Retain any remaining absolute 20 ms shortfall explicitly in section 5;
  do not claim 50 FPS or stock total-memory fit without evidence. Restore a normal
  non-profile build and stop every owned emulator run before handoff.

## 1. Finish live skeleton and bone integration

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
- `src/main.cpp` installs `StoredWorldCollision` and the narrower
  `StoredDitherCollisionPass`; it does not install the registered skeleton or
  Shop event environments around normal update/render.
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
**Work:** Give each frontend a session-lifetime owner for registered event
environments, rather than assembling them only inside tests. Reuse the shared
core APIs; platform code supplies devices, assets and presentation only.
Define one update boundary and one Draw boundary that install the required
scopes, preserve nested callbacks, and propagate terminal failure. Wire settings,
shared input consumption, language/font state, tile/view reads and real audio
interfaces. SDL currently loads only wind sounds 349–355; load the actual
resources required by each admitted event before enabling it. Define equivalent
Amiga audio/resource bindings, not dummy successful callbacks.

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

- Connect recovered patterns and symbol dispatch to both live generators,
  including synchronous actor creation and builder state.
- Implement skulls, spiders and remaining catacomb hazards: source-specific
  motion, collisions, alarms, animation and artwork.
- Implement area transition/entry/exit and verify natural cavern-to-catacomb
  progression. Do not substitute cavern actors or patterns.

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
Use the seed-8 route-4 tape for shade subtraction and channel overflow. Preserve
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
- Budget further Chip residency against the measured 928-byte startup
  headroom; remeasure after pack growth and through scene transitions.
  Restore the renderer's 120,000-byte resident-art gate: the current pack
  requires 169,294 bytes. Reconcile gameplay-family residency before admitting
  the additional encounter artwork on a stock-memory machine.
  Keep the gate unchanged until residency is reduced or a new whole-program
  allocation measurement justifies a different budget.
  Keep the 68000 CPU target; do not silently increase emulator Chip RAM
  to make memory checks pass.
- The gameplay, HUD, results, shop and clear packs and the cover masks are
  still linked into the executable, so ordinary memory holds them beside any
  Chip copy. Load them as per-scene files like the boot and front banks, one
  whole-file read per scene, and keep the startup failure codes meaningful.
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
relevant verification are complete. Record implementation and test results in
the cohesive commit, not a second status document or a completed-items section.
Keep format specifications, source audits and reproducible test instructions in
the linked technical references; keep priorities and remaining work here.

Use `SKIP_TO_GAME=1` for gameplay, `SKIP_LEVEL=1` for completion and `SKIP_INTRO=1`
for style tests. Test full startup when scene/display ownership changes.
Time-limit emulator runs, stop owned sessions and restore normal builds.
Never modify `data/` or commit copyrighted exports/generated payloads.
