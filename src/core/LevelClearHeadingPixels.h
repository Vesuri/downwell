#ifndef DOWNWELL_CORE_LEVEL_CLEAR_HEADING_PIXELS_H
#define DOWNWELL_CORE_LEVEL_CLEAR_HEADING_PIXELS_H
#include <cstdint>
namespace downwell {
// Exact specialization of floorBinary32(base - pass * q30 / 2^30) for
// Result Draw's anchors 80, 96 and 278, and passes 0..6. All intermediate
// binary64 values are exact (at most 39 significant bits). The first two
// anchors stay in [64,128), the last in [256,512). A fractional offset within
// half a float ULP rounds back to the integer anchor; ties choose that even
// float mantissa. Other fractions floor one pixel lower. No trig approximation
// is introduced here: q30 is the existing sampler's output.
struct LevelClearHeadingPixels {
    unsigned char nearOffset[7] = {}, farOffset[7] = {};
    bool compute(std::uint32_t q30) {
        if (q30 > (1u << 30)) return false;
        unsigned whole = 0;
        std::uint32_t fraction = 0;
        for (unsigned pass = 0; pass <= 6; ++pass) {
            nearOffset[pass] = whole + (fraction > (1u << 12));
            farOffset[pass] = whole + (fraction > (1u << 14));
            // fraction < 2^30 and q30 <= 2^30: addition cannot overflow.
            fraction += q30;
            whole += fraction >> 30;
            fraction &= (1u << 30) - 1;
        }
        return true;
    }
    int x(unsigned pass) const { return 80 - nearOffset[pass]; }
    int stageY(unsigned pass) const { return 96 - nearOffset[pass]; }
    int choiceY(unsigned pass) const { return 278 - farOffset[pass]; }
};
}
#endif
