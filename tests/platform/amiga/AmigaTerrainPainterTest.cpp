#include "platform/amiga/AmigaTerrainPainter.h"
#include "../../core/OwnedTerrainFixture.h"
#include "core/GameplayWorld.h"
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,
    UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
static void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
struct PaintLifecycle {
    template<class... A> bool birth(const A&...){return true;}
    template<class... A> bool retire(const A&...){return true;}
    template<class... A> bool activation(const A&...){return true;}
    template<class... A> bool membership(const A&...){return true;}
    template<class... A> bool move(const A&...){return true;}
    template<class... A> bool sprite(const A&...){return true;}
    template<class... A> bool imageIndex(const A&...){return true;}
    template<class... A> bool markDestroyed(const A&...){return true;}
};
int main(int argc,char** argv){
    require(argc==3,"usage: terrain-painter-test game.dwap phase6.bin");
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<UBYTE> pack{std::istreambuf_iterator<char>(input),{}};
    require(!pack.empty(),"game pack missing");
    std::ifstream preparedInput(argv[2],std::ios::binary);
    std::vector<UBYTE> prepared{std::istreambuf_iterator<char>(preparedInput),{}};
    require(AmigaTerrainPainter::bindPrepared(prepared.data(),prepared.size()),"prepared bank missing");
    static AmigaTerrainIndex index;
    unsigned seed=17,cases=0,directCases=0;
    auto next=[&](){seed=seed*1664525+1013904223;return seed;};
    for(auto& cell:index.cells)cell=next()>>24;
    std::vector<UBYTE> scratch(downwellTerrainBatchBytes);
    downwell::RoomPack rooms(nullptr,0);
    downwell::PackedRoom room{};
    static AmigaTerrainTiles tiles;
    {
        // Deliberately use frames unlike adjacency-derived frames, including
        // coincident walls. Retiring one lifetime must not erase the other or
        // make a later row rebuild resurrect the retired wall.
        PaintLifecycle lifecycle;
        auto records=std::make_unique<downwell::OwnedTerrainFixture>(
            downwell::GameplayTerrainInstances::Binding::bindDestruction(lifecycle));
        auto native=std::make_unique<GameplayWorld::NativeTerrain>();
        auto referenceWorld=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},records.get());
        auto nativeWorld=std::make_unique<GameplayWorld>(300,true,0,false);
        require(nativeWorld->bindNativeTerrain(*native),"bind native painting identities");
        downwell::GameMakerRandom random;
        struct Ops {
            downwell::GameplayTerrainInstances* records;downwell::InstanceHandle key;unsigned n=0,bits;
            bool place(int,int,int,bool& hit){hit=(bits>>n++)&1;return true;}
            bool point(int,int,int,bool& hit){hit=false;return true;}
            bool image(int frame){return !records||records->imageIndex(key,frame);}
            bool outOfView(){return true;}bool tile(int,int,int,int,int,int,int,int){return true;}
            bool destroy(){return false;}bool deactivate(){return false;}bool create(int,int,int){return true;}
        };
        for(unsigned i=0;i<4;++i){
            const int x=224+int(i/2)*16,y=192+int(i/3)*16;
            require(records->append(60,x,y,true)&&native->append(60,x,y),"painting fixture births");
            const auto r=records->handle(i),n=native->handle(i);
            require(records->createProperties(r,51,false,random)&&native->createProperties(n,51),"painting fixture Create");
            Ops ro{records.get(),r,0,i+2},no{nullptr,n,0,i+2};
            require(records->wallEvent(r,1,random,ro)&&native->wallEvent(n,1,random,no),"select retained frames");
        }
        static AmigaTerrainTiles empty;
        for(unsigned pass=0;pass<5;++pass){
            if(pass==1||pass==2)require(records->activation(records->handle(0),pass==2)&&
                native->activation(native->handle(0),pass==2),"toggle one coincident wall");
            if(pass==3)require(records->requestDestroy(records->handle(0))&&native->markDestroyed(native->handle(0)),"mark one coincident wall");
            if(pass==4)require(native->retire(native->handle(0)),"retire marked native wall");
            std::vector<UBYTE> a(downwellGameRingBytes),b(a.size()),ha(downwellLayerBytes),hb(ha.size());
            AmigaRenderer reference{},candidate{};
            amigaRendererInit(&reference,a.data(),ha.data());amigaRendererInit(&candidate,b.data(),hb.data());
            require(!AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),nullptr,0),"unbind owned reference");
            require(AmigaTerrainPainter::paint(&reference,rooms,room,nullptr,empty,nullptr,0,referenceWorld.get(),
                pack.data(),pack.data()+pack.size(),160,176,110,0,160,48),"owned reference paint");
            require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"bind owned batch");
            require(AmigaTerrainPainter::paint(&candidate,rooms,room,nullptr,empty,nullptr,0,nativeWorld.get(),
                pack.data(),pack.data()+pack.size(),160,176,110,0,160,48),"native retained paint");
            require(a==b&&ha==hb,"native retained wall pixels differ");
            require(std::any_of(a.begin(),a.end(),[](UBYTE b){return b!=0;}),"empty retained wall fixture");
            std::vector<UWORD> rows(48*22);
            require(AmigaTerrainPainter::paintCached(rows.data(),&candidate,rooms,room,nullptr,empty,nullptr,0,
                nativeWorld.get(),pack.data(),pack.data()+pack.size(),160,176,0,48),"retained direct-cache paint");
            for(unsigned y=0;y<48;++y)for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w){
                const unsigned offset=p*downwellGameRingPlaneBytes+y*48+12+w*2;
                require(rows[y*22+p*11+w]==UWORD((unsigned(a[offset])<<8)|a[offset+1]),"retained cached pixel mismatch");
            }
        }
    }
    for(int y=0;y<8192;y+=16){
        downwell::DeferredCreateInstance command{92,160+int(next()%11)*16,y};
        command.integerImageIndex=next()%26;
        require(tiles.append(command),"tile fixture");
    }
    const int clips[][4]={{110,0,160,284},{0,0,380,284},{112,9,1,263},
        {0,12,380,1},{-3,-7,390,300},{100,267,171,13},{0,0,0,284},
        {110,13,160,1},{110,13,160,3},{110,267,160,16}};
    for(int vx:{-520,-512,-16,0,1,159,160,161,319,320,1530})
    for(int vy:{-16,-1,0,1,175,176,177,4000,8177,8192})
    for(const auto& clip:clips){
        std::vector<UBYTE> a(downwellGameRingBytes+32),b,ha(downwellLayerBytes+32),hb;
        for(auto& v:a)v=next()>>24;
        for(auto& v:ha)v=next()>>24;
        b=a;hb=ha;
        AmigaRenderer reference{},candidate{};
        amigaRendererInit(&reference,a.data()+16,ha.data()+16);
        amigaRendererInit(&candidate,b.data()+16,hb.data()+16);
        const unsigned origin=next()%320;
        amigaRendererSetGameOrigin(&reference,origin);amigaRendererSetGameOrigin(&candidate,origin);
        require(!AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),nullptr,0),"unbind");
        require(AmigaTerrainPainter::paint(&reference,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),vx,vy,clip[0],clip[1],clip[2],clip[3]),"reference paint");
        require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"bind");
        require(AmigaTerrainPainter::paint(&candidate,rooms,room,&index,tiles,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),vx,vy,clip[0],clip[1],clip[2],clip[3]),"batch paint");
        if(a!=b||ha!=hb){std::cerr<<"viewport mismatch "<<vx<<','<<vy<<" clip "<<clip[0]<<','<<clip[1]<<','<<clip[2]<<','<<clip[3]<<'\n';return 1;}
        if(clip[0]==110&&clip[2]==160){
            std::fill(a.begin(),a.end(),0);
            require(!AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),nullptr,0),"unbind direct oracle");
            require(AmigaTerrainPainter::paint(&reference,rooms,room,&index,tiles,nullptr,0,nullptr,
                pack.data(),pack.data()+pack.size(),vx,vy,clip[0],clip[1],clip[2],clip[3]),"direct oracle");
            require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"bind direct");
            std::vector<UWORD> rows(clip[3]*22+16,0xbeef);
            const auto before=b,beforeHud=hb;
            require(AmigaTerrainPainter::paintCached(rows.data()+8,&candidate,rooms,room,&index,tiles,
                nullptr,0,nullptr,pack.data(),pack.data()+pack.size(),vx,vy,clip[1],clip[3]),"direct paint");
            require(b==before&&hb==beforeHud,"direct paint touched Chip buffers");
            for(unsigned i=0;i<8;++i)require(rows[i]==0xbeef&&rows[rows.size()-1-i]==0xbeef,"direct guard");
            for(int y=0;y<clip[3];++y)for(int plane=0;plane<2;++plane)for(int word=0;word<11;++word){
                const unsigned offset=16+plane*downwellGameRingPlaneBytes+
                    ((origin+clip[1]+y)%320)*48+12+word*2;
                const UWORD expected=UWORD((unsigned(a[offset])<<8)|a[offset+1]);
                require(rows[8+y*22+plane*11+word]==expected,"direct cached pixel mismatch");
            }
            // A failed transaction may alter the still-dirty cache, never
            // either Chip buffer. The caller must replace it with the oracle.
            const AmigaTerrainDecoration mirrored{0,51,vx+80,vy+clip[1]+8,0,true};
            require(!AmigaTerrainPainter::paintCached(rows.data()+8,&candidate,rooms,room,&index,tiles,
                &mirrored,1,nullptr,pack.data(),pack.data()+pack.size(),vx,vy,clip[1],clip[3]),"unsupported direct art accepted");
            require(b==before&&hb==beforeHud,"failed direct paint touched Chip buffers");
            // Failure must not leave the global painter in direct mode.
            require(AmigaTerrainPainter::paintCached(rows.data()+8,&candidate,rooms,room,&index,tiles,
                nullptr,0,nullptr,pack.data(),pack.data()+pack.size(),vx,vy,clip[1],clip[3]),"direct retry remained failed");
            ++directCases;
        }
        ++cases;
    }
    // Exercise every prepared frame independently, not only the subset seen
    // in sampled room windows or frames whose errors might be overpainted.
    for(unsigned frame=0;frame<26;++frame){
        AmigaTerrainTiles single;
        downwell::DeferredCreateInstance command{92,240,176};
        command.integerImageIndex=frame;
        require(single.append(command),"single prepared frame fixture");
        std::vector<UBYTE> game(downwellGameRingBytes),hud(downwellLayerBytes);
        AmigaRenderer renderer{};
        amigaRendererInit(&renderer,game.data(),hud.data());
        require(!AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),nullptr,0),"unbind frame oracle");
        require(AmigaTerrainPainter::paint(&renderer,rooms,room,nullptr,single,nullptr,0,nullptr,
            pack.data(),pack.data()+pack.size(),160,160,110,0,160,40),"frame oracle");
        require(AmigaTerrainPainter::bindBatch(pack.data(),pack.data()+pack.size(),scratch.data(),scratch.size()),"bind frame");
        std::vector<UWORD> rows(40*22);
        require(AmigaTerrainPainter::paintCached(rows.data(),&renderer,rooms,room,nullptr,single,
            nullptr,0,nullptr,pack.data(),pack.data()+pack.size(),160,160,0,40),"prepared frame");
        for(unsigned y=0;y<40;++y)for(unsigned p=0;p<2;++p)for(unsigned w=0;w<11;++w){
            const unsigned offset=p*downwellGameRingPlaneBytes+y*48+12+w*2;
            require(rows[y*22+p*11+w]==UWORD((unsigned(game[offset])<<8)|game[offset+1]),"prepared frame mismatch");
        }
    }
    std::cout<<"All 26 prepared frames independently match original planar artwork\n";
    std::cout<<cases<<" full terrain windows match scalar painter geometry and pixels\n";
    std::cout<<directCases<<" direct-cache spans match without touching Chip buffers\n";
}
