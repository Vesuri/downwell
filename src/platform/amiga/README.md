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
sprite channel 0. It embeds and validates the recovered room, object/event, and
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
its own interrupt counter. Normal copper swaps publish COP1LC without a
COPJMP1 strobe and therefore become live at the next vertical blank. Teardown
stops the display, restores `graphics.library`'s startup copper and the saved
interrupt/DMA state, permits multitasking, and only then restores the View.
It exits back to Workbench on a fresh left-mouse click.

PAL timing is intentionally direct: one simulation step will run per 50 Hz
VERTB. There is no 60-to-50 Hz catch-up scheduler, so the preserved per-step
game behavior runs at five-sixths of the Windows version's wall-clock speed.

`make verify-cross-replay` replaces joystick sampling only in a temporary
verification build, executes the shared 64-frame replay on an emulated A500,
and compares its rolling native-state digest with the host result. The normal
68000 executable is rebuilt immediately after a passing comparison.
