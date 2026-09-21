# Native object/event pack (`DWOB`)

The object pack is a deterministic, endian-explicit lowering of the complete
object/event binding graph in manifest schema 3. It complements the `DWRM`
room pack: rooms identify object IDs, while this pack resolves those IDs to
object properties and recovered VM code-entry IDs.

Run:

```sh
make generate-object-pack
make verify-object-pack
```

The generated payload is copyrighted derived game data and remains ignored
under `generated/`. All integers are little-endian; native loaders must decode
fields rather than mapping C/C++ structures over the bytes.

## Header — 40 bytes

| Offset | Type | Meaning |
| ---: | --- | --- |
| 0 | char[4] | `DWOB` magic |
| 4 | u16 | Format version, currently 1 |
| 6 | u16 | Header size, 40 |
| 8 | u32 | Source manifest schema, currently 3 |
| 12 | u32 | Object count, 386 for the pinned build |
| 16 | u32 | Event-binding count, 2,075 for the pinned build |
| 20 | u64 | First eight source SHA-256 bytes |
| 28 | u32 | Reserved, zero |
| 32 | u64 | Reserved, zero |

Object records immediately follow the header, followed by event-binding
records. Objects retain resource-table order and therefore have contiguous IDs
in this build. Event bindings retain their serialized order within each object.

## Object record — 36 bytes

`i32 objectId, spriteId, parentId, textureMaskId, depth; u32 flags,
firstBinding, bindingCount, reserved`

Flag bits 0–3 are visible, solid, persistent, and uses-physics respectively.
Names remain in the auditable JSON manifest and are not duplicated.

## Event-binding record — 16 bytes

`i32 eventType, eventSubtype, codeId; u32 reserved`

The pinned data has exactly one code action for every serialized event binding;
the generator verifies that invariant rather than silently flattening a more
general action list. The code ID is the traceability seam to both the recovered
GML and exact VM assembly. Inheritance remains explicit through `parentId` and
will be resolved by the native event dispatcher, not baked into this data.
