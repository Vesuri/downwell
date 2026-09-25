# Area-two data recovery

Run `make verify-area-two-patterns` to export and verify the catacomb tables.
The ignored output is `generated/downwell-area-two-patterns.json`. This is
an auditable intermediate representation, not a native asset pack or a
playable area-two implementation. The existing area-one pack is unchanged.

The exporter reads UTMT 0.9.2.0 VM assembly from the pinned Windows build
(`c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8`, bytecode 17).
It accepts only literal array assignments with a zero initial index, exact
unit increments and the source's final `[100]` maximum-index store. Computed
indices, extra array writes, unexpected string operations and unknown escapes
are rejected. It does not evaluate recovered GML.

Each family retains its original script/code IDs. Each record retains its
source index, original string resource ID, decoded
raw text (including CR/LF/tabs), normalized row view and every row's width.
The manifest records the original input hash, extraction tool version/commit,
manifest hash, each input assembly hash and exporter version/hash. Generated
payloads stay ignored; neither source game files nor bulk recovered patterns
are committed.

| Source array | Patterns | Row width |
| --- | ---: | ---: |
| `partStart` | 4 | 9 |
| `partMain` | 29 | 9 |
| `partEnd` | 5 | 9 |
| `partLeft` | 2 | 19 |
| `partRight` | 7 | 19 |
| `partShopR` | 3 | 19 |

Total: **50 patterns, 722 rows, 8,078 cells**. The two left-room tables are
preserved even though `scrLevelPat2` and common builder setup set
`leftRoomAmt=0`; exported data presence is not a claim that it is selected.

Tests include synthetic malformed instructions, exact raw-text/string-ID
preservation, irregular row widths and CRLF assembly files. All real VM
literals are also compared against the independently decoded GML view;
disagreement fails verification instead of changing the VM export. Repeated
generation followed by `--check` verifies deterministic output.
The final gate and a second independent `--check` pass; evidence is in
`/tmp/downwell-area-two-patterns-final.log`. No emulator run is needed for
this exporter-only change, and no playable build options were changed.

## Next implementation boundary

`scrBuilderCreation` selects `scrLevelPat2` for area 2. VM setup gives
`extraColumn=3`, `levelLength=13+global.level`, `genCount=-10`, and
`rightRoomAmt=choose(2,2)` (overridden to 3 on level 3). That apparently
redundant choice still consumes RNG. The per-character builder dispatches
to `builderCatacomb`, not `builderCavern`; its symbol/RNG/object rules must
be implemented before reusing the common pattern traversal.

`src/core/CatacombBuilderSymbols.h` now implements the area-specific symbol
dispatch for levels 1--3, transcribed from VM blocks 17--61. It keeps every
`choose` argument list, the overwritten `%` `irandom(level)` call, the
hard-mode override order and synchronous right-then-left paired skull Create
calls. Common terrain/reward symbols remain the common builder's responsibility.
An emission failure stops immediately; unsupported levels fail before any
operation. Coordinates are supplied after the common traversal computes them.

`make verify-catacomb-symbols` checks 306 operation-trace cases: every choice
index (including duplicate values), all three levels, both hard-mode states,
both random-threshold outcomes, object IDs/coordinates and emission failures.
The same cases pass AddressSanitizer/UndefinedBehaviorSanitizer. Pattern export
verification still passes with the unchanged output hash above.

This is shared semantic code, **not yet connected to either live generator**.
Its operations interface intentionally does not bind `choose` to WELL words:
in particular, the singleton `choose(126)` is present in the VM but its runner
word consumption has not been established here. The operation-trace test proves
dispatch/call ordering, not historical Windows RNG equivalence. Integration
also needs concrete synchronous actor creation and any observed builder-instance
state such as `spawnChance`; this helper currently owns only emission decisions.
No emulator run or frame-rate improvement is claimed for this isolated change.

## Actor integration findings

The next actor is not just another entry in `EnemyCreateProfile`: that table's
nonzero health also admits objects to `GameplayWorld::createGeneratedInstance`.
Adding stats before motion, collisions, presentation and alarms exist would
silently register an incomplete actor. Keep that supported-actor gate intact.

`enmJumpySkull` Create VM explicitly assigns `imgSp=0.175`. Existing supported
actors use twentieth-frame rates, which cannot represent this value.
`advanceEnemyImageFortiethsQ8` now supports 7/40 without floating-point target
work. Its residual unit remains fifths of one Q8 phase unit (256/40=32/5).
The existing twentieth-frame API delegates with twice the rate, preserving
all current stored rates and residuals. Forty updates at 7/40 produce seven
frames exactly. This represents the recovered rational rate, not a claim of
bit-identical historical floating-point accumulation at every boundary.

The presentation tests check 50,000 old/new-equivalent updates and 50,000
fortieth-rate updates, including zero speed, nonzero initial residuals and
rate changes. They pass normally and under address/undefined-behavior
sanitizers. Both natural host routes, upgrades, re-entry and the seed-4 shop
return pass (`/tmp/downwell-fortieth-animation-natural.log`). These are
existing-actor regression checks, not a playable skull verification.
The ordinary Amiga build and its 68000 instruction/64-bit-helper audits also
pass (`/tmp/downwell-fortieth-animation-amiga.log`). No emulator was launched
for this arithmetic-only change, and no new timing measurement is claimed.

Further source inspection identifies important prerequisites: skull Step uses
thin-wall versus solid collision handling, hit stun, viewport-gated movement,
landing re-arming and time-stop alarm compensation; its collision events can
also trigger jumps with a different random range. Skeleton throwing requires
synchronous bone Create before its velocity overrides; the bone Create itself
draws random values. Skeleton also uses alarms 2 and 3, beyond the two direct
alarm fields of the current shared enemy state. Spider Create makes ordered
line-collision queries to decide and shorten its hanging web. These cannot be
replaced with cavern behavior just to make the new patterns run.

Remaining work includes wiring catacomb symbol emission into traversal, new actor
and hazard behavior, matching offline art and both rendering backends, and
verified area 1-3 → 2-1 transition ownership. Do not remove the current
later-area guard or substitute cavern logic merely because these tables are
now available. No new Amiga performance or Windows visual-fidelity claim is
made by this data-export gate.
