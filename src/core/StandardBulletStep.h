#ifndef DOWNWELL_CORE_STANDARD_BULLET_STEP_H
#define DOWNWELL_CORE_STANDARD_BULLET_STEP_H

#include "EnemyContact.h"
#include "ProjectileSegment.h"

namespace downwell {
// scrBulletOov uses strict comparisons to the published view position.
// Default camMain publishes rounded pixels; callers must not substitute its
// private easing accumulator. Q12 also supports explicit synthetic fixtures.
inline bool standardBulletOutsideView(long yQ12, long viewTopQ12,
                                     int viewHeight = 284) {
    return yQ12 > viewTopQ12 + (viewHeight + 32L) * 4096L ||
           yQ12 < viewTopQ12 - 100L * 4096L;
}

inline bool fractionalBulletOutsideView(MachineGunTrajectory::Coordinate y,
                                       int originY, long viewTopQ12, int height = 284) {
    y.pixel += originY;
    auto compare = [&](long boundary) {
        long pixel = boundary / 4096, fraction = boundary % 4096;
        if (fraction < 0) { --pixel; fraction += 4096; }
        if (y.pixel != pixel) return y.pixel < pixel ? -1 : 1;
        const auto fixed = static_cast<std::uint32_t>(fraction) << 8;
        return y.fraction == fixed ? 0 : y.fraction < fixed ? -1 : 1;
    };
    return compare(viewTopQ12 + (height + 32L) * 4096) > 0 ||
           compare(viewTopQ12 - 100L * 4096) < 0;
}

// The adapters retain their current enemy-first impact scheduling. This
// shares the source's first-Step initialization, full segment query and
// retained fractional movement; it is not a full VM collision-event scheduler.
// Healthy observes terminal callback failure, separately from a consumed shot.
// On failure leave alive untouched and retain only work already completed.
template<class Random,class Enemy,class Wall,class Healthy,class PublishInitialization>
bool stepMachineGunBulletChecked(MachineGunTrajectory& motion,Random& random,
    Enemy enemy,Wall wall,bool& alive,Healthy healthy,PublishInitialization publishInitialization) {
    if(!healthy())return false;
    if(!motion.initialized){
        motion.initialize(random);
        // Both live worlds publish the first-Step angle here, before any
        // query. Keep this boundary shared when replacing the motion owner.
        if(!publishInitialization()||!healthy())return false;
    }
    const ProjectileSegment path(motion);
    bool consumed=enemy(path);
    if(!healthy())return false;
    if(!consumed){consumed=wall(path);if(!healthy())return false;}
    motion.advance();
    alive=!consumed;return true;
}
template<class Random,class Enemy,class Wall,class Healthy>
bool stepMachineGunBulletChecked(MachineGunTrajectory& motion,Random& random,
    Enemy enemy,Wall wall,bool& alive,Healthy healthy) {
    return stepMachineGunBulletChecked(motion,random,enemy,wall,alive,healthy,[]{return true;});
}
template<class Random,class Enemy,class Wall>
bool stepMachineGunBullet(MachineGunTrajectory& motion,Random& random,Enemy enemy,Wall wall){
    bool alive=false;
    stepMachineGunBulletChecked(motion,random,enemy,wall,alive,[]{return true;});
    return alive;
}

// Complete implemented projectile update. Storage adapters supply collision
// queries and their world origin. Animation runs separately before Begin Step,
// so a shot created during that phase retains its Create image on this Step.
// Consumed shots still finish this update; owners retire/compact them afterward.
// Resource-backed worlds defer view retirement to their selected End Step;
// resource-less arithmetic fixtures keep the combined adapter by default.
template<class Bullet,class Random,class Enemy,class Wall,class Healthy,class PublishInitialization>
bool stepMachineGunProjectileChecked(Bullet& bullet,Random& random,Enemy enemy,Wall wall,
    bool& alive,Healthy healthy,int originY,long viewTopQ12,int viewHeight,PublishInitialization publishInitialization,
    bool retireInStep=true){
    if(!healthy())return false;
    bool moving=false;
    if(!stepMachineGunBulletChecked(bullet.trajectory,random,enemy,wall,moving,healthy,publishInitialization))return false;
    bullet.x = bullet.trajectory.x.pixel;
    bullet.y = bullet.trajectory.y.pixel;
    alive=moving&&(!retireInStep||!fractionalBulletOutsideView(bullet.trajectory.y,originY,viewTopQ12,viewHeight));
    return true;
}
template<class Bullet,class Random,class Enemy,class Wall,class Healthy>
bool stepMachineGunProjectileChecked(Bullet& bullet,Random& random,Enemy enemy,Wall wall,
    bool& alive,Healthy healthy,int originY,long viewTopQ12,int viewHeight=284){
    return stepMachineGunProjectileChecked(bullet,random,enemy,wall,alive,healthy,
        originY,viewTopQ12,viewHeight,[]{return true;});
}
template<class Bullet,class Random,class Enemy,class Wall>
bool stepMachineGunProjectile(Bullet& bullet,Random& random,Enemy enemy,Wall wall,
    int originY,long viewTopQ12,int viewHeight=284){
    bool alive=false;
    stepMachineGunProjectileChecked(bullet,random,enemy,wall,alive,[]{return true;},originY,viewTopQ12,viewHeight);
    return alive;
}

// Vertical usual-gun subset. scrBulCheckSolid suppresses wall processing
// when the complete motion segment intersects an enemy. Spread, other guns
// and exact enemy collision-event scheduling remain separate work. The
// callbacks apply the current adapters' hit effects and return whether the
// ordinary non-piercing bullet was consumed. This is not a general VM Step.
struct StandardBulletPath {
    int x, fromY, toY;
    bool intersects(int left, int right, int top, int bottom) const {
        return left < right && top < bottom &&
               x >= left && x < right && fromY < bottom && toY >= top;
    }
    bool intersects(const ContactActor& actor) const {
        const ContactBounds bounds = contactWorldBounds(actor);
        return intersects(bounds.left, bounds.right, bounds.top, bounds.bottom);
    }
};

template<class Enemy, class Wall>
bool stepStandardBullet(int x, int& y, Enemy enemy, Wall wall) {
    const StandardBulletPath path{x, y, y + 8};
    y = path.toY;
    if (enemy(path)) return false;
    return !wall(path);
}
} // namespace downwell
#endif
