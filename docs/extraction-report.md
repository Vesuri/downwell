# Initial GameMaker extraction report

## Result

UndertaleModTool 0.9.2.0 (`03c7048438347799e0717b6e57a9358d000472e0`)
loaded the local build without warnings and identified it as GameMaker 2022.5,
VM bytecode 17, non-YYC.

The extraction produced:

- 2,501 root decompiled GML files;
- 2,501 root VM assembly files;
- zero reported GML decompiler failures;
- zero VM disassembler failures;
- 8,420 strings;
- a versioned JSON resource manifest;
- authoritative `GEN8` room order (distinct from physical room resource order);
- `GEN8` object/tile instance allocator boundaries;
- 2,075 object-event bindings;
- 5,186 placed room instances across 25 rooms;
- 132 room layers.
- two losslessly decoded embedded texture-page PNGs.

Manifest schema 3 also records sprite dimensions/origins/frame-to-texture-page
links, texture atlas rectangles and pages, sound/audio-group relationships,
font metrics, script-to-code bindings, constants, room dimensions/views,
instance transforms, authoritative instance-creation order, and instance-layer
membership. Numeric IDs are preserved throughout.

The pinned build records `GEN8.LastObj = 105186` and
`GEN8.LastTile = 10000000`. Serialized room-instance IDs occupy the contiguous
range 100000–105185, so 105186 is the source-backed starting boundary for
runtime-created object IDs. This value must be carried into native runtime data
rather than recomputed from whichever subset of rooms happens to be loaded.

There are 2,810 total `CODE` records. The remaining 309 records are child entries
whose instruction streams belong to root entries; they remain represented in
the manifest through their `parentId` relationship.

Fifty-three exported GML files are empty. They have no failure marker and map to
empty object events/code entries, so they are retained rather than treated as
decompiler errors.

## Lossless parser check

Without making a logical change, UndertaleModTool serialized its parsed model to
`generated/utmt-roundtrip.win`. The result is byte-for-byte identical to the
input:

```text
c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
```

This does not prove that every high-level decompilation is correct. It does show
that the parser understood and preserved the complete serialized container,
including all pointer relationships and otherwise unknown fields.

## Reproduction

```sh
make extract
make verify-extraction
```

All payload output is ignored by Git. The scripts, schema-producing source, and
this evidence record are committed material.
