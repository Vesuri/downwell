#include "core/SmokeBirth.h"
#include "core/SkeletonEffectBursts.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using downwell::MotionBinary64;
static void check(bool ok){if(!ok)std::abort();}
static MotionBinary64 number(double value){
    std::uint64_t bits;std::memcpy(&bits,&value,8);return {std::uint32_t(bits>>32),std::uint32_t(bits)};
}
static bool equal(MotionBinary64 value,double expected){
    const auto b=number(expected);return value.high==b.high && value.low==b.low;
}
static std::uint32_t floatBits(float value){std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
int main(){
    downwell::GameMakerRandom samples(42);
    for(unsigned i=0;i<100000;++i){
        // Broad normal exponents, signs and mantissas; products stay normal.
        MotionBinary64 a{(samples.nextU32()&0x800fffffu)|((700+samples.nextU32()%600)<<20),samples.nextU32()};
        MotionBinary64 b{(samples.nextU32()&0x800fffffu)|((700+samples.nextU32()%600)<<20),samples.nextU32()};
        auto scalar=[](MotionBinary64 x){std::uint64_t bits=(std::uint64_t(x.high)<<32)|x.low;double d;std::memcpy(&d,&bits,8);return d;};
        MotionBinary64 result;
        check(downwell::multiplyBinary64(a,b,result) && equal(result,scalar(a)*scalar(b)));
    }
    for (double a:{0.0,-0.0,1.0,-1.0,2.0,0.5,0.9}) for(double b:{0.0,-0.0,1.0,-1.0,2.0,0.5,0.9}) {
        MotionBinary64 result;check(downwell::multiplyBinary64(number(a),number(b),result) && equal(result,a*b));
    }
    // Exact halfway products exercise both odd/even rounding directions.
    for (unsigned ulps:{1u,3u,5u,7u}) {
        const double a=1+std::ldexp(double(ulps),-52);
        MotionBinary64 result;
        check(downwell::multiplyBinary64(number(a),number(1.5),result) && equal(result,a*1.5));
    }
    for (MotionBinary64 invalid: {MotionBinary64{0,1},MotionBinary64{0x7ff00000u,0},MotionBinary64{0x7ff80000u,0}}) {
        auto result=number(7);check(!downwell::multiplyBinary64(invalid,number(2),result) && equal(result,7));
    }
    auto result=number(7);
    check(!downwell::multiplyBinary64({0x00100000u,0},number(0.5),result) && equal(result,7));
    check(!downwell::multiplyBinary64({0x7fefffffu,0xffffffffu},number(2),result) && equal(result,7));
    unsigned births=0;
    for(unsigned seed=0;seed<1024;++seed){
        downwell::GameMakerRandom rng(seed),oracle(seed);
        const unsigned count=4+oracle.irandom(2);
        unsigned seen=0;
        const double x=96.123456789,y=-192.987654321;
        check(downwell::skeletonSmokeBurst(number(x),number(y),rng,[&](int object,MotionBinary64 px,MotionBinary64 py){
            const double oy=y+(int(oracle.nextU32()%5)-2),ox=x+(int(oracle.nextU32()%5)-2);
            downwell::SmokeBirth smoke;
            check(object==225 && smoke.create(px,py,rng));
            const double speed=oracle.random(2)+2;
            const double rate=oracle.random(0.13-0.05)+0.05;
            const double direction=oracle.random(359);
            check(equal(smoke.xx,double(float(ox))) && equal(smoke.yy,double(float(oy))));
            check(equal(smoke.speed,speed) && equal(smoke.direction,direction));
            check(smoke.imageSpeed.bits==floatBits(float(speed*rate)) && smoke.noCollision && !smoke.allSet);
            ++seen;++births;return true;
        }));
        check(seen==count && rng.drawCount()==oracle.drawCount() && rng.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)check(rng.stateWord(i)==oracle.stateWord(i));
        // The later first-Step draw must occur after the entire burst's Creates.
        downwell::SmokeBirth first;first.speed=number(3);
        check(first.sampleImageSpeed(rng));
        check(first.imageSpeed.bits==floatBits(float(3*(oracle.random(0.13-0.05)+0.05))));
        check(rng.drawCount()==oracle.drawCount());
    }
    std::printf("Smoke Create: %u synchronous births and 100000 binary64 products passed\n",births);
}
