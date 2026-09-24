#include "core/FrogBehavior.h"
#include "core/FractionalMotion.h"
#include <cstdlib>
#include <iostream>

void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}

int main() {
    long position = 0;
    unsigned char remainder = 0;
    downwell::integrateFifthsQ8(position, remainder, -1, 1);
    require(position == -1 && remainder == 1,
            "negative displacement preserves a nonnegative residual");
    downwell::integrateFifthsQ8(position, remainder, 0, 4);
    require(position == 0 && remainder == 0,
            "opposite fractional displacements cancel exactly");
    using downwell::frogPreparationDirection;
    auto direction = [](long x, long y, long playerX, bool grounded = true) {
        return frogPreparationDirection(grounded, x, y, playerX,
            -80 * 256L, -142 * 256L, 80 * 256L, 142 * 256L);
    };
    require(direction(0, 0, 0) == -1, "source GT equality prepares left");
    require(direction(0, 0, 1) == 1, "subpixel player X is retained for aim");
    require(direction(0, 0, -1) == -1, "player left prepares left");
    require(direction(0, 0, 1, false) == 0, "airborne frog retries instead of preparing");
    require(direction(-80 * 256L, 0, 0) == 0 && direction(80 * 256L, 0, 0) == 0 &&
            direction(0, -142 * 256L, 0) == 0 && direction(0, 142 * 256L, 0) == 0,
            "all four view boundaries are strict");
    require(direction(-80 * 256L + 1, 0, 0) == 1 &&
            direction(0, 142 * 256L - 1, 0) == -1,
            "inside boundary is eligible, with no added margin");
    require(frogPreparationDirection(true, 0, 0, 1,
                -80 * 256L, 1, 80 * 256L, 284 * 256L + 1) == 0,
            "fractional camera top must not round the frog into view");
    require(frogPreparationDirection(true, 240 * 256L, 176 * 256L, 240 * 256L + 1,
                160 * 256L, 34 * 256L, 320 * 256L, 318 * 256L) == direction(0, 0, 1),
            "SDL world and native relative coordinate adapters agree");
    std::cout << "frog behavior tests passed\n";
}
