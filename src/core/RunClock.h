#ifndef DOWNWELL_CORE_RUN_CLOCK_H
#define DOWNWELL_CORE_RUN_CLOCK_H
#include <cstdint>
namespace downwell {
// objPlayer_n Begin Step, after movement and before end-of-event death
// publication. This counts game updates, not PAL wall-clock milliseconds.
// Reset with scrGlobalStuff at a new run, not at every player/room creation.
struct RunClock {
    std::uint32_t ticks = 0;
    void advance(bool inMainRoom, bool publishedDeath, bool goalStop = false,
                 bool bossComplete = false, bool inGoalTimeField = false) {
        if (inMainRoom && !publishedDeath && !goalStop && !bossComplete && !inGoalTimeField)
            ++ticks;
    }
};
} // namespace downwell
#endif
