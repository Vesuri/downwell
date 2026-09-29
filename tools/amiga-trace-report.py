#!/usr/bin/env python3
"""Attribute an FS-UAE cycle-exact instruction trace to Downwell call stacks.

Input is a directory written by `bash amiga/profile.sh trace`: one
trace-<frame>.bin per capture (the Barto remote debugger's `monitor profile`
format) and the matching Downwell.elf. Every executed instruction is recorded
with its cycle cost and its registers before execution; nothing is sampled.

Call stacks are rebuilt from the instruction stream: a user-mode jsr/bsr whose
successor has SP lowered by four pushes a frame; a frame pops as soon as SP
rises above its return-address slot. Interrupt (supervisor) execution is kept
on a separate stack below an `[interrupt]` frame. Each frame's call site and
the leaf PC are expanded through addr2line's inline chain, so functions
inlined by LTO appear as their own logical frames.

Game frames are split at entries to downwellTraceFrameMark (TRACE_FRAMES
builds). Recording starts at the VBL ending the requested frame's wait, so a
capture's first frame lacks only its pre-VBL wait; its last frame is cut off
and excluded. Cycles are 68000 clocks; a PAL field is 142,102 of them.

  python3 tools/amiga-trace-report.py tmp/trace/run [--frames=N|N-M]
      [--min-fields=F] [--top=60] [--tree-min=0.5] [--focus=name]
      [--lines=name] [--collapsed=FILE]
"""
import argparse
import bisect
import collections
import os
import re
import shutil
import struct
import subprocess
import sys

FIELD_CYCLES = 142102
IRQ_PC = 0x7fffffff
BIN = os.path.expanduser('~/.local/opt/bin')


def tool(name):
    return shutil.which(name) or os.path.join(BIN, name)


def load_code(elf):
    """Function symbols and call-instruction successors from the ELF."""
    table = subprocess.run([tool('m68k-amiga-elf-objdump'), '-t', elf],
                           capture_output=True, text=True, check=True).stdout
    symbols = {}
    for line in table.split('\n'):
        m = re.match(r'^([0-9a-f]{8})\s.*\sF\s\.text\s+([0-9a-f]+)\s(.+)$', line)
        if m:
            symbols[int(m.group(1), 16)] = m.group(3)
    raw = list(symbols.items())
    names = subprocess.run([shutil.which('c++filt') or 'c++filt'],
                           input='\n'.join(re.sub(r'\.(lto_priv|constprop|isra|part)\.\d+', '', n)
                                           for _, n in raw) + '\n',
                           capture_output=True, text=True, check=True).stdout.split('\n')
    for (address, _), name in zip(raw, names):
        symbols[address] = name
    disassembly = subprocess.run(
        [tool('m68k-amiga-elf-objdump'), '-d', '--no-show-raw-insn', elf],
        capture_output=True, text=True, check=True).stdout
    calls, text = {}, {}
    pending = None
    for line in disassembly.split('\n'):
        m = re.match(r'^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$', line)
        if not m:
            continue
        address = int(m.group(1), 16)
        if pending is not None:
            calls[pending] = address
            pending = None
        mnemonic = m.group(2)
        text[address] = (mnemonic, m.group(3))
        if mnemonic == 'jsr' or mnemonic.startswith('bsr'):
            pending = address
    return symbols, calls, text


HELPERS = {'__mulsi3', '__udivsi3', '__divsi3', '__modsi3', '__umodsi3', 'memset', 'memcpy',
           'downwellMemoryCopy'}


BRANCH = re.compile(r'^(bra|b(hi|ls|cc|cs|hs|lo|ne|eq|vc|vs|pl|mi|ge|lt|gt|le)|db[a-z]+|jmp)(\.[bswl])?$')
# An operand that is a bare hex address (immediates start with '#').
ABSOLUTE = re.compile(r'(^|,)\s*[0-9a-f]{3,}\b')


def instruction_class(mnemonic, operands, symbol):
    """Coarse cost class: what the 68000 is doing at this PC."""
    if symbol in HELPERS:
        return 'library helper (mul/div/mem*)'
    if mnemonic == 'jsr' or mnemonic.startswith('bsr'):
        return 'call (jsr/bsr)'
    if mnemonic in ('rts', 'rte', 'rtr'):
        return 'return (rts)'
    if mnemonic.startswith('movem'):
        return 'register save/restore (movem)'
    if mnemonic in ('link', 'unlk'):
        return 'frame (link/unlk)'
    if mnemonic == 'pea' or operands.endswith('-(sp)'):
        return 'argument/temporary push'
    if '(sp)' in operands:
        return 'stack-frame operand'
    if BRANCH.match(mnemonic):
        return 'branch'
    if '(' in operands or ABSOLUTE.search(operands):
        return 'other memory operand'
    return 'register-only'


def records(path):
    """Yield (field, pc|None, cycles, sp, sr) for every traced instruction."""
    with open(path, 'rb') as source:
        data = memoryview(source.read())
    offset = 0

    def word():
        nonlocal offset
        value = struct.unpack_from('<I', data, offset)[0]
        offset += 4
        return value

    fields = word()
    sections = word()
    offset += 4 * sections  # hunk base addresses; PCs are hunk-0 relative
    offset += 16  # stack bounds
    for _ in range(3):  # ROM, Chip RAM, slow RAM snapshots
        size = word()
        offset += size
    word(), word()  # base clock, cycle unit
    unpack_regs = struct.Struct('<17I').unpack_from
    for field in range(fields):
        custom = word()
        if custom != 520:
            raise ValueError(f'unsupported custom register block: {custom}')
        offset += custom
        aga = word()
        if aga not in (0, 1024):
            raise ValueError(f'unsupported AGA block: {aga}')
        offset += aga
        for _ in range(2):  # DMA slot and resource records
            size, count = word(), word()
            offset += size * count
        cycles, _idle, count = word(), word(), word()
        end = offset + 4 * count
        accounted = 0
        while offset < end:
            pc = None
            while True:
                value = struct.unpack_from('<I', data, offset)[0]
                offset += 4
                if value >= 0xffff0000:
                    break
                if pc is None:
                    pc = value
            cost = 0xffffffff - value
            regs = unpack_regs(data, offset)
            offset += 68
            accounted += cost
            yield field, pc, cost, regs[15], regs[16]
        if offset != end:
            raise ValueError('instruction record boundary mismatch')
        if abs(accounted - cycles) > max(100, cycles // 1000):
            raise ValueError(f'field {field}: {accounted} cycles traced, {cycles} elapsed')
        size, _kind = word(), word()
        offset += size  # screenshot
    if offset != len(data):
        raise ValueError('unexpected trailing trace data')


class Symbolizer:
    def __init__(self, elf, symbols):
        self.elf = elf
        self.starts = sorted(symbols)
        self.names = [symbols[a] for a in self.starts]
        self.chains = {}

    def symbol(self, pc):
        i = bisect.bisect_right(self.starts, pc) - 1
        return self.names[i] if i >= 0 else '?'

    def resolve(self, pcs):
        pcs = sorted(set(p for p in pcs if p is not None and p != IRQ_PC) - set(self.chains))
        if not pcs:
            return
        out = subprocess.run(
            [tool('m68k-amiga-elf-addr2line'), '-a', '-f', '-i', '-C', '-e', self.elf],
            input='\n'.join(hex(p) for p in pcs) + '\n',
            capture_output=True, text=True, check=True).stdout.split('\n')
        current, chain = None, []
        i = 0
        while i < len(out):
            line = out[i]
            if line.startswith('0x') and re.fullmatch(r'0x[0-9a-f]+', line):
                if current is not None:
                    self.chains[current] = chain
                current, chain = int(line, 16), []
                i += 1
                continue
            if line and i + 1 < len(out):
                chain.append((short(line), out[i + 1].rsplit('/', 1)[-1]))
                i += 2
                continue
            i += 1
        if current is not None:
            self.chains[current] = chain

    def frames(self, pc):
        """Outermost-first logical frames at pc, including inlined callers."""
        if pc is None:
            return ['[outside program]']
        if pc == IRQ_PC:
            return ['[exception entry]']
        chain = self.chains.get(pc)
        if not chain or chain[0][0] == '??':
            return [short(self.symbol(pc))]
        names = []
        for name, location in reversed(chain):
            if GENERIC.match(name):
                name = f'{name} [{location.split(":")[0]}]'
            if not names or names[-1] != name:
                names.append(name)
        return names

    def line(self, pc):
        chain = self.chains.get(pc)
        return chain[0][1] if chain else '?'


def short(name):
    name = re.sub(r'\.(lto_priv|constprop|isra|part|cold)\.\d+', '', name)
    name = name.replace(' [clone ]', '')
    # Lambdas: keep owner and ordinal, drop signatures and thunk names.
    name = re.sub(r'\{lambda\((?:[^(){}]|\([^()]*\))*\)#(\d+)\}', r'{lambda#\1}', name)
    name = re.sub(r'::_FUN$', '', name)
    name = re.sub(r'::_FUN\(.*$', '', name)
    # Drop the outermost trailing parameter list; keep template/namespace context.
    stripped = name
    if stripped.endswith(' const'):
        stripped = stripped[:-6]
    if stripped.endswith(')') and not stripped.endswith('operator()'):
        depth = 0
        for i in range(len(stripped) - 1, -1, -1):
            c = stripped[i]
            if c == ')':
                depth += 1
            elif c == '(':
                depth -= 1
                if depth == 0:
                    stripped = stripped[:i]
                    break
    if stripped.endswith('operator'):
        stripped += '()'
    name = stripped.replace('downwell::', '').replace('(anonymous namespace)::', '')
    name = re.sub(r'::\{lambda#(\d+)\}::operator\(\)$', r'::{lambda#\1}', name)
    return name if len(name) <= 110 else name[:107] + '...'


GENERIC = re.compile(r'^(operator\(\)|operator\(\)<.*>|operator\(\)\(.*\))$')


# Cost owners for --causes, checked from the innermost frame outwards; the
# first frame matching any owner decides. Helpers are charged to callers.
# "Runner bookkeeping" is the emulated GameMaker runner state: lifetimes,
# room/layer/object lists, dirty queues, bounds caches, publication, checks.
CAUSES = [
    ('R-tree upkeep', r'RunnerSpatial(Index|Journal|Split|NodeStorage)|serviceSpatialIndex'),
    ('software float emulation', r'MotionBinary64|RunnerImageValue|[mM]ultiplyBinary64|'
     r'[dD]ivideBinary64|roundGameMaker'),
    ('runner bookkeeping', r'RunnerRoomMembership|RunnerLayer|GameplayRoomLayers|'
     r'RunnerObjectMembership|RunnerSpatialInstances|RunnerSpatialDirty|RunnerSpatialBounds|'
     r'RunnerSpatialPosition|RunnerStepSnapshot|RunnerObjectFamilies|RecordLifecycle|'
     r'GameplayCollisionOwner::(Binding|valid|refresh|reconcile|combatAlarm|dispatchCaptured|'
     r'cleanupDestroyed|prepareDraw|resolves)|GameplayCollisionView|Prepared(Terrain|UnitSprite)'
     r'\w*Bounds|PreparedSpriteGeometry|GameplayTerrainInstances::(append|createProperties)|'
     r'captureEnemyStepOrder|resolve(Enemy|Bullet)Lifetime|publish(Enemy|Player|Bullet)|'
     r'ProjectilePublication|visitRoomActors|InstanceLifecycle|DeferredInstanceQueue|'
     r'terrainCreateProperties'),
    ('generation logic', r'AreaOneBuilder|cavernEnemyChoice|GameMakerRandom|BuilderPattern|'
     r'AmigaGameplayGeneration|AmigaTerrainRunIndex|amigaAddTerrainObject|TerrainWallContacts|'
     r'AmigaAreaOneTerrainBuilder|createGenerated|appendBreakables|configureEnemy'),
    ('effect surface (currency/smoke)', r'PaletteEffectSurface|CurrencySmokeComposition|'
     r'EffectSurfacePlacement|AmigaGameplayEffects'),
    ('time-field outlines', r'AmigaFieldOverlay|amigaFieldWords|AmigaGoalField|TimeFieldOutlines|'
     r'FieldSpans|FieldWords'),
    ('ammo HUD', r'AmmoHud|redrawAmmo|redrawStreak|AmmoGauge|amigaRenderer\w*Hud|AmmoDivider'),
    ('terrain rows and scrolling', r'drawMainWorldSpan|AmigaTerrainPainter|visitWallRows|'
     r'drawWallPresentation|syncGameRows|AmigaGameplayRows|prepareStage|AmigaTerrainRaster'),
    ('actors, sprites and display', r'AmigaGameplayActors|AmigaGameplayDrawList|amigaRenderer|'
     r'drawAsset|buildHardwareSprite|AmigaGameplayScene|Copper|Bitmap::|AmigaHardware|'
     r'AmigaActorChannels|SpriteAssignment'),
    ('game logic', r'PlayerMotion|stepGameplayPlayer|WalkingEnemyMotion|SnailMotion|Frog|Bat|'
     r'Crawler|MachineGun|Projectile|EnemyContact|Contact|Currency|Gem|Smoke|Damage|Flash|'
     r'Corpse|Debris|Combo|Camera|Consumables|RunSession|retireMainRoom|advance|Alarm|'
     r'Activation|Laser|Shop|Box|Breakable|amigaResolveTerrain|Wall|Landing|GameplayWorld::step'),
]
CAUSE_PATTERNS = [(name, re.compile(pattern)) for name, pattern in CAUSES]
TRANSPARENT = re.compile(r'^(__u?(mul|div|mod)si3|memset|memcpy|downwellMemoryCopy|'
                         r'operator\(\).*)$')
IDLE_WORK = re.compile(r'serviceSpatialIndex|RunnerSpatial|\{lambda')


def cause(names):
    """Owner of one logical stack for the --causes table."""
    if 'AmigaPlatform::waitVblank' in names:
        below = names[names.index('AmigaPlatform::waitVblank') + 1:]
        if not any(IDLE_WORK.search(name) for name in below):
            return 'wait for vertical blank'
    for name in reversed(names):
        if TRANSPARENT.match(name):
            continue
        for owner, pattern in CAUSE_PATTERNS:
            if pattern.search(name):
                return owner
    return 'frame loop and other'


def parse_range(text, first, last):
    if text in (None, 'all'):
        return first, last
    if '-' in text:
        a, b = text.split('-', 1)
        return int(a), int(b)
    return int(text), int(text)


def attribute(path, start_frame, calls, mark, stacks, frame_cycles, entries):
    """Rebuild call stacks for one capture; accumulate cycles per raw stack.

    Returns the number of fields and the last (incomplete) frame number."""
    user, supervisor = [], []  # frames: (call-site pc, sp after the push)
    prev_user = prev_super = None  # (pc, sp)
    # FS-UAE starts recording at the vertical blank after the debugger stop,
    # i.e. after start_frame's mark and inside its waitVblank. Instructions
    # before the first recorded mark are start_frame's work (only its pre-VBL
    # wait/idle service is missing); the first recorded mark is start_frame+1.
    frame = start_frame
    fields = 0
    for field, pc, cost, sp, sr in records(path):
        fields = field + 1
        if pc == mark:
            frame += 1
        if pc == IRQ_PC or sr & 0x2000:
            if prev_super is None:
                supervisor.clear()
            elif pc is not None and pc != IRQ_PC:
                while supervisor and sp > supervisor[-1][1]:
                    supervisor.pop()
                ppc, psp = prev_super
                if ppc in calls and sp == psp - 4 and pc != calls[ppc]:
                    supervisor.append((ppc, sp))
            prev_super = (pc, sp) if pc is not None else None
            key = tuple(p for p, _ in user) + ('IRQ',) + tuple(p for p, _ in supervisor) + (pc,)
        else:
            prev_super = None
            supervisor.clear()
            while user and sp > user[-1][1]:
                user.pop()
            if prev_user is not None and pc is not None:
                ppc, psp = prev_user
                if ppc in calls and sp == psp - 4 and pc != calls[ppc]:
                    user.append((ppc, sp))
                    entries[(frame, pc)] += 1
            if pc is not None:
                prev_user = (pc, sp)
            key = tuple(p for p, _ in user) + (pc,)
        stacks[(frame,) + key] += cost
        frame_cycles[frame] += cost
    return fields, frame


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('trace_dir')
    parser.add_argument('--top', type=int, default=60)
    parser.add_argument('--tree-min', type=float, default=0.5,
                        help='hide tree nodes below this share (percent)')
    parser.add_argument('--tree-depth', type=int, default=40)
    parser.add_argument('--focus', action='append', default=[],
                        help='also print callers/callees of this logical frame')
    parser.add_argument('--frames', default=None,
                        help='restrict attribution to game frames N or N-M')
    parser.add_argument('--min-fields', type=float, default=0.0,
                        help='restrict attribution to frames at least this long')
    parser.add_argument('--collapsed', default=None,
                        help='write folded stacks (flamegraph.pl/speedscope input)')
    parser.add_argument('--lines', default=None,
                        help='per-source-line self cycles for this logical frame')
    parser.add_argument('--breakdown-depth', type=int, default=1,
                        help='per-frame columns come from this depth below runDownwell')
    parser.add_argument('--causes', action='store_true',
                        help='also charge each stack to a cost owner (see CAUSES)')
    args = parser.parse_args()

    directory = args.trace_dir
    elf = os.path.join(directory, 'Downwell.elf')
    traces = sorted((int(m.group(1)), os.path.join(directory, name))
                    for name in os.listdir(directory)
                    for m in [re.fullmatch(r'trace-(\d+)\.bin', name)] if m)
    if not traces:
        sys.exit('no trace-<frame>.bin captures in ' + directory)
    symbols, calls, text = load_code(elf)
    by_name = {v: k for k, v in symbols.items()}
    mark = by_name.get('downwellTraceFrameMark')
    if mark is None:
        sys.exit('ELF lacks downwellTraceFrameMark (not a TRACE_FRAMES build)')

    stacks = collections.Counter()  # (frame, call sites..., leaf) -> cycles
    entries = collections.Counter()  # (frame, callee entry pc) -> user-mode calls
    frame_cycles = collections.Counter()
    last_frames = set()  # each cut off by the end of its capture
    total_fields = 0
    for start_frame, path in traces:
        fields, last = attribute(path, start_frame, calls, mark, stacks,
                                 frame_cycles, entries)
        total_fields += fields
        last_frames.add(last)

    complete = [f for f in sorted(frame_cycles) if f not in last_frames]
    if not complete:
        sys.exit('captures contain no complete game frame')
    lo, hi = parse_range(args.frames, min(complete), max(complete))
    chosen = {f for f in complete if lo <= f <= hi and
              frame_cycles[f] >= args.min_fields * FIELD_CYCLES}
    sym = Symbolizer(elf, symbols)
    sym.resolve(p for key in stacks for p in key[1:] if isinstance(p, int))

    def logical(key):
        names = []
        for p in key[1:-1]:
            names.extend(['[interrupt]'] if p == 'IRQ' else sym.frames(p))
        names.extend(sym.frames(key[-1]))
        out = []
        for name in names:
            if not out or out[-1] != name:
                out.append(name)
        return tuple(out)

    folded = collections.Counter()
    per_frame = collections.defaultdict(collections.Counter)
    leaf_lines = collections.Counter()
    selected = 0
    for key, cycles in stacks.items():
        names = logical(key)
        root = names.index('runDownwell') if 'runDownwell' in names else -1
        depth = root + args.breakdown_depth
        bucket = names[depth] if 0 <= root and depth < len(names) else '(self)'
        if bucket == 'AmigaPlatform::waitVblank' and depth + 1 < len(names):
            bucket = 'waitVblank idle work'  # spin time stays under waitVblank
        per_frame[key[0]][bucket] += cycles
        if key[0] not in chosen:
            continue
        folded[names] += cycles
        selected += cycles
        if args.lines and names[-1] == args.lines:
            leaf_lines[sym.line(key[-1])] += cycles

    def ms(c):
        return c / FIELD_CYCLES * 20.0

    print(f'# Downwell instruction trace: {directory}')
    print(f'{len(traces)} capture(s), {total_fields} PAL fields; '
          f'{len(complete)} complete game frames; selected {len(chosen)} '
          f'frames, {selected:,} cycles = {ms(selected) / max(1, len(chosen)):.1f} ms/frame\n')
    print('## Game frames (frame mark to next mark; ms = cycles / 142102 x 20)')
    print('| frame | ms | fields | largest parts below runDownwell (ms) |')
    print('| ---: | ---: | ---: | --- |')
    for f in complete:
        c = frame_cycles[f]
        parts = ', '.join(f'{name} {ms(v):.1f}' for name, v in per_frame[f].most_common(6))
        print(f'| {f} | {ms(c):.1f} | {c / FIELD_CYCLES:.2f} | {parts} |')

    inclusive, exclusive = collections.Counter(), collections.Counter()
    children = collections.defaultdict(collections.Counter)
    parents = collections.defaultdict(collections.Counter)
    for names, cycles in folded.items():
        exclusive[names[-1]] += cycles
        seen = set()
        for i, name in enumerate(names):
            if name not in seen:
                inclusive[name] += cycles
                seen.add(name)
            if i + 1 < len(names):
                children[name][names[i + 1]] += cycles
            if i > 0:
                parents[name][names[i - 1]] += cycles

    def pct(c):
        return 100.0 * c / selected if selected else 0.0

    count = max(1, len(chosen))
    print(f'\n## Self time by logical frame (inlined functions separate), top {args.top}')
    print('| self % | ms/frame | function |')
    print('| ---: | ---: | --- |')
    for name, c in exclusive.most_common(args.top):
        print(f'| {pct(c):.2f} | {ms(c) / count:.2f} | `{name}` |')

    print(f'\n## Inclusive time, top {args.top}')
    print('| incl % | ms/frame | self % | function |')
    print('| ---: | ---: | ---: | --- |')
    for name, c in inclusive.most_common(args.top):
        print(f'| {pct(c):.2f} | {ms(c) / count:.2f} | {pct(exclusive[name]):.2f} | `{name}` |')

    # What the CPU is doing, by instruction class, over selected frames.
    classes = collections.Counter()
    for key, cycles in stacks.items():
        if key[0] not in chosen:
            continue
        pc = key[-1]
        if not isinstance(pc, int) or pc == IRQ_PC:
            classes['outside program / exception'] += cycles
            continue
        mnemonic, operands = text.get(pc, ('?', ''))
        name = sym.symbol(pc)
        if 'waitVblank' in name and 'service' not in name:
            classes['vblank wait loop'] += cycles
            continue
        classes[instruction_class(mnemonic, operands, re.sub(r'\(.*', '', name))] += cycles
    print('\n## Cycles by instruction class (leaf instruction)')
    print('| share % | ms/frame | class |')
    print('| ---: | ---: | --- |')
    for name, c in classes.most_common():
        print(f'| {pct(c):.2f} | {ms(c) / count:.2f} | {name} |')

    if args.causes:
        owners = collections.Counter()
        for names, cycles in folded.items():
            owners[cause(names)] += cycles
        print('\n## Cycles by cost owner (innermost recognised frame; helpers to callers)')
        print('| share % | ms/frame | owner |')
        print('| ---: | ---: | --- |')
        for name, c in owners.most_common():
            print(f'| {pct(c):.2f} | {ms(c) / count:.2f} | {name} |')

    # Out-of-line functions: inclusive cycles by ELF symbol and entry counts.
    symbol_inclusive = collections.Counter()
    for key, cycles in stacks.items():
        if key[0] not in chosen:
            continue
        seen = set()
        for p in key[1:]:
            if isinstance(p, int) and p != IRQ_PC:
                seen.add(sym.symbol(p))
        for name in seen:
            symbol_inclusive[name] += cycles
    symbol_calls = collections.Counter()
    for (f, pc), n in entries.items():
        if f in chosen:
            symbol_calls[sym.symbol(pc)] += n
    print(f'\n## Out-of-line functions by inclusive time, with call counts, top {args.top}')
    print('| incl ms/frame | calls/frame | cycles/call | function |')
    print('| ---: | ---: | ---: | --- |')
    for name, c in symbol_inclusive.most_common(args.top):
        n = symbol_calls[name]
        per = f'{c / n:,.0f}' if n else '-'
        print(f'| {ms(c) / count:.2f} | {n / count:.1f} | {per} | `{short(name)}` |')

    tree = {}
    for names, cycles in folded.items():
        node = tree
        for name in names[:args.tree_depth]:
            entry = node.setdefault(name, [0, {}])
            entry[0] += cycles
            node = entry[1]
    print(f'\n## Call tree (inclusive % and ms/frame, nodes >= {args.tree_min}%)')
    print('```')

    def walk(node, depth):
        for name, (c, sub) in sorted(node.items(), key=lambda kv: -kv[1][0]):
            if pct(c) < args.tree_min:
                continue
            print(f'{pct(c):6.2f}% {ms(c) / count:6.2f} {"  " * depth}{name}')
            walk(sub, depth + 1)
    walk(tree, 0)
    print('```')

    for want in args.focus:
        print(f'\n## Focus `{want}`: inclusive {pct(inclusive[want]):.2f}%, '
              f'self {pct(exclusive[want]):.2f}%')
        print('Callers:')
        for name, c in parents[want].most_common(15):
            print(f'  {pct(c):6.2f}%  {name}')
        print('Callees:')
        for name, c in children[want].most_common(25):
            print(f'  {pct(c):6.2f}%  {name}')

    if args.lines:
        print(f'\n## Self cycles of `{args.lines}` by source line')
        own = sum(leaf_lines.values())
        for line, c in leaf_lines.most_common(40):
            print(f'  {100.0 * c / max(1, own):6.2f}%  {pct(c):6.2f}% of all  {line}')

    if args.collapsed:
        with open(args.collapsed, 'w') as out:
            for names, cycles in folded.items():
                out.write(';'.join(n.replace(';', ',').replace(' ', '_') for n in names) +
                          f' {cycles}\n')


if __name__ == '__main__':
    main()
