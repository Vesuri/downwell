#include "platform/amiga/AmigaTerrainRaster.h"
#include "platform/amiga/framework/Bitmap.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
static unsigned copiedRows;
void AmigaHardware::blitterDrain() {}
void AmigaHardware::setBlitterNasty(bool) {}
void Bitmap::copy(const Bitmap& source,uint16_t x,uint16_t y,uint16_t sx,uint16_t sy,uint16_t w,uint16_t h,uint16_t) {
    assert(x==96 && sx==96 && y==sy && w==176 && y+h<=320 && h<=128);
    copiedRows+=h;
    for(unsigned p=0;p<2;++p) for(unsigned row=y;row<y+h;++row) {
        const unsigned at=p*320*48+row*48+12;
        std::memcpy(static_cast<UBYTE*>(data)+at,static_cast<const UBYTE*>(source.data)+at,22);
    }
}
int main() {
    auto raster=std::make_unique<AmigaTerrainRaster>();
    unsigned cases=0;
    unsigned partitions=0;
    for(unsigned column=0;column<3;++column)for(unsigned mask=0;mask<256;++mask){
        for(unsigned y=0;y<8192;++y)raster->dirty(column,y)=false;
        for(unsigned y=0;y<284;++y)raster->dirty(column,7908+y)=((mask>>(y%8))&1)!=0;
        unsigned first=7908,painted=0,copied=0;
        while(first<8192){
            const bool dirty=raster->dirty(column,first);
            const auto end=raster->runEnd(column,first,8192);
            assert(end>first&&end<=8192);
            for(unsigned y=first;y<end;++y){
                assert(raster->dirty(column,y)==dirty);
                // Simulate the compositor clearing a completed dirty run.
                raster->dirty(column,y)=false;
                if(dirty)++painted;else ++copied;
            }
            first=end;
        }
        unsigned expected=0;
        for(unsigned y=0;y<284;++y)expected+=(mask>>(y%8))&1;
        assert(painted==expected&&copied==284-expected);++partitions;
    }
    assert(raster->runEnd(3,0,284)==0&&raster->runEnd(0,5,5)==5&&raster->runEnd(0,0,8193)==0);
    std::printf("Terrain raster: %u mixed-span partitions repaint only dirty rows\n",partitions);
    downwell::ShopWorld shops{};
    shops.instances[0].allocated=true;
    for(int y : {-50, 0, 120, 8190, 8250}) {
        for(unsigned c=0;c<3;++c) for(unsigned r=0;r<8192;++r) raster->dirty(c,r)=false;
        auto& shop=shops.instances[0]; shop.y=y; ++shop.artRevision;
        raster->invalidateShops(shops);
        for(unsigned c=0;c<3;++c) for(int r=0;r<8192;++r)
            assert(raster->dirty(c,r)==(r>=y-21 && r<y+12));
        for(unsigned c=0;c<3;++c) for(unsigned r=0;r<8192;++r) raster->dirty(c,r)=false;
        raster->invalidateShops(shops);
        for(unsigned c=0;c<3;++c) for(unsigned r=0;r<8192;++r) assert(!raster->dirty(c,r));
    }
    for(unsigned col=0;col<3;++col) for(unsigned origin=0;origin<320;++origin)
    for(unsigned height : {1u,96u,97u,284u,320u}) {
        std::vector<UBYTE> source(downwellGameRingBytes+16), destination(source.size(),0xa5), display(source.size(),0x5a);
        for(unsigned i=0;i<source.size();++i) source[i]=(i*17+origin+col*7)%251;
        AmigaRenderer renderer{}; renderer.game=source.data()+8; renderer.gameOrigin=origin;
        AmigaGameplayRows debts;
        raster->transfer(&renderer,col,400,0,height,true,nullptr,nullptr,0,debts);
        for(unsigned y=0;y<height;++y) for(unsigned p=0;p<2;++p)
            assert(!std::memcmp(raster->row(col,400+y)+p*11,
                source.data()+8+p*320*48+((origin+y)%320)*48+12,22));
        renderer.game=destination.data()+8;
        debts.damage(origin,0,height);
        Bitmap backing(renderer.game,380,320,2,false,false,384);
        Bitmap target(display.data()+8,380,320,2,false,false,384);
        copiedRows=0;
        raster->transfer(&renderer,col,400,0,height,false,&target,&backing,1,debts);
        auto expected=std::vector<UBYTE>(source.size(),0xa5);
        auto expectedDisplay=std::vector<UBYTE>(source.size(),0x5a);
        for(unsigned y=0;y<height;++y) {
            const unsigned row=(origin+y)%320;
            for(unsigned p=0;p<2;++p) {
                const unsigned at=8+p*320*48+row*48+12;
                std::memcpy(expected.data()+at,source.data()+at,22);
                if(height>96) std::memcpy(expectedDisplay.data()+at,source.data()+at,22);
            }
            assert(debts.dirty(row,0));
            assert(debts.dirty(row,1)==(height<=96));
        }
        assert(destination==expected && display==expectedDisplay);
        assert(copiedRows==(height>96 ? height : 0));
        ++cases;
    }
    std::printf("Terrain raster: %u store/load/wrap/pipeline cases, exact pixels and preserved margins\n",cases);
}
