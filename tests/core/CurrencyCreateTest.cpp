#include "core/CurrencyCreate.h"
#include "core/CurrencyCreateVerification.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static void equal(MotionBinary64 a,double d){const auto b=real(d);assert(a.high==b.high&&a.low==b.low);}
struct Queries {
    GameMakerRandom& oracle;
    unsigned failures,wallCalls=0,tileCalls=0,publishes=0;
    bool tiles;
    int rx=0,ry=0,radius=4;
    bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){
        if(!wallCalls++){equal(x,12.25);equal(y,-3.5);hit=!tiles;return true;}
        rx=int(oracle.irandomSmall(radius*2))-radius;ry=int(oracle.irandomSmall(radius))-radius;
        equal(x,16+rx);equal(y,ry);
        hit=!tiles&&wallCalls-1<=failures;
        if(wallCalls-1<=failures&&radius<=64)radius+=4;
        return true;
    }
    bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){
        ++tileCalls;
        if(wallCalls==1){equal(x,12.25);equal(y,-3.5);hit=true;}
        else{equal(x,16);equal(y,0);hit=wallCalls-1<=failures;}
        return true;
    }
    bool publish(RunnerImageValue x,RunnerImageValue y){
        ++publishes;equal(x.promoted(),publishes<=2?16:16+rx);
        equal(y.promoted(),publishes==1?-3.5:publishes==4?ry:0);return true;
    }
};
struct FailureQueries {
    unsigned calls=0,fail;
    bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=calls==0;return ++calls!=fail;}
    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return ++calls!=fail;}
    bool publish(RunnerImageValue,RunnerImageValue){return ++calls!=fail;}
};
int main(){
    assert(CurrencyCreateVerification::run());
    unsigned cases=0;
    for(unsigned seed=0;seed<64;++seed)for(int area=1;area<=5;++area)
    for(unsigned failures:{0u,1u,15u,16u,31u,32u,33u,40u})for(bool tiles:{false,true})for(bool upgrade:{false,true}){
        GameMakerRandom random(seed),oracle(seed);const double vx=oracle.random(4)-2;
        double vy=oracle.random(4)-2;if(area==5)vy=-oracle.random(3);
        const double rates[]={-.5,-.3,.3,.5};const double rate=rates[oracle.chooseIndexSmall(4)];
        const int second=170+int(oracle.irandomSmall(70)),first=int(oracle.irandomSmall(70))-60;
        const int direction=oracle.chooseIndexSmall(2)?second:first;
        const auto baseDraws=oracle.drawCount();
        Queries queries{oracle,failures,0,0,0,tiles};CurrencyCreate birth;
        assert(birth.create(real(12.25),real(-3.5),area,upgrade,random,queries));
        equal(birth.xsp,vx);equal(birth.ysp,vy);equal(birth.imgSp,rate);equal(birth.imageSpeed.promoted(),double(float(rate)));
        assert(birth.direction==direction&&birth.size==0&&birth.getAmount==2&&!birth.attracted);
        assert(birth.active&&!birth.allSet&&!birth.suckable&&!birth.obtainable&&!birth.disappearing&&birth.flash==-1);
        equal(birth.gravity,area==3||area==4?.035/4:.035);equal(birth.hardGravity,.2);
        equal(birth.maximumGravity,area==3||area==4?.5:4);
        assert(birth.unobtime==10&&birth.obtime==180&&birth.distime==120&&birth.stuckKamo==0&&birth.bounceGain==1&&birth.stepRate==1);
        assert(birth.attractionRadius==(upgrade?48:24)&&birth.attractionSpeed==(upgrade?6:4));
        assert(birth.alarms.get(0)==10&&birth.alarms.get(3)==(upgrade?30:15)&&birth.alarms.get(1)==-1);
        assert(birth.rotationSpeed==5&&birth.gemSpeed==1&&birth.bounceSound==14&&birth.maxRadius==32);
        const unsigned attempts=failures<33?failures+1:33;
        assert(queries.wallCalls==attempts+1&&queries.publishes==4);
        assert(birth.attempts==int(failures<33?failures:33)&&birth.radius==queries.radius&&birth.ry<=0);
        equal(birth.xx,16+queries.rx);equal(birth.yy,queries.ry);
        assert(random.drawCount()==baseDraws+attempts*4&&random.drawCount()==oracle.drawCount());
        assert(random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));++cases;
    }
    struct Empty {
        unsigned calls=0;
        bool wall(MotionBinary64,MotionBinary64,bool& hit){++calls;hit=false;return true;}
        bool tile(MotionBinary64,MotionBinary64,bool& hit){++calls;hit=false;return true;}
        bool publish(RunnerImageValue,RunnerImageValue){assert(false);return false;}
    } empty;
    CurrencyCreate birth;GameMakerRandom random(9);
    assert(birth.create(real(12.25),real(-3.5),1,false,random,empty));
    assert(empty.calls==2&&random.drawCount()==8);equal(birth.xx,12.25);equal(birth.yy,-3.5);
    for(unsigned failure=1;failure<=7;++failure){
        FailureQueries queries{0,failure};GameMakerRandom rng(9);
        assert(!birth.create(real(12.25),real(-3.5),1,false,rng,queries));
        assert(queries.calls==failure&&rng.drawCount()==(failure<=3?8u:12u));
    }
    std::printf("Currency Create: %u area/upgrade/relocation/RNG cases and query failure prefixes passed\n",cases);
}
