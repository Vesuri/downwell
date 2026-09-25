#include "core/EffectSurfacePlacement.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
using P=downwell::EffectSurfacePlacement;
using N=downwell::MotionBinary64;
static void check(bool b){if(!b)std::abort();}
static N number(double d){std::uint64_t u;std::memcpy(&u,&d,8);return {std::uint32_t(u>>32),std::uint32_t(u)};}
static bool same(N n,double d){auto r=number(d);return n.high==r.high && n.low==r.low;}
int main(){
    unsigned cases=0;
    const P::View view{number(-12.25),number(40.5),number(160),number(284)};
    for(double x:{-13.,-12.25,-12.25+0.000001,-12.25+0.00002,1.,3.,147.75-0.00002,147.75,148.})
    for(double y:{40.,40.5,40.5+0.000001,40.5+0.00002,99.,101.,324.5,325.})
    for(unsigned flags=0;flags<16;++flags){
        const bool death=flags&1,low=flags&2,exists=flags&4,noControl=flags&8;
        const bool visible=x+12.25>1e-5 && x-147.75 < -1e-5 && y-40.5>1e-5 && y-324.5 < -1e-5;
        P::Draw fx,shade;
        check(P::fx(number(x),number(y),view,death,low,exists,fx));
        check(fx.target==((death||low)?P::direct:(visible&&exists)?P::effects:P::hidden));
        if(fx.target==P::direct)check(same(fx.x,x)&&same(fx.y,y));
        if(fx.target==P::effects)check(same(fx.x,x+12.25)&&same(fx.y,y-40.5));
        check(P::enemyShade(number(x),number(y),view,low,noControl,exists,shade));
        check(shade.target==((!low&&!noControl&&exists&&visible)?P::shade:P::hidden));
        if(shade.target==P::shade)check(same(shade.x,std::nearbyint(x/2)*2+12.25)&&same(shade.y,std::nearbyint(y/2)*2-40.5));
        ++cases;
    }
    for(int i=-8192;i<=8192;++i){N result;const double value=i/4.;check(P::snapShade(number(value),result));check(same(result,std::nearbyint(value/2)*2+0.));}
    P::Draw unchanged{P::direct,number(4),number(5)};
    check(!P::fx({0x7ff00000u,0},{},view,false,false,true,unchanged));
    check(unchanged.target==P::direct && same(unchanged.x,4)&&same(unchanged.y,5));
    std::printf("Effect placement: %u flag/edge cases and 16385 signed snapping cases passed\n",cases);
}
