#!/usr/bin/env python3
"""WI.3: move the tables of generated headers into Downwell.tables.

Each header listed in HEADERS keeps its linked form for the host build.
For the Amiga this tool writes two files per header:
- generated/installed/<name>.bin: one Downwell.tables section holding every
  table in the 68000's own layout (big-endian; structs as m68k-gcc lays
  them out, which the installed header static_asserts);
- generated/installed/<name>: the same header with each table replaced by
  an inline pointer of the same name, bound at startup (so `name[i][j]`
  reads as before), and name_count, its outer extent. The rest of the
  header is kept: `sizeof(t)/sizeof(t[0])` becomes t_count and
  static_asserts on sizeof() of a table are dropped.
generated/installed/bind-tables.h binds them all.

Tables are integer arrays, char arrays initialised by a string, and arrays
of the structs in STRUCTS, whose pointer fields hold a string literal or
another table's name. Pointers are stored as offsets from the section
start and listed in the section's relocation list; bind adds the section's
address to each once (the loaded file is writable).

Section layout, big-endian: u16 tables, u16 bound flag (0 in the file),
u32 relocations, u32 offset of each table, u32 position of each relocated
pointer, then the tables (each 4-byte aligned) and the strings.
--check fails if an output is stale.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'generated/installed'

# tag: generated header
HEADERS = [
    ('AMFA', 'amiga-ammo-factored.h'),
    ('AMDV', 'amiga-ammo-dividers.h'),
    ('HUDT', 'amiga-hud-text.h'),
    ('GOAL', 'downwell-goal-mask.h'),
    ('TFLD', 'downwell-time-field-mask.h'),
    ('ATTL', 'downwell-area-titles.h'),
    ('MTXT', 'downwell-menu-text.h'),
    ('SPCH', 'downwell-shop-speech.h'),
    ('PALS', 'downwell-palettes.h'),
    ('RISE', 'downwell-rising-texts.h'),
    ('RTPL', 'amiga-room-terrain-plan.inc'),
    ('SNDD', 'sound-durations.inc'),
    ('SNDG', 'sound-gains.inc'),
    ('HART', 'amiga-health-art.h'),
    ('DMGT', 'amiga-damage-text.h'),
    ('CMOT', 'contact-motion.inc'),
    ('STRG', 'smoke-trig.inc'),
]
# Fragments included inside a namespace or an initializer:
# - space: the namespace its tables live in when the file names none;
# - include: what bind-tables.h includes to see them (default: the file);
# - wrap: a declaration around a bare initializer fragment, in `space`;
# - omit: tables nothing in any Amiga build reaches; they are not installed
#   and their pointers stay null.
OPTIONS = {
    # AmigaPreparedHudText::streak() has no caller (AmigaGemStreakHud draws
    # the streak); the linked build's LTO dropped these 42 KB too.
    'amiga-hud-text.h': dict(omit={'streakEmpty', 'streakBackground', 'streakIconRows', 'streak', 'streakPairs'}),
    'amiga-room-terrain-plan.inc': dict(space='amiga_room_plan_detail',
                                        include='../../src/platform/amiga/AmigaRoomTerrainPlan.h'),
    'amiga-health-art.h': dict(space='AmigaHealthArt', include='../../src/platform/amiga/AmigaHealthArt.h'),
    'amiga-damage-text.h': dict(space='downwell_damage_bank',
                                include='../../src/platform/amiga/AmigaDamageText.h'),
    'smoke-trig.inc': dict(space='downwell::smoke_trig_detail', program=True,
                           include='../../src/core/SmokeTrig.h'),
    'contact-motion.inc': dict(space='downwell::contact_motion_detail', include='../../src/core/ContactMotion.h'),
    'sound-durations.inc': dict(space='downwell', wrap='static const std::uint16_t sourceSoundTicks[] = {'),
    'sound-gains.inc': dict(space='downwell', wrap='static const std::uint32_t sourceSoundGains[][2] = {'),
}

TYPES = {
    'unsigned char': 'B', 'std::uint8_t': 'B', 'uint8_t': 'B', 'UBYTE': 'B',
    'signed char': 'b', 'std::int8_t': 'b',
    'unsigned short': 'H', 'std::uint16_t': 'H', 'uint16_t': 'H', 'UWORD': 'H',
    'short': 'h', 'std::int16_t': 'h', 'int16_t': 'h', 'WORD': 'h',
    'unsigned': 'I', 'unsigned int': 'I', 'std::uint32_t': 'I', 'uint32_t': 'I', 'ULONG': 'I',
    'int': 'i', 'std::int32_t': 'i', 'int32_t': 'i', 'LONG': 'i',
    'std::int64_t': 'q', 'std::uint64_t': 'Q',
}
# Struct fields in order: 'p' is a pointer (a string or a table), the rest
# are struct format codes. m68k-gcc aligns int and pointers to 2 bytes.
STRUCTS = {
    'AreaTitle': 'pipI',            # core/LevelNumberText.h
    'ShopLanguageEntry': 'ppp',     # core/ShopText.h
    'PaletteEntry': 'piHHHH',       # core/Palettes.h
    'Command': 'IHHhhHHBB',         # AmigaRoomTerrainPlan.h
    'AmigaHealthPair': 'HH',        # AmigaHealthArt.h
    'AmigaHudGlyph': '16H16HiiII',  # AmigaRenderer.h
    'Span': 'IIp',                  # amiga-health-art.h
    'AmigaDamageGlyphs': '110B',    # AmigaRenderer.h
    'AmigaPreparedDamageText': 'II36H36H',  # AmigaDamageText.h
    'ContactMotionCodec::Correction': 'II',  # core/ContactMotionCodec.h
}
STRUCT_NAMESPACE = {'AreaTitle': 'downwell', 'ShopLanguageEntry': 'downwell', 'PaletteEntry': 'downwell',
                    'Command': 'amiga_room_plan_detail', 'AmigaHealthPair': '', 'AmigaHudGlyph': '',
                    'Span': 'AmigaHealthArt', 'AmigaDamageGlyphs': '', 'AmigaPreparedDamageText': '',
                    'ContactMotionCodec::Correction': 'downwell'}
FIELD_BYTES = {'p': 4, 'B': 1, 'b': 1, 'H': 2, 'h': 2, 'I': 4, 'i': 4, 'q': 8, 'Q': 8}

_ALTERNATIVES = '|'.join(sorted((re.escape(t) for t in list(TYPES) + list(STRUCTS) + ['char']),
                                key=len, reverse=True))
DECL = re.compile(r'(?:static|inline)\s+(?:const|constexpr)\s+(?:const\s+)?'
                  rf'(?P<type>{_ALTERNATIVES})\s+(?P<name>\w+)(?P<dims>(?:\[\d*\])*)\s*=\s*(?P<open>[{{"])')
NUMBER = re.compile(r'-?(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*')


def matching_brace(text, start):
    depth, i = 0, start
    while i < len(text):
        c = text[i]
        if c == '"':
            i = string_end(text, i)
            continue
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError('unbalanced initializer')


def string_end(text, start):
    i = start + 1
    while text[i] != '"':
        i += 2 if text[i] == '\\' else 1
    return i + 1


def decode_string(literal):
    out, i, body = bytearray(), 0, literal[1:-1]
    simple = {'n': 10, 't': 9, 'r': 13, '0': 0, '\\': 92, '"': 34, "'": 39}
    while i < len(body):
        c = body[i]
        if c != '\\':
            out += c.encode('utf-8')
            i += 1
            continue
        i += 1
        if body[i] in '01234567':
            j = i
            while j < len(body) and j < i + 3 and body[j] in '01234567':
                j += 1
            out.append(int(body[i:j], 8))
            i = j
        elif body[i] == 'x':
            j = i + 1
            while j < len(body) and body[j] in '0123456789abcdefABCDEF':
                j += 1
            out.append(int(body[i + 1:j], 16))
            i = j
        else:
            out.append(simple[body[i]])
            i += 1
    return bytes(out)


def strip_comments(text):
    out, i = [], 0
    while i < len(text):
        if text[i] == '"':
            end = string_end(text, i)
            out.append(text[i:end])
            i = end
        elif text.startswith('//', i):
            i = text.find('\n', i) if '\n' in text[i:] else len(text)
        elif text.startswith('/*', i):
            i = text.index('*/', i) + 2
        else:
            out.append(text[i])
            i += 1
    return ''.join(out)


def split_top(text):
    """Items of a brace body separated by commas at depth 0."""
    text = strip_comments(text)
    items, depth, start, i = [], 0, 0, 0
    while i < len(text):
        c = text[i]
        if c == '"':
            i = string_end(text, i)
            continue
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
        elif c == ',' and depth == 0:
            items.append(text[start:i].strip())
            start = i + 1
        i += 1
    items.append(text[start:].strip())
    return [item for item in items if item]


def namespaces_at(text, position):
    """The namespace path open at position (namespace blocks only)."""
    stack = []
    for match in re.finditer(r'namespace\s+([\w:]+)\s*\{|[{}]', text[:position]):
        if match.group(1):
            stack.append(match.group(1))
        elif match.group(0) == '{':
            stack.append(None)
        elif stack:
            stack.pop()
    return '::'.join(name for name in stack if name)


class Table:
    def __init__(self, space, name, data, pointers, count):
        # pointers: (byte offset in data, ('string', bytes) | ('table', name))
        self.space, self.name, self.data, self.pointers, self.count = space, name, data, pointers, count


def struct_table(kind, body, name, dims):
    fields = STRUCTS[kind]
    if 'p' not in fields:
        # Integers only: the initializer's numbers in order, whatever its
        # braces, packed by the struct's format codes.
        values = [int(v.rstrip('uUlL'), 0) for v in NUMBER.findall(strip_comments(body))]
        codes = re.findall(r'(\d*)([A-Za-z])', fields)
        flat = ''.join(code * int(n or 1) for n, code in codes)
        if len(values) % len(flat):
            raise ValueError(f'{name}: {len(values)} values do not fill {kind} structs')
        count = len(values) // len(flat)
        data = bytearray()
        for i, value in enumerate(values):
            code = flat[i % len(flat)]
            bits = FIELD_BYTES[code] * 8
            data += struct.pack('>' + code, value & ((1 << bits) - 1) if code.isupper() else value)
        inner = 1
        for d in dims[1:]:
            inner *= d
        if count % inner or (dims and dims[0] is not None and count != dims[0] * inner):
            raise ValueError(f'{name}: {count} {kind} structs do not match {dims}')
        return bytes(data), [], count // inner, struct.calcsize('>' + fields)
    if len(dims) != 1:
        raise ValueError(f'{name}: {kind} has pointers and is not a one-dimensional table')
    size = sum(FIELD_BYTES[f] for f in fields)
    data, pointers = bytearray(), []
    entries = split_top(body)
    for entry in entries:
        if not (entry.startswith('{') and entry.endswith('}')):
            raise ValueError(f'{name}: entry {entry[:40]} is not a braced struct')
        values = split_top(entry[1:-1])
        if len(values) != len(fields):
            raise ValueError(f'{name}: entry has {len(values)} fields, {kind} has {len(fields)}')
        for field, value in zip(fields, values):
            if field == 'p':
                if value.startswith('"'):
                    target = ('string', b''.join(decode_string(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', value)))
                elif value in ('nullptr', '0', 'NULL'):
                    target = None
                else:
                    target = ('table', value)
                if target:
                    pointers.append((len(data), target))
                data += b'\0\0\0\0'
            else:
                number = int(value.rstrip('uUlL'), 0)
                bits = FIELD_BYTES[field] * 8
                data += struct.pack('>' + field, number & ((1 << bits) - 1) if field.isupper() else number)
    assert len(data) == size * len(entries)
    return bytes(data), pointers, len(entries), size


CONSTANT = re.compile(r'Real::constant\((0x[0-9a-fA-F]+)u,(0x[0-9a-fA-F]+)u\)')


def convert_program(name, text, space):
    """The runner trig program: the operation order stays code; its table and
    every binary64 constant move into the section."""
    match = re.search(r'static const Real table\[64\]\[4\]=\{(.*?)\};\n', text, re.S)
    table = [(int(h, 16), int(l, 16)) for h, l in CONSTANT.findall(match.group(1))]
    if len(table) != 256:
        raise ValueError(f'{name}: the table has {len(table)} constants, not 256')
    text = text[:match.start()] + text[match.end():]
    constants, index = [], {}

    def constant(m):
        value = (int(m.group(1), 16), int(m.group(2), 16))
        if value not in index:
            index[value] = len(constants)
            constants.append(value)
        n = index[value]
        return f'Real::constant(smokeTrigConstants[{n}][0],smokeTrigConstants[{n}][1])'
    text = CONSTANT.sub(constant, text)
    text = re.sub(r'\btable\[([^\]]+)\]\[(\d)\]',
                  r'Real::constant(smokeTrigTable[\1][\2][0],smokeTrigTable[\1][\2][1])', text)
    if 'table[' in text or '0x' in re.sub(r'//[^\n]*', '', text).replace('index += ', ''):
        leftover = [line for line in text.splitlines() if not line.startswith('//') and
                    re.search(r'0x[0-9a-fA-F]{5,}', line)]
        if leftover:
            raise ValueError(f'{name}: constants remain: {leftover[:2]}')
    # Our transcription keeps the runner's instruction addresses as comments.
    text = re.sub(r'^// [0-9a-f]{8} [^\n]*\n', '', text, flags=re.M)
    tables = [Table(space, 'smokeTrigTable', struct.pack('>512I', *[w for pair in table for w in pair]), [], 64),
              Table(space, 'smokeTrigConstants',
                    struct.pack(f'>{2 * len(constants)}I', *[w for pair in constants for w in pair]), [],
                    len(constants))]
    header = ('inline const std::uint32_t (*smokeTrigTable)[4][2] = nullptr; '
              'inline constexpr unsigned smokeTrigTable_count = 64;\n'
              'inline const std::uint32_t (*smokeTrigConstants)[2] = nullptr; '
              f'inline constexpr unsigned smokeTrigConstants_count = {len(constants)};\n' + text)
    return header, section(name, tables), tables


def convert(name):
    text = (ROOT / 'generated' / name).read_text()
    options = OPTIONS.get(name, {})
    if options.get('program'):
        return convert_program(name, text, options['space'])
    if 'wrap' in options:
        text = f'{options["wrap"]}\n{text}\n}};\n'
    tables, pieces, cursor, sizes = [], [], 0, {}
    for match in DECL.finditer(text):
        if match.start() < cursor:
            continue
        kind, array = match.group('type'), match.group('name')
        dims = [int(d) if d else None for d in re.findall(r'\[(\d*)\]', match.group('dims'))]
        if match.group('open') == '"':
            close = string_end(text, match.end() - 1)
            data = decode_string(text[match.end() - 1:close]) + b'\0'
            pointers, count, shape = [], len(data), ''
            if kind != 'char' or len(dims) != 1:
                raise ValueError(f'{name}: {array} is not a char array')
        else:
            open_brace = match.end() - 1
            close = matching_brace(text, open_brace) + 1
            body = text[open_brace + 1:close - 1]
            if kind in STRUCTS:
                data, pointers, count, size = struct_table(kind, body, array, dims)
                sizes[kind] = size
                shape = ''.join(f'[{d}]' for d in dims[1:])
            else:
                values = [int(v.rstrip('uUlL'), 0) for v in NUMBER.findall(strip_comments(body))]
                inner = 1
                for d in dims[1:]:
                    inner *= d
                if dims[0] is None:
                    if len(values) % inner:
                        raise ValueError(f'{name}: {array} does not fill its rows')
                    dims[0] = len(values) // inner
                if len(values) != dims[0] * inner:
                    raise ValueError(f'{name}: {array} has {len(values)} values, not {dims[0] * inner}')
                code = TYPES[kind]
                bits = struct.calcsize(code) * 8
                data = struct.pack('>' + code * len(values),
                                   *[v & ((1 << bits) - 1) if code.isupper() else v for v in values])
                pointers, count = [], dims[0]
                shape = ''.join(f'[{d}]' for d in dims[1:])
        end = text.index(';', close) + 1
        if array in options.get('omit', ()):
            pieces.append(text[cursor:match.start()])
            pieces.append(f'inline const {kind} {f"(*{array}){shape}" if shape else f"*{array}"} = nullptr; '
                          f'inline constexpr unsigned {array}_count = {count}; // not installed: unused')
            cursor = end
            continue
        tables.append(Table(namespaces_at(text, match.start()) or options.get('space', ''),
                            array, data, pointers, count))
        pointer = f'(*{array}){shape}' if shape else f'*{array}'
        pieces.append(text[cursor:match.start()])
        pieces.append(f'inline const {kind} {pointer} = nullptr; '
                      f'inline constexpr unsigned {array}_count = {count};')
        cursor = end
    pieces.append(text[cursor:])
    header = ''.join(pieces)
    for table in tables:
        header = re.sub(rf'sizeof\(\s*{table.name}\s*\)\s*/\s*sizeof\(\s*{table.name}\s*\[\s*0\s*\]\s*\)',
                        f'{table.name}_count', header)
        header = re.sub(rf'^\s*static_assert\([^;]*sizeof\(\s*{table.name}\s*\)[^;]*;\s*$\n?', '', header,
                        flags=re.M)
    if 'wrap' in options:
        header = f'#include <cstdint>\nnamespace {options["space"]} {{\n{header}}}\n'
    if sizes:
        guards = ''.join(f'static_assert(sizeof(::{STRUCT_NAMESPACE[kind] + "::" if STRUCT_NAMESPACE[kind] else ""}'
                         f'{kind})=={size},'
                         f'"{kind} matches Downwell.tables");\n'
                         for kind, size in sorted(sizes.items()))
        header += guards
    return header, section(name, tables), tables


def section(name, tables):
    relocations = sum(len(t.pointers) for t in tables)
    start = 8 + 4 * len(tables) + 4 * relocations
    body, offsets = bytearray(), {}
    for table in tables:
        body += b'\0' * (-(start + len(body)) % 4)
        offsets[table.name] = start + len(body)
        body += table.data
    strings, positions = {}, []
    for table in tables:
        for at, (kind, value) in table.pointers:
            if kind == 'table':
                if value not in offsets:
                    raise ValueError(f'{name}: {table.name} points at unknown table {value}')
                target = offsets[value]
            else:
                if value not in strings:
                    strings[value] = start + len(body)
                    body += value + b'\0'
                target = strings[value]
            position = offsets[table.name] + at
            struct.pack_into('>I', body, position - start, target)
            positions.append(position)
    return (struct.pack('>HHI', len(tables), 0, relocations) +
            b''.join(struct.pack('>I', offsets[t.name]) for t in tables) +
            b''.join(struct.pack('>I', p) for p in positions) + bytes(body))


def binder(converted):
    lines = ['// Generated by tools/install-table-headers.py; binds the installed tables.',
             '#pragma once']
    lines += [f'#include "{OPTIONS.get(name, {}).get("include", name)}"' for _, name in HEADERS]
    lines += ['namespace downwell_installed_tables {',
              'inline unsigned long word32(const unsigned char* p) {',
              '    return (unsigned long)p[0] << 24 | (unsigned long)p[1] << 16 | (unsigned long)p[2] << 8 | p[3];',
              '}',
              '// One section: check its table count, relocate its pointers once.',
              'inline unsigned char* section(unsigned char* s, unsigned long bytes, unsigned tables) {',
              '    if (!s || bytes < 8 || (unsigned(s[0]) << 8 | s[1]) != tables) return nullptr;',
              '    const unsigned long relocations = word32(s + 4);',
              '    if (bytes < 8 + 4 * (tables + relocations)) return nullptr;',
              '    if (s[3]) return s;',
              '    const unsigned char* list = s + 8 + 4 * tables;',
              '    for (unsigned long i = 0; i < relocations; ++i) {',
              '        unsigned char* p = s + word32(list + 4 * i);',
              '        const unsigned long address = (unsigned long)s + word32(p);',
              '        p[0] = (unsigned char)(address >> 24); p[1] = (unsigned char)(address >> 16);',
              '        p[2] = (unsigned char)(address >> 8); p[3] = (unsigned char)address;',
              '    }',
              '    s[3] = 1;',
              '    return s;',
              '}',
              '// find(tag, bytes) returns the writable Downwell.tables section or nullptr.',
              'template<class Find> inline bool bind(Find find) {',
              '    unsigned long bytes = 0;',
              '    unsigned char* s = nullptr;']
    for (tag, name), (_, _, tables) in zip(HEADERS, converted):
        value = struct.unpack('>I', tag.encode())[0]
        lines.append(f'    s = find(0x{value:08x}ul, bytes); // {tag} {name}')
        lines.append(f'    s = section(s, bytes, {len(tables)});')
        lines.append('    if (!s) return false;')
        for i, table in enumerate(tables):
            qualified = f'::{table.space}::{table.name}' if table.space else f'::{table.name}'
            lines.append(f'    {qualified} = reinterpret_cast<decltype({qualified})>(s + word32(s + 8 + 4 * {i}));')
    lines += ['    return true;', '}', '}']
    return '\n'.join(lines) + '\n'


def outputs():
    converted = [convert(name) for _, name in HEADERS]
    files = {}
    for (tag, name), (header, blob, _) in zip(HEADERS, converted):
        files[OUT / name] = ('#pragma once\n// Installed form (tools/install-table-headers.py): tables are '
                             'bound from Downwell.tables.\n' + header).encode()
        files[OUT / (name + '.bin')] = blob
    files[OUT / 'bind-tables.h'] = binder(converted).encode()
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    files = outputs()
    if args.check:
        stale = [p for p, data in files.items() if not p.exists() or p.read_bytes() != data]
        for path in stale:
            print(f'{path.relative_to(ROOT)} is stale; run tools/install-table-headers.py', file=sys.stderr)
        return 1 if stale else 0
    OUT.mkdir(parents=True, exist_ok=True)
    for path, data in files.items():
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
    print(f'wrote {len(HEADERS)} installed headers to {OUT.relative_to(ROOT)}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
