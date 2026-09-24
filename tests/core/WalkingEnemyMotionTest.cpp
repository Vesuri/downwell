#include "core/WalkingEnemyMotion.h"
#include <cstdlib>
#include <iostream>
static void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    using downwell::WalkingEnemyMotion;
    const auto empty = [](long, long) { return false; };
    WalkingEnemyMotion falling;
    falling.initialize(0, 0, 0, 0, false);
    for (long n = 1; n <= 30; ++n) {
        falling.step(false, true, true, empty);
        require(falling.vy == n * 256 && falling.y == n * (n + 1) * 128,
                "shared walker retains exact gravity and integration fractions");
    }
    for (int n = 0; n < 20; ++n) falling.step(false, true, true, empty);
    require(falling.vy == 7 * 1280, "source maxgrav caps walking enemies");
    for (bool turtle : {false, true}) {
        WalkingEnemyMotion edge;
        edge.initialize(10, 20, 640, 0, true);
        bool first = true;
        edge.step(turtle, false, false, [&](long x, long y) {
            if (first) {
                require(x == (turtle ? 22 : 18) * 1280 && y == 21 * 1280,
                        "source rectangle edge probes use eight/twelve-pixel offsets");
                first = false;
            }
            return false;
        });
        require(edge.vx == -640 && edge.x == 10 * 1280 && edge.y == 20 * 1280 &&
                edge.publishedX == 10 && edge.publishedY == 20,
                "offscreen rules run but final integration/publication stays gated");
    }
    WalkingEnemyMotion wall;
    wall.initialize(0, 0, 2 * 1280, 0, false);
    wall.step(false, true, true, [](long x, long) { return x >= 2 * 1280; });
    require(wall.x == -1280 && wall.vx == -2 * 1280,
            "source walks to contact, reverses, then integrates reversed velocity");
    WalkingEnemyMotion land;
    land.initialize(0, 0, 0, 6 * 1280, false);
    const auto floor = [](long, long y) { return y >= 5 * 1280; };
    land.step(false, true, true, floor);
    require(land.y == 4 * 1280 && land.vy == 4 * 1280 && !land.grounded,
            "landing approach sets collision distance without prematurely setting grounded");
    land.step(false, true, true, floor);
    require(land.y == 4 * 1280 && land.vy == 0 && land.grounded,
            "next source Step recognizes one-pixel support and grounds the actor");
    WalkingEnemyMotion paused;
    paused.initialize(0, 0, 320, 0, false);
    paused.step(true, false, true, empty);
    require(paused.x == 0 && paused.y == 256 && paused.vx == 320,
            "paused turtle retains horizontal velocity and still falls");
    paused.step(true, true, true, empty);
    require(paused.x == 320, "walking phase resumes horizontal integration");
    std::cout << "Shared walking-enemy motion tests passed\n";
}
