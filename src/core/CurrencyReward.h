#ifndef DOWNWELL_CORE_CURRENCY_REWARD_H
#define DOWNWELL_CORE_CURRENCY_REWARD_H
#include "GemMotion.h"
#include "StableInstanceSlots.h"
namespace downwell {
struct CurrencyBirth {
    GemMotion motion;
    int amount;
    bool large;
};

// scrCurrencySpawn: tens first, then twos (including an odd final remainder).
// The fixed port capacity is explicit; rejected births consume no RNG. Source
// allocation failure and Create's embedded-wall relocation remain separate work.
template<class Random, class Emit>
unsigned spawnCurrencyReward(int amount, int x, int y, bool attracted,
                             unsigned available, Random& random, Emit emit) {
    unsigned emitted = 0;
    while (amount > 0 && emitted < available) {
        CurrencyBirth birth;
        birth.large = amount >= 10;
        birth.amount = birth.large ? 10 : 2;
        birth.motion.initialize(x, y, attracted, random);
        emit(birth);
        amount -= birth.amount;
        ++emitted;
    }
    return emitted;
}

// A sparse backend must not insert new instances before surviving older
// instances. Stable compaction keeps subsequent Step/RNG traversal in birth
// order, matching the dense host store instead of reusing its first hole.
template<class Record>
unsigned compactCurrencySlots(Record* records, unsigned capacity) {
    return compactInstanceSlots(records, capacity);
}
} // namespace downwell
#endif
