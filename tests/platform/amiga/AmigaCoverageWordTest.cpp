#include "platform/amiga/AmigaRenderer.h"
#include "platform/amiga/AmigaCoverageWordVerification.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,
    UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}

int main(){
    if(verifyAmigaCoverageWords()!=80){std::cerr<<"Target coverage corpus failed\n";return 1;}
    struct Clip {int x,y,w,h;};
    const Clip clips[]={{0,0,380,284},{110,13,160,256},{7,0,31,284}};
    std::vector<UBYTE> base(downwellGameRingBytes),actual(base.size()),expected(base.size()),split(base.size());
    for(unsigned i=0;i<base.size();++i)base[i]=UBYTE(i*37u+(i>>5));
    unsigned random=0x835231u,cases=0;
    for(bool game:{false,true})for(unsigned origin:{0u,1u,36u,319u})
    for(const auto clip:clips)for(int y:{-1,0,13,268,283,284})for(int x=-17;x<=384;++x){
        random=random*1664525u+1013904223u;
        const UWORD alpha=cases%13?UWORD(random):0;
        const UWORD low=UWORD(random>>16),high=UWORD(random*17213u);
        actual=expected=split=base;
        auto setup=[&](AmigaRenderer& r,UBYTE* pixels){
            amigaRendererInit(&r,game?pixels:nullptr,game?nullptr:pixels);
            amigaRendererSetGameOrigin(&r,origin);
            amigaRendererSetClipRect(&r,clip.x,clip.y,clip.w,clip.h);
            r.target=pixels;
        };
        AmigaRenderer renderer;setup(renderer,actual.data());
        amigaRendererDrawPlanarWord(&renderer,x,y,alpha,low,high);
        // Independent per-pixel oracle: alpha covers opaque black too; colour
        // bits outside alpha must never alter the old framebuffer.
        for(unsigned bit=0;bit<16;++bit){
            const int px=x+int(bit);
            if(!(alpha&(0x8000u>>bit))||px<0||px>=380||y<0||y>=284||
                px<clip.x||px>=clip.x+clip.w||y<clip.y||y>=clip.y+clip.h)continue;
            const unsigned row=game?(origin+unsigned(y))%320:unsigned(y);
            for(unsigned plane=0;plane<2;++plane){
                auto& byte=expected[plane*(game?downwellGameRingPlaneBytes:downwellPlaneBytes)+row*48+unsigned(px)/8];
                const auto mask=UBYTE(128u>>(unsigned(px)&7));
                if((plane?high:low)&(0x8000u>>bit))byte|=mask;else byte&=UBYTE(~mask);
            }
        }
        setup(renderer,split.data());
        for(unsigned pen=0;pen<4;++pen){
            const UWORD mask=alpha&(pen&1?low:UWORD(~low))&(pen&2?high:UWORD(~high));
            amigaRendererDrawCoverageWord(&renderer,x,y,mask,pen);
        }
        if(actual!=expected||split!=expected){
            std::cerr<<"Planar coverage mismatch: "<<game<<' '<<origin<<' '<<x<<' '<<y<<'\n';return 1;
        }
        ++cases;
    }
    std::cout<<cases<<" planar/single-pen word cases match the pixel oracle, including clipping, opaque black and ring wrap\n";
}
