#include "GameMakerRandom.h"
#include <cassert>
#include <limits>

namespace downwell {

void GameMakerRandom::setSeed(std::uint32_t seed) {
    seed_ = seed;
    std::uint32_t value = seed;
    for (std::uint32_t& word : state_) {
        // The 32-bit Windows runner uses the Microsoft C runtime LCG to
        // populate its 16-word WELL512 state, retaining the upper 16 bits.
        value = value * 0x343fdU + 0x269ec3U;
        word = value >> 16U;
    }
    stateIndex_ = 0;
    drawCount_ = 0;
}

std::uint32_t GameMakerRandom::nextU32() {
    ++drawCount_;
    const std::uint32_t a = state_[stateIndex_];
    std::uint32_t c = state_[(stateIndex_ + 13U) & 15U];
    const std::uint32_t b = a ^ c ^ (a << 16U) ^ (c << 15U);
    c = state_[(stateIndex_ + 9U) & 15U];
    c ^= c >> 11U;
    const std::uint32_t mixed = b ^ c;
    state_[stateIndex_] = mixed;
    const std::uint32_t d = mixed ^ ((mixed << 5U) & wellMask);
    stateIndex_ = (stateIndex_ + 15U) & 15U;
    const std::uint32_t prior = state_[stateIndex_];
    state_[stateIndex_] = prior ^ b ^ d ^ (prior << 2U) ^
                          (b << 18U) ^ (c << 28U);
    return state_[stateIndex_];
}

double GameMakerRandom::random(double upperExclusive) {
    constexpr double u32Scale = 1.0 / 4294967296.0;
    return static_cast<double>(nextU32()) * u32Scale * upperExclusive;
}

std::uint64_t GameMakerRandom::irandom(std::uint64_t upperInclusive) {
    assert(upperInclusive <=
           static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
    const std::uint64_t low = nextU32();
    const std::uint64_t high = nextU32() & 0x7fffffffU;
    const std::uint64_t positive63 = (high << 32U) | low;
    return positive63 % (upperInclusive + 1U);
}

std::uint32_t GameMakerRandom::irandomSmall(
    std::uint32_t upperInclusive) {
    assert(upperInclusive <= 0xffffU);
    const std::uint32_t divisor = upperInclusive + 1U;
    const std::uint32_t low = nextU32();
    const std::uint32_t high = nextU32() & 0x7fffffffU;
    // A power-of-two divisor uses only low bits, but both original stream
    // words must still be consumed. Common builder choices use bounds 1/3/7.
    if (!(divisor & (divisor - 1U))) return low & (divisor - 1U);
    /*
     * (high << 32 | low) % divisor, reduced a word at a time.  With a
     * divisor of at most 65536, appending each low 16-bit word to a
     * remainder fits uint32_t. No multiplication or 2^32 remainder needed.
     */
    std::uint32_t remainder = high % divisor;
    remainder = ((remainder << 16U) | (low >> 16U)) % divisor;
    return ((remainder << 16U) | (low & 0xffffU)) % divisor;
}

std::uint32_t GameMakerRandom::randomIntegerSmall(
    std::uint32_t upperExclusive) {
    assert(upperExclusive <= 0xffffU);
    const std::uint32_t value = nextU32();
    const std::uint32_t highProduct =
        (value >> 16U) * upperExclusive;
    const std::uint32_t lowCarry =
        ((value & 0xffffU) * upperExclusive) >> 16U;
    return (highProduct + lowCarry) >> 16U;
}

} // namespace downwell
