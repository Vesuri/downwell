#include "core/SkeletonEffectBursts.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
static void check(bool ok){if(!ok)std::abort();}
static downwell::MotionBinary64 number(double x){
    std::uint64_t bits;std::memcpy(&bits,&x,8);return {std::uint32_t(bits>>32),std::uint32_t(bits)};
}
static double real(downwell::MotionBinary64 x){
    const std::uint64_t bits=(std::uint64_t(x.high)<<32)|x.low;
    double value;std::memcpy(&value,&bits,8);return value;
}
struct Birth {int object;double x,y;};
int main(){
    unsigned births=0;
    for(unsigned seed=0;seed<1024;++seed) for(bool flash:{false,true}) for(bool box:{false,true}) for(bool published:{false,true})
    for(int capacity:{0,2,32}) {
        downwell::GameMakerRandom random(seed),oracle(seed);
        std::vector<Birth> actual,expected;
        // A synchronous Create probe models the source child draw sites. It
        // does not claim to implement the child's motion or raster behavior.
        auto childDraws=[](downwell::GameMakerRandom& rng,bool isFlash){
            if(isFlash){rng.irandomSmall(4);rng.irandomSmall(4);}
            else {rng.nextU32();rng.nextU32();rng.nextU32();}
        };
        auto create=[&](int object,downwell::MotionBinary64 x,downwell::MotionBinary64 y){
            if(int(actual.size())==capacity)return false;
            actual.push_back({object,real(x),real(y)});
            childDraws(random,flash);return true;
        };
        const double x=seed%2?96.125:-32.375,y=seed%3?192.75:-256.25;
        downwell::SmokeBurstCaller caller;
        bool result=published&&flash?(box?
                         downwell::flashBurstPublished<1,0,2>(caller,number(x),number(y),random,create):
                         downwell::flashBurstPublished<6,2,10>(caller,number(x),number(y),random,create)):
                         published?(box?
                         downwell::smokeBurstPublished<1,0>(caller,number(x),number(y),random,create):
                         downwell::smokeBurstPublished<5,1>(caller,number(x),number(y),random,create)):
                         box?(flash?downwell::boxFlashBurst(number(x),number(y),random,create):
                              downwell::boxSmokeBurst(number(x),number(y),random,create)):
                         (flash?downwell::skeletonFlashBurst(number(x),number(y),random,create):
                                downwell::skeletonSmokeBurst(number(x),number(y),random,create));
        // Independent 64-bit modulus and ordinary host-double arithmetic.
        const unsigned count=(box?1:4)+unsigned(oracle.irandom(box?0:flash?4:2));
        if(published)check(real(caller.smx)==x&&real(caller.smy)==y&&
            caller.smokenum==int(count)&&caller.vary==(box?0:flash?2:1)&&
            caller.pvary==(flash?(box?2:10):0));
        bool success=true;
        for(unsigned i=0;i<count;++i){
            double py,px;
            if(flash){const int radius=box?2:10;py=y+(oracle.random(radius*2)-radius);px=x+(oracle.random(radius*2)-radius);}
            else {py=y+(int(oracle.nextU32()%5)-2);px=x+(int(oracle.nextU32()%5)-2);}
            if(int(expected.size())==capacity){success=false;break;}
            expected.push_back({flash?224:225,px,py});
            if(flash){oracle.irandom(4);oracle.irandom(4);}
            else {oracle.nextU32();oracle.nextU32();oracle.nextU32();}
        }
        check(result==success && actual.size()==expected.size());
        for(unsigned i=0;i<actual.size();++i){
            check(actual[i].object==expected[i].object && actual[i].x==expected[i].x && actual[i].y==expected[i].y);
            ++births;
        }
        check(random.drawCount()==oracle.drawCount() && random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)check(random.stateWord(i)==oracle.stateWord(i));
    }
    // Synchronous children may mutate the caller. The repeat count remains
    // captured, but each subsequent child reads the current coordinates.
    downwell::SmokeBurstCaller caller;downwell::GameMakerRandom random(42),oracle(42);
    const unsigned count=4+oracle.irandom(2);unsigned emitted=0;
    check(downwell::smokeBurstPublished<5,1>(caller,number(10),number(20),random,[&](int,auto x,auto y){
        const double baseX=emitted?100:10,baseY=emitted?200:20;
        const double expectedY=baseY+int(oracle.nextU32()%5)-2;
        const double expectedX=baseX+int(oracle.nextU32()%5)-2;
        check(real(x)==expectedX&&real(y)==expectedY);
        caller.smx=number(100);caller.smy=number(200);caller.smokenum=0;++emitted;return true;
    })&&emitted==count&&caller.smokenum==0);
    // Flash reuses the very same caller fields after smoke. Child mutations
    // affect subsequent positions/radii, not the number of iterations.
    random=downwell::GameMakerRandom(71);oracle=downwell::GameMakerRandom(71);
    const unsigned flashCount=4+oracle.irandom(4);emitted=0;
    check(downwell::flashBurstPublished<6,2,10>(caller,number(10),number(20),random,[&](int object,auto x,auto y){
        const double baseX=emitted?100:10,baseY=emitted?200:20;
        const int radius=emitted?2:10;
        const double expectedY=baseY+oracle.random(radius*2)-radius;
        const double expectedX=baseX+oracle.random(radius*2)-radius;
        check(object==224&&real(x)==expectedX&&real(y)==expectedY);
        caller.smx=number(100);caller.smy=number(200);caller.pvary=2;caller.smokenum=0;
        random.irandomSmall(4);random.irandomSmall(4);oracle.irandom(4);oracle.irandom(4);
        ++emitted;return true;
    })&&emitted==flashCount&&caller.smokenum==0);
    check(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
    for(unsigned i=0;i<16;++i)check(random.stateWord(i)==oracle.stateWord(i));
    // Smoke overwrites the shared count/coordinates, but not flash's pvary.
    check(downwell::smokeBurstPublished<1,0>(caller,number(3),number(4),random,[](int,auto,auto){return true;}));
    check(caller.pvary==2&&caller.smokenum==1&&caller.vary==0);
    std::printf("Skeleton/box effect bursts: 24576 sequences, %u ordered child births passed\n",births);
}
