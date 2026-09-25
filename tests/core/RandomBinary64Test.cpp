#include "core/RandomBinary64.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
static void check(bool ok) { if (!ok) std::abort(); }
static downwell::MotionBinary64 bits(double d) {
    std::uint64_t raw; std::memcpy(&raw,&d,8);
    return {std::uint32_t(raw>>32),std::uint32_t(raw)};
}
int main() {
    std::uint32_t state = 7;
    unsigned cases = 0;
    for (double scale : {0.0,0.45,1.5,3.0,0.00001,65536.0})
        for (unsigned i = 0; i < 100000; ++i) {
            state = state*1664525u+1013904223u;
            const std::uint32_t word = i < 33 ? (i == 32 ? 0xffffffffu : (1u<<i)-1u) : state;
            const auto expected = bits((word/4294967296.0)*scale);
            downwell::MotionBinary64 actual;
            check(downwell::randomBinary64(word,bits(scale),actual));
            check(actual.high == expected.high && actual.low == expected.low);
            ++cases;
        }
    downwell::MotionBinary64 out{1,2};
    check(!downwell::randomBinary64(1,{0x00100000u,0},out) && out.high == 1 && out.low == 2);
    check(!downwell::randomBinary64(1,bits(-1),out) && out.high == 1 && out.low == 2);
    std::printf("Random binary64: %u exact scaled-word cases passed\n",cases);
}
