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
test pins all 58 fragments, their 691 rows and 8,021 cells, including the
irregular shop row.
