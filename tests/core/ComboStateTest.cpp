#include "core/ComboState.h"
#include <cstdlib>
#include <iostream>
static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    using downwell::ComboState;
    for (unsigned count = 0; count <= 101; ++count) {
        ComboState state;
        for (unsigned i = 0; i < count; ++i) state.enemyDefeated();
        require(state.best == 0, "airborne combo is not yet the best completed combo");
        unsigned calls = 0;
        const auto done = state.finish({}, [&](const ComboState::Reward& reward) {
            ++calls;
            require(state.count == count && state.best == 0 && state.record == 0,
                    "reward creation precedes statistics changes and reset");
            require(reward.combo == count && reward.gems == (count >= 8 ? 100u : 0u) &&
                    reward.ammo == (count >= 15 ? 1u : 0u) && reward.health == (count >= 25 ? 1u : 0u),
                    "default milestone grants are cumulative");
            const unsigned bonus = count >= 25 ? 4 : count >= 15 ? 3 : count >= 8 ? 2 : 0;
            require(reward.showBonusMax == bonus && reward.hideAlarm == 90 + 15 * bonus &&
                    reward.destroyAlarm == 105 + 15 * bonus,
                    "reward lifetime follows synchronous Create branch results");
            require(reward.soundMask == ((count >= 8 ? 1u : 0u) | (count >= 15 ? 2u : 0u) |
                    (count >= 25 ? 4u : 0u)), "all qualifying reward sounds are emitted");
        });
        require(calls == (count >= 5 ? 1u : 0u) && done.rewardCreated == (count >= 5) &&
                done.momentDelay == (count >= 5), "five starts reward UI, not the gem grant");
        require(done.achievementMask == ((count >= 10 ? 1u : 0u) | (count >= 30 ? 2u : 0u) |
                (count >= 100 ? 4u : 0u)), "achievement thresholds differ from reward milestones");
        require(state.count == 0 && state.best == count && state.record == count &&
                done.saveRecord == (count != 0), "completed combo updates records once");
        const auto repeated = state.finish({}, [](const ComboState::Reward&) { std::abort(); });
        require(!repeated.saveRecord && !repeated.rewardCreated && state.best == count,
                "repeated grounded completion cannot grant again or clear the best");
        state.beginRun();
        require(state.best == 0 && state.count == 0 && state.record == count,
                "new run preserves lifetime record only");
    }
    ComboState state;
    state.count = 5;
    state.finish({5,5,5}, [](const ComboState::Reward& reward) {
        require(reward.gems == 100 && reward.ammo == 1 && reward.health == 1,
                "milestone table is consumed rather than hard-coded at completion");
    });
    std::cout << "shared combo completion tests passed\n";
}
