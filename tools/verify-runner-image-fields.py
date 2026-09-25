#!/usr/bin/env python3
"""Guard reviewed image field binary32 storage in the pinned Windows runner."""
from hashlib import sha256
from pathlib import Path
import runpy
import struct
review = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = review["region"]
assert region(0x7FE3CC, 12) == b"image_index\0"
assert region(0x7FE47C, 12) == b"image_speed\0"
assert struct.unpack("<d", region(0x8A6C28,8))[0] == 0.00001
for start, end, digest in (
    (0x53CCC0, 0x53CCD8, "429bfb329e7c3020ceb72c0ae7086ce28e549e0de21e88bb5d4c09dd42c2feb0"),
    (0x5783D6, 0x578436, "97cb4491399d353d32a0548944e463abc7f7ca40b80f3c296e75c26e15e2b2c6"),
    (0x5797E0, 0x5797F8, "810c7264c1db450267c25e5b4de5857c26c0d535d5046183c5734d33de2c172a"),
    (0x430D0C, 0x430D22, "dfd0255b5282e2fc5ca46c4826e10ff4ff839c2f5d5bab168e923331a441b187"),
    (0x430E0E, 0x430E24, "88a8f4957a1aef678da7b40e03bfa06b2796bbd211a2bc7deb0b77e270ac5c70"),
    (0x432170, 0x4321AF, "3f6cd2270d083f86b2c714d83cdde1a214caff6308d7d3892394a05a8f2e622f"),
    (0x432250, 0x43228F, "b8c74954a310548a83b0331b5b1729671e9f8b8d0d62c5fb49e57d83906d95e1"),
    (0x42DD90, 0x42DDB1, "3b477aa2f2db6874eb6234c247eafe4a22b7af19e95a40da6d305d84c09f2fdf"),
    (0x50EBF0, 0x50EC06, "8fc0910f1d2d8035c3e83103aa75e5a745920681156c3f47d18a7368915a5a00"),
    (0x50EF0C, 0x50EFC0, "ed16169d69d02e8d555d838382f5d0c4899fc369551a6a518ff2d01da3ce9bb3"),
    (0x4976F0, 0x4976FE, "65f22d449a57f8c5cdc48b36d48cf9fbd10d8e35ae336bd4e00f016b2a455815"),
    (0x4977B2, 0x4977EA, "824cdcbaeebbff97a9fff0d00dda53a3b6598d420262984f56fd74df7c114fc5"),
    (0x4979E8, 0x4979F3, "6eac5342e3808148018dd3a35a296710671b01b546f4a319e5d6d17ecdea715c"),
    (0x50EE87, 0x50EEB0, "4a4b6db66c7c81da6f89b95b993963ff03e9b0fc23735059c1ddddd03c8984a8"),
    (0x50DBF0, 0x50DC89, "5bde1a04d1364494f8b93200c107ebd208782a0489c031f2a8894ae835433004"),
    (0x50EE2A, 0x50EE87, "700ae1a8476c7deac0f9947ba512bbb97a1673ff2424fcea5e02dec776da0e47"),
    (0x4E2855, 0x4E289C, "8a6a5da926ee43702ade4633a4f4d912926041b974b33981af6e1e6829a52d44"),
    (0x4BBD30, 0x4BBDA1, "ae0718a149130b196208ffc60c39f54486214dbc501370993a70e5c566e4f738"),
    (0x4F8FAA, 0x4F906B, "1bbe042cc209e6ddbf1739a9cd411617650e6b87df8bca40b0255cbe66b3311c"),
    (0x50EC49, 0x50EC8F, "ee18698466337c93ca6659489fef13e535d4efe9c615896c0cb4624017b1e0b1"),
    (0x430CF6, 0x430D0C, "ee0c82d0ddbb8696a0b991f8fe567f017599582f3a34091dc535460491955884"),
    (0x433550, 0x433569, "b39f2fde92a33d98e829dec4b2c1655d5e84bf0d8d69bb77fab7451d9d8e143d"),
    (0x46BDF0, 0x46BF17, "48dc1448c8be5655f4078a57639f8bc6d57da45892aab1beb216aeb4bc234ce9"),
    (0x50ED87, 0x50EE2A, "ba7a1a811120ba7609b1fb28b6ff38f2231034aa21921042e6970748a7cfee8f"),
):
    assert sha256(region(start, end - start)).hexdigest() == digest
print("Pinned Windows image field registration, narrowing setters and speed getter verified")
