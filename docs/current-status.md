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
- [x] Shared shop interaction and feedback requests (callable, not yet live).
- [x] Shared shop world ownership and registered-shop input/purchase hooks in
  both runtimes, verified with an isolated paired fixture.
- [x] Retained shared merchant feedback and purchased-item disappearance state
  (rendering/audio consumers still pending).
- [x] Table, glass-case and invisible-wall player collision masks shared by
  both adapters, including separate retirement rules.
- [x] Shared static counter/table/case drawing and offline planar fixtures;
  Amiga composition uses the terrain cache, not recurring furniture redraws.
- [x] Cased consumable icons (original frame 1), purchase disappearance and
  changed-row cache refresh; merchant/menu rendering remains pending.
- [ ] Live shop instances and entry/exit/control locking on both platforms.
- [ ] Shared menu interaction, prices, sold cases and merchant feedback.
- [ ] Offline shop art and native rendering without recurring terrain redraw.
- [ ] Deterministic purchases: insufficient funds, sold out, each item, exit/reentry.
- [ ] Bounded target capture/replay and original-game validation.

`ShopInventory` is currently a callable core, not a playable shop. Ordinary 1-1
marks its shop as already made; reaching shops naturally also needs subsequent
level progression. Do not silently change the original first-level generator
to make a shop appear for testing.
Explicit registration now drives interaction in both runtimes, but production
generation, shop children and rendering are not connected yet. The fixture is
not proof of a complete shop or Windows event-order equivalence.

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
