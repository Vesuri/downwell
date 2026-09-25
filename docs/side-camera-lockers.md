# Side-camera locker fidelity work

Both adapters now register generated object 62 and execute its camera behavior.
This closes the previously ignored locker behavior for the current rmMain
subset; it does not establish complete original-camera or achievement fidelity.

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
an overlap argument for Step release. The mask-overlap tests validate this metadata-derived
rectangle model, not a fresh Windows runner observation of edge semantics.

## Live integration

`RunSession` owns a bounded, creation-ordered room collection (128 entries,
explicit failure on overflow). New rooms/runs clear it without a large stack
temporary. Native builder registration bypasses plain terrain handling; SDL
materializes queued Creates through object 62 before the camera's later Step,
keeping preceding instance order and the actual hidden instance record.

Step tests overlap at the prior published camera position. The adapters apply
camera Collision after their ordinary actor/generation/retirement work and
before shop End Step/presentation. The snap therefore does not retroactively
change earlier Step queries. On Amiga the additional collision scroll is folded
into this frame's final delta and circular origin, including negative scroll.
The locker collection, easing/target fields and lock flag have a common digest
checked by the paired test. Runtime digests add an SLOC extension only when
locker state exists, preserving previous empty-room hashes.

The Amiga prefetch target now uses a read-only copy of the shared camera rules
to anticipate the destination column's locker height. Staging still transfers
at most 48 rows per update and maps them to the existing circular coordinates;
there is no extra Chip allocation or full-frame repaint. The first native run
exposed seven fallback transfers because old prefetch assumed an unchanged
height. After the prediction fix it returns to six prepared-buffer swaps and
one deliberately forced fallback.

## Verification and remaining scope

`main-camera-test` covers 356,955 overlap positions around both edge thresholds
at three locker positions, strict Create boundaries, equal-target collisions,
first snap, retargeting while locked, continued easing, release during death,
room-end ordering, no top clamp inside Collision, and room reset. Existing
shaft, side-room, rounding and easing tests remain enabled.

The initial primitive test passed normally and under ASan/UBSan
(`/tmp/downwell-side-camera-sanitized.log`). The 120-second-bounded native
cross-target replay retains the complete previous result vector
(`/tmp/downwell-side-camera-replay.log`), and normal startup is restored with
both 68000 audits clean. This native replay validates the unchanged unlocked
path; it predates live collision integration.

The integrated host checks cover collision scroll/ring wrapping, walking back
to the shaft, reset, pure prefetch prediction, synchronous generated creation,
capacity failure and complete room collection/easing digests. Paired room-entry
tests retain the original three seeds and add seed 99 with a synthetic central
locker to exercise lock/release phase timing; RNG words and draw counts match
throughout. Logs: `/tmp/downwell-side-lock-final-host.log` and
`/tmp/downwell-side-lock-paired.log`.

The bounded native camera test passes 600 updates, four locks and three
releases, 119 terrain comparisons and 14 alternating-display checks with zero
mismatches and an intact stack guard. Logs:
`/tmp/downwell-side-lock-native-v2.log` and
`/tmp/downwell-side-lock-native-v2-gdb.log`. It uses synthetic flat collision
geometry and explicit lockers, not a natural generated side-room playthrough.

Achievement/easter flags remain unimplemented. Collision iteration follows
creation order and rechecks the current published camera rectangle per pair;
multiple overlapping lockers and exact runner edge semantics still need a
Windows-runner check. General VM event-order fidelity beyond these modeled
camera phases remains an audit item.

Final integration validation also passes the complete `make test` host suite
(`/tmp/downwell-side-lock-full-host.log`), the expanded camera tests under
ASan/UBSan (`/tmp/downwell-side-lock-camera-sanitized.log`), and the bounded
cross-target replay (`/tmp/downwell-side-lock-final-replay.log`). The replay
retains the complete vector shown by the earlier unlocked baseline; locker
coverage comes from the paired tests and native camera gate above. Normal
startup options are restored to zero and both 68000 audits pass. These are
correctness checks, not a new frame-time benchmark.
