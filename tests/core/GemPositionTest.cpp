#include "core/GemMotion.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Random {
    std::uint32_t state=1,draws=0;
    unsigned next(unsigned limit){++draws;state=state*1664525u+1013904223u;return state%(limit+1);}
    unsigned randomIntegerSmall(unsigned limit){return next(limit);}
    unsigned irandomSmall(unsigned limit){return next(limit);}
    unsigned chooseIndexSmall(unsigned count){return next(count-1);}
};
static void check(long raw,const GemPosition& value) {
    assert(long(value)==raw);
    assert(value.pixel()==ReferenceGemMotion::pixel(raw));
    assert(value.rounded()==ReferenceGemMotion::rounded(raw));
    assert(value.q8()==ReferenceGemMotion::q8(raw));
}
int main() {
    unsigned positions=0,steps=0;
    for(long whole:{-10000L,-2L,-1L,0L,1L,2L,10000L})
    for(long fraction=0;fraction<12800;++fraction) {
        const long raw=whole*12800+fraction;
        GemPosition position(raw);check(raw,position);++positions;
        for(long delta:{-230400L,-115200L,-12800L,-6401L,-25L,-1L,0L,1L,25L,6401L,12800L,115200L,230400L})
            check(raw+delta,position+delta);
    }
    for(unsigned seed=1;seed<=128;++seed)for(int area=1;area<=5;++area)for(bool attracted:{false,true}) {
        BasicGemMotion<GemPosition> actual;
        ReferenceGemMotion reference;
        Random a{seed},b{seed};
        actual.initialize(int(seed)-64,-int(seed),attracted,a,seed&1,area);
        reference.initialize(int(seed)-64,-int(seed),attracted,b,seed&1,area);
        for(unsigned tick=0;tick<400;++tick) {
            std::vector<int> queriesA,queriesB;
            auto solid=[](std::vector<int>& log,int x,int y){log.push_back(x);log.push_back(y);return y>=80 || (x<0 && (y%17)==0);};
            const long targetX=(long(tick%97)-48)*256,targetY=(long(tick%89)-44)*256;
            const bool alive=actual.step(targetX,targetY,[&](int x,int y){return solid(queriesA,x,y);},a,seed&2);
            const bool expected=reference.step(targetX,targetY,[&](int x,int y){return solid(queriesB,x,y);},b,seed&2);
            assert(alive==expected && queriesA==queriesB && a.state==b.state && a.draws==b.draws);
            check(reference.x,actual.x);check(reference.y,actual.y);
            assert(actual.vx==reference.vx && actual.vy==reference.vy);
            assert(actual.attracted==reference.attracted && actual.attractorAtBirth==reference.attractorAtBirth);
            assert(actual.gravity==reference.gravity && actual.maximumGravity==reference.maximumGravity);
            assert(actual.homing.direction==reference.homing.direction && actual.homing.turnFifths==reference.homing.turnFifths && actual.homing.speedEighths==reference.homing.speedEighths);
            assert(actual.lifetime.age==reference.lifetime.age);
            assert(actual.sound.allSet==reference.sound.allSet && actual.sound.bounceSound==reference.sound.bounceSound && actual.sound.mySound==reference.sound.mySound);
            assert(actual.animation.phaseBits()==reference.animation.phaseBits() && actual.animation.speedBits()==reference.animation.speedBits());
            assert(actual.collectible(targetX,targetY,seed&2)==reference.collectible(targetX,targetY,seed&2));
            ++steps;
            if(!alive)break;
        }
    }
    std::printf("Gem positions: %u exhaustive fractional positions, %u exact trajectory/RNG/query steps\n",positions,steps);
}
