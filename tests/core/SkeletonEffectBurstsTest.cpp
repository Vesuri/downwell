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
    for(unsigned seed=0;seed<1024;++seed) for(bool flash:{false,true})
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
        bool result=flash?downwell::skeletonFlashBurst(number(x),number(y),random,create):
                          downwell::skeletonSmokeBurst(number(x),number(y),random,create);
        // Independent 64-bit modulus and ordinary host-double arithmetic.
        const unsigned count=4+unsigned(oracle.irandom(flash?4:2));
        bool success=true;
        for(unsigned i=0;i<count;++i){
            double py,px;
            if(flash){py=y+(oracle.random(20)-10);px=x+(oracle.random(20)-10);}
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
    std::printf("Skeleton effect bursts: 6144 sequences, %u ordered child births passed\n",births);
}
