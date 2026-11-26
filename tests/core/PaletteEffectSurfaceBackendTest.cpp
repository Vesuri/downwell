// A backend that accepts stamps (the Amiga's queued blits) records each
// stamp's word rectangle in the row extents. Oracles: the rectangles those
// per-stamp rows give (rebuilt here independently), and the pixels, visit
// and intersects of the exact CPU surface (a declining backend).
#include "core/PaletteEffectSurface.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
using Surface=downwell::PaletteEffectSurface<>;
constexpr unsigned W=160,H=284,words=W/16,stride=12,plane=stride*H;
struct Storage {std::array<std::uint16_t,plane*3> data{};};
static Storage exactStorage,deferredStorage;
static Surface exact,deferred;
static unsigned bit(const std::uint8_t* row,unsigned sx){return row[sx/8]&(0x80u>>(sx%8));}
// The accepting backend: the same per-pixel result a blit writes, inside the clip.
static bool accept(void* p,const Surface::Mask& m,const std::uint8_t* lo,const std::uint8_t* hi,int x,int y,
                   const Surface::Clip& clip){
    auto& s=*static_cast<Storage*>(p);
    for(unsigned sy=0;sy<m.height;++sy)for(unsigned sx=0;sx<m.width;++sx){
        const int dx=x+int(sx),dy=y+int(sy);
        if(dx<int(clip.left*16)||dx>=int(clip.right*16)||dy<int(clip.top)||dy>=int(clip.bottom))continue;
        const unsigned at=sy*m.stride;if(!bit(m.data+at,sx))continue;
        const unsigned word=1+unsigned(dx)/16,mask=0x8000u>>(unsigned(dx)%16),index=unsigned(dy)*stride+word;
        if(!lo){s.data[index]&=std::uint16_t(~mask);continue;}
        s.data[index]|=std::uint16_t(mask);
        s.data[plane+index]=std::uint16_t(bit(lo+at,sx)?s.data[plane+index]|mask:s.data[plane+index]&~mask);
        s.data[plane*2+index]=std::uint16_t(bit(hi+at,sx)?s.data[plane*2+index]|mask:s.data[plane*2+index]&~mask);
    }
    return true;
}
static void clearRect(void* p,unsigned left,unsigned top,unsigned n,unsigned rows){
    auto& s=*static_cast<Storage*>(p);
    for(unsigned k=0;k<3;++k)for(unsigned y=top;y<top+rows;++y)for(unsigned x=1+left;x<=left+n;++x)s.data[k*plane+y*stride+x]=0;
}
static bool decline(void*,const Surface::Mask&,const std::uint8_t*,const std::uint8_t*,int,int,const Surface::Clip&){return false;}
static void noWait(void*){}
// The previous per-stamp row extents of accepted adds.
static std::array<unsigned,H> oracleFirst,oracleLast;
static void oracleClear(){oracleFirst.fill(words);oracleLast.fill(0);}
static void oracleAdd(const Surface::Mask& m,int x,int y){
    if(x>=int(W)||y>=int(H)||x+int(m.width)<=0||y+int(m.height)<=0)return;
    const unsigned left=x<0?0:unsigned(x)/16,right=x+int(m.width)>int(W)?words:unsigned(x+int(m.width)+15)/16;
    const unsigned top=y<0?0:unsigned(y),bottom=y+int(m.height)>int(H)?H:unsigned(y+int(m.height));
    for(unsigned r=top;r<bottom;++r){if(left<oracleFirst[r])oracleFirst[r]=left;if(right>oracleLast[r])oracleLast[r]=right;}
}
static std::vector<unsigned> oracleRectangles(){
    std::vector<unsigned> out;unsigned y=0;
    while(y<H){
        if(oracleFirst[y]>=oracleLast[y]){++y;continue;}
        const unsigned top=y;unsigned left=oracleFirst[y],right=oracleLast[y];
        do{if(oracleFirst[y]<left)left=oracleFirst[y];if(oracleLast[y]>right)right=oracleLast[y];++y;}
        while(y<H&&oracleFirst[y]<oracleLast[y]);
        out.insert(out.end(),{left*16,top,(right-left)*16,y-top});
    }
    return out;
}
static std::vector<unsigned> rectangles(const Surface& s){
    std::vector<unsigned> out;
    s.visitRectangles([&](unsigned x,unsigned y,unsigned w,unsigned h){out.insert(out.end(),{x,y,w,h});});
    return out;
}
static std::vector<unsigned> pixels(const Surface& s){
    std::vector<unsigned> out;
    s.visit([&](unsigned x,unsigned y,std::uint16_t a,std::uint16_t l,std::uint16_t h){
        for(unsigned b=0;b<16;++b)if(a&(0x8000u>>b))out.insert(out.end(),{x+b,y,unsigned(bool(l&(0x8000u>>b))),unsigned(bool(h&(0x8000u>>b)))});
    });
    return out;
}
static unsigned seed=12345;
static unsigned next(){seed=seed*1103515245u+12345u;return (seed>>16)&0x7fff;}

int main(){
    assert(exact.bindStorage(exactStorage.data.data()+1,exactStorage.data.data()+plane+1,exactStorage.data.data()+plane*2+1,
        stride,{&exactStorage,decline,clearRect,noWait}));
    assert(deferred.bindStorage(deferredStorage.data.data()+1,deferredStorage.data.data()+plane+1,deferredStorage.data.data()+plane*2+1,
        stride,{&deferredStorage,accept,clearRect,noWait}));
    // Masks of 1-40 by 1-40 pixels, random bits, colors and empty rows.
    static std::uint8_t bank[64][3][5*40];
    struct Shape {Surface::Mask mask;const std::uint8_t* low;const std::uint8_t* high;};
    std::vector<Shape> shapes;
    for(unsigned i=0;i<64;++i){
        const unsigned w=1+next()%40,h=1+next()%40,s=(w+7)/8;
        for(unsigned k=0;k<3;++k)for(unsigned j=0;j<s*h;++j)bank[i][k][j]=std::uint8_t(next());
        for(unsigned r=0;r<h;r+=1+next()%7)for(unsigned j=0;j<s;++j)bank[i][2][r*s+j]=0;
        shapes.push_back({{bank[i][2],s*h,w,h,s},bank[i][0],bank[i][1]});
    }
    oracleClear();
    unsigned checks=0,stamps=0;
    for(unsigned round=0;round<3000;++round){
        const unsigned op=next()%100;
        if(op<3){exact.clear();deferred.clear();oracleClear();
            for(unsigned i=0;i<plane*3;++i)assert(deferredStorage.data[i]==0);}
        else if(op<85){
            const auto& s=shapes[next()%shapes.size()];
            const int x=int(next()%220)-40,y=int(next()%350)-40;
            assert(exact.add(s.mask,s.low,s.high,x,y)&&deferred.add(s.mask,s.low,s.high,x,y));
            oracleAdd(s.mask,x,y);++stamps;
        }else if(op<92){
            const auto& s=shapes[next()%shapes.size()];
            const int x=int(next()%220)-40,y=int(next()%350)-40;
            assert(exact.subtract(s.mask,x,y)&&deferred.subtract(s.mask,x,y));
        }else{
            const int left=int(next()%200)-20,top=int(next()%320)-20;
            const int right=left+int(next()%60),bottom=top+int(next()%60);
            assert(exact.intersects(left,top,right,bottom)==deferred.intersects(left,top,right,bottom));
        }
        if(round%7==0){
            assert(rectangles(deferred)==oracleRectangles());
            assert(pixels(deferred)==pixels(exact));
            assert(deferred.samePixels(exact));
            ++checks;
        }
    }
    std::printf("Palette effect accepting backend: %u stamps, %u rectangle/pixel checks against the per-stamp rows and the CPU surface passed\n",stamps,checks);
    return 0;
}
