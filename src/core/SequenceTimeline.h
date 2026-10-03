#ifndef DOWNWELL_CORE_SEQUENCE_TIMELINE_H
#define DOWNWELL_CORE_SEQUENCE_TIMELINE_H
#include "RunnerImageValue.h"
namespace downwell {
// Integer-only timeline arithmetic for looping, unit-rate sprite sequences.
// Track evaluation and GML image_index synchronization are separate operations.
struct SequenceTimeline {
    static bool valid(RunnerImageValue value) {
        const auto e = (value.bits >> 23) & 255u;
        return e != 255 && (e || !(value.bits & 0x7fffffu));
    }
    // Binary32 bits of a positive integer below 2^24 (exact).
    static std::uint32_t integerBits(unsigned value) {
        int field = 127 + 23;
        std::uint32_t mantissa = value;
        if (!(mantissa & 0xffff00u)) { mantissa <<= 16; field -= 16; }
        if (!(mantissa & 0xff0000u)) { mantissa <<= 8; field -= 8; }
        if (!(mantissa & 0xf00000u)) { mantissa <<= 4; field -= 4; }
        if (!(mantissa & 0xc00000u)) { mantissa <<= 2; field -= 2; }
        if (!(mantissa & 0x800000u)) { mantissa <<= 1; --field; }
        return (std::uint32_t(field) << 23) | (mantissa & 0x7fffffu);
    }
    // The runner's float(double(a)+double(b)) for valid operands. When the
    // exponents differ by at most 7, the exact sum fits in 32 bits, so the
    // binary64 sum is exact and one nearest-even rounding gives the result.
    // Other operands (and subnormal or overflowing results) take binary64.
    static bool sumBinary32(std::uint32_t a, std::uint32_t b, std::uint32_t& out) {
        if (!(a & 0x7fffffffu) || !(b & 0x7fffffffu)) {
            if (a & 0x7fffffffu) out = a;
            else if (b & 0x7fffffffu) out = b;
            else out = a & b; // -0 only when both are -0.
            return true;
        }
        if ((a & 0x7fffffffu) < (b & 0x7fffffffu)) { const auto t = a; a = b; b = t; }
        const int ea = int((a >> 23) & 255u), eb = int((b >> 23) & 255u);
        if (!eb || ea - eb > 7) return false;
        const std::uint32_t ma = ((a & 0x7fffffu) | 0x800000u) << (ea - eb);
        const std::uint32_t mb = (b & 0x7fffffu) | 0x800000u;
        std::uint32_t r = (a ^ b) & 0x80000000u ? ma - mb : ma + mb; // |a| >= |b|
        if (!r) { out = 0; return true; }
        int field = eb;
        if (r >= 0x1000000u) {
            unsigned shift = 1;
            while ((r >> shift) >= 0x1000000u) ++shift;
            const std::uint32_t rest = r & ((1u << shift) - 1), half = 1u << (shift - 1);
            r >>= shift; field += int(shift);
            if (rest > half || (rest == half && (r & 1u))) {
                if (++r == 0x1000000u) { r >>= 1; ++field; }
            }
        } else while (!(r & 0x800000u)) { r <<= 1; --field; }
        if (field < 1 || field > 254) return false;
        out = (a & 0x80000000u) | (std::uint32_t(field) << 23) | (r & 0x7fffffu);
        return true;
    }
    static bool add(RunnerImageValue a, RunnerImageValue b, RunnerImageValue& out) {
        if (sumBinary32(a.bits, b.bits, out.bits)) return true;
        auto sum = a.promoted();
        return sum.add(b.promoted()) && out.assign(sum);
    }
    static std::uint32_t reduce(std::uint32_t value, std::uint32_t divisor) {
        if (value < divisor) return value;
        auto shifted = divisor;
        while (shifted <= (value >> 1)) shifted <<= 1;
        do {
            if (value >= shifted) value -= shifted;
            shifted >>= 1;
        } while (shifted >= divisor);
        return value;
    }
    // Exact signed fmod for a normal/zero float and a positive integer length.
    // The significand has only 24 bits; reduction needs no 64-bit division.
    static bool remainder(RunnerImageValue input, unsigned length, RunnerImageValue& output) {
        if (!length || length > 65535 || !valid(input)) return false;
        const auto sign = input.bits & 0x80000000u;
        const int e = int((input.bits >> 23) & 255u);
        if (!e || e < 127) { output = input; return true; }
        auto mantissa = (input.bits & 0x7fffffu) | 0x800000u;
        int scale = e - 150;
        if (scale >= 0) {
            mantissa = reduce(mantissa, length);
            for (int i = 0; i < scale; ++i) {
                mantissa <<= 1;
                if (mantissa >= length) mantissa -= length;
            }
            scale = 0;
        } else {
            const unsigned shift = unsigned(-scale); // e >= 127 => shift <= 23
            const auto fraction = mantissa & ((1u << shift) - 1);
            mantissa = (reduce(mantissa >> shift, length) << shift) | fraction;
        }
        if (!mantissa) { output.bits = sign; return true; }
        while (!(mantissa & 0x800000u)) { mantissa <<= 1; --scale; }
        output.bits = sign | (std::uint32_t(scale + 150) << 23) | (mantissa & 0x7fffffu);
        return true;
    }
    // Source helper 0x4bbd30: float(fmod(float(fmod(x,L))+L,L)).
    static bool wrap(RunnerImageValue input, unsigned length, RunnerImageValue& output) {
        RunnerImageValue first, added;
        if (!remainder(input,length,first)) return false;
        if (!add(first, {integerBits(length)}, added)) return false;
        return remainder(added,length,output);
    }
    static bool advance(RunnerImageValue& position, RunnerImageValue speed,
                        unsigned length, bool& crossed) {
        if (!length || length > 65535 || !valid(position) || !valid(speed)) return false;
        RunnerImageValue next, bound{integerBits(length)};
        if (!add(position, speed, next)) return false;
        const bool negative = (next.bits & 0x80000000u) && (next.bits & 0x7fffffffu);
        const bool boundary = negative || (!(next.bits & 0x80000000u) && next.bits >= bound.bits);
        if (boundary && !wrap(next,length,next)) return false;
        position = next;
        crossed = boundary;
        return true;
    }
};
}
#endif
