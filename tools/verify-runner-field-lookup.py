#!/usr/bin/env python3
"""Pin reviewed instance_place dispatch; does not execute the Windows runner."""
from hashlib import sha256
from pathlib import Path
import runpy

audit = runpy.run_path(str(Path(__file__).with_name("verify-runner-choose.py")))
region = audit["region"]
for start, end, digest in (
    # Activation region wrapper/predicate and immediate flag/dirty transitions.
    (0x52ce57, 0x52ce6a, "3821ff37ce05c24591f9a3ed106968438c99c6893b7ba97f9bf49bdee01d0482"),
    (0x529660, 0x5296c5, "522eef58be76c480be6f5232f11526195a3ac6a87540136768b12e59cc103f89"),
    (0x52d040, 0x52d14d, "eda4791dc3d4f7433065e50a923ed1dfca3e1659c08c1f33a006be93530b02c3"),
    (0x467640, 0x467678, "cba757068aff96e0b61205df89cbbb28685f611c14f5b507a3d681e9b888ef72"),
    (0x4627d0, 0x46281c, "0ca6095a8151103bf924542f817fd86226f3edbce56733c6faf45d694bc4975e"),
    (0x4633e0, 0x463445, "e8f7c44bc1af35f9f644d3adf261f62d9714daaccc3cd8f64eba9cfdf30f0411"),
    # Constructor -> object binding -> ancestor-inclusive tail insertion.
    # Deferred reconciliation: room-list order, not pending-request order.
    # Layer-element integration hook and its stable unlink/head/tail primitives.
    (0x4266e0, 0x42679c, "dd0babd9e0005cfafac6148ce742b90df2f432e0b00dee603bfe59dabb17fa31"),
    (0x424b20, 0x424b57, "ef295cf1e4073eb69e536d2fb57d33fbb2627c8f914c743548933e99c8e06032"),
    (0x412c20, 0x412c52, "e88d63238b22f89cf79b8e7bca0fbf0f6ad5f847e7a9ad33c919bbc267b79294"),
    (0x412a00, 0x412a2f, "09296d5f50a7d421fd9a2726608a0300751e1ef08cbf62e0f854c785cab54d0e"),
    (0x48d630, 0x48d873, "aaef6d7b86d7d4c55befd9ec54f2a33d49f7bba4a41e6df923a5d9ede738c676"),
    (0x510900, 0x510925, "34b773c6ef3f329c07215974a76fe21f9da1595426ba3ccddb92c015e972ffa4"),
    (0x50dc7f, 0x50dc8e, "f184fab978a5ed06fc93573d2ab00e1d0afe30b2d97f5a91c7f2e6bcf29d3562"),
    (0x46171f, 0x4618ee, "c6449f2c7c86b62c2053956c6c6aaf60d5bc8caf5f545f8c09102fb0e07d733d"),
    (0x46b830, 0x46b8b2, "f69e99dd410a652360766445ca48c817b571a44dc5089db9ada54cd10022b2ea"),
    (0x478380, 0x47841d, "ef7694656f4ae8ac1934fb8d1dfac089f085d12408bd86352ef5ad609758e4a9"),
    (0x479880, 0x4798e6, "f1fcec78f8a0030802a25deb2000505047308032028eacd677b77cfd2fc680ef"),
    # Spatial insertion/root growth and recursive child selection.
    (0x4ffd40, 0x4ffe6f, "08761b0415c1031c8e3851173fbe379b018c68758869b32c1154cf3a8b60b56b"),
    (0x4fff80, 0x50018f, "4a66ec6c3c45b6a5f35abf6c841913b3f8e6109e890b8efd3e4477cbf47450b9"),
    # Six-plus-one split, minimum-fill assignment and quadratic seed choice.
    (0x5010e0, 0x50146a, "a6243773257524fb7c886e6955077d7fe43f35afb6174eda79915711420b9f78"),
    (0x4fc680, 0x4fc9ce, "19dbcb5e5d0e7b8daaa0bbb5f9e5cf3eeabc67b8c2a784c07c80e093c554b1d1"),
    (0x500490, 0x5006d0, "c197a09c52e4e2f81d3e724a120a5ba721b508a189dfae5312e6ba99d74331f3"),
    # Removal retry wrapper, condensation/reinsert, recursive erase, dirty pass.
    (0x502990, 0x502a79, "cce8a72b5fbc73b2eca5a42c9030224c78e5c99eaff73bf392b1a71488a0aa6f"),
    (0x502a80, 0x502b38, "e1124f135509039809b9782f88232e54514fc1d18779e40d70201d0e148e881a"),
    (0x502b40, 0x502c91, "9c165fa8b71bb910cdbc0ca31303510b43835becaa544dc01fab25223ba22735"),
    (0x502ca0, 0x502faf, "9f9f366b1e0120d789eb7f43a62f0dacf4732460d39a87460b128822b4f88f6d"),
    # Position mutation, spatial move-to-front and separate collision queue.
    (0x46bc60, 0x46bcbb, "d0d664eb8e95cc16eb60ae2e43f028f3cb86e8d2c65a6ed1534dfc9006d0412d"),
    (0x501e50, 0x501eeb, "01e304875dd3765c862d7a678598cb8c735cc55ac29cda95ca6ee0207f70e0ee"),
    (0x501ef0, 0x501f6f, "e1929af99fd7ef2ae676ad677567236da7a1acbf410a363640afb1ffabd545c6"),
    # Bounds-refresh publication gate and direct index publication.
    (0x4666b6, 0x4666fe, "786128d7493dc128f14a60d7746e8a1696ae91901cff4fd38e21feb1d21e001d"),
    (0x5020a0, 0x502241, "b4fe75e56e734480d7f25573db6e6ea9a35a1c25236576762826453a690b761c"),
    (0x52cd08, 0x52cd1b, "44e72dafd1f48a74fb4709652b9dcce60d86ed1502e3f142f57ff91afb8a4e08"),
    (0x529e50, 0x529ebb, "0a53a026073fef6cdb42aaac33df23820cc84026c94c69601d9a219afddfa30c"),
    (0x4fe560, 0x4fe8b1, "92aba2c026ca90179868445275b9736946e775ef155ac85e9b3757dc1ff5599a"),
    (0x500d30, 0x5010d5, "8ab166c1fa3996370f74bdf24ac388173e7cd745d5fe59f818ceeb7d84b575ef"),
    (0x5006f0, 0x5007e5, "00e84ad3ec45574f04c6171402e1ee0b690853971f12a44d6e7d57f5d9b5077d"),
    (0x5007f0, 0x500851, "69a31ba1914e1939fd06b7dddcf9b66e545a07ae61b42340793d0441ef970024"),
    (0x5016a0, 0x50175e, "f5a5bb7bac93310410275af37e5b88a7011205e9a21326f037843223cb16b542"),
):
    assert sha256(region(start, end-start)).hexdigest() == digest
assert region(0x80cd7c,15) == b"instance_place\0"
assert region(0x80cee0,25) == b"instance_activate_region\0"
assert region(0x7fe7e0,4) == bytes.fromhex("000080bf") # preference sentinel -1
assert region(0x795ab8,4) == bytes.fromhex("0000803f") # seed sentinel adjustment +1
print("Pinned Windows ancestor-list registration/removal, instance_place dispatch and first-hit traversal")
