#ifndef DOWNWELL_CORE_RUN_SESSION_H
#define DOWNWELL_CORE_RUN_SESSION_H
#include "RunGemStatistics.h"
#include "RunClock.h"
#include "RunCombo.h"
#include "RunStage.h"
#include "DeathResults.h"
#include "ResultsDestination.h"
#include "AmmoHudMotion.h"
#include "AmmoGaugeRows.h"
#include "Consumables.h"
#include "ShopWorld.h"
#include "SideCameraLocker.h"
#include "LevelExit.h"
#include "GoalTimeField.h"
#include "ExitDither.h"
#include "UpgradeInventory.h"
#include "CurrencyStep.h"
#include "MachineGunProfile.h"
#include "LevelClearController.h"
#include "FlashBallEffects.h"
#include "SmokeEffects.h"
namespace downwell {
// Persistent subset currently modeled by the port. ResetPlayer calls
// scrInitialize, which resets defaults then reloads records through scrLoadSave.
// Callers supply that snapshot; a new run must not erase it or count death twice.
struct RunRecords {
    std::uint64_t lifetimeGems = 0;
    std::uint32_t bestRunGems = 0;
    std::uint32_t bestCombo = 0;
};

// Shared ownership and reset boundary for the implemented run-level systems.
// World/room instances, player initialization, RNG reseeding and platform
// effects remain separate; this is not a whole-game restart by itself.
struct RunSession {
    RunGemStatistics gems;
    RunClock clock;
    RunCombo combo;
    RunStage stage;
    DeathResults results;
    AmmoHudMotion ammoHud;
    AmmoGaugeRows ammoGauge;
    Consumables consumables;
    UpgradeInventory upgrades;
    ShopWorld shops;
    // Room-owned effects are reset on run/room retirement, not on a repaint.
    FlashBallEffects<32> flashEffects;
    SmokeEffects<32> smokeEffects;
    bool effectsSucceeded=true;
    void advanceSmokeAnimation(){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.advanceAnimation(smokeEffects.beginPhase());
    }
    void stepSmoke(SmokeEffects<32>::Phase phase,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.step(phase,random);
    }
    void collideSmokeShopWalls(){
        collideSmokeWalls([](const ContactBounds&,auto){return true;});
    }
    template<class Terrain> void collideSmokeWalls(Terrain terrain){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.collideWalls(smokeEffects.beginPhase(),
            [&](const ContactBounds& bounds,auto emit){
                return terrain(bounds,emit) && shops.visitWallContacts(bounds,emit);
            });
    }
    bool emitSmokeBurst(MotionBinary64 x,MotionBinary64 y,GameMakerRandom& random){
        if(effectsSucceeded)effectsSucceeded=smokeEffects.skeletonBurst(x,y,random);
        return effectsSucceeded;
    }
    SideCameraLockers sideCameraLockers;
    bool gemHigh = false;
    void selectWeaponLevel(const CurrencyAccount& currency) {
        gemHigh = machineGunProfileForStreak(currency.streak).level != 0;
    }
    // Pickup-only upgrade effects: nonGemGainGem (combo rewards) does not
    // invoke scrRechargeGem. Ownership is sampled at collection, not birth.
    template<class Solid, class Random>
    CurrencyStepResult collectCurrency(GemMotion& motion, bool large, unsigned amount,
        PlayerMotion& player, CurrencyAccount& currency, Solid solid, Random& random) {
        const auto result = stepCurrency(motion, large, amount,
            player.frameStartXFixed(), player.frameStartYFixed(),
            player.offsetXFixed(), player.offsetYFixed(), currency, solid, random,
            upgrades.levels[20] != 0);
        if (result == CurrencyStepResult::collected && upgrades.levels[4])
            player.rechargeFromGem(amount);
        return result;
    }
    // Shop Create snapshots prices from run ownership; acquiring a card later
    // does not reprice already-created stock. Other style/difficulty modes
    // remain separate from the current usual-style, normal run adapter.
    template<class Range, class Choice>
    ShopWorld::Instance* createShop(int x, int y, Range random, Choice chooseFarewell, int health) {
        return shops.create(x, y, random, chooseFarewell, health, stage.area > 0 ? stage.area : 1,
                            false, 0, upgrades.levels[14] != 0);
    }
    LevelExit levelExit;
    GoalTimeField goalField;
    bool levelExitPresent = false;
    bool goalStopped = false, exitControlsLocked = false;
    bool exitDitherRequested = false, levelResultRequested = false;
    ExitDither exitDither;
    LevelClearController levelClear;
    LevelClearPresentation::Frame levelClearFrame;
    bool levelClearActive = false, nextLevelRequested = false;

    // One logical Draw phase per gameplay update, not per renderer repaint.
    // The source consumes field RNG even outside its camera drawing bounds.
    void gameplayPresentationStep(GameMakerRandom& random, bool timeStopped = false) {
        if (levelExitPresent) goalField.advanceDraw(timeStopped, random);
        results.presentationStep();
    }

    // Called only after the backend retires the old room's display/DMA.
    // No run reset: upgrades, currency, records and stage survive this boundary.
    bool enterLevelClear(unsigned style, bool heartGoal, int health,
                         PlayerMotion& player, GameMakerRandom& random) {
        if (!levelResultRequested || levelClearActive) return false;
        if (!levelClear.create(style, heartGoal, health, upgrades, player, random)) return false;
        levelClearActive = true;
        flashEffects.clear();
        smokeEffects.clear();
        // Room creation is followed by Draw before the next Step/Alarm phase.
        levelClearFrame = levelClear.presentation.advance(stage.area, stage.level,
            levelClear.selection.next, levelClear.selection.visible);
        levelResultRequested = exitDitherRequested = false;
        levelExitPresent = false;
        exitDither = {};
        nextLevelRequested = false;
        return true;
    }
    template<class Emit>
    bool stepLevelClear(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
                        CurrencyAccount& currency, PlayerMotion& player, int& health,
                        GameMakerRandom& random, Emit emit) {
        if (!levelClearActive || nextLevelRequested) return false;
        exitDither.advanceAlarm();
        const auto effect = [&](LevelClearSelection::Effect event, unsigned value) {
            if (event == LevelClearSelection::Effect::dither) {
                exitDitherRequested = true;
                exitDither.create(0, levelClear.motion.viewY());
            }
            if (event == LevelClearSelection::Effect::nextLevel) nextLevelRequested = true;
            emit(event, value);
        };
        levelClear.advanceAlarms(effect);
        const bool ok = levelClear.step(confirm, leftPressed, rightPressed, heartGoal,
            upgrades, consumables, currency, player, health, combo.healthReserve, random, effect);
        exitDither.step(levelClear.motion.viewY(), 284);
        levelClearFrame = levelClear.presentation.advance(stage.area, stage.level,
            levelClear.selection.next, levelClear.selection.visible);
        return ok;
    }
    // The loader consumes this once before constructing rmMain, matching
    // scrNextLevel(0). It must not use restart(), which clears run ownership.
    bool advanceAfterLevelClear() {
        if (!levelClearActive || !nextLevelRequested) return false;
        stage.advance();
        levelClearActive = nextLevelRequested = false;
        goalStopped = exitControlsLocked = false;
        exitDitherRequested = false;
        exitDither = {};
        return true;
    }

    void advanceLevelExitAlarms(int viewX, int viewY) {
        if (!levelExitPresent) return;
        exitDither.advanceAlarm();
        levelExit.advanceAlarms([&](LevelExit::Effect effect, int, int) {
            if (effect == LevelExit::Effect::ditherTransition) {
                exitDitherRequested = true;
                exitDither.create(viewX, viewY);
            }
            if (effect == LevelExit::Effect::resultRoom) levelResultRequested = true;
        });
    }
    void collideLevelExit(int playerX, int playerY, bool dead) {
        if (levelExitPresent && levelExit.meetsPlayer(playerX, playerY))
            levelExit.collide(dead, goalStopped, exitControlsLocked);
    }

    template<class Camera>
    void createLevelExit(int roomWidth, int y, GameMakerRandom& random, Camera& camera) {
        goalStopped = exitControlsLocked = false;
        exitDitherRequested = levelResultRequested = false;
        exitDither = {};
        levelExit.create(roomWidth, y, [&](LevelExit::Effect effect, int x, int childY) {
            if (effect == LevelExit::Effect::roomEndSetter) camera.setLevelExit(childY);
            else if (effect == LevelExit::Effect::goalTimeField) goalField.create(x, childY, random);
        });
        levelExitPresent = true;
    }

    RunRecords records() const {
        return {gems.lifetime, gems.record, combo.state.record};
    }
    void reset(const RunRecords& saved = {}) {
        gems = {};
        gems.lifetime = saved.lifetimeGems;
        gems.record = saved.bestRunGems;
        clock = {};
        combo = {};
        combo.state.record = saved.bestCombo;
        stage = {};
        results = {};
        ammoHud = {};
        ammoGauge = {};
        consumables = {};
        upgrades = {};
        shops.reset();
        flashEffects.clear();
        smokeEffects.clear();
        effectsSucceeded=true;
        sideCameraLockers.clear();
        gemHigh = false;
        levelExit = {};
        goalField = {};
        levelExitPresent = false;
        goalStopped = exitControlsLocked = false;
        exitDitherRequested = levelResultRequested = false;
        exitDither = {};
        levelClear = {};
        levelClearFrame = {};
        levelClearActive = nextLevelRequested = false;
    }
    ResultsDestination restart(DeathMenuSequence::Action action, const RunRecords& saved) {
        const auto destination = resultsDestination(action);
        if (destination == ResultsDestination::none) return destination;
        reset(saved);
        if (destination == ResultsDestination::gameplay) stage.advance(1);
        return destination;
    }
};
} // namespace downwell
#endif
