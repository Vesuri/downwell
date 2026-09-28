#ifndef DOWNWELL_CORE_SOLID_COLLISION_H
#define DOWNWELL_CORE_SOLID_COLLISION_H
#include "SmallUnsignedDivision.h"

namespace downwell {

struct SolidCollisionResult {
    int x;
    int y;
};

// scrWallCol blocks 32..35: after an upward contact, instance_place(xx,
// yy-16,objBox_n) marks one box's wallHp zero and changes ysp from zero to 1.
// The callback selects/mutates the world instance; this shared rule owns the
// query offset and response, including the same-frame downward displacement.
template<class Math, class BreakBox>
typename Math::Number ceilingResponse(typename Math::Number x,
    typename Math::Number y, BreakBox breakBox) {
    return breakBox(x, y - 16 * Math::unit()) ? Math::unit() : 0;
}

// VM blocks 12..23, also usable while a caller still resolves X separately.
// Do not call the full two-axis script with an artificial zero X velocity:
// its rounded X probe can move a fractional X into an adjacent wall.
template<class Math, class Query>
bool resolveVerticalSolidCollisionChecked(typename Math::Number x,
    typename Math::Number& y, typename Math::Number vy, Query meeting,int& result) {
    using N = typename Math::Number;
    const int sign = (vy > 0) - (vy < 0);
    const N delta = sign * Math::unit();
    const N magnitude = vy < 0 ? -vy : vy;
    const N probe = Math::round(y) + sign * Math::ceilAbs(vy);
    bool hit=false;
    if(!meeting(x,probe,hit))return false;
    if(!hit){result=0;return true;}
    y = Math::round(y);
    N move = 0;
    for(;;){
        if(!meeting(x,y+move+delta,hit))return false;
        if(hit)break;
        move += delta;
        if ((move < 0 ? -move : move) >= magnitude) {
            move = 0;
            break;
        }
    }
    y += move;
    result=sign;return true;
}

template<class Math, class Query>
int resolveVerticalSolidCollision(typename Math::Number x,
    typename Math::Number& y, typename Math::Number vy, Query meeting) {
    int result=0;
    resolveVerticalSolidCollisionChecked<Math>(x,y,vy,
        [&](typename Math::Number px,typename Math::Number py,bool& hit){
            hit=meeting(px,py);return true;
        },result);
    return result;
}

// scrCheckCollisionWith, VM blocks 1..40. This resolves contact positions and
// returns directions; the caller applies scrWallCol responses and only then
// integrates the remaining velocity. The query owns place_meeting semantics.
// Math supplies Number, unit(), round() and ceilAbs() in its coordinate units.
template<class Math, class Query>
SolidCollisionResult resolveSolidCollision(
    typename Math::Number& x, typename Math::Number& y,
    typename Math::Number vx, typename Math::Number vy, Query meeting) {
    using N = typename Math::Number;
    const int sx = (vx > 0) - (vx < 0);
    const int sy = (vy > 0) - (vy < 0);
    const N dx = sx * Math::unit();
    const N dy = sy * Math::unit();
    const N ax = vx < 0 ? -vx : vx;
    const N ay = vy < 0 ? -vy : vy;
    const N probeX = Math::round(x) + sx * Math::ceilAbs(vx);
    const N probeY = Math::round(y) + sy * Math::ceilAbs(vy);
    SolidCollisionResult result = {0, 0};
    if (meeting(probeX, y)) {
        x = Math::round(x);
        N move = 0;
        while (!meeting(x + move + dx, y)) {
            move += dx;
            if ((move < 0 ? -move : move) >= ax) {
                move = 0;
                break;
            }
        }
        x += move;
        result.x = sx;
    }
    result.y = resolveVerticalSolidCollision<Math>(x, y, vy, meeting);
    if (result.x == 0 && result.y == 0 && meeting(probeX, probeY)) {
        x = Math::round(x);
        y = Math::round(y);
        N moveX = 0;
        N moveY = 0;
        for (;;) {
            if (meeting(x + moveX + dx, y + moveY)) break;
            moveX += dx;
            if (meeting(x + moveX, y + moveY + dy)) break;
            moveY += dy;
            if ((moveY < 0 ? -moveY : moveY) >= ay) {
                moveX = moveY = 0;
                break;
            }
        }
        x += moveX;
        y += moveY;
        result.x = sx;
        result.y = sy;
    }
    return result;
}

// Fifths of Q8 preserve PlayerMotion's gravity and integration residuals.
// No floating point or 64-bit arithmetic is needed on the 68000.
struct FixedCollisionMath {
    using Number = long;
    static Number unit() { return 1280; }
    static Number round(Number value) {
        const bool negative = value < 0;
        const Number magnitude = negative ? -value : value;
        Number integer,remainder;
        if(static_cast<unsigned long>(magnitude)<=0xffffffffUL){
            const auto divided=SmallUnsignedDivision::full(std::uint32_t(magnitude),1280);
            integer=divided.quotient;remainder=divided.remainder;
        }else{
            // Preserve the host's wider long domain; target long is 32 bits.
            integer=magnitude/unit();remainder=magnitude%unit();
        }
        if (remainder > unit() / 2 ||
            (remainder == unit() / 2 && (integer & 1))) ++integer;
        return (negative ? -integer : integer) * unit();
    }
    static Number ceilAbs(Number value) {
        if (value < 0) value = -value;
        return ((value + unit() - 1) / unit()) * unit();
    }
};

} // namespace downwell
#endif
