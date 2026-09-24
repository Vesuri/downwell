#include "core/ComboState.h"
#include "core/ComboGrants.h"
#include "core/RunCombo.h"
#include <cstdlib>
#include <iostream>
static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    using downwell::ComboState;
    struct Live {
        downwell::RunCombo combo;
        downwell::CurrencyAccount currency;
        PlayerMotion player;
        int health = 4;
        unsigned calls = 0;
        unsigned boundaryGems = 0;
        static void boundary(void* opaque) {
            auto& value = *static_cast<Live*>(opaque);
            value.boundaryGems = value.currency.gameGem;
        }
        static void complete(void* opaque) {
            auto& value = *static_cast<Live*>(opaque);
            ++value.calls;
            value.combo.finish(value.player, value.currency, value.health);
        }
    } live;
    live.combo.state.count = 25;
    live.player.configureComboCompletion(Live::complete, &live, true);
    live.player.step({});
    require(live.calls == 0 && live.combo.state.count == 25, "time stop suppresses grounded completion");
    live.player.configureComboCompletion(Live::complete, &live);
    live.player.configureMovementBoundary(Live::boundary, &live);
    live.player.step({});
    require(live.calls == 1 && live.combo.state.best == 25 && live.combo.state.count == 0 &&
            live.currency.gameGem == 100 && live.player.ammoCapacity() == 9 && live.combo.healthReserve.pieces == 1,
            "grounded shared phase applies all grants synchronously");
    require(live.boundaryGems == 100, "death statistics boundary sees grounded movement rewards");
    live.player.step({});
    require(live.calls == 2 && live.currency.gameGem == 100 && live.player.ammoCapacity() == 9,
            "repeated grounded updates cannot award the same combo twice");
    live.player = PlayerMotion(false, 0);
    live.player.configureComboCompletion(Live::complete, &live);
    live.player.configureMovementBoundary(Live::boundary, &live);
    live.combo.state.count = 15;
    live.player.step({});
    require(live.calls == 3 && live.player.grounded() && live.player.ammo() == 9,
            "landing completes before recharge, retaining the newly increased capacity");
    require(live.boundaryGems == 100 && live.currency.gameGem == 200,
            "death statistics boundary precedes later wall-collision landing rewards");
    live.player = PlayerMotion(false, 0);
    live.player.configureComboCompletion(Live::complete, &live);
    live.player.configureSolidResolver([](void*, int x, int) { return x >= 3; }, nullptr, -60, 60);
    live.combo.state.count = 8;
    downwell::Input right; right.right = true;
    live.player.step(right);
    require(live.calls == 3 && live.combo.state.count == 8 && live.player.grounded(),
            "simultaneous side collision suppresses landing completion");
    live.player.step({});
    require(live.calls == 4 && live.combo.state.count == 0 && live.currency.gameGem == 300,
            "next grounded movement completes the deferred combo");
    PlayerMotion player(false, 1000);
    downwell::Input fire; fire.fire = true;
    player.step(fire);
    require(player.ammo() == 7, "reward fixture starts with spent charge");
    downwell::CurrencyAccount currency;
    currency.collect(3);
    downwell::HealthReserve reserve;
    int health = 3;
    ComboState::Reward grants;
    grants.gems = 100; grants.ammo = 1; grants.health = 1;
    const auto effects = downwell::applyComboGrants(grants, currency, player, health, reserve, 300, true);
    require(currency.currency == 103 && currency.gameGem == 103 && currency.streak == 103 &&
                currency.timer == 480, "bonus gems update all balances and the source streak timer");
    require(player.ammoCapacity() == 9 && player.ammo() == 9 && health == 4 && reserve.pieces == 0,
            "ammo reward raises capacity and refills while health first heals damage");
    require(effects.nonGemGain && effects.ammoEffect && effects.healSound &&
                effects.maximumHealthMessages == 0, "grant effects remain separate from platform rendering");
    player.step({}); player.step(fire);
    require(player.ammo() == 8, "upgraded charge is consumed normally");
    player.stompBounce();
    require(player.ammo() == 9, "stomping refills upgraded capacity");
    player.step({}); player.step(fire);
    player.contactKnockback(0, 0);
    require(player.ammo() == 9, "damage knockback refills upgraded capacity");
    for (unsigned i = 0; i < 400; ++i) player.step({});
    require(player.grounded() && player.ammo() == 9, "landing retains upgraded capacity");
    for (unsigned i = 0; i < 3; ++i)
        require(reserve.gain(health, 1) == 0 && health == 4, "full-health gains accumulate pieces");
    require(reserve.gain(health, 1) == 1 && health == 5 && reserve.maximum == 5 && reserve.pieces == 0,
            "four surplus health points increase maximum health and refill it");
    health = 3;
    require(reserve.gain(health, 11) == 2 && health == 7 && reserve.maximum == 7 && reserve.pieces == 1,
            "large heal first fills damage, then processes every complete piece group");
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
