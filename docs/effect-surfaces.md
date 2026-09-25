# Effect surface composition contract

Authority: VM assembly in the pinned Windows extraction, not the current SDL
sprite list. `EffectSurfacePlacement` implements routing and coordinates only;
it is not yet connected to either live renderer.

## Routing and coordinates

`drawFx` draws directly with `draw_self` when `global.death` or `global.lowSpec`
is true. This branch does not call the visibility test or require a surface.
Otherwise it draws only when `scrInView(0,0,0)` succeeds and `surfaceFx` exists.
The surface draw uses the source sprite/frame, angle, scales and alpha, a white
color multiplier, and world coordinates minus the view origin. Missing surfaces
do **not** fall back to a direct draw.

The visibility test uses the instance origin, not its sprite bounds. All four
comparisons are strict runner real comparisons; edge equality and its epsilon
band are excluded. Fractional camera coordinates are retained.

`drawShade` is enabled only when not low-spec, in view, not `noControl`, and
`surfaceShade` exists. It draws sprite 117, frame zero, at
`round(world / 2) * 2 - viewOrigin` on each axis. Snap world coordinates before
subtracting the camera. The round operation uses nearest-even ties, including
negative coordinates. It does not use the actor's sprite, scale, angle or alpha.
There is no death check in this helper. Both `subparentEnemy` and `bulletEnmBone`
call `drawShade` before their ordinary `draw_self`.

## Surface lifetime and ordering

Controller End Step calls `surfaceClear` for effects, dissipate, buttons and
shade. That helper clears existing surfaces to transparent black; it does not
create missing surfaces.

Controller Draw sets blend mode 3 (`bm_subtract` in the decompiled view), draws
the shade surface onto effects and buttons, then restores mode zero. It presents
effects, dissipate and buttons in that order at the view origin. A missing
surface is created instead of presented in that branch. Surface dimensions are
160 by `g_cameraHeight`.

Therefore an ordinary smoke overlay is not a faithful replacement. Exact GPU
blend/alpha behavior and global draw ordering must be established before choosing
the Amiga composition implementation; routing tests alone do not prove pixels.

Reviewed assembly SHA-256 values:

- `gml_GlobalScript_drawFx.asm`: `6e3dceea4db4e2baaf76c5d4103bd4ed930042ae769d209369d98246162b5d92`
- `gml_GlobalScript_drawShade.asm`: `c0aa27477ef320ed8e7f9bcc38bd4d468d0b84ed9dedd2d7bacd1e1844585c82`
- `gml_GlobalScript_scrInView.asm`: `cf66675bc8ea809f5bd42270ceee29d3e50eb9c3aa9d916205006db32e0e3432`
- `gml_Object_objControlerN_Draw_0.asm`: `7d0a0fa470414853da86b2dc0facfb9ae90e03493cb857e0abb4d3bf771653e9`
- `gml_Object_objControlerN_Step_2.asm`: `a684ff2052be352cd198317c5f5a62a532eb0abca03dba2da6418bc5dfa8152f`
