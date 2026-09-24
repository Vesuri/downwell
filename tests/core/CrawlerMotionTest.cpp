#include "core/CrawlerMotion.h"
#include <cstdlib>
#include <iostream>
static void require(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    using downwell::CrawlerMotion;
    const auto empty = [](long, long) { return false; };
    CrawlerMotion published;
    published.initialize(0, 0);
    require(published.imageDirection == 0, "Create retains the default image angle");
    published.latched = true;
    published.alarm(false, false, empty);
    require(published.direction == 1 && published.imageDirection == 0,
            "Alarm changes movement direction without publishing image_angle");
    published.step(false, false, empty);
    require(published.imageDirection == 3,
            "Step publishes the counterclockwise angle even when outside view");
    published.latched = true; published.direction = 3;
    published.step(false, false, empty);
    require(published.imageDirection == 5,
            "shared presentation retains the source's unnormalized 450-degree angle");
    CrawlerMotion falling;
    falling.initialize(0, 0);
    for (int i = 0; i < 40; ++i) falling.step(true, true, empty);
    require(falling.vy == 40 * 256L && falling.y == 820 * 256L,
            "crawler gravity is exactly .2, including above seven pixels/frame");
    const long before = falling.y;
    falling.step(true, false, empty);
    require(falling.y == before, "invisible unlatched crawler pauses falling");
    for (int cw = 0; cw < 2; ++cw) {
        for (unsigned dir = 0; dir < 4; ++dir) {
            CrawlerMotion c;
            c.initialize(0, 0); c.direction = dir; c.latched = true;
            c.alarm(cw, false, empty);
            require(c.direction == (dir + (cw ? 3 : 1)) % 4 && !c.latched && c.x == 0 && c.y == 0,
                    "outside corner turns toward lost support, unlatches, and obeys visibility");
            c.initialize(0, 0); c.direction = dir; c.latched = true;
            c.alarm(cw, true, [](long, long) { return true; });
            require(c.direction == (dir + (cw ? 1 : 3)) % 4 && c.latched,
                    "inside corner turns away from obstacle in both traversal directions");
        }
    }
    CrawlerMotion floor;
    CrawlerMotion fractional;
    fractional.initialize(0, 0);
    fractional.x = 100; fractional.y = 200; fractional.latched = true;
    fractional.alarm(true, true, [](long, long) { return true; });
    require(fractional.x == 100 && fractional.y == 200 - 1280,
            "alarm movement retains subpixel position instead of snapping to published x/y");
    floor.initialize(0, 0);
    const auto meeting = [](long x, long y) {
        return CrawlerMotion::meetsTiles(x, y, [](int px, int py) { return py >= 16 || px >= 16; });
    };
    for (int i = 0; i < 30 && !floor.latched; ++i) floor.step(true, true, meeting);
    require(floor.latched && floor.pixelY() == 11 && floor.vy == 0,
            "recovered ten-pixel mask lands immediately above the floor");
    for (int i = 0; i < 14; ++i) floor.alarm(true, true, meeting);
    require(floor.pixelX() == 11 && floor.pixelY() < 11 && floor.direction == 1,
            "clockwise crawler follows floor into upward wall travel");
    require(!CrawlerMotion::inView(0, 0, 0, -4096, 4096, 4096) &&
            CrawlerMotion::inView(1, 2, 0, 0, 8192, 4096, 64),
            "source visibility uses strict horizontal bounds and lower Step margin");
    std::cout << "Shared crawler motion tests passed\n";
}
