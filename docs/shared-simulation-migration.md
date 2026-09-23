# One gameplay simulation, two platform backends

The destination is one portable, 68000-compatible gameplay implementation in
`src/core`, used unchanged by SDL and Amiga. The Windows VM/resources are the
behavioral authority. Agreement between two independent adaptations is not a
substitute for this architecture or original-game validation.

Current debt: `StartupRuntime` and `AmigaPlayer`/`AmigaPortRuntime` still own
separate gameplay state and update sequences. The native classes contain domain
logic that does not belong under `src/platform`. Rendering/storage optimizations
do not justify separate rules for movement, enemies, collision or combat.

## First migration slice

`PlayerControls.h` owns the default non-water horizontal input rules and their
constants. Both existing runtimes call it; their former rule branches have
been removed. Focused source-backed tests execute this same implementation
with both temporary arithmetic adapters. `global.noControl` now explicitly
zeros horizontal velocity as the original script does.

The float adapter is transitional compatibility, not the final architecture.
It permits extraction without simultaneously changing every stored coordinate.
The final shared state must use one fixed-point representation on both CPUs.

The uncommitted native-only ceiling experiment was removed. The confirmed
ceiling omission remains unresolved and must be fixed in shared collision code,
including consistent rounding, masks and event order. It must not be lost from
the backlog merely because the separate experiment was discarded.

## Remaining migration

1. Consolidate player state, vertical integration, jump/gun state and collision
   ordering in core; provide adapters for existing presentation consumers.
2. Consolidate enemy, bullet and reward state/update code, retaining original
   event timing, masks and deterministic RNG consumption.
3. Make SDL and Amiga invoke the same world update. Remove the replaced
   simulation branches, rather than leaving two permanent selectable modes.
4. Keep the Amiga terrain cache, circular DMA scrolling, Copper lists, blitter,
   offline sprites, audio and input translation in its backend. SDL retains its
   own rendering/audio/input backend, not different gameplay.
5. Replace port-versus-port gameplay agreement tests with shared-core rule tests
   against VM evidence, plus cross-CPU replay and backend presentation tests.

Cross-target execution checks remain useful during migration. Their passing
status does not imply that this migration or original-game fidelity is complete.

First-slice validation: full host suite and bounded A500 replay pass with
unchanged native digest `bfbbd762b9b62a59`; both 68000 audits are clean.
SDL was rebuilt. The known ceiling and extended-combat failures remain open.

## Player-state extraction

`src/core/PlayerMotion.{h,cpp}` now owns the previously native player state and
update routine. The Amiga player header is only a compatibility alias; there is
no platform-specific player implementation file. Both the host test build and
the actual 68000 build compile the core implementation. Its behavioral tests
have moved to `tests/core/PlayerMotionTest.cpp` and include the core directly.

This relocation intentionally changes no simulation behavior. SDL's
`StartupRuntime::updateMainPlayer` has **not yet** been replaced. In particular,
the extracted implementation lacks upward ceiling collision handling that the
SDL path currently provides. Wiring SDL to it without resolving that omission
would remove implemented behavior, not establish fidelity. The original
`scrCheckCollisionWith` uses rounded predictive probes and a diagonal fallback;
`scrWallCol` also handles breaking boxes above the player. Simply transferring
the SDL axis loop would not completely reproduce those source rules either.

Next migration gate: finish integrating the shared source-backed collision
contract, then route SDL player motion, jump/shoot state, stomp and contact
responses through `PlayerMotion`, removing its old authoritative state updates.
Rendering mirrors may remain temporarily, but must not become a second writer.

Extraction validation: full host suite passes, including the relocated core
test; bounded cross-target replay retains `64 bfbbd762 b9b62a59`. The 68000
instruction and 64-bit-helper audits pass. This verifies the relocation, not
completion of SDL integration or original-game fidelity.

## Shared ceiling collision

`SolidCollision.h` transcribes `scrCheckCollisionWith` VM blocks 1..40:
rounded predictive probes, integer contact walks and the diagonal fallback
(including its reset of both displacements at the vertical limit). Tests cover
those branches, ties-to-even with negative coordinates, fractions and zero
velocity. The fixed arithmetic uses fifths of Q8, preserving the player's
gravity residual without floating-point or 64-bit target operations.

Both current runtimes now call the same vertical branch for upward contacts.
The complete two-axis routine is tested but is not yet the runtime collision
path: the current callers still integrate horizontal movement separately.
Calling the whole routine with a synthetic zero horizontal velocity was tested
and rejected: its rounded horizontal probe could snap fractional X into a wall.
The vertical-only adapter explicitly preserves X, with a regression fixture.

This removes the native missing-ceiling response. The full host suite passes.
The extended combat tape still fails, but its former first movement mismatch
at tick 666 is gone. The first combat mismatch remains at tick 702 (health 3/4),
and the first reported movement mismatch is now 704 after that knockback.
Final health is 3/3, kills 2/1, currency 18/6. These are diagnostic results,
not new expected values or relaxed acceptance criteria.

Still required: box-above destruction and its downward impulse, shared mask
semantics, full source event order, and replacing the SDL player state/update
with `PlayerMotion`. Neither the existing side-contact loop nor the floor
resolver should be mistaken for the complete original collision script.

Target verification: the bounded 64-frame cross-CPU replay passes with
unchanged digest `bfbbd762b9b62a59`, and both 68000 audits are clean. This replay
checks target execution stability, not the newly added ceiling case; the
source-branch fixtures and the longer host combat trace exercise that case.
The normal Amiga executable and playable SDL binary were rebuilt.
