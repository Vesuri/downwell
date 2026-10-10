#include "core/RunnerDirectionValue.h"
#ifdef DOWNWELL_FREESTANDING
bool nativeDirectionValue(downwell::MotionBinary64 input,downwell::RunnerImageValue& output){
    return downwell::RunnerDirectionValue::normalize(input,output);
}
#else
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 real(double v){
    std::uint64_t bits;std::memcpy(&bits,&v,8);
    return {std::uint32_t(bits>>32),std::uint32_t(bits)};
}
static void check(double input){
    const float narrowed=float(input);
    float expected=std::fmod(narrowed,360.0f);
    if(expected<0)expected+=360.0f;
    std::uint32_t bits;std::memcpy(&bits,&expected,4);
    RunnerImageValue actual;assert(RunnerDirectionValue::normalize(real(input),actual)&&actual.bits==bits);
}
int main(){
    const double edges[]={0.0,-0.0,360,-360,720,-720,361,-1,359.9999999,
        -0.000001,360.000001,16777216.5,-16777216.5,1e30,-1e30};
    for(double v:edges)check(v);
    std::uint32_t random=91;
    for(unsigned i=0;i<100000;){
        random=random*1664525u+1013904223u;
        const unsigned exponent=(random>>23)&255u;
        if(!exponent||exponent==255)continue;
        ++i;
        float value;std::memcpy(&value,&random,4);check(double(value));
    }
    RunnerImageValue output{0x3f800000u};
    assert(!RunnerDirectionValue::normalize({0x7ff00000u,0},output)&&output.bits==0x3f800000u);
    assert(RunnerDirectionValue::normalize(real(-0.000001),output)&&output.bits==0x43b40000u);
    assert(RunnerDirectionValue::normalize(real(-360),output)&&output.bits==0x80000000u);
    std::puts("Direction value: narrow-before-remainder, signed zero, negative adjustment and 100000 finite patterns passed");
}
#endif
