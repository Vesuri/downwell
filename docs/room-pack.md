# Native room pack (`DWRM`)

The room pack is a deterministic, endian-explicit lowering of manifest schema 3
for native SDL and Amiga loaders. The generated payload is copyrighted derived
game data and remains ignored under `generated/`; the generator and this format
specification are committed.

Run:

```sh
make generate-room-pack
make verify-room-pack
```

All integers and IEEE-754 single-precision values are little-endian. The Amiga
loader must decode fields rather than mapping native structures over the bytes.
No compiler padding is present.

## Header — 40 bytes

| Offset | Type | Meaning |
| ---: | --- | --- |
| 0 | char[4] | `DWRM` magic |
| 4 | u16 | Format version, currently 1 |
| 6 | u16 | Header size, 40 |
| 8 | u32 | Source manifest schema, currently 3 |
| 12 | u32 | Room count |
| 16 | u32 | Instance count |
| 20 | u32 | Layer count |
| 24 | u32 | Layer-membership count |
| 28 | u64 | First eight source SHA-256 bytes |
| 36 | u32 | Reserved, zero |

Arrays immediately follow in this order: room records, instance records, layer
records, then membership instance IDs. Rooms are stored in authoritative
`GEN8.RoomOrder`, not numeric resource-table order.

## Room record — 40 bytes

`i32 roomId, width, height, speed, creationCodeId; u32 flags,
firstInstance, instanceCount, firstLayer, layerCount`

Room flag bit 0 is persistence.

## Instance record — 48 bytes

`u32 instanceId; i32 objectId, x, y; f32 scaleX, scaleY, rotation,
imageSpeed, imageIndex; i32 creationCodeId, preCreateCodeId; u32 flags`

Instance flag bit 0 is resolved object persistence. Records are in serialized
instance-creation order within each room.

## Layer record — 20 bytes

`u32 layerId; i32 depth; u32 flags, firstMembership, membershipCount`

Layer flag bit 0 is visibility. Membership entries are u32 instance IDs. Version
1 asserts that every room instance belongs to exactly one layer.

Names stay in the auditable JSON manifest and are not duplicated in the runtime
pack. Numeric IDs provide the traceability seam back to GameMaker resources.
