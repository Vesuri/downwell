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
    // Independent scalar oracle across the integer fast-path range and its
    // boundaries. Fractional/epsilon neighbours must retain the real path.
    unsigned visibilityCases=0;
    std::uint32_t random=0x19c0ffeeu;
    auto next=[&](){random=random*1664525u+1013904223u;return random;};
    auto verify=[&](double x,double y,double vx,double vy,double w,double h){
        bool visible=false;
        check(P::inView(number(x),number(y),{number(vx),number(vy),number(w),number(h)},visible));
        check(visible==(x-vx>1e-5 && x-(vx+w)<-1e-5 && y-vy>1e-5 && y-(vy+h)<-1e-5));
        ++visibilityCases;
    };
    for(unsigned i=0;i<20000;++i){
        const int vx=int(next()%4194303u)-2097151,vy=int(next()%4194303u)-2097151;
        const int w=int(next()%1024u)-128,h=int(next()%1024u)-128;
        verify(vx+int(next()%1200u)-200,vy+int(next()%1200u)-200,vx,vy,w,h);
    }
    for(double origin:{-2097152.,-2097151.,-1048576.,-1.,-0.,0.,1.,1048576.,2097151.,2097152.})
    for(double offset:{-1.,0.,0.000001,0.00002,159.,160.-0.00002,160.-0.000001,160.,161.}){
        verify(origin+offset,origin+1,origin,origin,160,284);
        verify(origin+1,origin+offset,origin,origin,284,160);
    }
    for(N bad:{N{0x7ff00000u,0},N{0x7ff80000u,0},N{0,1}}){
        bool untouched=true;
        check(!P::inView(bad,{},{{},{},number(160),number(284)},untouched)&&untouched);
        check(!P::inView({},{},{{},{},bad,number(284)},untouched)&&untouched);
    }
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
    std::printf("Effect visibility: %u scalar integer/fractional comparisons passed\n",visibilityCases);
}
