# Activation lifecycle correction

Current checkpoint: the historical extended-combat failures recorded below
are superseded by the passing two-cadence, 6,000-update checks in
[the fidelity audit](original-behavior-audit.md#current-extended-combat-checkpoint).
Exact boundary and transform behavior in the original Windows runner remains
an open fidelity item; centre-only activation has now been removed.

## Bounding-box region inclusion

The shared activation helper now intersects the enemy's published mask bounds
with `(0,0,480,activatey)`. The width is rmMain's serialized width, and the VM
passes `(0,0,room_width,activatey,true)` to `instance_activate_region`.
[The GameMaker documentation](https://manual.gamemaker.io/lts/en/GameMaker_Language/GML_Reference/Asset_Management/Instances/Deactivating_Instances/instance_activate_region.htm)
describes bounding-box overlap, independent of precise mask pixels. The former
centre-Y predicate could miss an overlapping mask and ignored room width.

Both adapters use the existing resource-verified `EnemyContact` bounds and
unit flips. SDL resolves the serialized object sprite when the instance has
no animation override, as its renderer does. Without that fallback, the new
paired route check failed at update 882 because a dormant bat appeared to
have no mask. Two older unit fixtures also used sprite 175 (jellyfish hit)
for object 116; these now use the serialized bat sprite 153.

Tests cover region edges, empty boxes, overlap with an origin below the
frontier, horizontal exclusion, storage order and active/retired exclusions.
The live runtime test activates a snake centred at world Y=179 when its top
at 175 overlaps frontier 176. No extra frontier advances, RNG calls or changes
to actor execution order were introduced. Bounds are only resolved for
pending inactive records, not every active enemy every frame.

Host unit/resource checks and both natural exit/shop routes pass:
`/tmp/downwell-activation-bounds-unit.log` and
`/tmp/downwell-activation-bounds-natural.log`. This implements documented box
overlap using the existing integer-bound model. Exact touching-edge, flipped
pixel and fractional-coordinate behavior still needs original-runner capture;
modern documentation alone does not prove every detail of this GMS1 build.
General within-event visibility of newly activated instances is not a generic
VM scheduler implementation; this retains the existing player/actor phases.

The bounded native natural-route gate passes: exit at update 1,477, 211
upgrade and 135 exit-field comparisons, 49 shop-HUD comparisons, and the
43-update return to the shaft. Pixel failure counters remain zero and the
task-stack guard passes. Normal startup is restored with both 68000 audits
clean. Logs: `/tmp/downwell-activation-bounds-native.log` and
`/tmp/downwell-activation-bounds-native-gdb.log`.

The 6,000-update combat/death adapter regression also passes, including 3,502
paired dead updates and both later room re-entries. Log:
`/tmp/downwell-activation-bounds-combat.log`.

## Historical frontier/phase correction

Source evidence is `gml_Object_objPlayer_n_Step_1.asm`: its first block tests
view bottom > activatey - 112, adds exactly 16 to activatey, then activates the
region before gravity and movement. `scrPlayerInit` initializes activatey=0.
`gml_GlobalScript_scrOutofview.asm` calls instance_destroy, not deactivation.

Amiga now maintains this advancing frontier instead of a constant camera-distance
activation test. SDL performs its frontier update at the start of player Begin
Step rather than after all gameplay updates. Deferred inactive instances carry
an explicit activationPending flag; activation clears it. Destroyed/retired
instances cannot be reactivated merely because their active flag is false.
The pending state is included in the SDL deterministic digest.
Its pinned startup digest is now `5ebea3e340215cdc`; this is an explicit
checksum-layout change, not a new Windows behavior reference.

Tests cover ten 16-pixel frontier advances leaving a world-y 176 centre dormant,
the eleventh activating it before enemy motion, deferred insertion preserving
dormancy, and retired live-tape enemies no longer awaiting activation.
These older tests proved frontier progression and scheduling, not region
inclusion. The bounding-box work above supersedes their centre-only predicate.

The extended combat gate still fails at the tick-600 snake hit and tick-666
player movement. The bat at world y2368 still activates earlier in SDL; camera
fraction/frame alignment and region boundaries remain under investigation.
Do not claim full activation fidelity from matching the frontier algorithm.

Validation: full host suite passes; the bounded A500 canonical replay passes
with `bfbbd762b9b62a59`, and 68000 helper/instruction audits are clean. SDL was
rebuilt. The short target replay is a platform regression check, not coverage
of the disputed bat boundary in the extended host tape.
