#include "platform/amiga/AmigaTerrainRaster.h"
#include "platform/amiga/AmigaShopArtSnapshot.h"
#include "platform/amiga/framework/Bitmap.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
static unsigned copiedRows;
static_assert(sizeof(AmigaTerrainRaster)<32*1024,
    "Terrain pixels must remain a gameplay lease, not embedded startup storage");
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
    {
        auto raster=std::make_unique<AmigaTerrainRaster>();
        std::vector<UWORD> storage(AmigaTerrainRaster::storageBytes/2);
        assert(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        std::vector<UBYTE> walls(32*44),tiles(3*32*20);
        for(unsigned i=0;i<walls.size();++i)walls[i]=UBYTE(i*37+i/11);
        for(unsigned i=0;i<tiles.size();++i)tiles[i]=UBYTE(i*13+i/7);
        AmigaPatternRow band{walls.data(),{tiles.data(),tiles.data()+32*20,tiles.data()+64*20}};
        assert(!raster->borrowPatternBand(97,band));
        assert(!raster->borrowPatternBand(8192-8,band));
        raster->invalidate(88,120);
        assert(raster->borrowPatternBand(88,band));
        assert(raster->borrowPatternBand(104,{walls.data()+16*44,{nullptr,nullptr,nullptr}}));
        for(unsigned y=88;y<120;++y)assert(raster->dirty(1,y));
        for(unsigned y=88;y<120;++y)raster->setDirty(1,y,false);
        // A materialized replacement must not clobber the band's descriptor.
        auto* replacement=raster->writeRows(1,88,1);
        for(unsigned w=0;w<22;++w)replacement[w]=UWORD(0x4320+w);
        raster->invalidate(90,92);
        for(unsigned y=90;y<92;++y){assert(raster->dirty(1,y));raster->setDirty(1,y,false);}
        for(unsigned start=0;start<32;++start)for(unsigned height=1;height<=32-start;++height){
            std::vector<UBYTE> actual(downwellGameRingBytes,0x5a),expected(actual);
            AmigaRenderer renderer{};renderer.game=actual.data();renderer.gameOrigin=313;
            AmigaGameplayRows debts;
            for(unsigned y=0;y<height;++y){
                const unsigned offset=start+y,physical=(313+y)%320;
                UWORD reference[22];
                if(!offset)for(unsigned w=0;w<22;++w)reference[w]=UWORD(0x4320+w);
                else if(offset<16){
                    const AmigaPatternRow source{walls.data()+offset*44,
                        {tiles.data()+offset*20,tiles.data()+(32+offset)*20,tiles.data()+(64+offset)*20}};
                    source.write(reference,reference+11);
                }else for(unsigned w=0;w<22;++w)reference[w]=AmigaPatternRow::word(walls.data()+offset*44+w*2);
                UWORD scratch[22];const auto* read=raster->readRow(1,88+offset,scratch);
                assert(std::memcmp(read,reference,sizeof reference)==0);
                for(unsigned p=0;p<2;++p)std::memcpy(expected.data()+p*downwellGameRingPlaneBytes+physical*48+12,reference+p*11,22);
            }
            raster->transfer(&renderer,1,88+start,0,height,false,nullptr,nullptr,0,debts);
            assert(actual==expected);
        }
        std::puts("Pattern bands: 528 partial/wrapped transfers, independent row oracle, materialization and invalidation passed");
        // Exercise the entire descriptor suffix, not just the initial-fall
        // bands. In particular the last admissible band uses descriptor 511.
        for(unsigned first=8;first<=AmigaTerrainRaster::rows-24;first+=16){
            raster->invalidate(first,first+16);
            assert(raster->borrowPatternBand(first,band,true));
            for(unsigned line=0;line<16;++line){
                UWORD expected[22],scratch[22];
                band.offset(line).write(expected,expected+11);
                assert(!raster->dirty(1,first+line));
                assert(std::memcmp(raster->readRow(1,first+line,scratch),expected,sizeof expected)==0);
            }
        }
        // A rejected admission must preserve the last valid descriptor and
        // its dirty state, including partially specified tile references.
        const unsigned last=AmigaTerrainRaster::rows-24;
        raster->invalidate(last,last+16);
        assert(!raster->borrowPatternBand(last,{nullptr,{nullptr,nullptr,nullptr}},true));
        assert(!raster->borrowPatternBand(last,{walls.data(),{tiles.data(),nullptr,tiles.data()}},true));
        assert(!raster->borrowPatternBand(last+16,band,true));
        for(unsigned line=0;line<16;++line){
            UWORD expected[22],scratch[22];
            band.offset(line).write(expected,expected+11);
            assert(raster->dirty(1,last+line));
            assert(std::memcmp(raster->readRow(1,last+line,scratch),expected,sizeof expected)==0);
        }
        raster->releaseStorage();
        assert(!raster->borrowPatternBand(8,band));
        assert(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        assert(raster->borrowPatternBand(last,band,true));
        std::puts("Pattern bands: all 511 descriptor slots, rejected admission and lease retirement passed");
        // A preparation prefix must coexist with the unmaterialized suffix.
        // Geometry mutation revokes admission even though source loans remain.
        assert(raster->needsPatternMaterialization(last));
        assert(raster->materializePatternRows(last,0)==0);
        assert(raster->materializePatternRows(last,5)==5);
        assert(!raster->needsPatternMaterialization(last));
        assert(raster->needsPatternMaterialization(last+5));
        raster->invalidate(last+9,last+10);
        assert(raster->materializePatternRows(last+5,8)==4);
        assert(raster->materializePatternRows(last+9,8)==0);
        assert(raster->materializePatternRows(last+10,8)==6);
        for(unsigned line=0;line<16;++line){
            UWORD expected[22],scratch[22];
            band.offset(line).write(expected,expected+11);
            assert(std::memcmp(raster->readRow(1,last+line,scratch),expected,sizeof expected)==0);
        }
        assert(raster->dirty(1,last+9));
        assert(raster->materializePatternRows(AmigaTerrainRaster::rows,8)==0);
        std::puts("Pattern materialization: partial budgets, band edge and mutation revocation passed");
    }
    {
        auto raster=std::make_unique<AmigaTerrainRaster>();
        std::vector<UWORD> storage(AmigaTerrainRaster::storageBytes/2);
        assert(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        std::vector<UBYTE> walls(128*44),tiles(3*128*20);
        for(unsigned i=0;i<walls.size();++i)walls[i]=UBYTE(i*37+i/11);
        for(unsigned i=0;i<tiles.size();++i)tiles[i]=UBYTE(i*13+i/7);
        for(unsigned y=0;y<128;++y){
            if(y<32||y%3==0){
                const AmigaPatternRow source{walls.data()+y*44,
                    {tiles.data()+y*20,tiles.data()+(128+y)*20,tiles.data()+(256+y)*20}};
                raster->borrowPatternRow(1,100+y,source);
            }else if(y%3==1)raster->borrowPatternRow(1,100+y,walls.data()+y*44);
            else{
                auto* row=raster->writeRows(1,100+y,1);
                for(unsigned w=0;w<22;++w)row[w]=UWORD(y*701+w);
            }
        }
        // Invalidating and acknowledging preserve references, while a full
        // replacement severs them before writing ordinary pixel words.
        raster->invalidate(100,228);
        for(unsigned y=100;y<228;++y){assert(raster->dirty(1,y));raster->setDirty(1,y,false);}
        auto* replacement=raster->writeRows(1,103,1);
        for(unsigned w=0;w<22;++w)replacement[w]=UWORD(0x9810+w);
        for(unsigned origin:{0u,311u,319u})for(unsigned height:{1u,16u,128u}){
            std::vector<UBYTE> actual(downwellGameRingBytes,0x5a),expected(actual),display(actual);
            AmigaRenderer renderer{};renderer.game=actual.data();renderer.gameOrigin=origin;
            AmigaGameplayRows debts;debts.damage(origin,7,height);
            Bitmap backing(actual.data(),380,320,2,false,false,384);
            Bitmap target(display.data(),380,320,2,false,false,384);
            for(unsigned y=0;y<height;++y){
                UWORD scratch[22];const auto* source=raster->readRow(1,100+y,scratch);
                const unsigned physical=(origin+7+y)%320;
                for(unsigned p=0;p<2;++p)
                    std::memcpy(expected.data()+p*downwellGameRingPlaneBytes+physical*48+12,source+p*11,22);
            }
            raster->transfer(&renderer,1,100,7,height,false,&target,&backing,0,debts);
            assert(actual==expected);
            if(height>96)assert(display==expected);
        }
        std::puts("Pattern row references: mixed owned/borrowed/composed transfers, wrap, replacement and pipeline passed");
    }
    {
        auto raster=std::make_unique<AmigaTerrainRaster>();
        std::vector<UWORD> storage(AmigaTerrainRaster::storageBytes/2);
        assert(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        assert(raster->nextPreparation(300,428)==AmigaTerrainRaster::rows);
        std::vector<UBYTE> contiguous(24*44);
        for(unsigned y=32;y<56;++y)raster->borrowPatternRow(1,y,contiguous.data()+(y-32)*44);
        assert(raster->borrowedRunEnd(1,32,56)==56);
        assert(raster->borrowedRunEnd(1,35,39)==39);
        (void)raster->writeRows(1,40,1);
        assert(raster->borrowedRunEnd(1,32,56)==40);
        assert(raster->borrowedRunEnd(1,41,56)==56);
        raster->borrowPatternRow(1,48,contiguous.data()+17*44);
        assert(raster->borrowedRunEnd(1,41,56)==48);
        assert(raster->borrowedRunEnd(1,48,56)==49);
        raster->invalidate(320,368);
        assert(raster->nextPreparation(300,320)==AmigaTerrainRaster::rows);
        assert(raster->nextPreparation(300,428)==320);
        for(unsigned y=320;y<324;++y)raster->setDirty(1,y,false);
        assert(raster->nextPreparation(300,428)==324);
        // Failed speculative painting leaves the visible fallback debt intact,
        // but does not spend the next update retrying unsupported artwork.
        raster->skipPreparation(336);
        assert(raster->dirty(1,324)&&raster->nextPreparation(300,428)==336);
        raster->invalidate(322,324);
        assert(raster->nextPreparation(300,428)==322);
        assert(raster->nextPreparation(400,528)==AmigaTerrainRaster::rows);
        raster->invalidate(480,496);
        assert(raster->nextPreparation(400,528)==480);
        raster->releaseStorage();
        assert(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        assert(raster->nextPreparation(300,428)==AmigaTerrainRaster::rows);
    }
    {
        auto raster=std::make_unique<AmigaTerrainRaster>();
        std::vector<UWORD> storage(AmigaTerrainRaster::storageBytes/2,0x55aa);
        assert(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        raster->ready=true;raster->invalidate(312,328);
        raster->deferPreparationFallback(312);
        unsigned calls=0,bias=0;
        auto paint=[&](UWORD* pixels,unsigned first,unsigned count){
            ++calls;assert(count<=8);
            for(unsigned y=0;y<count;++y)for(unsigned w=0;w<22;++w)
                pixels[y*22+w]=UWORD((first+y)*31+w+bias);
            return true;
        };
        assert(raster->preparationFallback(312)&&!raster->preparationFallback(328));
        assert(raster->prepareFallback(312,328,paint)==8&&calls==1);
        assert(raster->nextPreparation(312,328)==320);
        // A failed producer must not publish partially written rows.
        assert(raster->prepareFallback(320,328,[](auto* p,unsigned,unsigned){p[0]=0;return false;})==-1);
        for(unsigned y=320;y<328;++y)assert(raster->dirty(1,y));
        assert(raster->prepareFallback(320,324,paint)==4);
        assert(raster->prepareFallback(324,328,paint)==4);
        for(unsigned y=312;y<328;++y){
            UWORD scratch[22];const auto* pixels=raster->readRow(1,y,scratch);
            assert(!raster->dirty(1,y));
            for(unsigned w=0;w<22;++w)assert(pixels[w]==UWORD(y*31+w));
        }
        raster->invalidate(316,318);
        assert(raster->preparationFallback(316)); // A task retains no source pixels.
        bias=42;
        assert(raster->prepareFallback(316,328,paint)==2); // Do not repaint clean suffix.
        UWORD scratch[22];assert(raster->readRow(1,316,scratch)[0]==UWORD(316*31+42));
        raster->deferPreparationFallback(0);assert(!raster->preparationFallback(0));
        raster->deferPreparationFallback(8191);assert(!raster->preparationFallback(8191));
        raster->deferPreparationFallback(316);raster->releaseStorage();
        assert(!raster->preparationFallback(316));
        assert(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        assert(!raster->preparationFallback(316));
        std::puts("Fallback preparation: bounded pieces, clipping, mutation, failure and lease retirement");
    }
    // A deliberately non-linear dictionary distinguishes row references from
    // contiguous mutable storage, including transfers across the pack end.
    {
        constexpr unsigned initialRows=137, dictionaryRows=7;
        std::vector<UBYTE> pack(12+initialRows*6+dictionaryRows*44);
        std::memcpy(pack.data(),"DWIT",4);
        auto put=[&](unsigned at,unsigned value){pack[at]=value>>8;pack[at+1]=value;};
        put(4,1);put(6,initialRows);put(8,dictionaryRows);
        for(unsigned c=0;c<3;++c)for(unsigned y=0;y<initialRows;++y)
            put(12+(c*initialRows+y)*2,(y*3+c)%dictionaryRows);
        for(unsigned r=0;r<dictionaryRows;++r)for(unsigned w=0;w<22;++w)
            put(12+initialRows*6+r*44+w*2,0x1234+r*941+w*57);
        AmigaInitialTerrain initial;
        assert(initial.bind(pack.data(),pack.size()));
        auto borrowed=std::make_unique<AmigaTerrainRaster>();
        auto materialized=std::make_unique<AmigaTerrainRaster>();
        std::vector<UWORD> borrowedPixels(AmigaTerrainRaster::storageBytes/2),materializedPixels(borrowedPixels.size());
        assert(borrowed->bindStorage(borrowedPixels.data(),AmigaTerrainRaster::storageBytes));
        assert(materialized->bindStorage(materializedPixels.data(),AmigaTerrainRaster::storageBytes));
        for(unsigned c=0;c<3;++c){
            std::fill_n(borrowed->writeRows(c,0,320),320*22,0xdead);
            std::fill_n(materialized->writeRows(c,0,320),320*22,0xdead);
            assert(initial.copy(c,materialized->writeRows(c,0,initialRows)));
        }
        assert(borrowed->borrowInitial(initial));
        // Band invalidation equals setDirty(c,y,true) per row: odd starts and
        // lengths, clamped bounds and borrowed-initial state bits included.
        {
            auto banded=std::make_unique<AmigaTerrainRaster>(),rows=std::make_unique<AmigaTerrainRaster>();
            std::vector<UWORD> bandedPixels(borrowedPixels.size()),rowPixels(borrowedPixels.size());
            assert(banded->bindStorage(bandedPixels.data(),AmigaTerrainRaster::storageBytes));
            assert(rows->bindStorage(rowPixels.data(),AmigaTerrainRaster::storageBytes));
            assert(banded->borrowInitial(initial)&&rows->borrowInitial(initial));
            unsigned bands=0;
            for(int first=-3;first<140;first+=7)for(int length=0;length<12;++length){
                const int end=first+length*5-1;
                banded->invalidate(first,end);
                for(unsigned c=0;c<3;++c)for(int y=first<0?0:first;y<end&&y<int(AmigaTerrainRaster::rows);++y)
                    rows->setDirty(c,unsigned(y),true);
                UWORD scratchA[22],scratchB[22];
                for(unsigned c=0;c<3;++c)for(unsigned y=0;y<200;++y){
                    assert(banded->dirty(c,y)==rows->dirty(c,y));
                    assert(!std::memcmp(banded->readRow(c,y,scratchA),rows->readRow(c,y,scratchB),44));
                }
                for(unsigned c=0;c<3;++c)for(unsigned y=0;y<200;++y)rows->setDirty(c,y,false),banded->setDirty(c,y,false);
                ++bands;
            }
            banded->invalidate(int(AmigaTerrainRaster::rows)-5,int(AmigaTerrainRaster::rows)+9);
            for(unsigned c=0;c<3;++c)for(unsigned y=AmigaTerrainRaster::rows-5;y<AmigaTerrainRaster::rows;++y)assert(banded->dirty(c,y));
            std::printf("Terrain raster: %u banded invalidations match per-row dirty publication\n",bands);
        }
        unsigned comparisons=0;
        auto compare=[&](){
            for(unsigned c=0;c<3;++c)for(unsigned origin : {0u,91u,319u})
            for(unsigned height : {1u,96u,137u,284u}){
                std::vector<UBYTE> a(downwellGameRingBytes,0xa5),b(a);
                std::vector<UBYTE> displayA(downwellGameRingBytes,0x5a),displayB(displayA);
                AmigaRenderer ra{},rb{};ra.game=a.data();rb.game=b.data();
                ra.gameOrigin=rb.gameOrigin=origin;
                AmigaGameplayRows da,db;
                da.damage(origin,0,height);db.damage(origin,0,height);
                Bitmap backingA(a.data(),380,320,2,false,false,384);
                Bitmap backingB(b.data(),380,320,2,false,false,384);
                Bitmap targetA(displayA.data(),380,320,2,false,false,384);
                Bitmap targetB(displayB.data(),380,320,2,false,false,384);
                borrowed->transfer(&ra,c,0,0,height,false,&targetA,&backingA,0,da);
                materialized->transfer(&rb,c,0,0,height,false,&targetB,&backingB,0,db);
                assert(a==b&&displayA==displayB);
                for(unsigned y=0;y<320;++y)for(unsigned buffer=0;buffer<2;++buffer)
                    assert(da.dirty(y,buffer)==db.dirty(y,buffer));
                ++comparisons;
            }
        };
        compare();
        // Pattern rows borrow their pixels through the mutable row storage,
        // rather than the initial-room directory. Mix both representations
        // with ordinary pixels and cross the physical ring wrap in transfer.
        for(unsigned c=0;c<3;++c)for(unsigned y=1;y<initialRows;y+=3){
            const unsigned reference=(y*3+c)%dictionaryRows;
            borrowed->borrowPatternRow(c,y,pack.data()+12+initialRows*6+reference*44);
        }
        compare();
        // Invalidation alone must not change backing pixels before repaint.
        borrowed->invalidate(20,70);compare();
        for(unsigned c=0;c<3;++c){
            std::fill_n(borrowed->writeRows(c,31,37),37*22,0xbeef);
            std::fill_n(materialized->writeRows(c,31,37),37*22,0xbeef);
        }
        compare();
        // A Chip-to-cache replacement must retire every borrowed row it writes.
        std::vector<UBYTE> chip(downwellGameRingBytes,0x5a);
        AmigaRenderer source{};source.game=chip.data();source.gameOrigin=309;
        AmigaGameplayRows debts;
        for(unsigned c=0;c<3;++c){
            borrowed->transfer(&source,c,81,0,40,true,nullptr,nullptr,0,debts);
            materialized->transfer(&source,c,81,0,40,true,nullptr,nullptr,0,debts);
        }
        compare();
        assert(borrowed->borrowInitial(initial));
        for(unsigned c=0;c<3;++c){
            assert(initial.copy(c,materialized->writeRows(c,0,initialRows)));
            assert(borrowed->dirty(c,20)&&borrowed->dirty(c,69));
            assert(!borrowed->dirty(c,70));
            UWORD scratch[22],oracle[22];
            for(unsigned y=0;y<initialRows;++y)
                assert(!std::memcmp(borrowed->readRow(c,y,scratch),initial.row(c,y,oracle),44));
        }
        compare();
        assert(!borrowed->materializeInitialColumn(3));
        std::fill_n(borrowed->writeRows(1,41,2),44,0x7654);
        std::fill_n(materialized->writeRows(1,41,2),44,0x7654);
        assert(borrowed->materializeInitialColumn(1));
        assert(borrowed->materializeInitialColumn(1)); // Idempotent after entry.
        assert(borrowed->dirty(1,20)&&borrowed->dirty(1,69));
        assert(!borrowed->dirty(1,70));
        compare();
        // Version 2 adds periodic corridor pixels. Exercise borrowing across
        // the initial dictionary end, mutation replacement and ring wrapping.
        assert(!borrowed->borrowCorridor(20,160));
        const auto corridorOffset=pack.size();pack.resize(pack.size()+16*44);put(4,2);
        for(unsigned y=0;y<16;++y)for(unsigned w=0;w<22;++w)
            put(corridorOffset+y*44+w*2,0x8123+y*131+w*37);
        assert(initial.bind(pack.data(),pack.size()));
        assert(borrowed->borrowCorridor(20,160));
        assert(!borrowed->borrowCorridor(8192,1)&&!borrowed->borrowCorridor(8191,2));
        for(unsigned y=20;y<180;++y){
            UWORD scratch[22];
            std::memcpy(materialized->writeRows(1,y,1),initial.corridorRow(y,scratch),44);
        }
        compare();
        borrowed->invalidate(40,60);compare();
        std::fill_n(borrowed->writeRows(1,40,20),20*22,0xcafe);
        std::fill_n(materialized->writeRows(1,40,20),20*22,0xcafe);compare();
        assert(borrowed->borrowInitial(initial));
        for(unsigned c=0;c<3;++c)assert(initial.copy(c,materialized->writeRows(c,0,initialRows)));
        compare();
        std::printf("Borrowed initial raster: %u exact transfers, replacement and reborrow lifecycle\n",comparisons);
    }
    auto raster=std::make_unique<AmigaTerrainRaster>();
    std::vector<UWORD> pixels(AmigaTerrainRaster::storageBytes/2,0xdead);
    assert(!raster->hasStorage()&&!raster->ready);
    assert(!raster->bindStorage(nullptr,AmigaTerrainRaster::storageBytes));
    assert(!raster->bindStorage(pixels.data(),AmigaTerrainRaster::storageBytes-2));
    assert(raster->bindStorage(pixels.data(),AmigaTerrainRaster::storageBytes));
    assert(!raster->bindStorage(pixels.data(),AmigaTerrainRaster::storageBytes));
    raster->ready=true;raster->setDirty(1,100,true);
    raster->releaseStorage();
    assert(!raster->ready&&!raster->hasStorage());
    assert(raster->bindStorage(pixels.data(),AmigaTerrainRaster::storageBytes));
    assert(!raster->ready&&!raster->dirty(1,100));
    assert(pixels.front()==0xdead&&pixels.back()==0xdead); // No pointless pixel clearing.
    unsigned cases=0;
    unsigned partitions=0;
    for(unsigned column=0;column<3;++column)for(unsigned mask=0;mask<256;++mask){
        for(unsigned y=0;y<8192;++y)raster->setDirty(column,y,false);
        for(unsigned y=0;y<284;++y)raster->setDirty(column,7908+y,((mask>>(y%8))&1)!=0);
        unsigned first=7908,painted=0,copied=0;
        while(first<8192){
            const bool dirty=raster->dirty(column,first);
            const auto end=raster->runEnd(column,first,8192);
            assert(end>first&&end<=8192);
            for(unsigned y=first;y<end;++y){
                assert(raster->dirty(column,y)==dirty);
                // Simulate the compositor clearing a completed dirty run.
                raster->setDirty(column,y,false);
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
    unsigned stagedCases = 0;
    raster->ready = true;
    for (bool directlyCached : {false,true})
    for (unsigned column=0; column<3; ++column)
    for (unsigned origin : {0u, 309u}) for (unsigned mask=0; mask<32; ++mask) {
        std::vector<UBYTE> stage(downwellGameRingBytes, 0xa5), expected(stage);
        unsigned painted=0, expectedPainted=0, done=0;
        AmigaGameplayRows debts;
        debts.damage(0,0,320);
        for (unsigned y=0; y<48; ++y) {
            const bool dirty = (mask >> (y%5)) & 1;
            raster->setDirty(column,8000+y,dirty);
            expectedPainted += dirty;
            for (unsigned p=0;p<2;++p) for (unsigned w=0;w<11;++w) {
                const UWORD old = UWORD(23*y+57*p+w+column);
                raster->row(column,8000+y)[p*11+w]=old;
                const UWORD fresh = dirty ? old^0x55aa : old;
                std::memcpy(expected.data()+p*320*48+((origin+y)%320)*48+12+2*w,&fresh,2);
            }
        }
        while (done<48) {
            AmigaRenderer renderer{};
            renderer.game=renderer.target=stage.data();
            renderer.gameOrigin=(origin+done)%320;
            unsigned batchPainted=0;
            const int prepared=raster->prepareStage(&renderer,column,8000+done,48-done,debts,
                [&](unsigned top,unsigned height) {
                    batchPainted+=height;
                    for (unsigned y=top;y<top+height;++y) {
                        assert(raster->dirty(column,8000+done+y));
                        for (unsigned p=0;p<2;++p) {
                            const unsigned at=p*320*48+((origin+done+y)%320)*48+12;
                            if(directlyCached)
                                std::memcpy(raster->row(column,8000+done+y)+p*11,expected.data()+at,22);
                            else std::memcpy(stage.data()+at,expected.data()+at,22);
                        }
                    }
                    return directlyCached?AmigaTerrainRaster::stageInCache:AmigaTerrainRaster::stageInBacking;
                });
            assert(prepared>0 && unsigned(prepared)<=48-done && batchPainted<=16);
            for (int y=0;y<prepared;++y) assert(!raster->dirty(column,8000+done+y));
            painted+=batchPainted; done+=prepared;
        }
        assert(stage==expected && painted==expectedPainted);
        for(unsigned y=0;y<320;++y) assert(debts.dirty(y,0)&&debts.dirty(y,1));
        ++stagedCases;
    }
    AmigaRenderer failedStage{};
    AmigaGameplayRows untouchedDebts;
    raster->setDirty(0,100,true);
    assert(raster->prepareStage(&failedStage,0,100,1,untouchedDebts,
        [](unsigned,unsigned){return false;})==-1);
    assert(raster->dirty(0,100));
    assert(raster->prepareStage(&failedStage,3,100,1,untouchedDebts,
        [](unsigned,unsigned){assert(false);return true;})==-1);
    std::printf("Terrain staging: %u mixed/cold wrapped spans, <=16 repainted rows, untouched display debts\n",stagedCases);
    downwell::ShopWorld shops{};
    AmigaShopArtSnapshot shopSnapshot;
    shopSnapshot.capture(shops);
    shops.allocatedCount=1;
    shops.instances[0].allocated=true;
    for(int y : {-50, 0, 120, 8190, 8250}) {
        for(unsigned c=0;c<3;++c) for(unsigned r=0;r<8192;++r) raster->setDirty(c,r,false);
        auto& shop=shops.instances[0]; shop.y=y; ++shop.artRevision;
        assert(shopSnapshot.changed(0,shop));
        shopSnapshot.capture(shops);
        assert(!shopSnapshot.changed(0,shop));
        raster->invalidateShops(shops);
        for(unsigned c=0;c<3;++c) for(int r=0;r<8192;++r)
            assert(raster->dirty(c,r)==(r>=y-21 && r<y+12));
        for(unsigned c=0;c<3;++c) for(unsigned r=0;r<8192;++r) raster->setDirty(c,r,false);
        raster->invalidateShops(shops);
        for(unsigned c=0;c<3;++c) for(unsigned r=0;r<8192;++r) assert(!raster->dirty(c,r));
    }
    // A room reset followed by a first birth may reuse exactly the same art
    // revision. Both before-Step snapshots and raster invalidation must forget
    // the previous room, even across many empty updates.
    const unsigned previousArt=shops.instances[0].artRevision;
    shops.reset();
    for(unsigned tick=0;tick<3;++tick){
        shopSnapshot.capture(shops);raster->invalidateShops(shops);
    }
    shops.allocatedCount=1;shops.instances[0].allocated=true;
    shops.instances[0].y=500;shops.instances[0].artRevision=previousArt;
    assert(shopSnapshot.changed(0,shops.instances[0]));
    raster->invalidateShops(shops);
    for(unsigned c=0;c<3;++c)for(unsigned r=0;r<8192;++r)
        assert(raster->dirty(c,r)==(r>=479&&r<512));
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
    {
        // Ring storage: a slot holds one world row; claiming it dirties the
        // previous holder, runs stop at a lap, and clean transfers that cross
        // a lap read each row's own slot.
        constexpr unsigned ring=AmigaTerrainRaster::ringRows;
        auto ringRaster=std::make_unique<AmigaTerrainRaster>();
        std::vector<UWORD> storage(AmigaTerrainRaster::storageBytes/2,0);
        assert(ringRaster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes));
        const unsigned lap=2*ring-8; // first row of the third lap
        assert(AmigaTerrainRaster::contiguousRows(lap-40)==40&&AmigaTerrainRaster::contiguousRows(lap)==ring);
        assert(!ringRaster->writeRows(1,lap-40,41));
        for(unsigned y=lap-40;y<lap+40;++y){
            auto* row=ringRaster->writeRows(1,y,1);
            for(unsigned w=0;w<22;++w)row[w]=UWORD(y*7+w);
        }
        assert(ringRaster->runEnd(1,lap-40,lap+40)==lap&&ringRaster->runEnd(1,lap,lap+40)==lap+40);
        AmigaGameplayRows debts;
        std::vector<UBYTE> actual(downwellGameRingBytes,0x5a);
        AmigaRenderer renderer{};renderer.game=actual.data();renderer.gameOrigin=290;
        ringRaster->transfer(&renderer,1,int(lap-40),0,80,false,nullptr,nullptr,0,debts);
        for(unsigned y=0;y<80;++y)for(unsigned p=0;p<2;++p){
            const unsigned physical=(290+y)%320;
            assert(std::memcmp(actual.data()+p*downwellGameRingPlaneBytes+physical*48+12,
                ringRaster->readRow(1,lap-40+y,nullptr)+p*11,22)==0);
        }
        for(unsigned y=lap-40;y<lap+40;++y)assert(!ringRaster->dirty(1,y));
        (void)ringRaster->writeRows(1,lap-40+ring,8);
        for(unsigned y=lap-40;y<lap+40;++y)assert(ringRaster->dirty(1,y)==(y<lap-32));
        assert(!ringRaster->dirty(1,lap-40+ring));
        std::puts("Terrain raster ring: slot eviction, lap-bounded runs and cross-lap transfer passed");
    }
}
