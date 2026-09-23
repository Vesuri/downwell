#ifndef DOWNWELL_GAME_MAKER_ROUNDING_H
#define DOWNWELL_GAME_MAKER_ROUNDING_H

namespace downwell {

// GML round: nearest integer, exact halves to even. Coordinates must be
// finite and fit a signed long. Explicit arithmetic avoids dependence on
// the host floating-point rounding mode and needs no target FPU/libm.
inline long roundGameMakerCoordinate(double value) {
    const long whole = static_cast<long>(value);
    const double fraction = value - whole;
    if (fraction > 0.5 || (fraction == 0.5 && whole % 2 != 0))
        return whole + 1;
    if (fraction < -0.5 || (fraction == -0.5 && whole % 2 != 0))
        return whole - 1;
    return whole;
}

inline long roundGameMakerQ8(long value) {
    const long whole = value / 256;
    const long fraction = value % 256;
    if (fraction > 128 || (fraction == 128 && whole % 2 != 0))
        return whole + 1;
    if (fraction < -128 || (fraction == -128 && whole % 2 != 0))
        return whole - 1;
    return whole;
}

} // namespace downwell
#endif
