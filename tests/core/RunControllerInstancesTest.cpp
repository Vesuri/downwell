#include "core/RunControllerInstances.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
using Store=RunControllerInstances<2>;
struct Owner {
    unsigned births=0,retirements=0;bool rejectBirth=false;
    bool birth(InstanceHandle,const Store::Record&){assert(false);return false;}
    bool birthAt(InstanceHandle,const Store::Record& r,RunnerImageValue x,RunnerImageValue y){
        assert(r.sourceId&&r.x.bits==x.bits&&r.y.bits==y.bits);++births;return !rejectBirth;
    }
    bool retire(InstanceHandle,const Store::Record&){++retirements;return true;}
    bool activation(InstanceHandle,const Store::Record&,bool){return true;}
    bool membership(InstanceHandle,const Store::Record&,bool){return true;}
};
int main(){
    {
        Owner owner;Store store(Store::Binding::bindPositionedLifetime(owner));InstanceHandle key;
        assert(store.create(100065,{}, {},[](InstanceHandle){return true;},key));
        int speed=60;
        assert(store.setAlarm(key,1,1)&&!store.advanceAlarm(key,1,speed)&&!store.valid()&&speed==60);
        assert(store.find(key)->alarms.get(1)==0&&store.clear()); // Unsupported event is never a successful no-op.
    }
    for(unsigned failure=0;failure<3;++failure){
        Owner owner;owner.rejectBirth=failure==1;Store store(Store::Binding::bindPositionedLifetime(owner));
        InstanceHandle a,b;unsigned initialized=0;
        const bool okay=store.create(100065,{0x3f000000u},{0x3fc00000u},[&](InstanceHandle h){
            assert(store.find(h)&&owner.births==1);++initialized;return failure!=2;
        },a);
        assert(okay==(failure==0)&&store.valid()==okay&&store.size()==1&&initialized==(failure==1?0u:1u));
        if(!okay){assert(!a.generation&&store.sourceHandle(100065).generation&&store.clear());continue;}
        assert(store.create(100066,{}, {},[&](InstanceHandle){++initialized;return true;},b));
        int speed=60;unsigned visits=0;
        assert(store.momentDelay(speed,[&](auto apply){++visits;return apply(b)&&apply(a);})&&visits==1&&speed==30);
        assert(store.find(a)->controllerSlow==30&&store.find(b)->controllerSlow==30);
        assert(store.activate(b,false)&&store.advanceAlarm(b,0,speed)&&store.find(b)->alarms.get(0)==1);
        assert(store.advanceAlarm(a,0,speed)&&speed==60&&store.find(a)->controllerSlow==0&&store.find(a)->alarms.get(0)==0);
        speed=35;assert(store.advanceAlarm(a,0,speed)&&speed==35&&store.find(a)->alarms.get(0)==-1);
        assert(store.activate(b,true)&&store.advanceAlarm(b,0,speed)&&speed==60);
        assert(initialized==2); // Requests and alarms never rerun global Create.
        const auto old=a;assert(store.clear()&&owner.retirements==2&&!store.find(old));
        assert(store.create(100065,{}, {},[](InstanceHandle){return true;},a)&&a.generation>old.generation);
        assert(!store.momentDelay(speed,[&](auto apply){return apply(old);})&&!store.valid()&&store.clear());
    }
    std::puts("Persistent controller ownership: positioned births, retained failures, ordered delay, inactive alarms and stale handles passed");
}
