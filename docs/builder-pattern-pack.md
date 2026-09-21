# Native builder-pattern pack (`DWBP`)

The builder-pattern pack is a deterministic, endian-explicit lowering of the
level fragments recovered from GameMaker GML. The generated payload is
copyrighted derived game data and remains ignored under `generated/`; only the
generator, reader, tests, and format specification are committed.

The first format revision contains the five `scrStartPat1` entrance fragments
and all 41 `partMain` fragments from `scrLevelPat1`. More pattern families can
be appended without changing the record layout.

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
| 4 | u16 | Format version, currently 1 |
| 6 | u16 | Header size, 32 |
| 8 | u32 | Source manifest schema, currently 3 |
| 12 | u32 | Pattern count |
| 16 | u32 | Total cell count |
| 20 | u64 | First eight source SHA-256 bytes |
| 28 | u32 | Reserved, zero |

Pattern records immediately follow the header, followed by raw printable ASCII
cells. Newlines and indentation are removed; row boundaries are recovered from
each record's width.

## Pattern record — 16 bytes

`u16 family, sourceIndex, width, rowCount; u32 firstCell, cellCount`

Family 1 is the area-one entrance family populated by `scrStartPat1`; family 2
is the regular area-one family populated by `scrLevelPat1`.
`sourceIndex` preserves the array index consumed by GameMaker's random choice.
`firstCell` is relative to the beginning of the cell payload.

## Portable reader

`src/core/BuilderPatternPack` decodes each field byte-by-byte. Construction
validates the magic, format/schema versions, pinned source-build hash, exact
payload size, pattern dimensions and ranges, source indices, and printable
cell data before exposing any pattern. The complete-pack test pins all 46
fragments, their 4,599 cells, and representative entrance and regular markers.
