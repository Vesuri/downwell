# Open work

## Extraction

- [x] Pin UndertaleModTool/UndertaleModLib 0.9.2.0 at commit `03c7048`.
- [x] Implement a read-only extraction command with an input-hash guard.
- [x] Export resource IDs/names, VM assembly, decompiled GML, object-event bindings,
  rooms, texture metadata, fonts, shaders, and audio metadata.
- [x] Define the initial versioned manifest and validate core cross-references.
- [x] Prove a byte-for-byte parser round trip of `data.win`.
- [x] Export and verify the authoritative runtime object/tile ID boundaries.
- [x] Recover authoritative room order and document the initial boot/transition
  behavior slice.
- Sample difficult code entries and compare decompilation to VM assembly.
- Expand the manifest with glyph/kerning, collision-mask, shader source, detailed
  room-layer contents, and extension-function metadata.

## Host reference

- [x] Implement and test the deterministic `rmInit` to `rmMenu` startup model.
- [x] Define a portable input-replay wire format and stable state digest.
- [x] Factor and test the fixed-size GameMaker alarm bank used by startup.
- [x] Add fixed-capacity instance storage and persistent room-transfer behavior.
- [x] Dispatch Begin Step, implemented alarms, Step, and End Step in fixed phases.
- [x] Add atomic room entry with serialized creation order and lifecycle phases.
- [x] Define and generate a deterministic native room-data pack.
- [x] Add an endian-safe native room-pack reader with complete-pack validation.
- [x] Bridge packed rooms into complete native instance definitions.
- [x] Load recovered `rmInit` and all 192 `rmMenu` instances in the SDL startup
  runtime.
- [x] Define and generate a deterministic native object/event binding pack.
- [x] Add an endian-safe object-pack reader with inherited-event lookup.
- [x] Route implemented startup lifecycle and step handlers through recovered
  object-event code IDs.
- [x] Translate `rmInit`/`rmMenu` room creation into deterministic camera,
  audio, control-lock, and deferred-spawn state.
- [x] Carry `GEN8.LastObj` in the native room pack and validate its boundary.
- [x] Add an atomic fixed-capacity deferred instance-creation queue.
- [x] Apply `rmMenu`'s deferred `ditherFade` creation before Room Start.
- [x] Translate `ditherFade` PreCreate, Create, and Alarm 0 startup behavior.
- Replace the smoke-state class with a GameMaker-compatible value/state layer.
- Select and verify the deterministic RNG algorithm against GameMaker output.
- Implement object lifecycle and event ordering.
- Execute recovered object/instance creation logic; implement collision and draw ordering.
- Add texture/font/audio resource loaders from generated manifests.
- Bring up the `rmInit` to `rmMenu` startup slice before broad resource coverage.

## Amiga

- Choose and document the dA JoRMaS framework fork to vendor.
- Fix display/chipset assumptions for a 320x568 source game on the target mode.
- Link a minimal 68020 executable and run/debug it under FS-UAE.
- Establish memory budgets and an asset conversion plan.
- Define the host/Amiga replay and state-digest seam before gameplay code moves.
