#include "core/EffectSurfacePlacement.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
using P=downwell::EffectSurfacePlacement;
using Q=downwell::Fixed16;
static void check(bool b){if(!b)std::abort();}
// Exact for every value used here (multiples of 1/65536 inside the range).
static Q number(double d){return Q::raw(std::int32_t(std::lround(d*65536.0)));}
static bool same(Q n,double d){return n.v==number(d).v;}
int main(){
    unsigned cases=0;
    // Independent double oracle: the runner's equality band is 1e-5, below
    // one Q16 step, so a Q16 coordinate is visible exactly when the reals
    // differ by more than the band.
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
        const int vx=int(next()%16383u)-8191,vy=int(next()%16383u)-8191;
        const int w=int(next()%1024u)-128,h=int(next()%1024u)-128;
        verify(vx+int(next()%1200u)-200,vy+int(next()%1200u)-200,vx,vy,w,h);
        verify(vx+(int(next()%(1200u*65536u))-200*65536)/65536.,vy+(int(next()%(1200u*65536u))-200*65536)/65536.,vx,vy,w,h);
    }
    for(double origin:{-16384.,-16383.,-8192.,-1.,0.,1.,8192.,16383.})
    for(double offset:{-1.,0.,1/65536.,2/65536.,159.,160.-2/65536.,160.-1/65536.,160.,161.}){
        verify(origin+offset,origin+1,origin,origin,160,284);
        verify(origin+1,origin+offset,origin,origin,284,160);
    }
    const P::View view{number(-12.25),number(40.5),number(160),number(284)};
    for(double x:{-13.,-12.25,-12.25+1/65536.,-12.25+2/65536.,1.,3.,147.75-1/65536.,147.75,148.})
    for(double y:{40.,40.5,40.5+1/65536.,40.5+2/65536.,99.,101.,324.5,325.})
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
    // round(x / 2) * 2, halves to even, against the double oracle.
    unsigned shadeCases=0;
    for(int i=-8192;i<=8192;++i){const double value=i/4.;check(same(P::snapShade(number(value)),std::nearbyint(value/2)*2));++shadeCases;}
    for(int i=-32767;i<=32767;++i){check(same(P::snapShade(number(i)),std::nearbyint(double(i)/2)*2));++shadeCases;}
    for(unsigned i=0;i<200000;++i){
        const Q q=Q::raw(std::int32_t(next()%(65534u*65536u))-32767*65536);
        check(same(P::snapShade(q),std::nearbyint(q.v/131072.)*2));++shadeCases;
    }
    for(int i=-16383;i<=16383;++i)check(P::snapShadePixel(i)==P::snapShade(Q::fromInteger(i)).v>>16);
    unsigned relativeCases=0;
    auto relative=[&](double a,double b){
        Q result;check(P::relative(number(a),number(b),result));
        check(same(result,a-b));++relativeCases;
    };
    for(unsigned i=0;i<100000;++i)
        relative(int(next()%32767u)-16383,int(next()%32767u)-16383);
    for(double a:{-16384.,-16383.,-8192.,-1.,0.,1.,8192.,16383.,0.25,-0.25})
    for(double b:{-16384.,-16383.,-8192.,-1.,0.,1.,8192.,16383.,0.25,-0.25})
        relative(a,b);
    P::PixelView pixels;
    check(P::pixelView({number(3),number(-4),number(160),number(284)},pixels)&&
          pixels.x==3&&pixels.y==-4&&pixels.width==160&&pixels.height==284);
    check(!P::pixelView({number(3.5),number(-4),number(160),number(284)},pixels));
    std::printf("Effect placement: %u flag/edge cases and %u snapping cases passed\n",cases,shadeCases);
    std::printf("Effect visibility: %u integer/fractional comparisons passed\n",visibilityCases);
    std::printf("Effect coordinates: %u relative cases passed\n",relativeCases);
}
