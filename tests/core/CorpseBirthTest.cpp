#include "core/CorpseBirth.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static void equal(MotionBinary64 a,double d){const auto b=real(d);if(a.high!=b.high||a.low!=b.low)std::fprintf(stderr,"got %08x:%08x expected %08x:%08x (%g)\n",a.high,a.low,b.high,b.low,d);assert(a.high==b.high&&a.low==b.low);}
struct Queries {
    GameMakerRandom& oracle;
    unsigned failCount,wallCalls=0,tileCalls=0,publishes=0;
    bool tileBlocked;
    int rx=0,ry=0,radius=4;
    bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){
        if(!wallCalls++){equal(x,12.25);equal(y,-3.5);hit=!tileBlocked;return true;}
        rx=int(oracle.irandomSmall(radius*2))-radius;
        ry=int(oracle.irandomSmall(radius*2))-radius;
        equal(x,16+rx);equal(y,ry);
        hit=!tileBlocked&&wallCalls-1<=failCount;
        if(wallCalls-1<=failCount&&radius<=32)radius+=4;
        return true;
    }
    bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){
        ++tileCalls;
        if(wallCalls==1){equal(x,12.25);equal(y,-3.5);hit=true;}
        else {equal(x,16);equal(y,0);hit=wallCalls-1<=failCount;}
        return true;
    }
    bool publish(RunnerImageValue x,RunnerImageValue y){
        ++publishes;
        equal(x.promoted(),publishes<=2?16:16+rx);
        equal(y.promoted(),publishes==1?-3.5:publishes==4?ry:0);
        return true;
    }
};
struct FailureQueries {
    unsigned calls=0,fail;
    bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=calls==0;return ++calls!=fail;}
    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return ++calls!=fail;}
    bool publish(RunnerImageValue,RunnerImageValue){return ++calls!=fail;}
};
int main(){
    unsigned cases=0;
    for(unsigned seed=0;seed<128;++seed)for(unsigned failures:{0u,1u,7u,8u,31u,32u,33u,40u})
    for(bool tiles:{false,true})for(bool upgrade:{false,true}){
        GameMakerRandom random(seed),oracle(seed);
        const double vx=oracle.random(6)-3,vy=oracle.random(4)-2;
        const double rates[]={-.5,-.3,0,.3,.5};const float rate=float(rates[oracle.chooseIndexSmall(5)]);
        Queries queries{oracle,failures,0,0,0,tiles};CorpseBirth birth;
        assert(birth.create(real(12.25),real(-3.5),upgrade,random,queries));
        equal(birth.xsp,vx);equal(birth.ysp,vy);equal(birth.imageSpeed.promoted(),double(rate));
        assert(birth.health==int(upgrade)&&birth.hitEffect==int(upgrade));
        assert(!birth.grounded&&!birth.active&&!birth.hit&&!birth.disappearing&&birth.noTouch&&birth.bouncing);
        assert(birth.explosionFx==102&&birth.flash==-1&&birth.bounceSound==-1);
        equal(birth.gravity,.1);equal(birth.hardGravity,.2);
        assert(birth.alarms.get(0)==180&&birth.alarms.get(1)==120&&birth.alarms.get(3)==8);
        const unsigned attempts=failures<33?failures+1:33;
        assert(queries.wallCalls==attempts+1&&queries.publishes==4);
        assert(birth.attempts==int(failures<33?failures:33)&&birth.radius==queries.radius);
        equal(birth.xx,16+queries.rx);equal(birth.yy,queries.ry);
        assert(random.drawCount()==3+attempts*4&&random.drawCount()==oracle.drawCount());
        assert(random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        ++cases;
    }
    struct Empty {
        unsigned calls=0;
        bool wall(MotionBinary64,MotionBinary64,bool& hit){++calls;hit=false;return true;}
        bool tile(MotionBinary64,MotionBinary64,bool& hit){++calls;hit=false;return true;}
        bool publish(RunnerImageValue,RunnerImageValue){assert(false);return false;}
    } empty;
    CorpseBirth birth;GameMakerRandom random(9);
    assert(birth.create(real(12.25),real(-3.5),false,random,empty));
    assert(empty.calls==2&&random.drawCount()==3);equal(birth.xx,12.25);equal(birth.yy,-3.5);
    for(unsigned failure=1;failure<=7;++failure){
        FailureQueries queries{0,failure};GameMakerRandom rng(9);
        assert(!birth.create(real(12.25),real(-3.5),false,rng,queries));
        assert(queries.calls==failure);
        assert(rng.drawCount()==(failure<=3?3u:7u));
    }
    for(int quarter=-4096;quarter<=4096;++quarter){
        RunnerImageValue coordinate,out;assert(coordinate.assign(real(quarter/4.)));
        assert(CorpseBirth::snap(coordinate,out));
        // Original fistp produces an integer before multiplying by 16;
        // negative values snapping to zero therefore lose their sign bit.
        equal(out.promoted(),int(std::nearbyint(quarter/64.))*16.);
    }
    std::printf("Corpse Create: %u relocation/RNG cases and 8193 fractional snap coordinates passed\n",cases);
}
