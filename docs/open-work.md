# Open work

This is the early milestone inventory. For the current ordered queue, verified
scope and remaining work, use [current-status.md](current-status.md). Some
unchecked entries below have since been partially or fully superseded.

## Extraction

- [x] Pin UndertaleModTool/UndertaleModLib 0.9.2.0 at commit `03c7048`.
- [x] Implement a read-only extraction command with an input-hash guard.
- [x] Export resource IDs/names, VM assembly, decompiled GML, object-event bindings,
  rooms, texture metadata, fonts, shaders, and audio metadata.
- [x] Define the initial versioned manifest and validate core cross-references.
- [x] Prove a byte-for-byte parser round trip of `data.win`.
- [x] Export and verify the authoritative runtime object/tile ID boundaries.
- [x] Export every embedded texture page as a decoded PNG.
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
- [x] Define and generate complete native sprite/frame/atlas metadata.
- [x] Add an endian-safe sprite-pack reader with full reference validation.
- [x] Load recovered texture atlases in SDL and draw a recovered player frame.
- [x] Replace the splash-bar placeholder with recovered splash sprites.
- [x] Define and generate an endian-safe native pack for recovered builder
  patterns, including all entrance and regular area-one fragments.
- [x] Select, mirror, interpret, and render the first recovered area-one
  entrance fragment after the deterministic empty-shaft prelude.
- [x] Continue into recovered regular area-one fragments with shared row
  overlap, mirroring, boundary walls, padding pillars, and RNG ordering.
- [x] Preserve generated-object Create results across deferred insertion,
  including block variants, scenery orientation, enemy alarms, and activation.
- [x] Apply usual-gun bullet collision to one-hit breakable blocks while
  retaining indestructible area walls and opening the resulting terrain cell.
- [x] Complete the first player landing/gunboots loop with airborne firing,
  shot gating, recoil state, landing detection, and full charge restoration.
- [x] Add the recovered downward activation frontier and out-of-view cleanup
  window for generated area-one geometry, scenery, and enemies.
- [x] Carry recovered area-one enemy health/reward state and resolve usual-gun
  damage, turtle immunity, death, and native animation independently per enemy.
- [x] Make the shared fire/up control jump from solid ground, apply variable
  jump-height release, then unlock gunboot fire on the next airborne press.
- [x] Extend the native builder pack with all ten recovered 19-cell-wide
  area-one right-room layouts and their special doorway/object symbols.
- [x] Substitute recovered right rooms at their scheduled generation counts,
  preserving no-repeat selection, mirroring, row overlap, and RNG order.
- [x] Interpret right-room doorway caps, torches, ambience, time fields,
  camera lockers, and alternating gem-chunk/gun-module occupants.
- [x] Preserve both irregular recovered area-one shop patterns and implement
  their no-mirroring selection path, while retaining the original 1-1 shop
  suppression for the default play style.
- [x] Give all six area-one enemies native fractional motion state and their
  recovered ground, jump, steering, wall-crawl, and initial latch behaviours.
- [x] Split defeated-enemy rewards into recovered two- and ten-value physical
  gems with bounce, attraction, collection, expiry, and fixed-capacity state.
- [x] Switch SDL to the recovered PC 380x284 presentation, centring the
  unchanged 160x284 playfield and moving health, currency, ammo, and streak
  presentation into the 4:3 HUD paths.
- [x] Resolve native player/enemy contacts with recovered stomp kills,
  gunboot recharge and bounce, contact damage, knockback, and invulnerability.
- [x] Pack and emit all four recovered area-one endings after the exact 1-1
  generation length, including the native result-line marker.
- [x] Preserve synchronous area-one breakable-block Create RNG consumption
  while native instance creation remains deferred.
- [x] Reach playable `rmMain` and land the native player on solid geometry
  generated from the recovered area-one entrance fragment.
- [x] Implement recovered 16×2 one-way thin-platform collision and correct
  area-one wall/block rendering and collision to the native 16×16 mask.
- [x] Trigger the menu title from recovered camera-center alignment so entering
  either edge of the well cannot deadlock the teleporter sequence.
- [x] Honor `playerDraw`'s `goalStop` guard so the frozen player is hidden
  during the menu-well camera transition.
- [x] Enter the recovered zero-health death state, disable player control,
  freeze the gameplay camera, and expose the 90+10-frame death-menu gate.
- [x] Reproduce the damaged player's alternating `dFlash` solely by hiding
  and showing its hardware sprite during the invulnerability alarm.
- [x] Animate the grounded standard-style player with the recovered eight-frame
  run sprite, exact 0.3-frame cadence, and run-entry reset.
- [x] Advance the four-frame standard-style idle sprite at its recovered
  0.1-frame cadence on hardware sprite 0.
- [x] Play and retrigger the four-frame gunboot shooting pose at its recovered
  0.25-frame cadence, stopping on the final frame.
- [x] Preserve the recovered one-, two-, four-, and six-frame animation cycles
  for every native area-one enemy state.
- [x] Select and animate the recovered five-frame ledge-balancing pose from
  the player's forward support probe.
- Replace the smoke-state class with a GameMaker-compatible value/state layer.
- [x] Recover the Windows runner's WELL512 stream and integer-range semantics
  from `Downwell.exe`, then pin portable test vectors.
- Implement object lifecycle and event ordering.
- Execute recovered object/instance creation logic; implement collision and draw ordering.
- Add texture/font/audio resource loaders from generated manifests.
- Bring up the `rmInit` to `rmMenu` startup slice before broad resource coverage.

## Amiga

- [x] Choose and document the dA JoRMaS framework fork to vendor.
- [x] Fix display/chipset assumptions for the recovered 380x284 4:3 mode.
- [x] Link a minimal 68000 executable and run/debug it as an A500 under FS-UAE.
- [x] Establish the initial Chip RAM budget and asset residency rules.
- [x] Embed and validate room, object/event, and builder-pattern packs with the
  shared endian-safe readers.
- [x] Establish PF1 as a copper-wrapped vertical circular buffer while PF2
  remains fixed for the HUD.
- [x] Define the host/Amiga replay-input seam before gameplay code moves.
- [x] Cross-compile the shared state-digest primitive and pin the initial Amiga
  port checkpoint against the host result.
- [x] Move the native player with allocation-free fixed-point state and map
  grounded fire to jump plus a later airborne press to gunboot recoil.
- [x] Emit the first native gunboot bullet on a dedicated hardware sprite and
  update the 4:3 side-panel ammo display when a charge is consumed.
- [x] Run one canonical replay through the shared native-port runtime on the
  host and an emulated 68000 A500, and require identical rolling state digests.
