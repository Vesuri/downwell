# Time-stop collision inputs

The source player object 35 uses sprite 31 as its explicit collision mask,
independent of the visible animation sprite. Shard object 238 uses sprite 91
and has no explicit replacement mask. Both sprites have origin (12,12), a
24x24 canvas and a single stored collision bitmap.

`make verify-time-stop-masks` exports only to ignored generated data, checks
the input hash before and after extraction, and verifies every stored bit:

| Sprite | Solid pixels (half-open) | Local untransformed bounds |
| --- | --- | --- |
| 31, sprPlayerIdle | [8,16) × [8,20) | [-4,4) × [-4,8) |
| 91, sprPoisonPotPieces | [8,16) × [10,13) | [-4,4) × [-2,1) |

There are no holes inside either stored collision rectangle. This permits a
rectangle specialization for their mask samples, but does not justify rounding
the query position first. Against the precise goal-field mask, the runner still
performs its transformed per-pixel intersection after the compatibility-mode
bounds test. Float narrowing, inverse sampling and negative-scale alignment
must agree with [the runner collision rules](runner-collision-bounds.md).

## Ordinary fields

`parentTimeField` (object 82) itself is instantiated by builder symbol `O`
and by `scrTypicalDamage` for the Timeout upgrade. Its sprite 597,
`maskCircle24`, has a 48×48 precise mask and origin (24,24), at unit scale.
It is not the scaled exit field. The stored bitmap contains 1,741 solid pixels;
each row is contiguous, so exact row intervals may replace bitmap storage,
but a geometric circle approximation may not replace those intervals.

Create initializes radius bounds to 24 and 26 and consumes one random word
for `24 + random(2)`. Draw resamples before checking camera visibility unless
`global.pTimeStop` is set. It draws white outline circles at radius and
radius−1, followed by a red outline at radius+1. Drawing never changes the
collision mask. `OrdinaryTimeField` separates radius advancement from snapshot
painting so repaints cannot consume RNG.

The mask exporter records sprite IDs, source hash and extractor version/commit.
`make verify-ordinary-time-field` generates ignored row tables and a manifest;
the verifier pins all bitmap bytes and the audited Create/Draw assembly.
Builder object-82 initialization already consumes the Create random word:
world registration must retain that result, not call Create again and consume
a second draw. Remaining ownership, lookup and rendering work is in the queue.

## Unrotated transformed sampling

The unrotated precise path at Windows runner `0x4b9227..0x4b93f6`
narrows reciprocal scales to binary32. For each negative reciprocal it
subtracts one from the corresponding position before inverse sampling.
Each subtraction, multiply and origin addition rounds separately to binary32;
sample coordinates are truncated before canvas and stored-mask lookup.

Compatibility bounds at `0x46616d..0x466328` compute the lower edge as
`float(float((margin-origin)*scale)+position)` and the upper edge as
`float(float(width*scale)+lower)`. They round both edges, swap if needed,
and subtract one from the upper inclusive edge. Computing the upper edge
independently from the position is not the same arithmetic contract.

`TimeStopGoalContact` specializes these rules for the two rectangular masks
above and an integral-centred, scale-2 goal field. It returns half-open bounds
internally, then tests shared solid pixels in X/Y traversal order. It does not
handle rotated actors or other time-field resources. Unsupported numeric
inputs fail explicitly. The nearest-even bounds assumption still requires
original-runtime FP-mode verification; source pinning and host scalar tests
are not execution of the Windows collision routine.

## Player event boundary

In `objPlayer_n` Begin Step, `scrPlMovement` precedes clock accounting. The
source then applies wrap mode and publishes `x=round(xx)`, `y=round(yy)`.
Camera/interactable queries follow, then the time-stop transition query uses
private `xx/yy`. `scrTimeShard` performs its own field lookup using the newly
published `x/y`. These are different positions at the same event boundary.

The clock's earlier field query and the later time-stop query must not be
merged merely because both use `parentTimeField`. The existing integer goal
query is not by itself evidence for fractional transition contacts. Remaining
integration and original-runner verification belong in [the queue](open-work.md).
