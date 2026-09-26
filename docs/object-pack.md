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
| 4 | u16 | Format version, currently 2 |
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
firstBinding, bindingCount; i32 creationDepth`

`depth` preserves the serialized OBJT value. `creationDepth` is the compatibility
`instance_create` depth, decoded from the pinned `__global_object_depths` VM
assembly. Pair its name/depth array slots before resolving names to resource IDs;
array slots are not object IDs. Unlisted resource IDs retain the source's zero
default. Dynamic Create uses this value before callbacks and room-list insertion.
Placed room instances retain their room/layer depth instead. Runtime depth writes
are a separate operation. Version 1 packs are rejected rather than silently
interpreting their reserved zero as a creation depth.

The generated `.manifest.json` sidecar records source and input hashes, extraction
tool identity, reviewed assembly hashes, generator hash and output hash. Both
payload and sidecar are ignored build products.

Flag bits 0–3 are visible, solid, persistent, and uses-physics respectively.
Names remain in the auditable JSON manifest and are not duplicated.

## Event-binding record — 16 bytes

`i32 eventType, eventSubtype, codeId; u32 reserved`

The pinned data has exactly one code action for every serialized event binding;
the generator verifies that invariant rather than silently flattening a more
general action list. The code ID is the traceability seam to both the recovered
GML and exact VM assembly. Inheritance remains explicit through `parentId` and
will be resolved by the native event dispatcher, not baked into this data.

## Portable reader

`src/core/ObjectPack` decodes every field explicitly and validates total size,
object IDs, event slices, parent IDs, and inheritance cycles before exposing
the pack. Event lookup first checks the requested object and then walks its
parent chain, returning both the recovered code ID and the object that owns the
binding. That owner is required to implement `event_inherited()` without
conflating it with ordinary inherited event lookup.
