#include "core/DeathMenuProgress.h"
#include "core/RunGemStatistics.h"
#include <cstdlib>
#include <iostream>
static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    downwell::RunGemStatistics stats;
    require(!stats.observeDeathMovement(false, 80), "no save at health depletion/publication");
    require(stats.observeDeathMovement(true, 80) && stats.lifetime == 80 && stats.record == 80,
            "first death movement saves run once");
    require(!stats.observeDeathMovement(true, 84) && stats.lifetime == 80 && stats.savedRunGems == 80,
            "post-death gems do not retroactively change saved statistics");
    stats.beginRun();
    require(stats.observeDeathMovement(true, 20) && stats.lifetime == 100 && stats.record == 80,
            "new run preserves lifetime and best-run record");
    stats.beginRun();
    require(stats.observeDeathMovement(true, 90) && stats.lifetime == 190 && stats.record == 90,
            "new high score replaces record");
    using downwell::DeathMenuProgress;
    using downwell::DeathMenuSequence;
    DeathMenuProgress progress;
    DeathMenuSequence menu;
    require(progress.initialize(84, 84, false), "initialize first run");
    progress.advance(menu);
    require(progress.animatedTotal == 0, "hidden progress does not fill");
    menu.resultShown = 6;
    menu.beginStep(false, false, false);
    for (unsigned tick = 1; tick < 200 && !menu.sequenceOver; ++tick) {
        const auto before = progress.animatedTotal;
        const auto frame = progress.advance(menu);
        require(frame.displayedTotal == before, "display captures value before filling");
        if (before < 84) {
            require(progress.animatedTotal == before + (84 - before + 14) / 15,
                    "remaining total uses source ceiling division");
            require(frame.fillSound == (tick % 3 == 0) && !frame.completedSound,
                    "fill sound every third increment, completion on subsequent phase");
        } else require(frame.completedSound, "completion publishes once after reaching total");
    }
    require(menu.sequenceOver && progress.animatedTotal == 84, "first run completes");
    require(!progress.advance(menu).completedSound, "completion sound is one-shot");

    menu = {};
    menu.resultShown = 6;
    menu.beginStep(false, false, false);
    require(progress.initialize(2010, 1020, false), "initialize crossing two goals");
    auto frame = progress.advance(menu);
    require(frame.displayedTotal == 990 && progress.animatedTotal == 1024 && frame.unlockedGoal == -1,
            "increment may overshoot; unlock is not retroactive");
    frame = progress.advance(menu);
    require(frame.unlockedGoal == 0 && progress.animatedTotal == 1000 && menu.goalNotice == 1,
            "next phase clamps to threshold and starts notice");
    for (int i = 0; i < 60; ++i) {
        menu.advanceAlarms();
        require(progress.advance(menu).unlockedGoal == -1 && progress.animatedTotal == 1000,
                "notice holds total and never duplicates unlock request");
    }
    progress.acknowledge(menu.beginStep(true, false, false));
    require(progress.goal == 1 && !menu.goalPassed && menu.inputDisabled,
            "acknowledgement advances goal and preserves sequence input lock");
    unsigned unlocks = 1;
    for (int i = 0; i < 1000 && !menu.sequenceOver; ++i) {
        auto effects = menu.beginStep(menu.goalNotice >= 3, false, false);
        progress.acknowledge(effects);
        menu.advanceAlarms();
        frame = progress.advance(menu);
        if (frame.unlockedGoal >= 0) {
            require(frame.unlockedGoal == 1, "second notice is the second threshold");
            ++unlocks;
        }
    }
    require(unlocks == 2 && menu.sequenceOver && progress.animatedTotal == 2010,
            "multiple unlocks resume filling to actual lifetime total");
    require(progress.separatorTotal == 990, "prior total marker remains unchanged");
    for (unsigned goal = 0; goal < 41; ++goal) {
        menu = {};
        menu.resultShown = 6;
        menu.beginStep(false, false, false);
        const auto threshold = DeathMenuProgress::goals[goal];
        require(progress.initialize(threshold, 1, false), "initialize exact unlock boundary");
        progress.advance(menu);
        require(!menu.goalPassed && !menu.sequenceOver,
                "reaching an exact threshold does not complete early");
        frame = progress.advance(menu);
        require(frame.unlockedGoal == static_cast<int>(goal), "all 41 threshold indices preserved");
        for (int i = 0; i < 60; ++i) menu.advanceAlarms();
        progress.acknowledge(menu.beginStep(true, false, false));
        require(progress.goal == goal + 1 && progress.maxed == (goal == 40),
                "only the final unlock acknowledgement sets maxed");
        require(progress.advance(menu).completedSound && menu.sequenceOver,
                "exact-boundary run completes after acknowledgement");
    }
    require(progress.initialize(500000, 0, true) && progress.goal == 41,
            "already-earned thresholds skipped on entry");
    require(progress.initialize(99999999998ULL, 0, true), "sentinel needs wider than 32 bits");
    require(!progress.initialize(99999999999ULL, 0, true), "invalid total rejected without table overrun");
    require(progress.initialize(99999999999ULL, 100, true) &&
            progress.total == 99999999990ULL && progress.animatedTotal == 99999999899ULL,
            "source caps lifetime total only after computing previous run total");
    require(progress.initialize(10, 20, false) && progress.animatedTotal == 0,
            "negative inferred previous total clamps to zero");
    std::cout << "shared death-menu progress tests passed\n";
}
