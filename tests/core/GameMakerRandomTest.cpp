#include "core/GameMakerRandom.h"
#include "core/TitleParticleRandom.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

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
    require(random.drawCount() == 8,
            "the adapter exposes the exact shared-stream consumption count");
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
    require(random.drawCount() == 0 && random.irandom(4) == expected[0] &&
                random.drawCount() == 2,
            "random_set_seed reproduces the integer stream");
}

void testRealRange() {
    downwell::GameMakerRandom random(0x12345678U);
    const double expected = 0x90dbc8d8ULL / 4294967296.0;
    require(std::abs(random.random() - expected) < 1.0e-15,
            "random scales one unsigned output by exactly 2^-32");
}

void testChoose() {
    for (unsigned seed = 0; seed < 32; ++seed) {
        downwell::GameMakerRandom actual(seed), reference(seed);
        for (unsigned count = 1; count <= 65535; ++count) {
            const auto word = reference.nextU32();
            require(actual.chooseIndexSmall(count) == word % count,
                    "choose uses the unsigned single-word remainder, not irandom");
            require(actual.drawCount() == reference.drawCount() &&
                    actual.stateIndex() == reference.stateIndex(),
                    "each nonempty choose consumes one word, including singleton");
            for (unsigned i = 0; i < 16; ++i)
                require(actual.stateWord(i) == reference.stateWord(i),
                        "choose preserves complete runner RNG state");
        }
    }
    downwell::GameMakerRandom singleton(0x12345678U);
    require(singleton.chooseIndexSmall(1) == 0 && singleton.drawCount() == 1 &&
            singleton.nextU32() == 0xe928654bU,
            "singleton choose still advances to the second pinned WELL word");
}

void testTitleParticleOrder() {
    struct Trace {
        std::vector<double> calls;
        unsigned gate;
        double random(double upper) { calls.push_back(upper); return 0; }
        unsigned chooseIndexSmall(unsigned count) {
            require(count == 2, "title sprite/gate choices have two arguments");
            calls.push_back(-2); return gate;
        }
    };
    Trace burst{{}, 1};
    downwell::consumeTitleBurstRandom(burst);
    std::vector<double> expected;
    for (unsigned i = 0; i < 20; ++i)
        for (double value : {0.1, 0.165, -2.0, 32.0, 140.0}) expected.push_back(value);
    require(burst.calls == expected, "title burst evaluates VM arguments right-to-left");
    for (unsigned gate : {0u, 1u}) {
        Trace rise{{}, gate};
        downwell::consumeTitleRiseRandom(rise);
        require(rise.calls == (gate ? std::vector<double>{-2, 0.4, -2, 32, 132}
                                   : std::vector<double>{-2}),
                "title rise gate preserves conditional choice and range order");
    }
    for (unsigned seed = 0; seed < 128; ++seed) {
        downwell::GameMakerRandom actual(seed), reference(seed);
        downwell::consumeTitleBurstRandom(actual);
        for (unsigned i = 0; i < 100; ++i) reference.nextU32();
        for (unsigned frame = 0; frame < 100; ++frame) {
            downwell::consumeTitleRiseRandom(actual);
            if (reference.nextU32() % 2)
                for (unsigned i = 0; i < 4; ++i) reference.nextU32();
            require(actual.drawCount() == reference.drawCount() && actual.stateIndex() == reference.stateIndex(),
                    "title events consume one word per builtin");
            for (unsigned i = 0; i < 16; ++i)
                require(actual.stateWord(i) == reference.stateWord(i), "title events retain full RNG state");
        }
    }
}

void testSmallIntegerPath() {
    constexpr std::uint32_t maxima[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 9, 12, 15, 30, 40, 50, 60,
        70, 180, 359, 255, 1023, 65535
    };
    for (std::uint32_t seed = 0; seed < 32; ++seed) {
        downwell::GameMakerRandom reference(seed * 0x1020304U + 7U);
        downwell::GameMakerRandom small(seed * 0x1020304U + 7U);
        for (unsigned sample = 0; sample < 200; ++sample) {
            const std::uint32_t maximum =
                maxima[sample % (sizeof(maxima) / sizeof(maxima[0]))];
            require(small.irandomSmall(maximum) ==
                        reference.irandom(maximum),
                    "32-bit small-range modulus matches recovered irandom");
        }
        require(small.drawCount() == reference.drawCount(),
                "small-range adapter consumes the same two stream words");
    }
    downwell::GameMakerRandom reference(0x98765432U), small(0x98765432U);
    for (std::uint32_t maximum = 0; maximum <= 65535; ++maximum) {
        require(small.irandomSmall(maximum) == reference.irandom(maximum),
                "every supported integer bound matches original 63-bit remainder");
        require(small.nextU32() == reference.nextU32(),
                "every bound preserves the following RNG stream word");
    }
    require(small.drawCount() == reference.drawCount(), "exhaustive bounds preserve draw count");
}

void testSmallScaledRealPath() {
    constexpr std::uint32_t ranges[] = {
        0, 1, 2, 3, 5, 60, 300, 359, 1024, 32767, 65535
    };
    for (std::uint32_t seed = 0; seed < 32; ++seed) {
        downwell::GameMakerRandom reference(seed * 0x3141592U + 11U);
        downwell::GameMakerRandom small(seed * 0x3141592U + 11U);
        for (unsigned sample = 0; sample < 200; ++sample) {
            const std::uint32_t range =
                ranges[sample % (sizeof(ranges) / sizeof(ranges[0]))];
            const std::uint32_t expected = static_cast<std::uint32_t>(
                reference.random(static_cast<double>(range)));
            require(small.randomIntegerSmall(range) == expected,
                    "32-bit scaled integer matches floor(random(range))");
        }
        require(small.drawCount() == reference.drawCount(),
                "scaled integer adapter consumes one stream word");
    }
}

} // namespace

int main() {
    testWell512Vector();
    testIntegerRangeAndReset();
    testRealRange();
    testChoose();
    testTitleParticleOrder();
    testSmallIntegerPath();
    testSmallScaledRealPath();
    std::cout << "GameMaker random tests passed\n";
    return 0;
}
