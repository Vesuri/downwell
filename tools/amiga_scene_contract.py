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
    # W1.9 Hot Casing's sprLeakBullet.
    467,
    # W1.6 corpses (bat, snake, snail, crawler, turtle; frog 172 above) and
    # the bullet-immune spark.
    112, 158, 165, 179, 281, 337,
    # W1.6 jump and landing dust.
    94, 95,
    # W1.7 gem bar caps, GEM HIGH / gem-sick motion, combo digits and icons,
    # COMBO REWARD! and COMBO!.
    393, 394, 395, 397, 400, 403, 404, 405, 406,
    # W1.7 upgrade icons (frame 1), drawn on the HUD layer.
    506, 507, 508, 509, 510, 511, 512, 516, 518, 520, 521, 522, 523, 524,
    525, 528, 529, 530, 531, 532, 533, 534, 535,
    # Pre-mirrored planar forms.
    62000, 62001, 62003, 62012, 62015, 62027, 62031, 62037, 62096,
    62127, 62139, 62153, 62154, 62157, 62162, 62163, 62164, 62166,
    62168, 62170, 62172, 62173, 62175, 62176, 62177, 62178, 62181, 62182, 62183,
    62184, 62185, 62186, 62187, 62188, 62205, 62206, 62207, 62208,
    62278, 62460, 62471, 62550, 62551, 62568, 62569, 62702, 62703,
    62158, 62165, 62179, 62281, 62337, 62094,
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
    # W1.6 "EMPTY!" (E, M, P, T and Y are gun-name letters above).
    65112,
    # W1.7 levelNumber's V (CAVERN).
    65141,
    # W1.8 merchant speech: "?", D, K and W.
    65113, 65123, 65130, 65142,
    # W1.10 styleSet's idle, run and air art for styles 1-4, with their
    # mirrored planar and DMA forms.
    *(base + ident for base in (0, 62000, 63000)
      for ident in (25, 26, 28, 29, 32, 33, 34, 35, 38, 40, 42)),
    # W2.3 the wall-kick pose sprPlayerWall (frame 0 is drawn).
    20, 62020, 63020,
    # W1.9 Rocket Jump's bulletBlast (spSplosion1) and its mirrored form.
    461, 62461,
    # W1.9 effect-surface art: explosions (both facings), the drone and the
    # Heart Balloon and the jetpack flame.
    463, 62463, 470, 62470, 131, 526, 700, 701, 21, 22, 23,
    # W1.10 hard mode's spawner enemies: sprEyefish 289, its corpse
    # sprEyefishDead 291 and sprNeonRedBall 352, with their mirrored planar
    # and DMA forms (sprEyefishDmg 290 needs hitStun, which nothing sets).
    289, 291, 352, 62289, 62291, 62352, 63289, 63352,
})

# Art drawn only on the CPU effect surface (drawFx), never by the blitter:
# the Chip plane bank leaves it out.
SURFACE_ONLY_IDS = frozenset({463, 62463, 470, 62470, 131, 526, 700, 701, 21, 22, 23})

# Ceilings are reviewed inputs, never inferred from generated payload sizes.
# DMA gameplay currently uses 115806 bytes; its ceiling leaves 2754 bytes
# growth, not permission to consume all of the measured free Chip RAM. Only
# the full-catalogue reference oracles load it; production gameplay leases
# the cavern banks below.
# W2.3's wall-kick pose takes the reference game planes past 176 KiB.
# W1.9 Hot Casing's sprLeakBullet 467 planes take them from 194,270 to
# 194,590 bytes, past 190 KiB.
# W1.9 Heart Balloon's surface-only art takes the reference game CPU bank
# from 493,356 to 495,612 bytes, past 482 KiB.
# The Safety Jetpack's flame takes it to 496,932, past 485 KiB.
# W1.10 hard mode's eyefish, its corpse and the neon red ball take it to
# 502,464, past 486 KiB, and the reference game planes from 194,590 to
# 196,726, past 191 KiB.
# W1.11's low-health red silhouettes (every living player pose, both facings)
# take it to 523,114, past 510 KiB, and its planes to 205,818, past 200 KiB.
BANK_BUDGETS = {'boot': 24 * 1024, 'front': 102 * 1024,
                'game': 511 * 1024, 'game-planes': 201 * 1024}

# Explicit cavern roots, independent of future additions to GAME_IDS. Derived
# mirror/DMA forms are admitted only for these roots. Later-area skeleton/bone
# and wood debris art must not become cavern residents. builderCavern
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
    # W1.9 Hot Casing's sprLeakBullet.
    467,
    # W1.5 fxTimeShard 238 draws sprite 91 (shared with area-four debris).
    91,
    # W1.6 objDeadBody 263 sprites, the bullet-immune spark 112 and the
    # player's jump/landing dust 94/95.
    112, 158, 165, 179, 281, 337, 94, 95,
    # W1.7 combo and streak HUD art.
    393, 394, 395, 397, 400, 403, 404, 405, 406,
    # W1.7 upgrade icons (frame 1), drawn on the HUD layer.
    506, 507, 508, 509, 510, 511, 512, 516, 518, 520, 521, 522, 523, 524,
    525, 528, 529, 530, 531, 532, 533, 534, 535,
    # W1.10 per-style player art.
    25, 26, 28, 29, 32, 33, 34, 35, 38, 40, 42,
    # W2.3 the wall-kick pose.
    20,
    # W1.9 Rocket Jump's blast, the explosions, the drone, the balloon and
    # the jetpack flame.
    461, 463, 470, 131, 526, 700, 701, 21, 22, 23,
    # W1.10 hardSet1's eyefish (and its corpse) and neon red ball.
    289, 291, 352,
})
CAVERN_IDS = frozenset(
    {root + offset for root in CAVERN_ROOTS for offset in (0, 62000, 63000)} |
    {65006, 65100, 65101, 65102, 65103, 65104, 65105, 65106, 65107,
     65108, 65109, 65110, 65111, 65112, 65113, 65123, 65130, 65141, 65142} |
    {65120 + ord(c) - ord('A') for c in 'ABCEGHILMNOPRSTUY'}) & GAME_IDS
# W1.3 room effects (gem chunk, sparkles, flash, red splash turns, fire,
# small smoke) raised the measured DMA atlas from 106,574 to 109,410 bytes.
# W1.4 gun art (modules, letters, casing, laser and spread shots, the laser
# muzzle's 270 page, the cropped pickup effect, notice icon and glyphs) adds
# 17,762 bytes: 228,948 -> 246,710 CPU, and 8,334 Chip plane bytes:
# 109,410 -> 117,744. Planar BOBs only, no DMA streams.
# W1.5 time-shard sprite 91 (six frames) adds 1,170 Chip plane bytes:
# 117,744 -> 118,914.
# W1.6 corpses (five mirrored pairs), the immune spark, jump/landing dust and
# the "!" glyph add 2,460 Chip plane bytes: 118,914 -> 121,374 (1 MiB Chip target).
# W1.7 combo digits and icons, COMBO REWARD!/COMBO!, the gem bar caps,
# GEM HIGH and gem-sick motion, the V glyph and the gem-high afterimage pages
# (red striped player poses, frames 1024..2047, both parities and facings)
# add 28,396 CPU bytes: 253,090 -> 281,486, and 16,640 Chip plane bytes:
# 121,374 -> 138,014; the 23 upgrade icons (moved out of the HUD bank) add
# 6,572 and 5,320 more: 288,058 and 143,334. W1.8's merchant-speech glyphs
# ("?", D, K, W) add 310 and 160: 288,368 and 143,494. The full reference
# catalogue: 300,192 and 148,722. W1.10's per-style idle, run and air art
# (styles 1-4, with mirrored, DMA and afterimage forms) adds 78,466 and
# 30,510: 366,834 and 174,004 (full catalogue 378,658 and 179,232).
# W2.3's wall-kick pose sprPlayerWall (frame 0, mirrored, DMA and afterimage
# forms) takes the cavern CPU bank past 360 KiB: ceiling 361 KiB.
# W1.9's Rocket Jump blast spSplosion1 (twelve 64x64 frames and their mirror)
# adds 17,512 CPU bytes: 368,798 -> 386,310 (full catalogue 398,138), and
# 13,538 Chip plane bytes: 175,094 -> 188,632 (193,860).
# The CPU-only effect-surface art (the two explosion sprites 463 and 470
# in four turns and both facings, and the drone) adds 94,346 CPU bytes and
# no Chip: 386,310 -> 480,656 (cavern planes 188,872). It is Fast RAM data
# on the A1200 target; W3 can trim the turns no route draws.
# W1.9 Hot Casing's sprLeakBullet 467 (eight frames) adds 420 CPU bytes:
# 481,112 -> 481,532, past 470 KiB. The Heart Balloon's 526, 700 and 701
# (four frames each, effect surface only) add 2,256: 483,788, past 472 KiB.
# The Safety Jetpack's flame (21-23, three frames each) adds 1,320: 485,108,
# past 473 KiB.
# W1.10 hard mode's eyefish 289 (four frames), its corpse 291 and the neon
# red ball 352 (four frames), with mirrored and DMA forms, add 5,532 CPU
# bytes: 490,640, past 474 KiB, and take the cavern planes to 191,500 Chip
# bytes, past 186 KiB.
# W1.11 playerDraw's low-health flicker: a shaderPlayerRed silhouette page
# (frames 2048 + 256 * mirrored) for every living player pose adds 20,654
# CPU bytes: 511,294, past 499 KiB, and 9,092 Chip plane bytes: 200,592,
# past 195 KiB.
BANK_BUDGETS.update({'cavern': 500 * 1024, 'cavern-planes': 196 * 1024})
# W1.14's garden jump poses (spin 1 frames 0-7, balancing 15 frames 0-4, the
# wall pose 20 frame 0, as mirrored hardware sprites in two DMA copies) add
# 8,288 bytes: 103,268 -> 111,556, past 108 KiB. The shared Chip region is
# still sized by the prepared fields (about 146.8 KB), so no Chip is added.
BANK_BUDGETS.update({'front': 109 * 1024})


def require_budget(bank, size):
    if bank not in BANK_BUDGETS:
        raise ValueError(f'undeclared scene bank: {bank}')
    if size <= 0 or size > BANK_BUDGETS[bank]:
        raise ValueError(f'{bank}: {size} bytes exceeds scene admission budget '
                         f'{BANK_BUDGETS[bank]}')
