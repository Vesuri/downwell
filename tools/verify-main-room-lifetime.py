#!/usr/bin/env python3
"""Pin the VM event bindings used by the current live retirement adapters."""
from hashlib import sha256
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
export = root / "extracted/utmt-0.9.2.0"
manifest = json.loads((export / "manifest.json").read_text())
source = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert sha256((root / "data/data.win").read_bytes()).hexdigest() == source
assert manifest["source"]["sha256"] == source
assert manifest["extractor"]["version"] == "0.9.2.0"
assert manifest["extractor"]["commit"] == "03c7048438347799e0717b6e57a9358d000472e0"
simple = "71aaac10eef02f61e15327032a995fe459f49451cd38b8434e071ec96268b315"
for object_id, parent in [(93,87),(87,56),(56,-1)]:
    obj = manifest["resources"]["objects"][object_id]
    assert obj["parentId"] == parent
    assert not any(e["eventType"] in (1,12) for e in obj["events"])
for object_id, name, subtype, code_id, digest in [
    (93,"objThinPlatform",2,1091,simple),
    (111,"enmSnake",2,1194,simple),
    (112,"enmGroundTurtle",2,1200,simple),
    (113,"enmFrog",2,1208,simple),
    (116,"enmBat",2,1232,simple),
    (117,"enmSnail",2,1239,simple),
    (119,"enmCrawler",2,1255,simple),
    (92,"objBox_n",2,1087,"0357a7eb6718651cbbc144371c48b9a1f2445a6b0131ae9f15ac404d8880c865"),
    (289,"Scattered16",0,2387,"28f7516d6d107d7dadbf1ccae5a02792436e188cbb5c97f0295754906c4fec8c"),
]:
    obj = manifest["resources"]["objects"][object_id]
    assert (obj["id"],obj["name"]) == (object_id,name)
    code_name = f"gml_Object_{name}_Step_{subtype}"
    event = [e for e in obj["events"] if (e["eventType"],e["eventSubtype"]) == (3,subtype)]
    assert len(event)==1 and event[0]["actions"]==[{"codeId":code_id,"codeName":code_name}]
    assert sha256((export/"Assembly"/(code_name+".asm")).read_bytes()).hexdigest()==digest
assert sha256((export/"Assembly/gml_GlobalScript_scrOutofview.asm").read_bytes()).hexdigest()== \
    "e06b55bcc8952fd6eb918461ffce8196a41cff92c7f7dfe97060160468a2fcff"
# Native box End Step specializes the natural Create invariant. Check the
# complete root-code export, not only the three files where the field occurs.
assembly = sorted((export / "Assembly").glob("*.asm"))
roots = {c["name"] for c in manifest["resources"]["code"] if c["parentId"] < 0}
assert {p.stem for p in assembly} == roots and len(roots) == 2501
corpus = sha256()
references = {}
for path in assembly:
    body = path.read_bytes()
    corpus.update(path.name.encode() + b"\0")
    corpus.update(body)
    text = body.decode()
    if re.search(r"\bautoTile\b", text):
        references[path.stem] = [line for line in text.splitlines() if re.search(r"\bautoTile\b", line)]
    assert not re.search(r"call[^\n]*\b(?:variable_instance_set|variable_struct_set|execute_string|execute_file)\b", text)
assert corpus.hexdigest() == "f0fdfee7e3ef7d80d4963bf41b032893163718404b656ab267688724a87dc5c1"
assert references == {
    "gml_Object_objBox_n_Create_0": ["pop.v.i self.autoTile"],
    "gml_Object_objBox_n_Step_0": ["push.v self.autoTile"] * 4,
    "gml_Object_objBox_n_Step_2": ["push.v self.autoTile"],
}
create = (export / "Assembly/gml_Object_objBox_n_Create_0.asm").read_text()
assert "pushi.e 0\npop.v.i self.autoTile" in create
print("Pinned 2501 root bodies: box autoTile is assigned false only; no dynamic variable setter/eval calls")
print("Pinned live scrOutofview callers: actors in End Step, Scattered16 in ordinary Step")
