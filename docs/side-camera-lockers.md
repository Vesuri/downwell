# Side-camera locker fidelity work

This is a confirmed shared missing feature, not a completed runtime change.
Both adapters currently ignore the camera behavior of generated object 62.
The shared collision/release primitives are implemented and tested; production
registration and collision-phase dispatch remain to be wired.

## Original evidence

The pinned Steam input is SHA-256
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`.
Evidence is the local UTMT 0.9.2.0 assembly export and resource manifest:

- `gml_Object_sideCamLocker_Create_0.asm`: x below 160 becomes 80; x above
  room width minus 160 becomes room width minus 80. Equality is unchanged.
- `gml_Object_camMain_Collision_sideCamLocker.asm`: only when `camPointy`
  differs from the other instance's y does it change the target. If not
  already side-locked, it also assigns `yy` and published y and latches the
  lock. A subsequent different locker changes the target without snapping.
  The event separately clears `global.easter` and `global.achNoSideroom`.
- `gml_Object_camMain_Step_0.asm`, blocks 56–62: while side-locked, test
  `place_meeting` at the existing published x/y. When no locker overlaps,
  snap vertical state to player y + 24 and horizontal state to the shaft
  target, set both slowdowns to one and release the lock. This branch is
  outside the death guard and precedes the room-end clamp.
- Manifest object 62 is `sideCamLocker`, using sprite 596. Object 64 is
  `camMain`, with explicit texture mask 583 and collision event subtype 62.
  Mask 583 is 160x284, origin (80,142), full margins (0,159,0,283).
  Mask 596 is 100x160, origin (50,80), full margins (0,99,0,159).
  Both are serialized axis-aligned rectangle masks.

`SideCameraLocker` expresses Create x normalization and the default unscaled
integer rectangles. `MainCameraMotion` now provides collision latching and
an overlap argument for Step release. Normal callers remain unlocked and keep
their previous behavior. The mask-overlap tests validate this metadata-derived
rectangle model, not a fresh Windows runner observation of edge semantics.

## Tests and remaining integration

`main-camera-test` covers 356,955 overlap positions around both edge thresholds
at three locker positions, strict Create boundaries, equal-target collisions,
first snap, retargeting while locked, continued easing, release during death,
room-end ordering, no top clamp inside Collision, and room reset. Existing
shaft, side-room, rounding and easing tests remain enabled.

The focused test passes normally and under ASan/UBSan
(`/tmp/downwell-side-camera-sanitized.log`). The 120-second-bounded native
cross-target replay retains the complete previous result vector
(`/tmp/downwell-side-camera-replay.log`), and normal startup is restored with
both 68000 audits clean. This native replay validates the unchanged unlocked
path; it does not exercise the new, not-yet-dispatched collision primitive.

Next integration must retain source event phases: register/reset lockers in
both world adapters, normalize their Create coordinates, test old published
positions during Step, then dispatch collisions after ordinary Step work.
Publish the resulting camera delta to the Amiga circular buffers without
retroactively changing the view used by generation/actor Step work. Include
the lock and instance lifetime in deterministic state checks. Achievement/
easter flags, multiple-overlap ordering and original-runner edge coverage must
not be claimed complete merely because the math helpers pass.
