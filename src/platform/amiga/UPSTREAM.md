# Amiga platform provenance

The reusable low-level framework from the dA JoRMaS C++ template and the local
Rescue on Fractalus and Revs ports is vendored in `framework/`. Downwell builds
and uses its hardware, bitmap, palette, sprite, utility, GCC-runtime, copper,
and machine-lifecycle components. The live 2+2 dual-playfield buffers are
non-owning `Bitmap` views, hardware sprite buffers are `Sprite` objects, the
copper palette comes from `Palette`, and `AmigaPlatform` owns the complete
Workbench takeover, raw VERTB timebase, COP1LC publication, and restoration.

Downwell retains a fixed-capacity append-only `CopperList` specialization: the
circular PF1 needs a variable scanline split that the template's indexed demo
list does not model. `AmigaMemory` likewise owns one game-specific allocation
set rather than many independent framework allocations. The 380-pixel logical
canvas, 384-pixel backing rows, 22-word fetch, circular PF1 and fixed PF2 are
therefore explicit adaptations rather than changes to generic framework rules.

The build selects the framework's portable C++ implementation (`NO_ASSEMBLER`)
so every linked instruction remains 68000-safe. The inherited VBR helper is
adapted to return address zero, which is the 68000's fixed vector table, rather
than emitting the 68010+ `MOVEC VBR` instruction.

Reference implementations inspected:

- `Rescue on Fractalus/src/platform/amiga`
- `Revs/src/platform/amiga`
- `dA JoRMaS/Template/C++`

The freestanding compiler support is supplied by the shared Amiga toolchain at
build time (`~/.local/support/gcc8_{a,c}_support.*`), as in the existing ports.
The framework's `GCCRuntime.cpp` supplies the C++ heap and runtime shims used by
`Palette`, `Bitmap`, `Sprite`, and future framework-owned allocations.
