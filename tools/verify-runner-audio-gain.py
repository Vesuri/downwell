#!/usr/bin/env python3
"""Pin reviewed gain controls in the untouched Windows runner, not DSP equivalence."""
from pathlib import Path
from hashlib import sha256
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    (0x558d70, 0x558dae, "a1455debdc66129e2b5ce096d68a2827b472f68de819e2858eb49eef37814ade"),
    (0x49e340, 0x49e39e, "a34bf8cc71f993441d7d3d1964bc564e6c8f8c913ff89b5f23ac1ed9fd8bef7a"),
    (0x4a0ef0, 0x4a10a6, "3decfaa0b10f4a8df2a4ec11b11e42174ddf1c2c38064f1d196e8998d9ee10ba"),
    (0x4a3670, 0x4a375b, "afeaac2496db5b3687c9861bdc244634e8c1dc33b64317db36b722883a25847d"),
    (0x4a2fc9, 0x4a2fd5, "6c8ad6eca6ca17b5fa0d4928f8e9cd91058798173f9ab5007cabfb0c9f33989c"),
    (0x49f26b, 0x49f27b, "34d80d7393b4e83f6098cddff67b4376366097c6aa5abb693c6c686d55e99e03"),
    (0x4a21f0, 0x4a2247, "fee414c0e8d1adae33cf5ec59773dc52b3a24dc0e3d7aeae2099dd9b85234c56"),
    (0x4a22d2, 0x4a236f, "0a56942a917a74b14132b35f83b1d6c462f8e02f986263380e50681cbb090735"),
    (0x4a23f9, 0x4a249a, "563d70b0f581d125595a53a30b4f71dabe74cccd05c441eb7c982bf8cd5c2caa"),
    (0x4d0230, 0x4d02ab, "33e7fe765ba9e8f8f1db487cccf177ab60820e7d6df62cdf5b73a513d23101f9"),
    (0x50f4ea, 0x50f559, "c01551f626b44554ac9b6d749a003f2efea00a4004f6a5f9033a41c77634e309"),
    (0x49edb0, 0x49ef5e, "484ba223a4c78bda7dfc412e13e87f06feecf7049942642f7bce88d121f93f46"),
    (0x4a0370, 0x4a0533, "37c7ce6ce8d7ffbedc58641ab5453dd7637c7c89631cd4979f2696455b10d610"),
    (0x4a1110, 0x4a12d0, "33e70cfc5103712dc583f9b3ea4f58158090295868642eeb2c2d1c74a4e835a4"),
    (0x49ec50, 0x49ed30, "8fee5c3ba0498ad35333ee46e9222ce96fbc7717bb45192caf8496c39aa1c779"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x795ab8, 4) == bytes.fromhex("0000803f")
print("Pinned runner gain narrowing, resource/voice separation, initialization and volume multiplication")
