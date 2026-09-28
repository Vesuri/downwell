#include "core/RunnerImageValue.h"
#include "core/RunnerIntegerValueVerification.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok) { if (!ok) std::abort(); }
int main() {
    check(downwell::RunnerIntegerValueVerification::run());
    std::uint32_t rng = 1;
    unsigned cases = 0;
    for (unsigned i = 0; i < 100000; ++i) {
        rng = rng * 1664525u + 1013904223u;
        check(downwell::RunnerIntegerValueVerification::checkPixel(rng));
        const std::int32_t integer=std::int32_t(rng&0x7fffffffu)*(rng&0x80000000u?-1:1);
        check(downwell::RunnerIntegerValueVerification::check(integer));
        downwell::RunnerImageValue direct;
        check(direct.assignInteger(integer));
        const float floatInteger=static_cast<float>(integer);
        std::uint32_t integerBits;std::memcpy(&integerBits,&floatInteger,4);
        check(direct.bits==integerBits);
        const auto high = (rng & 0x800fffffu) | ((900u + i % 250) << 20);
        rng = rng * 1664525u + 1013904223u;
        // Random discarded bits plus exact ties and neighbors, both signs.
        const auto low = i % 4 ? ((rng & 0xe0000000u) | (0x0fffffffu + i % 4)) : rng;
        const std::uint64_t raw = (std::uint64_t(high) << 32) | low;
        double source; std::memcpy(&source, &raw, 8);
        const float expected = static_cast<float>(source);
        std::uint32_t expectedBits; std::memcpy(&expectedBits, &expected, 4);
        downwell::RunnerImageValue value;
        check(value.assign({high, low}) && value.bits == expectedBits);
        const double promoted = expected;
        std::uint64_t promotedBits; std::memcpy(&promotedBits, &promoted, 8);
        const auto actual = value.promoted();
        check(actual.high == std::uint32_t(promotedBits >> 32) && actual.low == std::uint32_t(promotedBits));
        ++cases;
    }
    downwell::RunnerImageValue value;
    check(value.assign({0x80000000u,0}) && value.bits == 0x80000000u);
    check(value.promoted().high == 0x80000000u);
    check(!value.assign({0x7ff00000u,0}) && value.bits == 0x80000000u);
    check(!value.assign({0,1}) && value.bits == 0x80000000u);
    check(!value.assign({0x00100000u,0}) && value.bits == 0x80000000u);
    std::printf("Runner image fields: %u normal rounding/promotion cases and rejection checks passed\n", cases);
}
