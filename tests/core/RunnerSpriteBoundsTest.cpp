#include "core/RunnerSpriteBounds.h"
#include "core/RunnerUnitBoundsVerification.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
using Bounds=downwell::RunnerSpriteBounds;
using Real=Bounds::Real;
Real packed(float x){Real r;std::memcpy(&r.bits,&x,4);return r;}
float unpacked(Real r){float x;std::memcpy(&x,&r.bits,4);return x;}
int main(){
    assert(downwell::RunnerUnitBoundsVerification::run());
    {
        std::uint32_t seed=0x25468u;
        auto random=[&](){seed=seed*1664525u+1013904223u;return seed;};
        for(unsigned i=0;i<8192;++i){
            auto scalar=[&](){return int(random()&65535u)-32768;};
            downwell::RunnerUnitBoundsVerification::Resource resource{scalar(),scalar(),scalar(),scalar(),scalar(),scalar()};
            const float x=float(int(random()&0x7fffffu)-0x400000),y=float(int(random()&0x7fffffu)-0x400000);
            const auto sx=packed(i%7?1.f:-1.f),sy=packed(i%11?(i&1?-1.f:1.f):0.5f);
            auto px=packed(x),py=packed(y);
            if(i%13==0)px=packed(x+0.5f);
            if(i%17==0)py.bits=0x80000000u;
            if(i%19==0)resource.originX=40000; // Gate fallback.
            for(bool compatibility:{false,true}){
                Bounds::Box fast{},reference{};unsigned aCalls=0,bCalls=0;
                auto round=[](Real v,std::int32_t& out){return v.promoted().roundPixel(out);};
                const bool a=Bounds::fromSprite(resource,px,py,sx,sy,false,{},{},compatibility,
                    [&](Real v,std::int32_t& p){++aCalls;return round(v,p);},fast);
                const bool b=Bounds::fromSpriteBinary32(resource,px,py,sx,sy,false,{},{},compatibility,
                    [&](Real v,std::int32_t& p){++bCalls;return round(v,p);},reference);
                assert(a==b&&aCalls==bCalls&&fast.left.bits==reference.left.bits&&fast.right.bits==reference.right.bits&&
                    fast.top.bits==reference.top.bits&&fast.bottom.bits==reference.bottom.bits);
            }
        }
    }
    {
        // The unit-scale integer axis against its binary32 oracle: margins
        // across and beyond the gate, any finite position, both modes.
        std::uint32_t seed=0x9e3779b9u;
        auto random=[&](){seed=seed*1664525u+1013904223u;return seed;};
        auto margin=[&]()->Real{
            switch(random()%6){
            case 0:return {};
            case 1:return Real{0x80000000u};
            case 2:return packed(float(int(random()%4194305u)-2097152)); // to +/-2^22
            case 3:return packed(float(int(random()%2001u)-1000)+(random()&1?0.5f:0.f));
            default:return packed(float(int(random()%129u)-64));
            }
        };
        auto position=[&]()->Real{
            switch(random()%4){
            case 0:return Real{random()&0x80000000u}; // signed zero
            case 1:{Real r{random()};if(((r.bits>>23)&255)==255)r.bits^=0x40000000u;return r;}
            case 2:return Real{(random()&0x80000000u)|((random()%40u+107u)<<23)|(random()&0x7fffffu)};
            default:return packed(float(int(random()%4001u)-2000)+float(random()%1024u)/1024.f);
            }
        };
        unsigned fastAxes=0;
        for(unsigned i=0;i<4000000;++i){
            const Real low=margin(),high=margin(),pos=position();
            const int origin=i%23==0?int(random()%80000u)-40000:int(random()%201u)-100;
            const Real scale=i%29==0?packed(-1.f):packed(1.f);
            const bool compatibility=i&1;
            fastAxes+=scale.bits==0x3f800000u&&low.bits!=0x80000000u&&
                origin>=-32768&&origin<=32767&&std::fabs(unpacked(low))<2097152.f&&
                unpacked(low)==std::floor(unpacked(low))&&unpacked(high)==std::floor(unpacked(high))&&
                std::fabs(unpacked(high))<2097152.f;
            Real n1,f1,n2,f2;unsigned c1=0,c2=0;
            auto r1=[&](Real v,std::int32_t& out){++c1;return v.promoted().roundPixel(out);};
            auto r2=[&](Real v,std::int32_t& out){++c2;return v.promoted().roundPixel(out);};
            const bool a=Bounds::axis(low,high,origin,pos,scale,compatibility,r1,n1,f1);
            const bool b=Bounds::axisBinary32(low,high,origin,pos,scale,compatibility,r2,n2,f2);
            if(a!=b||c1!=c2||(a&&(n1.bits!=n2.bits||f1.bits!=f2.bits))){
                std::printf("axis mismatch low=%08x high=%08x origin=%d pos=%08x compat=%d: %d %08x %08x / %d %08x %08x\n",
                    unsigned(low.bits),unsigned(high.bits),origin,unsigned(pos.bits),int(compatibility),
                    int(a),unsigned(n1.bits),unsigned(f1.bits),int(b),unsigned(n2.bits),unsigned(f2.bits));
                return 1;
            }
        }
        std::printf("Unit-scale integer axis: 4000000 axes match binary32 (%u on the integer path)\n",fastAxes);
    }
    auto round=[](Real value,std::int32_t& result){return value.promoted().roundPixel(result);};
    unsigned comparisons=0,roundCalls=0,reassociatedDifferences=0;
    auto counted=[&](Real value,std::int32_t& result){++roundCalls;return round(value,result);};
    // Independent host arithmetic transcribes the SSE instruction order.
    auto axis=[](float low,float high,int origin,float pos,float scale,bool compatibility){
        float width=high-low;width=width+1.f;
        float a=low-float(origin);a=a*scale;
        float b=scale*width;a=a+pos;b=b+a;
        if(compatibility){a=std::nearbyint(a);b=std::nearbyint(b);}
        if(a>b)std::swap(a,b);
        if(compatibility)b=b-1.f;
        return std::pair<float,float>{a,b};
    };
    for(int i=0;i<8192;++i)for(bool compat:{false,true}){
        const float scales[]={-2.3f,-1.f,-0.125f,0.f,0.3f,1.f,1.5f,3.7f};
        const float x=float(i-4096)*0.31f,y=float(1000-i)*0.17f;
        const float sx=scales[i%8],sy=scales[(i/8)%8];
        const int ox=i%31-15,oy=i%19-9;
        Bounds::Box result;
        assert(Bounds::unrotated({packed(10),packed(11),packed(13),packed(13)},
            ox,oy,packed(x),packed(y),packed(sx),packed(sy),compat,counted,result));
        const auto horizontal=axis(10,13,ox,x,sx,compat),vertical=axis(11,13,oy,y,sy,compat);
        if(!compat && sx>0){
            float independent=(14.f-float(ox))*sx;independent=independent+x;
            if(independent!=horizontal.second)++reassociatedDifferences;
        }
        assert(unpacked(result.left)==horizontal.first&&unpacked(result.right)==horizontal.second);
        assert(unpacked(result.top)==vertical.first&&unpacked(result.bottom)==vertical.second);
        ++comparisons;
    }
    assert(roundCalls==8192*4);
    assert(reassociatedDifferences>0); // Fixtures reject independent far-edge calculation.
    // Full signed resource fields exercise nearest-even integer narrowing,
    // including values outside binary32's exact-integer range. No pixel-round
    // callback is used here, so large finite bounds remain admitted.
    {
        std::uint32_t seed=0x348712abu;
        auto integer=[&](){seed=seed*1664525u+1013904223u;
            std::int32_t value;std::memcpy(&value,&seed,4);return value;};
        for(unsigned i=0;i<4096;++i){
            downwell::RunnerUnitBoundsVerification::Resource r{integer(),integer(),integer(),integer(),integer(),integer()};
            if(i==0)r={-2147483647-1,2147483647,16777217,-16777217,16777219,-16777219};
            const float sx=i&1?-0.5f:1.5f,sy=i&2?-1.f:0.25f;
            Bounds::Box actual;
            assert(Bounds::fromSprite(r,packed(17.5f),packed(-23.25f),packed(sx),packed(sy),false,{},{},false,round,actual));
            const auto h=axis(float(r.marginLeft),float(r.marginRight),r.originX,17.5f,sx,false);
            const auto v=axis(float(r.marginTop),float(r.marginBottom),r.originY,-23.25f,sy,false);
            assert(unpacked(actual.left)==h.first&&unpacked(actual.right)==h.second&&
                   unpacked(actual.top)==v.first&&unpacked(actual.bottom)==v.second);
        }
    }
    unsigned rotatedComparisons=0;
    for(unsigned i=0;i<4096;++i)for(bool compat:{false,true}){
        const float angle=float(i)*0.013f,C=float(std::cos(double(angle))),S=float(std::sin(double(angle)));
        float left=10,right=13,top=11,bottom=13;
        if(i&1)std::swap(left,right);
        if(i&2)std::swap(top,bottom);
        const float sx=float(int(i%17)-8)*0.3f,sy=float(int(i%23)-11)*0.17f;
        const float x=float(int(i)-2048)*0.31f,y=float(int(i)-1000)*0.73f;
        Bounds::Box result;
        assert(Bounds::rotated({packed(left),packed(top),packed(right),packed(bottom)},12,12,
            packed(x),packed(y),packed(sx),packed(sy),packed(C),packed(S),compat,round,result));
        const float l=(std::min(left,right)-12.f)*sx,r=((std::max(left,right)-12.f)+1.f)*sx;
        const float t=(std::min(top,bottom)-12.f)*sy,b=((std::max(top,bottom)-12.f)+1.f)*sy;
        const float cl=C*l,cr=C*r,st=S*t,sb=S*b,ct=C*t,cb=C*b,sl=S*l,sr=S*r;
        float expected[]={(x+std::min(cl,cr))+std::min(st,sb),(y+std::min(ct,cb))-std::max(sl,sr),
                          (x+std::max(cl,cr))+std::max(st,sb),(y+std::max(ct,cb))-std::min(sl,sr)};
        if(compat)for(unsigned edge=0;edge<4;++edge)
            expected[edge]=float(int(std::nearbyint(expected[edge]))-(edge>=2?1:0));
        const Real actual[]={result.left,result.top,result.right,result.bottom};
        for(unsigned edge=0;edge<4;++edge)assert(unpacked(actual[edge])==expected[edge]);
        ++rotatedComparisons;
    }
    Bounds::Box collapsed;
    assert(Bounds::unrotated({},0,0,packed(0.5f),packed(-0.5f),{}, {},true,round,collapsed));
    assert(unpacked(collapsed.left)==0&&unpacked(collapsed.top)==0);
    assert(unpacked(collapsed.right)==-1&&unpacked(collapsed.bottom)==-1);
    Bounds::Box unchanged{packed(1),packed(2),packed(3),packed(4)},result=unchanged;
    auto fail=[](Real,std::int32_t&){return false;};
    assert(!Bounds::unrotated({},0,0,{}, {},packed(1),packed(1),true,fail,result));
    assert(std::memcmp(&result,&unchanged,sizeof(result))==0);
    assert(!Bounds::rotated({},0,0,{}, {},packed(1),packed(1),packed(1),{},true,fail,result));
    assert(std::memcmp(&result,&unchanged,sizeof(result))==0);
    assert(!Bounds::unrotated({},0,0,Real{0x7fc00000}, {},packed(1),packed(1),false,round,result));
    assert(std::memcmp(&result,&unchanged,sizeof(result))==0);
    std::printf("Unrotated sprite bounds: %u comparisons passed; %u distinguish reassociation\n",comparisons,reassociatedDifferences);
    std::printf("Rotated sprite bounds: %u mirrored/fractional/reversed-margin comparisons passed\n",rotatedComparisons);
}
