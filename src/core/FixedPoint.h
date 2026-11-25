#ifndef DOWNWELL_CORE_FIXED_POINT_H
#define DOWNWELL_CORE_FIXED_POINT_H
#include <cstdint>
#include "MotionBinary64.h"
namespace downwell {
// Q16.16 fixed point for motion and steering (2026-11-25 decision: the port
// no longer reproduces the VM's binary64 rounding bit for bit; host and
// Amiga share this arithmetic, so they still agree with each other).
// Range +-32767.99998, resolution 1/65536. Trig comes from quarter-wave
// tables with linear interpolation: sine within 2e-5, atan within 1e-5
// degrees. No floating point, divisions only 32/32.
namespace fixed_detail {
// round(sin(i * 90 / 256 degrees) * 65536) and
// round(atan(i / 256) in degrees * 65536), i = 0..256.
inline constexpr std::int32_t fixedSineQuarter[257]={
    0,402,804,1206,1608,2010,2412,2814,3216,3617,4019,4420,
    4821,5222,5623,6023,6424,6824,7224,7623,8022,8421,8820,9218,
    9616,10014,10411,10808,11204,11600,11996,12391,12785,13180,13573,13966,
    14359,14751,15143,15534,15924,16314,16703,17091,17479,17867,18253,18639,
    19024,19409,19792,20175,20557,20939,21320,21699,22078,22457,22834,23210,
    23586,23961,24335,24708,25080,25451,25821,26190,26558,26925,27291,27656,
    28020,28383,28745,29106,29466,29824,30182,30538,30893,31248,31600,31952,
    32303,32652,33000,33347,33692,34037,34380,34721,35062,35401,35738,36075,
    36410,36744,37076,37407,37736,38064,38391,38716,39040,39362,39683,40002,
    40320,40636,40951,41264,41576,41886,42194,42501,42806,43110,43412,43713,
    44011,44308,44604,44898,45190,45480,45769,46056,46341,46624,46906,47186,
    47464,47741,48015,48288,48559,48828,49095,49361,49624,49886,50146,50404,
    50660,50914,51166,51417,51665,51911,52156,52398,52639,52878,53114,53349,
    53581,53812,54040,54267,54491,54714,54934,55152,55368,55582,55794,56004,
    56212,56418,56621,56823,57022,57219,57414,57607,57798,57986,58172,58356,
    58538,58718,58896,59071,59244,59415,59583,59750,59914,60075,60235,60392,
    60547,60700,60851,60999,61145,61288,61429,61568,61705,61839,61971,62101,
    62228,62353,62476,62596,62714,62830,62943,63054,63162,63268,63372,63473,
    63572,63668,63763,63854,63944,64031,64115,64197,64277,64354,64429,64501,
    64571,64639,64704,64766,64827,64884,64940,64993,65043,65091,65137,65180,
    65220,65259,65294,65328,65358,65387,65413,65436,65457,65476,65492,65505,
    65516,65525,65531,65535,65536,
};
inline constexpr std::int32_t fixedAtanDegrees[257]={
    0,14668,29335,44001,58666,73329,87990,102648,117304,131955,146603,161246,
    175884,190517,205144,219765,234379,248986,263585,278177,292760,307334,321899,336454,
    350999,365534,380058,394570,409070,423558,438034,452496,466945,481380,495801,510207,
    524598,538973,553333,567676,582003,596312,610605,624879,639135,653372,667591,681790,
    695970,710129,724268,738387,752484,766560,780613,794645,808654,822641,836604,850544,
    864460,878352,892219,906062,919879,933671,947438,961178,974893,988580,1002241,1015875,
    1029481,1043060,1056611,1070133,1083627,1097092,1110529,1123936,1137313,1150661,1163979,1177267,
    1190524,1203751,1216947,1230111,1243245,1256347,1269417,1282455,1295461,1308435,1321376,1334285,
    1347161,1360004,1372813,1385590,1398332,1411041,1423717,1436358,1448965,1461538,1474076,1486580,
    1499049,1511483,1523882,1536246,1548575,1560868,1573127,1585349,1597536,1609687,1621803,1633882,
    1645926,1657933,1669904,1681839,1693738,1705600,1717426,1729215,1740967,1752683,1764362,1776004,
    1787610,1799179,1810710,1822205,1833663,1845084,1856467,1867814,1879123,1890396,1901631,1912829,
    1923990,1935113,1946200,1957249,1968261,1979236,1990173,2001074,2011937,2022763,2033552,2044303,
    2055018,2065695,2076336,2086939,2097505,2108034,2118526,2128981,2139399,2149780,2160125,2170432,
    2180703,2190937,2201134,2211295,2221419,2231507,2241558,2251572,2261551,2271492,2281398,2291267,
    2301101,2310898,2320659,2330384,2340074,2349727,2359345,2368927,2378474,2387985,2397460,2406901,
    2416306,2425675,2435010,2444310,2453574,2462804,2471999,2481159,2490285,2499376,2508433,2517455,
    2526443,2535397,2544317,2553203,2562055,2570873,2579658,2588409,2597126,2605811,2614461,2623079,
    2631664,2640215,2648734,2657220,2665673,2674093,2682482,2690837,2699161,2707452,2715711,2723939,
    2732134,2740298,2748430,2756531,2764600,2772638,2780644,2788620,2796564,2804478,2812361,2820213,
    2828035,2835826,2843587,2851318,2859019,2866690,2874330,2881941,2889523,2897075,2904597,2912090,
    2919554,2926989,2934395,2941772,2949120,
};
}
struct Fixed16 {
    std::int32_t v = 0;
    static constexpr Fixed16 raw(std::int32_t value) { return Fixed16{value}; }
    static constexpr Fixed16 fromInteger(std::int32_t value) { return Fixed16{value * 65536}; }
    // Nearest Q16 (halves away from zero); false outside the range or for
    // a value MotionBinary64 rejects.
    static bool fromBinary64(MotionBinary64 value, Fixed16& out) {
        if (!value.valid()) return false;
        if (value.zero()) { out.v = 0; return true; }
        const int exponent = int((value.high >> 20) & 2047u) - 1023;
        if (exponent > 14) return false;
        if (exponent < -18) { out.v = 0; return true; }
        // significand m (53 bits) * 2^(exponent - 52) * 2^16 = m >> (36 - exponent)
        const std::uint32_t mh = (value.high & 0xfffffu) | 0x100000u, ml = value.low;
        const unsigned shift = unsigned(36 - exponent); // 22..54
        std::uint32_t magnitude, half;
        if (shift >= 32) {
            const unsigned s = shift - 32; // 0..22
            magnitude = mh >> s;
            half = s ? (mh >> (s - 1)) & 1u : ml >> 31;
        } else {
            magnitude = (mh << (32 - shift)) | (ml >> shift);
            half = (ml >> (shift - 1)) & 1u;
        }
        magnitude += half;
        if (magnitude > 0x7fffffffu) return false;
        out.v = (value.high & 0x80000000u) ? -std::int32_t(magnitude) : std::int32_t(magnitude);
        return true;
    }
    // Exact: every Q16 value is a binary64.
    MotionBinary64 binary64() const {
        if (!v) return {};
        MotionBinary64 r = MotionBinary64::fromInteger(v);
        r.high -= 16u << 20;
        return r;
    }
    // GameMaker round (halves to even) of a Q16 value, as a whole number.
    static std::int32_t roundPixel(std::int32_t q16) {
        std::int32_t whole = (q16 + 0x8000) >> 16;
        if ((q16 & 0xffff) == 0x8000 && (whole & 1)) --whole;
        return whole;
    }
    // v * factor / 65536 for a factor below 1 (0..65535), rounded to
    // nearest with 32-bit products: the whole pixels' product is exact,
    // only the fraction's is rounded. Symmetric about zero.
    static std::int32_t scale(std::int32_t v, std::uint32_t factor) {
        const std::uint32_t m = std::uint32_t(v < 0 ? -v : v);
        const std::int32_t r = std::int32_t((m >> 16) * factor + (((m & 0xffffu) * factor + 0x8000u) >> 16));
        return v < 0 ? -r : r;
    }
    Fixed16 operator+(Fixed16 o) const { return Fixed16{v + o.v}; }
    Fixed16 operator-(Fixed16 o) const { return Fixed16{v - o.v}; }
    Fixed16 operator-() const { return Fixed16{-v}; }
    bool operator<(Fixed16 o) const { return v < o.v; }
    bool operator>(Fixed16 o) const { return v > o.v; }
    bool operator==(Fixed16 o) const { return v == o.v; }
    // 68020 muls.l: 32 x 32 -> 64.
    static Fixed16 multiply(Fixed16 a, Fixed16 b) {
        return Fixed16{std::int32_t((std::int64_t(a.v) * b.v) >> 16)};
    }
};
struct FixedTrig {
    // sin of Q16 degrees; any angle, reduced to [0, 360).
    static std::int32_t sine(std::int32_t degrees) {
        constexpr std::int32_t full = 360 * 65536;
        degrees %= full; if (degrees < 0) degrees += full;
        // 1024 table steps per turn: position = degrees * 128 / 45 (Q16).
        const std::uint32_t position = std::uint32_t(degrees) * 128u / 45u;
        const unsigned step = unsigned(position >> 16), quarter = step >> 8, index = step & 255u;
        const std::int32_t fraction = std::int32_t(position & 0xffffu);
        const auto* t = fixed_detail::fixedSineQuarter;
        std::int32_t a, b;
        if (quarter & 1) { a = t[256 - index]; b = t[255 - index]; }
        else { a = t[index]; b = t[index + 1]; }
        const std::int32_t value = a + std::int32_t(((b - a) * fraction) >> 16);
        return quarter & 2 ? -value : value;
    }
    static std::int32_t cosine(std::int32_t degrees) { return sine(degrees + 90 * 65536); }
    // atan(n / d) in Q16 degrees for 0 <= n <= d, d > 0.
    static std::int32_t atanRatio(std::uint32_t n, std::uint32_t d) {
        // Scale d below 2^16 (n <= d, so n << 16 fits), rounding both.
        unsigned shift = 0;
        while ((d >> shift) >= 0x10000u) ++shift;
        if (shift) {
            const std::uint32_t half = 1u << (shift - 1);
            n = (n >> shift) + ((n & ((half << 1) - 1)) >= half);
            d = (d >> shift) + ((d & ((half << 1) - 1)) >= half);
            if (d >= 0x10000u) { n >>= 1; d >>= 1; }
            if (n > d) n = d;
        }
        if (!d) return 0;
        const std::uint32_t ratio = (n << 16) / d; // Q16, <= 1
        const unsigned index = unsigned(ratio >> 8);
        if (index >= 256) return fixed_detail::fixedAtanDegrees[256];
        const std::int32_t a = fixed_detail::fixedAtanDegrees[index], b = fixed_detail::fixedAtanDegrees[index + 1];
        return a + std::int32_t(((b - a) * std::int32_t(ratio & 255u)) >> 8);
    }
    // GameMaker point_direction for screen coordinates (y down): degrees
    // counter-clockwise from +x in [0, 360); 0 for equal points.
    static std::int32_t pointDirection(std::int32_t dx, std::int32_t dy) {
        const std::int32_t ry = -dy;
        const std::uint32_t ax = std::uint32_t(dx < 0 ? -dx : dx), ay = std::uint32_t(ry < 0 ? -ry : ry);
        if (!ax && !ay) return 0;
        const std::int32_t theta = ax >= ay ? atanRatio(ay, ax) : 90 * 65536 - atanRatio(ax, ay);
        if (dx >= 0) return ry >= 0 ? theta : (theta ? 360 * 65536 - theta : 0);
        return ry >= 0 ? 180 * 65536 - theta : 180 * 65536 + theta;
    }
    // lengthdir_x (cosine) or lengthdir_y (-sine) of a Q16 length; like the
    // runner wrapper, fractions under 0.0001 snap to the integer part.
    static std::int32_t lengthdir(bool vertical, std::int32_t length, std::int32_t degrees) {
        const std::int32_t trig = vertical ? -sine(degrees) : cosine(degrees);
        std::int32_t value = std::int32_t((std::int64_t(length) * trig) >> 16);
        const std::int32_t magnitude = value < 0 ? -value : value, fraction = magnitude & 0xffff;
        if (fraction && fraction < 7) value = value < 0 ? -(magnitude & ~0xffff) : magnitude & ~0xffff;
        return value;
    }
};
}
#endif
