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
