#include "core/LimboShard.h"
#include "core/GameMakerRandom.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
struct LimboShardProbeOps {
    bool speed(downwell::RunnerImageValue);bool image(downwell::RunnerImageValue);
    bool alarm(unsigned,int);bool meeting(downwell::MotionBinary64,downwell::MotionBinary64,int,bool&);
    bool positionX(downwell::RunnerImageValue);bool positionY(downwell::RunnerImageValue);
    bool population(unsigned&);bool outside(downwell::RunnerImageValue,downwell::RunnerImageValue,bool&);
    bool viewTop(downwell::MotionBinary64&);bool destroy();
};
bool limboShardNativeProbe(downwell::LimboShard& shard,downwell::GameMakerRandom& random,LimboShardProbeOps& ops){
    return shard.create({}, {},false,random,ops)&&shard.step(true,{},random,ops)&&
        shard.bulletContact({}, {},random,ops);
}
#else
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace downwell;
static MotionBinary64 number(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {unsigned(b>>32),unsigned(b)};}
static double value(MotionBinary64 n){const std::uint64_t b=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&b,8);return d;}
static unsigned bits(float f){unsigned n;std::memcpy(&n,&f,4);return n;}
struct Ops {
    std::vector<int> calls;bool hit=false,out=false;unsigned count=0;double top=-10000;int reject=0;
    bool call(int n){calls.push_back(n);return n!=reject;}
    bool speed(RunnerImageValue){return call(1);}
    bool image(RunnerImageValue){return call(2);}
    bool alarm(unsigned index,int ticks){assert(index==2&&ticks==5);return call(3);}
    bool meeting(MotionBinary64,MotionBinary64,int object,bool& result){assert(object==56);result=hit;return call(4);}
    bool positionX(RunnerImageValue){return call(5);}
    bool positionY(RunnerImageValue){return call(6);}
    bool population(unsigned& n){n=count;return call(7);}
    bool outside(RunnerImageValue,RunnerImageValue,bool& result){result=out;return call(8);}
    bool viewTop(MotionBinary64& y){y=number(top);return call(9);}
    bool destroy(){return call(10);}
};
static void sameRandom(const GameMakerRandom& a,const GameMakerRandom& b){
    assert(a.drawCount()==b.drawCount());for(unsigned i=0;i<16;++i)assert(a.stateWord(i)==b.stateWord(i));
}
int main(){
    for(unsigned seed=0;seed<4096;++seed){
        GameMakerRandom random(seed),oracle(seed);LimboShard shard;Ops ops;
        assert(shard.create(number(240.125),number(511.875),seed&1,random,ops));
        const double vx=int(oracle.irandom(4))-2;
        const int vy[]={0,-1,-1,-1,-2,-2,-3};const double vertical=vy[oracle.nextU32()%7];
        const double ascent=oracle.random(.6-.1)+.1;
        const double rates[]={0,.01,.05};const double rate=rates[oracle.nextU32()%3];
        const double speed=rate*(oracle.nextU32()%2?1:-1);
        const float image=float(oracle.random(7));
        assert(value(shard.xsp)==vx&&value(shard.ysp)==vertical&&value(shard.ascendSpeed)==ascent);
        const auto expected=number(speed);
        assert(shard.savedSpeed.high==expected.high&&shard.savedSpeed.low==expected.low);
        assert(shard.imageSpeed.bits==bits(float(speed))&&shard.imageIndex.bits==bits(image));
        assert(shard.shardLimit==(seed&1?5:16)&&!shard.noTouch&&shard.alarm2==-1&&shard.drawsSelf());
        assert((ops.calls==std::vector<int>{1,2})&&random.drawCount()==7);sameRandom(random,oracle);
        for(unsigned frame=0;frame<20;++frame){
            const bool allowed=frame%3!=0;ops.calls.clear();ops.hit=frame%4==0;
            double x=value(shard.xx),y=value(shard.yy),xs=value(shard.xsp),ys=value(shard.ysp);
            if(allowed){if(ops.hit)xs=-xs*.7;y-=ascent;}
            if(std::abs(xs)<.025)xs=(xs>0?1:xs<0?-1:0)*.025;
            x+=xs;y+=ys;xs*=.97;ys*=.9;
            if(std::abs(xs)<.05)xs=0;if(std::abs(ys)<.05)ys=0;
            assert(shard.step(allowed,{},random,ops));
            assert(value(shard.xx)==x&&value(shard.yy)==y&&value(shard.xsp)==xs&&value(shard.ysp)==ys);
            assert(shard.x.bits==bits(float(std::round(x)))&&shard.y.bits==bits(float(std::round(y))));
            assert(shard.imageSpeed.bits==bits(allowed?float(speed):0.f));
            assert(ops.calls==(allowed?std::vector<int>{1,4,5,6,7,9}:std::vector<int>{1,5,6,7,9}));
        }
        sameRandom(random,oracle);
    }
    // Firing strip has four strict boundaries and a one-shot guard. A
    // stationary horizontal kick still consumes its random sample.
    for(int dx:{-16,-15,0,15,16})for(int dy:{0,1,159,160})for(bool allowed:{false,true}){
        GameMakerRandom r(3),oracle(3);LimboShard s;Ops ops;
        assert(s.create(number(240+dx),number(500+dy),false,r,ops));oracle=r;ops.calls.clear();
        const bool kicked=allowed&&dx>-16&&dx<16&&dy>0&&dy<160;
        if(kicked){oracle.nextU32();oracle.nextU32();}
        assert(s.step(allowed,{true,number(240),number(500)},r,ops));
        assert(s.noTouch==kicked&&s.alarm2==(kicked?5:-1));sameRandom(r,oracle);
        if(kicked){const auto n=r.drawCount();assert(s.step(true,{true,number(240),number(500)},r,ops));assert(r.drawCount()==n);}
    }
    for(double vx:{-2.,0.,2.}){
        GameMakerRandom r(9),oracle(9);LimboShard s;Ops ops;
        const double magnitude=oracle.random(2);
        assert(s.bulletContact(number(vx),number(4.25),r,ops));
        assert(value(s.xsp)==(vx==0?0:(vx<0?-magnitude:magnitude))&&value(s.ysp)==4.25);
        assert(s.noTouch&&s.alarm2==5&&(ops.calls==std::vector<int>{3}));sameRandom(r,oracle);
        assert(s.bulletContact(number(-vx),number(1),r,ops));sameRandom(r,oracle);
    }
    // The source makes both destruction calls and still reads the view after
    // the population-limited retirement; callbacks cannot free the record.
    {
        GameMakerRandom r(0);LimboShard s;Ops ops;
        assert(s.create(number(240),number(500),false,r,ops));ops.calls.clear();
        ops.count=17;ops.out=true;ops.top=1000;
        assert(s.step(false,{},r,ops)&&s.destroyRequested);
        assert((ops.calls==std::vector<int>{1,5,6,7,8,10,9,10}));
    }
    // Rejected y publication keeps x's accepted setter and private movement.
    {
        GameMakerRandom r(0);LimboShard s;Ops ops;
        assert(s.create(number(240),number(500),false,r,ops));
        s.xsp=number(2);s.ysp=number(3);ops.calls.clear();ops.reject=6;
        assert(!s.step(false,{},r,ops));
        assert(value(s.xx)==242&&value(s.yy)==503&&value(s.x.promoted())==242&&value(s.y.promoted())==500);
        assert((ops.calls==std::vector<int>{1,5,6}));
    }
    LimboShard s;for(int flash:{-1,0,1}){s.dflash=flash;assert(s.drawsSelf()==(flash<=0));}
    std::puts("Limbo shard: 4096 Create/RNG cases, 81920 Steps, strict firing bounds, bullet contact and retained setter/destruction prefixes passed");
}
#endif
