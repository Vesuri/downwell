#include "core/SmokeEffect.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
static void check(bool ok){if(!ok)std::abort();}
static std::uint32_t bits(float value){std::uint32_t b;std::memcpy(&b,&value,4);return b;}
int main(){
    unsigned frames=0,minimum=1000,maximum=0;
    for(unsigned seed=0;seed<1024;++seed){
        downwell::GameMakerRandom random(seed),oracle(seed);
        downwell::SmokeEffect effect;
        check(effect.create({}, {},random));
        const double speed=oracle.random(2)+2;
        float rate=float(speed*(oracle.random(0.13-0.05)+0.05));
        oracle.random(359);
        check(effect.drawFrame()==0 && effect.timeline.bits==0 && effect.imageIndex.bits==0);
        float position=0;
        unsigned age=0,directions=0;
        for(;age<128 && !effect.destroyRequested;++age){
            const float sum=position+rate;
            const bool end=sum>=9;
            position=end?std::fmod(std::fmod(sum,9.0f)+9.0f,9.0f):sum;
            check(effect.advanceImage());
            check(effect.imageIndex.bits==bits(position) && effect.timeline.bits==bits(position));
            check(effect.drawFrame()==unsigned(position) && effect.destroyRequested==end);
            if(end)break; // owner retires Animation End before ordinary Step
            check(effect.step(random,[&](auto,auto,auto& vx,auto& vy){++directions;vx={};vy={};return true;},
                [](auto,auto,auto,auto,bool& escaped){escaped=false;return true;}));
            if(age==0)rate=float(speed*(oracle.random(0.13-0.05)+0.05));
            check(effect.imageSpeed.bits==bits(rate));
        }
        ++age;frames+=age;if(age<minimum)minimum=age;if(age>maximum)maximum=age;
        check(effect.destroyRequested && directions==1 && random.drawCount()==4 && oracle.drawCount()==4);
        for(unsigned i=0;i<16;++i)check(random.stateWord(i)==oracle.stateWord(i));
        // Slot reuse must reset the lifetime, not inherit a retirement request.
        check(effect.create({}, {},random) && !effect.destroyRequested && effect.drawFrame()==0 && !effect.allSet);
    }
    std::printf("Smoke lifetime: %u animation updates, 1024 retirements, lifespan %u..%u passed\n",frames,minimum,maximum);
}
