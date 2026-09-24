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

## Still open beyond that milestone

- Level exit/progression, upgrade selection, other weapons/styles and later areas.
- Complete enemy roster, bosses and ending flow.
- Audio playback on both backends, remaining feedback/effects and pacing.
- Persistent saves/unlocks and complete menus/settings.
- Original collision masks, fractional query semantics and event-order audit.
- Broad Windows comparison across seeds and scenarios, not only shared tests.
- Sustained worst-case frame budget and practical A500 memory configuration.

Gameplay debugging uses `SKIP_TO_GAME=1`; intro tests are separate. Correctness
instrumentation must not be treated as a performance measurement.
