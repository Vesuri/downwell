#include "core/ConsumptionBloodEffect.h"
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
int main(){
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
