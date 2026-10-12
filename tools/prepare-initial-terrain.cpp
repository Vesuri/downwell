#include "platform/amiga/AmigaTerrainPainter.h"
#include "platform/amiga/AmigaInitialTerrain.h"
#include <array>
#include <algorithm>
#include <map>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <cstdlib>

// Host scalar renderer: hardware hooks deliberately decline all DMA paths.
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,
    UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
static void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
static std::vector<UBYTE> read(const char* path){
    std::ifstream input(path,std::ios::binary);require(bool(input),"missing terrain input");
    return {std::istreambuf_iterator<char>(input),{}};
}
int main(int argc,char** argv){
    require(argc==5,"usage: prepare-initial-terrain game.dwap phase6.bin rooms.dwrm output.bin");
    const auto pack=read(argv[1]),phase=read(argv[2]),roomBytes=read(argv[3]);
    downwell::RoomPack rooms(roomBytes.data(),roomBytes.size());downwell::PackedRoom room;
    require(rooms.valid()&&rooms.findRoom(11,room),"initial room resource");
    static AmigaTerrainIndex index;static AmigaTerrainTiles tiles;
    require(amigaBuildTerrainIndex({},index),"empty terrain");
    int rows=0;
    for(unsigned i=0;i<room.instanceCount;++i){
        downwell::PackedRoomInstance instance;require(rooms.roomInstance(room,i,instance),"room record");
        if(instance.objectId!=60&&instance.objectId!=93&&instance.objectId!=99)continue;
        require(amigaAddTerrainObject(index,instance.objectId,instance.x,instance.y,true),"room terrain publication");
        if(instance.objectId==60&&instance.y+32>rows)rows=instance.y+32;
    }
    require(rows>0&&rows<=8192,"initial raster extent");
    std::vector<UBYTE> game(downwellGameRingBytes),hud(downwellLayerBytes),scratch(downwellTerrainBatchBytes);
    AmigaRenderer renderer{};amigaRendererInit(&renderer,game.data(),hud.data());
    require(AmigaTerrainPainter::bindPrepared(phase.data(),phase.size()),"terrain phase bank");
    std::vector<UWORD> raster(3*unsigned(rows)*22);
    for(unsigned column=0;column<3;++column)for(int y=0;y<rows;y+=284){
        const int height=std::min(284,rows-y);
        auto* target=raster.data()+(column*rows+y)*22;
        require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"bind prepared painter");
        require(AmigaTerrainPainter::paintCached(target,&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),column*160,y,0,height),"compose initial rows");
        std::fill(game.begin(),game.end(),0);
        require(!AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),nullptr,0),"unbind scalar oracle");
        require(AmigaTerrainPainter::paint(&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),column*160,y,110,0,160,height),"scalar initial oracle");
        for(int line=0;line<height;++line)for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w){
            const unsigned offset=p*downwellGameRingPlaneBytes+line*48+12+w*2;
            require(target[line*22+p*11+w]==UWORD(unsigned(game[offset])*256+game[offset+1]),"initial scalar pixel mismatch");
        }
    }
    // A periodic raster, not pregenerated game geometry. Runtime admission
    // must prove this exact live wall neighbourhood before borrowing a row.
    require(amigaBuildTerrainIndex({},index),"empty corridor fixture");
    for(int y=0;y<=80;y+=16)for(int x:{160,320})
        require(amigaAddTerrainObject(index,60,x,y,true),"corridor fixture wall");
    require(AmigaTerrainPainter::corridorRows(index,tiles,32,48),"corridor admission");
    for(unsigned y=1;y<=4;++y)for(unsigned x=41;x<=53;++x){
        auto& cell=index.cells[y*AmigaTerrainIndex::columns+x];const auto saved=cell;
        cell^=AmigaTerrainIndex::wallAppearanceFlags;
        require(!AmigaTerrainPainter::corridorRows(index,tiles,32,48),"changed neighbourhood accepted as corridor");
        cell=saved;
    }
    require(tiles.appendPlatforms(240,32,16,1),"corridor platform fixture");
    require(!AmigaTerrainPainter::corridorRows(index,tiles,32,48),"platform accepted as empty corridor");
    tiles.clear();
    require(!AmigaTerrainPainter::corridorRows(index,tiles,0,16)&&
        !AmigaTerrainPainter::corridorRows(index,tiles,48,32)&&
        !AmigaTerrainPainter::corridorRows(index,tiles,8160,8192),"unbounded corridor admitted");
    std::array<UWORD,16*22> corridor;
    require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"corridor painter");
    require(AmigaTerrainPainter::paintCached(corridor.data(),&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
        pack.data(),pack.data()+pack.size(),160,32,0,16),"compose corridor");
    require(!AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),nullptr,0),"corridor scalar oracle");
    for(unsigned phaseY=0;phaseY<16;++phaseY){
        std::fill(game.begin(),game.end(),0);
        require(AmigaTerrainPainter::paint(&renderer,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),160,32+phaseY,110,0,160,16),"scalar corridor oracle");
        for(unsigned y=0;y<16;++y)for(unsigned plane=0;plane<2;++plane)for(unsigned w=0;w<11;++w){
            const unsigned offset=plane*downwellGameRingPlaneBytes+y*48+12+w*2;
            require(corridor[((y+phaseY)&15)*22+plane*11+w]==UWORD(unsigned(game[offset])*256+game[offset+1]),
                "periodic corridor scalar pixel mismatch");
        }
    }
    using Row=std::array<UWORD,22>;
    std::map<Row,unsigned> dictionary;std::vector<Row> entries;std::vector<unsigned> references;
    for(unsigned i=0;i<3*unsigned(rows);++i){
        Row row;std::copy_n(raster.data()+i*22,22,row.begin());
        auto found=dictionary.find(row);
        if(found==dictionary.end()){const unsigned id=entries.size();dictionary.emplace(row,id);entries.push_back(row);references.push_back(id);}
        else references.push_back(found->second);
    }
    require(entries.size()<65536,"initial row dictionary size");
    std::vector<UBYTE> output{'D','W','I','T'};
    auto word=[&](unsigned value){output.push_back(value>>8);output.push_back(value);};
    word(2);word(rows);word(entries.size());word(0);
    for(auto value:references)word(value);
    for(const auto& row:entries)for(auto value:row)word(value);
    for(auto value:corridor)word(value);
    AmigaInitialTerrain bank;require(bank.bind(output.data(),output.size()),"generated initial bank rejected");
    std::vector<UWORD> actual(raster.size()+2,0xbeef);
    for(unsigned column=0;column<3;++column)require(bank.copy(column,actual.data()+1+column*rows*22),"decode initial column");
    require(std::equal(raster.begin(),raster.end(),actual.begin()+1)&&actual.front()==0xbeef&&actual.back()==0xbeef,"decoded initial raster differs");
    for(unsigned y=0;y<32;++y){
        UWORD scratchRow[22];const auto* decoded=bank.corridorRow(y,scratchRow);
        require(decoded&&std::equal(decoded,decoded+22,corridor.data()+(y&15)*22),"decoded corridor differs");
    }
    require(!bank.copy(3,actual.data())&&!bank.copy(0,nullptr),"invalid destination accepted");
    require(!bank.bind(output.data(),output.size()-1)&&!bank.copy(0,actual.data()),"truncation retains stale bank");
    auto bad=output;bad[12]=bad[13]=255;require(!bank.bind(bad.data(),bad.size()),"invalid row reference accepted");
    std::ofstream file(argv[4],std::ios::binary);file.write(reinterpret_cast<const char*>(output.data()),output.size());
    require(bool(file),"initial terrain output failed");
    std::cout<<"Initial terrain: "<<rows<<" rows x 3, "<<entries.size()<<" unique rows, "<<output.size()
        <<" bytes; all scalar and decoded pixels match\n";
}
