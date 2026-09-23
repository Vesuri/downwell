# Amiga backend

This directory will contain the native Amiga implementation of the platform
boundary used by `src/core`.

The display is specified in `docs/amiga-display.md`: a 380x284 visible canvas
on a 384-pixel DMA backing raster, with PF1 assigned to the centred game well
and PF2 assigned to the HUD and side panels.

The target uses the reusable dA JoRMaS Amiga framework layer: hardware access,
bitmap views, copper lists, palettes, sprites, utilities, GCC runtime support,
and a Rescue-style platform takeover/restore shell. Downwell keeps its own game
runtime and memory layout; the demo-production timeline, `Part` hierarchy, and
tracker player are applications of the framework rather than platform services.
Provenance and the deliberate adaptations are tracked in `UPSTREAM.md`.
As in Rescue on Fractalus, scene-specific copper-list classes own the gameplay,
menu, and splash register programs; the main loop only builds and publishes the
appropriate typed list.

`AmigaRenderer` mirrors the important state transition in the shared renderer
contract: setting a clip rectangle selects PF1 and limits game drawing to the
central well; clearing the clip selects PF2 and restores the full 380x284 HUD
canvas. Its drawing coordinates map directly to pixels 0..379 of each
384-pixel-wide bitplane. The hardware fetch presents logical pixels 16..367:
each bitplane pointer skips the first two bytes, a four-byte modulo skips the
two bytes at each row's far edge, and `$30..$d8` fetches the intervening 22
words. The backing geometry remains 384 pixels wide for direct host-to-Amiga
coordinate mapping.

The current executable displays recovered area-one wall art on a 320-line
circular PF1, the recovered 4:3 HUD on the fixed PF2, and the player on hardware
sprite channel 0 in both the garden and gameplay. The garden foreground and
background are precomposed into independent static bitplanes and moved by
bitplane pointers and BPLCON1 while the player wakes above them. The vertical
well-entry shaft is likewise a cached bitplane addressed directly by its camera
position. It embeds and validates the recovered room, object/event, and
builder-pattern packs with the same endian-safe readers as the SDL reference.
The player alternates between recovered idle and air frames to make paired
copper-list and sprite-pointer swaps visible. Port-1 joystick motion drives a
small allocation-free 8.8 fixed-point player state: left/right accelerate and
decelerate, the fire button jumps from the grounded state, releasing it shortens
the jump, and later airborne press edges consume gunboot charge and add recoil.
Landing restores all eight charges. This is the first target-specialized
replacement for the host reference's floating-point GameMaker values. An
airborne press emits a bounded downward bullet on a shared actor sprite
channel and redraws both the side-panel ammo digit and its bottom-up vertical
meter; channel 0 remains dedicated to the player. Collected physical gems also
update the recovered 300-frame streak overlay on the fixed HUD playfield.

Its machine lifecycle is owned by the shared-style `AmigaPlatform` class and
follows Rescue on Fractalus: allocation and the two
`WaitTOF()` calls which retire the Workbench display happen before takeover;
the live program owns the raw VERTB vector, forbids multitasking, and waits on
its own interrupt counter. The platform publishes completed copper programs
through `AmigaHardware::setCopperList(list, false)`; hardware restarts them at
the next VBlank. The interrupt handler only acknowledges VERTB and advances
the counter. Each scene finishes its frame before another scene can publish.
Teardown stops
the display, restores `graphics.library`'s startup copper and the saved
interrupt/DMA state, permits multitasking, and only then restores the View.
It exits back to Workbench on a fresh left-mouse click.

PAL timing is intentionally direct: one simulation step will run per 50 Hz
VERTB. There is no 60-to-50 Hz catch-up scheduler, so the preserved per-step
game behavior runs at five-sixths of the Windows version's wall-clock speed.

`make verify-cross-replay` replaces joystick sampling only in a temporary
verification build, executes the shared 64-frame replay on an emulated A500,
and compares its rolling native-state digest with the host result. The normal
68000 executable is rebuilt immediately after a passing comparison.

`make verify-amiga-scenes` similarly substitutes deterministic input only in a
temporary build. Under an emulated A500 it traverses the complete timed splash
sequence, wakes the player, enters the well, waits for the title and camera
sequence, accepts the default style, lands on the recovered entrance platform,
then walks into more than 256 pixels of circular-buffer scrolling. It stops
only after a generated enemy is awake and that gameplay frame and copper list
have been published. It also restores the normal playable executable after
passing.

Automated startup/scene runs have a 120-second wall-clock budget, including
emulator startup. `VERIFY_TIMEOUT_SECONDS` can set a shorter test-specific
budget. Timeout, interruption, and completion clean up the verifier's emulator
and debugger; interactive runs are not time-limited.

The scene verifier checks the selected copper program after every complete
splash iteration: all 180 logo frames, 180 credits frames, 240 controls frames,
and both 60-frame fades. This catches a later branch overwriting a splash
publication, which a capture taken immediately after the first publish misses.
The original failure was a fall-through from the splash branch into the
gameplay transition publisher: it replaced the intended list with one pointing
at the HUD/credits raster for the entire stage. Splash, menu, and style branches
must finish their iteration without reaching the gameplay publisher.
Removing the splash branch's terminating `continue` makes this verifier report
600 overwritten frames; the corrected scene sequence passes.

`make verify-amiga-credits-frame` stops on splash stage 3 and captures the
fixed 384x284 two-plane HUD backing raster.  Its decoder applies the real
16-pixel hardware crop and writes `build/amiga-credits-frame.png`, pinning the
recovered 4:3 placement of the three contributor blocks.  The frame is
precomposed before it becomes visible and remains unchanged for the whole
splash stage.

Splash stages 5 and 6 retain GameMaker's 0.3-frame dither progression without
touching the displayed raster.  Twelve 176-byte mask tiles are prepared before
takeover; a third bitplane and copper pointer resets repeat the selected
8-line tile down the 284-line narrow display.  The controls and blank base
planes remain immutable throughout both fades.

`make verify-amiga-style-frame` stops on the first stable default-style frame,
dumps the exact 176x284 two-plane Chip RAM image through GDB, verifies its
pinned SHA-256, decodes the visible 160-pixel aperture to
`build/amiga-style-frame.png`, and restores the normal executable. This makes
style clipping, alignment, palette use, and text composition inspectable
without relying on a host-window screenshot.

`make verify-amiga-locked-style-frame` then moves the carousel to the locked
second style and pins that complete raster independently.  The deterministic
scene traversal now visits this state before returning to the default style;
this guards the single-frame `sprLocked` resource, both selection arrows, the
`LOCKED!` label, and the smoothed carousel offset.

`make verify-amiga-selected-style-frame` captures the first centered frame of
the accepted-style transition.  Its pinned raster guards the descending player
and the 80-line dither wipe before gameplay takes over, then restores the normal
executable like the other focused frame checks.

Native gameplay now clamps health at zero, switches to recovered `sprPlDead`,
disables player input, and freezes the camera.  The runtime retains the
recovered 90-frame death animation alarm followed by the 10-frame menu delay;
the death-menu presentation itself is the next layer above that explicit gate.
The preceding invulnerability interval follows `dFlash` by alternating the
player hardware sprite between visible and hidden records, without redrawing
either playfield.

Ground movement switches hardware sprite 0 from `sprPlayerIdle` to the
eight-frame `sprPlayerRunExg` sequence.  Its integer phase advances by the
recovered 0.3 frame per simulation step and resets to frame zero whenever the
player re-enters the running state, so animation adds no playfield work and no
fixed-point approximation drift.
The four-frame `sprPlayerIdle` sequence similarly advances at its recovered
0.1 frame per simulation step while the player remains stationary.
Each gunboot discharge resets the four-frame `sprPlayerShoot` sequence, which
then advances at 0.25 frame per simulation step and holds its final pose until
landing.  This too only updates hardware sprite 0.

Area-one actor sprite records retain each recovered state's real cycle length:
single-frame airborne poses, two-frame crawler and frog-air poses, four-frame
idle/flying/crawling poses, and the turtle's six-frame walk. Frog air frames
remain selected directly from vertical direction, matching its GML Step event.

When the grounded player stops, the native fixed-point controller performs the
recovered five-pixel forward support probe. Missing support selects the
five-frame `sprPlayerBalancing` hardware-sprite cycle at 0.25 frame per step;
support or movement returns to the ordinary idle/run states.

`make verify-amiga-menu-wake-frame` captures the first walking garden frame as
the two immutable 512x384 cache planes, their independent pointer/fine-scroll
state, and hardware sprite 0.  Its host decoder reconstructs the centered
160x284 aperture in `build/amiga-menu-wake-frame.png` and pins the complete
capture hash.  This also guards the monochrome cache rule: recovered white art
is retained while opaque source black clears the layer instead of becoming a
solid white silhouette.

`make verify-amiga-menu-title-frame` extends the same reconstruction with all
seven hardware-sprite strips of the fully revealed Downwell logo.  Its pinned
capture proves the logo is present and centered while the goal-stopped player
is correctly absent, before the vertical well-entry renderer takes over.

`make verify-amiga-menu-entry-frame` captures the first frame after that
renderer takes over: one immutable 176-pixel shaft plane addressed directly at
the current vertical camera position, plus the seven title sprite strips.  The
decoded `build/amiga-menu-entry-frame.png` therefore guards the handoff from
the independently scrolled garden planes to pointer-only vertical scrolling
without introducing a repaint path.

`make verify-amiga-gameplay-frame` stops on the first published frame where
the player has landed.  It captures both 320-line circular game planes, both
fixed HUD planes, the live ring origin, and all eight hardware-sprite channels.
The host decoder reconstructs the 352x284 fetched aperture in
`build/amiga-gameplay-frame.png`; its pinned hash covers the solid landing,
dual-playfield HUD composition, player, and gunboot shot together.  The normal
playable executable is rebuilt after the check.

`make verify-amiga-gameplay-active-frame` continues the same deterministic
run through more than 256 pixels of camera movement, alternating steering and
edge-triggered jumps/shots so the player cannot park indefinitely on the
entrance geometry.  It stops only when a generated enemy is awake, then uses
the same complete ring/HUD/sprite reconstruction to produce
`build/amiga-gameplay-active-frame.png`.  This pins a frame after circular
scrolling, hidden-row refills, actor allocation, and enemy activation have all
become live.
