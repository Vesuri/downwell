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

Therefore an ordinary smoke overlay is not a faithful replacement. Global draw
ordering, sampling and surface ownership still need live verification; routing
tests alone do not prove pixels.

## Pinned Windows blend path

`verify-runner-effect-blend.py` pins the executable hash, registration at
`0x519d41`, mode wrapper at `0x518ab0`, state cache/flush, backend dispatch,
factor translators and blend-descriptor construction. Mode 3 assigns internal
source factor 1 and destination factor 4 for both RGB and alpha. The RGB
translator at `0x66c390` produces D3D11 factors 1/4; the alpha translator at
`0x66c440` produces 1/6. Descriptor construction at `0x66c07e` sets both blend
operations to ADD. This is **not** a D3D subtract operation.

The [D3D11 factor definitions](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_blend)
identify these as ZERO/INV_SRC_COLOR for RGB and ZERO/INV_SRC_ALPHA for alpha.
The [descriptor contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_render_target_blend_desc)
confirms that color and alpha use separate factors. Consequently:

- RGB output is destination RGB times one minus source RGB.
- Alpha output is destination alpha times one minus source alpha.
- Normal mode restores SRC_ALPHA/INV_SRC_ALPHA for RGB and alpha.

Source sprite 117 (`sprLight`) is 96 by 96, origin (48,48), with only transparent
black and opaque black texels. It is exported offline as a planar masked asset,
not as a 96-pixel-wide hardware sprite. Its alpha coverage can suppress opaque
smoke with `smokeCoverage & ~shadeCoverage`. Under binary sampled alpha, surviving
smoke remains opaque and covered smoke becomes transparent; no grayscale fade or
RGB subtraction approximation is needed. Overlapping opaque shade coverage unions.
This conclusion is limited to this opaque/binary path, not all particle alpha,
shader, filtering or surface-recreation cases. It is static binary/API evidence,
not a captured execution of Windows rendering.

## Binary coverage staging

`BinaryEffectSurface` accumulates offline MSB-first smoke masks with wordwise OR,
then subtracts shade coverage with AND NOT. All additions precede subtraction;
interleaving them would allow a later smoke stamp to undo an earlier shade cut.
The default viewport is 160 by 284. This scratch lives in ordinary RAM, not in
a displayed bitplane or the AmigaDOS stack. It holds one coverage plane and
per-row dirty word bounds (at most 6,820 bytes), not two full color surfaces.

Clear visits only the previous touched word spans. Shade stamps do not expand
those spans; the consumer receives only nonzero result words with viewport
coordinates. Consumers still own retained-background restoration, final painter
order and publishing pixels to the target buffer. This helper does not allocate
Chip RAM, rebuild DMA sprites, scroll a framebuffer or consume game RNG.

Publishing uses `Renderer::drawCoverageWord` on the SDL path and
`amigaRendererDrawCoverageWord` on the native path. Both preserve unset bits.
The native operation clips a 16-bit span, splits unaligned output over at most
two destination words, maps Y through the current circular-buffer origin, and
updates only changed plane words. It does not call the blitter. Owners must
restore the previous effect damage from retained terrain before publishing the
new frame; these publishing primitives do not yet schedule live effects.

Reviewed assembly SHA-256 values:

- `gml_GlobalScript_drawFx.asm`: `6e3dceea4db4e2baaf76c5d4103bd4ed930042ae769d209369d98246162b5d92`
- `gml_GlobalScript_drawShade.asm`: `c0aa27477ef320ed8e7f9bcc38bd4d468d0b84ed9dedd2d7bacd1e1844585c82`
- `gml_GlobalScript_scrInView.asm`: `cf66675bc8ea809f5bd42270ceee29d3e50eb9c3aa9d916205006db32e0e3432`
- `gml_Object_objControlerN_Draw_0.asm`: `7d0a0fa470414853da86b2dc0facfb9ae90e03493cb857e0abb4d3bf771653e9`
- `gml_Object_objControlerN_Step_2.asm`: `a684ff2052be352cd198317c5f5a62a532eb0abca03dba2da6418bc5dfa8152f`
