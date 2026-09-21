# Open work

## Extraction

- [x] Pin UndertaleModTool/UndertaleModLib 0.9.2.0 at commit `03c7048`.
- [x] Implement a read-only extraction command with an input-hash guard.
- [x] Export resource IDs/names, VM assembly, decompiled GML, object-event bindings,
  rooms, texture metadata, fonts, shaders, and audio metadata.
- [x] Define the initial versioned manifest and validate core cross-references.
- [x] Prove a byte-for-byte parser round trip of `data.win`.
- [x] Recover authoritative room order and document the initial boot/transition
  behavior slice.
- Sample difficult code entries and compare decompilation to VM assembly.
- Expand the manifest with glyph/kerning, collision-mask, shader source, detailed
  room-layer contents, and extension-function metadata.

## Host reference

- Replace the smoke-state class with a GameMaker-compatible value/state layer.
- Define deterministic RNG and input replay formats.
- Implement object lifecycle and event ordering.
- Implement room loading, collision, and draw ordering.
- Add texture/font/audio resource loaders from generated manifests.
- Bring up the `rmInit` to `rmMenu` startup slice before broad resource coverage.

## Amiga

- Choose and document the dA JoRMaS framework fork to vendor.
- Fix display/chipset assumptions for a 320x568 source game on the target mode.
- Link a minimal 68020 executable and run/debug it under FS-UAE.
- Establish memory budgets and an asset conversion plan.
- Define the host/Amiga replay and state-digest seam before gameplay code moves.
