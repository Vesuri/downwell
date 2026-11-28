#include "core/SequenceTimeline.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static void require(bool value, int line) { if (!value) { std::fprintf(stderr,"Sequence check failed at line %d\n",line); std::abort(); } }
#define check(value) require((value), __LINE__)
static downwell::RunnerImageValue stored(float value) {
    downwell::RunnerImageValue out; std::memcpy(&out.bits,&value,4); return out;
}
static float wrapped(float value, unsigned length) {
    const float first = float(std::fmod(double(value),double(length)));
    const float added = first + float(length);
    return float(std::fmod(double(added),double(length)));
}
int main() {
    using downwell::SequenceTimeline;
    unsigned cases = 0;
    std::uint32_t rng = 7;
    for (unsigned i = 0; i < 100000; ++i) {
        rng = rng * 1664525u + 1013904223u;
        downwell::RunnerImageValue input; input.bits = (rng & 0x807fffffu) | ((1 + i % 254) << 23);
        float value; std::memcpy(&value,&input.bits,4);
        const unsigned length = 1 + (rng >> 16) % 65535;
        downwell::RunnerImageValue result;
        check(SequenceTimeline::remainder(input,length,result));
        check(result.bits == stored(float(std::fmod(double(value),double(length)))).bits);
        check(SequenceTimeline::wrap(input,length,result));
        check(result.bits == stored(wrapped(value,length)).bits);
        ++cases;
    }
    // Integer binary32 sums against the binary64 route and host arithmetic:
    // exponent gaps 0..9 around the 7-bit fast domain, signs, ties (low
    // mantissa bits), carries, cancellation and both exponent extremes.
    unsigned fast = 0;
    for (unsigned i = 0; i < 2000000; ++i) {
        rng = rng * 1664525u + 1013904223u; const auto r1 = rng;
        rng = rng * 1664525u + 1013904223u; const auto r2 = rng;
        rng = rng * 1664525u + 1013904223u; const auto r3 = rng;
        int ea = int(1 + r3 % 254);
        switch (r3 >> 29) { case 0: ea = 1 + (r3 >> 8) % 9; break; case 1: ea = 254 - (r3 >> 8) % 9; break; default: break; }
        int eb = ea - int((r3 >> 12) % 10);
        if (eb < 1) eb = 1;
        std::uint32_t ma = r1 & 0x7fffffu, mb = r2 & 0x7fffffu;
        if ((r3 >> 16) & 1) mb &= ~0xffu;           // ties after alignment
        if ((r3 >> 17) & 1) mb = ma;                // equal mantissas: cancellation and carries
        if ((r3 >> 18) & 3) mb ^= (r3 >> 20) & 7u;  // near-cancellation
        downwell::RunnerImageValue a{(r1 & 0x80000000u) | (std::uint32_t(ea) << 23) | ma};
        downwell::RunnerImageValue b{(r2 & 0x80000000u) | (std::uint32_t(eb) << 23) | mb};
        if ((r3 >> 22) & 1) { const auto t = a; a = b; b = t; }
        if ((r3 & 0xffu) == 0) a.bits &= 0x80000000u; // signed zeros
        if ((r3 & 0xffu) == 1) b.bits &= 0x80000000u;
        float fa, fb; std::memcpy(&fa,&a.bits,4); std::memcpy(&fb,&b.bits,4);
        const double exact = double(fa) + double(fb);
        downwell::RunnerImageValue reference, integer, result;
        auto sum = a.promoted();
        const bool referenceOk = sum.add(b.promoted()) && reference.assign(sum);
        const auto host = stored(float(exact)).bits;
        const unsigned hostField = (host >> 23) & 255u;
        // The binary64 route rejects subnormal and overflowing results.
        if (referenceOk) check(reference.bits == host);
        else check(hostField == 255 || (!hostField && (host & 0x7fffffffu)));
        if (SequenceTimeline::sumBinary32(a.bits,b.bits,integer.bits)) {
            check(referenceOk && integer.bits == reference.bits);
            ++fast;
        }
        check(SequenceTimeline::add(a,b,result) == referenceOk);
        if (referenceOk) check(result.bits == reference.bits);
        ++cases;
    }
    check(fast > 1000000);
    // Exponent gaps beyond sumBinary32's reach, up to past binary32 precision.
    for (unsigned n = 0; n < 4000000; ++n) {
        rng = rng * 1664525u + 1013904223u; const auto r1 = rng;
        rng = rng * 1664525u + 1013904223u; const auto r2 = rng;
        rng = rng * 1664525u + 1013904223u; const auto r3 = rng;
        const int ea = int(1 + r3 % 254), eb = ea - int(8 + (r3 >> 8) % 40);
        if (eb < 1) continue;
        std::uint32_t ma = r1 & 0x7fffffu, mb = r2 & 0x7fffffu;
        if ((r3 >> 16) & 1) ma = 0;                 // powers of two: borrow below a
        if ((r3 >> 17) & 1) mb &= ~0xffffu;
        downwell::RunnerImageValue a{(r1 & 0x80000000u) | (std::uint32_t(ea) << 23) | ma};
        downwell::RunnerImageValue b{(r2 & 0x80000000u) | (std::uint32_t(eb) << 23) | mb};
        if ((r3 >> 22) & 1) { const auto t = a; a = b; b = t; }
        downwell::RunnerImageValue reference, result;
        auto sum = a.promoted();
        const bool referenceOk = sum.add(b.promoted()) && reference.assign(sum);
        check(SequenceTimeline::add(a,b,result) == referenceOk);
        if (referenceOk) check(result.bits == reference.bits);
        ++cases;
    }
    for (unsigned n = 1; n <= 65535; ++n) check(SequenceTimeline::integerBits(n) == stored(float(n)).bits);
    for (unsigned length : {1u,4u,6u,8u}) for (float speed : {-20.0f,-0.45f,-0.3f,-0.05f,0.0f,0.2f,0.3f,20.0f}) {
        float expected = 0;
        auto position = stored(expected);
        for (unsigned tick = 0; tick < 1000; ++tick) {
            expected = float(double(expected) + double(speed));
            const bool boundary = expected < 0 || expected >= float(length);
            if (boundary) expected = wrapped(expected,length);
            bool crossed = !boundary;
            check(SequenceTimeline::advance(position,stored(speed),length,crossed));
            check(position.bits == stored(expected).bits && crossed == boundary);
            ++cases;
        }
    }
    auto position = stored(1); bool crossed = true;
    check(!SequenceTimeline::advance(position,stored(1),0,crossed) && position.bits == stored(1).bits && crossed);
    downwell::RunnerImageValue invalid; invalid.bits = 0x7f800000u;
    check(!SequenceTimeline::advance(position,invalid,4,crossed) && position.bits == stored(1).bits);
    check(!SequenceTimeline::advance(position,stored(1),65536,crossed) && position.bits == stored(1).bits);
    downwell::RunnerImageValue result;
    check(SequenceTimeline::remainder(stored(-0.0f),4,result) && result.bits == 0x80000000u);
    check(SequenceTimeline::wrap(stored(-0.0f),4,result) && result.bits == 0);
    invalid.bits = 1;
    check(!SequenceTimeline::advance(position,invalid,4,crossed) && position.bits == stored(1).bits);
    std::printf("Sequence timeline: %u signed remainder/wrap/advance/sum cases passed (%u integer sums)\n",cases,fast);
}
