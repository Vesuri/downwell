#include "core/MovingFx.h"
using N=downwell::MotionBinary64;using F=downwell::RunnerImageValue;
#ifdef DOWNWELL_FREESTANDING
#include "core/StoredMovingFxRegistration.h"
#include "core/MovingFxInstances.h"
bool nativeMovingFxOwner(downwell::MovingFxInstances<2>& owner,downwell::InstanceHandle key,bool allowed){
    return owner.advanceAnimation(key)&&owner.step(key,[&](const auto&,bool& value){value=allowed;return true;});
}
bool nativeMovingFxBirth(downwell::StoredMovingFxRegistration<8,2>& owner,N x,N y,downwell::InstanceHandle& child){
    return owner.create(x,y,child)&&owner.sprite(child,704,4)&&owner.imageSpeed(child,{0x3fb99999u,0x9999999au})&&
        owner.imageAngle(child,N::fromInteger(90))&&owner.velocity(child,true,{0xbfe00000u,0});
}
bool nativeMovingFxLifecycle(downwell::StoredMovingFxRegistration<8,2>& owner,downwell::InstanceHandle key,bool stopped,int width){
    return owner.advanceAnimation(key)&&owner.step(key,[&](auto h,const auto&,bool& allowed){
        return owner.timeAllowed(h,stopped,width,allowed);
    });
}
struct Ops {bool timeStopBound(bool&);bool imageSpeed(F,F&);bool position(bool,F,F&);};
bool nativeMovingFx(downwell::MovingFx& state,Ops& ops){return state.step(ops);}
#else
#include <cassert>
#include <vector>
#include <cstdio>
int main(){
    for(unsigned failure=0;failure<=9;++failure){
        struct Spawn {
            using Handle=unsigned;
            unsigned failure,calls=0,caller=0;
            bool accept(){return ++calls!=failure;}
            bool create(int object,N,N,Handle& child){assert(object==220);child=123;return accept();}
            bool myFx(Handle child){assert(calls==1&&child==123);if(!accept())return false;caller=child;return true;}
            bool sprite(Handle child,int sprite){assert(child==caller&&sprite==704);return accept();}
            bool imageSpeed(Handle,N){return accept();}bool imageAngle(Handle,N){return accept();}
            bool lengthdir(bool vertical,N,N,N& value){assert(calls==(vertical?7u:5u));value=N::fromInteger(vertical?2:1);return accept();}
            bool velocity(Handle child,bool vertical,N value){assert(child==caller&&value.high==N::fromInteger(vertical?2:1).high);return accept();}
        } ops{failure};
        assert(downwell::MovingFxSpawnScript::run(ops,{},{},704,{},{},{})==!failure);
        assert(ops.calls==(failure?failure:9)&&ops.caller==((!failure||failure>2)?123u:0u));
    }
    for(bool moving:{false,true})for(unsigned failure=0;failure<=4;++failure){
        downwell::MovingFx state;F x,y;x.assignInteger(10);y.assignInteger(20);
        assert(state.create(x,y)&&!state.create(x,y));
        state.xsp=N::fromInteger(1);state.ysp=N::fromInteger(-2);
        state.imageSpeed={0x3e800000u};state.imageRate=N::fromInteger(-1);
        struct Ops {
            downwell::MovingFx& state;bool moving;unsigned failure,calls=0;
            std::vector<int> writes;
            bool accept(){return ++calls!=failure;}
            bool timeStopBound(bool& result){assert(state.imageRate.high==0x3fd00000u);result=moving;return accept();}
            bool imageSpeed(F value,F& accepted){assert(!moving&&value.bits==0);accepted=value;writes.push_back(0);return accept();}
            bool position(bool vertical,F value,F& accepted){
                F expected;expected.assignInteger(vertical?(moving?18:20):(moving?11:10));
                assert(value.bits==expected.bits);writes.push_back(vertical?1:2);accepted=value;return accept();
            }
        } ops{state,moving,failure,0,{}};
        const unsigned count=moving?3:4;
        const bool ok=state.step(ops);
        assert(ok==(!failure||failure>count)&&ops.calls==(failure&&failure<=count?failure:count));
        if(ok){assert(ops.writes.back()==2);assert(state.y.bits==(moving?0x41900000u:0x41a00000u));}
    }
    std::puts("Moving FX: synchronous setter order, failure prefixes, stop-bound capture and Y-before-X publication passed");
}
#endif
