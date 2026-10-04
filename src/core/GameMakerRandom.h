#ifndef DOWNWELL_CORE_GAME_MAKER_RANDOM_H
#define DOWNWELL_CORE_GAME_MAKER_RANDOM_H

#include <array>
#include <cstddef>
#include <cstdint>
#include "SmallUnsignedRemainder.h"

namespace downwell {

// Windows runner-compatible random stream recovered from Downwell.exe.
// GameMaker's random functions are target-specific, so this deliberately
// models the 32-bit Windows runner rather than a generic host RNG.
class GameMakerRandom {
public:
    // Runtime-owned Fast/host storage; temporary RNG snapshots stay small.
    // Sixteen consumed entries remain behind the cursor for stateWord().
    struct Lookahead {
        static constexpr unsigned capacity = 512, aheadCapacity = capacity - 16;
        struct Entry { std::uint32_t output, mixed; } entries[capacity]{};
        std::array<std::uint32_t, 16> producer{};
        std::size_t producerIndex = 0;
        unsigned read = 0, available = 0, history = 0;
#ifdef DOWNWELL_AUDIT_RANDOM
        std::array<std::uint32_t, 16> oracle{};
        std::size_t oracleIndex = 0;
#endif
    };
    explicit GameMakerRandom(std::uint32_t seed = 0) { setSeed(seed); }
    GameMakerRandom(const GameMakerRandom& other);
    GameMakerRandom& operator=(const GameMakerRandom& other);

    // Storage must outlive this RNG and be exclusively owned by it. Binding
    // and filling do not consume a draw. Copies never borrow another cache.
    void bindLookahead(Lookahead& storage);
    void refill(unsigned steps = 4);
    void prepareLookahead(Lookahead& storage) {
#ifndef DOWNWELL_REFERENCE_RANDOM
        bindLookahead(storage);
        refill(Lookahead::aheadCapacity);
#else
        (void)storage; // Retained per-step oracle, including native profiles.
#endif
    }
    struct FrameRefill {
        GameMakerRandom& random;
        ~FrameRefill() { random.refill(); }
    };

    void setSeed(std::uint32_t seed);
    std::uint32_t seed() const { return seed_; }
    std::uint32_t nextU32();
    // Discard draws whose values no caller reads; same stream and count.
    void advance(unsigned draws);
    double random(double upperExclusive = 1.0);
    std::uint64_t irandom(std::uint64_t upperInclusive);
    // Windows choose builtin: one unsigned WELL word modulo argument count,
    // including count == 1. Not interchangeable with irandom(count - 1).
    // Nonempty small lists only; integer-to-float rounding in the runner is
    // exact throughout this range.
    // Keep literal list sizes visible to callers: powers of two reduce to a
    // mask instead of paying a generic helper call in hot gameplay paths.
#if defined(__GNUC__)
    __attribute__((always_inline))
#endif
    std::uint32_t chooseIndexSmall(std::uint32_t count) {
        assert(count > 0 && count <= 0xffffU);
        const auto value = nextU32();
        if (!(count & (count - 1U))) return value & (count - 1U);
        return SmallUnsignedRemainder::full(value, count);
    }
    // Exact irandom path for the small inclusive ranges used by Downwell's
    // generators. This preserves the runner's 63-bit modulus without asking
    // a 68000 target for 64-bit division.
    std::uint32_t irandomSmall(std::uint32_t upperInclusive);
    // Exact floor(random(upperExclusive)) for the small positive ranges used
    // by recovered Create events.  The multiply is split into 16-bit halves,
    // avoiding both floating point and a 64-bit multiply on a 68000.
    std::uint32_t randomIntegerSmall(std::uint32_t upperExclusive);

    std::size_t stateIndex() const { return stateIndex_; }
    std::uint32_t stateWord(std::size_t index) const;
    std::uint64_t drawCount() const { return drawCount_; }
#ifdef DOWNWELL_AUDIT_RANDOM
    static std::uint64_t auditedDraws;
#endif

private:
    static constexpr std::uint32_t wellMask = 0xda442d24U;

    std::array<std::uint32_t, 16> state_{};
    std::size_t stateIndex_ = 0;
    std::uint32_t seed_ = 0;
    std::uint64_t drawCount_ = 0;
    Lookahead* lookahead_ = nullptr;
    void resetLookahead();
    void consumeLookahead(unsigned draws);
};

} // namespace downwell

#endif
