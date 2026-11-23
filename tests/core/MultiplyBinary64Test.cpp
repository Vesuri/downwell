// multiplyBinary64 against the host's IEEE binary64 multiply (round to
// nearest-even) and against the earlier base-65536 carry-chain product it
// replaced, over edge mantissas and random operands. Every admitted result
// must equal both bit for bit; a refused one must be subnormal or not finite.
#include "core/MultiplyBinary64.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

using downwell::MotionBinary64;

bool reference(MotionBinary64 a, MotionBinary64 b, MotionBinary64& out) {
    if (!a.valid() || !b.valid()) return false;
    const auto sign=(a.high^b.high)&0x80000000u;
    if (a.zero() || b.zero()) {out={sign,0};return true;}
    const std::uint32_t av[4]={a.low&65535u,a.low>>16,a.high&65535u,((a.high>>16)&15u)|16u};
    const std::uint32_t bv[4]={b.low&65535u,b.low>>16,b.high&65535u,((b.high>>16)&15u)|16u};
    std::uint32_t product[8]={};
    for (unsigned i=0;i<4;++i) {
        std::uint32_t carry=0;
        for (unsigned j=0;j<4;++j) {
            const auto sum=av[i]*bv[j]+product[i+j]+carry;
            product[i+j]=sum&65535u;carry=sum>>16;
        }
        product[i+4]=carry;
    }
    const auto p0=product[0]|(product[1]<<16),p1=product[2]|(product[3]<<16);
    const auto p2=product[4]|(product[5]<<16),p3=product[6]|(product[7]<<16);
    const unsigned extra=(p3&512u)?1u:0u, shift=20+extra;
    int exponent=int((a.high>>20)&2047)+int((b.high>>20)&2047)-1023+int(extra);
    MotionBinary64::Words mantissa{(p3<<(32-shift))|(p2>>shift),(p2<<(32-shift))|(p1>>shift)};
    const auto guard=1u<<(shift-1);
    if ((p1&guard) && ((p1&(guard-1)) || p0 || (mantissa.low&1))) mantissa.add({0,1});
    if (mantissa.high&0x200000u) {
        mantissa={mantissa.high>>1,(mantissa.low>>1)|(mantissa.high<<31)};++exponent;
    }
    if (exponent<=0 || exponent>=2047) return false;
    out={sign|(unsigned(exponent)<<20)|(mantissa.high&0xfffffu),mantissa.low};
    return true;
}

std::uint64_t state = 0x9e3779b97f4a7c15ull;
std::uint64_t next() {
    state ^= state << 13; state ^= state >> 7; state ^= state << 17;
    return state;
}

double real(MotionBinary64 v) {
    const std::uint64_t bits = (std::uint64_t(v.high) << 32) | v.low;
    double d; std::memcpy(&d, &bits, 8); return d;
}

MotionBinary64 bits(std::uint64_t v) { return {std::uint32_t(v >> 32), std::uint32_t(v)}; }

unsigned failures = 0, admitted = 0;

void check(MotionBinary64 a, MotionBinary64 b) {
    MotionBinary64 got{}, old{};
    const bool ok = downwell::multiplyBinary64(a, b, got), oldOk = reference(a, b, old);
    if (ok != oldOk || (ok && (got.high != old.high || got.low != old.low))) {
        if (++failures <= 10) std::printf("differs from the carry-chain product: %08x%08x * %08x%08x\n",
                                          a.high, a.low, b.high, b.low);
        return;
    }
    if (!a.valid() || !b.valid()) return;
    const double host = real(a) * real(b);
    std::uint64_t hostBits; std::memcpy(&hostBits, &host, 8);
    if (ok) {
        ++admitted;
        if (bits(hostBits).high != got.high || bits(hostBits).low != got.low) {
            if (++failures <= 10) std::printf("differs from IEEE: %08x%08x * %08x%08x\n", a.high, a.low, b.high, b.low);
        }
    } else if (std::isfinite(host) && std::fabs(host) > 2.2250738585072014e-308) {
        // A product tiny before rounding is refused even when it rounds to
        // the smallest normal (both implementations do).
        if (++failures <= 10) std::printf("refused a normal product: %08x%08x * %08x%08x\n", a.high, a.low, b.high, b.low);
    }
}

} // namespace

int main() {
    // Mantissa extremes: all zeros, all ones, single bits and the halfway
    // patterns that exercise the guard and sticky bits, at several exponents.
    std::uint64_t mantissas[64]; unsigned n = 0;
    mantissas[n++] = 0; mantissas[n++] = 0xfffffffffffffull; mantissas[n++] = 1;
    mantissas[n++] = 0x8000000000000ull; mantissas[n++] = 0x7ffffffffffffull;
    for (unsigned bit = 0; bit < 52; bit += 3) mantissas[n++] = 1ull << bit;
    mantissas[n++] = 0x5555555555555ull; mantissas[n++] = 0xaaaaaaaaaaaaaull;
    mantissas[n++] = 0xffff0000ffffull; mantissas[n++] = 0x0000ffff0000full;
    const unsigned exponents[] = {1, 2, 511, 1022, 1023, 1024, 1535, 2045, 2046};
    for (unsigned i = 0; i < n; ++i)
        for (unsigned j = 0; j < n; ++j)
            for (unsigned ea : exponents)
                for (unsigned eb : exponents)
                    for (unsigned s = 0; s < 4; ++s)
                        check(bits((std::uint64_t(s & 1) << 63) | (std::uint64_t(ea) << 52) | mantissas[i]),
                              bits((std::uint64_t(s >> 1) << 63) | (std::uint64_t(eb) << 52) | mantissas[j]));
    check({0, 0}, {0x3ff00000u, 0}); check({0x80000000u, 0}, {0x3ff00000u, 0});
    // Random operands near the useful range, then anywhere.
    for (unsigned i = 0; i < 20000000u; ++i) {
        std::uint64_t x = next(), y = next();
        if (i & 1) {
            x = (x & 0x800fffffffffffffull) | (std::uint64_t(1023 - 40 + (next() % 80)) << 52);
            y = (y & 0x800fffffffffffffull) | (std::uint64_t(1023 - 40 + (next() % 80)) << 52);
        }
        check(bits(x), bits(y));
    }
    if (failures) { std::printf("multiplyBinary64: %u failures\n", failures); return 1; }
    std::printf("multiplyBinary64: %u admitted products equal IEEE and the carry-chain product\n", admitted);
    return 0;
}
