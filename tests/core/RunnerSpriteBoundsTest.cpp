#include "core/RunnerSpriteBounds.h"
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
    Bounds::Box collapsed;
    assert(Bounds::unrotated({},0,0,packed(0.5f),packed(-0.5f),{}, {},true,round,collapsed));
    assert(unpacked(collapsed.left)==0&&unpacked(collapsed.top)==0);
    assert(unpacked(collapsed.right)==-1&&unpacked(collapsed.bottom)==-1);
    Bounds::Box unchanged{packed(1),packed(2),packed(3),packed(4)},result=unchanged;
    auto fail=[](Real,std::int32_t&){return false;};
    assert(!Bounds::unrotated({},0,0,{}, {},packed(1),packed(1),true,fail,result));
    assert(std::memcmp(&result,&unchanged,sizeof(result))==0);
    assert(!Bounds::unrotated({},0,0,Real{0x7fc00000}, {},packed(1),packed(1),false,round,result));
    assert(std::memcmp(&result,&unchanged,sizeof(result))==0);
    std::printf("Unrotated sprite bounds: %u comparisons passed; %u distinguish reassociation\n",comparisons,reassociatedDifferences);
}
