#include "core/AmmoDividers.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

struct Draw { unsigned frame, y; };
static void check(unsigned ammo, unsigned cap, unsigned cost, unsigned bottom) {
    std::vector<Draw> expected, actual;
    const double filled = 184.0 * (static_cast<double>(ammo) / cap);
    if (cap < 20) {
        const double spacing = 184.0 * (1.0 / cap);
        for (double i = 184; i > 0; i -= spacing)
            if (i < filled) expected.push_back({1, static_cast<unsigned>(std::floor(bottom - i))});
    }
    const double spacing = 184.0 * (static_cast<double>(cost) / cap);
    for (double i = 184; i > 0; i -= spacing) {
        const double y = bottom - i;
        if (i < filled) {
            expected.push_back({0, static_cast<unsigned>(std::floor(y))});
            expected.push_back({1, static_cast<unsigned>(std::floor(y + 1))});
            expected.push_back({1, static_cast<unsigned>(std::floor(y - 1))});
        } else if (i == filled || i == 1)
            expected.push_back({1, static_cast<unsigned>(std::floor(y + 1))});
    }
    downwell::drawAmmoDividers(ammo, cap, cost, 288, bottom,
        [&](unsigned sprite, unsigned frame, int x, unsigned y) {
            if (sprite != 421 || x != 288) std::abort();
            actual.push_back({frame, y});
        });
    bool equal = expected.size() == actual.size();
    for (unsigned i = 0; equal && i < expected.size(); ++i)
        equal = expected[i].frame == actual[i].frame && expected[i].y == actual[i].y;
    if (!equal) {
        std::fprintf(stderr, "Divider mismatch ammo=%u cap=%u cost=%u bottom=%u (%zu/%zu draws)\n",
                     ammo, cap, cost, bottom, expected.size(), actual.size());
        std::exit(1);
    }
}
int main() {
    for (unsigned cap = 1; cap <= 64; ++cap)
        for (unsigned ammo = 0; ammo <= cap; ++ammo)
            for (unsigned cost : {1u, 2u, 3u, 7u})
                for (unsigned bottom : {229u, 233u, 237u, 241u}) check(ammo, cap, cost, bottom);
    for (unsigned cap : {127u, 184u, 255u, 368u, 1000u})
        for (unsigned ammo : {0u, 1u, cap / 2, cap}) check(ammo, cap, 1, 229);
    for (unsigned cap : {0x7fffffffu, 0xffffffffu})
        for (unsigned ammo : {1u, cap / 2, cap}) check(ammo, cap, cap / 2, 241);
    std::puts("Ammo divider draws match original binary64 loops");
}
