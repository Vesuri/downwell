#!/usr/bin/env python3
"""Verify the prepared-health/embedded-HUD link-time provenance contract."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--assembler', required=True)
parser.add_argument('--linker', required=True)
args = parser.parse_args()
header = (root/'generated/amiga-health-art.h').read_text()
include = (root/'generated/amiga-health-source.inc').read_text()
symbol = 'downwell_health_source_' + hashlib.sha256(
    (root/'generated/amiga-hud.dwap').read_bytes()).hexdigest()
assert re.search(r'extern "C" const UBYTE '+symbol+r'\[\]', header)
assert include.startswith(f'.globl {symbol}\n{symbol}:\n')
assert include.endswith('.incbin "../generated/amiga-hud.dwap"\n')
with tempfile.TemporaryDirectory(prefix='health-link-', dir=root/'tmp') as work:
    work = Path(work)
    consumer = work/'consumer.s'
    consumer.write_text(f'.globl _start\n_start:\n.long {symbol}\n')
    provider = work/'provider.s'
    provider.write_text(include)
    for name in ('consumer', 'provider'):
        subprocess.run([args.assembler, '-mcpu=68000', '-o', str(work/(name+'.o')),
                        str(work/(name+'.s'))], cwd=root/'amiga', check=True)
    command = [args.linker, '-e', '_start', '-o', str(work/'matched.elf'),
               str(work/'consumer.o'), str(work/'provider.o')]
    subprocess.run(command, check=True, capture_output=True)
    # Retain identical bytes but change their identity: stale consumers must
    # fail even though object sizes and all ordinary resource symbols match.
    provider.write_text(include.replace(symbol, symbol+'_different'))
    subprocess.run([args.assembler, '-mcpu=68000', '-o', str(work/'provider.o'),
                    str(provider)], cwd=root/'amiga', check=True)
    mismatch = subprocess.run(command, capture_output=True, text=True)
    assert mismatch.returncode != 0 and symbol in mismatch.stderr, mismatch.stderr
print('Health provenance: matching payload links; mismatched identity rejected')
