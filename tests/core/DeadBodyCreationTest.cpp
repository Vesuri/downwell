#include "core/DeadBodyCreation.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static void equal(MotionBinary64 a,double d){const auto b=real(d);assert(a.high==b.high&&a.low==b.low);}
struct Events {
    DeadBodySource& source;
    unsigned calls=0,fail=0;
    bool mutate=false;
    MotionBinary64 vertical{},horizontal{},scale{};
    bool next(unsigned expected){assert(++calls==expected);return calls!=fail;}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& child){
        assert(object==263);equal(x,12.25);equal(y,-3.5);
        if(!next(1))return false;
        // Simulate synchronous child Create modifying caller state.
        if(mutate){source.impactSp=real(2);source.imageXscale=real(-1);source.xsp=real(0.25);source.bodySprite=207;}
        child=42;return true;
    }
    bool sprite(unsigned h,int id){assert(h==42&&id==(mutate?207:206));return next(2);}
    bool verticalSpeed(unsigned h,MotionBinary64 v){assert(h==42);vertical=v;return next(3);}
    bool facing(unsigned h,MotionBinary64 v){assert(h==42);scale=v;return next(4);}
    bool explosion(unsigned h,int id){assert(h==42&&id==102);return next(5);}
    bool horizontalSpeed(unsigned h,MotionBinary64 v){assert(h==42);horizontal=v;return next(6);}
};
int main(){
    unsigned cases=0;
    for(double impact:{-4.,-0.00002,-0.000001,0.,0.000001,0.00002,1.,4.,4.000001,4.00002,9.})
    for(double vx:{-2.,-0.50002,-0.5,-0.499999,-0.49998,-0.1,0.,0.1,0.49998,0.499999,0.5,2.})
    for(double facing:{-1.,0.,1.})for(bool mutate:{false,true}){
        DeadBodySource source;source.x=real(12.25);source.y=real(-3.5);source.xsp=real(vx);source.imageXscale=real(facing);
        Events events{source};events.mutate=mutate;
        assert(createDeadBody(source,206,real(impact),102,events));assert(events.calls==6);
        double capped=impact-4>1e-5?4:impact,speed=vx,scale=facing;
        if(mutate){capped=2;speed=.25;scale=-1;}
        if(std::abs(speed)-.5 < -1e-5)speed=scale*1.5;
        const double expected=std::abs(capped)>1e-5?speed*capped:(speed>0?.5:speed<0?-.5:0.);
        equal(source.xsp,speed);equal(source.impactSp,capped);
        equal(events.vertical,capped);equal(events.scale,scale);equal(events.horizontal,expected);
        assert(source.myDeadBody==42);++cases;
    }
    for(unsigned fail=1;fail<=6;++fail){
        DeadBodySource source;source.x=real(12.25);source.y=real(-3.5);source.imageXscale=real(1);
        Events events{source};events.fail=fail;
        assert(!createDeadBody(source,206,real(9),102,events));assert(events.calls==fail);
        assert(source.myDeadBody==(fail==1?0u:42u));
        equal(source.impactSp,4);equal(source.xsp,fail==6?1.5:0);
    }
    std::printf("Corpse wrapper: %u scalar/branch/mutation cases and six failure boundaries passed\n",cases);
}
