#ifndef DOWNWELL_CORE_RED_BALL_BIRTH_H
#define DOWNWELL_CORE_RED_BALL_BIRTH_H
#include "SteeringEnemyBirthState.h"
namespace downwell {
// Object 118 Create. Complete synchronously before spawner deactivation.
// Lifetime/ID ownership and subsequent events belong to the enclosing owner.
struct RedBallBirth : SteeringEnemyBirthState {
    Real emitTimer{};
    template<class Random,class Ops> bool create(RunnerImageValue px,RunnerImageValue py,Random& random,Ops& ops){
        if(!inheritedCreate(px,py,ops))return false;
        health=30;money=20;xsp={};ysp={};
        if(!randomBinary64(random.nextU32(),{0x3fe00000u,0},velocity)||
            !velocity.add(Real::fromInteger(1)))return false;
        xx=x.promoted();yy=y.promoted();hitStun=false;
        moveSpeed={0x3fb99999u,0x9999999au};maxSpeed={0x3ff33333u,0x33333333u};
        acceleration={0x3fa99999u,0x9999999au};deceleration={0x3fd00000u,0};
        cone=90;rotateSpeed=Real::fromInteger(2);angle16={0x40368000u,0};
        Real targetY,targetX,angle,acceptedDirection;
        if(!ops.playerY(targetY)||!ops.playerX(targetX)||
            !ops.pointDirection(x.promoted(),y.promoted(),targetX,targetY,angle)||
            !ops.direction(angle,acceptedDirection))return false;
        direction=acceptedDirection;playerDirection=direction;scriptActive=true;
        emitTimer=Real::fromInteger(10);
        if(!ops.alarm(0,10)||!ops.alarm(2,30))return false;
        imageRate={0x3fc99999u,0x9999999au};
        RunnerImageValue requested,accepted;
        if(!requested.assign(imageRate)||!ops.imageSpeed(requested,accepted))return false;
        imageSpeed=accepted;initialized=true;return true;
    }
};
}
#endif
