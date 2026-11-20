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
struct NoPlayerTimeTransition { bool operator()(const PlayerMotion&,int,int)const{return true;} };
// W2.3: the player's Begin Step sound scripts (PlayerMotion::soundEvent), run
// right after its motion so their RNG draws precede shops and later objects.
struct NoPlayerSounds { bool operator()(PlayerMotion&)const{return true;} };
struct UnindexedPlayerGoalContact {
    bool operator()(RunSession& run,MotionBinary64 x,MotionBinary64 y,int facing,bool& hit)const{
        return run.playerGoalContact(x,y,facing,hit);
    }
};

// Shared implemented objPlayer_n Begin Step phase. Owners bind terrain and
// head-break callbacks first, then publish the returned motion and emit shots.
// Activation remains before this phase; enemy alarms remain after it.
// No renderer or instance-store representation is selected here.
template<class PublishPosition=NoPlayerPositionPublication,class PublishScale=NoPlayerScalePublication,
         class GoalContact=UnindexedPlayerGoalContact,class TimeTransition=NoPlayerTimeTransition,
         class PlayerSounds=NoPlayerSounds>
inline PlayerMotionDelta stepGameplayPlayer(PlayerMotion& player,
    PlayerDeathSequence& death, RunSession& run, CurrencyAccount& currency,
    int& health, GameMakerRandom& random, const Input& input,
    int originX, int originY, int viewX, int viewY, bool controlsLocked = false,
    PublishPosition publishPosition = {},PublishScale publishScale={},GoalContact goalContact={},TimeTransition timeTransition={},
    PlayerSounds playerSounds={}) {
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
        const auto previousGems=c.currency.gameGem;
        c.run.combo.finish(c.player, c.currency, c.health,
                           c.run.upgrades.levels[20] != 0);
        if(c.currency.gameGem!=previousGems)c.run.gemStreakRise.reset();
    }, &context, run.timeStopped); // pTimeStop before this Begin Step's transition
    player.configureMovementBoundary([](void* opaque) {
        auto& c = *static_cast<Context*>(opaque);
        // The first death movement (!prvdeath): sfxDeath, then the song's
        // gain to 0 and its stop, then saveStats.
        if(c.death.movementDead()&&!c.run.gems.recorded)c.run.playDeathSounds();
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
    if(!playerSounds(player)){run.effectsSucceeded=false;return motion;}
    // objPlayer_n Begin Step [11..18] skips the collision call itself when
    // goalStop/death suppress accounting, not only the eventual increment.
    // It queries the old published x/y before shop/interactable processing.
    bool inGoalField=false;
    if(!run.goalStopped && !death.active && !goalContact(run,
        MotionBinary64::fromInteger(originX + roundGameMakerQ8(player.frameStartXFixed())),
        MotionBinary64::fromInteger(originY + roundGameMakerQ8(player.frameStartYFixed())),
        player.facing(),inGoalField)){run.effectsSucceeded=false;return motion;}
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
    // objPlayer_n Begin Step [42..79], after published x/y and interaction
    // observation. A failed synchronous transition must stop later work.
    if(!timeTransition(player,shopPlayerX,shopPlayerY)){run.effectsSucceeded=false;return motion;}
    int facing = player.facing();
#ifndef DOWNWELL_REFERENCE_FRAME_WORK
    // A synchronous time-transition callback above may create the first shop.
    // Read membership here, not at frame entry. Empty families still observe
    // held input so a later birth cannot manufacture a new press edge.
    if(run.shops.allocatedCount){
#endif
    const auto shopRandom = [&random](unsigned lo, unsigned hi) {
        return lo + random.irandomSmall(hi - lo);
    };
    run.shops.keeperAlarms(shopRandom);
    run.shops.step(input, shopPlayerX, shopPlayerY, false, death.active,
        facing, currency, [&random] { return 1u + random.chooseIndexSmall(3); },
        [&run, &currency](unsigned id) { run.consumables.grant(id, currency); },
        [](auto&, auto) {});
    run.playShopSounds(viewX, viewY, shopPlayerX);
    player.face(facing);
    run.shops.advanceKeepers(shopPlayerX, false, run.stage.area, shopRandom);
    run.shops.advancePresentation();
#ifndef DOWNWELL_REFERENCE_FRAME_WORK
    }else run.shops.publishInput(input);
#endif
    DW_PROFILE_PHASE(profilePlayer);
    run.advanceLevelExitAlarms(viewX, viewY);
    death.publishHealth(health);
    death.advanceAlarms(player.grounded());
    const bool resultsWereActive=run.results.active;
    run.results.open(death.menuReady, run.gems.lifetime, currency.gameGem);
    run.playResultsOpenSound(resultsWereActive);
    return motion;
}

} // namespace downwell
#endif
