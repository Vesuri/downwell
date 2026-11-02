"""Reviewed native art admission, independent of what exporters can produce.

This is the current bootstrap/gameplay bank boundary, not a claim that the
gameplay bank has already been split by level. CPU and DMA gameplay art are
leased only across gameplay scenes.
IDs above 62000 are explicit host-prepared variants, not source resource IDs.
Do not infer admission from SELECTIONS or from the output of an exporter.
"""
VERSION = 'amiga-scene-admission/v1'

# PlayerMotion::spriteId(false): selection retains the living entry pose.
# Admit its offline DMA records explicitly, never the gameplay catalogue.
UPGRADE_PLAYER_IDS = frozenset({1, 3, 15, 27, 31, 37})

GAME_IDS = frozenset({
    0, 1, 3, 12, 15, 27, 31, 37, 51, 76, 84, 85, 91, 92, 96, 101, 102,
    115, 116, 117, 127, 133, 134, 139, 143, 147, 149, 150, 151, 152,
    153, 154, 157, 162, 163, 164, 166, 168, 170, 172, 173, 175, 176,
    177, 178, 181, 182, 183, 184, 185, 186, 187, 188, 205, 206, 207,
    208, 278, 458, 459, 460, 471, 480, 481, 482, 485, 486, 487, 496,
    497, 550, 551, 568, 569, 570, 571, 572, 602, 603, 605, 702, 703, 704, 707,
    711, 712, 83, 118,
    # W1.4 guns: modules, letters, casing, laser/spread shots, muzzle,
    # pickup effect and notice icon.
    111, 140, 141, 457, 465, 466, 472, 473, 601, 713,
    # Pre-mirrored planar forms.
    62000, 62001, 62003, 62012, 62015, 62027, 62031, 62037, 62096,
    62127, 62139, 62153, 62154, 62157, 62162, 62163, 62164, 62166,
    62168, 62170, 62172, 62173, 62175, 62176, 62177, 62178, 62181, 62182, 62183,
    62184, 62185, 62186, 62187, 62188, 62205, 62206, 62207, 62208,
    62278, 62460, 62471, 62550, 62551, 62568, 62569, 62702, 62703,
    # DMA sprite streams, including admitted effect/projectile variants.
    63000, 63001, 63003, 63012, 63015, 63027, 63031, 63037, 63084,
    63085, 63096, 63101, 63133, 63134, 63153, 63154, 63157, 63162,
    63163, 63164, 63166, 63168, 63170, 63173, 63175, 63176, 63177, 63178, 63181,
    63182, 63183, 63184, 63185, 63186, 63187, 63188, 63205, 63207,
    63208, 63278, 63458, 63459, 63460, 63485, 63486, 63487, 63496,
    63497, 63602, 63603, 63702, 63703, 63707,
    # World dither edge and native text assets.
    65006, 65100, 65101, 65102, 65103, 65104, 65105, 65106, 65107,
    65108, 65109, 65110,
    # W1.4 bordered gun-name letters and the notice's "+".
    65111, *(65120 + ord(c) - ord('A') for c in 'ABCEGHILMNOPRSTUY'),
})

# Ceilings are reviewed inputs, never inferred from generated payload sizes.
# DMA gameplay currently uses 115806 bytes; its ceiling leaves 2754 bytes
# growth, not permission to consume all of the measured free Chip RAM. Only
# the full-catalogue reference oracles load it; production gameplay leases
# the cavern banks below.
BANK_BUDGETS = {'boot': 24 * 1024, 'front': 96 * 1024,
                'game': 264 * 1024, 'game-planes': 124 * 1024}

# Explicit cavern roots, independent of future additions to GAME_IDS. Derived
# mirror/DMA forms are admitted only for these roots. Later-area skeleton/bone
# and wood/default debris art must not become cavern residents. builderCavern
# and spawnerSet1 define its enemy roots; breakablesDebris Create selects 115
# for area 1 (116 for area 2, 91 for area 4). Keep common effects/companions,
# merchant art and generated scenery: these are reachable without area change.
CAVERN_ROOTS = frozenset({
    0, 1, 3, 12, 15, 27, 31, 37, 51, 76, 84, 85, 92, 96, 101, 102,
    115, 117, 127, 133, 134, 139, 143, 147, 149, 150, 151, 152,
    153, 154, 157, 162, 163, 164, 166, 168, 170, 172, 173, 175, 176,
    177, 178, 181, 182, 183, 184, 185, 186, 187, 188, 278, 458, 459,
    471, 480, 481, 482, 485, 486, 487, 496, 497, 550, 551, 568, 569,
    570, 571, 572, 602, 603, 605, 702, 703, 704, 707, 711, 712, 83, 118,
    111, 140, 141, 457, 465, 466, 472, 473, 601, 713,
})
CAVERN_IDS = frozenset(
    {root + offset for root in CAVERN_ROOTS for offset in (0, 62000, 63000)} |
    {65006, 65100, 65101, 65102, 65103, 65104, 65105, 65106, 65107,
     65108, 65109, 65110, 65111} |
    {65120 + ord(c) - ord('A') for c in 'ABCEGHILMNOPRSTUY'}) & GAME_IDS
# W1.3 room effects (gem chunk, sparkles, flash, red splash turns, fire,
# small smoke) raised the measured DMA atlas from 106,574 to 109,410 bytes.
# W1.4 gun art (modules, letters, casing, laser and spread shots, the laser
# muzzle's 270 page, the cropped pickup effect, notice icon and glyphs) adds
# 17,762 bytes: 228,948 -> 246,710 CPU, and 8,334 Chip plane bytes:
# 109,410 -> 117,744. Planar BOBs only, no DMA streams.
BANK_BUDGETS.update({'cavern': 248 * 1024, 'cavern-planes': 116 * 1024})


def require_budget(bank, size):
    if bank not in BANK_BUDGETS:
        raise ValueError(f'undeclared scene bank: {bank}')
    if size <= 0 or size > BANK_BUDGETS[bank]:
        raise ValueError(f'{bank}: {size} bytes exceeds scene admission budget '
                         f'{BANK_BUDGETS[bank]}')
