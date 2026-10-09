#!/usr/bin/env python3
"""Prepare exact binary-alpha title-font masks; never convert glyphs on Amiga."""
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image

VERSION = 1
SOURCE = "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"

def mask_bytes(alpha, width, height):
    if len(alpha) != width*height or any(a not in (0, 255) for a in alpha):
        raise ValueError("glyph is not exact binary alpha")
    stride = ((width+15)//16)*2
    out = bytearray(stride*height)
    for y in range(height):
        for x in range(width):
            if alpha[y*width+x]:
                out[y*stride+x//8] |= 0x80 >> (x%8)
    return bytes(out), stride

def generate(manifest_path, output, check):
    raw = manifest_path.read_bytes()
    manifest = json.loads(raw)
    if manifest['schemaVersion'] != 3 or manifest['source']['sha256'] != SOURCE:
        raise ValueError("unreviewed resource manifest")
    font = manifest['resources']['fonts'][2]
    if font['id'] != 2 or font['scaleX'] != 1 or font['scaleY'] != 1:
        raise ValueError("unexpected title font")
    item = manifest['resources']['texturePageItems'][font['texturePageItemId']]
    page_path = manifest_path.parent/'TexturePages'/f"{item['texturePageId']}.png"
    page = Image.open(page_path).convert('RGBA')
    lines = ['// Generated title font masks; do not commit.', '#include "core/FontGlyphMasks.h"',
             'namespace downwell::title_glyphs {']
    records, maximum, total, previous = [], 0, 0, -1
    for i, glyph in enumerate(font['glyphs']):
        c = glyph['character']
        if c <= previous:
            raise ValueError('glyphs must be strictly sorted')
        previous = c
        x, y = item['sourceX']+glyph['sourceX'], item['sourceY']+glyph['sourceY']
        w, h = glyph['sourceWidth'], glyph['sourceHeight']
        if min(x,y,w,h) < 0 or x+w > page.width or y+h > page.height:
            raise ValueError('glyph outside atlas')
        glyph_image = page.crop((x,y,x+w,y+h))
        rgba = glyph_image.tobytes()
        if any(rgba[n+3] and rgba[n:n+3] != b'\xff\xff\xff' for n in range(0,len(rgba),4)):
            raise ValueError('colored glyph needs more than a tinted alpha mask')
        alpha = glyph_image.getchannel('A').tobytes()
        mask, stride = mask_bytes(alpha,w,h)
        # Independently decode every packed bit, including row padding.
        for row in range(h):
            for col in range(stride*8):
                actual = bool(mask[row*stride+col//8] & (0x80>>(col%8)))
                if actual != (col<w and alpha[row*w+col]==255):
                    raise ValueError('planar mask round trip failed')
        total += len(mask); maximum = max(maximum,len(mask))
        lines.append(f'static const std::uint8_t mask{i}[] = {{'+','.join(str(b) for b in mask or b'\0')+'};')
        records.append(f'{{{c},{{mask{i},{len(mask)},{w},{h},{stride}}},{glyph["offset"]}}},')
    lines += [f'static const std::uint8_t zero[{max(1,maximum)}] = {{}};',
              'static const FontGlyphMask glyphs[] = {',*records,'};',
              'static const FontGlyphMasks bank(2,glyphs,sizeof(glyphs)/sizeof(glyphs[0]),zero,sizeof(zero));','}']
    payload = ('\n'.join(lines)+'\n').encode()
    evidence = {'schemaVersion':1,'tool':{'name':Path(__file__).name,'version':VERSION,
        'sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},
        'sourceSha256':SOURCE,'manifestSha256':hashlib.sha256(raw).hexdigest(),
        'texture':{'name':page_path.name,'sha256':hashlib.sha256(page_path.read_bytes()).hexdigest()},
        'fontId':2,'glyphCount':len(records),'maskBytes':total,'zeroBytes':maximum,
        'outputSha256':hashlib.sha256(payload).hexdigest()}
    metadata = (json.dumps(evidence,indent=2)+'\n').encode()
    if check:
        if output.read_bytes()!=payload or output.with_suffix('.json').read_bytes()!=metadata:
            raise ValueError('generated glyph bank or provenance differs')
    else:
        output.parent.mkdir(parents=True,exist_ok=True)
        output.write_bytes(payload);output.with_suffix('.json').write_bytes(metadata)
    print(f'Title font: {len(records)} glyphs, {total} mask bytes; exact atlas round trip passed')

if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path,default=root/'extracted/utmt-0.9.2.0/manifest.json')
    parser.add_argument('--output',type=Path,default=root/'generated/downwell-title-glyphs.h')
    parser.add_argument('--check',action='store_true')
    args = parser.parse_args()
    generate(args.manifest,args.output,args.check)
