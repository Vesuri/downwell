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
        auto sum = first.promoted();
        if (!sum.add(MotionBinary64::fromInteger(length)) || !added.assign(sum)) return false;
        return remainder(added,length,output);
    }
    static bool advance(RunnerImageValue& position, RunnerImageValue speed,
                        unsigned length, bool& crossed) {
        if (!length || length > 65535 || !valid(position) || !valid(speed)) return false;
        auto sum = position.promoted();
        RunnerImageValue next, bound;
        if (!sum.add(speed.promoted()) || !next.assign(sum) ||
            !bound.assign(MotionBinary64::fromInteger(length))) return false;
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
