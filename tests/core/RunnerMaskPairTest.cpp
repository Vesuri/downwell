#include "core/RunnerMaskPair.h"
#include "core/SpriteCollisionPack.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>
#include <cstdio>
using namespace downwell;
using Pair=RunnerMaskPair;
using Real=RunnerImageValue;
Real bits(float x){Real n;std::memcpy(&n.bits,&x,4);return n;}
float value(Real n){float x;std::memcpy(&x,&n.bits,4);return x;}
std::vector<std::uint8_t> read(const char* path){
    std::ifstream file(path,std::ios::binary);assert(file);
    return {(std::istreambuf_iterator<char>(file)),{}};
}
bool trig(Real angle,Real& c,Real& s){
    float radians=-value(angle)*float(3.14159265358979323846);
    radians=radians/180.f;
    c=bits(float(std::cos(double(radians))));s=bits(float(std::sin(double(radians))));return true;
}
bool roundPixel(Real x,std::int32_t& out){return x.promoted().roundPixel(out);}
// Separate host arithmetic transcription. Source trig is injected identically
// into each implementation; this test does not establish runtime trig/FP mode.
bool referenceSample(const Pair::Shape& a,int path,int x,int y){
    if(path==0){
        const auto px=std::uint32_t(a.originX)-std::uint32_t(std::int32_t(std::nearbyint(value(a.x))))+std::uint32_t(x);
        const auto py=std::uint32_t(a.originY)-std::uint32_t(std::int32_t(std::nearbyint(value(a.y))))+std::uint32_t(y);
        return px<a.mask.width&&py<a.mask.height&&a.mask.contains(int(px),int(py));
    }
    const float sx=value(a.transform.scaleX),sy=value(a.transform.scaleY);
    const float ax=sx<0?value(a.x)-1.f:value(a.x),ay=sy<0?value(a.y)-1.f:value(a.y);
    const float dx=float(x)-ax,dy=float(y)-ay,rx=1.f/sx,ry=1.f/sy;
    float px,py;
    if(path==1){px=dx*rx;py=dy*ry;}
    else{
        Real c,s;assert(trig(a.transform.angle,c,s));
        px=(dy*value(s))+(dx*value(c));py=(dy*value(c))-(dx*value(s));
        px=px*rx;py=py*ry;
    }
    px=px+float(a.originX);py=py+float(a.originY);
    if(path==2&&(px<0||py<0||px>=float(a.mask.width)||py>=float(a.mask.height)))return false;
    return a.mask.contains(int(px),int(py));
}
Pair::Result reference(const Pair::Shape& a,const Pair::Shape& b,unsigned& path){
    if(!a.frameCount||!b.frameCount)return {};
    const float scales[]={value(a.transform.scaleX),value(a.transform.scaleY),value(b.transform.scaleX),value(b.transform.scaleY)};
    bool unit=true;
    for(float scale:scales){if(scale==0)return {};unit&=scale>=float(1.-1e-5)&&scale<=float(1.+1e-5);}
    path=(std::fabs(double(value(a.transform.angle)))>1e-5||std::fabs(double(value(b.transform.angle)))>1e-5)?2:unit?0:1;
    const int left=int(std::max(value(a.bounds.left),value(b.bounds.left)));
    const int right=int(std::min(value(a.bounds.right),value(b.bounds.right)));
    const int top=int(std::max(value(a.bounds.top),value(b.bounds.top)));
    const int bottom=int(std::min(value(a.bounds.bottom),value(b.bounds.bottom)));
    for(int x=left;x<=right;++x)for(int y=top;y<=bottom;++y)
        if(referenceSample(a,path,x,y)&&referenceSample(b,path,x,y))return {true,x,y};
    return {};
}
int main(int argc,char** argv){
    assert(argc==3);auto maskBytes=read(argv[1]),spriteBytes=read(argv[2]);
    SpriteMaskPack masks(maskBytes.data(),maskBytes.size());
    SpriteCollisionPack sprites(spriteBytes.data(),spriteBytes.size());
    assert(masks.valid()&&sprites.valid());
    RunnerMaskDispatch dispatch;
    unsigned paths[3]={},hits=0,misses=0;
    for(unsigned i=0;i<4446;++i){
        Pair::Shape a,b;
        const unsigned ids[]={i%741, i%2?597u:616u};
        Pair::Shape* shapes[]={&a,&b};
        for(unsigned j=0;j<2;++j){
            auto& s=*shapes[j];PackedSpriteCollision resource;
            assert(sprites.sprite(ids[j],resource)&&masks.mask(ids[j],0,s.mask));
            s.originX=resource.originX;s.originY=resource.originY;s.frameCount=resource.frameCount;
            s.x=bits(float(int((i+j*7)%23)-11)*0.43f);s.y=bits(float(int((i+j*3)%19)-9)*0.31f);
            const float scales[]={1.f,1.000009f,-1.f,2.f,0.3f,-2.3f};
            s.transform.scaleX=bits(scales[(i/741+j)%6]);
            s.transform.scaleY=bits(scales[(i/741)%6]);
            const unsigned branch=(i/741)%3;
            if(branch==0){s.transform.scaleX=bits(1.f);s.transform.scaleY=bits(1.000009f);}
            s.transform.angle=bits(branch==2?(j?-45.f:90.f):0.000001f);
            s.bounds={bits(-3.7f),bits(-4.1f),bits(3.9f),bits(4.7f)};
        }
        unsigned path=0;const auto expected=reference(a,b,path);++paths[path];
        Pair::Result actual;
        assert(Pair::test(a,b,dispatch,trig,roundPixel,actual));
        assert(actual.hit==expected.hit&&actual.x==expected.x&&actual.y==expected.y);
        hits+=actual.hit;misses+=!actual.hit;
    }
    assert(paths[0]&&paths[1]&&paths[2]&&hits&&misses);
    // Two matching pixels distinguish X-outer from Y-outer traversal.
    const std::uint8_t diagonal[]={0x40,0x80};
    Pair::Shape a,b;a.frameCount=b.frameCount=1;
    a.mask=b.mask={diagonal,2,2,1};
    a.bounds=b.bounds={bits(0),bits(0),bits(1),bits(1)};
    Pair::Result result;
    assert(Pair::test(a,b,dispatch,trig,roundPixel,result)&&result.hit&&result.x==0&&result.y==1);
    // Truncation happens after intersecting fractional bounds, not rounding.
    a.bounds=b.bounds={bits(0.6f),bits(0.6f),bits(0.9f),bits(1.9f)};
    assert(Pair::test(a,b,dispatch,trig,roundPixel,result)&&result.hit&&result.x==0&&result.y==1);
    b.transform.scaleX={};
    assert(Pair::test(a,b,dispatch,trig,roundPixel,result)&&!result.hit);
    b.transform.scaleX=bits(1);b.mask.bytes=nullptr;result={true,7,9};
    assert(!Pair::test(a,b,dispatch,trig,roundPixel,result)&&result.hit&&result.x==7&&result.y==9);
    b.frameCount=0;
    assert(Pair::test(a,b,dispatch,trig,roundPixel,result)&&!result.hit);
    b.frameCount=1;b.mask=a.mask;a.bounds.left.bits=0x7f800000;result={true,7,9};
    assert(!Pair::test(a,b,dispatch,trig,roundPixel,result)&&result.x==7);
    a.x=b.x={};a.originX=b.originX=-2147483647-1; // INT32_MIN endpoint and wrapping origin sum.
    a.bounds=b.bounds={{0xcf000000},bits(0),{0xcf000000},bits(1)};
    assert(Pair::test(a,b,dispatch,trig,roundPixel,result)&&result.hit&&result.x==(-2147483647-1)&&result.y==1);
    a.bounds.right={0x4f000000};b.bounds.right=a.bounds.right;result={true,7,9};
    assert(!Pair::test(a,b,dispatch,trig,roundPixel,result)&&result.x==7); // Out-of-int32 endpoint.
    std::printf("Mask pairs: 4446 original-resource comparisons, paths %u/%u/%u, hits %u misses %u; scan order and failures passed\n",
        paths[0],paths[1],paths[2],hits,misses);
}
