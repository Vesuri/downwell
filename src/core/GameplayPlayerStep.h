#ifndef DOWNWELL_CORE_GAMEPLAY_PLAYER_STEP_H
#define DOWNWELL_CORE_GAMEPLAY_PLAYER_STEP_H

#include "PlayerMotion.h"
#include "PlayerDeathSequence.h"
#include "RunSession.h"
#include "GameMakerRounding.h"
#include "Profile.h"

namespace downwell {
struct NoPlayerPositionPublication {
    bool operator()(int,int) const { return true; }
};
struct NoPlayerScalePublication { bool operator()(int)const{return true;} };

// Shared implemented objPlayer_n Begin Step phase. Owners bind terrain and
// head-break callbacks first, then publish the returned motion and emit shots.
// Activation remains before this phase; enemy alarms remain after it.
// No renderer or instance-store representation is selected here.
template<class PublishPosition=NoPlayerPositionPublication,class PublishScale=NoPlayerScalePublication>
inline PlayerMotionDelta stepGameplayPlayer(PlayerMotion& player,
    PlayerDeathSequence& death, RunSession& run, CurrencyAccount& currency,
    int& health, GameMakerRandom& random, const Input& input,
    int originX, int originY, int viewX, int viewY, bool controlsLocked = false,
    PublishPosition publishPosition = {},PublishScale publishScale={}) {
    death.beginStep(input.fire);
    run.combo.beginStep();
    struct Context {
        PlayerMotion& player;
        PlayerDeathSequence& death;
        RunSession& run;
        CurrencyAccount& currency;
        int& health;
        PublishScale& publishScale;
    } context{player, death, run, currency, health,publishScale};
    player.configureFacingPublication([](void* opaque,int value){
        return static_cast<Context*>(opaque)->publishScale(value);
    },&context);
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
    player.configureCollisionOrigin(originX,originY);
    const auto motion = player.step(input,
        controlsLocked || run.shops.noControl || run.exitControlsLocked,
        death.movementDead(), run.shops.interactable, run.goalStopped, run.gemHigh);
    // These callbacks point into this invocation and are only used by motion.
    // Clear them before any later checked operation can return early.
    player.configureComboCompletion(nullptr,nullptr);
    player.configureMovementBoundary(nullptr,nullptr);
    player.configureFacingPublication(nullptr,nullptr);
    if(!player.motionSucceeded()){
        run.effectsSucceeded=false;
        return motion;
    }
    // objPlayer_n Begin Step [11..18] skips the collision call itself when
    // goalStop/death suppress accounting, not only the eventual increment.
    // It queries the old published x/y before shop/interactable processing.
    bool inGoalField=false;
    if(!run.goalStopped && !death.active && !run.playerGoalContact(
        MotionBinary64::fromInteger(originX + roundGameMakerQ8(player.frameStartXFixed())),
        MotionBinary64::fromInteger(originY + roundGameMakerQ8(player.frameStartYFixed())),
        player.facing(),inGoalField))return motion;
    // Boss completion is outside the implemented area-one scope.
    run.clock.advance(true, death.active, run.goalStopped, false, inGoalField);
    DW_PROFILE_PHASE(profilePlayerShops);
    const int shopPlayerX = originX + roundGameMakerQ8(player.offsetXFixed());
    const int shopPlayerY = originY + roundGameMakerQ8(player.offsetYFixed());
    // Begin Step [25]: x=round(xx), y=round(yy), before camera/interactable
    // queries. This publishes instance coordinates, not private accumulators.
    if(!publishPosition(shopPlayerX,shopPlayerY)) {
        run.effectsSucceeded=false;return motion;
    }
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
    run.advanceLevelExitAlarms(viewX, viewY);
    death.publishHealth(health);
    death.advanceAlarms(player.grounded());
    run.results.open(death.menuReady, run.gems.lifetime, currency.gameGem);
    return motion;
}

} // namespace downwell
#endif
