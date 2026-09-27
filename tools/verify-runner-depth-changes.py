#!/usr/bin/env python3
"""Pin depth writes and the selected GMS2 layer service, not live parity."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

root = Path(__file__).resolve().parents[1]
audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
# Layer ID globals are in the PE's zero-filled virtual data tail, not raw
# on-disk initializer bytes. Pin that initial state without reading past raw data.
binary = audit["binary"]
for address in (0x925f84, 0x925f88):
    zero_filled = False
    for index in range(struct.unpack_from("<H", binary, audit["pe"] + 6)[0]):
        virtual_size, rva, raw_size = struct.unpack_from("<III", binary, audit["section_start"] + index * 40 + 8)
        base = audit["image_base"] + rva
        if base + raw_size <= address and address + 4 <= base + virtual_size:
            zero_filled = True
    assert zero_filled, hex(address)
data = (root / "data/data.win").read_bytes()
assert sha256(data).hexdigest() == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert data[8:12] == b"GEN8"
assert struct.unpack_from("<I", data, 16 + 0x2c)[0] == 2
assert struct.unpack("<I", region(0x8a5e54, 4))[0] == 511
# All serialized room resources are loaded before room entry. Their version-3
# sequence arrays are empty, so the extra sequence-layer allocator is skipped.
offset = 8
room_offsets = None
while offset < len(data):
    size = struct.unpack_from("<I", data, offset + 4)[0]
    if data[offset:offset + 4] == b"ROOM":
        count = struct.unpack_from("<I", data, offset + 8)[0]
        room_offsets = struct.unpack_from(f"<{count}I", data, offset + 12)
        break
    offset += 8 + size
assert room_offsets is not None and len(room_offsets) == 25
loaded_ids = []
for room in room_offsets:
    assert struct.unpack_from("<h", data, room + 0x26)[0] == 3
    sequences = struct.unpack_from("<I", data, room + 0x5c)[0]
    assert struct.unpack_from("<I", data, sequences)[0] == 0
    layers = struct.unpack_from("<I", data, room + 0x58)[0]
    for index in range(struct.unpack_from("<I", data, layers)[0]):
        layer = struct.unpack_from("<I", data, layers + 4 + index * 4)[0]
        loaded_ids.append(struct.unpack_from("<i", data, layer + 4)[0])
assert len(loaded_ids) == 132 and max(loaded_ids) == 131
assert region(0x7fe384, 6) == b"depth\0"
assert struct.unpack("<d", region(0x7fcc90, 8))[0] == 20.0
assert struct.unpack("<f", region(0x7fcc98, 4))[0] == 255.0
for start, end, digest in (
    # Room exit uninitializes elements in forward order. Type 2 looks up the
    # source ID, clears attachment/ID (not depth), and retains element links.
    (0x414960, 0x4149dc, "cccc29acda794280e269928e34aa40f1415ed983e47967492eaabee0869268ff"),
    (0x414c50, 0x414c8a, "e09d862c8bf049f5b1fce0a95ec22578bffb3ebf29795a2d6d039ca37deba518"),
    (0x50e791, 0x50e79d, "bcc4cf1446e0e92357c864fe9781bb3319f58e39c8fcf3d3180e96ff0a6b6b2e"),
    (0x48f0a0, 0x48f0ce, "856591947d3ad925a1b2c74121e0d42a779ebd948074a6ed84e22d573eee6ed6"),
    (0x48f1ce, 0x48f3e7, "d870fccbf690e0e4893c813bfa847930f78107ff9be5ef0a107de38b90f72779"),
    # Background payload: unit scales, raw animation copies, RGB/alpha split;
    # type-1 initialization jumps directly to the initialized-flag write.
    (0x413bc9, 0x413cb7, "fa67f36f83e09cdde02dc70cf91e79b181401c26aff8b396bd6e3f32a05a7075"),
    (0x412e87, 0x412ea5, "564d689d3aeb5eb9c7ca8a807e233508db07d9e430f750fb4622c875460b0fb1"),
    (0x41306b, 0x413094, "736ffe601f01900f0099b8d051282087c2f097432977eea3941422de1b2a397d"),
    # Global instance-ID buckets: 512 head/tail pairs, tail insertion and
    # ascending-bucket traversal used by room layer initialization.
    (0x401290, 0x4012c6, "c9627d4fbfafea1880f69ed1f7c7c3870cb314cab4795ad383412e5e64abd9a3"),
    (0x488818, 0x488889, "17eee26261072a69174cb1d5c1759b121c30694e32a1cd890221d3a36a796e61"),
    (0x488c54, 0x488cc2, "1a5a916ce797d2a37f6b909e7a8a6fe3d5be771130c317202ceb3dd431e15a50"),
    (0x422770, 0x4227bf, "ff5c5e79fb688379fc7ba39c66c15198712427b0b082f8c0bfc88bd12d914aac"),
    (0x413500, 0x413549, "ebeaeb70210528b11a83de37093e401013b849bc66dd28ba8bbf758cd19844e1"),
    (0x413549, 0x41363e, "941dc62c4376f9ea73d5bac93c9221c623e65169cb0a7217ff0976420b20873f"),
    (0x41363e, 0x413658, "535695633b44d55665158f58bf3b9505f52c99e1d2f944b4b4dd45730778bd68"),
    # Destination layer initialization precedes the serialized birth loop.
    (0x50ff82, 0x50ffa5, "01e92eb21b75bf6ac2038e3f56b09bbe5354617553d35b947a2574e00bacea97"),
    # Resource catalog loop, serialized-layer and empty sequence-list branches.
    (0x496cd5, 0x496d52, "fa126e1b78f63516f014f6efd70b05c2fa8bb25fb719a4a8b49fd213b06e9cc2"),
    (0x496dc2, 0x496de5, "da43f5890ac19906acd9026004828bee259248d45836db66af69290c471952c8"),
    (0x48b497, 0x48b4a9, "71bf9d7ba8d2065829fd9b9c94aa48316002046b2d46be627f112af8f7eb0f73"),
    (0x48b930, 0x48b974, "518029927e4253d985c94a63f08bdecee20b853697b90993e76488cbd223fffe"),
    (0x4d962c, 0x4d9637, "eca37ec7e9ad2b655ef98ed0622723b13993e26fe9351ae24801c94ecd84c4d3"),
    # GEN8 >= 2 selects the nonzero global flag, hence the layer path below.
    (0x4979e8, 0x4979f3, "6eac5342e3808148018dd3a35a296710671b01b546f4a319e5d6d17ecdea715c"),
    (0x430c91, 0x430ca7, "dae81cbfd493cd8f9a401a945f9f90f0a471a984dc906dbeb129bca11520c7ab"),
    (0x42d820, 0x42d841, "a6c93a5d3715e441c55ba35111fecd2dbe135ff5eff11c0c00ff5269b73288fd"),
    (0x431ce0, 0x431dab, "5627ea0186a90ba7885deb954a7d9d146cb62a04b65b347164c1dfab27b2b995"),
    # Activation service precedes pending depth service.
    (0x510900, 0x51097b, "215ce2528a1df233e1de7e48d862858fd16978244d8706c05b8b2f9fd69d686e"),
    # Selected queue loop calls layer migration, not room-list reinsertion.
    (0x50d220, 0x50d276, "7d021a4c5a9733220fa1547d2130da8864e453431f4149b46023726417627937"),
    (0x414660, 0x414742, "c37921a98596320ce8c9fa34dd260b5f1eb1a8c06c9ae4b11d8c542d48e6f249"),
    # Dynamic layer selection and instance-element attachment.
    (0x412820, 0x4128ce, "844ff5dceb7f1212b0926d58e5c05830284953ef899ab8fdfa9d7ae0bf27e22a"),
    (0x412a30, 0x412ab0, "6cda25459cdff969edb5900e9fae6122b469b20b55eac581a44d01bc29e67819"),
    (0x412ab0, 0x412bdc, "d6430edf874ff3b161026a9741e2f0f9772fc6476d05c9f08d5f7db9d00acc89"),
    # Room-entry fallback prefers the stored layer ID (including fixed), then
    # searches/creates a dynamic depth match before ordinary attachment.
    (0x413590, 0x413613, "c55dde2205b4c4d035dd44698216b4746fd49974cb1ece01ad4eee5659c20ae0"),
    # Serialized type-2 initialization binds an existing instance without
    # overwriting its depth or moving inactive placeholders to the tail.
    (0x412e60, 0x412fe7, "3de1827d6b974ffadfed1b4d6507b8ae0eb499634a49ed621a7331563f0cedd5"),
    # Element insertion, layer movement/merge, and reverse inactive tail moves.
    (0x4128d0, 0x4129cd, "be01d3600e3a984f13b2927abc893d7cb279e2449dcfc4c2f68ac3910b4d1f8b"),
    (0x4147b0, 0x414949, "0b5d221b348ec2f8d5ed5edee3b106279214ac4a81c5c2541f17ee49f07db835"),
    (0x426630, 0x4266af, "dc06a4a0d158f6f9b5f6e7ed1d29c4904841043923fbfaec0e9eb210b4c668fb"),
    # Instance detachment and empty dynamic-layer disposal.
    (0x4258e0, 0x42595d, "617c2bfaf8772dee654e8b1809276df86290e756396612c9090202ffa81747ca"),
    (0x425a40, 0x425b5a, "b20cd62a9d8c6490c080e1e04578b96668ec8d7cf174e7857b867d55e56513a7"),
    (0x425b60, 0x425c1b, "686844692cb51f131db70e17575d63c0a92b6a134ee41848e78c9efb1892591d"),
    # Ordinary Draw selects layers by positive room layer count; tail-to-head
    # layers, head-to-tail elements, inactive instance ends the element loop.
    (0x490dac, 0x490dcc, "554b33c5873f4a484c6d043c5297b0102ad377481225e86862c58a23ce5dd309"),
    (0x49037c, 0x4903aa, "42075b233d40d2b22d29fbaef10c46ea61cfbf9c284ff4f33a46d25ecbe5c010"),
    (0x4904b8, 0x49059b, "a115423ce85817e4e1075ff3fee15904e26034e5098c8e906af25c92183044a8"),
    (0x49074d, 0x490767, "8211696ebdffabaa68d2d4d7569be09e9f533e6c8356a661e1ac021ed6b7d8b6"),
    (0x49081e, 0x490835, "1b2d77b572d1a762154ae9bb95552c8db2f2b92ff5de9e4fbe3adb909a214147"),
    (0x490a64, 0x490a84, "626112d83e906f2a98d62865b5b7b392f8cbd5ccea5fc5a0b7a2acbbe69a3a1e"),
    # Room activation reconciliation invokes the selected layer re-linker
    # after object membership publication, for promotions then demotions.
    (0x48d760, 0x48d77f, "492fbee702f7d86fa5eeadb12fff133c47a3a4abfe606a86ae7e6083f8aca52c"),
    (0x48d848, 0x48d867, "b21446bbed167f74ffae0641c0c359b5b9e3268b072ce17bf830e806793d163a"),
    (0x4266e0, 0x42679c, "dd0babd9e0005cfafac6148ce742b90df2f432e0b00dee603bfe59dabb17fa31"),
    (0x412a00, 0x412a2f, "09296d5f50a7d421fd9a2726608a0300751e1ef08cbf62e0f854c785cab54d0e"),
    (0x412c20, 0x412c52, "e88d63238b22f89cf79b8e7bca0fbf0f6ad5f847e7a9ad33c919bbc267b79294"),
    # Serialized layers, placeholder instance elements, and room instance
    # attachment through the source instance-ID to layer lookup.
    (0x4136a3, 0x4136cf, "8eb8dd13087f44ce329a1f856617398b3a44e9f6010718357d465ac27abaa4db"),
    (0x413701, 0x41373e, "96fdd96e0f3ea69586d98efe4fb2bb16901af6a81d9746b3ab7d404f51d1c079"),
    (0x413cdc, 0x413cf6, "b9b8fe7dc78bcb0cd471a9ce9f7cdffa52eb68291854200fe5e65147a15772a1"),
    (0x413dee, 0x413e2b, "b0a9b78e517ce89bbe47e42b40eb27260a78af3f690fa6f50a4ef07b47194bdd"),
    (0x4143f6, 0x4144e8, "d409721cd9222c5a51a9a61c5b19551340e698ea0c70521a4e06085b870e2bec"),
    (0x48871f, 0x48873d, "98e137270f00baad3232036fdb8fcfc9dee532275112f1942cc83b4fb22e232c"),
    # Retirement removes an instance from the pending queue by stable compaction.
    (0x50f6c5, 0x50f6d9, "f00979b8847f3a5a9dd9913d8dbaae6267fc1618afbe618eb9a31f490d0fc389"),
    (0x50d740, 0x50d793, "6ee03835d9451e9681567d9b5cccda322bdf74663a62c36896bbff3127a9d541"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x50d223, 7) == bytes.fromhex("80 3d 18 7f b5 00 00")
assert region(0x50d22a, 2) == bytes.fromhex("74 4a")
assert 0x50d255 + struct.unpack("<i", region(0x50d251, 4))[0] == 0x414660
assert struct.unpack_from("<I", region(0x490a64, 32), 4)[0] == 0x4904f4
print("Pinned depth setter, unique pending queue, and selected GMS2 layer migration")
print("Legacy active-room re-sorting is not the selected depth-update path")
print("Pinned positive-layer-count Draw selection and inactive-instance layer termination")
