#include "GameMakerRandom.h"
#include "SmallUnsignedRemainder.h"
#include "Positive63Remainder.h"
#include "Well512.h"
#include <cassert>
#include <limits>

namespace downwell {

#ifdef DOWNWELL_AUDIT_RANDOM
std::uint64_t GameMakerRandom::auditedDraws = 0;
#endif

GameMakerRandom::GameMakerRandom(const GameMakerRandom& other) {
    *this = other;
}

GameMakerRandom& GameMakerRandom::operator=(const GameMakerRandom& other) {
    if (this == &other) return *this;
    for (unsigned i = 0; i < 16; ++i) state_[i] = other.stateWord(i);
    stateIndex_ = other.stateIndex_;
    seed_ = other.seed_;
    drawCount_ = other.drawCount_;
    // Assignment preserves the destination's own storage, never the source's.
    resetLookahead();
    return *this;
}

void GameMakerRandom::resetLookahead() {
    if (!lookahead_) return;
    lookahead_->producer = state_;
    lookahead_->producerIndex = stateIndex_;
    lookahead_->read = lookahead_->available = lookahead_->history = 0;
#ifdef DOWNWELL_AUDIT_RANDOM
    lookahead_->oracle = state_;
    lookahead_->oracleIndex = stateIndex_;
#endif
}

void GameMakerRandom::bindLookahead(Lookahead& storage) {
    if (lookahead_ == &storage) return;
    for (unsigned i = 0; i < 16; ++i) state_[i] = stateWord(i);
    lookahead_ = &storage;
    resetLookahead();
}

void GameMakerRandom::refill(unsigned steps) {
    if (!lookahead_) return;
    auto& ring = *lookahead_;
    const unsigned room = Lookahead::aheadCapacity - ring.available;
    if (steps > room) steps = room;
    while (steps--) {
        auto& entry = ring.entries[(ring.read + ring.available) & (Lookahead::capacity - 1)];
        const auto oldIndex = ring.producerIndex;
        entry.output = Well512::step(&ring.producer[0], ring.producerIndex);
        entry.mixed = ring.producer[oldIndex];
        ++ring.available;
    }
}

std::uint32_t GameMakerRandom::stateWord(std::size_t index) const {
    if (!lookahead_ || !lookahead_->history) return state_[index];
    const auto& ring = *lookahead_;
    const unsigned age = (index - stateIndex_) & 15u;
    if (!age) return ring.entries[(ring.read - 1) & (Lookahead::capacity - 1)].output;
    if (age <= ring.history)
        return ring.entries[(ring.read - age) & (Lookahead::capacity - 1)].mixed;
    return state_[index]; // Seed/snapshot words not yet overwritten by a draw.
}

void GameMakerRandom::consumeLookahead(unsigned draws) {
    auto& ring = *lookahead_;
#ifdef DOWNWELL_AUDIT_RANDOM
    // Host replay oracle: check even unread advance() draws individually.
    // No shadow state or verification work exists in production builds.
    while (draws--) {
        const auto output = Well512::reference(&ring.oracle[0], ring.oracleIndex);
        assert(output == ring.entries[ring.read].output);
        ring.read = (ring.read + 1) & (Lookahead::capacity - 1);
        --ring.available;
        if (ring.history < 16) ++ring.history;
        stateIndex_ = (stateIndex_ - 1) & 15u;
        assert(stateIndex_ == ring.oracleIndex);
        for (unsigned i = 0; i < 16; ++i) assert(stateWord(i) == ring.oracle[i]);
        ++auditedDraws;
    }
#else
    ring.read = (ring.read + draws) & (Lookahead::capacity - 1);
    ring.available -= draws;
    ring.history += draws;
    if (ring.history > 16) ring.history = 16;
    stateIndex_ = (stateIndex_ - draws) & 15u;
#endif
}

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
    resetLookahead();
}

std::uint32_t GameMakerRandom::nextU32() {
    ++drawCount_;
    if (lookahead_) {
        if (!lookahead_->available) refill(1);
        const auto value = lookahead_->entries[lookahead_->read].output;
        consumeLookahead(1);
        return value;
    }
    return Well512::step(&state_[0],stateIndex_);
}

void GameMakerRandom::advance(unsigned draws) {
    drawCount_ += draws;
    if (lookahead_) {
        while (draws) {
            if (!lookahead_->available) refill(draws);
            const unsigned count = draws < lookahead_->available ? draws : lookahead_->available;
            consumeLookahead(count);
            draws -= count;
        }
        return;
    }
    while (draws--) Well512::step(&state_[0],stateIndex_);
}

double GameMakerRandom::random(double upperExclusive) {
    constexpr double u32Scale = 1.0 / 4294967296.0;
    return static_cast<double>(nextU32()) * u32Scale * upperExclusive;
}

std::uint64_t GameMakerRandom::irandom(std::uint64_t upperInclusive) {
    assert(upperInclusive <=
           static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
    const std::uint32_t low = nextU32();
    const std::uint32_t high = nextU32() & 0x7fffffffU;
    const auto divisor=upperInclusive+1U;
    const auto remainder=Positive63Remainder::full({high,low},
        {static_cast<std::uint32_t>(divisor>>32),static_cast<std::uint32_t>(divisor)});
    return (std::uint64_t(remainder.high)<<32)|remainder.low;
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
    std::uint32_t remainder = SmallUnsignedRemainder::full(high, divisor);
    remainder = SmallUnsignedRemainder::append(remainder, static_cast<std::uint16_t>(low >> 16U), divisor);
    return SmallUnsignedRemainder::append(remainder, static_cast<std::uint16_t>(low), divisor);
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
