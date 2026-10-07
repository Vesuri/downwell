#include "core/ContactCoordinates.h"
#include "core/RunnerPointDirection.h"
#include <cassert>
#include <cstdio>
#include <climits>
using namespace downwell;
static bool same(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
static MotionBinary64 world(int q,int origin){
    auto out=MotionBinary64::fromInteger(q);
    if(!out.zero())out.high-=8u<<20;
    assert(out.add(MotionBinary64::fromInteger(origin)));return out;
}
static unsigned accepted=0,rejected=0;
static void check(int px,int py,int cx,int cy,int ox,int oy){
    ContactCoordinates actual;
    if(!actual.prepare(px,py,cx,cy,ox,oy)){++rejected;return;}
    ++accepted;
    const auto x=world(px,ox),y=world(py,oy);
    auto a=MotionBinary64::fromInteger(cx),b=MotionBinary64::fromInteger(cy);
    assert(a.add(MotionBinary64::fromInteger(ox))&&b.add(MotionBinary64::fromInteger(oy)));
    assert(same(x,actual.playerX)&&same(y,actual.playerY));
    RunnerImageValue fx,fy,fa,fb,dx,dy;
    assert(fx.assign(x)&&fy.assign(y)&&fa.assign(a)&&fb.assign(b));
    auto rx=fx.promoted(),ry=fy.promoted(),nx=fa.promoted(),ny=fb.promoted();
    nx.high^=0x80000000u;ny.high^=0x80000000u;
    assert(rx.add(nx)&&ry.add(ny)&&dx.assign(rx)&&dy.assign(ry));
    assert(dx.bits==actual.dx.bits&&dy.bits==actual.dy.bits);
    // Same arguments, axis bypass and complete post-atan rounding program.
    unsigned calls=0;
    auto atan=[&](auto yy,auto xx,auto& output){
        assert(same(yy,dy.promoted())&&same(xx,dx.promoted()));
        output=MotionBinary64{0x3ff00000u,0};++calls;return true;
    };
    MotionBinary64 expected,direction;
    assert(RunnerPointDirection::evaluate(a,b,x,y,expected,atan));
    const auto referenceCalls=calls;
    assert(RunnerPointDirection::fromDifferences(actual.dx,actual.dy,direction,atan));
    assert(same(expected,direction)&&calls==2*referenceCalls);
}
int main(){
    const int edges[]={INT_MIN,-16777217,-16777216,-16777215,-257,-256,-1,0,1,255,256,
        16777215,16777216,16777217,INT_MAX};
    const int origins[]={INT_MIN,-65537,-65536,-176,-1,0,1,176,240,65536,65537,INT_MAX};
    for(int p:edges)for(int q:edges)for(int origin:origins)
        check(p,q,origin,0,origin,origin==INT_MIN?INT_MAX:-origin);
    for(int origin:origins)for(int caller:origins)
        check(0,1,caller,caller,origin,origin);
    for(int p:edges)for(int q:edges)check(p,q,65536,-65536,0,0);
    std::uint32_t random=0x5642f522;
    auto next=[&](){random=random*1664525u+1013904223u;return random;};
    for(unsigned i=0;i<100000;++i){
        int px=int(next()%33554433u)-16777216,py=int(next()%33554433u)-16777216;
        int cx=int(next()%131073u)-65536,cy=int(next()%131073u)-65536;
        int ox=int(next()%1025)-512,oy=int(next()%1025)-512;
        check(px,py,cx,cy,ox,oy);
        if(i<1024)check(px,py,px/256,py/256,0,0);
    }
    ContactCoordinates untouched;untouched.dx.bits=0x12345678;
    assert(!untouched.prepare(INT_MAX,0,0,0,INT_MAX,0)&&untouched.dx.bits==0x12345678);
    std::printf("Contact coordinates: %u exact coordinate/difference/direction cases; %u domain rejections\n",accepted,rejected);
}
