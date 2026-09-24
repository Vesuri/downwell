# Current completion checklist

The Windows Steam game is the oracle. Port-to-port agreement proves neither
complete feature coverage nor original-game equivalence. This is the concise
working checklist; `feature-parity.md` contains chronological evidence and
superseded findings.

## Implemented and regression-tested subset

- Startup logos/credits/controls, garden/well transition, style-selection route.
- First-area generation, hardware-scrolled terrain, player, supported enemies,
  default gun, dropped gems, damage and basic combat.
- Shared run counters, combo grants, HUD, death/results and restart destinations.
- Selected exact SDL/Amiga image comparisons and deterministic combat tapes.
- Offline planar/sprite assets, 68000 instruction checks, bounded A500 emulator
  tests. Test configuration includes Fast RAM; stock-A500 memory fit is not proved.

These are coverage statements, not completed Windows-fidelity sign-offs.

## Next playable-system milestone: shops

- [x] Recover consumable stock selection and purchase order from VM assembly.
- [x] Shared stock/pricing/transaction core and focused regression tests.
- [x] Apply queued consumables in the original controller phase on both runtimes.
- [x] Shared shop interaction and feedback requests for registered shops.
- [x] Shared shop world ownership and registered-shop input/purchase hooks in
  both runtimes, verified with an isolated paired fixture.
- [x] Retained shared merchant feedback and purchased-item disappearance state
  (merchant audio/speech consumers still pending).
- [x] Table, glass-case and invisible-wall player collision masks shared by
  both adapters, including separate retirement rules.
- [x] Shared static counter/table/case drawing and offline planar fixtures;
  Amiga composition uses the terrain cache, not recurring furniture redraws.
- [x] Cased consumable icons (original frame 1), purchase disappearance and
  changed-row cache refresh.
- [x] Shared merchant Step/alarm controller and animation phase in both
  runtimes; enemy replacement remains pending.
- [x] Merchant moving-object drawing on SDL and Amiga, offline planar/DMA
  frames, and existing sprite allocation/BOB fallback (not terrain redraw).
- [x] Shared shop menu layout, extracted text and both rendering adapters;
  native dirty-region composition and banked offline art.
- [x] Shared interaction prompt state and drawing, including offline DMA art.
- [x] Configurable cavern stage/style shop generation and synchronous Shop
  creation dispatch; seeded cross-adapter generation tests include stock RNG.
- [x] Seven target retained-blitter/fresh-scalar shop/streak comparisons inside
  the bounded 600-tick terrain gate. This is a renderer fixture, not gameplay.
- [ ] Live shop instances and entry/exit/control locking on both platforms.
- [ ] Shared menu interaction, prices, sold cases and merchant feedback.
- [ ] Deterministic purchases: insufficient funds, sold out, each item, exit/reentry.
- [ ] Bounded target capture/replay and original-game validation.

Ordinary 1-1
marks its shop as already made; reaching shops naturally also needs subsequent
level progression. Do not silently change the original first-level generator
to make a shop appear for testing.
Explicit registration drives interaction, child state and rendering in both
runtimes. Builder callbacks register emitted shops synchronously, but normal
gameplay still uses ordinary 1-1 setup: it cannot yet progress to later stages
or select an implemented alternative style that naturally generates a shop.
Fixtures are not proof of a complete shop or Windows event-order equivalence.

### Next integration path (source audited)

Implement the original level-end/result flow rather than calling RunStage.advance
directly on a depth threshold. objLineToResult collision, when alive and not
already active, sets alarm 0 to five, goalStop and noControl. Alarm 0 creates
objTransDither and sets alarm 1 to 90. Alarm 1 enters rmResult; Result alarm 1
later calls scrNextLevel(0). The trigger's Create also creates goalTimeField at
y+32. Transition presentation and result/upgrade behavior remain unported.
Stage advancement alone would skip visible behavior and RNG-consuming events.

`LevelExit.h` now models inherited/derived child creation order, the guarded
collision latch and both alarm events. Its focused test checks trigger anchors,
dead-player exclusion, repeated collisions, fifth-tick dither creation and
result-room request ninety further alarm phases later. Live collision is now
connected; the playable result room remains a prerequisite for advancing levels.
Alarm requests alone are not a completed level exit.

The resource audit also found objLineToResult (306) and its parent marked
Visible=false. Both drawing adapters now omit its sprite 127: the Amiga terrain
collector no longer adds it as decoration or invalidates artwork rows for it.
Generated Create commands are retained for collision/transition integration.
Host rendering tests verify the original resource flag and omit drawing an
active trigger; native generation tests retain command-order checks while
excluding the trigger from the decoration stream.

The generated exit now also sets the shared camera's roomEnd to triggerY-176
on both live creation paths, reproducing roomEndSetter Create. camMain's cap
is applied to the target before easing, and to current Y only inside that same
target-exceeded branch. Reset clears the limit to its original -1 sentinel.
Focused tests cover easing, overshoot, retained death targets and reset; the
paired runtime regression passes.

`GoalTimeField.h` now models the child's Create radius and Draw outline requests
from VM assembly. Radius is exactly 128 + uint32/2^31, represented without target
floating point. Draw consumes RNG before the strict camera-bound check, except
while pTimeStop is active. The focused test compares 10,000 Draw events against
runner binary64 arithmetic, including all three outline radii/order and complete
RNG state. Its Draw event awaits live dispatch and backend circle rendering;
it does not yet implement field collision, wind audio,
time-stop effects or the result room. Repainting cached artwork must not repeat
the original Draw event's random consumption.

Both live builder adapters now call shared RunSession exit creation instead of
only setting the camera limit. This retains the trigger, applies its inherited
camera child first, then creates the goal field at y+32 and consumes one RNG
draw synchronously. New-run reset clears the trigger/alarms/field. A paired
runtime fixture verifies anchors, matching radius and draw counts before shop
creation; a host regression checks the camera limit and reset. SDL and native
68000 builds pass. This proves the Create path, not Draw dispatch or level
progression.

Shared PlayerMotion now accepts the original goalStop distinction: it skips
ordinary X/Y integration without skipping gravity, movement processing or
collision correction. Its existing collision adapter remains an approximation
of scrWallCol, not a newly completed exact collision port. Focused tests check
stationary position with advancing velocities/callbacks, resumed motion, and
predicted landing correction while stopped.

Both runtimes now dispatch collision against the exit's recovered rectangular
sprTouchWide mask using their current rounded player-mask adapter. A living
collision latches goalStop/noControl, suppresses subsequent position integration
and run-clock increments, and arms the shared alarm sequence. Sticky requests
record dither on tick 5 and the result room on tick 95. The result room loader
does not consume its request yet. Consequently reaching the live exit currently
stops the player without progressing. A paired fixture covers 100 subsequent updates
with held movement/fire, unchanged position/time, matching alarm requests and
new-run reset. Exact trigger-edge contacts have focused tests; full fractional
collision/event ordering and original Windows captures remain open.
The bounded SKIP_TO_GAME native cache gate still passes 600 simulation ticks,
101 fresh-render comparisons and the 256-byte task-stack margin with this
integration. That gate validates existing gameplay rendering, not reaching the
level exit on target; exit timing is covered by the paired host runtime fixture.

The tick-five request now creates shared ExitDither state. The recovered Create
anchors x=viewX+80, y=viewY-100 and arms alarm 0 for 15; ordinary Step advances
y by eight while below the current view bottom, without re-anchoring or clamping
overshoot. Both adapters advance this state and latch its source sound request
(187, priority 80, UI; no playback yet). SDL draws sprite 128 after the HUD,
clipped to the 160-pixel room surface. Focused/paired tests cover moving camera,
sound timing, matching world coordinates and repeat-render independence.
Amiga now copies a host-generated 384x648 single-plane mask into the retired
garden artwork bank once, then patches its DMA pointers. It temporarily enables
BPL5/6 with that same mask: both playfields' high pens are black and PF2 keeps
its existing HUD/sprite priority. This preserves the circular terrain buffers;
the world is not repainted for the wipe. Ordinary gameplay remains four planes.
The bank is pinned against shop/results replacement while the mask is in use,
and scene initialization releases that ownership after the prior scene retires.
The 31,104-byte mask fits the existing 32,140-byte bank with no new Chip allocation.
Every mask bit is tested against the recovered edge; all 284-line DMA windows
are bounded. Its ignored manifest records source/extractor and output hashes.
The bounded SKIP_TO_GAME exit fixture presents 48 wipe frames on A500 and checks
both live copper pointers, six-plane mode and the bank's bytes through full
coverage. It is not a Windows visual comparison, natural full-level playthrough,
or a 20ms performance claim. Result-room loading remains unimplemented.

The result-room prerequisite `LevelUpgradeOffers.h` now reproduces
resultRandomizeUg from VM assembly: inclusive slot/ID bounds, upgrade-cap
eligibility, duplicate rejection, 61 rejected rolls per slot, fallback 100 in
the first three slots, and the source's unchanged fourth slot on exhaustion.
Focused tests cover exhausted/single-candidate pools, final-attempt acceptance,
and 256 seeded comparisons of offered IDs plus full runner RNG state. This
component is not yet connected to a live level-clear menu; selecting upgrades,
room rendering and progression
remain work. It does not restrict the original upgrade pool to implemented
effects merely to make a menu usable.

`LevelClearSelection.h` models Result's default selection path: alarm 4 at 60
updates enters next=4, initial offers arm a 30-update confirmation lock, separate
right/left edges remain enabled during that lock, and a grant precedes feedback
and decrement/reroll. The final grant hides choices; the following Step arms
the 30-update departure alarm, followed by the source 90-update dither wait.
Tests exercise held confirm, simultaneous direction edges, additional awards,
zero-upgrade departure, event ordering and complete alarm timing. This is not
yet wired into either runtime's room loading. Original heart-goal application,
ongoing upgrade effects, presentation easing and result-room rendering remain
required; the selection model does not replace them with direct stage advance.

RunSession now owns a shared UpgradeInventory, reset at new-run boundaries.
Normal IDs 0..22 retain source caps: 2, 19 and 21 have cap zero; the other twenty
have cap one. Grants retain one-based acquisition order, duplicate currency
refunds, immediate Youth/Apple healing (also after a refund), Super Charge's
charge/refill, and Fortune's direct currency/gameGem award without pickup streak
side effects. IDs 100..105 still queue the existing consumables. Both runtime
adapters expose this same grant path. Focused tests cover caps, refunds,
overflow healing and exhausted offers; paired fixtures compare native/host
grants and host rendering tests check reset. Persistent combat modifiers,
effects/sound playback and the live level-clear menu are not implemented by
inventory ownership alone and remain open.

`LevelClearController.h` now composes the selection, offer sampler and real
inventory grant path. Its Create refills charge, consumes Result's gemSp random
draw, snapshots the heart-goal state and fixes the option count from style/Youth.
Step generates offers from current ownership and applies grants synchronously.
The source's repeated heart-goal health assignment is preserved even on the
update after Apple healing. Tests cover that ordering, full Create/offer RNG
state, style option counts and the complete 211-update request sequence.
This controller still awaits live room integration; presentation/easing, sound
playback and its conditional wind-sample RNG, achievement/loading effects and
next-room construction remain outside this tested subset.

The offline `clear` asset profile now exports frame-zero icons for IDs 0..22
and the rice-ball fallback, localized names/descriptions and the clear/choose
labels. Centered multiline text retains top anchors and langString's single-
space fallback for absent keys. Its 43,052-byte pack has 74 records, generated
host-label lookup and input/tool/output provenance; all generated content stays
ignored. Every icon/text raster is compared to extracted source pixels/glyphs.
The full pack is too large for the existing artwork bank, but four distinct
offer bundles plus shared labels have a conservative 11,564-byte bound inside
the 32,140-byte bank. Selected-resource copying and room rendering are still
pending; no additional resident Chip assets have been introduced.

`AmigaClearBank.h` now constructs a bounded DWAP bank for up to four offered
upgrades plus the clear/choose labels. It copies already-planar bytes, preserves
resource IDs/origins/provenance and deduplicates repeated fallback bundles.
All selected records, payload ranges and capacity are validated before writing,
so a failed replacement cannot corrupt the previous bank. Tests cover 256 offer
sets, byte-for-byte payload equality, adjacent-memory guards, malformed offsets,
undersized buffers and duplicates; the largest sampled bank is 9,464 bytes.
This helper is not yet called by the live level-clear renderer. Its eventual
caller must retire the exit-mask DMA and drain the blitter before reusing that
same bank; passing the bounds test does not establish safe scene ownership.

## Still open beyond that milestone

Shared LevelClearMotion now implements Result's player/camera easing and
departure acceleration. Showing choices changes targets before that same
update's easing; entering next=7 does not accelerate until the following Step.
HudReal's small-divisor operation preserves binary64 rounding without target
floating point. Tests compare 20,000 divisions and 10,000 motion updates with
host binary64, and controller tests cover transition ordering and room reset.
A freestanding -m68000 compile probe has no undefined arithmetic helpers.
This is not a frame-time measurement: live room loading/rendering still needs
integration.

LevelClearCarousel now preserves signed option positions and the source's
subtract/divide-by-five/add rounding sequence. The shared controller initializes
it from the style-dependent cursor and updates it after navigation/confirmation,
including the last confirmation frame that hides the menu. Hidden/departing
frames leave it unchanged. Tests compare 20,000 direction-changing updates and
all four option X coordinates against binary64, plus controller ordering.
A freestanding 68000 compile probe has no external arithmetic helpers. This
does not yet render the options or implement their Draw-event vertical bobbing.

RunSession now owns the level-clear controller and exposes explicit entry,
Step and next-room consumption boundaries. Entry requires the live exit request,
consumes Result Create once and retires old exit state without resetting run
ownership. Its departure alarm starts a room-local dither; the final request is
consumed once to apply scrNextLevel(0), including area rollover. A 211-update
session test checks timing, guards, preserved upgrades/records and new-run reset.
The native build passes its 68000 instruction/helper audits. These boundaries
are not yet invoked by either live room loader: backends must first retire old
DMA/artwork ownership and implement the result-room presentation. Wind audio
and its conditional RNG consumption remain unimplemented.

LevelClearPresentation now snapshots the stateful border portion of Result Draw:
both inclusive boundary rows, incoming-area inset, six-pixel scrolling (three
in area 3), and level-three border replacement after next>2. Shadow and choice
animation clocks preserve reveal/navigation resets and hidden-choice behavior.
RunSession advances this once per result update and retains a replayable frame;
rendering the same snapshot never advances clocks or scrolling. Tests cover
500 frames for every area/level combination and repeated command emission.
This supplies border commands and clocks, not completed backend rendering,
trigonometric text shadows/icon bobbing, or player animation.

The clear art profile now also exports frames 7/13 of all five current/incoming
area border resources from scrInitialize's levelTile table. This expands the
ignored pack to 84 records / 44,828 bytes (SHA-256
`a4a5b3266eb7c319d269dfbc75689bbfa9cdb94c6f87c501f30dc403e7970411`).
AmigaClearBank selects only the requested area's and its successor's four
border records, preserving frame IDs as well as resource IDs. The revised
four-offer-plus-borders upper bound is 12,620 bytes within the existing
32,140-byte bank; the largest of 256 sampled banks is 10,520 bytes. Every
exported border pixel is compared with the extracted texture, and bank tests
check all four area pairs, payload equality, invalid areas and untouched guards.
This supersedes the earlier 74-record and 11,564-byte bounds above. The bank
still awaits live scene/DMA ownership integration.

Both runtime adapters now expose the same explicit result entry/update path,
using their actual player, health, inventory, currency and RNG state. Entry
refreshes the SDL ammo mirror after Result Create's refill. Their ordinary
gameplay dispatchers refuse to advance while result ownership is active;
the result update advances the runtime tick without running enemies/builders.
The paired scene replay now continues from its exit fixture through 211 result
updates with navigation and confirmation, comparing effects, offers, all RNG
words, motion, grants and departure requests. This is adapter integration,
not an automatic live scene switch: the application presenters still need to
invoke entry after retiring the old display and then render these snapshots.

SDL Game now invokes that entry automatically on the live exit request and
routes subsequent input to the result controller instead of gameplay. A shared
LevelClearLayout emits borders, player anchor, six red/one white title passes,
upgrade icon positions/clamping and selected name/description anchors. It uses
integer-only binary64 arithmetic after receiving trigonometric samples; SDL
currently supplies those samples with host sin/cos and resolves localized text.
The scene suppresses gameplay/HUD rendering, handles left/right edges and draws
the departure wipe in result coordinates. Entry also prepares the first Draw
snapshot without prematurely advancing alarms. Live Game tests reach selection,
check both title stacks, held-input navigation and repeated-render independence;
focused layout tests cover source order, icon bob/clamp and owned-name suffixes.
The normal falling player pose is retained from gameplay; full inherited player
animation and optional bonus/heart Draw branches remain unaudited here. Audio
and goal-field heart detection are still absent. Next-level requests still wait
for a loader (the SDL screen consequently stops under its final wipe), and the
Amiga application has not yet switched to this presenter. This is visible SDL
integration, not completed progression, native presentation or Windows parity.

Native presenter integration exposed a directory-contract bug in the selected
clear bank: renderer lookups require records sorted by resource ID/frame, while
the builder had emitted selection order. The builder now sorts its bounded
18-entry directory before copying payloads. All 256 bank fixtures verify strict
ordering, four correct area-border frames and unchanged payload bytes.
The renderer lookup cache now validates its record against the current directory
before reuse, preventing same-address/same-size bank replacement from returning
a different resource at an old slot. This cache path is also enabled in host
tests; replacement and directory-shrink regressions plus the existing 20,580
offline sprite comparisons pass. These fixes prepare safe native lookup; they
do not complete the result presenter or its DMA handoff.

AmigaClearBorderCache now tracks per-buffer rows in the existing 320-line ring.
Camera/tile movement changes its origin; row signatures request painting only
when a physical row's source content changes. Signatures preserve the overlapping
current/incoming border seam and the finite source loops, including area-three
speed and level-three replacement. Repeated frames produce no writes. A test
paints requested scanlines with the real Amiga renderer and compares both planar
buffers against fresh source-order drawing for 2,880 frames across all area/level
combinations. After initial fill, every tested update paints fewer than 40 rows;
total row writes are below ten percent of full repainting. Offline pack tests
also enforce the recovered 16x16/origin(8,8) geometry and Limbo's overlapping
32x32/origin(16,16) exception used by this planner.
This cache is not yet published by the application's copper or measured on the
68000; the comparison establishes pixel retention, not a 20ms frame budget.

AmigaClearBorderRenderer now connects that planner to the production planar
renderer, grouping adjacent dirty scanlines with matching source anchors/masks
into clipped spans. It uses the existing row clear and masked asset drawing
paths (including their hardware-blitter path when enabled), validates all four
border frames before changing state, and returns the painted-row count. The
2,880-frame comparison now invokes this production helper instead of a test-only
scanline painter; truncated packs are rejected without changing pixels/cache.
A freestanding -m68000 compile has only existing renderer API dependencies,
with no arithmetic helpers. Live scene publication, hardware blitter execution
and timing remain unverified for this helper.

The host player Create path no longer resets health/currency to first-run
defaults on every rmMain entry. Shared PlayerMotion::enterRoom resets motion,
animation/counters and room callbacks while preserving ammo capacity and facing,
refilling charge and applying entry input delay. This follows scrPlayerInit's
global.ammo -> global.stammo and global.plxDir -> image_xscale assignments;
currency playerCreated refreshes its entry grace timer without erasing totals.
New-run reset still owns default health/currency/capacity initialization. Tests
cover a moving, shooting player with increased capacity and retained facing,
plus existing startup/restart rendering regressions. The room loader itself is
still not connected to the final result request; this removes a reset hazard
before that integration, not a claim of next-stage playability.

SDL now consumes the completed result request to reload rmMain for area-one
levels 2 and 3. It preserves the run/RNG/inventory, retires old shops and camera
state, seeds held input for the new player, and passes current level/style/Member's
Card state to the existing AreaOneBuilder instead of always using 1-1 defaults.
New instance shot/recharge counters are rebased without inventing HUD events.
Live Game regressions follow injected exits through actual selection/departure
and two successive room loads, checking retained health/currency/capacity,
inventory accumulation, level-two -10 generation count/shop eligibility and
resumed gameplay. SDL builds successfully. The area-two request is intentionally
left pending because no area-two builder is implemented; the port does not
mislabel recycled area-one terrain. Native next-room loading and the entrance
dither/audio/full upgrade effects remain unfinished, so this does not establish
complete progression or original-game parity.

AmigaPortRuntime now has an explicit prepareNextAreaOneLevel boundary matching
the SDL area's supported progression. It advances the completed request once,
keeps run globals/RNG/tick state in place, refills the player, retires actors and
old shop/camera state, and clears callbacks/index readiness before the owner
rebuilds terrain. It does not reconstruct/copy the large runtime. The paired
scene regression follows selection into both entry paths, comparing retained
health/currency/capacity/inventory and verifying untouched native RNG, empty
actor sets, reset camera/ring origin and one-shot consumption. Native terrain
construction is deliberately still the owner's responsibility: the application
has not yet invoked this boundary or connected its result presenter.
Standalone -O3/-m68000 stack reports give 1,572 bytes for preparation and 672
for its nested player reset; native instruction/helper audits pass. These
compiler reports are not a measured complete call-chain stack margin.

The native builder initializer now uses the runtime-owned RNG directly and
derives level/style/Member's Card setup from the current run. The application
rebuild path uses this initializer instead of resetting a separate RNG and
copying it back; first-stage confirmation precedes builder Create. The paired
scene test rebuilds the native index/collision callbacks after clear departure,
checks every RNG word against SDL room creation, and follows 90 further updates
with matching positions, grounding and random draw counts. Its live builder
observer now dispatches shop/exit creation just as the production observer does.
This verifies next-room runtime/terrain construction on the host; the native
application’s clear-screen presenter and handoff still remain to be connected.
The production rebuild change also passes the 45-second-bounded native results
reentry gate with SKIP_TO_GAME=1: three destinations, final gameplay at 200
updates, no checked renderer/generation failures and an intact 256-byte stack
guard. This uses A500 CPU/512 KiB Chip plus 8 MiB Fast, not stock-memory proof.
The emulator was stopped and the normal uninstrumented build restored. This
existing death-results gate does not exercise the new level-clear presenter.

Clear artwork now includes all twelve area/level headings, composed offline as
the source Result Draw string (stage number, newline, localized CLEARED). Shared
layout selects the same heading ID on both targets; SDL no longer separately
constructs that string. The native selected bank includes only the current
stage heading. The export has 96 records / 51,452 bytes, SHA-256
`3a3d8e440f49ab21e7c0fb5600d70765042837f97d49866617b8eb5f23dc3bda`.
Source-pixel verification covers every record; the conservative selected-bank
bound is 12,908 bytes within the existing 32,140-byte allocation. This prepares
native title presentation without runtime glyph composition; it does not yet
connect the native clear-screen scene.

The native clear overlay now has a retained, per-buffer planar command cache.
It validates artwork before modifying pixels, merges overlapping dirty regions,
and replays intersecting commands in source order, retaining disjoint static
regions. Borders and the player remain separate from this overlay. A 360-frame
alternating-buffer fixture follows shared Result layout through camera movement,
carousel changes and departure, comparing both planar buffers byte-for-byte
against full command rasterization. It clears 1,305,599 pixels versus 16,358,400
for full-frame clears; repeated frames clear none. Invalid assets and command
lists fail without changing pixels, and disappearing commands are erased.
The helper compiles for -m68000; its reported local stack requirement is 1,488
bytes, not a complete runtime stack measurement. It still needs live scene
publication and the production layout adapter (including target trigonometry
and owned-name suffix handling). Forced-pen text still uses the existing scalar
renderer path and needs framework mask-blit acceleration before timing claims.
No emulator performance claim follows from these host raster comparisons.

Unflipped forced-pen assets now use the same clipped/ring-split planar dispatch
as normal sprites. In Chip RAM, the existing framework Bitmap::patternWithMask
recolors each destination plane from the offline opacity mask; no glyph or mask
is generated at runtime. The CPU fallback composes bytes rather than individual
pixels. Flipped assets retain their scalar path. An independent pixel oracle
covers 3,840 combinations of pens, offsets, clipping, HUD/game targets and ring
origins, including 2,072 successful emulated blit calls. This checks dispatch
coordinates and fallback bytes, not the physical blitter's shift implementation.
The 360-frame retained-overlay comparison and existing 20,580 sprite comparisons
also pass, and the native build passes 68000 instruction/64-bit helper audits.
The 45-second-bounded SKIP_TO_GAME results-restart gate also passes all three
destinations and 200 subsequent gameplay updates with the stack guard intact.
Its emulator is stopped and the normal build restored. That gate checks scene
reentry/state and existing failure counters, not pixel-exact recolored output
or timing; those hardware-specific checks remain outstanding.

Clear-scene assets now have an explicit simultaneous-residency partition of the
retired 76,800-byte garden storage: departure mask at 0..31,103, selected artwork
at 31,104..62,859 (31,756 bytes available), and preserved HUD artwork starting at
62,860. This requires no new Chip allocation. Preparation and offer changes are
transactional on validation failure. A 256-selection regression checks selected
bank bytes, immutable mask, HUD region and allocation guards; native builds now
embed the 51,452-byte offline clear pack in read-only storage and pass the 68000
audits. This helper is not yet invoked by the application. Crucially, entry must
retire ALL gameplay DMA, not merely the exit-mask pointers: camera staging can
rotate a world display into the garden region that the clear mask will reuse.

- Level exit/progression, upgrade selection, other weapons/styles and later areas.
- Complete enemy roster, bosses and ending flow.
- Audio playback on both backends, remaining feedback/effects and pacing.
- Persistent saves/unlocks and complete menus/settings.
- Original collision masks, fractional query semantics and event-order audit.
- Broad Windows comparison across seeds and scenarios, not only shared tests.
- Sustained worst-case frame budget and practical A500 memory configuration.

Gameplay debugging uses `SKIP_TO_GAME=1`; intro tests are separate. Correctness
instrumentation must not be treated as a performance measurement.
