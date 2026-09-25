#include "core/MotionBinary64.h"
#include "core/GameMakerRounding.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using downwell::MotionBinary64;
static MotionBinary64 bits(double value) {
    std::uint64_t raw; std::memcpy(&raw, &value, 8);
    return {std::uint32_t(raw >> 32), std::uint32_t(raw)};
}
static double value(MotionBinary64 bits) {
    std::uint64_t raw = (std::uint64_t(bits.high) << 32) | bits.low;
    double out; std::memcpy(&out, &raw, 8); return out;
}
static void check(bool ok) { if (!ok) std::abort(); }
int main() {
    unsigned state = 1234;
    auto next = [&] { state = state * 1664525u + 1013904223u; return state; };
    for (unsigned i = 0; i < 200000; ++i) {
        MotionBinary64 a{(next() & 0x800fffffu) | ((970 + next()%90) << 20), next()};
        MotionBinary64 b{(next() & 0x800fffffu) | ((970 + next()%90) << 20), next()};
        const auto expected = bits(value(a) + value(b));
        check(a.add(b) && a.high == expected.high && a.low == expected.low);
    }
    for (int start : {0, 32, 128, 704, 990, 2048, 4096, 8192})
        for (double initial : {-3.0, -3.1, -3.2}) {
            double vy = initial, y = start;
            auto targetV = bits(vy), targetY = bits(y);
            for (unsigned frame = 0; frame < 1000; ++frame) {
                vy += 0.1; y += vy;
                check(targetV.add(bits(0.1)) && targetY.add(targetV));
                check(targetV.high == bits(vy).high && targetV.low == bits(vy).low);
                check(targetY.high == bits(y).high && targetY.low == bits(y).low);
                std::int32_t pixel;
                check(targetY.roundPixel(pixel) && pixel == downwell::roundGameMakerCoordinate(y));
            }
        }
    for (double x : {-2.5, -1.5, -0.5, 0.0, 0.5, 1.5, 2.5}) {
        auto a = bits(x); std::int32_t pixel;
        check(a.roundPixel(pixel) && pixel == downwell::roundGameMakerCoordinate(x));
        check(a.add(bits(-x)) && a.zero());
    }
    auto invalid = MotionBinary64{0x7ff00000u, 0};
    check(!invalid.add(bits(1)) && invalid.high == 0x7ff00000u);
    for (auto rejected : {MotionBinary64{0,1}, MotionBinary64{0x7ff00000u,1},
                          MotionBinary64{0xfff00000u,0}}) {
        auto a = bits(1);
        check(!a.add(rejected) && a.high == bits(1).high && a.low == 0);
    }
    auto smallest = MotionBinary64{0x00100000u,0};
    check(!smallest.add({0x80100000u,1}) && smallest.high == 0x00100000u && smallest.low == 0);
    auto largest = MotionBinary64{0x7fefffffu,0xffffffffu};
    check(!largest.add(largest) && largest.high == 0x7fefffffu && largest.low == 0xffffffffu);
    for (auto a : {bits(0.0), bits(-0.0), bits(1.0), bits(-1.0),
                   MotionBinary64{0x3fffffffu,0xffffffffu}, MotionBinary64{0x00100000u,0}})
        for (auto b : {bits(0.0), bits(-0.0), bits(1.0), bits(-1.0),
                       MotionBinary64{0x3ca00000u,0}, MotionBinary64{0x3ca00000u,1}}) {
            const auto expected = bits(value(a) + value(b));
            auto actual = a;
            check(actual.add(b) && actual.high == expected.high && actual.low == expected.low);
        }
    std::puts("Motion binary64: 200000 additions, 24000 trajectory updates and rounding cases passed");
}
