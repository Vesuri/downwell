#ifndef DOWNWELL_CORE_UPGRADE_INVENTORY_H
#define DOWNWELL_CORE_UPGRADE_INVENTORY_H
#include "Consumables.h"
namespace downwell {
// scrUpgrades / scrGainUg: normal upgrade IDs are 0..22. The three cap-zero
// entries remain in the ID space but are ineligible for resultRandomizeUg.
// Ongoing weapon/enemy effects of the owned upgrades are separate work.
struct UpgradeInventory {
    static constexpr unsigned highestId = 22;
    unsigned char levels[highestId + 1] = {};
    unsigned char order[highestId + 2] = {}; // source ugOrder is one-based
    unsigned count = 0;
    struct Effects {
        unsigned healing = 0, maximumHealthMessages = 0;
        bool fortuneSound = false; // soundPlay(167,60,0,1)
    } effects;
    static unsigned cap(unsigned id) { return id == 2 || id == 19 || id == 21 ? 0 : id <= highestId ? 1 : 0; }
    bool eligible(unsigned id) const { return id <= highestId && levels[id] < cap(id); }
    bool grant(unsigned id, Consumables& consumables, CurrencyAccount& currency,
               PlayerMotion& player, int& health, HealthReserve& reserve) {
        effects = {};
        if (id >= 100 && id <= 105) return consumables.grant(id, currency);
        if (id > highestId) return false;
        if (!eligible(id)) currency.currency += 100;
        else {
            ++levels[id];
            order[++count] = static_cast<unsigned char>(id);
        }
        // Independent source branches: these still execute after a cap refund.
        if (id == 13 || id == 6) {
            effects.healing = id == 13 ? 4 : 1;
            effects.maximumHealthMessages = reserve.gain(health, effects.healing);
        }
        if (id == 19) player.increaseAmmoCapacity(4);
        if (id == 21) {
            currency.currency += 400;
            currency.gameGem += 400;
            effects.fortuneSound = true;
        }
        return true;
    }
};
}
#endif
