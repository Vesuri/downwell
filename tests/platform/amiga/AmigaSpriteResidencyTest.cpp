#include "platform/amiga/AmigaRenderer.h"
#include "core/ProfileCounters.h"
#include <cstdio>
#include <cstdlib>
extern "C" { volatile unsigned long gDownwellProfileCounters[profileCounterCount]{}; }

int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,
    UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
static void require(bool okay,const char* message){
    if(!okay){std::fprintf(stderr,"sprite residency: %s\n",message);std::exit(1);}
}
int main(){
    UBYTE record[24]{};record[11]=1;record[23]=4;
    UBYTE pixels[4]={0x12,0x34,0x56,0x78};
    UWORD destination[8]{};
    AmigaResolvedSpritePart part{pixels,pixels+sizeof pixels,record,1};
    require(amigaRendererDrawResolvedSpritePart(&part,downwellPlayfieldX+32,20,destination,1)==1,"first draw");
    require(destination[2]==0x1234&&destination[3]==0x5678,"first bank pixels");
    require(amigaRendererDrawResolvedSpritePart(&part,downwellPlayfieldX+33,20,destination,1)==1,"unchanged bank draw");
    require(gDownwellProfileCounters[counterSpritePayloadCopies]==1&&
        gDownwellProfileCounters[counterSpriteHeaderOnly]==1,"unchanged payload still uses header-only update");
    // Model release/reload at the same address. Neither pointer nor height
    // identifies the immutable bank's lifetime.
    amigaRendererReleaseHardwareSpriteCopies();
    pixels[0]=0xab;pixels[1]=0xcd;pixels[2]=0xef;pixels[3]=0x01;
    require(amigaRendererDrawResolvedSpritePart(&part,downwellPlayfieldX+36,20,destination,1)==1,"reentry draw");
    require(destination[2]==0xabcd&&destination[3]==0xef01,"new bank must replace old payload");
    require(gDownwellProfileCounters[counterSpritePayloadCopies]==2,"new lifetime forces one payload copy");
    // Reusing/reinitializing the destination allocation is another lifetime
    // boundary, even when its numerical address has not changed.
    amigaRendererReleaseHardwareSpriteCopies();
    destination[2]=destination[3]=0;
    require(amigaRendererDrawResolvedSpritePart(&part,downwellPlayfieldX+36,20,destination,1)==1&&
        destination[2]==0xabcd&&destination[3]==0xef01,"reinitialized destination gets its payload");
    require(gDownwellProfileCounters[counterSpritePayloadCopies]==3,"destination reuse forces one copy");
    std::puts("Sprite bank address reuse: payload follows current residency");
}
