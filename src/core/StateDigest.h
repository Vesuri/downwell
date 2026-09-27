#ifndef DOWNWELL_CORE_STATE_DIGEST_H
#define DOWNWELL_CORE_STATE_DIGEST_H

#include <cstdint>
#include <cstring>

namespace downwell {

// Stable FNV-1a builder for cross-build state comparisons. Multi-byte values
// are fed least-significant byte first, independent of host endianness.
class StateDigest {
public:
    void addBool(bool value) { addByte(value ? 1 : 0); }

    void addU32(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8)
            addByte(static_cast<std::uint8_t>(value >> shift));
    }

    void addI32(std::int32_t value) {
        addU32(static_cast<std::uint32_t>(value));
    }

    void addU64(std::uint64_t value) {
        // Constant split: a variable 64-bit shift requires __lshrdi3 when
        // target optimization elects not to unroll the old eight-byte loop.
        addU32(static_cast<std::uint32_t>(value));
        addU32(static_cast<std::uint32_t>(value >> 32));
    }

    void addF32(float value) {
        std::uint32_t bits;
        static_assert(sizeof(bits) == sizeof(value), "32-bit float required");
        std::memcpy(&bits, &value, sizeof(bits));
        addU32(bits);
    }

    std::uint64_t value() const { return value_; }

private:
    void addByte(std::uint8_t value) {
        value_ ^= value;
        /* FNV prime = 2^40 + 435, expanded for freestanding 32-bit targets. */
        /* Volatile keeps GCC from folding this back into __muldi3 on 68000. */
        const volatile std::uint64_t prior = value_;
        value_ = (prior << 40) + (prior << 8) + (prior << 7) +
                 (prior << 5) + (prior << 4) + (prior << 1) + prior;
    }

    std::uint64_t value_ = 14695981039346656037ULL;
};

} // namespace downwell

#endif
