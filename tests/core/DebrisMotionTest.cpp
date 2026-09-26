#include "core/DebrisMotion.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>
#include <utility>
static double real(downwell::MotionBinary64 n){
    const std::uint64_t bits=(std::uint64_t(n.high)<<32)|n.low;double v;std::memcpy(&v,&bits,8);return v;
}
static std::uint64_t bits(double v){std::uint64_t n;std::memcpy(&n,&v,8);return n;}
int main(){
    using Real=downwell::MotionBinary64;
    for(unsigned seed=0;seed<32;++seed)for(unsigned pattern=0;pattern<256;++pattern){
        downwell::GameMakerRandom random(seed);downwell::DebrisMotion effect;
        assert(effect.create({0x403f4000u,0},{0xc0501000u,0},1,random)); // 31.25, -64.25
        if(seed%4==0)effect.ysp=Real::fromInteger(-2);
        if(seed%4==1)effect.xsp={};
        effect.disappearing=seed%2;effect.flash=-1;
        auto oracle=random;
        float x=float(real(effect.x.promoted())),y=float(real(effect.y.promoted()));
        double vx=real(effect.xsp),vy=real(effect.ysp);
        float speed=float(real(effect.imageRate));bool destroyed=false;
        std::vector<std::pair<std::uint64_t,std::uint64_t>> queries;
        auto meeting=[&](double px,double py){
            const bool hit=(pattern>>queries.size())&1u;
            queries.emplace_back(bits(px),bits(py));return hit;
        };
        vy+=vy<-1?0.1:0.08;
        if(meeting(double(x)+vx,y)){vx*=-1;vx*=0.7;}
        if(meeting(x,double(y)+vy)){
            if(vy<=0)vy*=0;else vy=double(oracle.irandom(0));
            vx*=0.7;const double rates[]={-0.3,-0.1,0,0.1,0.3};
            speed=float(rates[oracle.chooseIndexSmall(5)]);
        }
        if(std::abs(vx)<0.05)vx=(vx>0?1:vx<0?-1:0)*0.05;
        x=float(double(x)+vx);y=float(double(y)+vy);
        if(meeting(x,y)){
            if(!meeting(double(x)+8,y))x=float(double(x)+8);
            else if(!meeting(double(x)-8,y))x=float(double(x)-8);
            if(!meeting(x,double(y)-8))y=float(double(y)-8);
            else if(!meeting(x,double(y)+8))y=float(double(y)+8);
            destroyed=meeting(x,y);
        }
        unsigned calls=0;
        assert(effect.step(false,random,[&](Real px,Real py,bool& hit){
            assert(calls<queries.size());
            assert(queries[calls]==std::make_pair(bits(real(px)),bits(real(py))));
            hit=(pattern>>calls++)&1u;return true;
        }));
        assert(calls==queries.size()&&real(effect.x.promoted())==x&&real(effect.y.promoted())==y);
        assert(bits(real(effect.xsp))==bits(vx)&&bits(real(effect.ysp))==bits(vy));
        assert(real(effect.imageSpeed.promoted())==speed&&effect.destroyRequested==destroyed);
        assert(effect.flash==(effect.disappearing?1:-1));
        assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
    }
    downwell::GameMakerRandom random(7);downwell::DebrisMotion frozen;
    assert(frozen.create({}, {},1,random));const auto initial=frozen;
    const auto draws=random.drawCount();
    for(unsigned tick=0;tick<40;++tick){
        for(unsigned alarm=0;alarm<3;++alarm)frozen.advanceAlarm(alarm);
        assert(frozen.step(true,random,[](Real,Real,bool&){assert(false);return false;}));
        assert(!frozen.destroyRequested&&!frozen.disappearing&&frozen.noTouch);
        assert(frozen.alarms.get(0)==30&&frozen.alarms.get(1)==15&&frozen.alarms.get(2)==7);
        assert(frozen.x.bits==initial.x.bits&&frozen.y.bits==initial.y.bits&&frozen.imageSpeed.bits==0&&frozen.flash==-1);
        assert(random.drawCount()==draws);
    }
    frozen.alarms.set(1,1);frozen.alarms.set(2,1);
    frozen.advanceAlarm(1);frozen.advanceAlarm(2);
    assert(frozen.disappearing&&!frozen.noTouch);
    assert(frozen.step(true,random,[](Real,Real,bool&){return false;}));
    assert(frozen.alarms.get(1)==0&&frozen.alarms.get(2)==0); // Already fired: no refund.
    frozen.alarms.set(0,1);frozen.advanceAlarm(0);assert(frozen.destroyRequested);
    assert(frozen.step(false,random,[](Real,Real,bool&){assert(false);return false;}));
    downwell::DebrisMotion failed;assert(failed.create({}, {},1,random));
    assert(!failed.step(false,random,[](Real,Real,bool&){return false;}));
    std::puts("Debris motion: 8192 ordered collision/RNG cases, alarm edges and complete freeze passed");
}
