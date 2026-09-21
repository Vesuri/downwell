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
        for (int shift = 0; shift < 64; shift += 8)
            addByte(static_cast<std::uint8_t>(value >> shift));
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
        value_ *= 1099511628211ULL;
    }

    std::uint64_t value_ = 14695981039346656037ULL;
};

} // namespace downwell

#endif
