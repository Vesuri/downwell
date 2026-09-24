#ifndef DOWNWELL_CORE_LEVEL_CLEAR_CAROUSEL_H
#define DOWNWELL_CORE_LEVEL_CLEAR_CAROUSEL_H
#include "HudReal.h"
namespace downwell {
// Signed magnitude wrapper for Result's carousel only. Preserve rounding at
// each source subtraction/division/addition, including crossing screen x=0.
struct LevelClearCoordinate {
    HudReal magnitude;
    bool negative = false;
    static LevelClearCoordinate integer(int value) {
        return {HudReal::integer(value < 0 ? unsigned(-value) : unsigned(value)), value < 0};
    }
    LevelClearCoordinate negate() const { return {magnitude, magnitude.mantissa && !negative}; }
    LevelClearCoordinate add(LevelClearCoordinate other) const {
        if (negative == other.negative) return {magnitude.add(other.magnitude), negative};
        if (magnitude.less(other.magnitude)) return {other.magnitude.subtract(magnitude), other.negative};
        const auto result = magnitude.subtract(other.magnitude);
        return {result, result.mantissa && negative};
    }
    LevelClearCoordinate divideFive() const { return {magnitude.divideSmall(5), negative}; }
};
struct LevelClearCarousel {
    LevelClearCoordinate position = LevelClearCoordinate::integer(42);
    int target = 42;
    void create(unsigned cursor) {
        target = 80 - 38 * static_cast<int>(cursor);
        position = LevelClearCoordinate::integer(target);
    }
    void step(unsigned cursor) {
        target = 80 - 38 * static_cast<int>(cursor);
        const auto delta = LevelClearCoordinate::integer(target).add(position.negate());
        position = position.add(delta.divideFive());
    }
    LevelClearCoordinate optionX(unsigned index) const {
        return position.add(LevelClearCoordinate::integer(38 * static_cast<int>(index)));
    }
};
}
#endif
