#ifndef DOWNWELL_CORE_MOTION_BINARY64_H
#define DOWNWELL_CORE_MOTION_BINARY64_H
#include "BitCount.h"
#include <cstdint>

namespace downwell {
#ifdef DOWNWELL_ADD_BINARY64_HOOK
// A build may supply an equal sum (the install converter's twin,
// tools/install-data/binary64_amiga.s); addPortable is its oracle and must
// stay bit-identical to it, failures included. The game does not use a hook.
struct MotionBinary64;
extern "C" bool downwellAddBinary64Hook(MotionBinary64* self, MotionBinary64 other);
#endif
// Finite normal binary64 addition and signed pixel rounding, using only
// 32-bit integer operations. Not a general floating-point implementation:
// subnormal operands/results, infinity, NaN and overflow return false.
// Intended to preserve observable motion rounding without target FP helpers.
struct MotionBinary64 {
    std::uint32_t high = 0, low = 0;
    struct Words {
        std::uint32_t high, low;
        bool nonzero() const { return high || low; }
        static unsigned leadingZeros32(std::uint32_t value) { return downwell::leadingZeros32(value); }
        Words left(unsigned count) const {
            if (!count) return *this;
            if (count>=64) return {0,0};
            if (count>=32) return {low<<(count-32),0};
            return {(high<<count)|(low>>(32-count)),low<<count};
        }
        Words rightJam(unsigned count) const {
            if (!count) return *this;
            if (count >= 64) return {0, unsigned(nonzero())};
            if (count >= 32) {
                const unsigned n = count - 32;
                return {0, (high >> n) | unsigned(low != 0 || (n && (high << (32-n)) != 0))};
            }
            return {high >> count, (low >> count) | (high << (32-count)) |
                unsigned((low << (32-count)) != 0)};
        }
        void leftOne() { high = (high << 1) | (low >> 31); low <<= 1; }
        void add(Words other) {
            const auto before = low; low += other.low;
            high += other.high + unsigned(low < before);
        }
        void subtract(Words other) {
            const auto before = low; low -= other.low;
            high -= other.high + unsigned(before < other.low);
        }
    };
    bool valid() const {
        const unsigned e = (high >> 20) & 2047;
        return e != 2047 && (e || (!(high & 0xfffff) && !low));
    }
    bool zero() const { return !(high & 0x7fffffff) && !low; }
    static MotionBinary64 fromInteger(std::int32_t value) {
        if (!value) return {};
        Words m{0, value < 0 ? 0u - static_cast<std::uint32_t>(value) : static_cast<std::uint32_t>(value)};
        const unsigned top=31-Words::leadingZeros32(m.low);
        const int exponent=1023+int(top);
        m=m.left(52-top);
        return {(value < 0 ? 0x80000000u : 0u) | (unsigned(exponent) << 20) | (m.high & 0xfffff), m.low};
    }
    bool add(MotionBinary64 other) {
#ifdef DOWNWELL_ADD_BINARY64_HOOK
        return downwellAddBinary64Hook(this, other);
#else
        return addPortable(other);
#endif
    }
    bool addPortable(MotionBinary64 other) {
        if (!valid() || !other.valid()) return false;
        if (zero()) {
            if (!other.zero()) *this = other;
            else high &= other.high; // -0 + -0 = -0; mixed zeros = +0.
            return true;
        }
        if (other.zero()) return true;
        MotionBinary64 a = *this, b = other;
        if ((a.high & 0x7fffffff) < (b.high & 0x7fffffff) ||
            ((a.high & 0x7fffffff) == (b.high & 0x7fffffff) && a.low < b.low)) {
            a = other; b = *this;
        }
        int exponent = (a.high >> 20) & 2047;
        const unsigned sign = a.high & 0x80000000u;
        Words ma{((a.high & 0xfffff) | 0x100000u) << 3 | (a.low >> 29), a.low << 3};
        Words mb{((b.high & 0xfffff) | 0x100000u) << 3 | (b.low >> 29), b.low << 3};
        mb = mb.rightJam(unsigned(exponent - int((b.high >> 20) & 2047)));
        if ((a.high ^ b.high) & 0x80000000u) {
            ma.subtract(mb);
            if (!ma.nonzero()) { high = low = 0; return true; }
            // At most 55 leading positions after cancellation. Skip them in
            // bounded 32-bit operations instead of a per-bit 68000 loop.
            const unsigned shift=ma.high?Words::leadingZeros32(ma.high)-8:
                24+Words::leadingZeros32(ma.low);
            ma=ma.left(shift);exponent-=int(shift);
        } else {
            ma.add(mb);
            if (ma.high & 0x1000000u) { ma = ma.rightJam(1); ++exponent; }
        }
        const unsigned tail = ma.low & 7;
        const bool up = tail > 4 || (tail == 4 && (ma.low & 8));
        // All discarded bits have already been encoded in tail's sticky bit.
        ma = {ma.high >> 3, (ma.low >> 3) | (ma.high << 29)};
        if (up) ma.add({0, 1});
        if (ma.high & 0x200000u) { ma = ma.rightJam(1); ++exponent; }
        if (exponent <= 0 || exponent >= 2047) return false;
        high = sign | (unsigned(exponent) << 20) | (ma.high & 0xfffff);
        low = ma.low;
        return true;
    }
    bool roundPixel(std::int32_t& result) const {
        if (!valid()) return false;
        const int exponent = int((high >> 20) & 2047) - 1023;
        if (zero() || exponent < -1) { result = 0; return true; }
        if (exponent > 30) return false;
        Words m{(high & 0xfffff) | 0x100000u, low};
        m = m.rightJam(unsigned(49-exponent));
        const unsigned tail = m.low & 7;
        std::uint32_t value = (m.low >> 3) | (m.high << 29);
        if (tail > 4 || (tail == 4 && (value & 1))) ++value;
        if (value > 0x7fffffffu) return false;
        result = (high & 0x80000000u) ? -static_cast<std::int32_t>(value) : static_cast<std::int32_t>(value);
        return true;
    }
};
} // namespace downwell
#endif
