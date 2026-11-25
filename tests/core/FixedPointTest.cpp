// Q16 fixed point against the VM-exact binary64 forms it replaces.
#include "core/FixedPoint.h"
#include "core/WhiteJellyStep.h"
#include "core/RunnerPointDirection.h"
#include "core/NarrowAtan.h"
#include "core/EffectSpriteDraw.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace downwell;
static double toDouble(MotionBinary64 v){
    const std::uint64_t bits=(std::uint64_t(v.high)<<32)|v.low;double d;std::memcpy(&d,&bits,8);return d;
}
static MotionBinary64 fromDouble(double d){
    std::uint64_t bits;std::memcpy(&bits,&d,8);return MotionBinary64{std::uint32_t(bits>>32),std::uint32_t(bits)};
}
int main(){
    // Conversions: exact round trip of Q16 values, nearest for others.
    // High bits only: an LCG's low bits have short periods.
    std::uint32_t rng=12345;auto next=[&]{rng=rng*1664525u+1013904223u;return rng>>8;};
    for(int i=0;i<200000;++i){
        const std::int32_t raw=std::int32_t(next()<<8^next())>>(next()%16);
        Fixed16 back;assert(Fixed16::fromBinary64(Fixed16::raw(raw).binary64(),back)&&back.v==raw);
        assert(toDouble(Fixed16::raw(raw).binary64())==raw/65536.0);
        const double d=(std::int32_t(next()<<8^next())/65536.0)*(next()%3?1:1e-3);
        Fixed16 f;assert(Fixed16::fromBinary64(fromDouble(d),f));
        assert(std::fabs(f.v/65536.0-d)<=0.5/65536+1e-12);
    }
    Fixed16 f;assert(!Fixed16::fromBinary64(fromDouble(40000.0),f));
    // Trig against libm.
    double worstSine=0,worstAtan=0;
    for(std::int32_t a=-720*65536;a<720*65536;a+=977){
        worstSine=std::fmax(worstSine,std::fabs(FixedTrig::sine(a)/65536.0-std::sin(a/65536.0*M_PI/180)));
    }
    assert(worstSine<3e-5);
    for(int i=0;i<100000;++i){
        const std::int32_t dx=std::int32_t(next()%(600*65536))-300*65536,dy=std::int32_t(next()%(600*65536))-300*65536;
        double expected=std::atan2(-dy/65536.0,dx/65536.0)*180/M_PI;if(expected<0)expected+=360;
        double got=FixedTrig::pointDirection(dx,dy)/65536.0,diff=std::fabs(got-expected);
        if(diff>180)diff=360-diff;worstAtan=std::fmax(worstAtan,diff);
    }
    assert(worstAtan<2e-3);
    for(std::uint32_t d:{0xffffu,0x10000u,0x1ffffu,0xfffffffu,0x7fffffffu})
        assert(FixedTrig::atanRatio(d,d)==45*65536&&FixedTrig::atanRatio(0,d)==0);
    assert(FixedTrig::pointDirection(0,0)==0&&FixedTrig::pointDirection(0,-65536)==90*65536&&
        FixedTrig::pointDirection(0,65536)==270*65536&&FixedTrig::pointDirection(-65536,0)==180*65536&&
        FixedTrig::pointDirection(65536,0)==0);
    // lengthdir and point_direction against the retained VM-exact forms.
    double worstLength=0,worstDirection=0;
    for(int i=0;i<20000;++i){
        const MotionBinary64 speed=fromDouble((next()%3001)/1000.0),angle=fromDouble((next()%3600000)/10000.0);
        for(bool vertical:{false,true}){
            MotionBinary64 a,b;
            assert(WhiteJellyStep::movementComponent(vertical,speed,angle,a));
            assert(WhiteJellyStep::movementComponentReference(vertical,speed,angle,b));
            worstLength=std::fmax(worstLength,std::fabs(toDouble(a)-toDouble(b)));
        }
        const MotionBinary64 x=fromDouble(int(next()%240)+(next()%100)/100.0),y=fromDouble(int(next()%3000)+(next()%100)/100.0),
            tx=fromDouble(int(next()%240)+(next()%100)/100.0),ty=fromDouble(int(next()%3000)+(next()%100)/100.0);
        MotionBinary64 reference;
        if(!RunnerPointDirection::evaluate(x,y,tx,ty,reference,narrowAtan))continue;
        Fixed16 ax,ay,bx,by;
        assert(Fixed16::fromBinary64(x,ax)&&Fixed16::fromBinary64(y,ay)&&Fixed16::fromBinary64(tx,bx)&&Fixed16::fromBinary64(ty,by));
        double diff=std::fabs(FixedTrig::pointDirection(bx.v-ax.v,by.v-ay.v)/65536.0-toDouble(reference));
        if(diff>180)diff=360-diff;worstDirection=std::fmax(worstDirection,diff);
    }
    assert(worstLength<1e-3);
    assert(worstDirection<0.01);
    // drawFx rounds binary32 positions to pixels as MotionBinary64::roundPixel
    // (halves to even) inside smallPixel, refusing the rest.
    unsigned rounded=0;
    for(std::uint64_t bits=0;bits<0x100000000ull;bits+=4099){
        for(std::uint32_t near:{0u,1u,0x7fffffu}){
            const std::uint32_t b=std::uint32_t(bits)^near;float f;std::memcpy(&f,&b,4);
            std::int32_t a,r;
            const bool ok=downwell::EffectSpriteDraw::roundedPixel({b},a);
            const bool expected=std::isfinite(f)&&std::fabs(f)<float(1<<21)&&
                downwell::RunnerImageValue{b}.promoted().roundPixel(r);
            assert(ok==expected&&(!ok||a==r));++rounded;
        }
    }
    for(float f:{0.5f,1.5f,2.5f,-0.5f,-1.5f,0.49999997f,-2.5f,1048575.5f,2097151.f})
        {std::uint32_t b;std::memcpy(&b,&f,4);std::int32_t a,r;
         assert(downwell::EffectSpriteDraw::roundedPixel({b},a)&&downwell::RunnerImageValue{b}.promoted().roundPixel(r)&&a==r);}
    std::printf("Fixed point: sine %.2g, atan %.2g deg; lengthdir %.2g px, point_direction %.2g deg from the VM forms\n",
        worstSine,worstAtan,worstLength,worstDirection);
}
