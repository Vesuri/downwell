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
y+32. These events and their required result/upgrade behavior remain unported.
Stage advancement alone would skip visible behavior and RNG-consuming events.

`LevelExit.h` now models inherited/derived child creation order, the guarded
collision latch and both alarm events. Its focused test checks trigger anchors,
dead-player exclusion, repeated collisions, fifth-tick dither creation and
result-room request ninety further alarm phases later. It is not connected to
live collision/room loading yet; child Create behavior and the playable result
room remain prerequisites. Alarm requests alone are not a completed level exit.

The resource audit also found objLineToResult (306) and its parent marked
Visible=false. Both drawing adapters now omit its sprite 127: the Amiga terrain
collector no longer adds it as decoration or invalidates artwork rows for it.
Generated Create commands are retained for collision/transition integration.
Host rendering tests verify the original resource flag and omit drawing an
active trigger; native generation tests retain command-order checks while
excluding the trigger from the decoration stream. This fixes presentation,
not the still-unimplemented end-of-level collision dispatch.

The generated exit now also sets the shared camera's roomEnd to triggerY-176
on both live creation paths, reproducing roomEndSetter Create. camMain's cap
is applied to the target before easing, and to current Y only inside that same
target-exceeded branch. Reset clears the limit to its original -1 sentinel.
Focused tests cover easing, overshoot, retained death targets and reset; the
paired runtime regression passes. This does not yet create the goal field,
consume its random radius/audio choices, stop the player or enter rmResult.

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
