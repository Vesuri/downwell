#include "core/WallBackgroundTiles.h"
#include <cassert>
#include <array>
#include <cstdio>
using namespace downwell;
static MotionBinary64 n(int value){return MotionBinary64::fromInteger(value);}
int main(){
    WallBackgroundTiles<4> tiles;
    assert(tiles.append(n(5),n(7))&&tiles.append(n(5),n(7))); // Do not merge coincident births.
    assert(tiles.append(n(-8),n(30))&&tiles.append(n(28),n(-8)));
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
    assert(!tiles.paintRegion(n(0),n(0),0,0,32,40,raster,[](int,int,int,int){return true;}));
    tiles.clear();assert(tiles.valid()&&tiles.size()==0);
    assert(tiles.append(n(0),n(0)));
    assert(!tiles.paintRegion(n(0),n(0),0,0,32,40,raster,[](int,int,int,int){return false;}));
    assert(tiles.size()==1&&tiles.valid()); // Drawing failure does not erase retained state.
    tiles.clear();assert(tiles.append(n(2147483640),n(0)));
    unsigned calls=0;
    assert(tiles.paintRegion(n(0),n(0),2147483640,0,2147483647,16,raster,[&](int x,int y,int w,int h){
        ++calls;return x==2147483640&&y==0&&w==7&&h==16;}));assert(calls==1);
    std::puts("Retained wall tiles: duplicate order, full/strip raster equivalence, clipping, overflow edges and failures passed");
}
