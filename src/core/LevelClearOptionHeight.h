#ifndef DOWNWELL_CORE_LEVEL_CLEAR_OPTION_HEIGHT_H
#define DOWNWELL_CORE_LEVEL_CLEAR_OPTION_HEIGHT_H
#include "HudReal.h"
namespace downwell {
// Retained Result Draw program, with |optionX-80| supplied by the caller.
inline HudReal levelClearOptionHeightReference(HudReal displacement, HudReal absoluteSine) {
    const auto distance = displacement.divideSmall(2);
    auto y = HudReal::integer(346).add(distance)
        .subtract(absoluteSine.timesSmall(6)).add(HudReal::integer(8));
    if (HudReal::integer(354).less(y)) y = HudReal::integer(354);
    return y;
}
inline HudReal levelClearOptionHeight(HudReal displacement, HudReal absoluteSine) {
#ifndef DOWNWELL_REFERENCE_CLEAR_OPTION_HEIGHT
    // For a normal sine sample in [0,1], displacement >=12 gives distance
    // >=6 and bob <=6. Each rounded source operation is monotonic, so the
    // result cannot be below 354. The source clamp makes it exactly 354.
    // Skip the entire curve, not just its divide helper. Adjacent options
    // are 38 pixels apart: at most one needs the remaining source program.
    // Non-sine/custom samples retain the general arithmetic fallback.
    if (!HudReal::integer(1).less(absoluteSine) &&
        !displacement.less(HudReal::integer(12))) return HudReal::integer(354);
#endif
    return levelClearOptionHeightReference(displacement, absoluteSine);
}
}
#endif
