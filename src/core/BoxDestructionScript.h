#ifndef DOWNWELL_CORE_BOX_DESTRUCTION_SCRIPT_H
#define DOWNWELL_CORE_BOX_DESTRUCTION_SCRIPT_H
#include "BreakableState.h"
namespace downwell {
// objBox_n Step. Child creation callbacks execute Create synchronously. A
// destroy request must retain this instance through the remaining event.
// This is the event body, not permission to retire a box in a bullet query.
struct BoxDestructionScript {
    struct Caller {BreakableState state;MotionBinary64 x,y;};
    // Read must resolve the same generation each time, including after the
    // destruction request. Child Create can change caller fields or compact
    // other storage; never retain a borrowed record pointer across callbacks.
    template<class Read,class Ops>
    static bool runRetained(Read read,Ops& ops,bool& succeeded){
        if(!succeeded)return false;
        auto check=[&](bool ok){if(!ok)succeeded=false;return ok;};
        Caller caller;
        auto refresh=[&](){return check(read(caller)&&caller.state.wallHp.valid()&&caller.x.valid()&&caller.y.valid());};
        if(!refresh())return false;
        if(caller.state.exhausted()){
            if(!check(ops.soundOverlap(caller.state.breakSound,50,0,1,"waterThings"))||!refresh())return false;
            auto smokeY=caller.y;
            if(!check(smokeY.add(MotionBinary64::fromInteger(4)))||
               !check(ops.smoke(caller.x,smokeY,1,0))||!refresh()||
               !check(ops.flash(caller.x,caller.y,1,0,2))||!refresh())return false;
            auto effectY=caller.y;
            if(!check(effectY.add(MotionBinary64::fromInteger(12)))||
               !check(ops.effect(caller.x,effectY,603,{0x3fe00000u,0},
                                MotionBinary64::fromInteger(270),-60000)))return false;
            if(ops.chainReaction()){
                typename Ops::Handle bullet;
                if(!refresh()||!check(ops.create(266,caller.x,caller.y,bullet))||!check(ops.myBullet(bullet))||
                   !check(ops.bulletDirection(bullet,90))||!check(ops.bulletDamage(bullet,20))||
                   !check(ops.sound(342,50,0,1)))return false;
            }
            if(ops.chainReaction()){
                const int dx[]={0,16,0,-16},dy[]={-16,0,16,0};
                for(unsigned i=0;i<4;++i){
                    if(!refresh())return false;
                    auto qx=caller.x,qy=caller.y;typename Ops::Handle target;bool truth=false;
                    if(!check(qx.add(MotionBinary64::fromInteger(dx[i]))&&
                              qy.add(MotionBinary64::fromInteger(dy[i])))||
                       !check(ops.place(92,qx,qy,target,truth))||!check(ops.target(target)))return false;
                    if(truth){
                        if(ops.chainReaction()&&!check(ops.targetAlarm(target,0,5)))return false;
                        if(!refresh())return false;
                        if(caller.state.autoTile&&!check(ops.targetChecked(target,false)))return false;
                    }
                }
            }
            if(!check(ops.destroySelf())||!refresh())return false;
            typename Ops::Handle debris;
            if(!check(ops.create(370,caller.x,caller.y,debris)))return false;
            if(!ops.lowSpec()){
                if(!refresh()||!check(ops.create(370,caller.x,caller.y,debris)))return false;
            }
            if(ops.area()!=4){
                if(!refresh())return false;
                if(caller.state.haveGem==1&&!check(ops.currency(3)))return false;
                if(!refresh())return false;
                if(caller.state.haveGem==2&&!check(ops.currency(10)))return false;
            }
        }
        return check(ops.alarmStop(0));
    }
    // Fixed-caller convenience for scalar fixtures. Live worlds use the
    // retained reader above to resolve their slot/generation before every read.
    template<class Ops>
    static bool run(const BreakableState& state,MotionBinary64 x,MotionBinary64 y,
                    Ops& ops,bool& succeeded){
        return runRetained([&](Caller& caller){caller={state,x,y};return true;},ops,succeeded);
    }
};
}
#endif
