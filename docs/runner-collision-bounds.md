# Windows collision-bound contract

## Transform and mask mutation

VM setters `0x432290`, `0x4322e0` and `0x432090` narrow their numeric arguments
to binary32 before calling scale-X `0x46b6d0`, scale-Y `0x46b780` and angle
`0x46b560`. Each compares the old/new field first: equal values, including the
two signed zeros, return without a write or queue operation. A changed field is
stored without angle normalization, updates flag `0x4000` from unit scales,
zero angle, alpha one and blend `0xffffff`, sets bounds-dirty bit `8`, then calls
`0x501e50`. `RunnerSpatialInstances::changeTransform` implements this admitted
normal/zero domain with explicit alpha/tint inputs and rejects unsupported FP
values rather than guessing their behavior.

Mask setter `0x46bfd0` likewise does nothing for the same integer ID; otherwise
it stores the ID, sets bit `8` and queues eligible work. `changeMask` implements
that integer boundary, including storing missing IDs. Its VM wrapper `0x432500`
first narrows to binary32, promotes again, then uses x87 `fistp`; that numeric
conversion still needs the caller's verified rounding policy. Neither method
performs an immediate bounds refresh. The stored lifecycle adapter exposes both
with lifetime/object checks; production setter-site routing remains open.

Evidence is the pinned Windows executable, checked read-only by
`python3 tools/verify-runner-collision-bounds.py`. The verifier pins reviewed
machine code, not runtime results. Work priorities remain in [open-work.md](open-work.md).

## Entry points and storage

- `bbox_left` registration at `0x430cab` selects getter `0x42d500`;
  `bbox_right` selects `0x42d540`. Dirty bounds call `0x466010`.
- Instance offsets `0xe4/0xe8/0xec/0xf0` store left/top/right/bottom as binary32.
  Position is at `0xb0/0xb4`, scale at `0x9c/0xa0`, angle at `0xa4`.
- The bbox routine selects explicit mask index `0x144`, falling back to sprite
  index `0x84`. A missing resource collapses bounds to the instance position.
- `place_meeting` wrapper `0x52b1f0` converts supplied coordinates to binary32
  and calls `0x4fee30`; this is distinct from reading bbox getter values.
- `0x4fee30..0x4fee79` checks that the caller is an ordinary instance (kind 1),
  calls the same `0x4fe560` search used by `instance_place`, with the final
  collect-all argument zero, then returns whether the resulting pointer is
  non-null. Thus a boolean meeting query retains the search's preparation and
  restoration mutations; it is not permission to replace it with an arbitrary
  resource scan. The verifier pins this reviewed helper too.

## Cache refresh and spatial publication

Native terrain's prepared default geometry follows collision-resource identity,
not drawing-sprite identity. An explicit current/cached sprite still uses the
prepared shape when it equals the default sprite or the object has a fixed
nonnegative mask ID. This follows `SpriteCollisionPack::collisionSprite`'s mask
precedence. Different unmasked sprites, moved query-cache positions and positions
outside the prepared grid retain the general geometry path. Reusing geometry
does not skip dirty-queue visits, cache writes, removal/reinsertion or change
spatial publication order; the scalar owner remains the differential oracle.
`NativeTerrainLifetimes::readCollision` returns a synchronous projection of
identity, spatial flags, current/cached sprite and cached-origin offset. Source
segments and materialized changes remain authoritative; retained constructors
read their existing fields. The projection is not stored between operations and
does not read wall animation state that refresh does not consume. Stale handles
fail without changing the output. The source-record differential exercises this
projection after mutations, retirement and slot reuse against the individual
reference getters.

At `0x466045` the routine saves the old cached bounds before resolving the
mask/sprite. Failed lookup (`0x466077..0x4660b9`) copies position into both
endpoints, clears instance bit `0x200`, and forces the update argument to zero.
The common tail clears bounds-dirty bit `8`. It does **not** remove an existing
spatial entry or clear membership bit `0x8000` on this path.

For a resolved resource, `0x46669c` copies its precise-mask byte into instance
bit `0x200`. After clearing dirty bit `8`, a nonzero update argument invokes
`0x5020a0` with the saved old bounds. Recalculation with update zero changes the
cache/flags but leaves the index untouched. A later publication can therefore
require the index removal helper's full-root retry when the cached old bounds
no longer enclose its stored entry.

`RunnerSpatialBoundsUpdate::finishRefresh` models this finalization, not sprite
lookup or mask geometry. The reviewed ranges are pinned by
`tools/verify-runner-field-lookup.py`; component tests cover missing-resource
birth, retained old entries, restoration and update-disabled refresh.

`RunnerSpatialInstances::refreshSprite` joins compact `DWSC` metadata lookup,
ordinary sprite geometry and this finalization. It applies an explicit mask ID
before the artwork ID; invalid explicit masks do not fall back. A valid pack
with a missing resource takes the source's position-collapse path, while an
invalid pack fails the owner instead of masquerading as an absent sprite.
Resolved special resources are rejected rather than approximated with ordinary
geometry. The caller still owns resource-kind support, source trigonometry and
the integer-rounding policy. Zero (including negative zero) angle bypasses trig;
fractional compatibility-off geometry bypasses integer rounding.

The shared `GameplayCollisionOwner` prepares all 741 immutable sprite
descriptors at construction, before accepting any instance births. The packed
`SpriteCollisionPack` decoder remains the reference and is also usable without
preparation. Its prepared view borrows an owner-resident native-endian table
(26,676 bytes of descriptor payload), outside the Amiga task stack. Copies of
that view borrow the same table; its storage must remain immutable and alive.
Preparation changes no resource IDs, RNG state, instance visibility, bounds
publication or Create ordering. Insufficient preparation capacity fails owner
initialization instead of silently preparing only some resources. Collision
queries and sprite setters use this same prepared view on both backends.

`PreparedObjectDefaults` likewise prepares the 386 immutable object bindings
before births: sprite/mask IDs, binary32 OBJT/compatibility depths, default
property bits, initial instance flags and artwork frame counts (28 bytes per record). The original
`bindObjectFromPacks` path remains the immediate-decoding reference. Prepared
binding still reads current family flags, rebinds membership, installs mask and
depth defaults, executes the sprite setter, and publishes dirty queues in that
order. Dynamic preparation flags, active state, image index, timeline and
membership links are not part of the prepared table. The compatibility Create
depth override is still installed after binding, not substituted for OBJT depth
inside it. Same-sprite rebinding retains the source setter's no-op behavior.

Fresh shared births use `attachObjectPrepared`: attach a new lifetime, initialize
properties, preserve the source membership rebind/revision updates, and install
the prepared initial flags/defaults. For ordinary initial flags (8, optionally
inactive bit 2), no special state and supported frame counts, general sprite
setter/reset machinery is unnecessary: the starting sprite is -1, image index
and timeline are zero, and cached frame is -1. The callback-free binding interval
can coalesce repeated dirty-queue head marks to one final mark. Birth attachment
guarantees the slot is fresh; neither Create nor index publication moves earlier.
Unusual states retain the general prepared-binding path, including failure
ordering. General rebinding remains separate and does not assume fresh state.

`PreparedSpriteGeometry` prepares origin-relative compatibility edges for
unit-scale, unrotated sprites before births. The shared owner uses these only
for integer positions with magnitude below 2^21, with signed-16-bit source
margins/origins and its fixed integer-preserving `roundPixel` policy. Every
intermediate then lies inside +/-2^22 and is exactly representable: sorting and
the far-edge decrement can precede translation. Runtime work is position
validation, translation and final binary32 encoding; publication still uses
the original bounds/index owner. Fractional/large positions, other transforms,
special resources and unavailable templates retain the generic path. Generic
rounding callbacks are never bypassed by `RunnerSpriteBounds` itself. The target
template is 18 bytes (13,338 bytes for 741 entries, plus table metadata).

The sprite-pack gate exercises this adapter across all 741 recovered metadata
records and checks owner flags/cache publication against the full artwork
metadata view. It also covers mask overrides, missing-resource retirement,
rotated geometry supplied with explicit trigonometric inputs, special-resource
rejection and failed rounding. This verifies composition of the decoded data and
geometry routines; it is not a live Windows runtime/FP-mode comparison or proof
that the adapter is wired into gameplay.

`RunnerSpatialInstances::collideCompatibility` joins the ordinary compatibility
branch of `0x464d30` to resource lookup and `RunnerMaskPair`. Self and instance
flags `0x100001` are rejected before refresh (distinct from query candidate
filter `0x100003`). With updates enabled, the other instance refreshes before
self, and only dirty instances refresh; update-disabled calls refresh neither.
Broad phase uses strict binary32 comparisons against inclusive far edges plus
one. It does not normalize/reorder cached rectangles or substitute integer
overlap tests.

`GameplayCollisionOwner::collisionOverlap` exposes that pair test for native
identities without registering duplicate terrain records. Native snapshots
combine actor publication state or run-owned terrain coordinates/cached
geometry with current appearance. Both ownership modes use
`RunnerSpriteOverlap` for the geometric tail, after exclusions and ordered
lazy refresh. The native adapter covers admitted ordinary resources; this API
does not select candidates or enable event scheduling. Native/reference matrix
agreement is not independent evidence of the Windows runtime FP environment.

Only after overlap does the routine resolve masks/artwork and require nonzero
resource frame counts. Two rectangular-mode resources hit without mask pixels
or image-index conversion. If either is precise, both stored mask frames are
selected by truncating image indices then signed-modulo wrapping by mask count.
An unavailable payload fails the owner rather than returning a spurious miss.
Special artwork with an explicit ordinary mask follows that mask; unsupported
special-resource dispatch without an override is rejected. This adapter does
not implement the separate compatibility-off pair routine.

The resource-backed sprite test connects `place` through this adapter to the
pair kernel and obtains a generation-checked field identity. It checks refresh
order, update-disabled cache behavior, mask overrides, direct-versus-query flag
gates and missing payloads. Actual gameplay lifecycle/scheduler hooks and the
runtime FP/trigonometry verification remain separate requirements.

## Unrotated ordinary-sprite branch

`0x46616d..0x466328` treats each axis in this order, narrowing every operation
to binary32 (margins are inclusive):

```
width = (high - low) + 1
near = ((low - origin) * scale) + position
far = (scale * width) + near
```

Compatibility mode rounds both endpoints to integers and converts them back
to binary32, then sorts them, then subtracts one from the far endpoint. In
fractional mode it sorts without rounding or subtracting. Zero scale can
therefore yield reversed inclusive endpoints in compatibility mode; this is
distinct from the spatial index's later endpoint normalization. Computing the
far edge independently from `(high + 1 - origin)` changes rounding and is not
equivalent.

`RunnerSpriteBounds::unrotated` implements this ordinary-sprite branch with
an explicit rounding callback. Its host tests use nearest-even and compare
16,384 cases against a separate floating-point instruction-order transcription,
including cases that distinguish reassociation. This does not establish the
original runtime's FP control word, nor cover rotated or skeletal resources.

`fromSprite` specializes unrotated +1/+1 scale with integer positions of
magnitude below 2^21 and signed 16-bit margins/origins. Every intermediate then
has magnitude below 2^22 and is exactly representable in binary32. Integer
geometry and direct binary32 encoding reproduce the original operations in
this domain. Compatibility rounding still calls the supplied policy in the
original order; failures stop immediately. If a policy changes an integer,
conversion, sorting and far-edge subtraction use the retained binary32 path.
Fractional positions, other scales/rotations and larger resources use
`fromSpriteBinary32`. Differential host cases and the native field verifier
cover both mirror axes, reversed margins, signed zero, policy failures and extreme policy outputs.
This adds neither a bounds cache nor a change in creation/query ordering.
The general binary32 geometry path narrows signed integer resource fields and
rounded pixel outputs directly with `assignInteger`. All int32 inputs are exact
in binary64, so this matches the former integer-to-binary64-to-binary32 path,
including nearest-even ties beyond 2^24 and INT_MIN. Only conversion changes:
arithmetic, callback order, sorting, and the rotated path's integer decrement
before conversion remain unchanged. Host tests include full-width resource
fields against an independent floating-point operation-order transcription.
The resource-backed collision owner rounds published binary32 coordinates
directly through `RunnerImageValue::roundPixel`, retaining nearest-even results
and the existing promoted adapter's rejection of magnitudes at or above 2^31.
It does not alter the supplied rounding-policy interface or establish a new
original-runner FP-mode assumption. The retained promotion-and-binary64 rounding
path remains the differential oracle, including signed zero, exponent-zero
promotion, ties and unchanged outputs on failure.

## Rotated ordinary-sprite branch

`0x4663da..0x4664ab` subtracts sprite origins from inclusive margins, adds one
to the upper edges, and applies scale. It does not rotate the artwork's full
24×24 canvas when computing sprite 84's collision bounds: that sprite has
margin rectangle `(10,11)..(13,13)`, origin `(12,12)` and AxisAlignedRect mode.

`0x4664ab..0x4664f9` computes angle × float-pi / float-180 with a binary32
rounding after each operation. The result is promoted to binary64 for cosine
(`0x74b5f0`) and sine (`0x74b7b0`), then each result is narrowed to binary32.
This resembles the lengthdir wrappers but the subsequent bbox operations do
not apply lengthdir's tiny-fraction snapping.

For scaled local edges L/R/T/B and narrowed cosine C and sine S, the ordinary
branch sorts the separately rounded products and evaluates:

```
left   = (x + min(C*L, C*R)) + min(S*T, S*B)
right  = (x + max(C*L, C*R)) + max(S*T, S*B)
top    = (y + min(C*T, C*B)) - max(S*L, S*R)
bottom = (y + max(C*T, C*B)) - min(S*L, S*R)
```

Every multiply/add/subtract is binary32. Reassociation, double-only corner
rotation or translating already-rounded integer bounds is not equivalent.

The ordinary rotated branch sorts each pair of serialized margins before
subtracting the origin, adding one to its upper edge and applying scale.
Unlike the unrotated branch, compatibility conversion decrements the rounded
upper **integer** before converting it back to binary32. `RunnerSpriteBounds`
keeps these branches separate and accepts already-narrowed cosine/sine values.
The blood-contact adapter shares the geometry and retains half-open integer
output. General geometry tests cover mirrored scales and reversed margins;
the separate blood tests include its angle conversion and precise sampling.

## Compatibility branch and limits of this evidence

Byte `0x9360b0` changes the edge convention. If set, the bbox routine uses x87
`fistp` to convert each edge to integer and subtracts one from right/bottom.
If clear, internal bounds remain fractional and the public right/bottom
getters subtract one instead. Conversion depends on the runtime FP control word.

The `OPTN` chunk dispatch at `0x4985d2..0x4985e4` calls loader `0x47b9f0`
with the chunk payload. It recognizes the negative marker and version 2,
then `0x47bcc0..0x47bcd8` assigns the byte from bit `0x08000000` at payload
offset 8. The pinned data contains marker `0x80000000`, version 2 and flags
`0x28400214`: **compatibility mode is enabled at load**. The verifier checks
both this serialized value and the loader/dispatch machine code. This is a
static load-path conclusion, not a runtime FP-control-word measurement.

For exact integer positions inside the prepared range, zero angle and scales
of exactly +1 or -1, `PreparedUnitSpriteBounds` uses resource-local integer
offsets prepared before gameplay. All intermediate edges remain exactly
representable in binary32. For an axis with prepared inclusive offsets L/R,
negative scale gives `[-R-1, -L-1]`, not `[-R, -L]`: the original scales the
upper endpoint before the compatibility decrement. The same rule applies to
vertical reflection. It requires no additional mirror tables.

The live owner retains the general geometry path for fractional/out-of-range
positions, non-unit scales, rotation, unsupported metadata and the special
missing-mask branch. Tests compare every pinned sprite in all four flip
combinations with `fromSpriteBinary32`, including coordinate extremes; native
prepared-resource verification exercises all four orientations as well. This
does not extend the admitted FP-mode assumptions to other geometry paths.

## Precise collision dispatch

Instance collision routine `0x464d30` adds one to inclusive upper bounds for
its broad overlap tests in this mode. If either sprite is precise, it calls
`0x4b8eb0`; the fractional-mode alternative is `0x4b96b0`. Both sprites' frame,
position, scale, angle and computed bounds are supplied. Frame selection is
truncated toward zero, then normalized to the sprite's frame count.

At `0x4b8fb8..0x4b8ff9`, the compatibility routine intersects the bounds and
converts the extrema with `cvttss2si`. Its loops include both endpoint pixels.
The unit-scale, near-zero-angle fast path rounds positions with x87 `fistp`
and indexes masks from origin plus pixel minus position. Other scales and
angles branch to `0x4b9227`, with rotated sampling at `0x4b93f6`. Goal fields
have scale 2, so their blood interactions cannot be justified by the unit-scale
fast path alone. Generalized integer rectangle/field tests prove their supplied
geometry only, not the rotated sampling of this latter path.

The scaled near-zero-angle path (`0x4b926a..0x4b93f6`) is distinct from the
rotated path even at angle zero. It computes each axis as
`((worldPixel - adjustedPosition) * reciprocalScale) + origin`, then truncates
with `cvttss2si` **before** testing canvas limits. Thus a sample in `(-1,0)`
becomes pixel zero and may hit. The rotated path tests the fractional coordinate
against zero first. `RunnerMaskTransform::prepareAxisAligned` preserves this
distinction; explicit negative-edge fixtures reject substituting the rotated
path with sine zero/cosine one.

At dispatch, all four scales are compared inclusively against binary32 versions
of `1 +/- epsilon`; both promoted angles must lie within double `+/- epsilon`.
The executable initializes epsilon at `0x8a6c28` to `1e-5`. This is an initial
data value, not proof of the value at every runtime call. The unit fast path's
position rounding and pair-wide dispatch are separate from the scaled sampler.

`RunnerMaskDispatch` models this pair-wide choice, with configurable epsilon
and the pinned initial value as its default. `TimeStopGoalContact` uses that
default for its unrotated ordinary/goal-field specialization: near-unit partners
round positions for sampling, while scale-2 goal fields retain scaled sampling.
Bounds are still calculated with the actual scales before choosing sampled
pixels. The specialization retains its existing nearest-even policy; this
does not independently establish the original runtime FP control word.

## Rotated sampling

`0x4b93f6..0x4b947a` computes sine/cosine of each **negated** angle with
the same float pi/division and float-result narrowing as bounds. Reciprocal
scales come from binary32 `1/scale`. Negative scales shift that sprite's
position by minus one before evaluating coordinates.

`BloodMaskSample` evaluates that negated input through the recovered trig
program. It does not replace the signed-input evaluation with a sign flip on
the positive-angle sine result; polynomial symmetry is not assumed.

For each inclusive candidate world pixel, with `dx/dy` relative to that
adjusted position, the unit uses this order (round every operation to binary32):

```
sampleX = ((dy*S) + (dx*C)) * reciprocalScaleX + originX
sampleY = ((dy*C) - (dx*S)) * reciprocalScaleY + originY
```

Both coordinates must be nonnegative and strictly smaller than sprite width
and height, even before the optional precise-mask byte test. For that test,
`cvttss2si` truncates sample coordinates toward zero. Success requires both
sprites to pass. X is the outer scan loop and Y the inner loop. The sprite
flag at offset `0x8b` controls whether the mask byte is consulted; a false flag
does not bypass the canvas-bound checks.

`RunnerMaskTransform` shares this rotated-branch transform with the blood
sampler. It accepts the already-narrowed inverse sine/cosine, prepares reciprocal
scales and negative-scale position adjustments once, and can sample a borrowed
`SpriteMaskPack` view. Integer-only arithmetic is checked against separately
evaluated scalar binary32 expressions for 65,536 mirrored/fractional samples.
It rejects zero scales and unsupported nonfinite/subnormal arithmetic rather
than silently treating failure as a miss. Unit-scale arithmetic skips identity
multiplications/divisions; this does not select the runner's distinct near-zero
angle fast path. That dispatch, trig identity and original FP mode are not
established by the transform tests.

The loader at `0x4b7b45` maps serialized mode 1 to flag `0x89` (precise) and
mode 2 to `0x8a` (rotated-rectangle handling). Flag `0x8b` is different: it
indicates expanded mask bytes. `0x4b7e3f..0x4b7e61` retains serialized mask
count/data; `0x4bbbb0` lazily expands those bits and sets `0x8b` whenever stored
masks exist, regardless of the precise-mode flag. The collision routine calls
this expansion for both sprites. Sprite 84 has one stored mask, so its pixels
are tested when paired with precise sprite 616. The stored mask is exactly
columns 10..13, rows 11..13 inside its 24×24 canvas, not the whole canvas.

`tools/export-blood-mask.csx` exports the source mask with UTMT 0.9.2.0 using
`DOWNWELL_BLOOD_MASK_OUTPUT` and `DOWNWELL_DATA_WIN_SHA256`, following the goal
mask exporter convention. Keep output at `extracted/utmt-0.9.2.0/blood-mask.json`
and any CLI round-trip file under `generated/`, never `data/`.
`python3 tools/verify-blood-mask.py` checks the input identity and every mask bit.
`BloodMaskSample` uses the shared transform with unit scale and specializes this rectangle;
its prepared trig is independent of queried pixels. Active FP mode and signed
trig equivalence still need original-runtime verification.

`BloodCollisionBounds` specializes the earlier bbox arithmetic for sprite 84,
unit scales and nearest-even conversion, returning half-open integer edges.
Its scalar test isolates arithmetic from trig: it does not independently
prove Windows trig identity, FP mode, or the sampling routine described here.

`BloodGoalContact` covers both integral-centred goal fields (sprite 616, scale 2)
and ordinary fields (sprite 597, scale 1). It selects the sampling branch from
both sprites' scales/angles. `BloodMaskSample` then uses rounded integer
positions, unrotated fractional sampling, or the rotated transform as selected.
Bounds still use the actual blood angle, even when sampling dispatch treats it
as near zero. The stored blood rectangle's strictly positive edges make direct
interval tests equivalent to truncating coordinates and reading its verified
bits. These query adapters do not select instance identity or activation state;
the scheduler/registry must supply eligible fields.

## Shared stored-mask pair kernel

`RunnerMaskPair` composes the three sampling branches for two resolved ordinary
resources. The caller supplies the selected stored mask frame, cached bounds,
source frame count, transform and origin for each partner. It must still apply
instance eligibility, resource resolution, lazy bounds refresh and the outer
decision to require precise testing; this kernel is not a replacement for those
gates. Inverse trigonometry and integer position rounding remain explicit inputs.

The kernel intersects float bounds, truncates the intersection endpoints to
signed integers, and scans inclusive X outer/Y inner. It retains the first
shared solid pixel for diagnostics, though the runner returns only a boolean.
Both partners use the pair-wide dispatch selection, including the near-unit
rounded-position path's 32-bit integer coordinate arithmetic. Unsupported
numeric inputs or unavailable masks fail without replacing the result. Zero
scale or zero frames produce a successful miss. Integer overflow in the scan
increment is avoided without changing the admitted scan endpoints.

The resource-backed test covers all 741 masks against ordinary/goal field masks
in all three paths, using a separate host arithmetic transcription and injected
trigonometric values. Explicit fixtures distinguish scan order and truncation
from rounding. This is arithmetic/composition evidence, not proof of the
original runtime FP mode, complete instance collision dispatch, or live wiring.

## Ordinary point queries

`position_meeting` is registered at `0x52cdaa` to wrapper `0x52b780`.
The indexed path retains narrowed binary32 query coordinates for the precise
test but truncates its broad-query coordinates toward zero in compatibility
mode. Its candidate callback (`0x52d890`) filters suppressed/destroyed/inactive
records, checks instance/object-family identity, then calls `0x465520` with
precise testing enabled. These are not sprite-to-sprite queries.

The ordinary precise-mask routine (`0x4ba850`) receives integer positions from
the outer routine's x87 conversions. It normalizes the truncated image index
against mask count, then computes each local delta as
`float(point) - (float(instance) - 0.5f)`. For `abs(angle) < float(0.0001)` it
divides the deltas directly by scale and adds the sprite origins. Equality
takes the rotated branch. That branch computes inverse-angle radians with
binary32 `-angle*pi/180`, narrows double sine/cosine, and evaluates
`(dy*sin + dx*cos)/scaleX + originX` and
`(dy*cos - dx*sin)/scaleY + originY`, rounding after each operation.

The final samples are floored and tested against canvas dimensions. In
particular, -0.5 is outside, not sample zero. This differs from the pair
kernel's truncated scaled sampling, mirrored-position adjustment, reciprocal
multiplication and rotation threshold. `RunnerMaskPoint` keeps this geometry
separate and consumes an explicitly selected mask and rounded positions.
Its inverse-trig callback makes the original math dependency explicit.

`make verify-runner-mask-point` pins the reviewed Windows bytes/constants and
compares 25,080 ordinary samples against an independent host binary32
transcription, including mirrored scales and the exact angle threshold.
Unsupported arithmetic leaves outputs unchanged. This does not establish
the runtime's x87 rounding mode, skeletal/rotated-box resource gates, mask
availability fallback, broad-query traversal or live point-query integration.
Those outer operations must be connected before wall corner events can use it.

`RunnerSpatialInstances::pointCandidate` implements the ordinary per-instance
gate at `0x465520`: refresh dirty bounds first, reject outside bounds or flags
`0x100001`, select explicit mask instead of artwork, reject missing/zero-frame
resources, then optionally sample the precise mask. Near edges are inclusive;
far edges are exclusive after adding binary32 `1` in compatibility mode or
`-0.00001` otherwise. The candidate itself does not filter inactive instances;
that belongs to the outer traversal. Rounding callbacks execute in source
order: query Y, query X, instance Y, instance X. Negative image indices use
positive mask-count modulo. Zero serialized mask count preserves the source's
unexpanded-mask true fallback; missing required offline mask bytes are a port
failure, not permission to invent that fallback. Candidate tests cover 1,680
combinations plus lifetime rejection, frame selection and terminal failure.
The geometric tail is shared in `RunnerSpritePoint`; native actor/terrain
candidate testing reads the published bounds and appearance, performs the same
lazy refresh, then uses this tail. Mixed owner tests compare fractional,
mirrored, rotated, missing-mask, inactive and destroyed candidates in both
compatibility modes. The explicit candidate API alone does not enable selectors.
Native `terrainPositionMeeting` now supplies the outer object-resource path for
the audited 60/93 families: source population/tree-height preparation, ancestor
registration order, lazy unindexed refresh and insertion, then shared
`RunnerPointTraversal`. Preparation publishes indexed membership without
changing the run's cached-resource stage. The indexed path flushes the shared
dirty queue and filters exclusions/family before precise candidate testing;
the list path preserves registration order. Compatibility truncation applies
only to the tree query, not to candidate coordinates. This does not admit
general native selectors or temporary `place_meeting` movement, and does not
schedule wall End Step by itself.

`actorTerrainPlaceMeeting` explicitly tests a published actor against the audited
60/93 terrain families using shared family preparation and `RunnerPlaceQuery`.
Temporary movement and restoration use `RunnerSpatialPosition`, preserving
dirty-queue effects. Indexed queries restore the saved cached bounds; list
queries retain the source's temporary cache until lazy refresh. Regression
checks compare both the boolean result and subsequent nonpublishing/publishing
overlap tests against reference storage. No family-specific collision phase or
new trajectory is enabled by this adapter.

Run-owned terrain callers are not admitted by that actor API: reconstructing
their cache from the immutable position cannot represent the list branch's
retained temporary bounds. Code 986's four cardinal shape-query offsets are
exactly `(0,-16)`, `(16,0)`, `(0,16)`, `(-16,0)`. Together with the origin, these
fit in three spare bits of the existing four-byte packed wall word. The native
store retains this cached origin independently of event state; normal bounds
refresh clears it, and lifetime reuse resets it. Bounds readers, old-index
removal and candidate collection reconstruct the cached geometry at that origin
while retaining the actual instance position. No per-cell rectangle is added.
Offsets outside this audited domain reject rather than silently approximate.

Native `terrainPlaceMeeting` now borrows one non-reentrant transaction record
for an object-60 caller at these offsets (or its origin). It uses the shared
family preparation, `RunnerPlaceQuery`, source position setter and precise
overlap routines. Bounds refresh sees the borrowed position and publishes its
cached origin; restoration preserves queue changes and retains temporary cache
state only on the list path. The indexed path restores the cache origin saved
after flushing, not an earlier stale snapshot. General wall motion and arbitrary
offsets remain unadmitted. This connects explicit wall queries; automatic mixed
End Step scheduling and tile/limboShard effects remain separate work.

`wallEndStepEvent` connects a scheduler-selected native wall to these real
shape/point queries and the shared code-986 handler. Native state remains
borrowed for the event, so nested queries see its current image. Destruction
and activation publish through the typed world; tile, limboShard and out-of-view
callbacks remain the caller's responsibility. The explicit area and RNG are
passed through unchanged, including failure prefixes. This adapter is not an
automatic terrain phase or a production limboShard implementation.

`RunnerSpatialInstances::point` supplies object-family traversal through the
same preparation/index ownership used by `instance_place`. It filters flags
`0x100003`, and indexed candidates additionally require kind 1 and matching
ancestry. The pinned suppression predicate at `0x501e40` is an unconditional
false stub. There is no calling-instance exclusion or temporary movement.
Preparation chooses the source list/index branch from family population and
index height; only the indexed route flushes pending bounds. Compatibility
mode truncates the broad-query coordinates toward zero (including SSE's
integer-indefinite conversion), while precise candidates retain raw binary32
coordinates. Traversal stops at the first hit or terminal callback failure.

The stored-world adapter exposes this as `positionMeeting` for object-resource
selectors. Tests exercise the real indexed and list routes, dirty publication,
first-hit order, inherited families and no self mutation; 81 queries against
authored room terrain compare the resource-backed adapter with direct bounds
and original-mask checks. These checks do not execute wall End Step or establish
the full Windows runtime FP environment. All/instance-ID selectors and wall
scheduler integration remain separate work.

## Native non-precise terrain lines

Native `terrainNonPreciseLine` admits wall (60, including 99) and platform
(93) selectors. It shares `RunnerLineTraversal` with the reference owner:
family preparation selects list/tree traversal, only the tree path flushes
pending bounds, and candidates retain source exclusion and not-me gates.
The candidate refreshes dirty bounds before `RunnerLineBounds` and resource
frame-count checks. The result is the first hit in source traversal order,
not the closest geometric hit. Native tests compare selected generation
handles for fractional/reversed/zero-length lines, sparse and dense terrain,
deactivation and reactivation. This admission does not extend to projectile
parents 59/83, precise line masks, all/instance selectors or incomplete solid
families; it does not establish source-faithful live bullet motion.

`terrainCollisionPoint` admits those same native terrain families, with
precise and non-precise candidate sampling. Its shared `RunnerCollisionPoint`
traversal follows registrations directly: unlike `position_meeting` and
`collision_line`, it neither prepares a family nor flushes the spatial index.
Each visited candidate performs its own lazy refresh after outer exclusions
and not-me filtering. First-hit tests retain the dirty bounds of an unvisited
later wall, guarding against accidentally substituting a tree-based query.

## Cross-target mutation tape

### Native resource residency and lifetime publication

The Amiga executable embeds the same generated `downwell-sprites.dwsc`
metadata as the host and constructs `GameplayCollisionOwner` before its world.
Player, enemy, breakable, terrain, ordinary and goal time-field births/retirements publish through
that owner, including in-place Retry. Its storage is static, not on the task
stack; the current 68000 layout consumes 1,419,024 bytes before the 26,700-byte
sprite metadata pack and the 511,580-byte original collision-mask pack. Mask
bytes are borrowed directly from the executable's read-only data, not converted
into Chip bitmaps or reconstructed from artwork alpha. This is an expanded-memory development configuration,
not evidence of stock-A500 memory fit.

Ordinary object-82 fields resolve directly from `OrdinaryTimeFields`, without
a per-frame population pass or duplicated coordinates. The native view reserves
64 disjoint slots after terrain; generation is the shared field birth identity.
The registry binds the original sprite 597 and default properties before radius
initialization. Runtime Create publishes birth before consuming its radius RNG;
builder adoption preserves the already-consumed sample. Field Draw changes only
radius, not the collision sprite or transform, so it does not rebuild bounds.

Native object-255 exit fields and their object-371 entrance descendants share a 64-slot lifetime store after the
ordinary-field keys. Each synchronous goal child publishes resource defaults,
then scale X=2 and scale Y=2, before `RunSession` consumes radius RNG. Repeated
exit creation retains earlier collision identities rather than rebinding the
latest goal's slot. Room exit and Retry retire these records; Retry preserves
the monotonic birth cursor and binding. Partial birth/scale/retirement failures
are terminal and retain accepted mutations. This does not establish complete
cross-family child order (room-end setters remain unregistered) or enable
identity-bearing field selection. The existing
single-goal presentation model also does not yet draw multiple overlapping
goal fields from a deliberately repeated exit fixture.

Entrance Create uses that same default/both-scales publication before the
shared radius sample. On level one it publishes a destroy mark, then room-entry
reconciliation and the owner's room-ordered cleanup retire the marked record.
Later levels retain the entrance field and its original scaled collision mask.
The initial seed-2 checkpoint stops at draw 330; the live entrance Create
consumes draw 331, instead of importing a post-Create RNG cursor and skipping
the field's ownership. Forecast-only terrain fixtures can still request the
post-entrance checkpoint. Failed marking/reconciliation/retirement is terminal.

Room teardown retires fields in their own creation order before invalidating
keys. A failed callback preserves completed retirements, retains the failed
record, and makes the owner terminal. Retry preserves the binding and monotonic
birth sequence, so a reused field slot cannot resolve an old handle. Tests
cover real generated-command adoption, native registry population/resources,
Retry, and callback failure/RNG ordering.
It does not finish cross-family room creation/teardown order or admit field
selection queries: relevant effect/projectile lifetimes still need registration.

The task requires an 8 KiB stack. Launch scripts set it explicitly; the small
entry wrapper returns 20 before entering the application if less is supplied.
Scene verification preserves a 256-byte stack guard and records the first
failing boundary without resetting it. `verify_stack_stage.gdb` is diagnostic:
its marker means the breakpoint was reached, not that the reported stage passed.

This wiring does not replace the native world's existing collision callbacks:
other families and their original scheduling still need integration. Wall End Step
is not dispatched by its production scheduler. Scene verification exercises
the resident-mask query route against an active room wall and an out-of-room
miss; its gameplay-frame gate requires both checks and the intact stack guard.
These checks establish resource availability, not complete collision coverage.
The registered terrain supplies
the retained painter, with changed-wall notifications routed to sparse row
damage. No broad collision-parity or frame-budget claim follows from this.

Wall activation and destruction marks publish occupancy changes immediately,
with typed bindings validating local bounds, generation, object identity and
the current typed-store pointer on each call. `GameplayCollisionView::resolveLocal`
avoids materializing the generic thirteen-pointer result when the family is
already known; it does not cache pointers across compaction or mutations.
Tests compare typed lookup against the retained generic resolver for every
family, missing stores, invalid keys and reused lifetimes.
Wall occupancy publication remains separate from coalesced presentation damage.
Room insertion uses the original stable upper bound
of the binary32 depth key. Searching inward from both list ends avoids walking
all retained terrain for foreground births, without changing equal-depth order
or allocating another index. Negative depths, fractional keys, signed-zero ties
and extreme finite keys are compared against the full ordered-list oracle.
The native gameplay Draw entry
calls the owner's checked cleanup: marked terrain, enemy and box identities survive all phase
reconciliation, then retire in active-room order followed by inactive-room
order. A pending-mark flag avoids scanning unchanged rooms. Mark publication
also maintains a sparse ordered room-list projection: it searches backward for
the preceding marked peer, without running retirement callbacks. Reconciliation
relinks marked peers in their destination list; removal/rebinding clears the
old mark. Draw entry visits only this projection, revalidating each marked
generation and typed record before retirement. Unmarked typed lifetimes are
maintained by their checked birth/retire bindings, not re-audited by each Draw
sweep. The callback contract permits only current-entry removal, not new marks,
births or arbitrary membership mutation. Compact projection links cost four
bytes per room slot; mark publication still has a worst-case room-list search,
so sparse cleanup alone is not a worst-case frame-budget proof. Enemy cleanup
compacts the typed store without changing surviving lifetime handles; box
cleanup invalidates its lifetime without recycling another slot's identity.
The engine-mark methods run after any game Destroy/CleanUp callbacks and do
not stand in for those callbacks. They are idempotent for a live marked record;
stale handles or missing destruction publication fail the world. Player
destruction remains unsupported rather than silently bypassing typed retirement.
Legacy native death/out-of-view paths still need migration to source handlers;
the typed cleanup wiring alone does not establish their phase or event order.

Spatial insertion uses a bounded descent path and reverse unwind rather than
recursive split frames. Child selection, bounds propagation and sibling order
remain source-ordered. The independent scalar-tree comparison includes a
2,400-instance terrain grid as well as removal/reinsertion tapes.

Child selection retains its binary32 implementation as the fallback/reference.
The pixel-bound fast path accepts only exact integer coordinates with magnitude
below 2^24, non-inverted rectangles, candidate/incoming union extents at most
65535, and each such union area at most 2^24. Every candidate area and union area is then
an integer in [0, 2^24], as is the nonnegative enlargement. All are exactly
representable in binary32, so integer subtraction/multiplication gives the
same comparison values. Selection still minimizes enlargement, then area,
then original slot order. Signed zeros compare equal on both paths. Fractional,
larger or invalid inputs use the retained path; no approximate arithmetic or
different collision traversal order is introduced.
The shared bitwise comparison uses normal unsigned bit ordering for equal-sign
values (reversed for negatives); identical bits already compare false. Only
opposite signs need the explicit signed-zero equality check. This preserves the
previous comparator for all bit patterns; numeric admission remains a separate
gate. Native boundary fixtures and host raw-bit comparisons guard the change.
Coordinate admission shifts the 24-bit mantissa down and reconstructs it to
detect any discarded fractional bits. The admitted shift range is 0–23, so
reconstruction cannot overflow; this is identical to the low-bit mask test
without constructing a mask or loading a table. Tests exercise each mantissa
bit for both signs at every admitted exponent.
Child selection decodes and scores one candidate at a time, without a temporary
array or a union of all candidates. Only candidate/incoming unions enter the
source comparisons, so the all-candidate union need not satisfy these bounds.
If any later candidate fails admission, selection remains unpublished and the
reference handles the complete request.
`RunnerSpatialIndex` begins with integer-backed bounds. Each inserted rectangle
is decoded once, and its leaf and ancestor bounds remain signed integer pixels.
A tagged-at-owner union keeps the node storage size unchanged. Child selection,
unions, closed overlap queries and partitioning use these coordinates directly;
the area/extent checks above still apply. If those checks fail, that operation
reconstructs exact binary32 inputs and uses the retained arithmetic reference.

The first inserted fractional, out-of-range or negative-zero coordinate converts
all retained node bounds back to binary32 before insertion. This conversion is
exact for the admitted integers. Negative zero triggers conversion rather than
discarding its sign. The tree remains binary32 until clear; removal does not
try to infer that it has become integral again. Integer read queries use direct
closed comparisons; fractional queries and line queries reconstruct binary32
bounds without changing the tree's representation. No callbacks run during
conversion, and traversal callbacks must not mutate the index.

The standalone `ChildSelection` helper still borrows an unchanged incoming box
and strided candidate references for arithmetic verification. The index's
`IntegralStorage=false` instantiation forces original binary32 scoring and
storage for cross-target comparison. Verification covers populated-tree
fallback, fractional reads, released-node reuse and clear, alongside the
independent scalar-tree mutation and traversal tests. Representation does not
change partition seeds, ties, child order or instance identities.

Quadratic partitioning retains the stronger complete-union bound: all seven
rectangles together must have extents at most 65535 and area at most 2^24.
Pair-union area is
at least each member's area, so `(union-a)-b` stays within [-whole, whole];
enlargements and their differences have the same bound. All comparison values
are therefore exact binary32 integers. The source's initial `-whole-1` score
can round at the upper boundary, but its unset/equality clause still admits
the first pair because every pair score is at least `-whole`. The integer path
admits that first pair explicitly, then preserves the source's strict score,
group-population and minimum-fill tie rules. Node distribution still follows
original entry-array order, not assignment order. Fractional or oversized
partitions retain the binary32 reference implementation.

Within an admitted integer partition, candidate growth scores are cached only
for the duration of that split. Assigning an entry changes one group's bounds
and area, so only that group's scores are recomputed in the next candidate
walk. Population-based tie decisions are still evaluated live, in original
array order. This scratch does not persist in tree nodes or change seed choice,
minimum fill, final redistribution order or the binary32 fallback.

The fixed-point movement adapter (`FixedCollisionMath`, fifths of Q8) retains
nearest-even rounding at 1280 units per pixel. Its nonnegative 32-bit magnitude
uses `SmallUnsignedDivision` to obtain quotient and remainder together. The
portable reference uses ordinary unsigned division/modulo. The 68000 twin uses
one `DIVU.W` only when the dividend's high word is below the divisor, proving
the quotient fits in 16 bits. Otherwise it divides the high word first and
feeds that remainder into a second division; the same bound holds. The divisor
must be nonzero and at most 65535. This changes neither tie handling nor the
existing fixed-point input/output range. Wider host `long` magnitudes retain
the original arithmetic. Shared native verification checks divider overflow
boundaries, both rounding signs and both tie parities; host tests cover all
1280 remainders at selected small and large quotients.

`RunnerSpatialVerification.h` supplies the `SPATIAL` result in the host/native
replay comparison. It checks transform writes (including signed zero, alpha and
tint dependencies), all 256 low object-flag combinations in both preparation
modes, intermediate binding callback state, dirty queues, retirement and handle
reuse. Assertions contribute an explicit failure count independently of the
state digest; the host reference refuses to produce a passing result on failure.

Build `build/port-replay-reference` for the host. The native tape uses
`REPLAY_VERIFY=1 SKIP_TO_GAME=1` and `amiga/verify_replay.sh`, with
`VERIFY_TIMEOUT_SECONDS` bounding emulator/debugger execution. Restore the
ordinary Amiga build after verification, including after failure. The debugger
also checks the task-stack guard before publishing the comparison marker.

This exercises the portable collision owner's mutation paths on 68000, not a
live encounter, rendered output, timing budget, resource residency or the
Windows runner itself. Original-source contracts and live integration remain
separate requirements.
