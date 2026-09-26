# Activation source contract

Remaining integration and verification belong in [open work](open-work.md).
Implementation history and past test results belong in the git log.

## Main-room frontier

`gml_Object_objPlayer_n_Step_1.asm` tests view bottom > activatey - 112,
adds exactly 16 once per Begin Step, then calls
`instance_activate_region(0,0,room_width,activatey,true)` before gravity and
movement. `scrPlayerInit` initializes activatey to zero; rmMain width is 480.
`scrOutofview` destroys instances rather than deactivating them.

`MainRoomActivation` supplies this frontier operation to both adapters.
`activationPending` distinguishes dormant records from destroyed records;
an inactive flag alone is not permission to reactivate an instance.

## Windows region predicate

In the pinned Steam executable, wrapper `0x529660` narrows region coordinates
to binary32 and constructs the upper edges by adding width/height. It calls
the inactive-instance traversal with predicate `0x52d040`.

For an instance with a valid sprite or mask, the predicate recalculates dirty
bounds through `0x466010(instance,1)`. It marks the instance outside only when:

```
region.left > bounds.right || bounds.left > region.right ||
region.top > bounds.bottom || bounds.top > region.bottom
```

These are strict comparisons: equality intersects. If neither resource is
valid, the predicate applies the same inclusive region comparisons to the
instance origin. The main-room enemy adapter supports known nonempty masks,
not this general missing-resource path.

This build loads compatibility-mode inclusive mask bounds; see
[collision bounds](runner-collision-bounds.md). The portable `ContactBounds`
uses exclusive upper edges. For supported integer axis-aligned masks, the
source region `(0,0)..(480,activatey)` therefore becomes:

```
left <= 480 && right > 0 && top <= activatey && bottom > 0
```

This conversion does not establish fractional/rotated mask rounding or the
runtime FP control word. Sprite fallback and explicit-mask selection must
remain consistent with source resource metadata.

## Immediate activation transitions versus membership processing

`0x467640` requests deactivation. When instance flag `0x80000` is clear it
enqueues the instance in the pending list at `0xb5a070`, then sets flag `2`.
Changed flags invalidate both cached activation traversals (`0x8a65e4/5`).

`0x4627d0` requests activation. When flag `0x80000` is set it enqueues the
instance in that pending list. If flag `2` is set, it calls dirty-queue marker
`0x501e50` before clearing `2`. Changed flags invalidate the same caches.
The enqueue routine `0x4633e0` appends only if the instance is absent; a repeated
request does not move an existing entry. This differs from spatial dirty
marking, which removes and prepends an existing entry.

The region predicate activates only when `(flags & 0x100003) == 2` and the
inside/outside request matches its region result. These immediate routines
do not themselves call ancestor-list unlink/rebind. Their deferred membership
processing must be traced before inferring reactivation order for
`instance_place`; flag changes alone do not prove immediate tail insertion.

## Evidence and checks

`tools/verify-runner-field-lookup.py` pins the reviewed wrapper, predicate,
activation/deactivation and unique-enqueue bytes against the immutable Windows
executable. `tests/core/MainRoomActivationTest.cpp` covers the shared supported
mask boundary conversion, frontier cadence and active/retired exclusions.
These are static-source and component checks, not Windows encounter captures
or proof of complete event/membership scheduling.
