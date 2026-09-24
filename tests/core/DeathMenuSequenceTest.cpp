#include "core/DeathMenuSequence.h"
#include <cstdlib>
#include <iostream>
static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    using Menu = downwell::DeathMenuSequence;
    Menu menu;
    const int reveals[] = {45, 55, 67, 81, 97, 127, 147};
    unsigned next = 0;
    for (int tick = 1; tick <= 180; ++tick) {
        menu.beginStep(false, false, false);
        const unsigned sound = menu.advanceAlarms();
        const bool reveal = next < 7 && tick == reveals[next];
        require(sound == (reveal ? 1u : 0u), "exact result reveal alarm schedule");
        if (reveal) {
            require(menu.resultShown == static_cast<int>(next), "rows reveal in order");
            require(!menu.visible[next] && menu.rowOffset[next] == 4,
                    "alarm does not publish row visibility before next Begin Step");
            ++next;
        }
    }
    require(next == 7 && menu.visible[6] && menu.rowOffset[6] == 0,
            "all rows settle without automatically completing gem progression");
    require(!menu.sequenceOver && menu.cursorBounceTenths == 12,
            "progress completion is separate and cursor bounce preserves overshoot");
    Menu skip;
    require(skip.beginStep(true, true, false).action == Menu::Action::None,
            "initial press skips results, not the menu");
    require(skip.resultShown == 6 && !skip.visible[5], "skip publishes rows next update");
    skip.advanceAlarms();
    skip.beginStep(false, false, false);
    skip.sequenceOver = true; // Results Draw finishes filling, not the renderer.
    for (int i = 1; i < 20; ++i) {
        require(skip.beginStep(true, false, false).action == Menu::Action::None,
                "confirmation is blocked before the twentieth alarm update");
        skip.advanceAlarms();
    }
    require(skip.beginStep(true, false, false).action == Menu::Action::Retry,
            "retry accepted after lockout");
    require(skip.beginStep(true, false, true).action == Menu::Action::StyleSelect,
            "navigation precedes confirmation");
    require(skip.beginStep(true, false, true).action == Menu::Action::Surface,
            "Windows exposes the third choice");
    require(skip.beginStep(true, false, true).action == Menu::Action::Retry,
            "right wraps to retry");
    skip.beginStep(false, true, false);
    const auto both = skip.beginStep(false, true, true);
    require(skip.cursor == 2 && both.navigationSounds == 2,
            "simultaneous edges cancel before wrapping and emit both sounds");
    skip.sequenceOver = false;
    skip.startUnlockNotice();
    for (int tick = 1; tick <= 75; ++tick) {
        const unsigned sounds = skip.advanceAlarms();
        require(sounds == (tick == 45 ? 2u : 0u), "unlock notice sound timing");
        require(skip.goalNotice == (tick < 45 ? 1 : tick < 60 ? 2 : 3),
                "unlock notice advances after 45 then 15 updates");
    }
    const auto acknowledge = skip.beginStep(true, false, false);
    require(acknowledge.acknowledgeUnlock && !skip.goalPassed && skip.goalNotice == 0,
            "completed notice emits one unlock acknowledgement");
    require(skip.inputDisabled && skip.inputAlarm == 20,
            "acknowledgement also restarts lockout while results are incomplete");
    std::cout << "shared death-menu sequence tests passed\n";
}
