#include "platform/amiga/AmigaRenderer.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,
    UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int);
static unsigned blits;
// Independent pixel model of transfer coordinates, not the Bitmap blitter.
int amigaBlitterDrawPlanarMasked(const UBYTE* source,const UBYTE* mask,UWORD sw,UWORD sh,
    UWORD stride,UBYTE* dest,UWORD dh,UWORD dx,UWORD dy,UWORD sx,UWORD sy,
    UWORD width,UWORD height,int pen){
    if(pen!=-1 || sx+width>sw || sy+height>sh || dx+width>380 || dy+height>dh)
        std::abort();
    ++blits;
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
        const unsigned si=(sy+y)*(stride/8)+(sx+x)/8,sm=0x80u>>((sx+x)&7);
        if(!(mask[si]&sm))continue;
        const unsigned di=(dy+y)*48+(dx+x)/8,dm=0x80u>>((dx+x)&7);
        for(unsigned plane=0;plane<2;++plane){
            auto& d=dest[di+plane*dh*48];
            d=(d&~dm)|((source[si+plane*sh*(stride/8)]&sm)?dm:0);
        }
    }
    return 1;
}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
static void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(int argc,char** argv){
    require(argc==2,"usage: terrain-batch-test game.dwap");
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<UBYTE> pack{std::istreambuf_iterator<char>(input),{}};
    require(!pack.empty(),"game pack missing");
    std::vector<UBYTE> scratch(downwellTerrainBatchBytes+32,0xaa);
    AmigaTerrainBatch batch{};
    require(amigaRendererBindTerrainBatch(&batch,pack.data(),pack.data()+pack.size(),
        scratch.data()+16,downwellTerrainBatchBytes),"tile binding failed");
    unsigned seed=0x44574150,cases=0;
    auto next=[&](){seed=seed*1664525+1013904223;return seed;};
    const int clips[][4]={{0,0,380,284},{110,0,160,284},{112,9,1,263},
        {0,12,380,1},{-3,-7,390,300},{100,267,171,13},
        {0,0,0,284},{400,0,5,100},{0,300,100,3},{-30,0,5,100}};
    for(int px=0;px<16;++px)for(int py=0;py<16;++py)
    for(unsigned origin:{0u,307u,319u})for(bool hud:{false,true})
    for(bool dma:{false,true})for(const auto& clip:clips){
        std::vector<UBYTE> a(downwellGameRingBytes+32),b,ha(downwellLayerBytes+32),hb;
        for(auto& v:a)v=next()>>24;
        for(auto& v:ha)v=next()>>24;
        b=a;hb=ha;
        AmigaRenderer reference{},candidate{};
        amigaRendererInit(&reference,a.data()+16,ha.data()+16);
        amigaRendererInit(&candidate,b.data()+16,hb.data()+16);
        if(dma)amigaRendererEnableBlitter(&candidate);
        amigaRendererSetGameOrigin(&reference,origin);amigaRendererSetGameOrigin(&candidate,origin);
        amigaRendererSetClipRect(&reference,clip[0],clip[1],clip[2],clip[3]);
        amigaRendererSetClipRect(&candidate,clip[0],clip[1],clip[2],clip[3]);
        if(hud){reference.target=reference.hud;candidate.target=candidate.hud;}
        amigaRendererBeginTerrainBatch(&batch,&candidate,102-px,-8-py);
        for(unsigned row=0;row<20;++row){
            const unsigned above=next(),middle=next(),below=next();
            const int y=int(row)*16-8+py;
            amigaRendererBatchTerrainWallRow(&batch,px-8,y,above,middle,below,26);
            for(unsigned col=0;col<26;++col){
                const unsigned bit=2u<<col;
                if(!(middle&bit))continue;
                const unsigned frame=((above&bit)?1:0)|((middle&(bit<<1))?2:0)|
                    ((below&bit)?4:0)|((middle&(bit>>1))?8:0);
                amigaRendererDrawAsset(&reference,pack.data(),pack.data()+pack.size(),
                    51,frame,int(col)*16+px-8,y,0);
            }
        }
        for(unsigned draw=0;draw<104;++draw){
            const int frame=draw%26;
            // Repeated coincident tiles interspersed with arbitrary painter order.
            const int x=draw%3?int(next()%27)*16-16+px:112+px;
            const int y=draw%3?int(next()%20)*16-16+py:py;
            amigaRendererDrawAsset(&reference,pack.data(),pack.data()+pack.size(),51,frame,x+8,y+8,0);
            require(amigaRendererBatchTerrainTile(&batch,frame,x+8,y+8),"grid tile rejected");
            if(draw==52){
                amigaRendererFlushTerrainBatch(&batch);
                require(a==b&&ha==hb,"mid-batch flush pixels differ");
            }
        }
        amigaRendererFlushTerrainBatch(&batch);
        require(a==b&&ha==hb,"batched pixels differ from ordered tile draws");
        for(unsigned i=0;i<16;++i)require(scratch[i]==0xaa&&scratch[scratch.size()-i-1]==0xaa,
            "scratch guard changed");
        require(!amigaRendererBatchTerrainTile(&batch,26,8,8),"unknown frame accepted");
        require(!amigaRendererBatchTerrainTile(&batch,0,px+9,py+8),"off-grid tile accepted");
        ++cases;
    }
    require(blits>0,"transfer path not exercised");
    std::cout<<cases<<" ordered terrain batches match reference pixels ("<<blits<<" transfers)\n";
}
