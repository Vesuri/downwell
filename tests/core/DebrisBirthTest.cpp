#include "core/DebrisBirth.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
static double real(downwell::MotionBinary64 n){
    const std::uint64_t bits=(std::uint64_t(n.high)<<32)|n.low;
    double value;std::memcpy(&value,&bits,8);return value;
}
static std::uint32_t single(double n){float value=float(n);std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
int main(){
    struct Words {
        std::uint32_t value;unsigned calls=0;
        std::uint32_t nextU32(){++calls;return value;}
        unsigned chooseIndexSmall(unsigned count){return nextU32()%count;}
    };
    for(std::uint32_t word:{0u,1u,0x7fffffffu,0xffffffffu}){
        Words words{word};downwell::DebrisBirth birth;
        assert(birth.create(1,words)&&words.calls==4);
        const double unit=double(word)/4294967296.0;
        assert(real(birth.xsp)==unit*4-2&&real(birth.ysp)==unit*1.5-0.5);
        assert(birth.imageIndex.bits==single(unit*7));
        if(word==0xffffffffu)assert(birth.imageIndex.bits==single(7));
    }
    for(unsigned seed=0;seed<1024;++seed)for(int area=0;area<=6;++area){
        downwell::GameMakerRandom actual(seed),oracle(seed);
        downwell::DebrisBirth birth;
        // Reuse a dirty object so Create must restore defaults as well as RNG fields.
        birth.spriteId=-1;birth.flash=42;birth.disappearing=true;birth.noTouch=false;
        birth.alarms.set(3,123);
        const double x=oracle.random(4)-2,y=oracle.random(1.5)-0.5;
        const double rates[]={-0.5,-0.3,0,0.3,0.5};
        const double rate=rates[oracle.chooseIndexSmall(5)],index=oracle.random(7);
        assert(birth.create(area,actual));
        assert(real(birth.xsp)==x&&real(birth.ysp)==y&&real(birth.imageRate)==rate);
        assert(birth.imageSpeed.bits==single(rate)&&birth.imageIndex.bits==single(index));
        assert(birth.spriteId==(area==2?116:area==1||area==3||area==5?115:91));
        assert(real(birth.gravity)==0.08&&real(birth.hardGravity)==0.1);
        assert(!birth.disappearing&&birth.noTouch&&birth.flash==-1);
        assert(birth.alarms.get(0)==30&&birth.alarms.get(1)==15&&birth.alarms.get(2)==7&&birth.alarms.get(3)==-1);
        assert(actual.drawCount()==4&&actual.drawCount()==oracle.drawCount()&&actual.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(actual.stateWord(i)==oracle.stateWord(i));
        assert(actual.nextU32()==oracle.nextU32());
    }
    std::puts("Debris Create: 7168 area/seed cases preserve four draws, float fields and defaults");
}
