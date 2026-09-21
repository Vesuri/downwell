# Tools

This directory is for reproducible, read-only GameMaker extraction, IR
normalization, asset conversion, replay generation, and state comparison.

Every extractor must:

1. Refuse to write under `data/`.
2. Verify or at least report the input hashes.
3. Record its own version/commit and schema version.
4. Preserve resource IDs and raw VM assembly.
5. Write bulk output only under `extracted/` or `generated/`.

## UndertaleModTool

`bootstrap-utmt.sh` downloads the pinned, self-contained Apple-silicon CLI for
UndertaleModTool 0.9.2.0 into ignored `tools/vendor/`. It verifies the official
release SHA-256 before extracting or executing it:

```sh
tools/bootstrap-utmt.sh
```

## Reproducible extraction

`extract.sh` first verifies the exact `data.win` SHA-256, then exports:

- UndertaleModTool's parsed build information;
- decoded PNGs for every embedded texture page;
- all root decompiled GML entries and strings;
- exact VM assembly for every root code entry;
- a schema-3 JSON manifest containing resource IDs, symbols, authoritative room
  and instance-creation order, object/event bindings, room instances/views/layers,
  layer membership, object/tile instance allocator boundaries, sprite atlas links,
  sound groups, font metrics, constants, code relationships, variables, and
  functions.

It writes only to ignored `extracted/` and `generated/` directories:

```sh
tools/extract.sh
```

`verify-extraction.sh` checks the immutable input hash, byte-identical parser
round trip, manifest counts/ID ranges/references, export counts, and reported
decompiler/disassembler failures. Run it directly or with:

```sh
make verify-extraction
```

## Native room pack

`generate-room-pack.py` lowers schema-3 room data into the deterministic `DWRM`
format documented in `docs/room-pack.md`. The copyrighted payload is generated
locally under ignored `generated/`; only the generator and format are committed.
