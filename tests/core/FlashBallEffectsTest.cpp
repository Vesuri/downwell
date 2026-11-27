#include "core/FlashBallEffects.h"
#include "core/FlashBallInstances.h"
#include "core/GameMakerRandom.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
bool nativeBoxFlash(downwell::FlashBallEffects<32>& effects,downwell::Fixed16 x,
                    downwell::Fixed16 y,downwell::GameMakerRandom& random){
    return effects.boxBurst(x,y,random);
}
bool nativeRegisteredFlash(downwell::FlashBallInstances<32>& effects,std::uint32_t id,
                          downwell::MotionBinary64 x,downwell::MotionBinary64 y,
                          downwell::GameMakerRandom& random){
    downwell::InstanceHandle key;
    return effects.create(id,x,y,random,key)&&effects.advanceAnimation(key)&&effects.activate(key,false);
}
#else
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
static void check(bool ok){if(!ok)std::abort();}
static downwell::MotionBinary64 number(double x){
    std::uint64_t bits;std::memcpy(&bits,&x,8);return {std::uint32_t(bits>>32),std::uint32_t(bits)};
}
static float scalar(downwell::RunnerImageValue x){float value;std::memcpy(&value,&x.bits,4);return value;}
// Exact for the Q16 values used here.
static downwell::Fixed16 q(double x){return downwell::Fixed16::raw(std::int32_t(std::lround(x*65536)));}
// The registered owner's binary64 Create (narrow, jitter, narrow).
template<class Random> static bool flashOracle(downwell::FlashBallEffect& child,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,Random& random){
    using downwell::MotionBinary64;
    if(!child.x.assign(x)||!child.y.assign(y))return false;
    auto jx=child.x.promoted();
    if(!jx.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2))||!child.x.assign(jx))return false;
    auto jy=child.y.promoted();
    return jy.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2))&&child.y.assign(jy);
}
struct Reference {double x,y;float index=1;};
using Registered=downwell::FlashBallInstances<2>;
struct Owner {
    using Record=Registered::Record;
    unsigned calls=0,failAt=0,births=0,images=0,moves=0,deaths=0,retirements=0;
    bool event(){return ++calls!=failAt;}
    bool birth(downwell::InstanceHandle,const Record&){check(false);return false;}
    bool birthAt(downwell::InstanceHandle key,const Record& r,downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        check(key.generation==r.lifetimeId&&r.sourceId>=1000&&r.state.imageIndex.bits==0&&
              r.state.imageSpeed.bits==0x3f800000u&&r.state.vrange==0&&x.bits==r.x.bits&&y.bits==r.y.bits);
        ++births;return event();
    }
    bool move(downwell::InstanceHandle,const Record&,downwell::RunnerImageValue,downwell::RunnerImageValue){++moves;return event();}
    bool imageIndex(downwell::InstanceHandle,const Record&,downwell::RunnerImageValue){++images;return event();}
    bool markDestroyed(downwell::InstanceHandle,const Record& r){check(r.state.destroyRequested);++deaths;return event();}
    bool retire(downwell::InstanceHandle,const Record&){++retirements;return event();}
    bool activation(downwell::InstanceHandle,const Record&,bool){return event();}
    bool membership(downwell::InstanceHandle,const Record&,bool){check(false);return false;}
    bool sprite(downwell::InstanceHandle,const Record&,int){check(false);return false;}
    bool mask(downwell::InstanceHandle,const Record&,int){check(false);return false;}
    bool transform(downwell::InstanceHandle,const Record&,downwell::RunnerTransformField,
                   downwell::RunnerImageValue,downwell::RunnerImageValue,std::uint32_t){check(false);return false;}
};
static void registeredTests(){
    for(unsigned seed=0;seed<1024;++seed){
        Owner owner;Registered effects(Registered::Binding::bindPositionedActor(owner));
        downwell::GameMakerRandom random(seed),oracle(seed);downwell::FlashBallEffect reference;
        const auto x=number(seed%2?16777217.25:-64.125),y=number(-192.375);
        downwell::InstanceHandle key;
        check(effects.create(1000,x,y,random,key)&&flashOracle(reference,x,y,oracle));
        const auto* r=effects.find(key);check(r&&r->x.bits==reference.x.bits&&r->y.bits==reference.y.bits);
        check(owner.births==1&&owner.images==1&&owner.moves==2&&effects.size()==1);
        check(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)check(random.stateWord(i)==oracle.stateWord(i));
        check(effects.activate(key,false));
        const auto calls=owner.calls;check(effects.advanceAnimation(key)&&owner.calls==calls);
        check(effects.activate(key,true));
        for(unsigned frame=0;frame<7;++frame){
            check(effects.advanceAnimation(key)&&reference.advanceImage());
            check(r->state.imageIndex.bits==reference.imageIndex.bits&&r->destroyRequested==reference.destroyRequested);
        }
        check(owner.deaths==1&&effects.size()==1); // Destruction is not retirement.
        check(effects.advanceAnimation(key)&&owner.deaths==1);
        check(effects.retire(key)&&!effects.find(key)&&effects.size()==0);
        downwell::InstanceHandle newer;check(effects.create(1001,x,y,random,newer));
        check(newer.slot==key.slot&&newer.generation!=key.generation&&!effects.find(key));
        check(effects.clear()&&effects.size()==0&&owner.retirements==2);
    }
    // Birth, image-index, X setter and Y setter failures retain exactly the
    // already-published prefix and consume no later RNG draws.
    for(unsigned fail=1;fail<=4;++fail){
        Owner owner;owner.failAt=fail;Registered effects(Registered::Binding::bindPositionedActor(owner));
        bool worldGood=true;effects.observeFailure(worldGood);
        downwell::GameMakerRandom random(19);downwell::InstanceHandle key;
        check(!effects.create(1000,number(10),number(20),random,key)&&!key.generation);
        check(!effects.valid()&&!worldGood&&effects.size()==1&&owner.calls==fail);
        check(random.drawCount()==(fail<=2?0:fail==3?2:4));
        const auto retained=effects.sourceHandle(1000);check(retained.generation&&effects.find(retained));
        const auto draws=random.drawCount();
        check(!effects.create(1001,number(0),number(0),random,key)&&random.drawCount()==draws);
    }
    for(bool destructionFailure:{false,true}){
        Owner owner;Registered effects(Registered::Binding::bindPositionedActor(owner));
        downwell::GameMakerRandom random(42);downwell::InstanceHandle key;
        check(effects.create(1000,number(0),number(0),random,key));
        for(unsigned i=0;i<6;++i)check(effects.advanceAnimation(key));
        owner.failAt=owner.calls+(destructionFailure?2:1);
        check(!effects.advanceAnimation(key)&&!effects.valid()&&effects.size()==1);
        const auto* r=effects.find(key);
        check(r&&!r->destroyRequested&&r->state.destroyRequested==destructionFailure);
        check(owner.deaths==unsigned(destructionFailure));
    }
    Owner owner;Registered effects(Registered::Binding::bindPositionedActor(owner));
    downwell::GameMakerRandom random(21);downwell::InstanceHandle first,second,failed;
    check(effects.create(1000,number(0),number(0),random,first));
    check(effects.create(1001,number(0),number(0),random,second));
    const auto draws=random.drawCount();const auto calls=owner.calls;
    check(!effects.create(1002,number(0),number(0),random,failed)&&!failed.generation);
    check(random.drawCount()==draws&&owner.calls==calls&&effects.size()==2);
}
int main(){
    registeredTests();
    unsigned births=0;
    for(unsigned seed=0;seed<1024;++seed)for(bool box:{false,true}){
        downwell::GameMakerRandom random(seed),oracle(seed);
        downwell::FlashBallEffects<16> effects;
        const auto before=effects.beginAnimation();
        // The compact owner is Q16 (W3.20): positions inside its range.
        const double x=seed%2?16000.25:-64.125,y=seed%3?192.375:-16000.25;
        check(box?effects.boxBurst(q(x),q(y),random):effects.skeletonBurst(q(x),q(y),random));
        std::vector<Reference> expected;
        const unsigned count=(box?1:4)+unsigned(oracle.irandom(box?0:4));
        for(unsigned i=0;i<count;++i){
            const int radius=box?2:10;
            const double py=y+(oracle.random(radius*2)-radius),px=x+(oracle.random(radius*2)-radius);
            const double finalX=px+int(oracle.irandom(4))-2,finalY=py+int(oracle.irandom(4))-2;
            expected.push_back({finalX,finalY,1});
        }
        check(random.drawCount()==oracle.drawCount());
        for(unsigned i=0;i<16;++i)check(random.stateWord(i)==oracle.stateWord(i));
        check(effects.advanceAnimation(before)); // Births do not age in an older phase.
        for(unsigned frame=0;frame<=7;++frame){
            for(unsigned i=0;i<count;++i){
                const auto& slot=effects.slot(i);
                check(bool(slot.handle)==(frame<7));
                if(frame<7){
                    // The Q16 sample is within half a step and narrowed once.
                    const auto near=[](float actual,double wanted){
                        return std::fabs(actual-wanted)<=1/131072.+std::fabs(wanted)*0x1p-24;};
                    check(near(scalar(slot.effect.x),expected[i].x) && near(scalar(slot.effect.y),expected[i].y));
                    check(scalar(slot.effect.imageIndex)==expected[i].index);
                    check(slot.effect.drawFrame()==unsigned(expected[i].index));
                }
            }
            for(unsigned i=count;i<16;++i)check(!effects.slot(i).handle);
            check(effects.advanceAnimation(effects.beginAnimation()));
            for(auto& ref:expected)ref.index=float(ref.index+0.3f);
        }
        const auto draws=random.drawCount();
        check(effects.create(q(0),q(0),random)>count); // Stable handle on slot reuse.
        check(random.drawCount()==draws+4);
        births+=count;
    }
    downwell::FlashBallEffects<1> full;
    downwell::GameMakerRandom random(9);
    check(full.create(q(0),q(0),random)==1);
    const auto draws=random.drawCount();
    check(!full.create(q(0),q(0),random) && draws==random.drawCount());
    unsigned visited=0;
    full.visitNewest([&](const downwell::FlashBallEffect&){++visited;});check(visited==1);
    full.clear();visited=0;
    full.visitNewest([&](const downwell::FlashBallEffect&){++visited;});check(visited==0);
    downwell::FlashBallEffects<3> ordered;
    check(ordered.create(q(10),q(0),random));
    const auto older=ordered.beginAnimation();
    for(unsigned i=0;i<6;++i)check(ordered.advanceAnimation(older));
    check(ordered.create(q(100),q(0),random));
    check(ordered.advanceAnimation(older)); // Retire older slot only.
    check(ordered.create(q(200),q(0),random)); // Reuses lower slot.
    std::vector<int> order;
    ordered.visitNewest([&](const downwell::FlashBallEffect& effect){order.push_back(effect.pixelX());});
    check(order.size()==2 && order[0]>=198 && order[1]<=102);
    std::printf("Flash effects: %u skeleton/box births across 1024 seeds, seven-update retirement and slot reuse passed\n",births);
}
#endif
