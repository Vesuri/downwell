#include "core/RunnerMaskPoint.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static RunnerImageValue real(float value){RunnerImageValue r;std::memcpy(&r.bits,&value,4);return r;}
static float value(RunnerImageValue r){float f;std::memcpy(&f,&r.bits,4);return f;}
int main(){
    unsigned calls=0,cases=0;
    auto trig=[&](RunnerImageValue a,RunnerImageValue& c,RunnerImageValue& s){
        ++calls;
        const float radians=(-value(a)*value({0x40490fdb}))/180.0F;
        c=real(float(std::cos(double(radians))));s=real(float(std::sin(double(radians))));return true;
    };
    const std::uint8_t bytes[]={0xa5,0x40,0x5a,0x80,0xff,0xc0};
    const SpriteMaskPack::Mask mask{bytes,10,3,2};
    for(float angle:{0.0F,value({0x38d1b716}),value({0x38d1b717}),-90.0F,31.0F,180.0F})
    for(float sx:{-2.0F,-1.0F,0.3F,1.0F,2.0F})
    for(float sy:{-1.0F,0.7F,1.0F,2.0F})
    for(int px=-9;px<=9;++px)for(int py=-5;py<=5;++py){
        RunnerMaskPoint::Shape shape{3,-2,4,1,real(sx),real(sy),real(angle)};
        float dx=float(px)-(float(shape.x)-0.5F),dy=float(py)-(float(shape.y)-0.5F);
        const bool rotate=std::fabs(angle)>=value({0x38d1b717});
        if(rotate){
            RunnerImageValue c,s;trig(shape.angle,c,s);
            const float first=dy*value(s)+dx*value(c),second=dy*value(c)-dx*value(s);
            dx=first;dy=second;
        }
        const float expectedX=dx/sx+float(shape.originX),expectedY=dy/sy+float(shape.originY);
        RunnerImageValue x{0xdeadbeef},y{0xdeadbeef};const auto previous=calls;
        assert(RunnerMaskPoint::sample(shape,px,py,trig,x,y));
        assert(calls-previous==unsigned(rotate));
        assert(x.bits==real(expectedX).bits&&y.bits==real(expectedY).bits);
        bool hit=false;assert(RunnerMaskPoint::contains(shape,mask,px,py,trig,hit));
        assert(hit==mask.contains(int(std::floor(expectedX)),int(std::floor(expectedY))));++cases;
    }
    RunnerMaskPoint::Shape mirrored;mirrored.scaleX=real(-1);
    bool hit=true;
    assert(RunnerMaskPoint::contains(mirrored,mask,0,0,trig,hit)&&!hit); // -0.5 floors to -1, not zero.
    RunnerMaskPoint::Shape invalid;invalid.scaleX={};
    RunnerImageValue x{123},y{456};
    assert(!RunnerMaskPoint::sample(invalid,0,0,trig,x,y)&&x.bits==123&&y.bits==456);
    assert(!RunnerMaskPoint::contains(invalid,mask,0,0,trig,hit)&&!hit);
    invalid.scaleX=real(1);invalid.angle=real(90);
    assert(!RunnerMaskPoint::sample(invalid,0,0,[](auto,auto&,auto&){return false;},x,y));
    std::printf("Point-mask source-order sampling: %u cases, angle boundary, mirrored floor and failure preservation passed\n",cases);
}
