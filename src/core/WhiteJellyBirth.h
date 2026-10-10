#ifndef DOWNWELL_CORE_WHITE_JELLY_BIRTH_H
#define DOWNWELL_CORE_WHITE_JELLY_BIRTH_H
#include "SteeringEnemyBirthState.h"
namespace downwell {
// Synchronous parentLife -> subparentEnemy -> enmWhiteJelly Create.
// Owners supply builtin setters and original point_direction; no approximate
// heading or pre-sampled RNG may be substituted at this birth boundary.
struct WhiteJellyBirth : SteeringEnemyBirthState {
    template<class Random,class Ops> bool create(RunnerImageValue px,RunnerImageValue py,Random& random,Ops& ops){
        if(!inheritedCreate(px,py,ops))return false;
        RunnerImageValue requested,accepted;
        // The concrete event consumes its random_range before xx/yy and the
        // steering globals are read. velocity is retained even if Step never
        // uses it: removing the draw changes all following room RNG.
        bulletThrough=false;health=25;money=6;xsp={};ysp={};
        if(!randomBinary64(random.nextU32(),{0x3fe00000u,0},velocity)||
            !velocity.add(Real::fromInteger(1)))return false;
        xx=x.promoted();yy=y.promoted();hitStun=false;
        moveSpeed=maxSpeed={0x3fc99999u,0x9999999au};
        acceleration={0x3fa99999u,0x9999999au};deceleration={0x3f50624du,0xd2f1a9fcu};
        cone=10;rotateSpeed={0x3ff80000u,0};angle16={0x40368000u,0};
        Real targetY,targetX,angle,publishedDirection;
        // VM pushes ply before plx when evaluating point_direction arguments.
        if(!ops.playerY(targetY)||!ops.playerX(targetX)||
            !ops.pointDirection(x.promoted(),y.promoted(),targetX,targetY,angle)||
            !ops.direction(angle,publishedDirection))return false;
        direction=publishedDirection;playerDirection=direction;scriptActive=true;
        imageRate={0x3fc99999u,0x9999999au};
        if(!requested.assign(imageRate)||!ops.imageSpeed(requested,accepted))return false;
        imageSpeed=accepted;initialized=true;return true;
    }
};
}
#endif
