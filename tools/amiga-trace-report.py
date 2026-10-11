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
builds), using the marker's recorded argument, not a count of marker entries.
Recording may include the requested marker or begin after it. Conservatively,
both the requested first and final frames are excluded. Cycles are 68000 clocks; a
PAL field is 142,102 of them.

  python3 tools/amiga-trace-report.py tmp/trace/run [--frames=N|N-M]
      [--min-fields=F] [--top=60] [--tree-min=0.5] [--focus=name]
      [--lines=name] [--collapsed=FILE]
"""
import argparse
import bisect
import collections
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys

FIELD_CYCLES = 142102
IRQ_PC = 0x7fffffff
BIN = os.path.join(os.environ.get('AMIGA_TOOLCHAIN') or os.path.expanduser('~/.local/opt'), 'bin')


def tool(name):
    return shutil.which(name) or os.path.join(BIN, name)


class CallSites(dict):
    """Call successors plus the validated frame-marker argument reader."""
    marker = None


def marker_reader(text, mark):
    addresses = sorted(text)
    following = addresses[addresses.index(mark) + 1:][:1]
    mnemonic, operands = text[mark]
    match = re.fullmatch(r'4\(sp\),d([0-7])', operands.replace(' ', ''))
    if mnemonic != 'move.l' or not match or not following or text[following[0]][0] != 'rts':
        raise ValueError('unsupported frame marker ABI; cannot identify game frames safely')
    return following[0], int(match.group(1))


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
    calls, text = CallSites(), {}
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
    for address, name in symbols.items():
        if name == 'downwellTraceFrameMark':
            calls.marker = marker_reader(text, address)
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


def field_cycle_carry(accounted, elapsed, first_cost, pending=0):
    """Validate whole-instruction accounting across one PAL boundary.

    DMA priority can stall an instruction across VBlank. FS-UAE puts that
    instruction and its entire cost in the following field. Keep those cycles
    attached to the instruction; accept a deficit only when the next field's
    first instruction covers it and the paired totals reconcile.
    """
    tolerance = max(100, elapsed // 1000)
    delta = accounted - elapsed
    if pending > tolerance:
        if first_cost < pending or abs(delta - pending) > tolerance:
            raise ValueError('unreconciled instruction crossing a field boundary')
        return 0
    if delta > tolerance:
        # A deficit just below tolerance must not be forgotten: its matching
        # surplus can land just above it. Require the same instruction and
        # paired-total evidence as a larger boundary crossing.
        if not pending or first_cost < pending or abs(delta - pending) > tolerance:
            raise ValueError('unexplained field cycle mismatch')
        return 0
    if -65535 <= delta < 0:
        return -delta
    if delta >= 0:
        return 0
    raise ValueError('unexplained field cycle mismatch')


def records(path, include_registers=False):
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
    pending_cycles = 0
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
        first_cost = None
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
            if first_cost is None:
                first_cost = cost
            regs = unpack_regs(data, offset)
            offset += 68
            accounted += cost
            item = (field, pc, cost, regs[15], regs[16])
            yield item + (regs,) if include_registers else item
        if offset != end:
            raise ValueError('instruction record boundary mismatch')
        try:
            pending_cycles = field_cycle_carry(accounted, cycles, first_cost or 0, pending_cycles)
        except ValueError as error:
            raise ValueError(f'field {field}: {accounted} cycles traced, {cycles} elapsed: {error}') from error
        size, _kind = word(), word()
        offset += size  # screenshot
    if offset != len(data):
        raise ValueError('unexpected trailing trace data')
    if pending_cycles and pending_cycles > max(100, cycles // 1000):
        raise ValueError('capture ends before a field cycle deficit can be reconciled')


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
    # This name is an attribution key, not just a label. Never truncate it:
    # long owner template arguments can otherwise hide the method entirely,
    # merging unrelated methods and erasing inline stack frames.
    return name


def display_name(name):
    """Elide template arguments only for display; retain the member name."""
    result = []
    depth = 0
    for char in name:
        if char == '<':
            if depth == 0:
                result.append('<...>')
            depth += 1
        elif char == '>' and depth:
            depth -= 1
        elif not depth:
            result.append(char)
    return ''.join(result) if not depth else name


def matching_names(names, requested):
    """Exact logical name wins; otherwise a substring selects specializations."""
    if requested in names:
        return [requested]
    return sorted(name for name in names if requested in name)


GENERIC = re.compile(r'^(operator\(\)|operator\(\)<.*>|operator\(\)\(.*\))$')


# Cost owners for --causes, checked from the innermost frame outwards; the
# first frame matching any owner decides. Helpers are charged to callers.
# "Runner bookkeeping" is the emulated GameMaker runner state: lifetimes,
# room/layer/object lists, dirty queues, bounds caches, publication, checks.
CAUSES = [
    ('R-tree upkeep', r'RunnerSpatial(Index|Journal|Split|NodeStorage)|serviceSpatialIndex'),
    ('software float emulation', r'MotionBinary64|RunnerImageValue|[mM]ultiplyBinary64|'
     r'[dD]ivideBinary64|roundGameMaker'),
    ('runner bookkeeping', r'RunnerRoomMembership|RunnerObservedRoomMembership|RunnerRun(?:Room)?Membership|RunnerLayer|GameplayRoomLayers|'
     r'RunnerObjectMembership|RunnerObservedRegistration|RunnerSpatialInstances|RunnerSpatialLifetimes|RunnerSpatialDirty|RunnerSpatialBounds|'
     r'RunnerSpatialPosition|RunnerStepSnapshot|RunnerObjectFamilies|RecordLifecycle|'
     r'GameplayCollisionOwner::(Binding|valid|refresh|reconcile|combatAlarm|dispatchCaptured|'
     r'cleanupDestroyed|prepareDraw|resolves)|GameplayCollisionView|Prepared(Terrain|UnitSprite)'
     r'\w*Bounds|PreparedSpriteGeometry|GameplayTerrainInstances(?:Scalar|Runs)?::(append|createProperties|spatial)|TerrainOwnershipRuns|'
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
    ('HUD redraws', r'AmmoHud|redrawAmmo|redrawStreak|redrawCurrency|redrawHealth|AmigaHudText|'
     r'AmmoGauge|amigaRenderer\w*Hud|amigaHudStage|AmmoDivider'),
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


def bare(name):
    """Name without template arguments or parameter lists: owner patterns
    must not match types that merely appear in a signature."""
    previous = None
    while previous != name:
        previous = name
        name = re.sub(r'<[^<>]*>|\([^()]*\)', '', name)
    return name


def cause(names):
    """Owner of one logical stack for the --causes table."""
    if 'AmigaPlatform::waitVblank' in names:
        below = names[names.index('AmigaPlatform::waitVblank') + 1:]
        if not any(IDLE_WORK.search(name) for name in below):
            return 'wait for vertical blank'
    for name in reversed(names):
        if TRANSPARENT.match(name):
            continue
        name = bare(name)
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


def frame_markers(path, start_frame, calls, mark):
    """Read actual arguments, whether capture includes the requested marker or not."""
    if getattr(calls, 'marker', None) is None:
        raise ValueError('missing validated frame marker ABI')
    ready, register = calls.marker
    pending = None
    previous = None
    result = {}
    for index, (_, pc, _, sp, sr, regs) in enumerate(records(path, include_registers=True)):
        if sr & 0x2000:
            continue
        if pc == mark:
            if pending is not None:
                raise ValueError('nested frame markers')
            pending = index, sp
        elif pc == ready and pending is not None:
            if pending[1] != sp:
                raise ValueError('frame marker stack changed')
            frame = regs[register]
            if ((previous is None and frame not in (start_frame, start_frame + 1)) or
                    (previous is not None and frame != previous + 1)):
                raise ValueError(f'unexpected frame marker argument {frame} after {previous}')
            result[pending[0]] = frame
            previous, pending = frame, None
    if pending is not None:
        raise ValueError('capture ends inside frame marker')
    return result


def attribute(path, start_frame, calls, mark, stacks, frame_cycles, entries):
    """Rebuild call stacks for one capture; accumulate cycles per raw stack.

    Returns the number of fields and the last (incomplete) frame number."""
    user, supervisor = [], []  # frames: (call-site pc, sp after the push)
    prev_user = prev_super = None  # (pc, sp)
    # The VBL capture may include the requested marker or start after it.
    # Decode its argument instead of assuming the next marker is start+1.
    markers = frame_markers(path, start_frame, calls, mark)
    frame = start_frame
    fields = 0
    for index, (field, pc, cost, sp, sr) in enumerate(records(path)):
        fields = field + 1
        if index in markers:
            frame = markers[index]
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


def complete_frame_numbers(frame_cycles, partial_frames):
    return [f for f in sorted(frame_cycles) if f not in partial_frames]


def frame_summary(frame_cycles, partial_frames, elf_hash):
    """Keep exact cycles after disposable instruction traces are removed."""
    return {'schema': 'downwell-frame-cycles/v1', 'elf_sha256': elf_hash,
            'pal_field_cycles': FIELD_CYCLES,
            'frames': [{'frame': f, 'cycles': frame_cycles[f]}
                       for f in complete_frame_numbers(frame_cycles, partial_frames)],
            'partial_frames': sorted(partial_frames)}


def file_hash(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def merge_frame_cycles(target, partial, seen_complete, source, start, last):
    complete = set(source) - {start, last}
    overlap = complete & seen_complete
    if overlap:
        raise ValueError(f'overlapping complete-frame captures: {sorted(overlap)}')
    seen_complete.update(complete)
    target.update(source)
    partial.update((start, last))


def select_frames(frame_cycles, partial_frames, frames=None, min_fields=0.0, partial_frame=None):
    if partial_frame is not None:
        if frames is not None or min_fields:
            raise ValueError('partial attribution cannot be combined with whole-frame filters')
        if partial_frame not in partial_frames or partial_frame not in frame_cycles:
            raise ValueError('requested frame is not a captured partial frame')
        return {partial_frame}
    complete = complete_frame_numbers(frame_cycles, partial_frames)
    if not complete:
        raise ValueError('captures contain no complete game frame')
    lo, hi = parse_range(frames, min(complete), max(complete))
    chosen = {f for f in complete if lo <= f <= hi and frame_cycles[f] >= min_fields * FIELD_CYCLES}
    if not chosen:
        raise ValueError('no complete frames match the selection; capture before the requested frame')
    return chosen


def source_line_report(lines, selected_cycles, top):
    """Report coverage explicitly: a truncated ranking cannot prove absence."""
    own = sum(lines.values())
    shown = lines.most_common(top)
    omitted = own - sum(cycles for _, cycles in shown)
    yield (f'{len(shown)}/{len(lines)} source locations shown; '
           f'{own:,} self cycles total; {omitted:,} self cycles omitted. '
           'Called functions are excluded.')
    for line, cycles in shown:
        yield (f'  {100.0 * cycles / max(1, own):6.2f}%  '
               f'{100.0 * cycles / max(1, selected_cycles):6.2f}% of all  {line}')


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('trace_dir')
    parser.add_argument('--top', type=int, default=60)
    parser.add_argument('--tree-min', type=float, default=0.5,
                        help='hide tree nodes below this share (percent)')
    parser.add_argument('--tree-depth', type=int, default=40)
    parser.add_argument('--focus', action='append', default=[],
                        help='print callers/callees for an exact logical name or matching substring')
    parser.add_argument('--frames', default=None,
                        help='restrict attribution and the frame table to game frames N or N-M')
    parser.add_argument('--frame-summary', default=None,
                        help='save exact complete-frame cycles and ELF hash for matched comparisons')
    parser.add_argument('--partial-frame', type=int, default=None,
                        help='attribute only a captured slice of N; never a frame-duration measurement')
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
    partial_frames = set()  # recording starts/ends inside a game frame
    seen_complete = set()
    total_fields = 0
    for start_frame, path in traces:
        trace_cycles = collections.Counter()
        fields, last = attribute(path, start_frame, calls, mark, stacks,
                                 trace_cycles, entries)
        total_fields += fields
        try:
            merge_frame_cycles(frame_cycles, partial_frames, seen_complete,
                               trace_cycles, start_frame, last)
        except ValueError as error:
            sys.exit(str(error))

    complete = complete_frame_numbers(frame_cycles, partial_frames)
    try:
        chosen = select_frames(frame_cycles, partial_frames, args.frames, args.min_fields, args.partial_frame)
    except ValueError as error:
        sys.exit(str(error))
    unit = 'captured slice' if args.partial_frame is not None else 'frame'
    if args.frame_summary:
        summary = frame_summary(frame_cycles, partial_frames, file_hash(elf))
        summary['tool'] = 'amiga-trace-report/frame-summary-v1'
        summary['tool_sha256'] = file_hash(__file__)
        summary['inputs'] = {os.path.basename(path): file_hash(path) for _, path in traces}
        with open(args.frame_summary, 'w') as target:
            json.dump(summary, target, indent=2)
            target.write('\n')
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
        if args.lines and args.lines in names[-1]:
            leaf_lines[sym.line(key[-1])] += cycles

    def ms(c):
        return c / FIELD_CYCLES * 20.0

    print(f'# Downwell instruction trace: {directory}')
    if args.partial_frame is not None:
        print('PARTIAL ATTRIBUTION ONLY: not a complete frame, not a frame-budget measurement.\n')
    print(f'{len(traces)} capture(s), {total_fields} PAL fields; '
          f'{len(complete)} complete game frames; selected {len(chosen)} '
          f'{unit}(s), {selected:,} cycles = {ms(selected) / max(1, len(chosen)):.1f} ms/{unit}\n')
    print('## Captured partial frame slice' if args.partial_frame is not None else
          '## Game frames (frame mark to next mark; ms = cycles / 142102 x 20)')
    print('| frame | ms | fields | largest parts below runDownwell (ms) |')
    print('| ---: | ---: | ---: | --- |')
    for f in sorted(chosen):
        c = frame_cycles[f]
        parts = ', '.join(f'{display_name(name)} {ms(v):.1f}' for name, v in per_frame[f].most_common(6))
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
    print(f'| self % | ms/{unit} | function |')
    print('| ---: | ---: | --- |')
    for name, c in exclusive.most_common(args.top):
        print(f'| {pct(c):.2f} | {ms(c) / count:.2f} | `{display_name(name)}` |')

    print(f'\n## Inclusive time, top {args.top}')
    print(f'| incl % | ms/{unit} | self % | function |')
    print('| ---: | ---: | ---: | --- |')
    for name, c in inclusive.most_common(args.top):
        print(f'| {pct(c):.2f} | {ms(c) / count:.2f} | {pct(exclusive[name]):.2f} | `{display_name(name)}` |')

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
    print(f'| share % | ms/{unit} | class |')
    print('| ---: | ---: | --- |')
    for name, c in classes.most_common():
        print(f'| {pct(c):.2f} | {ms(c) / count:.2f} | {name} |')

    if args.causes:
        owners = collections.Counter()
        for names, cycles in folded.items():
            owners[cause(names)] += cycles
        print('\n## Cycles by cost owner (innermost recognised frame; helpers to callers)')
        print(f'| share % | ms/{unit} | owner |')
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
    print(f'| incl ms/{unit} | calls/{unit} | cycles/call | function |')
    print('| ---: | ---: | ---: | --- |')
    for name, c in symbol_inclusive.most_common(args.top):
        n = symbol_calls[name]
        per = f'{c / n:,.0f}' if n else '-'
        print(f'| {ms(c) / count:.2f} | {n / count:.1f} | {per} | `{display_name(short(name))}` |')

    tree = {}
    for names, cycles in folded.items():
        node = tree
        for name in names[:args.tree_depth]:
            entry = node.setdefault(name, [0, {}])
            entry[0] += cycles
            node = entry[1]
    print(f'\n## Call tree (inclusive % and ms/{unit}, nodes >= {args.tree_min}%)')
    print('```')

    def walk(node, depth):
        for name, (c, sub) in sorted(node.items(), key=lambda kv: -kv[1][0]):
            if pct(c) < args.tree_min:
                continue
            print(f'{pct(c):6.2f}% {ms(c) / count:6.2f} {"  " * depth}{display_name(name)}')
            walk(sub, depth + 1)
    walk(tree, 0)
    print('```')

    for requested in args.focus:
        matches = matching_names(inclusive, requested)
        if not matches:
            print(f'\n## Focus `{requested}`: no matching logical frame')
        for want in matches:
            print(f'\n## Focus `{want}`: inclusive {pct(inclusive[want]):.2f}%, '
                  f'self {pct(exclusive[want]):.2f}%')
            print('Callers:')
            for name, c in parents[want].most_common(15):
                print(f'  {pct(c):6.2f}%  {display_name(name)}')
            print('Callees:')
            for name, c in children[want].most_common(25):
                print(f'  {pct(c):6.2f}%  {display_name(name)}')

    if args.lines:
        print(f'\n## Self cycles of `{args.lines}` by source line')
        for line in source_line_report(leaf_lines, selected, args.top):
            print(line)

    if args.collapsed:
        with open(args.collapsed, 'w') as out:
            for names, cycles in folded.items():
                out.write(';'.join(n.replace(';', ',').replace(' ', '_') for n in names) +
                          f' {cycles}\n')


if __name__ == '__main__':
    main()
