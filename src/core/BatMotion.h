#ifndef DOWNWELL_CORE_BAT_MOTION_H
#define DOWNWELL_CORE_BAT_MOTION_H

#include "GameMakerRounding.h"
#include "BatHitState.h"
#include "SolidCollision.h"
#include "EnemyContact.h"
#include <cstdint>

namespace downwell {

// Angles are degrees Q16; positions/velocities Q8; speed is exact 1/40 pixels.
// CORDIC point_direction and an interpolated one-degree sine table avoid target floating
// point and the old ten-degree heading bins. Trig is a bounded approximation,
// not a claim of bit-identical GameMaker double arithmetic.
inline long batPointDirection(long dx, long screenDy) {
    if (dx == 0) return screenDy < 0 ? 90L * 65536 : screenDy > 0 ? 270L * 65536 : 0;
    if (screenDy == 0) return dx < 0 ? 180L * 65536 : 0;
    long x = dx, y = -screenDy, angle = 0;
    long magnitude = (x < 0 ? -x : x);
    const long ay = y < 0 ? -y : y;
    if (ay > magnitude) magnitude = ay;
    while (magnitude > (1L << 22)) { x /= 2; y /= 2; magnitude /= 2; }
    while (magnitude < (1L << 20)) { x *= 2; y *= 2; magnitude *= 2; }
    if (x < 0) { x = -x; y = -y; angle = 180L * 65536; }
    // round(atan(2^-i) * 180/pi * 65536), generated mathematically.
    static const long angles[] = {
        2949120, 1740967, 919879, 466945, 234379, 117304, 58666, 29335,
        14668, 7334, 3667, 1833, 917, 458, 229, 115
    };
    for (unsigned i = 0; i < 16 && y != 0; ++i) {
        // Explicit arithmetic shift: no implementation-defined negative >>.
        const long shiftedX = x >= 0 ? x >> i : -1 - ((-1 - x) >> i);
        const long shiftedY = y >= 0 ? y >> i : -1 - ((-1 - y) >> i);
        if (y > 0) { x += shiftedY; y -= shiftedX; angle += angles[i]; }
        else { x -= shiftedY; y += shiftedX; angle -= angles[i]; }
    }
    if (angle < 0) angle += 360L * 65536;
    if (angle >= 360L * 65536) angle -= 360L * 65536;
    return angle;
}

inline int batNormalizeDirection(int direction) {
    direction %= 360;
    return direction < 0 ? direction + 360 : direction;
}

inline long batSine(int direction) {
    // round(sin(degrees*pi/180)*32768), quarter-wave symmetry.
    static const std::uint16_t sine[] = {
        0, 572, 1144, 1715, 2286, 2856, 3425, 3993, 4560, 5126, 5690, 6252, 6813,
        7371, 7927, 8481, 9032, 9580, 10126, 10668, 11207, 11743, 12275, 12803, 13328, 13848,
        14365, 14876, 15384, 15886, 16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720, 20174,
        20622, 21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730, 25102, 25466,
        25822, 26170, 26510, 26842, 27166, 27482, 27789, 28088, 28378, 28660, 28932, 29197, 29452,
        29698, 29935, 30163, 30382, 30592, 30792, 30983, 31164, 31336, 31499, 31651, 31795, 31928,
        32052, 32166, 32270, 32365, 32449, 32524, 32588, 32643, 32688, 32723, 32748, 32763, 32768
    };
    direction = batNormalizeDirection(direction);
    const bool negative = direction >= 180;
    if (negative) direction -= 180;
    if (direction > 90) direction = 180 - direction;
    return negative ? -(long)sine[direction] : (long)sine[direction];
}

struct BatMotion {
    long x, y;
    long speedX, speedY;
    int direction;
    int speedFortieths;
    int facing = 1;
    unsigned short directionFraction = 0; // Q16 tail retained across pursuit turns.
    int collisionMask=153;
};

inline long batFractionalSine(int direction, unsigned short fraction) {
    // The adjacent Q15 values differ by at most 572: multiplication fits
    // signed 32 bits on 68000. Keep integer headings bit-for-bit unchanged.
    const long first = batSine(direction);
    if (!fraction) return first;
    return first + (batSine(direction + 1) - first) * fraction / 65536L;
}

template<class Solid>
bool batMeetsTiles(long x, long y, int facing, Solid solid,int mask=153) {
    const auto b = contactWorldBounds({FixedCollisionMath::round(x) / 5,
        FixedCollisionMath::round(y) / 5, facing, 1, mask});
    return solid(b.left, b.top) || solid(b.right - 1, b.top) ||
           solid(b.left, b.bottom - 1) || solid(b.right - 1, b.bottom - 1);
}

// Bat Step calls scrCheckCollisionWith, zeros only the blocked components,
// calls xscaleXsp on the remaining horizontal velocity, then integrates.
struct NoBatScale { bool operator()(int)const{return true;} };
template<class Solid,class Scale=NoBatScale>
bool resolveBatMotion(BatMotion& bat, Solid solid,Scale scale={}) {
    long x = bat.x * 5, y = bat.y * 5;
    const auto contact = resolveSolidCollision<FixedCollisionMath>(x, y,
        bat.speedX * 5, bat.speedY * 5, [&](long px, long py) {
            return batMeetsTiles(px, py, bat.facing, solid,bat.collisionMask);
        });
    if (contact.x) bat.speedX = 0;
    if (contact.y) bat.speedY = 0;
    bat.x=x/5;bat.y=y/5;
    // xscaleXsp requires abs(xsp) > .2. In the existing Q8 motion
    // representation 51/256 is below that boundary and 52/256 is above.
    if (bat.speedX>51 || bat.speedX<-51) {
        const int facing=bat.speedX<0?-1:1;
        if(!scale(facing))return false;
        bat.facing=facing;
    }
    bat.x += bat.speedX;
    bat.y += bat.speedY;
    return true;
}

// Active/non-stunned enmBat Step subset. Caller supplies exactly one nextU32
// draw for random_range(-20,20); no extra or integer-rounded jitter draws.
// The rectangle query supports aligned 16x16 terrain cells. Exact runner mask
// rounding and parentWall filtering remain adapter work. stepActiveBat gates stun.
template<class Solid,class Scale=NoBatScale>
bool stepBatMotion(BatMotion& bat, long targetX, long targetY,
                   std::uint32_t random, Solid solid,Scale scale={}) {
    const long jitter = (long)(random >> 16) * 40 +
        (long)(((random & 65535u) * 40u) >> 16) - 20L * 65536;
    const long target = batPointDirection(targetX - bat.x, targetY - bat.y) + jitter;
    long difference = (long)batNormalizeDirection(bat.direction) * 65536 + bat.directionFraction - target;
    if (difference > 180L * 65536) difference -= 360L * 65536;
    else if (difference < -180L * 65536) difference += 360L * 65536;
    // Preserve the VM's ordered branches, including acceleration overshoot.
    if ((difference < 0 ? -difference : difference) < 20L * 65536) {
        if (bat.speedFortieths < 50) bat.speedFortieths += 2;
    } else if (bat.speedFortieths > 40) {
        --bat.speedFortieths;
    }
    if (bat.speedFortieths > 50) bat.speedFortieths -= 8;
    if (difference > 0) bat.direction -= 10;
    else if (difference < 0) bat.direction += 10;
    bat.direction = batNormalizeDirection(bat.direction);
    bat.speedX = batFractionalSine(bat.direction + 90, bat.directionFraction) * bat.speedFortieths / 5120;
    bat.speedY = -batFractionalSine(bat.direction, bat.directionFraction) * bat.speedFortieths / 5120;
    return resolveBatMotion(bat, solid,scale);
}

// Recovered active-bat hit branch precedes pursuit. Stun skips both movement
// and its random jitter draw; it does not integrate the new speed immediately.
struct BatActiveStep { bool hit; bool succeeded; };
template<class Random, class Solid,class Scale=NoBatScale>
BatActiveStep stepActiveBat(BatMotion& bat, BatHitState& hit, long targetX, long targetY,
                   Random& random, Solid solid,Scale scale={}) {
    const bool consumed = hit.consume();
    if (consumed) {
        bat.direction = hit.direction; bat.directionFraction = hit.fraction;
        bat.speedFortieths = 120;
    }
    const bool succeeded=hit.stunned || stepBatMotion(bat, targetX, targetY, random.nextU32(), solid,scale);
    return {consumed,succeeded};
}

} // namespace downwell
#endif
