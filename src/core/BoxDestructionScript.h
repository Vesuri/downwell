#ifndef DOWNWELL_CORE_BOX_DESTRUCTION_SCRIPT_H
#define DOWNWELL_CORE_BOX_DESTRUCTION_SCRIPT_H
#include "BreakableState.h"
namespace downwell {
// objBox_n Step. Child creation callbacks execute Create synchronously. A
// destroy request must retain this instance through the remaining event.
// This is the event body, not permission to retire a box in a bullet query.
struct BoxDestructionScript {
    template<class Ops>
    static bool run(const BreakableState& state,MotionBinary64 x,MotionBinary64 y,
                    Ops& ops,bool& succeeded){
        if(!succeeded)return false;
        auto check=[&](bool ok){if(!ok)succeeded=false;return ok;};
        if(!check(state.wallHp.valid()&&x.valid()&&y.valid()))return false;
        if(state.wallHp.zero()||(state.wallHp.high&0x80000000u)){
            if(!check(ops.soundOverlap(state.breakSound,50,0,1,"waterThings")))return false;
            auto smokeY=y;
            if(!check(smokeY.add(MotionBinary64::fromInteger(4)))||
               !check(ops.smoke(x,smokeY,1,0))||!check(ops.flash(x,y,1,0,2)))return false;
            auto effectY=y;
            if(!check(effectY.add(MotionBinary64::fromInteger(12)))||
               !check(ops.effect(x,effectY,603,{0x3fe00000u,0},
                                MotionBinary64::fromInteger(270),-60000)))return false;
            // The VM reads this global again after synchronous child Create.
            if(ops.chainReaction()){
                typename Ops::Handle bullet;
                if(!check(ops.create(266,x,y,bullet))||!check(ops.myBullet(bullet))||
                   !check(ops.bulletDirection(bullet,90))||!check(ops.bulletDamage(bullet,20))||
                   !check(ops.sound(342,50,0,1)))return false;
            }
            if(ops.chainReaction()){
                const int dx[]={0,16,0,-16},dy[]={-16,0,16,0};
                for(unsigned i=0;i<4;++i){
                    auto qx=x,qy=y;typename Ops::Handle target;bool truth=false;
                    if(!check(qx.add(MotionBinary64::fromInteger(dx[i]))&&
                              qy.add(MotionBinary64::fromInteger(dy[i])))||
                       !check(ops.place(92,qx,qy,target,truth))||!check(ops.target(target)))return false;
                    if(truth){
                        if(ops.chainReaction()&&!check(ops.targetAlarm(target,0,5)))return false;
                        if(state.autoTile&&!check(ops.targetChecked(target,false)))return false;
                    }
                }
            }
            if(!check(ops.destroySelf()))return false;
            typename Ops::Handle debris;
            if(!check(ops.create(370,x,y,debris)))return false;
            if(!ops.lowSpec()&&!check(ops.create(370,x,y,debris)))return false;
            if(ops.area()!=4){
                if(state.haveGem==1&&!check(ops.currency(3)))return false;
                if(state.haveGem==2&&!check(ops.currency(10)))return false;
            }
        }
        // Even the intact-box branch calls alarmStop(0).
        return check(ops.alarmStop(0));
    }
};
}
#endif
