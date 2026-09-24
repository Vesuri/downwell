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
#include "LevelExit.h"
#include "GoalTimeField.h"
#include "ExitDither.h"
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
    ShopWorld shops;
    LevelExit levelExit;
    GoalTimeField goalField;
    bool levelExitPresent = false;
    bool goalStopped = false, exitControlsLocked = false;
    bool exitDitherRequested = false, levelResultRequested = false;
    ExitDither exitDither;

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
        shops = {};
        levelExit = {};
        goalField = {};
        levelExitPresent = false;
        goalStopped = exitControlsLocked = false;
        exitDitherRequested = levelResultRequested = false;
        exitDither = {};
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
