"""Reviewed native art admission, independent of what exporters can produce.

This is the current bootstrap/gameplay bank boundary, not a claim that the
gameplay bank has already been split by level or unloaded during startup.
IDs above 62000 are explicit host-prepared variants, not source resource IDs.
Do not infer admission from SELECTIONS or from the output of an exporter.
"""
VERSION = 'amiga-scene-admission/v1'

GAME_IDS = frozenset({
    0, 1, 3, 12, 15, 27, 31, 37, 51, 76, 84, 85, 91, 92, 96, 101, 102,
    115, 116, 117, 127, 133, 134, 139, 143, 147, 149, 150, 151, 152,
    153, 154, 157, 162, 163, 164, 166, 168, 170, 172, 173, 175, 176,
    177, 178, 181, 182, 183, 184, 185, 186, 187, 188, 205, 206, 207,
    208, 278, 458, 459, 460, 471, 480, 481, 482, 485, 486, 487, 496,
    497, 550, 551, 568, 569, 602, 603, 605, 702, 703, 707,
    # Pre-mirrored planar forms.
    62000, 62001, 62003, 62012, 62015, 62027, 62031, 62037, 62096,
    62127, 62139, 62153, 62154, 62157, 62162, 62163, 62164, 62166,
    62168, 62170, 62172, 62173, 62177, 62178, 62181, 62182, 62183,
    62184, 62185, 62186, 62187, 62188, 62205, 62206, 62207, 62208,
    62278, 62460, 62471, 62550, 62551, 62568, 62569, 62702, 62703,
    # DMA sprite streams, including admitted effect/projectile variants.
    63000, 63001, 63003, 63012, 63015, 63027, 63031, 63037, 63084,
    63085, 63096, 63101, 63133, 63134, 63153, 63154, 63157, 63162,
    63163, 63164, 63166, 63168, 63170, 63173, 63177, 63178, 63181,
    63182, 63183, 63184, 63185, 63186, 63187, 63188, 63205, 63207,
    63208, 63278, 63458, 63459, 63460, 63485, 63486, 63487, 63496,
    63497, 63602, 63603, 63702, 63703, 63707,
    # World dither edge and native text assets.
    65006, 65100, 65101, 65102, 65103, 65104, 65105, 65106, 65107,
    65108, 65109, 65110,
})

# Ceilings are reviewed inputs, never inferred from generated payload sizes.
# DMA gameplay currently uses 112970 bytes; its ceiling leaves 1718 bytes
# growth, not permission to consume all of the measured free Chip RAM.
BANK_BUDGETS = {'boot': 24 * 1024, 'front': 96 * 1024,
                'game': 240 * 1024, 'game-planes': 112 * 1024}


def require_budget(bank, size):
    if bank not in BANK_BUDGETS:
        raise ValueError(f'undeclared scene bank: {bank}')
    if size <= 0 or size > BANK_BUDGETS[bank]:
        raise ValueError(f'{bank}: {size} bytes exceeds scene admission budget '
                         f'{BANK_BUDGETS[bank]}')
