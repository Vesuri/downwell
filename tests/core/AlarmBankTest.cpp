#include "core/AlarmBank.h"
#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testResetAndCountdown() {
    downwell::AlarmBank alarms;
    for (std::size_t index = 0; index < downwell::AlarmBank::count; ++index)
        require(alarms.get(index) == -1, "alarms initialize inactive");

    alarms.set(0, 2);
    require(alarms.advance() == 0, "alarm does not fire early");
    require(alarms.get(0) == 1, "alarm counts down by one");
    const auto fired = alarms.advance();
    require(downwell::AlarmBank::fired(fired, 0), "alarm fires on reaching zero");
    require(alarms.get(0) == -1, "fired alarm becomes inactive");
    require(alarms.advance() == 0, "inactive alarm remains quiet");
}

void testSimultaneousAndRearm() {
    downwell::AlarmBank alarms;
    alarms.set(0, 1);
    alarms.set(7, 1);
    alarms.set(11, 2);
    const auto first = alarms.advance();
    require(downwell::AlarmBank::fired(first, 0), "alarm zero fires");
    require(downwell::AlarmBank::fired(first, 7), "alarm seven fires concurrently");
    require(!downwell::AlarmBank::fired(first, 11), "alarm eleven remains armed");

    // An event handler can rearm an alarm after dispatch.
    alarms.set(0, 3);
    const auto second = alarms.advance();
    require(downwell::AlarmBank::fired(second, 11), "alarm eleven fires next tick");
    require(alarms.get(0) == 2, "rearmed alarm resumes countdown");
}

void testZeroAndNegativeDoNotCount() {
    downwell::AlarmBank alarms;
    alarms.set(3, 0);
    alarms.set(4, -5);
    require(alarms.advance() == 0, "non-positive alarms do not dispatch");
    require(alarms.get(3) == 0, "zero alarm remains unchanged");
    require(alarms.get(4) == -5, "negative alarm remains unchanged");
}

} // namespace

int main() {
    testResetAndCountdown();
    testSimultaneousAndRearm();
    testZeroAndNegativeDoNotCount();
    std::cout << "alarm bank tests passed\n";
    return 0;
}
