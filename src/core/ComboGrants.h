#ifndef DOWNWELL_CORE_COMBO_GRANTS_H
#define DOWNWELL_CORE_COMBO_GRANTS_H
#include "ComboState.h"
#include "CurrencyAccount.h"
#include "PlayerMotion.h"
namespace downwell {
// gainHp's persistent reserve. Overflow healing becomes heart pieces; each
// full group increases maximum health and fills it. Presentation stays outside.
struct HealthReserve {
    int maximum = 4;
    unsigned pieces = 0;
    unsigned piecesPerHeart = 4;
    unsigned gain(int& health, unsigned amount) {
        const int gain = static_cast<int>(amount);
        if (health + gain <= maximum) { health += gain; return 0; }
        const int difference = maximum - health;
        health = maximum;
        pieces += static_cast<unsigned>(gain - difference);
        unsigned increases = 0;
        while (pieces >= piecesPerHeart) {
            ++maximum; health = maximum;
            pieces -= piecesPerHeart;
            ++increases;
        }
        return increases;
    }
};
struct ComboGrantEffects {
    bool nonGemGain = false, ammoEffect = false, healSound = false;
    unsigned maximumHealthMessages = 0;
};
inline ComboGrantEffects applyComboGrants(const ComboState::Reward& reward,
        CurrencyAccount& currency, PlayerMotion& player, int& health,
        HealthReserve& reserve, unsigned streakTimerStart = 300, bool lessResist = false) {
    ComboGrantEffects effects;
    if (reward.gems) {
        // nonGemGainGem shares these mutations with scrGainGem, but not the
        // dropped-gem pickup path or its upgrade multipliers.
        currency.collect(reward.gems, streakTimerStart, lessResist);
        effects.nonGemGain = true;
    }
    if (reward.ammo) {
        player.increaseAmmoCapacity(reward.ammo);
        effects.ammoEffect = true;
    }
    if (reward.health) {
        effects.maximumHealthMessages = reserve.gain(health, reward.health);
        effects.healSound = true;
    }
    return effects;
}
} // namespace downwell
#endif
