#!/usr/bin/env python3
"""Audit the install archive: its members, their checksums and their origin.
From Wing Commander 2's tools/check_release.py.

    python3 tools/check_release.py [DIRECTORY]   (default build/release)

Downwell-<VERSION>.lha (package_release.archive_name) is walked header by
header (level-zero LH5, the release date, the drawer prefix) and every member
is decoded independently with Lhasa (`lha pq`): each must equal its build
output (package_release.members), with its CRC. The archive must hold exactly
those members, so only our files: no member may be one of the player's files
or a data file derived from them (package_release.original_hashes), be named
like an installed data file, or hold any 64-byte block of the player's files,
forward or byte-reversed (tools/check-original-bytes.py). The extracted tree's
names are checked with amigaxdev.release.check_extracted_names. Then the
program and converter are HUNK executables, the icons have their types (the
program's a tool icon), Install.info names Downwell, Install and ReadMe carry
this version, and Install's list of converted files is the converter's.
"""
import argparse
import hashlib
import importlib.util
import re
import struct
import subprocess
import tempfile
from pathlib import Path
from package_release import (FILES, OWN_DATA, PREFIX, ROOT, STAMP, archive_name, members,
                             original_hashes, release)

DATE = '26.11.2026'
HUNK = b'\0\0\3\xf3'
ICONS = {'Downwell.inf': 3, 'Install.info': 4, 'ReadMe.info': 4, '@drawer': 2}


def original_bytes():
    spec = importlib.util.spec_from_file_location('check_original_bytes', ROOT / 'tools/check-original-bytes.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    blocks = module.original_blocks(ROOT / 'data')
    assert len(blocks) > 100000, "the player's files are missing (data/)"
    return module, blocks


def read_members(archive):
    raw = archive.read_bytes()
    pos, payloads = 0, {}
    while pos < len(raw) and raw[pos]:
        size = raw[pos]
        header = raw[pos + 2:pos + 2 + size]
        assert len(header) == size and sum(header) & 255 == raw[pos + 1], 'bad header checksum'
        assert header[:5] in (b'-lh5-', b'-lh0-') and header[18] == 0, 'expected level-zero LH5'
        packed, unpacked = struct.unpack_from('<II', header, 5)
        assert struct.unpack_from('<I', header, 13)[0] == STAMP, 'wrong release date'
        n = header[19]
        assert size == 22 + n
        stored = header[20:20 + n].decode('ascii').replace('\\', '/')
        if stored == PREFIX + '.info':
            name = '@drawer'
        else:
            assert stored.startswith(PREFIX + '/'), 'wrong installation drawer: ' + stored
            name = stored[len(PREFIX) + 1:]
        assert name not in payloads, 'duplicate member: ' + name
        pos += size + 2
        assert len(raw[pos:pos + packed]) == packed, 'truncated payload'
        data = subprocess.run(['lha', 'pq', str(archive.resolve()), stored], check=True, capture_output=True).stdout
        assert len(data) == unpacked and release.crc16(data) == struct.unpack_from('<H', header, 20 + n)[0], \
            'bad payload CRC: ' + name
        payloads[name] = data
        pos += packed
    assert raw[pos:] == b'\0', 'trailing bytes after the archive end'
    return payloads


def check_archive(archive, expected):
    payloads = read_members(archive)
    drawer = payloads.pop('@drawer')
    assert set(payloads) == set(expected), \
        f'wrong archive contents: missing {sorted(set(expected) - set(payloads))}, ' \
        f'unexpected {sorted(set(payloads) - set(expected))}'
    for name, data in expected.items():
        assert payloads[name] == data, 'member differs from its build output: ' + name
    payloads['@drawer'] = drawer
    # Only our files: none of the player's, nothing derived from them.
    hashes = original_hashes()
    installed = {f'Downwell.{k}'.lower() for k in FILES}
    module, blocks = original_bytes()
    for name, data in payloads.items():
        assert name.lower() not in installed, 'installed data file in the archive: ' + name
        assert hashlib.sha256(data).hexdigest() not in hashes, "the player's file or derived data: " + name
        copy = module.first_copy(data, blocks)
        assert not copy, f"{name}: offset {copy and copy[0]} holds a block of the player's {copy and copy[1][0]}"
    with tempfile.TemporaryDirectory(prefix='downwell-release-', dir=ROOT / 'tmp') as tmp:
        subprocess.run(['lha', 'xq', str(archive.resolve())], cwd=tmp, check=True)
        release.check_extracted_names(tmp, [PREFIX + '.info'] + [PREFIX + '/' + n for n in expected])
        for name, data in expected.items():
            assert (Path(tmp) / PREFIX / name).read_bytes() == data, 'extracted member differs: ' + name
    for name in ('Downwell', 'DownwellInstallData'):
        assert payloads[name][:4] == HUNK, 'not an Amiga HUNK executable: ' + name
    for name, kind in ICONS.items():
        assert payloads[name][:4] == b'\xe3\x10\0\1' and payloads[name][48] == kind, 'bad icon: ' + name
    assert payloads['Downwell.inf'][50:58] == bytes(8), 'the program icon has a default tool or tool types'
    assert b'APPNAME=Downwell\0' in payloads['Install.info']
    assert b'Installer\0' in payloads['Install.info'], 'Install.info default tool'
    version = (ROOT / 'VERSION').read_text().strip().encode()
    script = payloads['Install']
    assert b'$VER: Install ' + version + b' (' + DATE.encode() + b')' in script, 'Install version'
    assert b'version ' + version + b' (' + DATE.encode() + b'):' in payloads['ReadMe'], 'ReadMe version'
    for heading in (b'What to expect:', b'Requirements:', b'Installation:', b'Playing:', b'History:', b'Credits:',
                    b'app 360740', b'version 1.0.5'):
        assert heading in payloads['ReadMe'], heading
    assert b'TODO' not in payloads['ReadMe'], 'ReadMe placeholder left'
    # Install copies only members, and checks exactly the converter's files.
    used = set(re.findall(rb'\(tackon #source "([^"]+)"\)', script))
    assert used <= {n.encode() for n in expected}, sorted(used - {n.encode() for n in expected})
    for k in OWN_DATA:
        assert f'"Downwell.{k}"'.encode() in script, f'Install does not install Downwell.{k}'
    select = re.search(rb'\(select #n ((?:\s*"[^"]+")+)\)', script)
    listed = re.findall(rb'"([^"]+)"', select[1])
    assert sorted(listed) == sorted(f'Downwell.{k}'.encode() for k in FILES), "Install's converted-file list"
    assert f'(< #n {len(FILES)})'.encode() in script, "Install's converted-file count"
    assert b'(setstack 8192)' in script, 'the program icon stack'
    print(f'PASS: {archive.name}: {len(payloads)} allowlisted LH5 members; independent decompression, CRCs, '
          "build identity, none of the player's files, derived data or blocks, names, versions and icons; "
          f'{archive.stat().st_size} bytes, SHA-256 {hashlib.sha256(archive.read_bytes()).hexdigest()}')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('dist', type=Path, nargs='?', default=ROOT / 'build/release')
    args = parser.parse_args()
    version = (ROOT / 'VERSION').read_text().strip()
    (ROOT / 'tmp').mkdir(exist_ok=True)
    check_archive(args.dist / archive_name(version), members())


if __name__ == '__main__':
    main()
