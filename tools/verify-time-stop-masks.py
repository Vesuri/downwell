#!/usr/bin/env python3
"""Verify original player/shard masks before specializing field queries."""
import base64
import hashlib
import json
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
digest = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
assert hashlib.sha256((root / "data/data.win").read_bytes()).hexdigest() == digest
record = json.loads((root / "generated/time-stop-masks.json").read_text())
assert record["sourceSha256"] == digest
assert (record["extractor"],record["version"],record["commit"]) == (
    "UndertaleModTool","0.9.2.0","03c7048438347799e0717b6e57a9358d000472e0")
export=root/"extracted/utmt-0.9.2.0"
metadata=json.loads((export/"manifest.json").read_text())
assert metadata["source"]["sha256"]==digest
obj=metadata["resources"]["objects"][82]
sprite=metadata["resources"]["sprites"][597]
assert (obj["name"],obj["spriteId"],obj["textureMaskId"])==("parentTimeField",597,-1)
assert (sprite["name"],sprite["originX"],sprite["originY"],sprite["separateMasks"])==("maskCircle24",24,24,"Precise")
for event,sha in (("Create","5b10efdb1390d9b54a0e809265363a67e478369828c9352f0aac70d25a5cdce4"),
                  ("Draw","4a70e5ea73dc206e9977e425d4a34e05b41c0f0dae6d0f6c0839b830b44b81c4")):
    assert hashlib.sha256((export/f"Assembly/gml_Object_parentTimeField_{event}_0.asm").read_bytes()).hexdigest()==sha
expected = (
    (31,"sprPlayerIdle",8,16,8,20,"76c9c0411bcc38717855ce9e8699c829324464855edbc72d774bd32088626979"),
    (91,"sprPoisonPotPieces",8,16,10,13,"d3957e5f4643a448c15473e55942c4e56554fbefb99deb99806177e98ba0010e"),
)
assert len(record["sprites"]) == len(expected)+1
for sprite, (id,name,left,right,top,bottom,mask_hash) in zip(record["sprites"],expected):
    assert (sprite["id"],sprite["name"]) == (id,name)
    assert len(sprite["masks"]) == 1
    mask = sprite["masks"][0]
    assert mask["width"] == mask["height"] == 24
    data = base64.b64decode(mask["data"],validate=True)
    assert len(data) == 72 and hashlib.sha256(data).hexdigest() == mask_hash
    for y in range(24):
        for x in range(24):
            assert bool(data[y*3+x//8] & (128>>(x%8))) == (left<=x<right and top<=y<bottom)
field = record["sprites"][2]
assert (field["id"],field["name"]) == (597,"maskCircle24")
assert len(field["masks"]) == 1
mask=field["masks"][0]
assert mask["width"] == mask["height"] == 48
raw=base64.b64decode(mask["data"],validate=True)
assert len(raw)==288
assert hashlib.sha256(raw).hexdigest()=="f323187eb5a287f069835dea91b91f940fba459f73a6f2242c44c9073ec43772"
rows=[]
for y in range(48):
    pixels=[x for x in range(48) if raw[y*6+x//8] & (128>>(x%8))]
    left,right=(pixels[0],pixels[-1]+1) if pixels else (0,0)
    assert pixels==list(range(left,right))
    rows.append((left,right))
if "--generate" in sys.argv:
    header=("// Generated sprite 597 collision mask; do not commit.\n"
            f"// data.win SHA256: {digest}\n"
            "// UndertaleModTool 0.9.2.0, 03c7048438347799e0717b6e57a9358d000472e0\n"
            "#ifndef DOWNWELL_GENERATED_TIME_FIELD_MASK_H\n#define DOWNWELL_GENERATED_TIME_FIELD_MASK_H\n"
            "namespace downwell {\nstatic constexpr unsigned char timeFieldMaskRows[48][2] = {\n"+
            "\n".join(f"    {{{a},{b}}}," for a,b in rows)+"\n};\n}\n#endif\n")
    (root/"generated/downwell-time-field-mask.h").write_text(header)
    (root/"generated/downwell-time-field-mask.mask").write_bytes(raw)
    manifest={key:record[key] for key in ("sourceSha256","extractor","version","commit")}
    manifest.update(spriteId=597,rawMaskSha256=hashlib.sha256(raw).hexdigest(),
                    outputSha256=hashlib.sha256(header.encode()).hexdigest(),rows=48)
    (root/"generated/downwell-time-field-mask.json").write_text(json.dumps(manifest,indent=2)+"\n")
print("Verified 1152 player/shard pixels and all 2304 ordinary time-field pixels")
