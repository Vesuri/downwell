# Native builder-pattern pack (`DWBP`)

The builder-pattern pack is a deterministic, endian-explicit lowering of the
level fragments recovered from GameMaker GML. The generated payload is
copyrighted derived game data and remains ignored under `generated/`; only the
generator, reader, tests, and format specification are committed.

Format revision 3 contains the entrance, regular, right-room, and shop
fragments recovered for area one. It preserves row lengths individually;
notably, one recovered shop row is 21 cells wide while its neighbours are 19.

Run:

```sh
make generate-builder-pattern-pack
make verify-builder-pattern-pack
```

All integers are little-endian. The Amiga reader must decode fields rather than
mapping native structures over the bytes. No compiler padding is present.

## Header — 32 bytes

| Offset | Type | Meaning |
| ---: | --- | --- |
| 0 | char[4] | `DWBP` magic |
| 4 | u16 | Format version, currently 3 |
| 6 | u16 | Header size, 32 |
| 8 | u32 | Source manifest schema, currently 3 |
| 12 | u32 | Pattern count |
| 16 | u32 | Total cell count |
| 20 | u64 | First eight source SHA-256 bytes |
| 28 | u32 | Total row count |

Pattern records immediately follow the header, then row records, then raw
printable ASCII cells. Newlines and indentation are removed.

## Pattern record — 20 bytes

`u16 family, sourceIndex, width, rowCount; u32 firstRow, firstCell, cellCount`

Family 1 is the area-one entrance family populated by `scrStartPat1`; family 2
is the regular area-one family populated by `scrLevelPat1`; family 3 is the
right-room family and family 4 is the shop family.
`sourceIndex` preserves the array index consumed by GameMaker's random choice.
`firstCell` is relative to the beginning of the cell payload.

## Row record — 8 bytes

`u32 firstCell, cellCount`

Each row points into the shared cell payload. This makes irregular source rows
lossless while retaining a compact, endian-explicit representation.

## Portable reader

`src/core/BuilderPatternPack` decodes each field byte-by-byte. Construction
validates the magic, format/schema versions, pinned source-build hash, exact
payload size, pattern dimensions and ranges, contiguous row spans, source
indices, and printable cell data before exposing any pattern. The complete-pack
test pins all 62 fragments, their 737 rows and 8,435 cells, including the
irregular shop row and four area-one endings.

## Preparation boundary

The pack is immutable; the generated level is not. `AreaOneBuilder::step`
selects fragments when the camera reaches the generation threshold, using the
same random stream as live gameplay. `emitPattern` invokes each creation
callback synchronously. A prepared representation must preserve that schedule:

| Input/work | Safe preparation | Work that must remain at its original execution point |
| --- | --- | --- |
| Pattern and row records | Decode ranges, widths and row locations; retain numeric source indices | Select the pattern using the current RNG and generation history |
| Cell geometry | Precompute normal/mirrored relative positions and fixed object candidates | Translate by the current builder Y; preserve cell and row order |
| Enemy markers, including empty cells | Classify marker meaning | Consume the per-cell random word even for non-enemy cells; preserve conditional extra draws |
| Object resource defaults | Resolve immutable sprite/mask/depth values, inherited alarms and unit bounds | Apply current family flags, instance identity, membership and synchronous Create side effects |
| Reward and shop markers | Decode marker location/type | Consult current health/style and perform the original random choices |
| Spatial and layer membership | Reserve storage and prepare immutable resource metadata | Publish in creation order against the current population; preserve query and draw ordering |

In particular, the random calls inside a Create callback occur before the
next cell's random call. Bulk-enqueuing a fragment and executing its callbacks
later is not equivalent. Callback failure also stops further cells, random
draws and row advancement; preparation must not advance those effects early.
Side-room wall coordinates, irregular row widths, the between-row random word
and the optional three-row pillar tail are part of the schedule, not padding
that can be silently omitted.

Testing a prepared executor requires comparison with immediate execution while
interleaving external RNG draws, health changes and callbacks that consume RNG,
including early callback failures. Comparing only a level generated from one
initial seed cannot establish equivalence. The decoded views' memory and
preparation time must also be measured; eliminating pack reads does not remove
the live spatial-index construction cost.

A boolean collision result alone does not justify omitting spatial state.
`GameplayCollisionOwner::registeredPlaceMeeting` discards the selected identity,
but its `RunnerSpatialInstances::place` path still performs family preparation,
conditional dirty-bound publication, temporary caller movement and lazy precise
candidate refresh. Those mutations can affect subsequent identity-bearing
queries. An alternative boolean search must preserve the applicable state
transitions as well as the hit result; tests must compare later selections and
pending state, not just boolean answers. This is separate from the `point`
path, whose contract deliberately omits a global dirty flush.
