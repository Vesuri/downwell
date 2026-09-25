# Skeleton effect ownership before live admission

Source: untouched Windows `data.win`, SHA-256
`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`,
UTMT 0.9.2.0 export. Resource metadata identifies objects; the corresponding
`Assembly/gml_*.asm` scripts establish argument evaluation and callback order.
This is an integration audit, not a claim that these effects are implemented.

## Death is not just removal plus a gem reward

`SkeletonActor::step` already exposes the source call sequence. Its live owner
must complete each effect's Create synchronously before running the next call.
Batching all effect creation after currency generation changes the shared RNG
stream, even when the final list of visible particles looks identical.

| Source call | Owned objects / side effects required |
| --- | --- |
| `scrEnemyDeath` | Combo/kill counters, gunshot stop, achievement state, moment delay and level-begin cue |
| `scrBloodfx(10,3)` | **Two** object-220 moving effects, sprite 84; the function ignores the supplied arguments |
| `scrSmokefx(xx,yy,5,1)` | Object 225; variable count and per-child random position/velocity/animation |
| `scrFlashballfx(xx,yy,6,2,10)` | Object 224; variable count, random birth position and additional Create jitter |
| `scrCurrencySpawn(money)` | Existing currency owner, after all preceding effect Creates |
| Death sound 132 | Source position in call sequence; playback need not own RNG |
| `scrDeadBody(206,takenImpact,102)` | Object 263 Create, then sprite/velocity/facing overrides; also modifies the dying skeleton's xsp |
| Two debris Creates | Object 370 twice at published `(x,y-5)`; full Create includes terrain-dependent relocation |
| Destruction | Request retirement; finish the current event's impact reset and hit presentation first |

## Creation contracts that cannot be replaced by an arbitrary RNG skip

- Blood emits at published x/y, not the precise xx/yy accumulators. Its two
  angles are `random(20)` and `180-random(20)`, respectively. Object 220's Create
  initializes motion state without RNG; `emitMovingFx` then assigns sprite,
  animation speed, angle and the direction-derived velocity.
- Smoke count calls `irandom_range(-vary,vary)`. For each child, the VM draws
  the **Y choice before the X choice**, then immediately calls object 225's
  Create. That Create samples speed, animation multiplier and direction before
  the next child's coordinates are chosen. It stores xx/yy and starts at
  animation index 1. Coordinate choices use `choose`, not `irandom`.
- Flash-ball count also uses `irandom_range`. Its per-child Y range draw
  precedes X. Object 224's Create then adds independent integer jitter to X
  and Y and sets image speed to 0.3. Those extra draws must not be omitted.
- Corpse Create inherits object 90's health/hit defaults, then samples X/Y
  velocity and chooses signed animation speed. Keep those samples even though
  `scrDeadBody` subsequently overwrites velocity. The wrapper caps impact at
  four, sets the new corpse's sprite/facing/explosion effect, and replaces the
  **source skeleton's** xsp with facing times 1.5 when its magnitude is below
  0.5. Zero impact still gives the corpse a horizontal speed of magnitude 0.5.
- Debris Create first queries walls/tiles. A blocked birth snaps to the grid
  and runs a bounded random relocation search before sampling velocity,
  animation speed and initial frame. Consequently RNG consumption depends on
  actual terrain, not just the number of debris requests. Area selects the
  debris sprite. Its two instance-count branches currently assign identical
  alarms; that does not remove the terrain-dependent work earlier in Create.

`SkeletonEffectBursts.h` now implements the skeleton's smoke/flash count and
birth-coordinate arithmetic, calling the child creator synchronously per birth.
The scope is exactly the skeleton's (5,1) smoke and (6,2,10) flash parameters;
it is not a general particle engine. Across 1,024 seeds, two burst types and
three capacity limits, 6,144 sequences compare 15,454 child births and the
complete RNG state against host arithmetic. Child Create is a draw-order probe
in this test, not an implemented smoke/flash actor. Sanitizers and 68000
compilation pass without floating-point helpers. The regression is part of
`make test` and available as `make verify-skeleton-effect-bursts`.

`verify-runner-effect-ranges.py` pins the original `irandom_range` wrapper at
0x53c2e0 and its 63-bit/two-word modulo helper at 0x5410c0, plus `random_range`
at 0x53d840. Integer bounds use inclusive width plus the lower bound. Real
ranges draw random(1), multiply by width and add the lower bound; the flash
bounds +/-10 keep that arithmetic exactly representable before position
addition. This is not a general equivalence claim about x87 expressions.

Flash-ball child state is now owned by `FlashBallEffects`: builtin-coordinate
binary32 narrowing, X then Y integer jitter, frame-one start, speed 0.3,
three verified unit sequence keys and Animation End retirement. The fixed pool
keeps stable handles and phase cutoffs. 1,024 seeded full bursts (6,178 births)
match scalar positions/RNG/animation, retire after seven animation updates and
reuse slots without reusing handles. Normal/sanitizer and 68000 compile checks
pass. Exported sprite-96 sequence metadata is now explicitly verified.

This pool is not yet called by the live world or either renderer. Offline-art
preparation uncovered noncanonical colors: every opaque pixel in sprite-96
frames 0/2 is RGB 254 and in frame 1 is RGB 7 (436 opaque pixels per frame).
The exact four-pen exporter initially rejected those colors. Follow-up export
of shader 0 (`shaderTemplate`, selected by the default DOWNWELL palette) now
resolves their color classes. The embedded GLSL thresholds red/blue at 0.5,
classifies light using green >= 0.8, and dark using the remaining low-channel
sum. Thus opaque gray 254 maps to light, and gray 7 maps to dark. This is not
a general grayscale threshold: intermediate gray can map to the special blue
class. The generator only accepts the two reviewed opaque colors for sprite 96;
its general four-pen guard remains unchanged. Transparent pixels stay
transparent; dark flash pixels use opaque black pen 3, not transparent pen 0.

`export-default-shader.sh` records tool version/commit and source hash in ignored
generated JSON; `verify-default-shader.py` pins the reviewed GLSL and the VM's
default palette selection. This is embedded shader evidence, not execution of
the Windows D3D shader. The default shader's final light/dark RGB values are
250/256 and 5/256; our existing white/black Amiga pens preserve the classes,
not exact final Windows RGB. Full alternate palettes and SDL postprocessing
remain separate work.

All three flash frames now have offline planar, mirrored and DMA records.
The independent artwork test evaluates the source shader classes and checks
90 skeleton/bone/flash facings, including rejection of unreviewed colors and
semi-transparent inputs. Pack reproducibility and existing garden/exit-mask
checks pass. The game pack grows from 320,958 to 323,118 bytes (+2,160), SHA-256
`d029f6b121ad7761f496887dba2491d0ac821f26055f4e4af63523c853355de9`.
No runtime asset conversion is added. Live effect collection/drawing remains
unconnected, so this is asset readiness rather than visible gameplay completion.

Smoke child state, flash rendering, corpse physics and debris relocation still
need their own adapters; this audit does not establish those adapters.
No request may silently succeed when required child storage is exhausted.

## Integration boundary

The current cavern contact/bullet helpers retire enemies and emit rewards
immediately. Do not route skeleton lethal damage through those shortcuts.
Skeleton bullet and stomp handlers instead reduce health and leave Step to run
the sequence above. Existing shared player movement can perform the bounce,
but a complete effect owner must also provide recharge effects, damage effects
and Exploding Knees child creation when applicable.

Next owner work should join shared actor/projectile/effect storage and phase
dispatch, then expose the resulting retained state to both renderers. Fixed
capacity, creation order, synchronous Create RNG and event-boundary retirement
are part of that contract. Offline artwork alone does not supply this ownership.
