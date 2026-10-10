#ifndef DOWNWELL_CORE_STEERING_ENEMY_BIRTH_STATE_H
#define DOWNWELL_CORE_STEERING_ENEMY_BIRTH_STATE_H
#include "RunnerImageValue.h"
#include "RandomBinary64.h"
#include "MultiplyBinary64.h"
namespace downwell {
// Shared storage for the two steering families (115 and 118), and their
// identical parentLife -> subparentEnemy Create prefix. Concrete events must
// still publish their own fields, RNG draws and builtin writes in VM order.
struct SteeringEnemyBirthState {
    using Real=MotionBinary64;
    RunnerImageValue x{},y{},imageSpeed{};
    Real direction{};
    Real xx{},yy{},xsp{},ysp{},velocity{},moveSpeed{},maxSpeed{},acceleration{},deceleration{};
    Real rotateSpeed{},angle16{},playerDirection{},imageRate{},takenImpact{},hitAngle{},hitDirection{};
    int health=0,money=0,cone=0,hitDamage=0;
    unsigned myFx=0; // Unassigned until emitMovingFx returns its child ID.
    bool alive=false,scriptActive=false,hit=false,stomped=false,hitStun=false,grounded=false;
    bool timefieldImmune=false,damageStun=false,inWater=false,bulletImmune=false,bulletThrough=false;
    bool initialized=false;
    static bool snap(RunnerImageValue input,RunnerImageValue& output){
        Real divided;std::int32_t grid;
        if(!multiplyBinary64(input.promoted(),{0x3fb00000u,0},divided)||!divided.roundPixel(grid)||
            grid < -134217728 || grid > 134217727)return false;
        return output.assignInteger(grid*16);
    }
protected:
    template<class Ops> bool inheritedCreate(RunnerImageValue px,RunnerImageValue py,Ops& ops){
        if(initialized)return false;
        x=px;y=py;
        alive=true;health=1;xsp={};ysp={};money=1;takenImpact={};hitAngle={};
        RunnerImageValue requested,accepted;
        if(!snap(x,requested)||!ops.position(false,requested,accepted))return false;
        x=accepted;
        if(!snap(y,requested)||!ops.position(true,requested,accepted))return false;
        y=accepted;
        timefieldImmune=damageStun=inWater=bulletImmune=bulletThrough=hit=stomped=hitStun=false;
        hitDirection={};hitDamage=0;grounded=false;scriptActive=false;
        return true;
    }
};
}
#endif
