# SDL / Amiga feature parity

Parity with the implemented SDL reference is the current priority. This does
not mean completeness against the Windows game. Do not count a matching
port-runtime replay on two CPUs as proof that StartupRuntime and
AmigaPortRuntime implement identical behavior.

## Current evidence and remaining work

| Slice | Current evidence | Remaining parity work |
| --- | --- | --- |
| Publisher, credits, controls, garden, well, style select | Native implementations and decoded scene comparison tests | Full-sequence display regression after gameplay changes |
| Player, camera, landing, jumping, gunboots | SDL/native comparison through entrance and first scrolling section | Longer combat tapes; current position comparisons allow fixed-point tolerances |
| Area-one geometry and endings | Shared builder, complete native first-area generation, terrain oracle | Side-room reversals and offscreen destruction coverage |
| Bullets and gems | Shared 16-bullet / 64-gem limits | Dense encounter behavior and ordering comparisons |
| Actor presentation | Offline hardware sprites plus masked planar overflow; no actor discarded just because DMA slots are full | Overlap/draw-order comparison with SDL; broader crowded-scene timing |
| Enemies and rewards | Six native enemy types; unit tests for combat, stomps and rewards | Compare every active enemy and gem against SDL, not just player state |
| HUD | Health, ammo, currency and streak implementations | State-driven image comparisons for damage, depletion and streak expiry |
| Death | Native control lock and death timer | End-to-end SDL/native death-state comparison |

Sound playback, complete shops/upgrades, later areas and a complete death/restart
UI must not be described as Amiga parity omissions merely because the Windows
game has them: establish which portions SDL actually implements first.

## Crowded actor rendering

Simulation capacities are shared in `src/core/GameplayLimits.h`. The six regular
actor DMA buffers plus channel 7 remain a separate rendering limit. A multi-part
enemy either fits completely in the remaining channels or becomes one masked
planar BOB; it is never cut in half by channel exhaustion. All eight channels
are assigned in reverse SDL painter order (gems, bullets, player, enemies).
Hardware sprites still rely on playfield priority at horizontal edges.

Overflow BOBs use offline planar assets (including mirrored enemy variants),
the existing Bitmap/blitter path, and the inactive scrolling display buffer.
The terrain backing is never modified by actors. Touched physical ring rows
are marked for restoration on reuse; unchanged terrain remains cached. BOB
clipping is confined to the gameplay area and handles the ring boundary.

SCENE_VERIFY compares actual BOB blits against a scalar compositor. The
`amiga/verify_gameplay_bobs.gdb` script forces all non-player actors through
this path until at least 100 actor draws have been checked, including sparse scenes. Run it against a
`SKIP_TO_GAME=1 SCENE_VERIFY=1 BOB_VERIFY=1` clean build using the bounded startup verifier and
`READY_MARKER=DOWNWELL_GAMEPLAY_BOBS_READY`.

Initial normal gameplay timing after this change: 600 frames / 601 PAL fields
(49.92 FPS), one two-field frame at tick 528. This is a regression from the
earlier sparse-rendering 600/600 result, not a blanket 50 FPS claim. Preserve
the visible actors while fixing the remaining deadline miss.

Validation for this slice: full host suite passed (including 12,348 offline
hardware-sprite comparisons and mirrored planar/ring restoration checks);
the 600-tick cache run passed 146 fresh-terrain comparisons; forced BOB mode
passed 102 scalar-versus-blitter actor comparisons with no pixel mismatch.
The latter is a correctness test with expensive instrumentation, not an FPS
measurement. Its earlier 600-tick form exceeded the 45-second wall-clock limit;
the smaller test exits as soon as its draw coverage requirement is satisfied.

## Wall-snail orientation

The Amiga renderer now follows SDL's downward-crawling vertical flip. This is
also explicit in recovered `gml_Object_enmSnail_Step_0.asm` block 20:
`image_yscale = -sign(ysp)`. Sprite resource 177 remains unchanged; synthetic
frame `256+n` contains the offline vertical flip of source frame `n`, with
origin Y adjusted to preserve the anchor. Both horizontal facings have planar
and DMA variants. No target-side pixel flip or conversion is introduced.

Tests compare every flipped frame against a reflected original raster, test
the direction reversal, and compare all prepared DMA bodies with the pixel
oracle (13,132 comparisons including the new variants).

## Actor overlap order

SDL's `Game::renderMain` paints enemies, player, bullets and gems in that order.
The previous fixed channel-zero player overrode bullets/gems; allocating later
enemies to hardware after an earlier BOB also reversed painter order.
`AmigaActorChannels` now allocates the frontmost actors to the lowest DMA
channels. Once an entire actor cannot fit, it and every actor behind it use
BOBs in original back-to-front order. No channel is reused by a smaller actor
behind the first BOB. Player BOBs also use offline mirrored planar variants.
The channel planner is tested for 256 one-/two-part actor sequences at every
channel budget from zero to eight.

The target forced-BOB test passed 100 scalar-versus-blitter comparisons with
the player included. Startup now also has a 512 KiB Chip allocation check;
see `amiga-memory.md` for the reproduced shortage and offline deduplication fix.
Final normal-build timing for the ordered renderer: 600 frames / 603 PAL
fields (49.75 FPS), with a three-field frame at 528 and a two-field frame at
574. These misses remain open; the former 50 FPS result is not applicable to
this build. The playable SKIP_TO_GAME build is restored after profiling.

## Enemy lifecycle comparison

The scene parity test now checks two actual SDL enemy retirements against the
native runtime, in addition to strict cleanup-boundary unit tests. Native
awake enemies retire outside `viewTop-160 .. viewTop+3000`, matching
`gml_Script_scrOutofview` VM code. Retirement clears the activation-pending
state so an old enemy cannot wake again. Future prebuilt, not-yet-awake enemies
are deliberately excluded from this live-instance cleanup.

`DOWNWELL_COMPARE_ACTORS=1` on the scene test emits both actor populations.
This reveals an unresolved **creation-timing mismatch**, not merely rounding:
after the current tape, SDL's first turtle is at (228,2521), whereas the native
turtle is at (214,2139). SDL incrementally creates fragments when the builder
is within 128 pixels of the viewport bottom; native terrain is prebuilt and
enemy movement uses a different wake gate, while alarms already run. The
next simulation work must reproduce creation/alarm/RNG ordering and terrain
availability. Do not widen position tolerances to call this parity, or replace
SDL with the native runtime to conceal it. The static raster cache can remain
precomputed, but it must not dictate when simulation objects become live.
