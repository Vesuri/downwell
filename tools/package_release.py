#!/usr/bin/env python3
"""Create the deterministic Amiga LHA install archive (LH5, level-0 headers).
From Wing Commander 2's tools/package_release.py, on AmigaXDev's amigaxdev.release.

    python3 tools/package_release.py [OUTPUT_DIRECTORY]   (default build/release)

  Downwell-<VERSION>.lha: "Downwell Install.info" and "Downwell Install/" with
  Install, ReadMe, their icons, the program and its tool icon template
  (Downwell.inf), DownwellInstallData, Downwell.fields, Downwell.columns (our own
  computation) and the converter's licences.

The archive holds only our files. Every other data file the game reads is built
from the player's own Steam copy by DownwellInstallData, so none is packaged:
a member whose SHA-256 is one of the player's files (install.c's INPUTS) or of a
data file derived from them (the installed names and their generated/ inputs)
is refused, and so is any member named like an installed data file.
The program must be the stripped shipping build (`make package-release`).
"""
import argparse
import hashlib
import re
import sys
from pathlib import Path
import _amigaxdev  # noqa: F401  (checks and finds the shared helpers)
from amigaxdev import release

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools/install-data'))
from test_install import FILES, SECTIONS  # noqa: E402  (the converter's installed names)

PREFIX = 'Downwell Install'
STAMP = release.dos_stamp(2026, 11, 26)
ICONS = ROOT / 'release/icons'
EXE = ROOT / 'amiga/out/Downwell.exe'
CONVERTER = ROOT / 'build/install-data/DownwellInstallData.exe'
# Our own computation (amiga/Makefile), installed by the Installer into data/.
OWN_DATA = ('fields', 'columns')
WBTOOL = 3
DO_TYPE = 48  # struct DiskObject: magic, version, struct Gadget (44 bytes), do_Type


def player_hashes():
    """The SHA-256 of each of the player's files, from install.c's INPUTS."""
    source = (ROOT / 'tools/install-data/install.c').read_text()
    hashes = set(re.findall(r'\{"[^"]+", "([0-9a-f]{64})"\}', source))
    assert len(hashes) == 12, 'install.c INPUTS'
    return hashes


def derived_paths():
    """Every derived data file this checkout holds: the development build's
    installed-name copies, the generated/ inputs and the converter's outputs."""
    paths = [ROOT / 'amiga/out' / f'Downwell.{k}' for k in FILES]
    paths += [ROOT / 'generated' / v for v in list(FILES.values()) + list(SECTIONS.values())]
    return [p for p in paths if p.is_file()]


def original_hashes():
    return player_hashes() | {hashlib.sha256(p.read_bytes()).hexdigest() for p in derived_paths()}


def tool_icon():
    """The game template icon (game.inf.b64, a project icon) as a tool icon: only
    do_Type changes. It has no default tool and no tool types; Installer sets its
    stack (8 KB) when it copies it to Downwell.info."""
    data = bytearray(release.installer_icon(ICONS, 'Downwell', game=True))
    assert data[:4] == b'\xe3\x10\0\1' and data[DO_TYPE] == 4, 'game.inf.b64 is not a project icon'
    assert data[50:58] == bytes(8), 'game.inf.b64 has a default tool or tool types'
    data[DO_TYPE] = WBTOOL
    return bytes(data)


def shipping_executable(path):
    data = path.read_bytes()
    options = path.with_suffix('.hunk-options')
    if path == EXE and (not options.exists() or options.read_text().strip() != '1'):
        raise SystemExit(f'{path} is not stripped: build the shipping program with make package-release')
    return data


def members(executable=EXE):
    out = ROOT / 'amiga/out'
    files = {
        'Downwell': shipping_executable(executable),
        'Downwell.inf': tool_icon(),
        'DownwellInstallData': CONVERTER.read_bytes(),
        'Install': (ROOT / 'release/Install').read_bytes(),
        'Install.info': release.installer_icon(ICONS, 'Downwell', require=(b'MINUSER=AVERAGE\0',)),
        'ReadMe': (ROOT / 'release/ReadMe').read_bytes(),
        'ReadMe.info': release.template_icon(ICONS, 'readme.info.b64'),
        'LICENSE.LGPL.txt': (ROOT / 'tools/install-data/COPYING.LIB').read_bytes(),
        'LICENSE.Tremor.txt': (ROOT / 'tools/install-data/tremor/COPYING').read_bytes(),
        'LICENSE.libogg.txt': (ROOT / 'tools/install-data/ogg/COPYING').read_bytes(),
    }
    for k in OWN_DATA:
        files[f'Downwell.{k}'] = (out / f'Downwell.{k}').read_bytes()
    return files


def refuse_derived(files):
    """No member may be one of the player's files or a file derived from them."""
    installed = {f'Downwell.{k}'.lower() for k in FILES}
    for name in files:
        if name.lower() in installed or name.lower().endswith(('.win', '.ogg', '.dat', '.ini')):
            raise SystemExit('refusing to package an installed data file: ' + name)
    release.refuse_originals(files, original_hashes())


def archive_name(version):
    return f'Downwell-{version}.lha'


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('output_directory', type=Path, nargs='?', default=ROOT / 'build/release')
    args = parser.parse_args()
    version = (ROOT / 'VERSION').read_text().strip()
    files = members()
    refuse_derived(files)
    archive = release.write(args.output_directory / archive_name(version),
                            release.archive(PREFIX, files, release.template_icon(ICONS, 'drawer.info.b64'), STAMP))
    print(f'{archive}: {len(files)} drawer contents plus drawer icon, {archive.stat().st_size} bytes, '
          f'SHA-256 {hashlib.sha256(archive.read_bytes()).hexdigest()}')


if __name__ == '__main__':
    main()
