#ifndef DOWNWELL_CORE_RUN_GEM_STATISTICS_H
#define DOWNWELL_CORE_RUN_GEM_STATISTICS_H
#include <cstdint>
namespace downwell {
// Gem subset of saveStats, invoked by scrPlMovement on first death movement.
// Save-file I/O and other records are separate; neither belongs in simulation.
struct RunGemStatistics {
    std::uint64_t lifetime = 0;
    std::uint32_t record = 0;
    std::uint32_t savedRunGems = 0;
    bool recorded = false;
    void beginRun() { recorded = false; savedRunGems = 0; }
    bool observeDeathMovement(bool started, std::uint32_t runGems) {
        if (!started || recorded) return false;
        savedRunGems = runGems;
        lifetime += runGems;
        if (runGems > record) record = runGems;
        recorded = true;
        return true;
    }
};
} // namespace downwell
#endif
