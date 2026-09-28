#!/usr/bin/env python3
"""Pin reviewed object-266 events used by the shared source-step contract."""
from hashlib import sha256
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
assert manifest["source"]["sha256"] == "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
obj = manifest["resources"]["objects"][266]
assert (obj["name"], obj["parentId"], obj["spriteId"]) == ("bulletDrone", 89, 458)
parent = manifest["resources"]["objects"][89]
assert parent["name"] == "parentPBullet" and parent["parentId"] == -1
assert sha256((export / "Assembly/gml_Object_parentPBullet_Create_0.asm").read_bytes()).hexdigest() == \
    "2a8a1682f62d2b8a80b389ce6bd90baa2e1165b9c1d9b54fc2a0c82f5bfd49c6"
sprite = manifest["resources"]["sprites"][459]
assert len(sprite["frames"]) == 6 and (sprite["originX"], sprite["originY"]) == (8, 8)
events = {
    "Create_0": (2268, "4a73ce9a102aa1f6e41253d42c694db83ca59256c1905c484f236a23bc2f8576"),
    "Alarm_0": (2269, "5fe9821f231d7373f71f2df0f42a481f744b4a2d5a482fb3eac90ca460b6f600"),
    "Step_2": (2270, "5daab67fc60337936392d83c43dc43d2068d3b73e937650cc1c5e8fc1264913f"),
    "Step_0": (2271, "0ed2783e86ea549da2ab4c3bfc4632f950fc0b4f7cb171d77f9131444e34b12e"),
    "Other_7": (2273, "7fa420002321759f60d9103f49806b50a0673341e139e47b46bb18672ccf6d78"),
    "Draw_0": (2274, "cccfacd2babdeeb250bc8700308bc9f01c53cf4a0806704590c3dce3cb5a55f7"),
}
actions = [a for e in obj["events"] for a in e["actions"]]
for event, (code_id, digest) in events.items():
    name = f"gml_Object_bulletDrone_{event}"
    assert {"codeId": code_id, "codeName": name} in actions
    data = (export / "Assembly" / f"{name}.asm").read_bytes()
    assert sha256(data).hexdigest() == digest, name
step = (export / "Assembly/gml_Object_bulletDrone_Step_0.asm").read_text()
assert [s for s in step.splitlines() if s.startswith("call.i")] == [
    "call.i random_range(argc=2)", "call.i lengthdir_x(argc=2)", "call.i lengthdir_y(argc=2)",
    "call.i lengthdir_x(argc=2)", "call.i lengthdir_y(argc=2)", "call.i gml_Script_scrBulCheckSolid(argc=0)"]
assert step.index("call.i gml_Script_scrBulCheckSolid") < step.index("pop.v.v self.x\n") < step.index("pop.v.v self.y\n")
for name, digest in {
    "scrOutofview": "e06b55bcc8952fd6eb918461ffce8196a41cff92c7f7dfe97060160468a2fcff",
    "scrBulletOov": "1b393802ddd4cc84545f3571f4a244d0baf692d7ddc1ae49a3b8c9aefeb1f022",
}.items():
    assert sha256((export / "Assembly" / f"gml_GlobalScript_{name}.asm").read_bytes()).hexdigest() == digest
end_step = (export / "Assembly/gml_Object_bulletDrone_Step_2.asm").read_text()
assert [s for s in end_step.splitlines() if s.startswith("call.i")] == [
    "call.i gml_Script_scrOutofview(argc=0)", "call.i gml_Script_scrBulletOov(argc=0)"]
print("Pinned bulletDrone 266: first-Step RNG, Alarm deceleration, solid-before-position and draw/expiry gates")
