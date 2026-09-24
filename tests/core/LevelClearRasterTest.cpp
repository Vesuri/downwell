#include "core/LevelClearRaster.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <initializer_list>
static void require(bool ok) {
    if (!ok) { std::fputs("binary32 floor mismatch\n", stderr); std::abort(); }
}
static void check(downwell::LevelClearCoordinate coordinate) {
    const auto n = coordinate.magnitude;
    double value = std::ldexp(double(n.mantissa), n.exponent - 52);
    if (coordinate.negative) value = -value;
    const float narrowed = float(value);
    const int expected = narrowed >= 2147483648.0F ? 2147483647 :
        narrowed <= -2147483648.0F ? (-2147483647 - 1) : int(std::floor(narrowed));
    require(downwell::LevelClearRaster::floorBinary32(coordinate) == expected);
}
static void checkDouble(double value) {
    std::uint64_t bits;
    const bool negative = std::signbit(value);
    value = std::fabs(value);
    std::memcpy(&bits, &value, sizeof bits);
    check({value ? downwell::HudReal{(bits & 0xfffffffffffffULL) | (1ULL << 52),
        int((bits >> 52) & 2047) - 1023} : downwell::HudReal{}, negative});
}
int main() {
    std::uint32_t random = 93;
    for (unsigned i = 0; i < 1000000; ++i) {
        random = random * 1664525u + 1013904223u;
        const std::uint64_t high = std::uint64_t(random) << 21;
        random = random * 1664525u + 1013904223u;
        const auto mantissa = ((high | (random & 0x1fffffu)) & 0xfffffffffffffULL) | (1ULL << 52);
        check({{mantissa, int(i % 193) - 160}, bool(random & 0x80000000u)});
    }
    for (double value : {0.0, 0.5, 1.0, 79.999996185302734375, 80.0, 142.0, 302.0,
                         32768.0, 2147483648.0, std::ldexp(1.0, -150), std::ldexp(1.0, -149)}) {
        for (int sign : {-1, 1}) {
            checkDouble(sign * value);
            if (value) {
                checkDouble(sign * std::nextafter(value, 0.0));
                checkDouble(sign * std::nextafter(value, std::numeric_limits<double>::infinity()));
            }
        }
    }
    std::puts("1,000,000 binary32-floor coordinates and boundary neighbors passed");
}
