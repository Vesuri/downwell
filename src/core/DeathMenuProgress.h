#ifndef DOWNWELL_CORE_DEATH_MENU_PROGRESS_H
#define DOWNWELL_CORE_DEATH_MENU_PROGRESS_H
#include "DeathMenuSequence.h"
#include <cstdint>

namespace downwell {
// Explicit once-per-update results phase, corresponding to DeathMenu Draw
// blocks 16--48. Rendering consumes its snapshot; drawing it twice must not
// advance currency or grant another unlock. Persistence is an external owner.
struct DeathMenuProgress {
    inline static constexpr std::uint64_t goals[42] = {
        1000,2000,3000,4000,5000,7500,10000,12500,15000,17500,
        20000,25000,30000,35000,40000,45000,50000,55000,60000,65000,
        70000,75000,80000,85000,90000,95000,100000,110000,120000,130000,
        140000,150000,175000,200000,225000,250000,300000,350000,400000,
        450000,500000,99999999999ULL
    };
    struct Frame {
        std::uint64_t displayedTotal = 0;
        std::uint64_t nextGoal = 1000;
        std::uint64_t previousGoal = 0;
        int unlockedGoal = -1; // one-shot request for unlockNotice(index)
        bool fillSound = false; // 328, pitch uses pre-increment progress
        bool completedSound = false; // 330
    };
    std::uint64_t total = 0;
    std::uint64_t animatedTotal = 0;
    std::uint64_t separatorTotal = 0;
    unsigned goal = 0;
    unsigned filling = 0;
    bool maxed = false;

    // Create computes the previous total before unlockGoalNum caps totalGems.
    // Reject only a previous total outside the table (the source would overrun).
    // The caller must persist `total` if the source cap changes its saved value.
    bool initialize(std::uint64_t lifetimeGems, std::uint64_t runGems,
                    bool unlockMax) {
        const auto previous = lifetimeGems > runGems ? lifetimeGems - runGems : 0;
        if (previous >= goals[41]) return false;
        *this = {};
        total = lifetimeGems >= goals[41] ? 99999999990ULL : lifetimeGems;
        animatedTotal = previous;
        separatorTotal = animatedTotal;
        while (goals[goal] <= animatedTotal) ++goal;
        maxed = unlockMax;
        return true;
    }
    void acknowledge(const DeathMenuSequence::Effects& effects) {
        if (!effects.acknowledgeUnlock) return;
        if (goal < 41) ++goal;
        if (goal >= 41) maxed = true;
    }
    Frame advance(DeathMenuSequence& menu) {
        Frame frame;
        // Crossing is checked BEFORE filling. A last increment can overshoot;
        // the following phase clamps at the goal and starts the notice.
        if (animatedTotal >= goals[goal] && !menu.goalPassed) {
            animatedTotal = goals[goal];
            menu.startUnlockNotice();
            frame.unlockedGoal = static_cast<int>(goal);
        }
        frame.displayedTotal = animatedTotal;
        frame.nextGoal = goals[goal];
        frame.previousGoal = goal ? goals[goal - 1] : 0;
        if (menu.visible[5] && menu.visible[6] && !menu.goalPassed) {
            if (animatedTotal < total) {
                const auto remainingStep = (total - animatedTotal + 14) / 15;
                const auto goalStep = (frame.nextGoal - frame.previousGoal + 29) / 30;
                animatedTotal += remainingStep < goalStep ? remainingStep : goalStep;
                frame.fillSound = (++filling % 3) == 0;
            } else {
                animatedTotal = total;
                frame.completedSound = !menu.sequenceOver;
                menu.sequenceOver = true;
            }
        }
        return frame;
    }
};
} // namespace downwell
#endif
