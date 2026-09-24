#ifndef DOWNWELL_CORE_RUN_SESSION_H
#define DOWNWELL_CORE_RUN_SESSION_H
#include "RunGemStatistics.h"
#include "RunClock.h"
#include "RunCombo.h"
#include "RunStage.h"
#include "DeathResults.h"
#include "ResultsDestination.h"
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
