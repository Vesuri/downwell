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

Remaining work includes source-backed catacomb symbol emission, new actor
and hazard behavior, matching offline art and both rendering backends, and
verified area 1-3 → 2-1 transition ownership. Do not remove the current
later-area guard or substitute cavern logic merely because these tables are
now available. No new Amiga performance or Windows visual-fidelity claim is
made by this data-export gate.
