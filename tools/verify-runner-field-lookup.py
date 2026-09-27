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
    (0x479328, 0x4793bb, "e09bfccc39cfcb82c81dce4f49ef2632dd3998b67324d967efe88a843b421a18"),
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
    # Explicit retirement: root-retry removal, indexed flag, both dirty lists.
    (0x501f70, 0x502098, "90432a2851366b11348bd01edc0cfdbba309d9175126691a1d9b115f97048c38"),
    (0x466045, 0x4660b9, "531eb148f104406280a20a2993122756bc804f3372be8a8fd4a9d3715d817d06"),
    (0x46669c, 0x4666fe, "217756a0efd85d15bd7b89020e34bc2625cef26c621972f7ded7d570a2dfd771"),
    (0x4666b6, 0x4666fe, "786128d7493dc128f14a60d7746e8a1696ae91901cff4fd38e21feb1d21e001d"),
    (0x5020a0, 0x502241, "b4fe75e56e734480d7f25573db6e6ea9a35a1c25236576762826453a690b761c"),
    (0x52cd08, 0x52cd1b, "44e72dafd1f48a74fb4709652b9dcce60d86ed1502e3f142f57ff91afb8a4e08"),
    (0x529e50, 0x529ebb, "0a53a026073fef6cdb42aaac33df23820cc84026c94c69601d9a219afddfa30c"),
    (0x4fe560, 0x4fe8b1, "92aba2c026ca90179868445275b9736946e775ef155ac85e9b3757dc1ff5599a"),
    (0x500d30, 0x5010d5, "8ab166c1fa3996370f74bdf24ac388173e7cd745d5fe59f818ceeb7d84b575ef"),
    (0x4798f0, 0x479951, "a37d63bc53c6e8dccc5af694b3c84d0df396305ad21de4408e88a078f4218358"),
    (0x47b740, 0x47b78a, "c955cf45d9c30e2e49a481b40eab91104affe58df06e278cb46c83ab6aa04cbf"),
    # Initial flags, inherited events, descendant collision targets, endpoints.
    (0x47b7c9, 0x47b8d1, "3fff4294221825fc0ea7bc36ac19509a43f92b5ea891e4d6952b2ec4ab2fdef1"),
    (0x478550, 0x478582, "f4920da5e35ae3d1d98aeecfcaf11caaa66abfea52a44bfcb8b96c6a70f694e7"),
    (0x478830, 0x478b60, "677fe6e09bcd1cab26cffc605cf9177ec3fbd07482590d493a12f874e25faf03"),
    (0x478b60, 0x478e2c, "d649039de47de8d9274d8a0167b08345e1d369f72101240cb1cecd28bbf939f5"),
    (0x4782a0, 0x478373, "6b5c800439dbec8fdc6b477affaba2e237f9feafc6194083d995e4e4cf868d3f"),
    (0x47a116, 0x47a1ca, "670d7a98cc5d7dd7e082f39d1963ab91f8fcaf37e3cbbc7215325916ac0b7c82"),
    (0x47a2bf, 0x47a321, "bac12cf091ea7f64c2ae682711236a9758ab3a3fbf6244c264c1e519387fc8d9"),
    # Dynamic birth: object properties, ancestor preparation, direct bounds
    # publication, then PreCreate/Create in the outer wrapper.
    (0x46b8b2, 0x46b968, "3e69c8d2b83943e5082497d4fe1a0c318c19d6a75d2d87ce7486196d340fff8d"),
    (0x4879ca, 0x4879f9, "535bf8b3a2d70fd9afd80154394a18a3951636397ee46aaaaac478ce1dc573c3"),
    (0x501dc0, 0x501e39, "4751cef7426104833df6ea3b254c2ee65285756569c641373f018abe9bdc236b"),
    (0x4fdcf0, 0x4fdd9e, "0900e53961c9b92bbb3d0b7fd7a3201651efe731aeff731ae2ebfd217fd3b17e"),
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
