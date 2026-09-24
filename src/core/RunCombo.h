#ifndef DOWNWELL_CORE_RUN_COMBO_H
#define DOWNWELL_CORE_RUN_COMBO_H
#include "ComboGrants.h"
namespace downwell {
// Shared live controller; adapters only forward source phase events/storage.
struct RunCombo {
    ComboState state;
    HealthReserve healthReserve;
    ComboState::Reward reward;
    ComboState::Completion completion;
    ComboGrantEffects grants;
    void beginStep() { reward = {}; completion = {}; grants = {}; }
    void finish(PlayerMotion& player, CurrencyAccount& currency, int& health, bool lessResist = false) {
        const auto result = state.finish({}, [&](const ComboState::Reward& value) {
            reward = value;
            grants = applyComboGrants(value, currency, player, health, healthReserve, 300, lessResist);
        });
        // A second empty grounded call must not discard this update's effects.
        completion.rewardCreated |= result.rewardCreated;
        completion.momentDelay |= result.momentDelay;
        completion.saveRecord |= result.saveRecord;
        completion.achievementMask |= result.achievementMask;
    }
    template<class Digest> void appendState(Digest& digest) const {
        digest.addU32(state.count); digest.addU32(state.best); digest.addU32(state.record);
        digest.addI32(healthReserve.maximum); digest.addU32(healthReserve.pieces);
    }
};
} // namespace downwell
#endif
