#ifndef DOWNWELL_CORE_WALKING_ENEMY_MOTION_H
#define DOWNWELL_CORE_WALKING_ENEMY_MOTION_H
#include "SolidCollision.h"
#include "EnemyContact.h"
namespace downwell {
struct NoWalkingPresentation {
    bool operator()(int,int)const{return true;}
};
// Shared dry-world enmSnake/enmGroundTurtle Step movement, in fifths of Q8.
struct WalkingEnemyMotion {
    long x = 0, y = 0, vx = 0, vy = 0;
    int facing = 1;
    int publishedX = 0, publishedY = 0;
    bool grounded = true, initialized = false;
    void initialize(int px, int py, long speedX, long speedY, bool ground) {
        x = px * 1280L; y = py * 1280L; vx = speedX; vy = speedY;
        facing = vx < 0 ? -1 : 1; grounded = ground; initialized = true;
        publishedX = px; publishedY = py;
    }
    template<class Solid>
    bool meetsTiles(long px, long py, bool turtle, Solid solid) const {
        const auto b = contactWorldBounds({FixedCollisionMath::round(px) / 5,
            FixedCollisionMath::round(py) / 5, facing, 1, turtle ? 186 : 163});
        // Supported area-one terrain consists of aligned 16x16 cells.
        return solid(b.left, b.top) || solid(b.right - 1, b.top) ||
               solid(b.left, b.bottom - 1) || solid(b.right - 1, b.bottom - 1);
    }
    template<class Meeting,class Presentation=NoWalkingPresentation>
    bool step(bool turtle, bool walking, bool visible, Meeting meeting,Presentation presentation={}) {
        vy += 256;
        if (vy >= 7 * 1280L) vy = 7 * 1280L;
        // Source clears active whenever grounded, then checks !active && grounded.
        if (grounded && !meeting(x + ((vx > 0) - (vx < 0)) * (turtle ? 12 : 8) * 1280L,
                                  y + 1280)) vx = -vx;
        if (meeting(x + vx, y)) {
            const long delta = ((vx > 0) - (vx < 0)) * 1280L;
            while (!meeting(x + delta, y)) x += delta;
            vx = -vx;
        }
        if (meeting(x, y + vy)) {
            if (vy > 0) {
                if (meeting(x, y + 1280)) { vy = 0; grounded = true; }
                else {
                    long distance = 0;
                    for (long i = 1280; !meeting(x, y + i); i += 1280) distance += 1280;
                    vy = distance;
                }
            }
        } else grounded = meeting(x, y + 1280);
        // VM Step selects sprite, then writes scale only for nonzero xsp,
        // before the visibility-gated xx/yy integration and x/y setters.
        const int sprite=turtle ? (!grounded ? 187 : walking ? 188 : 186) : grounded ? 163 : 164;
        const int scale=(vx>0)-(vx<0);
        if(!presentation(sprite,scale))return false;
        if (scale != 0) facing = scale;
        if (visible) {
            if (!turtle || walking) x += vx;
            y += vy;
            publishedX = static_cast<int>(FixedCollisionMath::round(x) / 1280);
            publishedY = static_cast<int>(FixedCollisionMath::round(y) / 1280);
        }
        return true;
    }
};
}
#endif
