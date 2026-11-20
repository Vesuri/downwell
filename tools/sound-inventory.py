#!/usr/bin/env python3
"""W2.1: static inventory of the sound effects world 1 reaches.

Reads the UTMT export (extracted/utmt-0.9.2.0: manifest.json, CodeEntries),
the audio manifest (extracted/audio/manifest.json) and, for sizes, the Paula
conversion manifest (extracted/amiga-sfx/manifest.json from
tools/convert-amiga-sfx.py). Writes docs/sound-inventory.md: names, IDs,
sizes and verdicts only, no audio payload. --check fails if the committed
document differs.

Reachability is a closure from the world-1 rooms over object events (with
parent events where the child has none or calls event_inherited), script
calls and object creation (names, and the numeric object IDs stored in
chunkOrCrate, pBulObject, bStatObject and spEnemy). The level builders'
symbol cases are limited to the symbols in the area-1 patterns. CUTS lists
the code that world 1 cannot run, with the reason; entries guarded entirely
by global.debugMode are skipped. Sound IDs are taken from the sound calls'
first arguments, sound-holding variables and the material/gun tables.
"""
import argparse
import collections
import hashlib
import json
import pathlib
import re
import sys

VERSION = '1.0.0'
ROOT = pathlib.Path(__file__).resolve().parents[1]
UTMT = ROOT / 'extracted/utmt-0.9.2.0'
CE = UTMT / 'CodeEntries'
AUDIO = ROOT / 'extracted/audio/manifest.json'
PAULA = ROOT / 'extracted/amiga-sfx/manifest.json'
OUTPUT = ROOT / 'docs/sound-inventory.md'

ROOMS = ['rmInit', 'rmMenu', 'rmPlayMenu', 'rmMain', 'rmResult',
         'rmGroundRuin', 'rmGroundGrave', 'rmGroundMeteor', 'rmGroundDouble']

# Code or objects world 1 cannot run, and why. A cut stops the closure there.
CUTS = {
    'gml_GlobalScript_builderCatacomb': 'area 2 builder',
    'gml_GlobalScript_builderAquifer': 'area 3 builder',
    'gml_GlobalScript_builderLimbo': 'area 4 builder',
    'gml_GlobalScript_builderLimbo_new': 'area 4 builder',
    'gml_GlobalScript_builderBoss': 'area 5 builder',
    'gml_GlobalScript_builderSpacey': 'unused builder',
    'gml_GlobalScript_scrLevelPat2': 'area 2 patterns',
    'gml_GlobalScript_scrLevelPat3': 'area 4 patterns',
    'gml_GlobalScript_scrLevelPat4': 'area 3 patterns',
    'gml_GlobalScript_scrLevelPat6': 'area 5+ patterns',
    'gml_GlobalScript_scrLevelPatLimboHard': 'area 4 hard patterns',
    'gml_GlobalScript_spawnerSet2': 'area 2 spawners',
    'gml_GlobalScript_spawnerSet3': 'area 3 spawners',
    'gml_GlobalScript_spawnerSet4': 'area 4 spawners',
    'gml_GlobalScript_spawnerSet5': 'area 5 spawners',
    'gml_GlobalScript_hardSet2': 'area 2 hard spawners',
    'gml_GlobalScript_hardSet3': 'area 3 hard spawners',
    'gml_GlobalScript_hardSet4': 'area 4 hard spawners',
    'gml_GlobalScript_hardSet5': 'area 5 hard spawners',
    'hardSpikeController': 'area 2 hard mode (scrBuilderCreation)',
    'WaterLine': 'area 3 (scrBuilderCreation)',
    'WaterLineBottom': 'area 4 (scrBuilderCreation)',
    'limboShard': 'area 5 (objWall_n Step 2)',
    'endingFadeOut': 'the ending after the boss',
    'endingSparkle': 'the ending (objControlerN Alarm 5)',
    'Credit': 'the credits',
    'DebugMenu': 'debug builds only',
    'shopKeeperEnemy': 'the criminal route: nothing sets global.criminal in this build',
    'pooferBullet': 'enmEyefish never arms Alarm 1',
}
# Sounds a reachable script plays only outside world 1.
SOUND_CUTS = {
    1: 'water only (scrCheckInWater, area 3)',
    306: 'oxygen, area 3 only', 307: 'water only', 308: 'area 3 bubbles',
    310: 'oxygen, area 3 only', 313: 'water footsteps (scrJump in water)',
    314: 'water footsteps (scrJump in water)', 309: 'area 3 clam',
    6: 'sndPlip: enemies outside world 1 only',
    **{i: 'objBox_n: wood boxes are area 2' for i in range(175, 179)},
    **{i: 'objBox_n: metal boxes are area 4' for i in range(179, 183)},
}
# Name references that only initialise a default (the gun sound is played
# from global.pBulSound in scrShotSound).
NAME_REF_IGNORE = {'gml_GlobalScript_scrInitialize'}
# Port status of each world-1 sound-argument RNG draw (compact GameplayWorld,
# the Amiga's shipping path). Audio-gated draws happen only when a sound is
# not playing, so the port needs the original sample's duration in ticks.
RNG_STATUS = {
    'gml_GlobalScript_scrEnemyStomped': 'W1.9: only with Exploding Knees (pugExplodingKnees)',
    'gml_GlobalScript_scrJump': 'not world 1: water only',
    'gml_GlobalScript_scrUpButtonFunctions': 'consumed: jump and wall kick (RunSession::playPlayerSounds)',
    'gml_GlobalScript_scrTypicalBulletDamage': 'consumed (StandardBulletImpact)',
    'gml_GlobalScript_soundBullet': 'consumed (compactBulletWallHit; foliage, so one sample)',
    'gml_GlobalScript_soundFootstep': 'consumed (RunSession::playPlayerSounds)',
    'gml_GlobalScript_soundLand': 'consumed, audio-gated: only while sfxGunRecharge is not playing (SoundVoices)',
    'gml_Object_Result_Step_0': 'consumed, audio-gated: while global.voidWind is not playing (RunSession::resultStepWind)',
    'gml_Object_Scattered16_Collision_objPlayer_n': 'consumed (ScatteredEvents); W1.9 knees branch',
    'gml_Object_Scattered16_Step_0': 'consumed (ScatteredEvents)',
    'gml_Object_TorchFire_Collision_objPlayer_n': 'W1.9: only with Exploding Knees',
    'gml_Object_bulletExplosion1_Create_0': 'W1.9: only the decoy balloon creates it (every bStatExplode is 0)',
    'gml_Object_bulletLaser_Collision_subparentEnemy': 'consumed (GameplayWorld laser immune branch)',
    'gml_Object_goalTimeField_Collision_objPlayer_n': 'consumed, audio-gated: while global.voidWind is not playing (RunSession::collideGoalFieldWind)',
    'gml_Object_objBox_n_Create_0': 'consumed for area 1 (AreaOneBuilder); the other cases are other areas',
    'gml_Object_objCurrency_Step_0': 'consumed (CurrencyStepSound)',
}
# Owners whose sounds the Amiga port does not reach (PC, mobile or Steam menus).
NOT_ON_AMIGA = {
    'DisplayMenu': 'PC display menu, dropped on the Amiga (W1.10)',
    'LanguageMenu': 'language menu, dropped on the Amiga (W1.10)',
    'OptionMenuAndroid': 'Android options', 'OptionMenuIpad': 'iPad options',
    'SteamLeaderboardMenu': 'Steam leaderboards', 'TrophyMenu': 'trophies',
    'TextBox': 'PC release notes', 'TextBoxChoice': 'PC release notes',
    'releaseNotes': 'PC release notes',
}
# Scene per owner object; anything else is gameplay.
SCENES = {
    'objTitle': 'front', 'PlayMenu': 'front', 'objTransDither': 'front',
    'EnterSequence': 'front', 'objEnterDither': 'front',
    'objPauseMenu': 'pause', 'ConfirmMenu': 'pause', 'ShaderMenu': 'pause',
    'OptionMenu': 'pause', 'OptionMenuPC': 'pause', 'RecordMenu': 'pause',
    'ButtonMenu': 'pause', 'ButtonAdjustMenu': 'pause',
    'Result': 'clear', 'DeathMenu': 'results',
    'Shop': 'shop', 'shopKeeper': 'shop', 'shopCase': 'shop', 'moduleText': 'shop',
}
SCENE_ORDER = ['front', 'gameplay', 'shop', 'pause', 'clear', 'results']
# Verdicts beyond "keep". Seconds are at the crop; the Amiga keeps all of
# Chip for samples but nothing streams, so long sounds are cut or dropped.
VERDICTS = {
    191: 'cut: loop a short segment of the void ambience, or drop',
    186: 'cut: loop a short segment of the clear-screen whoosh',
    188: 'cut: shorten the title sting',
    349: 'cut: shorten (time-field wind)', 350: 'cut: shorten (time-field wind)',
    351: 'cut: shorten (time-field wind)', 352: 'cut: shorten (time-field wind)',
    353: 'cut: shorten (time-field wind)', 354: 'cut: shorten (time-field wind)',
    355: 'cut: shorten (time-field wind)',
    343: 'cut: loop a short segment (jetpack loop)',
}
LONG_SECONDS = 2.0

CALLS = (r'(?:soundPlay|soundPlayOL|audio_play_sound|audio_stop_sound|audio_is_playing|'
         r'audio_sound_gain|audio_sound_pitch)')
CALL = re.compile(CALLS + r'\s*\(\s*(choose\([^)]*\)|\d+)')
CALL_LOOP = re.compile(r'(?:soundPlay|soundPlayOL)\s*\(\s*(choose\([^)]*\)|\d+)\s*,\s*\d+\s*,\s*1\b')
VARS = (r'(?:breakSound|bounceSnd|windsnd|sndSplash|spawnSound|limbobSound|jumpySound|'
        r'ambCycle|meowsnd)')
ASSIGN = re.compile(r'\b' + VARS + r'\s*=\s*(choose\([^)]*\)|\d+)\s*;')
# Sound tables: element [n][0] of the material and hit tables is a count.
TABLE = re.compile(r'global\.(sfxFs|sfxJl|sfxBhit|bStatSound)\[[^\]]*\]\[\s*([^\]]*)\]\s*=\s*(\d+)\s*;')
COUNTED = {'sfxFs', 'sfxJl', 'sfxBhit'}
# Every choose() draws one word, even with a single argument
# (docs/runner-choose.md).
SOUND_RNG = re.compile(r'(?:soundPlay|soundPlayOL)\s*\(\s*(choose\([^)]*\)|global\.sfx\w+\[[^;]*irandom_range)'
                       r'|\b' + VARS + r'\s*=\s*choose\(')
OBJVAR = re.compile(r'(?:chunkOrCrate|global\.pBulObject|global\.bStatObject\[[^\]]*\]\[[^\]]*\]|'
                    r'spEnemy\[[^\]]*\]\[[^\]]*\])\s*=\s*(choose\([^)]*\)|\d+)\s*;')
QUERY = re.compile(r'\b(?:with|instance_exists|instance_number|place_meeting|instance_place|'
                   r'instance_position|position_meeting|collision_\w+|instance_nearest|'
                   r'instance_furthest|distance_to_object|object_is_ancestor|instance_find|place_free)'
                   r'\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\)')
COMPARE = re.compile(r'(?:object_index|parent)\s*[!=]=\s*\w+|\w+\s*[!=]=\s*object_index|case\s+\w+\s*:')
CASE = re.compile(r'case "(.)":((?:(?!case "|default:).)*)', re.S)
IDENT = re.compile(r'\b[A-Za-z_]\w*\b')
BUILDERS = ('gml_GlobalScript_scrBuilder', 'gml_GlobalScript_builderCavern')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load():
    resources = json.loads((UTMT / 'manifest.json').read_text())['resources']
    objects = {o['id']: o for o in resources['objects']}
    code = {f.stem: f.read_text(errors='replace') for f in CE.glob('*.gml')}
    return resources, objects, code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    resources, objects, code = load()
    byname = {o['name']: o for o in objects.values()}
    functions = {}
    for stem, text in code.items():
        if stem.startswith('gml_GlobalScript_'):
            for m in re.finditer(r'^function (\w+)', text, re.M):
                functions[m[1]] = stem
    symbols = set()
    for f in list(CE.glob('gml_GlobalScript_scr*Pat1.gml')) + [CE / 'gml_GlobalScript_groundRoom.gml']:
        for q in re.findall(r'"([^"]*)"', f.read_text()):
            symbols |= set(q.replace('\\r', '').replace('\\n', '').replace('\\t', ''))

    def debug_only(text):
        return re.match(r'\s*if \(global\.debugMode\)\s*\{', text) is not None and \
            text.strip().endswith('}') and text.strip().count('if (global.debugMode)') == 1 and \
            re.sub(r'\s+', '', text).startswith('if(global.debugMode){') and \
            _balanced_tail(text)

    def refs(stem):
        text = code.get(stem, '')
        if stem in BUILDERS:
            text = CASE.sub(lambda m: m[0] if m[1] in symbols else ' ', text)
        scripts = {functions[t] for t in set(IDENT.findall(text)) if t in functions and functions[t] != stem}
        bare = COMPARE.sub(' ', QUERY.sub(' ', text))
        created = {t for t in set(IDENT.findall(bare)) if t in byname}
        for m in OBJVAR.finditer(text):
            for n in map(int, re.findall(r'\d+', m[1])):
                if n and n in objects:
                    created.add(objects[n]['name'])
        return scripts, created

    def events(obj):
        out, seen, current = [], set(), obj
        inherit = None
        while current:
            for e in current['events']:
                key = (e['eventType'], e['eventSubtype'])
                if key in seen and not (inherit and key in inherit):
                    continue
                names = [a['codeName'] for a in e['actions']]
                out += names
                seen.add(key)
            inherit = {(e['eventType'], e['eventSubtype']) for e in current['events']
                       if any('event_inherited' in code.get(a['codeName'], '') for a in e['actions'])}
            current = objects.get(current['parentId'])
        return out

    rooms = {r['name']: r for r in resources['rooms']}
    todo, reached_objects, reached_code = [], set(), set()
    for name in ROOMS:
        room = rooms[name]
        if room.get('creationCodeName'):
            todo.append(('code', room['creationCodeName']))
        for inst in room['instances']:
            todo.append(('object', objects[inst['objectId']]['name']))
            if inst.get('creationCodeName'):
                todo.append(('code', inst['creationCodeName']))
    while todo:
        kind, name = todo.pop()
        if name in CUTS:
            continue
        if kind == 'object':
            if name in reached_objects:
                continue
            reached_objects.add(name)
            todo += [('code', c) for c in events(byname[name])]
            continue
        if name in reached_code or name not in code or debug_only(code[name]):
            continue
        reached_code.add(name)
        scripts, created = refs(name)
        todo += [('code', s) for s in scripts] + [('object', o) for o in created]

    # Owner objects per code entry: each reached object's events and the
    # scripts they call (not the objects they create).
    owners = collections.defaultdict(set)
    for name in reached_objects:
        stack, seen = [c for c in events(byname[name]) if c in reached_code], set()
        while stack:
            c = stack.pop()
            if c in seen:
                continue
            seen.add(c)
            owners[c].add(name)
            stack += [s for s in refs(c)[0] if s in reached_code]

    audio = {s['id']: s for s in json.loads(AUDIO.read_text())['sounds']}
    by_audio_name = {s['name']: i for i, s in audio.items()}
    paula = {s['id']: s for s in json.loads(PAULA.read_text())['sounds']} if PAULA.exists() else {}
    sounds = collections.defaultdict(lambda: {'code': set(), 'owners': set(), 'loop': False})
    # A table's sounds belong to the code that plays from it, not to the
    # initialisation scripts (scrInitialize also runs from menus and Retry).
    # Guns play global.pBulSound, copied from bStatSound, in scrShotSound.
    readers = collections.defaultdict(set)
    for stem in reached_code:
        for table in ('sfxFs', 'sfxJl', 'sfxBhit'):
            if re.search(r'soundPlay\w*\(\s*global\.' + table + r'\[', code[stem]):
                readers[table].add(stem)
    readers['bStatSound'] = {'gml_GlobalScript_scrShotSound'} & reached_code
    rng_sites = []
    names_rx = re.compile(r'\b(' + '|'.join(map(re.escape, sorted(by_audio_name, key=len, reverse=True))) + r')\b')
    for stem in sorted(reached_code):
        text = code[stem]
        found = set()
        for rx in (CALL, ASSIGN):
            for m in rx.finditer(text):
                found |= {int(n) for n in re.findall(r'\d+', m[1]) if int(n) in audio}
        for m in TABLE.finditer(text):
            i = int(m[3])
            if (m[1] not in COUNTED or m[2].strip() != '0') and i in audio:
                s = sounds[i]
                for r in readers.get(m[1], ()):
                    s['code'].add(r)
                    s['owners'] |= owners.get(r, set()) or {r}
        if stem not in NAME_REF_IGNORE:
            for m in names_rx.finditer(text):
                found.add(by_audio_name[m[1]])
        looped = {int(n) for m in CALL_LOOP.finditer(text) for n in re.findall(r'\d+', m[1])}
        for i in found:
            s = sounds[i]
            s['code'].add(stem)
            s['owners'] |= owners.get(stem, set()) or {stem}
            s['loop'] |= i in looped
        for m in SOUND_RNG.finditer(text):
            line = text.count('\n', 0, m.start()) + 1
            rng_sites.append((stem, line, text.splitlines()[line - 1].strip()))
    if 191 in sounds or 'voidAmbience' in reached_objects:
        sounds[191]['loop'] = True    # voidAmbience plays ambCycle with loop 1
        sounds[191]['owners'].add('voidAmbience')
    sounds[186]['loop'] = True        # Result Create: soundPlayOL(186, 80, 1, 1, "UI")

    rows = []
    for i in sorted(sounds):
        a = audio[i]
        s = sounds[i]
        owners_shown = sorted(o if not o.startswith('gml_') else o.split('_', 2)[-1] for o in s['owners'])
        scenes = sorted({SCENES.get(o, 'gameplay') for o in s['owners']
                         if o in reached_objects and o not in NOT_ON_AMIGA} or {'gameplay'},
                        key=SCENE_ORDER.index)
        p = paula.get(i)
        size = p['bytes'] if p else None
        seconds = size / 8363 if size else None
        if not a['output'].startswith('sound-effects/'):
            verdict = 'drop: music, deferred to WM'
        elif i in SOUND_CUTS:
            verdict = 'drop: ' + SOUND_CUTS[i]
        elif s['owners'] and all(o in NOT_ON_AMIGA for o in s['owners']):
            verdict = 'drop: ' + '; '.join(sorted({NOT_ON_AMIGA[o] for o in s['owners']}))
        elif i in VERDICTS:
            verdict = VERDICTS[i]
        elif i == 0:
            verdict = 'drop: sfxNull placeholder'
        elif seconds and seconds > LONG_SECONDS:
            verdict = 'keep; long, review the crop'
        else:
            verdict = 'keep'
        rows.append({'id': i, 'name': a['name'], 'bytes': size, 'seconds': seconds, 'loop': s['loop'],
                     'scenes': scenes, 'owners': owners_shown, 'verdict': verdict,
                     'volume': a['volume']})
    text = render(rows, reached_objects, rng_sites, paula)
    if args.check:
        if OUTPUT.read_text() != text:
            sys.exit(f'{OUTPUT} is stale: run tools/sound-inventory.py')
        print(f'{OUTPUT.relative_to(ROOT)} verified: {len(rows)} sounds')
        return
    OUTPUT.write_text(text)
    print(f'{OUTPUT.relative_to(ROOT)} written: {len(rows)} sounds, {len(reached_objects)} objects')


def _balanced_tail(text):
    body = text.strip()
    start = body.index('{')
    depth = 0
    for k in range(start, len(body)):
        depth += {'{': 1, '}': -1}.get(body[k], 0)
        if depth == 0:
            return body[k + 1:].strip() == ''
    return False


def render(rows, reached_objects, rng_sites, paula):
    def kib(n):
        return f'{n / 1024:.1f}'
    kept = [r for r in rows if not r['verdict'].startswith('drop')]
    out = ['# World 1 sound inventory', '',
           'Generated by `tools/sound-inventory.py` (W2.1); do not edit by hand.',
           'It lists names, IDs, sizes and verdicts only: no audio payload.',
           'Sizes are signed 8-bit mono at 8,363 Hz after the -30 dB tail crop',
           '(`tools/convert-amiga-sfx.py`), before any per-sound rate or cut',
           'chosen in W2.2.', '',
           f'- Tool {VERSION}; UTMT manifest sha256 `{sha(UTMT / "manifest.json")[:16]}`;',
           f'  audio manifest sha256 `{sha(AUDIO)[:16]}`' +
           (f'; Paula conversion sha256 `{sha(PAULA)[:16]}`.' if PAULA.exists() else '.'),
           f'- {len(reached_objects)} objects are reachable from the world-1 rooms; '
           f'{len(rows)} sounds are referenced by their code, '
           f'{len(kept)} of them kept or cut ({kib(sum(r["bytes"] or 0 for r in kept))} KiB '
           f'before cuts).', '',
           '## Totals by scene', '',
           'A sound used in several scenes is counted in each.', '',
           '| Scene | Sounds | KiB | Long (> 2 s) or cut, KiB | The rest, KiB |',
           '|---|---:|---:|---:|---:|']
    for scene in SCENE_ORDER:
        rs = [r for r in kept if scene in r['scenes']]
        long_ = sum(r['bytes'] or 0 for r in rs if r['verdict'].startswith('cut') or (r['seconds'] or 0) > LONG_SECONDS)
        total = sum(r['bytes'] or 0 for r in rs)
        out.append(f'| {scene} | {len(rs)} | {kib(total)} | {kib(long_)} | {kib(total - long_)} |')
    out += ['', 'Scenes: front (title, garden, style select), gameplay (levels, side',
            'rooms, time-stop), shop, pause (pause menu and its screens), clear (the',
            'upgrade screen between levels), results (death and results).', '',
            '## Sounds', '',
            '| ID | Name | Bytes | s | Loop | Scenes | Played by | Verdict |',
            '|---:|---|---:|---:|:-:|---|---|---|']
    for r in rows:
        owners = ', '.join(r['owners'][:6]) + (f' +{len(r["owners"]) - 6}' if len(r['owners']) > 6 else '')
        b = f'{r["bytes"]:,}' if r['bytes'] is not None else '–'
        s = f'{r["seconds"]:.2f}' if r['seconds'] is not None else '–'
        out.append(f'| {r["id"]} | `{r["name"]}` | {b} | {s} | {"yes" if r["loop"] else ""} | '
                   f'{", ".join(r["scenes"])} | {owners} | {r["verdict"]} |')
    out += ['', '## Sound-argument RNG draws', '',
            'Each of these consumes the world RNG whether or not audio plays, so the',
            'port must make the same draws. Port status is for the compact',
            '`GameplayWorld`, the Amiga\'s shipping path.', '',
            '| Code entry | Line | Source | Port |', '|---|---:|---|---|']
    for stem, line, src in rng_sites:
        status = RNG_STATUS.get(stem, '**unaudited**')
        out.append(f'| `{stem.replace("gml_", "")}` | {line} | `{src.replace("|", "\\|")}` | {status} |')
    return '\n'.join(out) + '\n'


if __name__ == '__main__':
    main()
