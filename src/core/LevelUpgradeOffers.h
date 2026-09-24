#ifndef DOWNWELL_CORE_LEVEL_UPGRADE_OFFERS_H
#define DOWNWELL_CORE_LEVEL_UPGRADE_OFFERS_H
#include "GameMakerRandom.h"
namespace downwell {
// resultRandomizeUg VM: levelUgMax is an inclusive last index, not a count.
// The caller owns upgrade levels/caps and localization of the resulting IDs.
struct LevelUpgradeOffers {
    unsigned ids[4] = {};
    template<class Eligible>
    bool randomize(unsigned lastIndex, unsigned highestUpgradeId,
                   GameMakerRandom& random, Eligible eligible) {
        if (lastIndex >= 4 || highestUpgradeId > 0xffffu) return false;
        ids[0] = ids[1] = ids[2] = 100;
        // Deliberately do not reset slot 3: the source leaves its previous
        // value intact if all 61 attempts fail (initial Result Create sets 0).
        for (unsigned slot = 0; slot <= lastIndex; ++slot) {
            for (unsigned rejected = 0; rejected <= 60; ++rejected) {
                const unsigned candidate = random.irandomSmall(highestUpgradeId);
                bool duplicate = false;
                if (eligible(candidate)) {
                    for (unsigned prior = 0; prior < slot; ++prior)
                        if (ids[prior] == candidate) duplicate = true;
                    if (!duplicate) { ids[slot] = candidate; break; }
                }
            }
        }
        return true;
    }
};
}
#endif
