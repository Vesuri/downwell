#include "core/TimeShardMotion.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>
#include <utility>
using namespace downwell;
static MotionBinary64 number(double d){unsigned long long n;std::memcpy(&n,&d,8);return {unsigned(n>>32),unsigned(n)};}
static void same(MotionBinary64 a,double d){const auto b=number(d);assert(a.high==b.high && a.low==b.low);}
int main(){
    unsigned cases=0,skipped=0;
    for(unsigned mask=0;mask<256;++mask)for(bool stopped:{false,true})
    for(double initialX:{-2.,-.04,0.,.05,1.})for(double initialY:{-2.,-1.,0.,2.}){
        const unsigned seed=mask*31+cases;GameMakerRandom rng(seed),oracle(seed);
        TimeShardMotion shard;shard.xx=number(240.25);shard.yy=number(176.75);
        shard.x.assign(shard.xx);shard.y.assign(shard.yy);
        shard.xsp=number(initialX);shard.ysp=number(initialY);shard.savedSpeed=number(-.3);
        shard.alarm0=3;shard.alarm1=0;shard.alarm2=5;shard.disappearing=true;
        double xx=240.25,yy=176.75,vx=initialX,vy=initialY;float speed=0;
        // A solid query is a pure function of position within one Step:
        // each distinct position takes the mask bit of its first query.
        bool dead=false;std::vector<std::pair<double,double>> probes,distinct;
        auto answer=[&](double x,double y){
            for(unsigned i=0;i<distinct.size();++i)if(distinct[i].first==x&&distinct[i].second==y)return bool((mask>>i)&1);
            distinct.emplace_back(x,y);return bool((mask>>(distinct.size()-1))&1);
        };
        auto hit=[&](double x,double y){probes.emplace_back(x,y);return answer(x,y);};
        if(!stopped){vy+=vy<-1?.1:.08;speed=float(-.3);}
        if(hit(xx+vx,yy)){vx*=-1;vx*=.7;}
        if(hit(xx,yy+vy)){
            if(vy<=0)vy*=0;else vy=double(oracle.irandom(0));
            vx*=.7;const double rates[]={-.3,-.1,0,.1,.3};speed=float(rates[oracle.nextU32()%5]);
        }
        if(std::abs(vx)<.05)vx=(vx>0?1:vx<0?-1:0)*.05;
        xx+=vx;yy+=vy;vx*=.9;vy*=.9;
        if(std::abs(vx)<.05)vx=0;
        if(std::abs(vy)<.05)vy=0;
        if(hit(xx,yy)){
            if(!hit(xx+8,yy))xx+=8;else if(!hit(xx-8,yy))xx-=8;
            if(!hit(xx,yy-8))yy-=8;else if(!hit(xx,yy+8))yy+=8;
            if(hit(xx,yy))dead=true;
        }
        // The Step may skip a query that repeats the first probe's position
        // (the resting shard's final meeting); the rest come in order.
        unsigned calls=0,next=0;bool counted=false,outside=false;
        assert(shard.step(stopped,rng,[&](auto x,auto y,bool& result){
            if(next==2&&probes.size()>2&&probes[2]==probes[0]){++next;++skipped;}
            assert(next<probes.size());same(x,probes[next].first);same(y,probes[next].second);
            ++next;++calls;result=answer(probes[next-1].first,probes[next-1].second);return true;
        },[&](){assert(shard.destroyRequested==dead);counted=true;return 20;},
        [&](const auto& state,bool& retire){assert(counted);same(state.y.promoted(),176.75);outside=true;retire=false;return true;}));
        if(probes.size()>2&&probes[2]==probes[0]&&calls==2){++next;++skipped;}
        assert(next==probes.size() && outside && shard.destroyRequested==dead);
        same(shard.xx,xx);same(shard.yy,yy);same(shard.xsp,vx);same(shard.ysp,vy);
        same(shard.x.promoted(),std::nearbyint(xx));same(shard.y.promoted(),std::nearbyint(yy));
        same(shard.imageSpeed.promoted(),double(speed));assert(shard.dflash==1);
        assert(shard.alarm0==(stopped?4:3) && shard.alarm1==0 && shard.alarm2==5);
        assert(rng.drawCount()==oracle.drawCount());
        for(unsigned i=0;i<16;++i)assert(rng.stateWord(i)==oracle.stateWord(i));
        ++cases;
    }
    for(bool overflow:{false,true})for(bool escaped:{false,true}){
        TimeShardMotion shard;GameMakerRandom rng(5);shard.xx=number(20.5);shard.yy=number(10.5);
        shard.xsp=number(1);shard.ysp=number(1);
        assert(shard.step(true,rng,[](auto,auto,bool& hit){hit=false;return true;},
            [&](){return overflow?21:20;},[&](const auto& state,bool& retire){
                assert(state.destroyRequested==overflow);retire=escaped;return true;
            }));
        assert(shard.destroyRequested==(overflow||escaped));same(shard.x.promoted(),22);same(shard.y.promoted(),12);
    }
    TimeShardMotion failed;GameMakerRandom rng(5);unsigned queries=0;
    assert(!failed.step(true,rng,[&](auto,auto,bool&){++queries;return false;},
        [](){assert(false);return 0;},[](const auto&,bool&){assert(false);return true;}));
    assert(queries==1 && rng.drawCount()==0);
    assert(skipped>0);
    std::printf("Time-shard Step: %u cases (%u repeated final queries reused), exact motion, ordered probes/RNG, stop behavior and post-retirement publication passed\n",cases,skipped);
}
