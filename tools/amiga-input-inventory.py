#!/usr/bin/env python3
"""WI.2: classify every generated/ input of the shipping Amiga build.

The shipping set is read from the Amiga build itself, never listed by hand:
- the headers each shipping object really included (amiga/obj/*.d, written
  by -MMD; run after a normal `make -C amiga`, not a verify build);
- the .incbin/.include lines of the shipping assembly objects, under the
  shipping ASFLAGS (so the reference-only .ifdef blocks drop out);
- the data files `all` copies beside the executable (out/Downwell.*);
  Downwell.tables counts as each of the inputs it packs.

Each input is then classified:
- own: our computation; no byte of the player's files reaches it. Stays linked.
- layout: sizes, budgets or offsets of converted files, no game bytes. May
  stay linked while the converter reproduces those files byte for byte.
- derived: built from the player's files (data.win chunks, language.ini or
  the runner Downwell.exe). WI.3 moves these out of the executable and WI.4's
  converter builds them on the Amiga.

Writes docs/amiga-input-inventory.md. --check fails if the build's input set
and the table differ, or if the committed document is stale.
"""
import argparse
import re
import runpy
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'docs/amiga-input-inventory.md'

# UndertaleModTool 0.9.2.0 export (tools/extract.sh): its manifest and
# texture pages, CodeEntries and Assembly views of data.win.
UTMT = 'UTMT export'
AUDIO = 'tools/extract-audio.py'
SFX = f'{AUDIO} -> tools/sfx-bank.py (ffmpeg swr resample) -> tools/pack-amiga-sfx.py'
HUD = f'{UTMT} -> tools/generate-amiga-hud.py'
GRAPHICS = 'SPRT, TPAG, TXTR (texture page PNGs)'

# name: (class, sources, how it is made, what it holds)
TABLE = {
    # --- own computation
    'atan-q16.inc': ('own', '-', 'tools/generate-atan-table.py', 'atan samples from a Decimal series'),
    'clear-motion.inc': ('own', '-', 'tools/generate-clear-motion.cpp (LevelClearMotion.h)',
                         'Result-room easing states computed by our model'),
    'downwell-field-spans.h': ('own', '-', 'tools/generate-field-spans.cpp', 'goal/time field outline spans'),
    'downwell-field-columns.h': ('own', '-', 'tools/generate-field-columns.cpp', 'field column commands'),
    'amiga-field-columns.bin': ('own', '-', 'tools/generate-field-columns.cpp', 'field column commands (data file)'),
    'amiga-field-bands.bin': ('own', '-', 'tools/generate-field-columns.cpp', 'field arc bands (data file)'),
    'amiga-field-images.bin': ('own', '-', 'tools/generate-field-images.cpp', 'field images from our raster'),
    # --- layout: sizes and budgets of converted files
    'amiga-scene-banks.h': ('layout', '-', 'tools/generate-amiga-hud.py', 'scene bank sizes and budgets'),
    'amiga-transient-banks.h': ('layout', '-', 'tools/generate-amiga-transient-banks.py',
                                'transient bank sizes and budgets'),
    'amiga-sfx.h': ('layout', '-', 'tools/pack-amiga-sfx.py', 'sound bank sizes and counts'),
    'amiga-cavern-planes.h': ('layout', '-', 'tools/generate-amiga-plane-bank.py', 'cavern plane bank layout'),
    'amiga-tables.h': ('layout', '-', 'tools/pack-amiga-tables.py', 'Downwell.tables size and tags'),
    # --- language.ini
    'downwell-area-titles.h': ('derived', 'language.ini', 'tools/generate-area-titles.py', 'area title strings'),
    'downwell-rising-texts.h': ('derived', 'language.ini', 'tools/generate-area-titles.py', 'rising texts'),
    'downwell-shop-speech.h': ('derived', 'language.ini', 'tools/generate-shop-text.py', 'merchant speech'),
    'downwell-menu-text.h': ('derived', 'language.ini', 'tools/generate-menu-text.py', 'menu strings'),
    # --- Downwell.exe
    'smoke-trig.inc': ('derived', 'Downwell.exe (runner sin/cos path and its 64-entry table)',
                       'tools/export-smoke-trig.py (capstone)', 'runner trig program'),
    'contact-motion.inc': ('derived', 'Downwell.exe (through smoke-trig.inc)',
                           'tools/generate-contact-motion.cpp', 'contact motion decoder states'),
    # --- data.win: rooms, objects, code
    'downwell-runtime-rooms.dwrm': ('derived', 'data.win ROOM, OBJT', f'{UTMT} -> tools/generate-room-pack.py',
                                    'room instances'),
    'amiga-room-terrain-plan.inc': ('derived', 'data.win ROOM', f'{UTMT} -> tools/generate-room-terrain-plan.py',
                                    'room terrain plan'),
    'downwell-objects.dwob': ('derived', 'data.win OBJT', 'tools/generate-object-pack.py', 'object table'),
    'downwell-builder-patterns.dwbp': ('derived', 'data.win CODE',
                                       f'{UTMT} CodeEntries -> tools/generate-builder-pattern-pack.py',
                                       'level-builder patterns (data file)'),
    'downwell-palettes.h': ('derived', 'data.win SHDR, CODE', f'{UTMT} Assembly -> tools/generate-palettes.py',
                            'palette shader colours'),
    # --- data.win: sprites and masks
    'downwell-sprites.dwsc': ('derived', 'data.win SPRT, TPAG', f'{UTMT} -> tools/generate-sprite-pack.py',
                              'sprite collision boxes'),
    'downwell-encounter-masks.dwsm': ('derived', 'data.win SPRT masks, OBJT',
                                      'tools/export-collision-masks.csx -> tools/generate-collision-masks.py '
                                      '-> tools/generate-encounter-masks.py', 'encounter masks (data file)'),
    'downwell-goal-mask.h': ('derived', 'data.win SPRT masks', 'tools/export-goal-mask.csx -> '
                             'tools/generate-goal-mask.py', 'goal field mask'),
    'downwell-time-field-mask.h': ('derived', 'data.win SPRT masks', 'tools/export-time-stop-masks.csx -> '
                                   'tools/verify-time-stop-masks.py', 'time-stop field mask'),
    # --- data.win: graphics banks (generate-amiga-hud.py profiles)
    'amiga-boot.dwap': ('derived', f'data.win {GRAPHICS}', HUD, 'boot bank (data file)'),
    'amiga-front.dwap': ('derived', f'data.win {GRAPHICS}', HUD, 'front-end bank (data file)'),
    'amiga-game.dwap': ('derived', f'data.win {GRAPHICS}, FONT; language.ini', HUD, 'gameplay bank (data file)'),
    'amiga-cavern.dwap': ('derived', f'data.win {GRAPHICS}', f'{HUD} (from amiga-game.dwap)',
                          'cavern bank (data file)'),
    'amiga-results.dwap': ('derived', f'data.win {GRAPHICS}, FONT; language.ini (labels written in the tool)', HUD, 'results bank (data file)'),
    'amiga-shop.dwap': ('derived', f'data.win {GRAPHICS}, FONT; language.ini', HUD, 'shop bank (data file)'),
    'amiga-pause.dwap': ('derived', f'data.win {GRAPHICS}, FONT; language.ini (labels written in the tool)', HUD, 'pause bank (data file)'),
    'amiga-clear.dwap': ('derived', f'data.win {GRAPHICS}, FONT; language.ini', HUD, 'level-clear bank (data file)'),
    'amiga-hud.dwap': ('derived', f'data.win {GRAPHICS}', HUD, 'HUD bank (data file)'),
    'amiga-game.exit-mask': ('derived', f'data.win {GRAPHICS}', HUD, 'exit wipe mask (data file)'),
    'amiga-game.entry-mask': ('derived', f'data.win {GRAPHICS}', HUD, 'entry wipe mask (data file)'),
    'amiga-game.selection-mask': ('derived', f'data.win {GRAPHICS}', HUD, 'selection mask (data file)'),
    'amiga-game.planes': ('derived', f'data.win {GRAPHICS}', 'tools/generate-amiga-plane-bank.py '
                          '(from amiga-game.dwap, amiga-cavern.dwap)', 'gameplay plane bank (data file)'),
    'amiga-cavern.planes': ('derived', f'data.win {GRAPHICS}', 'tools/generate-amiga-plane-bank.py',
                            'cavern plane bank (data file)'),
    # --- data.win: tables derived from the banks
    'amiga-effect-stamps.dwef': ('derived', f'data.win {GRAPHICS}', 'tools/generate-effect-stamps.py '
                                 '(from amiga-game.dwap)', 'effect stamps'),
    'amiga-terrain-phase6.bin': ('derived', f'data.win {GRAPHICS}', 'tools/generate-terrain-phase.py '
                                 '(from amiga-game.dwap)', 'shifted terrain tiles'),
    'amiga-initial-terrain.bin': ('derived', f'data.win {GRAPHICS}, ROOM', 'tools/generate-initial-terrain.py '
                                  '(from amiga-game.dwap, downwell-rooms.dwrm)', 'first-field terrain'),
    'amiga-pattern-terrain.bin': ('derived', f'data.win {GRAPHICS}, CODE', 'tools/generate-pattern-terrain.py '
                                  '(from amiga-game.dwap, the builder patterns)', 'pattern wall rows (data file)'),
    'amiga-ammo-dividers.h': ('derived', f'data.win {GRAPHICS}', 'tools/generate-ammo-dividers.py '
                              '(from amiga-hud.dwap)', 'ammo bar dividers'),
    'amiga-ammo-factored.h': ('derived', f'data.win {GRAPHICS}', 'tools/probe-ammo-complete.py '
                              '(from amiga-hud.dwap)', 'factored ammo bar art'),
    'amiga-health-art.h': ('derived', f'data.win {GRAPHICS}', 'tools/generate-amiga-health-art.py '
                           '(from amiga-hud.dwap)', 'health bar art'),
    'amiga-hud-text.h': ('derived', f'data.win {GRAPHICS}, FONT', 'tools/generate-amiga-hud-text.py '
                         '(from amiga-hud.dwap, amiga-game.dwap)', 'HUD glyph words'),
    'amiga-damage-text.h': ('derived', f'data.win {GRAPHICS}, FONT', 'tools/generate-amiga-damage-text.py '
                            '(from amiga-game.dwap)', 'damage number glyphs'),
    # --- data.win: sound
    'sound-durations.inc': ('derived', 'data.win SOND, AUDO; audiogroup1-6.dat, amb1.ogg, sfxfalling.ogg, sfxbossfirsthit.ogg (lengths only)', f'{AUDIO} -> tools/sound-durations.py',
                            'sound lengths in steps'),
    'sound-gains.inc': ('derived', 'data.win SOND', f'{AUDIO} -> tools/sound-durations.py', 'sound volumes'),
    'amiga-sfx-run.bin': ('derived', 'data.win SOND, AUDO (WAV, Ogg); sfxfalling.ogg', SFX, 'run sound bank (data file)'),
    'amiga-sfx-front.bin': ('derived', 'data.win SOND, AUDO (WAV, Ogg)', SFX, 'front-end sound bank (data file)'),
    'amiga-sfx-results.bin': ('derived', 'data.win SOND, AUDO (WAV)', SFX, 'results sound bank (data file)'),
    'amiga-sfx-clear.bin': ('derived', 'data.win SOND, AUDO (WAV)', SFX, 'level-clear sound bank (data file)'),
}


def make_database():
    command = '. amiga/env.sh >/dev/null && make -C amiga -pq FORCE_HUNK_OPTIONS'
    result = subprocess.run(['bash', '-c', command], cwd=ROOT, capture_output=True, text=True)
    return result.stdout


def variable(db, name):
    match = re.search(rf'^{name} :?= (.*)$', db, re.M)
    if not match:
        sys.exit(f'amiga/Makefile does not define {name}')
    return match.group(1).split()


def rule(db, target):
    """The prerequisites and recipe lines of one explicit rule."""
    lines = db.splitlines()
    for i, line in enumerate(lines):
        if line.startswith(target + ':') and not line.startswith(target + '::'):
            prereqs = line[len(target) + 1:].split('|')[0].split()
            recipe = []
            for follow in lines[i + 1:]:
                if follow.startswith('#'):
                    continue
                if not follow.startswith('\t'):
                    break
                recipe.append(follow.strip())
            return prereqs, recipe
    sys.exit(f'amiga/Makefile has no rule for {target}')


def generated_name(path):
    match = re.search(r'generated/([^ ]+)$', path)
    return match.group(1) if match else None


def assembly_inputs(source, defsyms):
    """.incbin/.include of one assembly file, honouring .ifdef/.endif."""
    found, stack = set(), []
    for line in (ROOT / 'amiga' / source).read_text().splitlines():
        words = line.split()
        if not words:
            continue
        if words[0] == '.ifdef':
            stack.append(words[1] in defsyms)
        elif words[0] == '.ifndef':
            stack.append(words[1] not in defsyms)
        elif words[0] == '.else':
            stack[-1] = not stack[-1]
        elif words[0] == '.endif':
            stack.pop()
        elif words[0] in ('.incbin', '.include') and all(stack):
            name = generated_name(words[1].strip('"'))
            if name:
                found.add(name)
    return found


def installed_layout(linked):
    """generated/installed/ headers: the bound pointers and counts only."""
    for name in linked:
        if name.startswith('installed/'):
            header = name[len('installed/'):]
            if header == 'smoke-trig.inc':
                # Code, not a table: the runner's trig operations in order,
                # transcribed; its 64-entry table and constants are bound.
                TABLE[name] = ('derived', 'Downwell.exe (the trig path\'s operation order; no constants)',
                               'tools/install-table-headers.py (from smoke-trig.inc)', 'trig program code')
                continue
            holds = 'binds every installed table' if header == 'bind-tables.h' else \
                f'pointers and counts for {header}'
            TABLE[name] = ('layout', '-', 'tools/install-table-headers.py', holds)


def shipping_inputs():
    db = make_database()
    objects = variable(db, 'OBJECTS')
    asflags = ' '.join(variable(db, 'ASFLAGS'))
    defsyms = set(re.findall(r'--defsym (\w+)=', asflags))
    linked, missing = set(), []
    for obj in objects:
        prereqs, _ = rule(db, obj)
        source = next(p for p in prereqs if p.endswith(('.s', '.c', '.cpp')))
        if source.endswith('.s'):
            linked |= assembly_inputs(source, defsyms)
            continue
        depfile = ROOT / 'amiga' / obj.replace('.o', '.d')
        if not depfile.exists():
            missing.append(str(depfile.relative_to(ROOT)))
            continue
        text = depfile.read_text().replace('\\\n', ' ')
        target = text.split(':', 1)[0].strip()
        for word in text.split('\n', 1)[0].split(':', 1)[1].split():
            name = generated_name(word)
            if name:
                linked.add(name)
        if target != obj:
            missing.append(f'{depfile.relative_to(ROOT)} (target {target})')
    if missing:
        sys.exit('dependency files missing; build with make -C amiga first: ' + ', '.join(missing))
    files = {}
    all_prereqs, _ = rule(db, 'all')
    for target in all_prereqs:
        if not target.startswith('out/Downwell.') or target.endswith('.exe'):
            continue
        prereqs, recipe = rule(db, target)
        if not recipe:
            # A pattern rule (out/Downwell.snd%): its stem fills the prerequisite.
            for pattern in re.findall(r'^(out/Downwell\.[^:%\s]*%):', db, re.M):
                head = pattern[:-1]
                if target.startswith(head):
                    prereqs, recipe = rule(db, pattern)
                    prereqs = [p.replace('%', target[len(head):]) for p in prereqs]
                    break
        copy = next((line for line in recipe if line.startswith('cp ')), None)
        if copy is None:
            sys.exit(f'{target} is not copied from generated/')
        source = [word for word in copy.split()[1:] if not word.startswith('-')][0]
        source = prereqs[0] if source == '$<' else source
        name = generated_name(source)
        if not name:
            sys.exit(f'{target} does not come from generated/: {source}')
        if name == 'amiga-tables.bin':
            # WI.3's container: each section is an input in its own right.
            pack = runpy.run_path(str(ROOT / 'tools/pack-amiga-tables.py'))
            for tag, section, _ in pack['SECTIONS']:
                # A header table's section (tools/install-table-headers.py)
                # is that header's data, installed.
                if section.startswith('installed/') and section.endswith('.bin'):
                    section = section[len('installed/'):-len('.bin')]
                files[section] = f'{target[len("out/"):]} ({tag})'
            continue
        files[name] = target[len('out/'):]
    return linked, files


def document(linked, files):
    counts = {}
    for name in linked | set(files):
        counts[TABLE[name][0]] = counts.get(TABLE[name][0], 0) + 1
    out = ['# Amiga build inputs (WI.2)', '',
           'Generated by `tools/amiga-input-inventory.py` from the shipping Amiga build:',
           'the headers its objects include (`amiga/obj/*.d`), the `.incbin`s of its',
           'assembly under the shipping flags, and the data files `make -C amiga` copies',
           'beside the executable (`Downwell.tables` as each input it packs). `--check`',
           'fails when the build and this table differ.',
           '',
           f'{len(linked)} inputs are linked into `Downwell.exe` and {len(files)} are data files; '
           f'{counts.get("own", 0)} are our own computation, {counts.get("layout", 0)} only describe '
           f'converted files and {counts.get("derived", 0)} are derived from the player\'s files; '
           f'{sum(TABLE[n][0] == "derived" for n in linked)} derived inputs are still linked.',
           '',
           '- **own**: no byte of the player\'s files reaches it; it stays linked.',
           '- **layout**: sizes, budgets or offsets of converted files and no game bytes. It may stay',
           '  linked while the converter reproduces those files byte for byte.',
           '- **derived**: built from the player\'s `data.win` (by chunk), `language.ini` or the',
           '  runner `Downwell.exe`. WI.3 moves the linked ones into data files; WI.4\'s converter',
           '  builds them all from the player\'s copy.',
           '',
           '"UTMT export" is the UndertaleModTool 0.9.2.0 export of `data.win`',
           '(`tools/extract.sh`): its manifest, texture pages, CodeEntries and Assembly.',
           '']
    for title, names, show_file in (('Linked into the executable', sorted(linked), False),
                                    ('Data files beside the executable', sorted(files), True)):
        out += [f'## {title}', '']
        out += ['| Input | Class | Sources | Made by | Holds |' if not show_file else
                '| Input | Installed as | Class | Sources | Made by | Holds |',
                '|---|---|---|---|---|' if not show_file else '|---|---|---|---|---|---|']
        order = {'derived': 0, 'layout': 1, 'own': 2}
        for name in sorted(names, key=lambda n: (order[TABLE[n][0]], n)):
            kind, sources, made, holds = TABLE[name]
            cells = [f'`{name}`'] + ([f'`{files[name]}`'] if show_file else []) + [kind, sources, made, holds]
            out.append('| ' + ' | '.join(cells) + ' |')
        out.append('')
    player = sorted({part.strip().split(' ')[0] for name in linked | set(files)
                     for part in TABLE[name][1].replace(';', ',').split(',')
                     if TABLE[name][0] == 'derived' and part.strip().split(' ')[0] in
                     ('data.win', 'language.ini', 'Downwell.exe', 'audiogroup1-6.dat',
                      'amb1.ogg', 'sfxfalling.ogg', 'sfxbossfirsthit.ogg')})
    chunks = sorted({word for name in linked | set(files) for word in
                     re.findall(r'\b[A-Z]{4}\b', TABLE[name][1])})
    out += ['## What the converter reads', '',
            'Player files: ' + ', '.join(f'`{p}`' for p in player) + '.',
            '`data.win` chunks: ' + ', '.join(chunks) + '.',
            'The run bank holds `sfxFalling` (186), whose only copy is the loose',
            '`sfxfalling.ogg`. `sound-durations.inc` holds the length of all 361 sounds, so it',
            'also reads the 10 songs in `audiogroup1-6.dat`, `amb1.ogg` and',
            '`sfxbossfirsthit.ogg`. Their lengths are all it needs: the converter can read each',
            'Ogg\'s last granule position rather than decode them. The wind bank',
            '(`amiga-wind.bin`) is linked only by the wind probe, not the game.', '']
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--check', action='store_true', help='fail on drift instead of writing')
    args = parser.parse_args()
    linked, files = shipping_inputs()
    installed_layout(linked)
    found = linked | set(files)
    unknown, gone = sorted(found - set(TABLE)), sorted(set(TABLE) - found)
    if unknown or gone:
        for name in unknown:
            print(f'unclassified shipping input: {name}', file=sys.stderr)
        for name in gone:
            print(f'classified but no longer a shipping input: {name}', file=sys.stderr)
        return 1
    text = document(linked, files)
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != text:
            print(f'{OUTPUT.relative_to(ROOT)} is stale; run tools/amiga-input-inventory.py', file=sys.stderr)
            return 1
        print(f'amiga inputs: {len(linked)} linked, {len(files)} data files, all classified')
        return 0
    OUTPUT.write_text(text)
    print(f'wrote {OUTPUT.relative_to(ROOT)}: {len(linked)} linked, {len(files)} data files')
    return 0


if __name__ == '__main__':
    sys.exit(main())
