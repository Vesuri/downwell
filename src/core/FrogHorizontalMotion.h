#ifndef DOWNWELL_CORE_FROG_HORIZONTAL_MOTION_H
#define DOWNWELL_CORE_FROG_HORIZONTAL_MOTION_H

#include "EnemyContact.h"

namespace downwell {

// Q16 keeps rebound displacement out of platform-specific float/Q8 mirrors.
// Source enmFrog Step multiplies xsp by -0.3. Integer rounding here is a
// bounded approximation, not bit-identical GameMaker double arithmetic.
struct FrogHorizontalMotion {
    long position = 0;
    long velocity = 0;
    static long toQ8(long value) {
        return value < 0 ? -((-value + 128) / 256) : (value + 128) / 256;
    }
    template<class Meeting>
    void step(Meeting meeting) {
        const long next = position + velocity;
        const long pixel = next / 65536 - (next % 65536 < 0 ? 1 : 0);
        const auto mask = contactSpriteBounds(166);
        if (meeting(static_cast<int>(pixel) + (velocity < 0 ? mask.left : mask.right - 1))) {
            const long numerator = -velocity * 3;
            velocity = numerator < 0 ? -((-numerator + 5) / 10) : (numerator + 5) / 10;
        } else {
            position = next;
        }
    }
};

} // namespace downwell
#endif
