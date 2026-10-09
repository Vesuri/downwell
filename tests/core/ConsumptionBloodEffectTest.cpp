#include "core/ConsumptionBloodEffect.h"
#include "core/ConsumptionBloodInstances.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 real(double value){std::uint64_t b;std::memcpy(&b,&value,8);return {unsigned(b>>32),unsigned(b)};}
static void equal(MotionBinary64 value,double expected){const auto b=real(expected);assert(value.high==b.high&&value.low==b.low);}
struct Environment {
    ConsumptionBloodEffect& effect;unsigned calls=0,reject=0,positions=0;double grav=.2,fall=.125;bool mutateY=false;
    bool accept(){return ++calls!=reject;}
    bool gravity(MotionBinary64 v){equal(v,.2);grav=.2;return accept();}
    MotionBinary64 gravity(){return real(grav);}
    MotionBinary64 fallSpeed(){return real(fall);}
    bool imageSpeed(RunnerImageValue v){assert(v.bits==0x3e99999au);return accept();}
    bool imageIndex(RunnerImageValue v){assert(v.bits==effect.imageIndex.bits);return accept();}
    bool position(RunnerImageValue x,RunnerImageValue y){
        assert(x.bits==effect.x.bits&&y.bits==effect.y.bits);++positions;
        if(mutateY&&positions==1)assert(effect.y.assignInteger(300));
        return accept();
    }
    bool destroy(){assert(effect.destroyRequested);return accept();}
};
struct NativeAudit {
    using Record=ConsumptionBloodInstances<1>::Record;
    unsigned calls=0,reject=0,moves=0,marks=0;
    bool accept(){return ++calls!=reject;}
    bool birth(InstanceHandle,const Record&){return false;}
    bool birthAt(InstanceHandle,const Record& r,RunnerImageValue x,RunnerImageValue y){
        assert(r.sourceId==105187&&r.x.bits==x.bits&&r.y.bits==y.bits&&r.state.alarm0==-1);return accept();
    }
    bool imageIndex(InstanceHandle,const Record& r,RunnerImageValue v){assert(r.state.imageIndex.bits==v.bits);return accept();}
    bool move(InstanceHandle,const Record& r,RunnerImageValue x,RunnerImageValue y){
        assert(r.state.x.bits==x.bits&&r.state.y.bits==y.bits);++moves;return accept();
    }
    bool markDestroyed(InstanceHandle,const Record& r){assert(r.state.destroyRequested&&!r.destroyRequested);++marks;return accept();}
    bool retire(InstanceHandle,const Record&){return true;}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool sprite(InstanceHandle,const Record&,int){return false;}
    bool mask(InstanceHandle,const Record&,int){return false;}
    bool transform(InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return false;}
};
struct Globals {
    MotionBinary64 grav{};
    bool gravity(MotionBinary64 v){grav=v;return true;}
    MotionBinary64 gravity(){return grav;}MotionBinary64 fallSpeed(){return real(.125);}
};
int main(){
    for(unsigned seed=0;seed<64;++seed){
        using Owner=ConsumptionBloodInstances<1>;NativeAudit audit;Owner owner(Owner::Binding::bindPositionedActor(audit));
        Globals globals;GameMakerRandom random(seed),referenceRandom(seed);InstanceHandle key;
        ConsumptionBloodEffect reference;Environment env{reference};
        assert(owner.create(105187,real(240.25),real(192.5),random,globals,key)&&key.generation!=105187&&
            reference.create(real(240.25),real(192.5),referenceRandom,env));
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==referenceRandom.stateWord(i));
        assert(owner.activate(key,false)&&owner.advanceAnimation(key)&&owner.advanceAlarm(key,0)&&owner.step(key,globals)&&
            owner.find(key)->state.alarm0==120&&owner.find(key)->state.timeline.bits==0);
        assert(owner.activate(key,true));
        for(unsigned tick=0;tick<120;++tick){
            assert(owner.advanceAnimation(key)&&reference.advanceImage(env)&&owner.advanceAlarm(key,0));
            if(tick==119)assert(reference.kill(env));
            assert(owner.step(key,globals)&&reference.step(env));
            const auto* r=owner.find(key);
            assert(r&&r->x.bits==reference.x.bits&&r->y.bits==reference.y.bits&&r->state.imageIndex.bits==reference.imageIndex.bits&&
                r->state.xsp.high==reference.xsp.high&&r->state.xsp.low==reference.xsp.low&&
                r->state.ysp.high==reference.ysp.high&&r->state.ysp.low==reference.ysp.low&&r->destroyRequested==reference.destroyRequested);
        }
        assert(audit.marks==1&&audit.moves==357&&owner.size()==1&&owner.retire(key)&&!owner.find(key));
    }
    for(unsigned failure=1;failure<=7;++failure){
        using Owner=ConsumptionBloodInstances<1>;NativeAudit audit;audit.reject=failure;
        Owner owner(Owner::Binding::bindPositionedActor(audit));bool worldGood=true;owner.observeFailure(worldGood);
        Globals globals;GameMakerRandom random(2);InstanceHandle key;
        bool okay=owner.create(105187,real(240),real(192),random,globals,key);
        if(okay)okay=owner.advanceAnimation(key);
        if(okay)okay=owner.step(key,globals);
        if(okay)okay=owner.kill(key);
        assert(!okay&&!owner.valid()&&!worldGood&&audit.calls==failure);
        const auto retained=owner.sourceHandle(105187);assert(owner.find(retained)&&owner.size()==1);
        const auto draws=random.drawCount();assert(!owner.step(retained,globals)&&random.drawCount()==draws&&audit.calls==failure);
        assert(owner.clear()&&!owner.find(retained));
    }
    std::puts("Native consumption blood: 7680 lifecycle frames, activation gating and seven retained failures passed");
    for(unsigned seed=0;seed<128;++seed){
        ConsumptionBloodEffect effect;Environment env{effect};GameMakerRandom random(seed),oracle(seed);
        assert(effect.create(real(240.25),real(192.5),random,env));
        double vy=oracle.random(3)-2,vx=oracle.random(1);if(oracle.chooseIndexSmall(2))vx=-vx;
        const auto frame=oracle.irandomSmall(7);equal(effect.xsp,vx);equal(effect.ysp,vy);
        equal(effect.imageIndex.promoted(),frame);assert(effect.alarm0==120&&env.calls==3);
        assert(random.drawCount()==oracle.drawCount());for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        float x=240.25f,y=192.5f,timeline=float(frame);
        for(unsigned tick=0;tick<120;++tick){
            assert(effect.advanceImage(env));timeline=float(timeline+.3f);if(timeline>=8)timeline-=8;
            equal(effect.imageIndex.promoted(),timeline);
            assert(effect.step(env));x=float(double(x)+vx);y=float(double(y)+vy);vx/=1.01;vy+=env.grav;y=float(double(y)-env.fall);
            equal(effect.x.promoted(),x);equal(effect.y.promoted(),y);equal(effect.xsp,vx);equal(effect.ysp,vy);
        }
        assert(env.positions==360&&effect.kill(env)&&effect.destroyRequested);
    }
    for(unsigned failure=1;failure<=6;++failure){
        ConsumptionBloodEffect effect;Environment env{effect};env.reject=failure;GameMakerRandom random(2);
        bool okay=effect.create(real(240),real(192),random,env);
        if(okay)okay=effect.step(env);
        assert(!okay&&!effect.valid&&env.calls==failure);
        const auto draws=random.drawCount();assert(!effect.step(env)&&env.calls==failure&&random.drawCount()==draws);
    }
    {
        ConsumptionBloodEffect effect;Environment env{effect};GameMakerRandom random(2);
        assert(effect.create(real(240),real(192),random,env));
        const auto velocity=effect.ysp;env.mutateY=true;
        auto expected=real(300);assert(expected.add(velocity));RunnerImageValue rounded;
        assert(rounded.assign(expected));expected=rounded.promoted();assert(expected.add(real(-.125))&&rounded.assign(expected));
        assert(effect.step(env)&&effect.y.bits==rounded.bits); // Re-read Y after the X setter callback.
    }
    std::puts("Consumption blood: 15360 scalar motion/animation frames, source RNG and six failure boundaries passed");
}
