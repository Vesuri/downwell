#include "core/SnailMotion.h"
#include "core/SnailHitState.h"
#include <cstdlib>
#include <iostream>
static void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    struct Random {
        unsigned calls = 0;
        unsigned randomIntegerSmall(unsigned upper) {
            require(upper == 30, "snail hit uses source random-range width");
            ++calls; return 17;
        }
    } random;
    downwell::SnailHitState hit;
    hit.pending = true;
    require(hit.consume(random) == -1 && random.calls == 0, "collision waits for next enemy Step");
    hit.advanceFrame();
    require(hit.consume(random) == 67 && hit.alarm2 == 4 && hit.stunned,
            "first hit adds five to the disabled minus-one alarm");
    hit.pending = true;
    hit.advanceFrame();
    require(hit.consume(random) == 67 && hit.alarm2 == 8 && random.calls == 2,
            "repeated hits extend the remaining alarm, rather than resetting it");
    for (int i = 0; i < 8; ++i) hit.advanceFrame();
    require(!hit.stunned && hit.alarm2 == 0, "Alarm 2 clears hit-stun on its zero frame");
    hit.advanceFrame();
    require(hit.alarm2 == -1, "zero alarm disables on the following frame");
    using downwell::SnailMotion;
    const auto rightWall = [](int x, int) { return x >= 8; };
    SnailMotion snail;
    snail.initialize(0, 0, true);
    for (int i = 0; i < 1000; ++i) require(snail.step(rightWall), "snail stays attached");
    require(snail.y == 100L * 1280 && snail.vy == 128 && snail.facing == 1 && snail.verticalFlip == -1,
            "a thousand steps preserve exact .1 displacement and descending flip");
    SnailMotion left;
    left.initialize(0, 0, false);
    for (int i = 0; i < 1000; ++i)
        require(left.step([](int x, int) { return x <= -8; }), "left wall supports snail");
    require(left.y == -100L * 1280 && left.facing == -1 && left.verticalFlip == 1,
            "negative motion and left attachment retain source precision");
    SnailMotion paused;
    paused.initialize(0, 0, true);
    require(paused.step(rightWall, true) && paused.attached && paused.y == 0,
            "attachment precedes the time-stop movement guard");
    SnailMotion embedded;
    embedded.initialize(0, 0, true);
    require(!embedded.step([](int, int) { return true; }), "embedded spawn is destroyed");
    require(!embedded.step([](int, int) { return false; }), "detached spawn is destroyed");
    SnailMotion edge;
    edge.initialize(0, 0, true);
    for (int i = 0; i < 120 && edge.vy > 0; ++i)
        require(edge.step([](int x, int y) { return x >= 8 && y < 16; }), "finite wall retains actor");
    require(edge.vy < 0 && edge.verticalFlip == 1, "loss of point support reverses movement");
    std::cout << "Shared snail motion tests passed\n";
}
