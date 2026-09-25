#include "core/SmokeDistance.h"
#include "core/SmokeMotion.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok){if(!ok)std::abort();}
static downwell::MotionBinary64 number(double x){std::uint64_t b;std::memcpy(&b,&x,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static float value(std::uint32_t bits){float x;std::memcpy(&x,&bits,4);return x;}
static bool oracle(float sx,float sy,float x,float y){
    const float dx=float(double(x)-double(sx)),dy=float(double(y)-double(sy));
    const float xx=dx*dx,yy=dy*dy,sum=xx+yy;
    const float distance=float(std::sqrt(double(sum)));
    return double(distance)-16>1e-5;
}
int main(){
    // The float sqrt acceptance threshold: both sides and the exact adjacent
    // squared-distance values, independently through host sqrt and VM epsilon.
    for(std::uint32_t b=0x437fff00;b<0x43800100;++b)
        check((double(float(std::sqrt(double(value(b)))))-16>1e-5)==(b>=0x4380000c));
    unsigned cases=0;
    auto run=[&](float sx,float sy,float x,float y){
        bool escaped=false;
        check(downwell::smokeEscapedDistance(number(sx),number(sy),number(x),number(y),escaped));
        check(escaped==oracle(sx,sy,x,y));++cases;
    };
    for(int sign:{-1,1})for(std::uint32_t b=0x417fff80;b<0x41800080;++b)
        for(float lateral:{0.f,0.001f,0.01f,0.1f,1.f})run(0,0,sign*value(b),lateral);
    downwell::GameMakerRandom random(42);
    for(unsigned i=0;i<100000;++i){
        const float sx=float(int(random.nextU32()%200000)-100000)/8;
        const float sy=float(int(random.nextU32()%200000)-100000)/8;
        const float dx=float(int(random.nextU32()%400000)-200000)/8192;
        const float dy=float(int(random.nextU32()%400000)-200000)/8192;
        run(sx,sy,sx+dx,sy+dy);
    }
    bool unchanged=true;
    check(!downwell::smokeEscapedDistance(number(0.1),{}, {}, {},unchanged) && unchanged);
    std::printf("Smoke distance: %u coordinate cases and 512 sqrt-boundary floats passed\n",cases);
}
