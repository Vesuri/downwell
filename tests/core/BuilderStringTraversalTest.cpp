#include "core/BuilderStringTraversal.h"
#include "core/CatacombBuilderDispatch.h"
#include "core/GameMakerRandom.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
using namespace downwell;
static void require(bool ok){if(!ok)std::abort();}
int main(int argc,char** argv){
    require(argc==2);
    std::ifstream stream(argv[1],std::ios::binary);
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)),{});
    BuilderPatternPack pack(bytes.data(),bytes.size());require(pack.valid());
    // Geometry oracle lives in Python and splits the original strings into
    // lines, independently of this source-index character loop.
    for(unsigned id=0;id<pack.patternCount();++id)
    for(bool reverse:{false,true})for(bool wrap:{false,true})
    for(bool pillars:{false,true})for(unsigned extra:{0u,4u}){
        const std::uint8_t* text=nullptr;std::uint32_t length=0;
        require(pack.literalData(id,text,length));
        BuilderStringTraversal cursor;cursor.y=704;cursor.reversed=reverse;
        cursor.wrap=wrap;cursor.setPillar=pillars;
        std::cout<<"case "<<id<<' '<<reverse<<' '<<wrap<<' '<<pillars<<' '<<extra<<'\n';
        auto event=[](char kind,int symbol,int x,int y){
            std::cout<<kind<<' '<<symbol<<' '<<x<<' '<<y<<'\n';return true;
        };
        require(cursor.run(text,length,extra,
            [&](char c,int x,int y){return event('A',c,x,y);},
            [&](char c,int x,int y){return event('C',c,x,y);},
            [&](int x,int y){return event('P',60,x,y);}));
        std::cout<<"end "<<cursor.y<<'\n';
    }
    // Every failed callback retains its exact prefix, with no further work.
    const std::uint8_t sample[]="\r\n\t.j\r\n\t.e\r\n\t";
    unsigned total=0;
    for(unsigned failAt=0;failAt<=18;++failAt){
        BuilderStringTraversal cursor;cursor.y=704;
        unsigned calls=0;
        auto callback=[&](){++calls;return calls!=failAt;};
        const bool ok=cursor.run(sample,sizeof(sample)-1,0,
            [&](char,int,int){return callback();},[&](char,int,int){return callback();},
            [&](int,int){return callback();});
        if(!failAt)total=calls;
        require(ok==(failAt==0||failAt>total));
        if(!ok){require(calls==failAt&&!cursor.succeeded);const auto before=calls;
            require(!cursor.run(sample,sizeof(sample)-1,0,
                [&](char,int,int){return callback();},[&](char,int,int){return callback();},
                [&](int,int){return callback();})&&before==calls);
        }
    }
    // Real pack -> area-specific dispatcher. Retain the scalar per-cell
    // implementation as the RNG oracle; child callbacks consume the same stream.
    for(unsigned id=0;id<pack.patternCount();++id)for(unsigned level=1;level<=3;++level)for(bool hard:{false,true}){
        GameMakerRandom random(id+1),reference(id+1);
        CatacombBuilderDispatch actual,expected;
        BuilderStringTraversal cursor,oracle;cursor.y=oracle.y=704;
        std::vector<int> births,wanted;
        auto create=[&](int object,int x,int y){births.insert(births.end(),{object,x,y});random.nextU32();return true;};
        auto refCreate=[&](int object,int x,int y){wanted.insert(wanted.end(),{object,x,y});reference.nextU32();return true;};
        auto common=[](char,int,int){return true;};auto pillar=[](int,int){return true;};
        require(actual.pattern(pack,id,cursor,level,hard,4,random,create,common,pillar));
        const std::uint8_t* text=nullptr;std::uint32_t length=0;require(pack.literalData(id,text,length));
        require(oracle.run(text,length,4,[&](char c,int x,int y){return expected.cell(c,x,y,level,hard,reference,refCreate);},common,pillar));
        require(births==wanted && actual.spawnChance==expected.spawnChance && cursor.y==oracle.y);
        require(random.drawCount()==reference.drawCount());
        for(unsigned word=0;word<16;++word)require(random.stateWord(word)==reference.stateWord(word));
    }
    {
        BuilderStringTraversal cursor,nested;
        CatacombBuilderDispatch dispatch;
        GameMakerRandom random(1);
        auto create=[](int,int,int){return true;};
        auto pillar=[](int,int){return true;};
        auto common=[](char,int,int){return true;};
        unsigned calls=0;
        require(!dispatch.pattern(pack,0,cursor,1,false,0,random,create,
            [&](char,int,int){
                ++calls;
                require(!dispatch.pattern(pack,0,nested,1,false,0,random,create,common,pillar));
                return true; // Cannot swallow the nested failure.
            },pillar));
        require(calls==1&&!cursor.succeeded&&!dispatch.succeeded);
        const auto draws=random.drawCount();
        require(!dispatch.pattern(pack,0,cursor,1,false,0,random,create,common,pillar));
        require(random.drawCount()==draws);
    }
}
