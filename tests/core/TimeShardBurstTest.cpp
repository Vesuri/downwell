#include "core/TimeShardBurst.h"
// UTMT 0.9.2.0 gml_GlobalScript_scrTimeShard.asm SHA-256:
// 2403af36b17616f6b2f86e2fe55c31b0b94fe93bd65433d5a0fc99044adf2d6a.
// data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8.
#include "core/GameMakerRandom.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
#include <cmath>
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 number(double d){unsigned long long n;std::memcpy(&n,&d,8);return {unsigned(n>>32),unsigned(n)};}
static void same(MotionBinary64 a,double d){auto b=number(d);assert(a.high==b.high && a.low==b.low);}
int main(){
    for(unsigned seed=0;seed<1024;++seed)for(bool field:{false,true}){
        GameMakerRandom rng(seed),oracle(seed);TimeShardEffects<4> effects;TimeShardBurst burst;
        unsigned lookups=0,components=0;double angle=30,scatter=0,speed=0;
        const RunnerImageValue x{0x43700000},y{0x43300000}; // 240,176
        assert(burst.emit(x,y,number(2),number(-3),false,rng,effects,
            [&](auto px,auto py,auto& id,auto& fx,auto& fy){
                assert(px.bits==x.bits && py.bits==y.bits);++lookups;
                id=field?100+lookups:-4;fx=number(100+lookups);fy=number(150);return true;
            },[&](auto ax,auto ay,auto bx,auto by,auto& out){
                if(field){assert(lookups==2);same(ax,102);same(ay,150);same(bx,240);same(by,176);}
                else{assert(lookups==1);same(ax,0);same(ay,0);same(bx,2);same(by,-3);}
                out=number(angle);return true;
            },[&](auto length,auto degrees,bool vertical,auto& out){
                const unsigned child=components/2;
                if(!vertical){
                    if(child==2)angle+=180;
                    scatter=angle+(-45+oracle.random(90));speed=1+oracle.random(5);
                    TimeShardBirth birth;assert(birth.create(x.promoted(),y.promoted(),false,oracle));
                    assert(effects.size()==child+1 && rng.drawCount()==oracle.drawCount());
                    for(unsigned j=0;j<16;++j)assert(rng.stateWord(j)==oracle.stateWord(j));
                }
                same(length,speed);same(degrees,scatter);++components;
                // A sentinel vector isolates burst ordering from trig accuracy.
                out=number(vertical?-7:9);return true;
            }));
        assert(components==6 && effects.size()==3 && rng.drawCount()==24);
        assert(burst.field==(field?102:-4) && burst.shard==3);same(burst.direction,210);
        same(burst.scatteredDirection,scatter);same(burst.speed,speed);
        effects.visitNewest([](auto,const auto& shard){same(shard.xsp,9);same(shard.ysp,-7);});
    }
    TimeShardEffects<1> effects;TimeShardBurst burst;GameMakerRandom rng(3);
    auto lookup=[](auto,auto,auto& id,auto&,auto&){id=-4;return true;};
    auto direction=[](auto,auto,auto,auto,auto& out){out={};return true;};
    unsigned components=0;
    assert(!burst.emit({}, {},{}, {},false,rng,effects,lookup,direction,
        [&](auto,auto,bool,auto& out){++components;out={};return true;}));
    assert(effects.size()==1 && rng.drawCount()==10 && components==2 && burst.shard==0);
    effects.clear();components=0;
    assert(!burst.emit({}, {},{}, {},false,rng,effects,lookup,direction,
        [&](auto,auto,bool y,auto& out){++components;if(y)return false;out=number(11);return true;}));
    assert(components==2 && effects.size()==1 && effects.get(burst.shard));
    same(effects.get(burst.shard)->xsp,11); // No rollback after successful X write.
    for(unsigned seed=0;seed<512;++seed){
        TimeShardEffects<3> live;TimeShardBurst b;GameMakerRandom random(seed);
        assert(b.emit({}, {},{}, {},false,random,live,lookup,
            [&](auto,auto,auto,auto,auto& out){out=number(seed%360);return true;},
            [&](auto length,auto angle,bool vertical,auto& out){
                MotionBinary64 x,y;
                if(!SmokeDirection::vector(length,angle,x,y,smokeTrig))return false;
                out=vertical?y:x;
                // Check the actual published component after source narrowing
                // and fractional snap, separately from burst RNG sequencing.
                auto real=[](MotionBinary64 n){unsigned long long bits=(static_cast<unsigned long long>(n.high)<<32)|n.low;double d;std::memcpy(&d,&bits,8);return d;};
                const float radians=(float(real(angle))*float(3.14159265358979323846))/180.f;
                float v=float(real(length))*float(vertical?std::sin(double(radians)):std::cos(double(radians)));
                if(vertical)v=-v;
                double integral;const float fractional=float(std::modf(double(v),&integral));
                same(out,std::abs(fractional)<.0001f?double(float(integral)):double(v));
                return true;
            }));
        assert(live.size()==3 && random.drawCount()==24);
    }
    std::puts("Shard burst: 2048 source-order/RNG cases, repeated field lookup, synchronous births and partial failures passed");
}
