#include "core/SmokeDirection.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using downwell::MotionBinary64;
static void check(bool ok){if(!ok)std::abort();}
static MotionBinary64 number(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static double real(MotionBinary64 v){const std::uint64_t b=(std::uint64_t(v.high)<<32)|v.low;double d;std::memcpy(&d,&b,8);return d;}
static bool same(MotionBinary64 v,double d){auto n=number(d);return n.high==v.high && n.low==v.low;}
static double snap(float value){double integral;const float fraction=float(std::modf(double(value),&integral));return std::abs(fraction)<0.0001f?double(float(integral)):double(value);}
int main(){
    downwell::GameMakerRandom random(42);
    unsigned cases=0;
    auto run=[&](double length,double angle){
        const float speed=float(length),degrees=float(angle),scaled=degrees*float(3.14159265358979323846),radian=scaled/180.f;
        MotionBinary64 x,y;
        check(downwell::SmokeDirection::vector(number(length),number(angle),x,y,
            [&](MotionBinary64 input,MotionBinary64& cosine,MotionBinary64& sine){
                check(same(input,double(radian)));cosine=number(std::cos(real(input)));sine=number(std::sin(real(input)));return true;
            }));
        check(same(x,snap(float(std::cos(double(radian)))*speed)));
        check(same(y,snap(-(float(std::sin(double(radian)))*speed))));++cases;
    };
    for(unsigned i=0;i<100000;++i)run(random.random(2)+2,random.random(359));
    for(double angle:{0.,0.000001,45.,90.,180.,270.,359.})for(double speed:{0.,2.,3.,4.})run(speed,angle);
    for(float value:{-0.000101f,-0.000099f,0.000099f,0.000101f,0.99999f,1.00001f,-0.99999f,-1.00001f}){
        MotionBinary64 result;
        check(downwell::SmokeDirection::component({0x40800000u},number(double(value)/4),false,result));
        check(same(result,snap(value)));
    }
    std::printf("Smoke direction wrapper: %u host-trig cases and integer-snap boundaries passed\n",cases);
}
