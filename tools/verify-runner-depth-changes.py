#!/usr/bin/env python3
"""Pin depth writes and the selected GMS2 layer service, not live parity."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

root = Path(__file__).resolve().parents[1]
audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
data = (root / "data/data.win").read_bytes()
assert sha256(data).hexdigest() == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert data[8:12] == b"GEN8"
assert struct.unpack_from("<I", data, 16 + 0x2c)[0] == 2
assert region(0x7fe384, 6) == b"depth\0"
assert struct.unpack("<d", region(0x7fcc90, 8))[0] == 20.0
for start, end, digest in (
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
