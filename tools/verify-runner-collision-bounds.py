#!/usr/bin/env python3
"""Pin reviewed Windows bbox/meeting code; not a runner execution test."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x42d500, 0x42d58d, "01d352d29507697e25bcc4c1321372c36791afcc0bb8f99b6276e7caf04f15f7"),
    (0x52b1f0, 0x52b27f, "32dad8432b67077aa41fdcf5a2a9fe7aba74fdeaeb856f435ecbadcacb80755d"),
    (0x466010, 0x4666fe, "44030a7ef3c09e6484c14483fb429217d33f1e087d6b97a8c7327e1b0b91f22f"),
    (0x47bcba, 0x47bce0, "da4711425d941efd63fecf899c4d91e0aa274a2b22620cf9a7ca79b8f86c9e14"),
    (0x4985d2, 0x4985e9, "f22a7e471ee17b0bd02cdcb02f59b629ba48a21460715553aa7af698a73d4ce2"),
    (0x47b9f0, 0x47ba11, "585fd396a2619d2ca5f71c137b87e5fce0d15e2a7ea52ccb6b51b26061b4c866"),
    (0x464d30, 0x4650ab, "5715cddc60c08661840dce8488493b71ff4820d976cb48d282d3702a494cbdf8"),
    (0x4b8eb0, 0x4b9227, "0d24f4ba2a1d20fac0d39aa798b10d393e070d2f45289e84bedbfb709b4258f2"),
    (0x4b9227, 0x4b93f6, "d061e6ea5cb8c0d63bfccd1045f1214bef141e9db85228e993cc8c386a65c303"),
    (0x4b93f6, 0x4b96a6, "09f250dd406633f83c22398a96544a9cc1f391b185bc300f761c97b7631f8ff1"),
    (0x4b7b3f, 0x4b7b5f, "ee9d98d86a41c40ba91c5338c3c8870333f9321b20313c1d2bfc7d95b0ef24da"),
    (0x4b7e3f, 0x4b7e61, "4c167845629831eab10a85fee7a1c54e1a1b85361ba1f90049e100b1e420e8f3"),
    (0x4bbbb0, 0x4bbd23, "5a2c64a5f93bb3335407bbb57892af85f1ebfb70c502e91dde7194dabfb226eb"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x7fe38c,10) == b"bbox_left\0"
assert region(0x80c958,14) == b"place_meeting\0"
data = (audit["root"] / "data/data.win").read_bytes()
assert sha256(data).hexdigest() == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert data[:4] == b"FORM" and struct.unpack_from("<I", data, 4)[0] == len(data)-8
offset, options = 8, []
while offset < len(data):
    size = struct.unpack_from("<I", data, offset+4)[0]
    assert offset+8+size <= len(data)
    if data[offset:offset+4] == b"OPTN":
        options.append(data[offset+8:offset+8+size])
    offset += 8+size
assert offset == len(data) and len(options) == 1
marker, version, flags = struct.unpack_from("<IIQ", options[0])
assert marker == 0x80000000 and version == 2 and flags == 0x28400214
assert flags & 0x08000000
print("Pinned Windows collision bounds and OPTN compatibility mode: enabled (integer inclusive edges)")
