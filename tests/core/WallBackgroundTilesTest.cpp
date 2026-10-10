#include "core/WallBackgroundTiles.h"
#include <cassert>
#include <array>
#include <cstdio>
using namespace downwell;
static MotionBinary64 n(int value){return MotionBinary64::fromInteger(value);}
int main(){
    WallBackgroundTiles<4> tiles;
    bool hit=true;
    assert(tiles.contains(n(0),n(0),hit)&&!hit);
    assert(tiles.append(n(5),n(7))&&tiles.append(n(5),n(7))); // Do not merge coincident births.
    assert(tiles.append(n(-8),n(30))&&tiles.append(n(28),n(-8)));
    for(int y=-10;y<50;++y)for(int x=-10;x<50;++x){
        bool expected=false;
        for(unsigned i=0;i<tiles.size();++i){std::int32_t tx,ty;
            assert(tiles.at(i)->x.roundPixel(tx)&&tiles.at(i)->y.roundPixel(ty));
            expected|=x>=tx&&x<tx+16&&y>=ty&&y<ty+16;
        }
        assert(tiles.contains(n(x),n(y),hit)&&hit==expected);
    }
    auto raster=[](auto value,std::int32_t& out){return value.roundPixel(out);};
    for(int camera=-4;camera<=4;++camera){
        std::array<unsigned,32*40> whole{},strips{};
        auto painter=[](auto& pixels){return [&](int x,int y,int w,int h){
            assert(x>=0&&y>=0&&x+w<=32&&y+h<=40&&w>0&&h>0);
            for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)++pixels[yy*32+xx];return true;
        };};
        assert(tiles.paintRegion(n(camera),n(camera),0,0,32,40,raster,painter(whole)));
        for(int row=0;row<40;++row)
            assert(tiles.paintRegion(n(camera),n(camera),0,row,32,row+1,raster,painter(strips)));
        assert(whole==strips);
        for(int y=0;y<40;++y)for(int x=0;x<32;++x){
            unsigned expected=0;
            for(unsigned i=0;i<tiles.size();++i){std::int32_t tx,ty;
                assert(raster(tiles.at(i)->x,tx)&&raster(tiles.at(i)->y,ty));
                expected+=x>=tx-camera&&x<tx-camera+16&&y>=ty-camera&&y<ty-camera+16;
            }
            assert(whole[y*32+x]==expected);
        }
    }
    assert(!tiles.append(n(0),n(0))&&!tiles.valid()&&tiles.size()==4);
    hit=true;assert(!tiles.contains(n(0),n(0),hit)&&hit);
    assert(!tiles.paintRegion(n(0),n(0),0,0,32,40,raster,[](int,int,int,int){return true;}));
    tiles.clear();assert(tiles.valid()&&tiles.size()==0);
    auto fractional=n(0);assert(fractional.add({0x3fe00000u,0}));
    assert(tiles.append(fractional,fractional));
    auto query=[&](MotionBinary64 x,MotionBinary64 y,bool expected){
        assert(tiles.contains(x,y,hit)&&hit==expected);
    };
    auto right=fractional;assert(right.add(n(16)));
    // Use powers of two on either side of epsilon to avoid rounding ties.
    const MotionBinary64 inside{0x3ee00000u,0},outside{0x3ef00000u,0};
    int comparison;assert(compareRunnerReal(inside,{},comparison)&&comparison==0&&
        compareRunnerReal(outside,{},comparison)&&comparison>0);
    auto lower=fractional;auto negative=inside;negative.high^=0x80000000u;assert(lower.add(negative));
    query(lower,fractional,true);query(fractional,lower,true);
    lower=fractional;negative=outside;negative.high^=0x80000000u;assert(lower.add(negative));
    query(lower,fractional,false);query(fractional,lower,false);
    auto upper=right;negative=inside;negative.high^=0x80000000u;assert(upper.add(negative));
    query(upper,fractional,false);query(fractional,upper,false);
    upper=right;negative=outside;negative.high^=0x80000000u;assert(upper.add(negative));
    query(upper,fractional,true);query(fractional,upper,true);
    query(right,fractional,false);query(fractional,right,false);
    hit=true;assert(!tiles.contains({0x7ff00000u,0},{},hit)&&hit);
    tiles.clear();
    assert(tiles.append(n(0),n(0)));
    assert(!tiles.paintRegion(n(0),n(0),0,0,32,40,raster,[](int,int,int,int){return false;}));
    assert(tiles.size()==1&&tiles.valid()); // Drawing failure does not erase retained state.
    tiles.clear();assert(tiles.append(n(2147483640),n(0)));
    unsigned calls=0;
    assert(tiles.paintRegion(n(0),n(0),2147483640,0,2147483647,16,raster,[&](int x,int y,int w,int h){
        ++calls;return x==2147483640&&y==0&&w==7&&h==16;}));assert(calls==1);
    std::puts("Retained wall tiles: source point queries, epsilon/fractional edges, full/strip raster equivalence and failures passed");
}
