# Native room pack (`DWRM`)

The room pack is a deterministic, endian-explicit lowering of manifest schema 3
and pinned room-layer metadata for native SDL and Amiga loaders. The generated payload is copyrighted derived
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
| 4 | u16 | Format version, 4 for the complete reference catalogue |
| 6 | u16 | Header size, 40 |
| 8 | u32 | Source manifest schema, currently 3 |
| 12 | u32 | Room count |
| 16 | u32 | Instance count |
| 20 | u32 | Layer count |
| 24 | u32 | Layer-membership count |
| 28 | u64 | First eight source SHA-256 bytes |
| 36 | u32 | First runtime-created object ID (`GEN8.LastObj + 1`) |

Arrays immediately follow in this order: room records, instance records, layer
records, then membership instance IDs. Rooms are stored in authoritative
`GEN8.RoomOrder`, not numeric resource-table order.

## Admitted runtime projection

The Amiga executable embeds `downwell-runtime-rooms.dwrm`, generated alongside
the complete reference pack. Its explicit room set is 7 (initialization),
8 (style selection), 11 (gameplay), 16 (results), and 20 (garden). These are
the destinations of `StartupRuntime` and the serialized-room consumers of
`GameplayWorld` and `AmigaMenu`. The recovered initialization selects room 20
unconditionally; alternative gardens, Steam/demo/debug/trailer rooms are not
reachable through those callers. Missing rooms fail through `findRoom`; there
is no fallback to the full catalogue. Admitting another room requires changing
`RUNTIME_ROOMS` in the generator and reviewing the fixed 28 KiB payload budget.

This projection uses version 5, a 44-byte header, and an additional u32 at offset
40: the maximum loaded layer ID from the **complete original catalogue**.
The loader validates every retained layer against that maximum; version 4
derives the same value while validating the full catalogue. Both seed the
process layer allocator identically, including after re-entry. Room payload
indices are rebased, but IDs, serialized order, views, instance bytes and layer
membership are unchanged. The runtime instance-ID seed is also unchanged.

The sidecar JSON records the full input pack hash, source hash, pinned extractor
version/commit, generator hash, admitted IDs and budget. The regression test
checks retained payload equality and that growing an unrelated room by 10,000
instances leaves the projection byte-identical. Global identity metadata is
deliberately still global: changing its maximum must change the seed.

The current projection is a permanently resident union of supported rooms,
not a per-room loading lease. It excludes unreachable content without claiming
that garden payloads have already been unloaded during gameplay. SDL and
source-wide fixtures still use the complete reference pack.

## Room record — 488 bytes

`i32 roomId, width, height, speed, creationCodeId; u32 flags,
firstInstance, instanceCount, firstLayer, layerCount`

Room flag bit 0 is persistence.
The 40-byte prefix is followed by eight 56-byte serialized view records (14
signed integers each, in `VIEW_FIELDS` order in the generator).

## Instance record — 48 bytes

`u32 instanceId; i32 objectId, x, y; f32 scaleX, scaleY, rotation,
imageSpeed, imageIndex; i32 creationCodeId, preCreateCodeId; u32 flags`

Instance flag bit 0 is resolved object persistence. Records are in serialized
instance-creation order within each room.

## Layer record — 48 bytes

`u32 layerId; i32 depth; u32 flags, firstMembership, membershipCount, type;
i32 backgroundSpriteId; u32 backgroundColor, backgroundFlags;
f32 backgroundFirstFrame, backgroundAnimationSpeed; u32 backgroundAnimationSpeedType`

Layer flag bit 0 is visibility. Type 1 is background; type 2 is instances.
Membership entries are u32 instance IDs; every room instance belongs to exactly
one instance layer. Background flag bits 0–4 are visible, foreground,
horizontal tiling, vertical tiling and stretch. Color retains its serialized
32-bit value, including alpha; sprite ID -1 means no sprite. Animation speed
type retains the original numeric value. Instance layers have sprite ID -1
and zero background fields.

`make generate-room-pack` exports scalar layer metadata when needed and checks
it against the raw ROOM records before packing. The metadata records the full
source hash and pinned extractor version/commit. All layer offsets and speeds
are zero in this build, with no effect name or properties; verification rejects
changes to those omitted fields. DWRM v3 is not backward compatible with v2.
The shared reader exposes background fields, but construction and drawing of
background runtime elements remain separate from instance reservations.

Names stay in the auditable JSON manifest and are not duplicated in the runtime
pack. Numeric IDs provide the traceability seam back to GameMaker resources.

## Portable reader

`src/core/RoomPack` reads fields byte-by-byte and converts explicit
little-endian integers and IEEE-754 floats. It never casts the payload to a C++
record, so alignment, compiler padding, and the Amiga's big-endian byte order do
not affect decoding.

Construction validates magic, format/schema versions, source-hash prefix,
overflow-safe total size, every room/layer slice, and that every serialized
instance ID is below the runtime allocator boundary before any view is exposed.
Unknown layer types and backgrounds with instance memberships are rejected.
Tests cover malformed synthetic packs and also open the complete locally
generated payload, confirming the audited totals and the `rmInit`/`rmMenu`
creation-order anchors.

`PackedRoomLoader` is the bounded bridge from these decoded views to
`RoomLoader`. It resolves each instance's unique layer membership and transfers
identity, position, scale, rotation, image state, creation-code references,
persistence, layer ID, and depth into native instance storage. A missing or
duplicate layer membership rejects the complete room before runtime state is
changed.

The SDL startup runtime opens the generated pack once and uses this bridge for
both `rmInit` and the transition to `rmMenu`. The integration test confirms 192
recovered menu instances plus the persistent controller, including the first
player instance/object anchor. Room creation then adds `ditherFade` from the
pack's runtime allocator boundary. Most recovered menu event code remains
untranslated.
