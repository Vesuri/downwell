#include "platform/amiga/AmigaGameplayEffects.h"
#include "platform/amiga/AmigaGameplayRows.h"
#include "platform/amiga/framework/Bitmap.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static unsigned drains, copiedRows;
static bool copied[320];
void AmigaHardware::setBlitterNasty(bool) {}
void AmigaHardware::blitterDrain() { ++drains; }
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){std::abort();}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){std::abort();}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){std::abort();}
Bitmap::Bitmap(void* p,uint16_t w,uint16_t h,uint16_t planes,bool inter,bool own,uint16_t dma)
    : data(p),width(w),height(h),bitplanes(planes),dataWidth(dma),widthInBytes(dma/8),
      rowSizeInBytes(dma/8),bitplaneSizeInBytes(dma/8*h),interleaved(inter),owner(own),blittable(false) {}
Bitmap::~Bitmap() {}
void Bitmap::copy(const Bitmap& source,uint16_t x,uint16_t y,uint16_t sx,uint16_t sy,
                  uint16_t w,uint16_t h,uint16_t mask) {
    assert(x==sx && y==sy && !(x%16) && !(w%16) && mask==0xffff);
    assert(y+h<=height && x+w<=dataWidth);
    for(unsigned row=y;row<y+h;++row) {
        assert(!copied[row]); copied[row]=true; ++copiedRows;
        for(unsigned p=0;p<2;++p) {
            const unsigned at=p*bitplaneSizeInBytes+row*widthInBytes+x/8;
            std::memcpy(static_cast<UBYTE*>(data)+at,
                        static_cast<const UBYTE*>(source.data)+at,w/8);
        }
    }
}
static void clearCopies(){copiedRows=0;for(auto& row:copied)row=false;}
int main(){
    constexpr unsigned bytes=downwellGameRingBytes;
    std::array<UBYTE,bytes> backing{}, pixels[2]{},expected{};
    std::array<UBYTE,downwellLayerBytes> hud{};
    unsigned random=225;
    auto next=[&](){random=random*1664525u+1013904223u;return random;};
    for(auto& byte:backing)byte=UBYTE(next()>>24);
    hud.fill(0x5a);
    const auto pristine=backing;
    Bitmap source(backing.data(),380,320,2,false,false,384);
    Bitmap display[2]={{pixels[0].data(),380,320,2,false,false,384},
                       {pixels[1].data(),380,320,2,false,false,384}};
    AmigaGameplayRows rows;
    rows.reset(); rows.damage(0,0,320);
    for(unsigned b=0;b<2;++b){clearCopies();rows.sync(display[b],source,b);assert(pixels[b]==backing);}
    AmigaRenderer renderer;
    // Deliberately points at the backing/HUD: publishing must rebind its copy,
    // without changing the caller's renderer or either pristine allocation.
    amigaRendererInit(&renderer,backing.data(),hud.data());
    downwell::BinaryEffectSurface<> surface;
    bool owed[2][320]{};
    std::array<UBYTE,8*7> mask{};
    for(unsigned tick=0;tick<1024;++tick){
        const unsigned b=tick%2,origin=(tick*7)%320;
        clearCopies(); rows.sync(display[b],source,b);
        unsigned required=0;
        for(unsigned y=0;y<320;++y){assert(copied[y]==owed[b][y]);required+=owed[b][y];owed[b][y]=false;}
        assert(copiedRows==required && pixels[b]==backing);
        surface.clear();
        // Include empty retirement frames, sparse rows, both horizontal clips,
        // multiple words per row, and shade subtraction removing all coverage.
        for(auto& byte:mask)byte=UBYTE(next()>>24);
        const int x=int(next()%180)-20,y=int(next()%294)-7;
        const downwell::BinaryEffectSurface<>::Mask image{mask.data(),unsigned(mask.size()),61,7,8};
        if(tick%5){assert(surface.add(image,x,y));assert(surface.add(image,120,280));}
        if(tick%11==0){assert(surface.subtract(image,x,y));assert(surface.subtract(image,120,280));}
        expected=backing;
        bool visible=false;
        surface.visit([&](unsigned sx,unsigned sy,std::uint16_t bits){
            visible=true;const unsigned row=(origin+sy)%320;owed[b][row]=true;
            for(unsigned bit=0;bit<16;++bit)if(bits&(0x8000u>>bit)){
                const unsigned dx=110+sx+bit,at=row*48+dx/8;
                const auto value=UBYTE(0x80u>>(dx%8));
                expected[at]|=value;expected[at+15360]&=UBYTE(~value);
            }
        });
        const auto other=pixels[1-b];const unsigned before=drains;
        AmigaGameplayEffects::draw(renderer,display[b],b,origin,surface,rows);
        assert(drains-before==unsigned(visible));
        assert(pixels[b]==expected && pixels[1-b]==other && backing==pristine);
        for(unsigned yrow=0;yrow<320;++yrow)for(unsigned slot=0;slot<2;++slot)
            assert(rows.dirty(yrow,slot)==owed[slot][yrow]);
        for(auto byte:hud)assert(byte==0x5a);
        assert(renderer.game==backing.data());
    }
    // No new effect: both rings still repay their own last-frame damage.
    for(unsigned b=0;b<2;++b){clearCopies();rows.sync(display[b],source,b);assert(pixels[b]==backing);}
    std::puts("Gameplay effects: 1024 alternating/wrapping frames, exact damage restoration, untouched backing/HUD");
}
