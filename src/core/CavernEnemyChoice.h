#ifndef DOWNWELL_CORE_CAVERN_ENEMY_CHOICE_H
#define DOWNWELL_CORE_CAVERN_ENEMY_CHOICE_H

#include <cassert>
#include <cstdint>

namespace downwell {

// builderCavern blocks 12–38. random(level + 2) <= level consumes a word
// for EVERY cell, including non-enemy cells. Integer cutoffs exactly match
// the runner's word / 2^32 scaled double for cavern levels 1–3. Hard mode
// short-circuits into a second word: || (global.hardMode && random(5) > 1),
// true for words above 2^32 / 5.
template<class Random>
int cavernEnemyChoice(char symbol, unsigned level, Random& random, bool hard = false) {
    assert(level >= 1 && level <= 3);
    const std::uint32_t cutoffs[] = {0x55555555U, 0x80000000U, 0x99999999U};
    if (random.nextU32() > cutoffs[level - 1] && (!hard || random.nextU32() <= 0x33333333U)) return -1;
    switch (symbol) {
    case 'a': return 111;
    case 'b': return 116;
    case 'c': return level >= 2 && random.chooseIndexSmall(2) != 0 ? 119 : -1;
    case 'd': return 113;
    case 'e': return 117;
    case 't': return 112;
    case '%': {
        const int enemies[] = {113, 111, 119, 112};
        return enemies[random.chooseIndexSmall(4)];
    }
    default: return -1;
    }
}

} // namespace downwell
#endif
