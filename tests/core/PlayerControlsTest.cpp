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

    // Independent source-order transcription: left, right, idle, both-key
    // override. Compare rules within each storage adapter, not float to Q8.
    for (int initial=-1536; initial<=1536; ++initial)
        for (unsigned flags=0; flags<16; ++flags) {
            const bool ground=flags&1, left=flags&2, right=flags&4, locked=flags&8;
            Number actual=Number(initial)*Math::ratio(1)/256;
            Number expected=actual;
            int actualFacing=-1,expectedFacing=-1;
            const auto cap=ground?Math::ratio(2):Math::ratio(5,2);
            const auto boost=Math::ratio(ground?2:1);
            if(locked) expected=0;
            else {
                if(left) {
                    expected-=Math::ratio(1,5);
                    if(expected>0)expected-=boost;
                    if(expected < -cap)expected=-cap;
                    expectedFacing=-1;
                }
                if(right) {
                    expected+=Math::ratio(1,5);
                    if(expected<0)expected+=boost;
                    if(expected>cap)expected=cap;
                    expectedFacing=1;
                }
                if(!left&&!right) {
                    const int sign=(expected>0)-(expected<0);
                    expected-=sign*(ground?Math::ratio(2,5):Math::ratio(1,10));
                    const auto magnitude=expected<0?-expected:expected;
                    if(magnitude<Math::ratio(3,10))expected=0;
                }
                if(left&&right)expected=0;
            }
            downwell::applyPlayerHorizontalInput<Math>(actual,actualFacing,ground,left,right,locked);
            require(actual==expected && actualFacing==expectedFacing,
                    "horizontal rules preserve source branch order, one-sided caps and absolute friction threshold");
        }
    for(int sign : {-1,1}) {
        speed=sign*Math::ratio(1,32);
        downwell::applyPlayerHorizontalInput<Math>(speed,facing,true,false,false);
        require(speed*sign<0,"small grounded speed can reverse under source friction");
    }
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
