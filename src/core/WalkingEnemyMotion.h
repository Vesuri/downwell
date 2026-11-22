#ifndef DOWNWELL_CORE_WALKING_ENEMY_MOTION_H
#define DOWNWELL_CORE_WALKING_ENEMY_MOTION_H
#include <limits>
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
    // floorLimit: a y below which no solid can be met (the world's lowest
    // terrain). See the count to the floor below.
    template<class Meeting,class Presentation=NoWalkingPresentation>
    bool step(bool turtle, bool walking, bool visible, Meeting meeting,Presentation presentation={},
              Number floorLimit=std::numeric_limits<Number>::max()) {
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
                    // Counts to the floor. A walker that landed on a wall's
                    // corner by its edge column slides down the wall's side
                    // (its turning x rounds on and off the wall), and once
                    // its top leaves the wall's last row y + vy still meets
                    // it while y + 1 does not. The source then counts to the
                    // next solid below, however far; with none it never
                    // returns. Past floorLimit there is none: keep falling.
                    Number distance{};
                    bool found = true;
                    for (Number i = one; !meeting(x, y + i); i += one) {
                        if (y + i > floorLimit) { found = false; break; }
                        distance += one;
                    }
                    if (found) vy = distance;
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
