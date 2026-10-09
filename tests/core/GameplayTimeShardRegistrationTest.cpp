#include "core/GameplayTimeShardRegistration.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
// The freestanding compatibility header elides assertions. Retain these
// expressions so the compile-only probe actually instantiates phase dispatch.
#undef assert
#define assert(expression) do { if(!(expression)) __builtin_trap(); } while(0)
#endif
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include <cstdio>
#endif
using namespace downwell;
struct Owner {
    unsigned calls=0,fail=0,births=0,marks=0,retires=0,populationCalls=0;
    bool embedded=false;
    bool positionAfterDestroy=false;
    RunnerImageValue lastX{},lastY{};
    std::uint32_t next=210001;
    bool call(){return ++calls!=fail;}
    template<class R> bool birth(InstanceHandle h,const R& r,std::uint32_t& id){
        assert(h.generation==r.lifetimeId&&r.objectId==238&&r.alarms.get(2)==-1);
        ++births;id=next++;return call();
    }
    template<class R> bool imageIndex(InstanceHandle h,const R& r,RunnerImageValue){assert(h.generation==r.lifetimeId);return call();}
    template<class R> bool position(InstanceHandle h,const R& r,RunnerImageValue x,RunnerImageValue y){
        assert(h.generation==r.lifetimeId);positionAfterDestroy=r.destroyRequested;lastX=x;lastY=y;return call();
    }
    template<class R> bool markDestroyed(InstanceHandle,const R& r){assert(!r.destroyRequested);++marks;return call();}
    template<class R> bool retire(InstanceHandle,const R&){++retires;return call();}
    template<class R> bool animationMember(InstanceHandle,const R& r,bool& member){member=r.active&&!r.destroyRequested;return call();}
    template<class R> bool activation(InstanceHandle,const R&,bool){return call();}
    template<class R> bool meeting(InstanceHandle,const R&,MotionBinary64,MotionBinary64,bool& hit){hit=embedded;return call();}
    bool population(unsigned& count){++populationCalls;count=births-marks;return call();}
};
static void lifecycle(){
    Owner owner;GameplayTimeShardRegistration<2,Owner> registry(owner);TimeShardEffects<2> effects;
    GameMakerRandom random(2);assert(effects.bindPhases(registry));
    const auto local=effects.create(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(192),false,random);
    assert(local==1&&effects.sourceId(local)==210001&&random.drawCount()==6);
    auto key=registry.handle(local);auto* record=registry.schedulerRecord(key);assert(record);
    auto oracle=*effects.get(local);auto expectedRandom=random;
    auto outside=[](const auto&,bool& value){value=false;return true;};
    for(unsigned tick=0;tick<5;++tick){
        assert(oracle.advanceImage()&&registry.dispatchAnimation(key,effects));
        for(unsigned alarm=0;alarm<3;++alarm){
            oracle.advanceAlarm(alarm);
            if(record->alarms.advanceOne(alarm))assert(registry.dispatchAlarm(key,effects,alarm));
        }
        assert(oracle.step(true,expectedRandom,[](auto,auto,bool& hit){hit=false;return true;},[]{return 1;},outside));
        assert(registry.dispatchStep(key,effects,true,random,outside));
        const auto* actual=effects.get(local);assert(actual);
        assert(record->x.bits==oracle.x.bits&&record->y.bits==oracle.y.bits&&
            record->imageIndex.bits==oracle.imageIndex.bits&&record->imageSpeed.bits==oracle.imageSpeed.bits);
        assert(actual->xx.high==oracle.xx.high&&actual->xx.low==oracle.xx.low&&
            actual->yy.high==oracle.yy.high&&actual->yy.low==oracle.yy.low);
        assert(record->alarms.get(0)==oracle.alarm0&&record->alarms.get(1)==oracle.alarm1&&record->alarms.get(2)==oracle.alarm2);
    }
    assert(registry.activation(key,false));const auto calls=owner.calls;
    assert(registry.dispatchStep(key,effects,false,random,outside)&&owner.calls==calls);
    assert(registry.activation(key,true));
    owner.embedded=true;
    assert(registry.dispatchStep(key,effects,false,random,outside));
    assert(record->destroyRequested&&owner.marks==1&&registry.size()==1&&!effects.size()&&!owner.retires);
    assert(owner.positionAfterDestroy&&record->x.bits==owner.lastX.bits&&record->y.bits==owner.lastY.bits);
    assert(registry.retireMarked(key,effects)&&!registry.find(key)&&!registry.size());
    const auto next=effects.create({}, {},false,random);const auto newKey=registry.handle(next);
    assert(next==2&&newKey.slot==key.slot&&newKey.generation!=key.generation&&!registry.find(key)&&effects.sourceId(next)==210002);
    assert(effects.clear()&&!registry.size());
}
static void failures(){
    // Every world callback in Create/Step/cleanup is a checked prefix.
    unsigned total=0;
    for(unsigned failure=0;failure<=total;++failure){
        Owner owner;owner.fail=failure;owner.embedded=true;
        GameplayTimeShardRegistration<2,Owner> registry(owner);TimeShardEffects<2> effects;
        GameMakerRandom random(2);assert(effects.bindPhases(registry));
        auto local=effects.create({}, {},false,random);
        bool ok=local!=0;
        if(ok)ok=registry.dispatchStep(registry.handle(local),effects,false,random,[](const auto&,bool& value){value=false;return true;});
        if(ok)ok=registry.retireMarked(registry.handle(local),effects);
        if(!failure){assert(ok);total=owner.calls;continue;}
        assert(!ok&&owner.calls==failure&&!effects.succeeded());
        const auto draws=random.drawCount();const auto calls=owner.calls;
        assert(!effects.create({}, {},false,random)&&random.drawCount()==draws&&owner.calls==calls);
        assert(failure==1?random.drawCount()==0:random.drawCount()>=6);
        assert(registry.size()==1); // Failed registration/retirement is not recycled.
    }
}
static void alarmsAndContact(){
    Owner owner;GameplayTimeShardRegistration<2,Owner> registry(owner);TimeShardEffects<2> effects;
    GameMakerRandom random(2);assert(effects.bindPhases(registry));
    const auto local=effects.create({}, {},false,random);const auto key=registry.handle(local);
    auto* r=registry.schedulerRecord(key);assert(r);
    r->alarms.set(0,123);r->alarms.set(1,77);r->alarms.set(2,1);
    assert(r->alarms.advanceOne(2)&&registry.dispatchAlarm(key,effects,2));
    assert(!effects.get(local)->noTouch&&r->alarms.get(0)==123&&r->alarms.get(1)==77);
    const auto draws=random.drawCount();
    assert(registry.dispatchContact(key,effects,{}, {},random)&&random.drawCount()==draws+2&&r->alarms.get(2)==5);
    assert(registry.dispatchContact(key,effects,{}, {},random)&&random.drawCount()==draws+2);
    r->alarms.set(0,1);
    assert(r->alarms.advanceOne(0)&&registry.dispatchAlarm(key,effects,0)&&r->destroyRequested&&registry.size()==1);
    assert(registry.retireMarked(key,effects)&&!registry.size());
    assert(!registry.dispatchAnimation(key,effects)&&!registry.valid());
}
bool nativeTimeShardRegistrationProbe(){lifecycle();failures();alarmsAndContact();return true;}
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
int main(){assert(nativeTimeShardRegistrationProbe());std::puts("Native shard registration: phase publication, stopped motion, retained destruction, slot reuse and checked failure prefixes passed");}
#endif
