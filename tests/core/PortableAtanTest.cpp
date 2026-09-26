#include "core/PortableAtan.h"
#include "core/RunnerImageValue.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 number(double d){unsigned long long n;std::memcpy(&n,&d,8);return {unsigned(n>>32),unsigned(n)};}
static unsigned bits(float f){unsigned n;std::memcpy(&n,&f,4);return n;}
int main(){
    GameMakerRandom random(42);
    for(unsigned i=0;i<100000;++i){
        double a=std::ldexp(random.random(2)+1,int(i%100)-50),b=std::ldexp(random.random(2)+1,int(i%71)-35);
        if(i&1)a=-a;
        if(i&2)b=-b;
        MotionBinary64 actual;assert(divideBinary64(number(a),number(b),actual));
        const auto expected=number(a/b);assert(actual.high==expected.high && actual.low==expected.low);
    }
    unsigned checked=0;
    auto run=[&](float y,float x){
        MotionBinary64 result;RunnerImageValue narrowed;
        assert(portableAtan(number(y),number(x),result) && narrowed.assign(result));
        assert(narrowed.bits==bits(float(std::atan2(double(y),double(x)))));++checked;
    };
    for(unsigned i=0;i<20000;++i)run(float(random.random(2000)-1000),float(random.random(2000)-1000));
    for(float y:{-1.f,-0.f,0.f,1.f})for(float x:{-1.f,-0.f,0.f,1.f})run(y,x);
    for(int exponent=-120;exponent<=120;++exponent){
        const float v=std::ldexp(1.25f,exponent);run(v,1);run(-v,1);run(v,-1);run(1,v);
    }
    MotionBinary64 unchanged=number(7);assert(!divideBinary64(number(1),{},unchanged));
    assert(unchanged.high==number(7).high && unchanged.low==number(7).low);
    std::printf("Portable atan candidate: %u float-result comparisons; 100000 exact normal division cases passed\n",checked);
}
