# Effect surface composition contract

Authority: VM assembly in the pinned Windows extraction, not the current SDL
sprite list. `EffectSurfacePlacement` implements routing and coordinates;
the live controller composition uses it on both backends.

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

Visibility has an exact-integer specialization when both coordinates and all
four view fields are integral binary64 values with magnitude below 2^21.
Within that domain the edge sums and comparison differences are exact and fit
signed 32 bits; every nonzero difference is outside the runner's epsilon band.
Strict integer inequalities therefore give the same result without six software
binary64 additions. Signed zero is accepted. Fractional, larger and unsupported
values continue through the checked real-arithmetic path; they are never rounded
into this specialization. Placement and world-space shade snapping are unchanged.

Camera subtraction also specializes this exact-integer domain, preserving the
negative-zero result of `-0 - +0`. Shade snapping retains even world positions;
odd positions choose the neighbouring multiple of four, equivalent to
nearest-even rounding of `world/2` before multiplying by two. Zero is then
published as positive zero, as in the general path. Both helpers retain checked
binary64 fallbacks outside the domain; no fractional input is quantized early.

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

`objEnterDither` instead calls `drawOnSurface(global.surfaceFx)`. That script
checks surface existence, then draws at world-minus-view coordinates without
`scrInView`, death or low-spec guards. Its origin may be outside the viewport
while its 800-pixel-high image overlaps it. Opaque black must replace covered
surface pixels, not erase alpha or draw white. The source helper is pinned by
`verify-runner-instance-ids.py`.

`prepareWithSurfaceProducers` admits these separately supplied producers after
the ordinary late effects and before shade subtraction. Normal drawFx families
still require direct world drawing in death/low-spec mode; unconditional surface
producers can coexist with that requirement. A missing surface skips enumeration,
and failed enumeration discards partial composition. Synthetic pixel tests check
offscreen origins, black replacement, transparent holes and these mode gates.
This is the shared composition route, not admission of natural entry-dither
births or its transformed source artwork. Native integration must preserve the
immutable pointer-scrolled cover rather than rasterizing a full wipe each tick.

The host Game renderer now consumes registered entry-dither records through
this route. `EffectMaskAssets` borrows the same offline mask bank and aliases
its opacity as both color planes (opaque-black pen 3), without allocating or
converting textures. `EntryDitherMask` supplies the shared window calculation
used by the Amiga too. The compact representation only admits the verified
centered, unit-scale, integer-position default-depth case; other transforms or
missing art fail rather than silently showing an incorrect wipe. Game render
tests create the effect through player Create and compare full coverage at the
initial black frame and across the reveal, including transparent pixels.
The stored runtime owns the normal player-created entry lifetime and the host
executable loads the bank. Create follows camera creation; ordinary room alarms,
Step and cleanup maintain the effect without a scoped test dispatcher. Native
automatic birth/Step integration remains separate from its verified registered
lifetime and immutable-bank presentation path below.

The native scene's `entryMask` entry point accepts that immutable 31,104-byte
Chip-RAM bank and a view-relative Y. It rejects null/unaligned pointers, wrong
lengths and unsupported finite-sprite positions before patching the selected
copper slot. Valid calls reuse the existing two mask-plane pointer patches and
display-mode patch; neither a copper rebuild nor bitmap conversion is needed.
The caller must retain the bank until both displayed slots stop referencing it.
Scene tests use copper-method doubles; pixel tests compare 917 viewports with
the source sprite. `make verify-amiga-entry-wipe` additionally runs a native
presentation fixture through 25 covered frames, covering the lowest supported
position, the edge sweep and fully transparent padding, followed by two frames
detaching the alternating copper slots. It checks both mask DMA pointers,
six-plane mode and unchanged bank bytes, then four-plane mode and cleared mask
pointers on each detached slot. `coverDetached()` requires both lists to be
valid and unreferenced; failed updates cannot authorize reuse. This query does
not itself establish a DMA fence: the caller must publish the detached list and
cross VBlank before releasing the resident bank. The fixture verifies that
shop-bank selection remains rejected before that fence, then replaces the bank
with shop artwork and checks the copied bytes after it.
The test uses the retired-garden overlay region, with a distinct resident-cover
tag that prevents shop/results or the exit cover from overwriting an entry bank.
It has a 45-second emulator deadline (configurable up to 60), stops its emulator,
and restores the normal build. The A500 test configuration has 512 KiB Chip and
8 MiB Fast RAM; this is neither a stock-memory proof nor a frame-budget result.
The immutable entry asset adds 31,104 bytes to executable data, with no new
Chip allocation. Registered entry Draw records now select that bank through
the ordinary renderer. When both copper slots are detached, the next frame's
VBlank releases its resident tag before other overlays may claim the storage.
Automatic native lifetime admission and production entry-wipe timing remain
open, as does support for simultaneously required entry/exit covers or overlay
banks; these combinations fail rather than overwrite active DMA storage.

`GameplayEntryDithers` now supplies native typed lifetimes using the same
`EntryDither` Create/Step/Alarm implementation as the stored adapter. Its two
fixed slots retain original numeric IDs separately from monotonic generations.
Each coordinate setter publishes to the resource-backed owner before the next
source read. The mixed-family alarm traversal includes these records. The normal
gameplay tick captures active entry lifetimes after alarms, updates them after
camMain (64) and before builder Step, and rechecks each lifetime before dispatch.
Reused slots retain registration order through monotonic generations. Camera X
is narrowed from the existing Q12 adapter to binary32 before the effect reads
it; rounded display-window coordinates are not used. This does not establish
original camera arithmetic or alternate-camera object selection. Inactive and
destroyed records do not advance. Alarm expiry marks destruction, with actual
retirement at Draw cleanup; retry and next-level teardown clear the owner too.
The layer visitor retains the original -200000 depth and does not origin-cull
the effect. Automatic production creation still requires source-ordered numeric
allocation and the initial camera read. The BOB
draw list extracts a separate cover command rather than producing a BOB.
`EntryDitherPlacement` shares centered/integral placement validation between
SDL and Amiga. Multiple covers, unsupported positions and non-default depth
are rejected rather than flattened into a different picture.

The immutable hardware path is eligible only when no admitted enemy shade is
visible (or noControl suppresses shade). Otherwise the native renderer adds
the black entry mask to the same shared surface compositor as SDL, before
shade subtraction, and publishes its changed coverage through the existing
effect renderer. Death does not suppress the entry producer or shade holes.
This fallback preserves the currently implemented alpha contract; it is not a
20 ms performance claim or full original-GPU raster verification. Tests check
hardware eligibility at visible/offscreen enemy positions and black-cover
shade subtraction with normal, noControl and death settings.

Resource-backed tests exercise 120 alarm updates, 119 Steps, activation,
offscreen Draw eligibility, cleanup, generation-safe reuse, retry and failed
Create prefixes. The bounded entry-wipe verifier also runs an explicit-ID
native lifetime fixture and retires it before testing the cover presentation.
The presentation sweep now supplies registered records at fixture positions
and exercises normal cover-command extraction and copper publication; the
verifier does not patch its own alternative mask into the display list.
That fixture does not establish natural event ordering or entry-wipe timing.

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

`prepareWithLateEffects` additionally accepts source-ordered surface producers
between smoke and controller publication, before shade subtraction. The caller
supplies resolved offline artwork and exact published world coordinates; this
entry point does not choose subimages, round coordinates or enumerate families.
The scalar composition test overlays all fourteen debris frames on existing
effects across camera/flag combinations, checking palette replacement and shade
holes. Enumeration failure discards partial coverage. Direct fallback and absent
surfaces do not enumerate these producers. This establishes the composition
mechanism, not natural debris births, live global Draw order or Windows GPU
sampling; those remain integration and verification work.

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
| objEnterDither (surface producer) | -200000 |
| objControlerN (surface publication) | -300000 |

Flash balls have no parent or Draw override and their Create does not change
depth. Their default sprite draw belongs ahead of the player and ordinary
actors, but behind the controller's surface publication. Both port draw lists
place flashes at that boundary. This does not establish equal-depth instance
ordering or fix every existing actor family ordering. Smoke's death/low-spec
direct fallback has a different boundary from controller-composited smoke.

Entry dither contributes to surfaceFx at -200000, after ordinary debris (-1000)
and before the controller (-300000). Its shared depth with flash balls does not
make them interchangeable: flash draws directly, whereas the dither's pixels
are presented by the later controller. `visitEntryDitherDraw` uses the stored
world's source-layer traversal and carries the resolved depth and published
record to the renderer. The live room-entry test verifies this lifetime remains
drawable with an offscreen origin and disappears on its destruction mark.
Native mixed-family Draw ownership and final pixel composition remain open.

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
producers such as currency. Its default storage keeps separate alpha and two
color planes in ordinary RAM (at most 18,300 bytes including row bounds and
optional backend bindings). An incoming
mask replaces the old colors under that mask; colors are not ORed together.
Opaque black is distinct from transparent coverage. Shade removes alpha only,
and publication exposes color bits only under surviving alpha. Mask, low and
high source planes share dimensions/stride and must each have the supplied
mask byte length available. No source payload is mutated or converted.

Stamping resolves the destination row once and accumulates its touched word
range locally before publishing row bounds. Fully transparent source words
need no color reads or destination work. In the scalar path, surviving in-bounds alpha
expands these ranges; shade subtraction never expands them. Clearing and
publication therefore retain the same sparse coverage contract, including
rows whose alpha has subsequently been erased completely.

`AmigaGameplayEffects` accepts both white-only and palette coverage. Its palette
path publishes the final alpha/low/high word directly: clipping, ring addressing
and horizontal alignment are computed once, then each plane uses masked
replacement. Opaque black remains covered; colour bits outside alpha are ignored.
The white-only API uses the same primitive. Neither path reconstructs separate
colour passes or allocates a bitmap. Per-buffer physical-row debt is unchanged.
It preserves terrain backing,
HUD and untouched pixels; neither variant changes display pointers or allocates
Chip RAM itself. `verify-palette-effects` exercises original gem/smoke/shade art
and all pens, including opaque-black overlap and borrowed padded storage.

### Native planar composition

`AmigaEffectSurface` binds a separate 20,448-byte Chip workspace at startup:
three 192x284 planes, with one guard word on each side of the 160-pixel view.
The ordinary-RAM owner retains row metadata and its scalar fallback storage.
The workspace follows the resident asset pack in the same allocation and is
released with it; allocation failure retains the return-code-20 path.

Immutable Chip-resident artwork uses the shared framework's masked Bitmap
copies for both color planes and masked pattern writes for alpha. Shade clears
alpha without reconstructing color. Clipping changes source/destination
rectangles, not artwork. Guard words accommodate shifted fetch spans. Queued
operations preserve producer order; CPU fallback and overlap queries
drain the queue before touching storage. Unsupported masks use the
same scalar stamping into that storage, not a second displayed representation.

DMA stamps conservatively expand row bounds to their clipped rectangles.
Clearing uses the prior vertical extent and includes guard words; alpha still
decides visibility, overlap and publication. The controller painter boundary,
ring addressing and per-display restoration debt remain unchanged. Overlap
testing is still a CPU operation, not a blitter-zero test.

An overlap query outside the viewport or the producer's retained row extent
does not read pixels and therefore does not drain queued work. Nonempty
candidate regions synchronize before reading final alpha. This distinction is
important with queued clearing/composition: metadata rejection is immediate,
but a pixel-dependent answer must observe completed stamps and shade clearing.

The [Hardware Reference Manual, Zero Flag](https://www.theflatnet.de/pub/cbm/amiga/AmigaDevDocs/hard_6.html)
permits read-only zero detection with destination DMA disabled, after the blit
completes. That does not make rectangle area alone a sufficient dispatch policy.
`CurrencySmokeComposition` also uses the overlap API for whole-surface visibility,
where a CPU scan can stop at the first occupied word; a hardware test consumes
the entire rectangle. Distinguish that consumer from sparse negative-overlap
tests, account for queue draining/register setup, and measure dense workloads
before replacing the current scalar scan.

Final publication uses the same queued Bitmap masked copies, with the alpha
plane shared by the two color planes. Producer metadata supplies conservative
rectangles: contiguous occupied row bands share their horizontal union, while
gaps remain undamaged. Each rectangle splits at the circular display wrap and
marks only that display buffer's restoration debt. No CPU planar scan or
intermediate asset conversion is needed. Alpha, including shade holes and
opaque black, controls the final writes. The CPU word publisher remains the
fallback for surfaces not bound to the native workspace and the verification
oracle. This changes neither the controller's ordering nor the terrain backing.

The native BOB verifier mirrors every requested stamp into an independent
ordinary-RAM scalar surface and compares final alpha and covered color bits.
Its startup fixtures include all shifts, viewport edges, overlapping gem/smoke
art and shade holes, followed by live-frame comparisons. They establish
backend equivalence for those paths, not original Windows shader fidelity or
the complete frame budget.

The native verifier also snapshots the display immediately before effect
publication, applies the old CPU publisher to that snapshot, then compares the
entire two-plane ring after DMA drains. The oracle storage exists only in the
verification build. This detects changes outside the effect rectangles as well
as mismatched effect pixels; it does not prove every possible dense encounter
or eliminate the need for separate timing tapes.

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

Clear visits only the previous touched word spans. The palette surface also
tracks a conservative first/last touched row, making clear/publication of an
untouched surface constant-time. Shade stamps do not expand these ranges;
complete subtraction retains the range until clear so stale color words are
still erased. Overlap queries stop at the first covered pixel. The consumer
receives only nonzero result words with viewport
coordinates. Consumers still own retained-background restoration, final painter
order and publishing pixels to the target buffer. This helper does not allocate
Chip RAM, rebuild DMA sprites, scroll a framebuffer or consume game RNG.

Publishing uses `Renderer::drawCoverageWord` on the SDL path and
`amigaRendererDrawCoverageWord` on the native path. Both preserve unset bits.
The native operation clips a 16-bit span, splits unaligned output over at most
two destination words, maps Y through the current circular-buffer origin, and
updates only changed plane words. It does not call the blitter. Owners must
restore the previous effect damage from retained terrain before publishing the
new frame. Natural smoke creation and collision dispatch remain separate from
rendering the smoke owner.

`AmigaGameplayEffects::draw` binds a copy of the renderer to the retired display
ring and publishes white coverage at the shaft offset. It drains outstanding
blits once before the first CPU write, then records contiguous touched-row runs
in `AmigaGameplayRows` for that buffer only. An empty surface neither drains nor
adds damage. Before that buffer is reused, its normal terrain sync restores the
previous physical rows, independent of the new camera origin or effect lifetime.
The backing and HUD remain untouched. The live scene places this operation after
ordinary actors and flash balls. Sprite assignment tests final effect alpha
against each candidate's canvas, stopping at the first overlap. Only that actor
and actors behind it fall back to planar drawing; the non-overlapping frontmost
suffix remains eligible for DMA sprites. Shade holes and cleared surfaces do
not block assignment, while opaque black does. Canvas bounds are conservative:
transparent sprite pixels may still cause fallback. This preserves order, not
a demonstrated 20 ms budget. `make test-amiga-gameplay-effects` exercises publication followed by
restoration across alternating rings, clipping, wrapping and effect retirement.

Reviewed assembly SHA-256 values:

- `gml_GlobalScript_drawFx.asm`: `6e3dceea4db4e2baaf76c5d4103bd4ed930042ae769d209369d98246162b5d92`
- `gml_GlobalScript_drawShade.asm`: `c0aa27477ef320ed8e7f9bcc38bd4d468d0b84ed9dedd2d7bacd1e1844585c82`
- `gml_GlobalScript_scrInView.asm`: `cf66675bc8ea809f5bd42270ceee29d3e50eb9c3aa9d916205006db32e0e3432`
- `gml_Object_objControlerN_Draw_0.asm`: `7d0a0fa470414853da86b2dc0facfb9ae90e03493cb857e0abb4d3bf771653e9`
- `gml_Object_objControlerN_Step_2.asm`: `a684ff2052be352cd198317c5f5a62a532eb0abca03dba2da6418bc5dfa8152f`
