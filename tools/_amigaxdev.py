"""Find AmigaXDev's shared Python package (amigaxdev) under any python3 and check
the release helpers against manifest/release-helpers.sha256 before importing
them, as Prince of Persia 2's tools/_amigaxdev.py pins its Mac helpers."""
import hashlib
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PYLIB = Path(os.environ.get('AMIGA_PYLIB') or Path.home() / '.local/share/amiga/pylib')
for line in (ROOT / 'manifest/release-helpers.sha256').read_text().splitlines():
    digest, name = line.split('  ', 1)
    if hashlib.sha256((PYLIB / name).read_bytes()).hexdigest() != digest:
        raise RuntimeError('SHARED HELPER / SHA-256 MISMATCH: ' + name)
sys.path.insert(0, str(PYLIB))
