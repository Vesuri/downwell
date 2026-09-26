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

Currency is also a `drawFx` producer, not an independent direct sprite in the
normal path. `objCurrency_Draw_0` selects sprite 133/134 for small/large gems,
updates its collision mask, and calls `drawFx` only when `dflash` is false.
Its compatibility depth 10000 precedes smoke at 1000, so smoke can replace gem
pixels on the surface before the controller subtracts shade. A smoke-only
overlay above directly drawn gems would incorrectly leave gem pixels visible
inside shade holes. Live integration must migrate both producers together.
The currency Draw assembly is pinned by `verify-effect-depths.py`.

`CurrencySmokeComposition` provides the shared colored controller surface.
Callers enumerate visible (non-dflash) currency in original Draw order using
published coordinates, followed by the smoke owner; shade is applied only after
both producers. Supply the backend's raster sampling policy explicitly. Its
ordinary-RAM surface owns no DMA storage and never changes simulation or RNG.
Death/low-spec requests return `directRequired()` with an empty surface: the
backend must draw each producer at its own world-depth boundary, not flatten
them into a controller overlay. A missing normal effect surface remains empty.
Preparation failure clears partial output so it cannot publish stale pixels.

Currency Step publishes rounded `xx`/`yy` into its builtin `x`/`y` before Draw;
the private integration fractions must not be passed directly to `drawFx` or
truncated at drawing time. Both adapters use `GemMotion::rounded` for that
publication. Its signed nearest-even operation is applied relative to the even
integer main-room origin (240,176), preserving absolute tie parity. The source
Step tail is pinned alongside Draw, and live combat comparisons include both
published coordinates as well as the private motion state.

### Compatibility depths

Do not use the zero `Depth` fields in the GMS2 object records as draw order.
`__global_object_depths` builds parallel depth/name arrays, resolves names with
`asset_get_index`, and writes the resource-indexed `__objectID2Depth` table.
The compatibility `instance_create` calls `object_get_depth` and then
`instance_create_depth`. The controller's persistent rmInit instance also has
its serialized layer at -300000. Lower depths draw in front:

| Object | Compatibility depth |
|---|---:|
| enmSkeleton | 14000 |
| objCurrency | 10000 |
| fxSmoke (direct fallback) | 1000 |
| bulletStandard / bulletEnmBone | 0 |
| objPlayer_n | -50000 |
| fxFlashBall | -200000 |
| objControlerN (surface publication) | -300000 |

Flash balls have no parent or Draw override and their Create does not change
depth. Their default sprite draw belongs ahead of the player and ordinary
actors, but behind the controller's surface publication. Both port draw lists
place flashes at that boundary. This does not establish equal-depth instance
ordering or fix every existing actor family ordering. Smoke's death/low-spec
direct fallback has a different boundary from controller-composited smoke.

`make verify-effect-depths` checks the 342 table entries from VM assembly,
pins the reviewed compatibility functions and flash Create, and checks the
controller's room-layer depth and persistence against the extraction manifest.

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

`PaletteEffectSurface` extends the opaque/binary-alpha composition to colored
producers such as currency. It keeps separate alpha and two color planes in
ordinary RAM (at most 18,200 bytes including dirty-row bounds). An incoming
mask replaces the old colors under that mask; colors are not ORed together.
Opaque black is distinct from transparent coverage. Shade removes alpha only,
and publication exposes color bits only under surviving alpha. Mask, low and
high source planes share dimensions/stride and must each have the supplied
mask byte length available. No source payload is mutated or converted.

`AmigaGameplayEffects` accepts both white-only and palette coverage. Its palette
path divides a word into disjoint pen masks and uses the same CPU masked-word
publisher and per-buffer physical-row debt. It preserves terrain backing,
HUD and untouched pixels; neither variant changes display pointers or allocates
Chip RAM. `verify-palette-effects` exercises original gem/smoke/shade art and
all pens, including opaque-black overlap. The live scene still needs joint
currency/smoke binding, source visibility and painter-boundary integration.

`EffectMaskAssets` borrows all nine sprite-84 smoke frames, sprite-117 shade,
and all four frames of each currency sprite (133/134) directly from the immutable
offline DWAP pack. Each descriptor exposes both color planes and its coverage
mask; these must stay together so opaque black is not treated as transparent.
Binding requires all eighteen records. It validates the version,
source-hash prefix, sorted directory, payload boundaries, planar dimensions and
three-plane record flags before exposing descriptors. Signed cropped origins
are retained, including the last smoke frame; consumers subtract those origins
from the source anchor rather than recentering the visible pixels. A failed
rebind invalidates all accessors. The pack must outlive the descriptors.

This shared reader performs no pixel conversion, allocation or Chip-memory
copy, and directory validation happens once at binding, not per frame. Host and
native adapters can use the same offline bytes. The composition regression
uses these descriptors and independently checks them against the pack directory;
live scene asset binding remains to be connected.

`SmokeComposition` prepares a read-only snapshot of the smoke owner's published
builtin coordinates and animation frames. It unions every routed smoke mask
before subtracting shade coverage, and reports a distinct `direct` or
`controller` painter boundary. Death/low-spec draws neither require the effect
surface nor receive shade subtraction. Missing surfaces and strict view-origin
tests retain the `drawFx` routing contract. The caller enumerates only instances
whose source Draw invokes `drawShade`, and supplies the verified screen-sampling
policy; this helper does not establish Windows raster rounding by itself.

No game RNG or simulation state is modified. Empty owners and failed preparation
clear old coverage; consumers must not publish a failed frame. The ordinary-RAM
scratch belongs to the persistent scene, not a target task-stack local. The
retained display's previous damage is still repaid independently by its backend.
`make verify-smoke-composition` compares complete smoke lifetimes and all routing
flags with an independent scalar mask oracle. This preparation path is not yet
connected to live scene rendering.

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

`AmigaGameplayEffects::draw` binds a copy of the renderer to the retired display
ring and publishes white coverage at the shaft offset. It drains outstanding
blits once before the first CPU write, then records contiguous touched-row runs
in `AmigaGameplayRows` for that buffer only. An empty surface neither drains nor
adds damage. Before that buffer is reused, its normal terrain sync restores the
previous physical rows, independent of the new camera origin or effect lifetime.
The backing and HUD remain untouched. The caller must place this operation at
the source controller's painter-order boundary; it is not yet called by the
live scene. `make test-amiga-gameplay-effects` exercises publication followed by
restoration across alternating rings, clipping, wrapping and effect retirement.

Reviewed assembly SHA-256 values:

- `gml_GlobalScript_drawFx.asm`: `6e3dceea4db4e2baaf76c5d4103bd4ed930042ae769d209369d98246162b5d92`
- `gml_GlobalScript_drawShade.asm`: `c0aa27477ef320ed8e7f9bcc38bd4d468d0b84ed9dedd2d7bacd1e1844585c82`
- `gml_GlobalScript_scrInView.asm`: `cf66675bc8ea809f5bd42270ceee29d3e50eb9c3aa9d916205006db32e0e3432`
- `gml_Object_objControlerN_Draw_0.asm`: `7d0a0fa470414853da86b2dc0facfb9ae90e03493cb857e0abb4d3bf771653e9`
- `gml_Object_objControlerN_Step_2.asm`: `a684ff2052be352cd198317c5f5a62a532eb0abca03dba2da6418bc5dfa8152f`
