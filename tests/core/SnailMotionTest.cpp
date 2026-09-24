#include "core/SnailMotion.h"
#include <cstdlib>
#include <iostream>
static void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
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
