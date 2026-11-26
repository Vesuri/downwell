#include "platform/amiga/AmigaPatternTerrain.h"
#include "platform/amiga/AmigaTerrainPainter.h"
#include "platform/amiga/AmigaTerrainRunSurface.h"
#include "platform/amiga/AmigaTerrainRaster.h"
#include "platform/amiga/framework/Bitmap.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <cstdlib>
#include <memory>

int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,
    UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
void AmigaHardware::blitterDrain(){std::abort();}
void AmigaHardware::setBlitterNasty(bool){std::abort();}
void Bitmap::copy(const Bitmap&,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t){std::abort();}
static void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
static std::vector<UBYTE> read(const char* path){
    std::ifstream in(path,std::ios::binary);require(bool(in),"missing input");
    return {std::istreambuf_iterator<char>(in),{}};
}
static unsigned word(const UBYTE* p){return unsigned(p[0])*256+p[1];}
int main(int argc,char** argv){
    require(argc==3,"usage: pattern-terrain-test game.dwap pattern-terrain.bin");
    auto pack=read(argv[1]),bytes=read(argv[2]);
    AmigaPatternTerrain bank;require(bank.bind(bytes.data(),bytes.size()),"bank rejected");
    static AmigaTerrainIndex index;
    static AmigaTerrainTiles tiles;
    downwell::RoomPack rooms(nullptr,0);downwell::PackedRoom room{};
    std::vector<UBYTE> game(downwellGameRingBytes),hud(downwellLayerBytes);
    std::vector<UBYTE> scratch(downwellTerrainBatchBytes);
    auto raster=std::make_unique<AmigaTerrainRaster>();
    std::vector<UWORD> storage(AmigaTerrainRaster::storageBytes/2);
    require(raster->bindStorage(storage.data(),AmigaTerrainRaster::storageBytes),"borrowed raster storage");
    AmigaRenderer renderer{};amigaRendererInit(&renderer,game.data(),hud.data());
    require(AmigaTerrainPainter::bindPatterns(bytes.data(),bytes.size()),"painter bank rejected");
    // No terrain batch bound: the original scalar masked sprite painter is
    // independent of both this bank and the generator's Python pixel logic.
    const unsigned count=word(bytes.data()+6);
    unsigned checked=0,misses=0;
    auto check=[&](unsigned middle,unsigned above,unsigned below){
        const auto* entry=bank.find(middle,above,below);
        if(!entry)++misses;
        std::fill(std::begin(index.cells),std::end(index.cells),0);
        const unsigned masks[]={above,middle,below};
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<13;++col)
            if(masks[row]&(1u<<col))index.cells[(row+1)*128+41+col]=24;
        std::fill(game.begin(),game.end(),0);
        (void)AmigaTerrainPainter::bindBatch(nullptr,nullptr,nullptr,0);
        require(AmigaTerrainPainter::paint(&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),160,24,107,0,165,16),"scalar paint failed");
        UWORD actual[16*22+2];std::fill(std::begin(actual),std::end(actual),0xbeef);
        require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"batch bind");
        require(AmigaTerrainPainter::paintCached(actual+1,&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),160,24,0,16),"prepared/fallback paint");
        for(unsigned row=0;row<16;++row)for(unsigned plane=0;plane<2;++plane)for(unsigned w=0;w<11;++w){
            const unsigned offset=plane*downwellGameRingPlaneBytes+row*48+12+w*2;
            require(actual[1+row*22+plane*11+w]==word(game.data()+offset),"prepared wall pixels differ");
        }
        require(actual[0]==0xbeef&&actual[16*22+1]==0xbeef,"destination overrun");
        const bool borrowed=AmigaTerrainPainter::borrowPatternRows(*raster,index,tiles,nullptr,0,nullptr,160,24,16);
        require(borrowed==bool(entry),"borrow admission differs from exact row lookup");
        if(borrowed){
            require(raster->borrowedRunEnd(1,24,40)==40,"immutable strip lost contiguous storage");
            for(unsigned y=0;y<16;++y){
                UWORD row[22];raster->setDirty(1,24+y,true);
                require(raster->dirty(1,24+y),"borrowed row lost damage");
                const auto* source=raster->readRow(1,24+y,row);
                require(std::equal(source,source+22,actual+1+y*22),"borrowed row pixels differ");
                raster->setDirty(1,24+y,false);
            }
            // A mutable fix-up must sever the immutable row view, not paint
            // into the source bank or retain a stale pointer after replacement.
            std::fill_n(raster->writeRows(1,24,16),16*22,0xabcd);
            UWORD row[22];require(raster->readRow(1,24,row)[0]==0xabcd,"mutable row retained borrowed source");
        }
        for(unsigned first=0;entry&&first<16;++first){
            UWORD partial[16*22];bank.copy(entry,first,16-first,partial);
            require(std::equal(partial,partial+(16-first)*22,actual+1+first*22),"partial row differs");
        }
        ++checked;
    };
    for(unsigned i=0;i<count;++i){
        const auto* entry=bytes.data()+AmigaPatternTerrain::headerBytes+i*AmigaPatternTerrain::entryBytes;
        const unsigned m=word(entry),a=word(entry+2),b=word(entry+4);
        require(bank.find(m,a,b)==entry+6,"lookup misses prepared row");
        check(m,a,b);
        // Destroy/create each wall and each seam neighbour. Hits must match;
        // misses must decline rather than reuse an approximate pattern.
        for(unsigned bit=0;bit<13;++bit){
            check(m^(1u<<bit),a,b);check(m,a^(1u<<bit),b);check(m,a,b^(1u<<bit));
        }
    }
    require(misses&&checked>=count,"mutation/fallback control absent");
    // Paint-ahead publishes disjoint four-line pieces. Live tile overlays
    // must be identical to one scalar pass even when a later mutation makes
    // the previously prepared piece dirty again.
    unsigned staged=0;
    std::fill(std::begin(index.cells),std::end(index.cells),0);
    for(unsigned row=1;row<=3;++row)for(unsigned col:{42u,52u})index.cells[row*128+col]=24;
    for(unsigned frame=16;frame<26;++frame){
        tiles.clear();
        require(tiles.appendCreatedTile(92,192,32,frame),"tile fixture admission");
        require(!AmigaTerrainPainter::borrowPatternRows(*raster,index,tiles,nullptr,0,nullptr,160,24,16),
            "live tile overlay admitted immutable wall-only pixels");
        std::fill(game.begin(),game.end(),0);
        (void)AmigaTerrainPainter::bindBatch(nullptr,nullptr,nullptr,0);
        require(AmigaTerrainPainter::paint(&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),160,24,107,0,165,16),"scalar tile fixture");
        require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"batch bind");
        for(unsigned first=0;first<16;first+=4){
            UWORD actual[4*22];
            require(AmigaTerrainPainter::paintCached(actual,&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
                pack.data(),pack.data()+pack.size(),160,24+first,0,4),"partial live fix-up");
            for(unsigned y=0;y<4;++y)for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w)
                require(actual[y*22+p*11+w]==word(game.data()+p*downwellGameRingPlaneBytes+(first+y)*48+12+w*2),
                    "partial live fix-up differs from scalar");
            ++staged;
        }
        raster->invalidate(24,40);
        unsigned pieces=0;
        for(unsigned y=raster->nextPreparation(24,40);y<40;y=raster->nextPreparation(24,40)){
            if(raster->needsPatternMaterialization(y)){
                require(raster->materializePatternRows(y,8)>0,"borrowed row materialization progresses");
                continue;
            }
            require(++pieces<=3,"prepared row made no bounded progress");
            require(AmigaTerrainPainter::preparePatternPiece(*raster,index,tiles,nullptr,0,nullptr,160,y),
                "prepared row admission/piece failed");
        }
        const bool borrowed=frame==16||frame==17||frame==22||frame==23;
        require(pieces==(borrowed?1u:3u),"grouped rows borrow strips; other rows need two bounded pieces");
        for(unsigned y=0;y<16;++y){
            UWORD scratch[22];const auto* row=raster->readRow(1,24+y,scratch);
            for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w)
                require(row[p*11+w]==word(game.data()+p*downwellGameRingPlaneBytes+y*48+12+w*2),
                    "retained row plan differs after tile mutation");
        }
    }
    for(unsigned overlap=0;overlap<2;++overlap){
        tiles.clear();
        require(tiles.appendCreatedTile(92,overlap?192:160,32,16),"overlap fixture admission");
        if(overlap)require(tiles.appendCreatedTile(92,192,32,17),"duplicate tile fixture admission");
        std::fill(game.begin(),game.end(),0);
        (void)AmigaTerrainPainter::bindBatch(nullptr,nullptr,nullptr,0);
        require(AmigaTerrainPainter::paint(&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),160,24,107,0,165,16),"scalar overlap fixture");
        require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"batch bind");
        raster->invalidate(24,40);
        unsigned pieces=0;
        for(unsigned y=raster->nextPreparation(24,40);y<40;y=raster->nextPreparation(24,40)){
            require(++pieces<=3&&AmigaTerrainPainter::preparePatternPiece(*raster,index,tiles,nullptr,0,nullptr,160,y),
                "overlap materialization failed");
        }
        require(pieces==3,"overlapping wall/tile or tile/tile was incorrectly admitted as an OR composition");
        for(unsigned y=0;y<16;++y){
            UWORD decoded[22];const auto* row=raster->readRow(1,24+y,decoded);
            for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w)
                require(row[p*11+w]==word(game.data()+p*downwellGameRingPlaneBytes+y*48+12+w*2),
                    "overlap fallback changed scalar painter order");
        }
    }
    unsigned decorationEdges=0;
    for(unsigned sprite:{139u,550u,568u,569u})for(unsigned frame=0;frame<17;++frame)
    for(bool flip:{false,true}){
        AmigaPatternTerrain::Overlay art;
        if(!bank.overlay(sprite,frame,flip,art))continue;
        // A bitmap just touching the row is absent; moving it one pixel
        // inward must restore masked composition, for every offline variant.
        for(int top:{24-int(art.height),25-int(art.height),39,40})for(unsigned tile=0;tile<2;++tile){
            tiles.clear();
            if(tile)require(tiles.appendCreatedTile(92,192,32,16),"decoration tile admission");
            const AmigaTerrainDecoration deco{0,int(sprite),192,top+art.originY,int(frame),flip};
            std::fill(game.begin(),game.end(),0);
            (void)AmigaTerrainPainter::bindBatch(nullptr,nullptr,nullptr,0);
            require(AmigaTerrainPainter::paint(&renderer,rooms,room,&index,tiles,&deco,1,nullptr,
                pack.data(),pack.data()+pack.size(),160,24,107,0,165,16),"scalar decoration boundary");
            require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"batch bind");
            raster->invalidate(24,40);
            unsigned pieces=0;
            for(unsigned y=raster->nextPreparation(24,40);y<40;y=raster->nextPreparation(24,40)){
                if(raster->needsPatternMaterialization(y)){
                    require(raster->materializePatternRows(y,8)>0,"decorated boundary materialization progresses");
                    continue;
                }
                require(++pieces<=3&&AmigaTerrainPainter::preparePatternPiece(*raster,index,tiles,&deco,1,nullptr,160,y),
                    "decoration boundary preparation");
            }
            const bool touches=top<40&&top+int(art.height)>24;
            require(pieces==(touches?3u:1u),"non-intersecting decoration forced raster materialization");
            for(unsigned y=0;y<16;++y){
                UWORD decoded[22];const auto* row=raster->readRow(1,24+y,decoded);
                for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w)
                    require(row[p*11+w]==word(game.data()+p*downwellGameRingPlaneBytes+y*48+12+w*2),
                        "decoration boundary pixels differ from scalar");
            }
            ++decorationEdges;
        }
    }
    std::cout<<decorationEdges<<" decoration edge/mutation cases preserve pixels and skip absent composition\n";
    tiles.clear();
    for(unsigned i=0;i<33;++i)require(tiles.appendCreatedTile(92,192,32,16),"overflow fixture admission");
    raster->invalidate(24,40);
    UWORD before[22],after[22];
    const auto* old=raster->readRow(1,24,before);
    std::copy_n(old,22,before);
    require(!AmigaTerrainPainter::preparePatternPiece(*raster,index,tiles,nullptr,0,nullptr,160,24),
        "oversized row plan must retain the live fallback");
    const auto* retained=raster->readRow(1,24,after);
    require(std::equal(before,before+22,retained),"failed plan changed pixels");
    for(unsigned y=24;y<40;++y)require(raster->dirty(1,y),"failed plan acknowledged damage");
    tiles.clear();
    unsigned overlays=0;
    unsigned groupCases=0;
    for(unsigned code=0;code<625;++code){
        const auto* group=bank.tileGroup(code);
        if(!group)continue;
        for(unsigned column:{0u,4u,8u}){
            std::fill(game.begin(),game.end(),0);
            amigaRendererSetClipRect(&renderer,107,0,165,16);
            unsigned digits=code;
            for(unsigned cell=0;cell<4;++cell,digits/=5){
                const unsigned digit=digits%5;
                if(!digit)continue;
                const unsigned frames[]={16,17,22,23};
                amigaRendererDrawAsset(&renderer,pack.data(),pack.data()+pack.size(),51,frames[digit-1],
                    110+int(column+cell)*16,8,0);
            }
            for(unsigned first:{0u,8u}){
                UWORD actual[8*22]{};
                AmigaTerrainRunSurface surface(actual,first,8,107,165,nullptr);
                surface.addTileGroup(group,column);
                for(unsigned y=0;y<8;++y)for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w)
                    require(actual[y*22+p*11+w]==word(game.data()+p*downwellGameRingPlaneBytes+(first+y)*48+12+w*2),
                        "offline tile group differs from scalar sprites");
                ++groupCases;
            }
        }
    }
    for(unsigned sprite:{139u,550u,568u,569u})for(unsigned frame=0;frame<17;++frame)
    for(bool flip:{false,true}){
        AmigaPatternTerrain::Overlay art;
        if(!bank.overlay(sprite,frame,flip,art))continue;
        for(int anchorX=94;anchorX<=302;anchorX+=16)for(int anchorY:{-8,0,7,16,31,40}){
            std::fill(game.begin(),game.end(),0x5a);
            amigaRendererSetClipRect(&renderer,107,0,165,32);
            amigaRendererDrawAsset(&renderer,pack.data(),pack.data()+pack.size(),sprite,frame,anchorX,anchorY,flip);
            UWORD actual[32*22];std::fill(std::begin(actual),std::end(actual),0x5a5a);
            AmigaTerrainRunSurface surface(actual,0,32,107,165,nullptr);
            require(surface.overlay(art,anchorX,anchorY),"prepared overlay alignment");
            for(unsigned y=0;y<32;++y)for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w)
                require(actual[y*22+p*11+w]==word(game.data()+p*downwellGameRingPlaneBytes+y*48+12+w*2),
                    "prepared overlay pixels differ");
            ++overlays;
        }
    }
    require(!bank.bind(bytes.data(),bytes.size()-1)&&!bank.find(2050,2050,2050),"truncation retained stale bank");
    auto bad=bytes;bad[AmigaPatternTerrain::headerBytes+6]=bad[AmigaPatternTerrain::headerBytes+7]=255;
    require(!bank.bind(bad.data(),bad.size()),"invalid raster reference admitted");
    bad=bytes;
    const unsigned groupCount=word(bytes.data()+12);
    const unsigned groupDirectory=unsigned(bytes.size())-1250-groupCount*320;
    bad[groupDirectory]=groupCount>>8;bad[groupDirectory+1]=groupCount;
    require(!bank.bind(bad.data(),bad.size()),"invalid tile group reference admitted");
    bank.release();require(!bank.find(0,0,0),"released bank retained pixels");
    std::cout<<checked<<" pattern/seam/mutation rows match scalar pixels; "<<misses<<" exact fallback misses; "
        <<overlays<<" clipped/flipped overlay cases; "<<staged<<" staged tile fix-ups; "
        <<groupCases<<" clipped tile-group strips\n";
}
