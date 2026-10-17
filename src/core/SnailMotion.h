#ifndef DOWNWELL_CORE_SNAIL_MOTION_H
#define DOWNWELL_CORE_SNAIL_MOTION_H
#include "SolidCollision.h"
#include "EnemyContact.h"
#include "RunnerTransformField.h"
namespace downwell {
struct NoSnailTransform {
    bool operator()(RunnerTransformField,int)const{return true;}
};
// Math selects how a coordinate is held; FixedCollisionMath is the oracle.
template<class Math>
struct BasicSnailMotion {
    using Number = typename Math::Number;
    Number x{}, y{}, vy = Math::fixed(128); // fifth-Q8: exactly 0.1 pixel
    int facing = 1, verticalFlip = 1;
    bool initialized = false, attached = false;
    void initialize(int px, int py, bool descending) {
        *this = {};
        x = Math::pixels(px); y = Math::pixels(py);
        vy = Math::fixed(descending ? 128 : -128);
        initialized = true;
    }
    static int pixel(Number p) { return Math::floorPixel(p); }
    template<class Solid,class Transform=NoSnailTransform>
    bool step(Solid solid, bool timeStopped = false,Transform transform={}) {
        const auto meeting = [this, &solid](Number px, Number py) {
            const int ix = Math::roundPixel(px);
            const int iy = Math::roundPixel(py);
            const auto b = contactPixelBounds(ix, iy, facing, verticalFlip, 177);
            // Current area-one wall cells are 16x16; this mask is smaller.
            return solidCorners(solid, b.left, b.top, b.right - 1, b.bottom - 1);
        };
        if (!attached) {
            if (meeting(x, y)) return false;
            int nextFacing;
            if (meeting(x + Math::pixels(8), y)) nextFacing = 1;
            else if (meeting(x - Math::pixels(8), y)) nextFacing = -1;
            else return false;
            if(!transform(RunnerTransformField::scaleX,nextFacing))return false;
            facing=nextFacing;
            attached = true;
        }
        if (timeStopped) return true;
        const Number zero{};
        const int sign = (vy > zero) - (vy < zero);
        const auto collision = resolveSolidCollision<Math>(x, y, zero, vy, meeting);
        if (collision.y != 0 || !solid(pixel(x) + facing * 16, pixel(y + Math::pixels(sign * 8))))
            vy = -vy;
        y += vy;
        if (vy != zero) {
            const int nextFlip=vy<zero?1:-1;
            if(!transform(RunnerTransformField::scaleY,nextFlip))return false;
            verticalFlip=nextFlip;
        }
        return true;
    }
};
using SnailMotion = BasicSnailMotion<FixedCollisionMath>;
} // namespace downwell
#endif
