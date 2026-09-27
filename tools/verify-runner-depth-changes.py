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
    # Retirement removes an instance from the pending queue by stable compaction.
    (0x50f6c5, 0x50f6d9, "f00979b8847f3a5a9dd9913d8dbaae6267fc1618afbe618eb9a31f490d0fc389"),
    (0x50d740, 0x50d793, "6ee03835d9451e9681567d9b5cccda322bdf74663a62c36896bbff3127a9d541"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x50d223, 7) == bytes.fromhex("80 3d 18 7f b5 00 00")
assert region(0x50d22a, 2) == bytes.fromhex("74 4a")
assert 0x50d255 + struct.unpack("<i", region(0x50d251, 4))[0] == 0x414660
print("Pinned depth setter, unique pending queue, and selected GMS2 layer migration")
print("Legacy active-room re-sorting is not the selected depth-update path")
