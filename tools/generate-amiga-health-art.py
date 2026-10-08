#!/usr/bin/env python3
"""Generate health setup words and exact immutable-source admission bytes."""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'generated/amiga-health-art.h'
INPUTS = ['data/data.win', 'generated/amiga-hud.dwap', 'generated/amiga-hud.manifest.json',
          'tools/generate-amiga-health-art.cpp', 'tools/generate-amiga-health-art.py',
          'src/core/HudNumbers.h', 'src/platform/amiga/AmigaRenderer.c',
          'src/platform/amiga/AmigaRenderer.h']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    payload = subprocess.check_output([str(ROOT/'build/generate-amiga-health-art'),
                                       'generated/amiga-hud.dwap'], cwd=ROOT)
    pack = (ROOT/INPUTS[1]).read_bytes()
    source_symbol = 'downwell_health_source_' + hashlib.sha256(pack).hexdigest()
    source_include = (f'.globl {source_symbol}\n{source_symbol}:\n'
                      '.incbin "../generated/amiga-hud.dwap"\n').encode()
    magic, version, header, count, _ = struct.unpack_from('>4sHHI8s', pack)
    assert (magic, version, header) == (b'DWAP', 2, 20)
    wanted = {(415, 0), (415, 1), (378, 2)}
    spans = [(0, 20)]
    for i in range(count):
        offset = 20+i*24
        r = struct.unpack_from('>HHhhHHHHII', pack, offset)
        if r[:2] not in wanted:
            continue
        wanted.remove(r[:2])
        assert r[8]+r[9] <= len(pack)
        spans.extend([(offset, 24), (r[8], r[9])])
    assert not wanted
    lines = []
    for i, (offset, size) in enumerate(spans):
        lines.append(f'static const UBYTE bytes{i}[]={{' + ','.join(map(str, pack[offset:offset+size])) + '};')
    lines += ['struct Span { unsigned offset,size; const UBYTE* bytes; };',
              'static const Span spans[]={']
    lines += [f'{{{offset},{size},bytes{i}}},' for i, (offset, size) in enumerate(spans)]
    lines += ['};', f'static constexpr unsigned minimumBytes={max(a+b for a,b in spans)};']
    payload += ('\n'.join(lines)+'\n').encode()
    payload += (f'#ifdef DOWNWELL_PLATFORM_AMIGA\nextern "C" const UBYTE {source_symbol}[];\n'
                'inline bool linkedSource(const UBYTE* source,const UBYTE* end){\n'
                f'    return source=={source_symbol} && end==source+{len(pack)};\n'
                '}\n#endif\n').encode()
    manifest = {'format': 'amiga-health-art-v1', 'generator_version': 3,
                'extractor': json.loads((ROOT/INPUTS[2]).read_text())['extractor'],
                'resource_ids': [415, 378, 65011], 'per_heart': 4,
                'inputs_sha256': {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in INPUTS},
                'output_sha256': hashlib.sha256(payload).hexdigest(),
                'source_include_sha256': hashlib.sha256(source_include).hexdigest()}
    for path, data in [(OUTPUT, payload), (OUTPUT.with_suffix('.json'),
                       (json.dumps(manifest, indent=2)+'\n').encode()),
                       (OUTPUT.with_name('amiga-health-source.inc'), source_include)]:
        if args.check:
            if not path.exists() or path.read_bytes()!=data:
                raise SystemExit(f'stale health art: {path}')
        else:
            temp = path.with_suffix(path.suffix+'.tmp')
            temp.write_bytes(data)
            temp.replace(path)


if __name__ == '__main__':
    main()
