#ifndef DOWNWELL_CORE_GAME_MAKER_RANDOM_H
#define DOWNWELL_CORE_GAME_MAKER_RANDOM_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

// Windows runner-compatible random stream recovered from Downwell.exe.
// GameMaker's random functions are target-specific, so this deliberately
// models the 32-bit Windows runner rather than a generic host RNG.
class GameMakerRandom {
public:
    explicit GameMakerRandom(std::uint32_t seed = 0) { setSeed(seed); }

    void setSeed(std::uint32_t seed);
    std::uint32_t seed() const { return seed_; }
    std::uint32_t nextU32();
    double random(double upperExclusive = 1.0);
    std::uint64_t irandom(std::uint64_t upperInclusive);
    // Exact irandom path for the small inclusive ranges used by Downwell's
    // generators. This preserves the runner's 63-bit modulus without asking
    // a 68000 target for 64-bit division.
    std::uint32_t irandomSmall(std::uint32_t upperInclusive);

    std::size_t stateIndex() const { return stateIndex_; }
    std::uint32_t stateWord(std::size_t index) const { return state_[index]; }
    std::uint64_t drawCount() const { return drawCount_; }

private:
    static constexpr std::uint32_t wellMask = 0xda442d24U;

    std::array<std::uint32_t, 16> state_{};
    std::size_t stateIndex_ = 0;
    std::uint32_t seed_ = 0;
    std::uint64_t drawCount_ = 0;
};

} // namespace downwell

#endif
