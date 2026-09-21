#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testWell512Vector() {
    downwell::GameMakerRandom random(0x12345678U);
    constexpr std::uint32_t expected[] = {
        0x90dbc8d8U, 0xe928654bU, 0xbaeb07efU, 0x05841a9dU,
        0x0a800b46U, 0xc07bd57bU, 0x49b80c24U, 0x00554459U
    };
    for (std::uint32_t value : expected)
        require(random.nextU32() == value,
                "WELL512 output matches the recovered Windows runner vector");
    require(random.seed() == 0x12345678U,
            "random_get_seed retains the seed rather than the stream position");
}

void testIntegerRangeAndReset() {
    downwell::GameMakerRandom random(0x12345678U);
    constexpr std::uint64_t maxima[] = {4, 1, 6, 100, 0xffffffffULL};
    constexpr std::uint64_t expected[] = {0, 1, 4, 91, 1467452704ULL};
    for (std::size_t index = 0; index < 5; ++index)
        require(random.irandom(maxima[index]) == expected[index],
                "irandom uses two outputs and an inclusive modulus");

    random.setSeed(0x12345678U);
    require(random.irandom(4) == expected[0],
            "random_set_seed reproduces the integer stream");
}

void testRealRange() {
    downwell::GameMakerRandom random(0x12345678U);
    const double expected = 0x90dbc8d8ULL / 4294967296.0;
    require(std::abs(random.random() - expected) < 1.0e-15,
            "random scales one unsigned output by exactly 2^-32");
}

} // namespace

int main() {
    testWell512Vector();
    testIntegerRangeAndReset();
    testRealRange();
    std::cout << "GameMaker random tests passed\n";
    return 0;
}
