#include "core/StandardBulletStep.h"
#include <cstdlib>
#include <iostream>

static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}

int main() {
    using downwell::StandardBulletPath;
    using downwell::stepStandardBullet;
    int y = 10, enemies = 0, walls = 0;
    const bool alive = stepStandardBullet(20, y,
        [&](const StandardBulletPath& path) {
            ++enemies;
            require(path.x == 20 && path.fromY == 10 && path.toY == 18,
                    "enemy query receives the complete motion segment");
            return path.intersects(20, 21, 18, 19);
        }, [&](const StandardBulletPath& path) {
            ++walls;
            return path.intersects(20, 21, 11, 12);
        });
    require(!alive && y == 18 && enemies == 1 && walls == 0,
            "enemy at endpoint suppresses nearer wall processing");

    y = 10;
    require(!stepStandardBullet(20, y,
        [](const StandardBulletPath&) { return false; },
        [&](const StandardBulletPath& path) {
            ++walls;
            return path.intersects(20, 21, 10, 11);
        }) && walls == 1 && y == 18,
        "wall at initial position consumes bullet when no enemy is hit");
    y = 10;
    require(stepStandardBullet(20, y,
        [](const StandardBulletPath&) { return false; },
        [](const StandardBulletPath&) { return false; }) && y == 18,
        "unobstructed usual bullet advances eight pixels");

    const StandardBulletPath path{20, 10, 18};
    require(path.intersects(20, 21, 10, 11) && path.intersects(20, 21, 18, 19),
            "both segment endpoints are included");
    require(!path.intersects(19, 20, 10, 19) &&
            !path.intersects(21, 22, 10, 19) &&
            !path.intersects(20, 21, 9, 10) &&
            !path.intersects(20, 21, 19, 20),
            "half-open object bounds exclude adjacent pixel cells");
    std::cout << "Shared standard bullet step tests passed\n";
}
