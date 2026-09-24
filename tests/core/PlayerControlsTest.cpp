#include "core/PlayerControls.h"
#include "core/PlayerLifeRules.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

static void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}

template<class Math> void checkControls() {
    using Number = typename Math::Number;
    Number speed = 0;
    int facing = -1;
    for (int i = 0; i < 40; ++i)
        downwell::applyPlayerHorizontalInput<Math>(speed, facing, true, false, true);
    require(speed == Math::ratio(2) && facing == 1, "ground speed uses original cap");
    downwell::applyPlayerHorizontalInput<Math>(speed, facing, true, true, false);
    require(speed < 0 && facing == -1, "ground reversal uses two-pixel impulse");
    for (int i = 0; i < 40; ++i)
        downwell::applyPlayerHorizontalInput<Math>(speed, facing, false, false, true);
    require(speed == Math::ratio(5, 2), "air speed uses original cap");
    downwell::applyPlayerHorizontalInput<Math>(speed, facing, false, true, true);
    require(speed == 0 && facing == 1, "both inputs stop, with right-facing source order");
    speed = Math::ratio(2);
    downwell::applyPlayerHorizontalInput<Math>(speed, facing, true, false, false);
    require(std::fabs(static_cast<double>(speed) / Math::ratio(1) - 1.6) < .004,
            "ground friction is four tenths, subject only to numeric storage");
    downwell::applyPlayerHorizontalInput<Math>(speed, facing, false, true, false, true);
    require(speed == 0 && facing == 1, "noControl stops without changing facing");
}

int main() {
    for (unsigned remaining = 0; remaining <= 90; ++remaining)
        require(downwell::playerDamageVisible(remaining) == (remaining % 2 == 0),
                "default damage flash alternates and both endpoints remain visible");
    downwell::Input input;
    input.left = input.right = input.fire = input.quit = input.pause = true;
    const auto alive = downwell::playerGameplayInput(input, 1);
    require(alive.left && alive.right && alive.fire && alive.quit && alive.pause,
            "living player input remains unchanged");
    for (int health : {0, -1}) {
        const auto dead = downwell::playerGameplayInput(input, health);
        require(!dead.left && !dead.right && !dead.fire && dead.quit && dead.pause,
                "depleted health disables gameplay input but preserves menu/platform requests");
    }
    checkControls<downwell::FixedPlayerMath>();
    checkControls<downwell::FloatPlayerMath>();
    std::cout << "shared player control tests passed\n";
}
