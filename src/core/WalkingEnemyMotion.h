#ifndef DOWNWELL_CORE_WALKING_ENEMY_MOTION_H
#define DOWNWELL_CORE_WALKING_ENEMY_MOTION_H
#include "SolidCollision.h"
#include "EnemyContact.h"
namespace downwell {
struct NoWalkingPresentation {
    bool operator()(int,int)const{return true;}
};
// Shared dry-world enmSnake/enmGroundTurtle Step movement, in fifths of Q8.
// Math selects how a coordinate is held; FixedCollisionMath is the oracle.
template<class Math>
struct BasicWalkingEnemyMotion {
    using Number = typename Math::Number;
    Number x{}, y{}, vx{}, vy{};
    int facing = 1;
    int publishedX = 0, publishedY = 0;
    bool grounded = true, initialized = false;
    void initialize(int px, int py, long speedX, long speedY, bool ground) {
        x = Math::pixels(px); y = Math::pixels(py); vx = Math::fixed(speedX); vy = Math::fixed(speedY);
        facing = vx < Number{} ? -1 : 1; grounded = ground; initialized = true;
        publishedX = px; publishedY = py;
    }
    template<class Solid>
    bool meetsTiles(Number px, Number py, bool turtle, Solid solid) const {
        const auto b = contactPixelBounds(Math::roundPixel(px), Math::roundPixel(py),
            facing, 1, turtle ? 186 : 163);
        // Supported area-one terrain consists of aligned 16x16 cells.
        return solidCorners(solid,b.left,b.top,b.right-1,b.bottom-1);
    }
    template<class Meeting,class Presentation=NoWalkingPresentation>
    bool step(bool turtle, bool walking, bool visible, Meeting meeting,Presentation presentation={}) {
        const Number zero{}, one = Math::pixels(1);
        vy += Math::fixed(256);
        if (vy >= Math::pixels(7)) vy = Math::pixels(7);
        // Source clears active whenever grounded, then checks !active && grounded.
        if (grounded && !meeting(x + Math::pixels(((vx > zero) - (vx < zero)) * (turtle ? 12 : 8)),
                                  y + one)) vx = -vx;
        if (meeting(x + vx, y)) {
            const Number delta = Math::pixels((vx > zero) - (vx < zero));
            while (!meeting(x + delta, y)) x += delta;
            vx = -vx;
        }
        if (meeting(x, y + vy)) {
            if (vy > zero) {
                if (meeting(x, y + one)) { vy = zero; grounded = true; }
                else {
                    Number distance{};
                    for (Number i = one; !meeting(x, y + i); i += one) distance += one;
                    vy = distance;
                }
            }
        } else grounded = meeting(x, y + one);
        // VM Step selects sprite, then writes scale only for nonzero xsp,
        // before the visibility-gated xx/yy integration and x/y setters.
        const int sprite=turtle ? (!grounded ? 187 : walking ? 188 : 186) : grounded ? 163 : 164;
        const int scale=(vx>zero)-(vx<zero);
        if(!presentation(sprite,scale))return false;
        if (scale != 0) facing = scale;
        if (visible) {
            if (!turtle || walking) x += vx;
            y += vy;
            publishedX = Math::roundPixel(x);
            publishedY = Math::roundPixel(y);
        }
        return true;
    }
};
using WalkingEnemyMotion = BasicWalkingEnemyMotion<FixedCollisionMath>;
}
#endif
