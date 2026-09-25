#ifndef DOWNWELL_CORE_GAMEPLAY_PLAYER_STEP_H
#define DOWNWELL_CORE_GAMEPLAY_PLAYER_STEP_H

#include "PlayerMotion.h"
#include "PlayerDeathSequence.h"
#include "RunSession.h"
#include "GameMakerRounding.h"
#include "Profile.h"

namespace downwell {

// Shared implemented objPlayer_n Begin Step phase. Owners bind terrain and
// head-break callbacks first, then publish the returned motion and emit shots.
// Activation remains before this phase; enemy alarms remain after it.
// No renderer or instance-store representation is selected here.
inline PlayerMotionDelta stepGameplayPlayer(PlayerMotion& player,
    PlayerDeathSequence& death, RunSession& run, CurrencyAccount& currency,
    int& health, GameMakerRandom& random, const Input& input,
    int originX, int originY, int viewX, int viewY, bool controlsLocked = false) {
    death.beginStep(input.fire);
    run.combo.beginStep();
    struct Context {
        PlayerMotion& player;
        PlayerDeathSequence& death;
        RunSession& run;
        CurrencyAccount& currency;
        int& health;
    } context{player, death, run, currency, health};
    player.configureComboCompletion([](void* opaque) {
        auto& c = *static_cast<Context*>(opaque);
        c.run.combo.finish(c.player, c.currency, c.health,
                           c.run.upgrades.levels[20] != 0);
    }, &context);
    player.configureMovementBoundary([](void* opaque) {
        auto& c = *static_cast<Context*>(opaque);
        c.run.gems.observeDeathMovement(c.death.movementDead(), c.currency.gameGem);
    }, &context);
    DW_PROFILE_PHASE(profilePlayerMotion);
    const auto motion = player.step(input,
        controlsLocked || run.shops.noControl || run.exitControlsLocked,
        death.movementDead(), run.shops.interactable, run.goalStopped, run.gemHigh);
    DW_PROFILE_PHASE(profilePlayerShops);
    const int shopPlayerX = originX + roundGameMakerQ8(player.offsetXFixed());
    const int shopPlayerY = originY + roundGameMakerQ8(player.offsetYFixed());
    run.shops.observePlayer(shopPlayerX, shopPlayerY, player.grounded(), player.speedXFixed());
    int facing = player.facing();
    const auto shopRandom = [&random](unsigned lo, unsigned hi) {
        return lo + random.irandomSmall(hi - lo);
    };
    run.shops.keeperAlarms(shopRandom);
    run.shops.step(input, shopPlayerX, shopPlayerY, false, death.active,
        facing, currency, [&random] { return 1u + random.chooseIndexSmall(3); },
        [&run, &currency](unsigned id) { run.consumables.grant(id, currency); },
        [](auto&, auto) {});
    player.face(facing);
    run.shops.advanceKeepers(shopPlayerX, false, run.stage.area, shopRandom);
    run.shops.advancePresentation();
    DW_PROFILE_PHASE(profilePlayer);
    // These callbacks point into this invocation, never retain them in a
    // copied world or in the next frame's movement.
    player.configureComboCompletion(nullptr, nullptr);
    player.configureMovementBoundary(nullptr, nullptr);
    // Begin Step checks the old published x/y before x=round(xx), y=round(yy).
    // Do not substitute the newly moved shop coordinates or Draw's radius.
    const bool inGoalField = run.levelExitPresent && run.goalField.meetsPlayer(
        originX + roundGameMakerQ8(player.frameStartXFixed()),
        originY + roundGameMakerQ8(player.frameStartYFixed()));
    // Boss completion is outside the implemented area-one scope.
    run.clock.advance(true, death.active, run.goalStopped, false, inGoalField);
    run.advanceLevelExitAlarms(viewX, viewY);
    death.publishHealth(health);
    death.advanceAlarms(player.grounded());
    run.results.open(death.menuReady, run.gems.lifetime, currency.gameGem);
    return motion;
}

} // namespace downwell
#endif
