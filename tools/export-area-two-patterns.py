#!/usr/bin/env python3
"""Export auditable catacomb pattern literals from pinned UTMT VM assembly.

This is data recovery, not an area-two gameplay implementation. No GML is
evaluated and no original game file is modified.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile

SOURCE_SHA256 = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"
FAMILIES = (("Start", "partStart", 4), ("Level", "partMain", 29),
            ("End", "partEnd", 5), ("Left", "partLeft", 2),
            ("Right", "partRight", 7), ("Shop", "partShopR", 3))
STORE = re.compile(r'^push\.s "((?:\\.|[^"\\\n])*)"@(\d+)\n'
                   r'conv\.s\.v\npushi\.e -1\npush\.v self\.i\nconv\.v\.i\n'
                   r'pop\.v\.v \[array\]self\.(\w+)$', re.MULTILINE)
INCREMENT = "push.v self.i\npushi.e 1\nadd.i.v\npop.v.v self.i"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def decode_literal(encoded):
    escapes = {"r": "\r", "n": "\n", "t": "\t", "\\": "\\", '"': '"'}
    def replace(match):
        require(match[1] in escapes, f"unsupported VM string escape: {match[0]}")
        return escapes[match[1]]
    return re.sub(r"\\(.)", replace, encoded)


def parse_vm(source, array, expected_count):
    require(expected_count > 0, "expected a nonempty pinned pattern family")
    source = source.replace("\r\n", "\n")
    matches = list(STORE.finditer(source))
    require(len(matches) == expected_count, f"{array}: unexpected literal assignment count")
    require(len(re.findall(r'^push\.s ', source, re.MULTILINE)) == len(matches),
            f"{array}: unrecognized string operation")
    require(re.search(r'pushi\.e 0\npop\.v\.i self\.i\n(?:push\.i \d+\nsetowner\.e\n)?$',
                      source[:matches[0].start()]), f"{array}: index does not start at zero")
    result = []
    for index, match in enumerate(matches):
        require(match[3] == array, f"{array}: unexpected array store")
        if index:
            require(source[matches[index-1].end():match.start()].strip() == INCREMENT,
                    f"{array}: nonsequential or computed pattern index")
        text = decode_literal(match[1])
        rows = text.splitlines()
        while rows and not rows[0].strip():
            rows.pop(0)
        while rows and not rows[-1].strip():
            rows.pop()
        rows = [row[1:] if row.startswith("\t") else row for row in rows]
        require(rows and all(row and all(0x20 <= ord(c) <= 0x7e for c in row) for row in rows),
                f"{array}: empty or non-ASCII normalized row")
        result.append({"sourceIndex": index, "stringId": int(match[2]),
                       "text": text, "rows": rows, "rowWidths": [len(row) for row in rows]})
    maximum = f"push.v self.i\npushi.e -1\npushi.e 100\npop.v.v [array]self.{array}"
    require(source[matches[-1].end():].lstrip().startswith(maximum + "\n"),
            f"{array}: missing source maximum-index assignment")
    require(len(re.findall(r'pop\.v\.v \[array\]self\.', source)) == len(matches) + 1,
            f"{array}: unexpected extra array mutation")
    return result


def build(root):
    export = root / "extracted/utmt-0.9.2.0"
    manifest_bytes = (export / "manifest.json").read_bytes()
    manifest = json.loads(manifest_bytes)
    require(manifest["source"]["sha256"] == SOURCE_SHA256, "unexpected Windows input hash")
    require(manifest["schemaVersion"] == 3 and manifest["gameMaker"]["bytecodeVersion"] == 17,
            "unexpected manifest/bytecode format")
    require(manifest["extractor"]["version"] == "0.9.2.0", "unexpected extractor version")
    families, inputs = [], []
    for script, array, count in FAMILIES:
        name = f"gml_GlobalScript_scr{script}Pat2.asm"
        definitions = [s for s in manifest["resources"]["scripts"] if s["name"] == f"scr{script}Pat2"]
        require(len(definitions) == 1 and definitions[0]["codeName"] == name[:-4],
                f"{array}: script/code identity differs from the manifest")
        definition = definitions[0]
        data = (export / "Assembly" / name).read_bytes()
        patterns = parse_vm(data.decode("utf-8"), array, count)
        inputs.append({"name": name, "sha256": hashlib.sha256(data).hexdigest()})
        families.append({"array": array, "script": name,
                         "scriptId": definition["id"], "codeId": definition["codeId"], "maximumIndex": count-1,
                         "patterns": patterns})
    return {"schemaVersion": 1, "area": 2, "source": manifest["source"],
            "extractor": manifest["extractor"], "bytecodeVersion": 17,
            "exporter": {"name": Path(__file__).name, "version": 1,
                         "sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
            "manifestSha256": hashlib.sha256(manifest_bytes).hexdigest(),
            "inputs": inputs, "families": families}


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=root / "generated/downwell-area-two-patterns.json")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    require(not args.output.resolve().is_relative_to((root / "data").resolve()), "data/ is read-only")
    result = build(root)
    payload = (json.dumps(result, indent=2, ensure_ascii=True) + "\n").encode("utf-8")
    if args.check:
        require(args.output.read_bytes() == payload, "area-two export differs from VM inputs")
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        descriptor, temporary = tempfile.mkstemp(dir=args.output.parent, prefix=args.output.name + ".")
        try:
            with os.fdopen(descriptor, "wb") as stream:
                stream.write(payload)
            os.replace(temporary, args.output)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)
    patterns = [p for family in result["families"] for p in family["patterns"]]
    print(f"Area two: {len(patterns)} patterns, {sum(len(p['rows']) for p in patterns)} rows, "
          f"{sum(sum(p['rowWidths']) for p in patterns)} cells; sha256 {hashlib.sha256(payload).hexdigest()}")
    for family in result["families"]:
        print(f"{family['array']}: {len(family['patterns'])} patterns; widths "
              f"{sorted({w for p in family['patterns'] for w in p['rowWidths']})}")


if __name__ == "__main__":
    main()
