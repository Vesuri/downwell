#ifndef DOWNWELL_CORE_CATACOMB_BUILDER_SYMBOLS_H
#define DOWNWELL_CORE_CATACOMB_BUILDER_SYMBOLS_H

namespace downwell {
// builderCatacomb VM blocks 17--61. Operations deliberately retain choose
// argument lists (including singleton/identical choices), rather than assuming
// how the historical Windows runner maps each builtin to random words.
// Ops: choose(const int*, unsigned), irandom(unsigned), randomFiveBelow(unsigned),
// create(objectId, x, y). create is synchronous; false aborts emission.
// x/y are the already evaluated buildx and y + 16*k coordinates.
template<class Ops>
bool emitCatacombSymbol(char symbol, int x, int y, unsigned level,
                        bool hardMode, Ops& ops) {
    if (level < 1 || level > 3) return false;
    static const int coin[] = {0, 1};
    static const int frequent[] = {0, 1, 1};
    static const int rare[] = {0, 0, 0, 1};
    static const int skull[] = {126};
    static const int skullPair[] = {126, 126};
    static const int mixed[] = {126, 126, 111};
    static const int levelOne[] = {0, 0, 1};
    static const int levelThree[] = {0, 0, 1, 1, 1};
    switch (symbol) {
    case 'A': return ops.create(97, x, y);
    case 'M': return ops.create(102, x, y);
    case 'I': return true;
    case 'J': return !ops.choose(frequent, 3) || ops.create(296, x, y);
    case 'Z': return ops.create(297, x, y);
    case 'Y': return !ops.choose(frequent, 3) || ops.create(299, x, y);
    case 'P': return ops.create(300, x, y);
    case 'a': return ops.choose(coin, 2) != 1 || ops.create(111, x, y);
    case 'b': return !ops.choose(rare, 4) || ops.create(116, x, y);
    case 'c':
        if (ops.choose(coin, 2) == 1) {
            const int object = ops.choose(skull, 1);
            return ops.create(object, x, y);
        }
        return true;
    case 'd':
        if (ops.choose(coin, 2) == 1)
            return ops.create(126, x + 8, y) && ops.create(126, x - 8, y);
        return true;
    case 'e': return !ops.randomFiveBelow(level) || ops.create(120, x, y);
    case 'i': return ops.create(204, x, y);
    case 'j': return ops.choose(coin, 2) != 1 || ops.create(120, x, y);
    case '%': {
        // This overwritten draw precedes the level choice and hard-mode override.
        int chance = ops.irandom(level);
        if (level == 1) chance = ops.choose(levelOne, 3);
        else if (level == 2) chance = ops.choose(coin, 2);
        else chance = ops.choose(levelThree, 5);
        if (hardMode) chance = 1;
        if (!chance) return true;
        const int object = level <= 2 ? ops.choose(mixed, 3) : ops.choose(skullPair, 2);
        return ops.create(object, x, y);
    }
    default: return true; // Common builder handles the remaining symbols.
    }
}
}
#endif
