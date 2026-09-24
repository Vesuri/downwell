#include "core/MainCameraMotion.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void require(bool value, const char* text) {
    if (!value) { std::cerr << text << '\n'; std::exit(1); }
}
int main() {
    for (long whole : {-3L, -2L, 0L, 1L, 2L, 1456L}) {
        for (long fraction : {-2049L, -2048L, -2047L, 0L, 2047L, 2048L, 2049L}) {
            downwell::MainCameraMotion pixel;
            pixel.x = pixel.y = whole * 4096 + fraction;
            const long expected = downwell::roundGameMakerCoordinate(pixel.x / 4096.0);
            require(pixel.pixelX() == expected && pixel.pixelY() == expected,
                    "published camera coordinates use exact Q12 nearest-even rounding");
            require(pixel.x == whole * 4096 + fraction,
                    "publication does not discard fractional camera easing state");
        }
    }
    downwell::MainCameraMotion camera;
    camera.step(240 * 256L, 176 * 256L);
    require(camera.targetX == 240L * 4096 && camera.targetY == 200L * 4096 &&
            camera.slowdownX == 10 && camera.slowdownY == 5,
            "shaft uses coincident horizontal clamps and source downward bias");
    camera.step(160 * 256L - 1, 800 * 256L);
    require(camera.x == 80L * 4096 && camera.targetY == 200L * 4096 && camera.slowdownY == 5,
            "left room retains vertical target and smoothing despite player descent");
    camera.step(160 * 256L, 900 * 256L);
    require(camera.targetX == 80L * 4096 && camera.targetY == 200L * 4096,
            "exact shaft boundary changes neither target");
    camera.step(160 * 256L + 1, 300 * 256L);
    require(camera.x == 240L * 4096 && camera.y == 324L * 4096 && camera.slowdownY == 1,
            "return from side room snaps both axes with source slowdown one");
    camera.step(320 * 256L + 1, 1000 * 256L);
    require(camera.x == 400L * 4096 && camera.targetY == 324L * 4096,
            "right room also retains vertical target");
    camera.step(320 * 256L, 1100 * 256L);
    require(camera.targetX == 400L * 4096, "right equality retains prior side target");
    camera.reset(240, 176);
    camera.targetY = 400L * 4096;
    camera.step(0, 0, true);
    require(camera.targetY == 400L * 4096 && camera.y > 176L * 4096 && camera.x == 240L * 4096,
            "death retains targets but continues easing without synthetic scroll");
    camera.reset(240, 100);
    camera.step(240 * 256L, -100 * 256L);
    require(camera.y == 142L * 4096, "camera centre is clamped to source top limit");
    camera.reset(240, 176);
    double expected = 176;
    for (int i = 0; i < 1000; ++i) {
        const int playerY = 176 + i * 3;
        camera.step(240 * 256L, playerY * 256L);
        expected += (playerY + 24 - expected) / 5;
        require(std::fabs(camera.y / 4096.0 - expected) < .002,
                "Q12 camera easing stays within .002 pixels of source equations");
    }
    camera.reset(240, 299900);
    camera.step(240 * 256L, 300000 * 256L);
    require(camera.targetY == 300024L * 4096 && camera.y > 299900L * 4096 &&
            camera.y < 2147483647L, "camera covers the serialized room height in signed 32-bit state");
    std::cout << "shared main camera tests passed\n";
}
