#ifndef DOWNWELL_CORE_CONSUMABLES_H
#define DOWNWELL_CORE_CONSUMABLES_H
#include "ComboGrants.h"
namespace downwell {
// scrGainUg IDs 100..105, consumed by scrControlBeginstepCheck, not at purchase.
struct Consumables {
    struct Notification { unsigned type = 0, amount = 0; };
    struct Effects {
        Notification notifications[6];
        unsigned count = 0, healingSounds = 0, maximumHealthMessages = 0;
    } effects;
    std::uint8_t pending[6] = {};

    bool grant(unsigned id, CurrencyAccount& account) {
        if (id < 100 || id > 105) return false;
        auto& count = pending[id - 100];
        if (count >= 100) account.currency += 100; // scrGainUg cap fallback
        else ++count;
        return true;
    }
    void beginStep(PlayerMotion& player, int& health, HealthReserve& reserve) {
        effects = {};
        // Six independent source ifs. Each consumes ONE unit, in ID order,
        // without the pause/time-stop/death guards used by other controller work.
        for (unsigned item = 0; item < 6; ++item) {
            if (!pending[item]) continue;
            const unsigned healing = item == 0 || item == 4 || item == 5 ? 1 : item == 2 ? 2 : 0;
            const unsigned charge = item == 1 || item == 4 ? 1 : item == 3 ? 2 : 0;
            if (item == 5) ++reserve.maximum; // Curry raises max BEFORE healing.
            if (healing) {
                ++effects.healingSounds;
                effects.maximumHealthMessages += reserve.gain(health, healing);
            }
            if (charge) player.increaseAmmoCapacity(charge);
            --pending[item];
            effects.notifications[effects.count++] = {
                item == 4 ? 2u : healing ? 1u : 0u, item == 2 || item == 3 ? 2u : 1u};
        }
    }
};
}
#endif
