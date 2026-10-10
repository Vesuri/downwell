#ifndef DOWNWELL_CORE_WHITE_JELLY_BIRTH_H
#define DOWNWELL_CORE_WHITE_JELLY_BIRTH_H
#include "RunnerImageValue.h"
#include "RandomBinary64.h"
#include "MultiplyBinary64.h"
namespace downwell {
// Synchronous parentLife -> subparentEnemy -> enmWhiteJelly Create.
// Owners supply builtin setters and original point_direction; no approximate
// heading or pre-sampled RNG may be substituted at this birth boundary.
struct WhiteJellyBirth {
    using Real=MotionBinary64;
    RunnerImageValue x{},y{},imageSpeed{};
    Real direction{}; // Read back from the owner's builtin setter; do not assume its storage type.
    Real xx{},yy{},xsp{},ysp{},velocity{},moveSpeed{},maxSpeed{},acceleration{},deceleration{};
    Real rotateSpeed{},angle16{},playerDirection{},imageRate{},takenImpact{},hitAngle{},hitDirection{};
    int health=0,money=0,cone=0,hitDamage=0;
    bool alive=false,scriptActive=false,hit=false,stomped=false,hitStun=false,grounded=false;
    bool timefieldImmune=false,damageStun=false,inWater=false,bulletImmune=false,bulletThrough=false;
    bool initialized=false;
    static bool snap(RunnerImageValue input,RunnerImageValue& output){
        Real divided;std::int32_t grid;
        if(!multiplyBinary64(input.promoted(),{0x3fb00000u,0},divided)||!divided.roundPixel(grid)||
            grid < -134217728 || grid > 134217727)return false;
        return output.assignInteger(grid*16);
    }
    template<class Random,class Ops> bool create(RunnerImageValue px,RunnerImageValue py,Random& random,Ops& ops){
        if(initialized)return false;
        x=px;y=py;
        // Inherited parentLife and the prefix before move_snap.
        alive=true;health=1;xsp={};ysp={};money=1;takenImpact={};hitAngle={};
        RunnerImageValue requested,accepted;
        if(!snap(x,requested)||!ops.position(false,requested,accepted))return false;
        x=accepted;
        if(!snap(y,requested)||!ops.position(true,requested,accepted))return false;
        y=accepted;
        timefieldImmune=damageStun=inWater=bulletImmune=bulletThrough=hit=stomped=hitStun=false;
        hitDirection={};hitDamage=0;grounded=false;scriptActive=false;
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
