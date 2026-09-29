#include "AmigaEffectSurface.h"
#include <exec/types.h>
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"
#include "AmigaGameplayRows.h"
inline void* operator new(__SIZE_TYPE__,void* address)noexcept{return address;}
extern "C" { volatile unsigned gDownwellEffectDmaStamps,gDownwellEffectDmaChecks,gDownwellEffectDmaFailures; }
extern "C" __attribute__((noinline)) void downwellEffectSurfaceFailed(){__asm__ volatile("":::"memory");}
namespace {
using Surface=downwell::PaletteEffectSurface<>;
const Surface* boundSurface=nullptr;
UBYTE* boundStorage=nullptr;
#ifdef DOWNWELL_BOB_VERIFY
alignas(Surface) UBYTE referenceStorage[sizeof(Surface)];
Surface* reference=nullptr;
#endif
void wait(void*){AmigaHardware::blitterDrain();}
void clear(void* context,unsigned first,unsigned end){
#ifdef DOWNWELL_BOB_VERIFY
    reference->clear();
#endif
    if(first>=end)return;
    auto* data=static_cast<UBYTE*>(context);
    for(unsigned plane=0;plane<3;++plane)
        AmigaHardware::blitterClear(reinterpret_cast<UWORD*>(data+plane*AmigaEffectSurface::planeBytes+
            first*AmigaEffectSurface::rowBytes),12,end-first,0);
}
bool stamp(void* context,const Surface::Mask& m,const std::uint8_t* low,const std::uint8_t* high,int x,int y){
#ifdef DOWNWELL_BOB_VERIFY
    if(low)reference->add({m,low,high},x,y);else reference->subtract(m,x,y);
#endif
    // Immutable DWAP images are word-aligned. Unusual host-style masks and
    // Fast-RAM entry masks retain the scalar path, after draining queued DMA.
    if((reinterpret_cast<ULONG>(m.data)&1)||reinterpret_cast<ULONG>(m.data)>=0x200000UL||
        (low&&((reinterpret_cast<ULONG>(low)|reinterpret_cast<ULONG>(high))&1))||
        (low&&(reinterpret_cast<ULONG>(low)>=0x200000UL||reinterpret_cast<ULONG>(high)>=0x200000UL))||
        (m.stride&1)||m.stride>=8192)return false;
    unsigned sx=0,sy=0,width=m.width,height=m.height;
    if(x<0){sx=unsigned(-x);width-=sx;x=0;}
    if(y<0){sy=unsigned(-y);height-=sy;y=0;}
    if(width>unsigned(160-x))width=160-x;
    if(height>unsigned(284-y))height=284-y;
    if(!width||!height)return true;
    auto* data=static_cast<UBYTE*>(context);
    Bitmap mask(const_cast<std::uint8_t*>(m.data),m.width,m.height,1,false,false,m.stride*8);
    Bitmap alpha(data,192,284,1,false,false,192);
    if(low){
        Bitmap sourceLow(const_cast<std::uint8_t*>(low),m.width,m.height,1,false,false,m.stride*8);
        Bitmap sourceHigh(const_cast<std::uint8_t*>(high),m.width,m.height,1,false,false,m.stride*8);
        Bitmap targetLow(data+AmigaEffectSurface::planeBytes,192,284,1,false,false,192);
        Bitmap targetHigh(data+AmigaEffectSurface::planeBytes*2,192,284,1,false,false,192);
        targetLow.copyWithMask(sourceLow,mask,x+16,y,sx,sy,sx,sy,width,height);
        targetHigh.copyWithMask(sourceHigh,mask,x+16,y,sx,sy,sx,sy,width,height);
    }
    alpha.patternWithMask(mask,x+16,y,sx,sy,width,height,low?0xffff:0);
#ifdef DOWNWELL_BOB_VERIFY
    ++gDownwellEffectDmaStamps;
#endif
    return true;
}
}
bool AmigaEffectSurface::bind(downwell::PaletteEffectSurface<>& surface,void* storage){
    if(!storage||(reinterpret_cast<ULONG>(storage)&1)||reinterpret_cast<ULONG>(storage)>=0x200000UL)return false;
    auto* data=static_cast<UBYTE*>(storage);
#ifdef DOWNWELL_BOB_VERIFY
    reference=new(referenceStorage) Surface;
#endif
    if(!surface.bindStorage(reinterpret_cast<UWORD*>(data+2),
        reinterpret_cast<UWORD*>(data+planeBytes+2),reinterpret_cast<UWORD*>(data+planeBytes*2+2),
        rowBytes/2,{storage,stamp,clear,wait}))return false;
    boundSurface=&surface;boundStorage=data;return true;
}
bool AmigaEffectSurface::publish(const Surface& surface,Bitmap& display,
    unsigned buffer,unsigned origin,AmigaGameplayRows& rows){
    if(&surface!=boundSurface)return false;
    Bitmap mask(boundStorage,192,284,1,false,false,192);
    Bitmap colors(boundStorage+planeBytes,192,284,2,false,false,192);
    surface.visitRectangles([&](unsigned x,unsigned y,unsigned width,unsigned height){
        rows.damage(origin,y,height,static_cast<UBYTE>(1u<<buffer));
        unsigned destination=(origin+y)%downwellGameRingHeight;
        while(height){
            const unsigned count=height<downwellGameRingHeight-destination?
                height:downwellGameRingHeight-destination;
            display.copyWithMask(colors,mask,downwellPlayfieldX+x,destination,
                16+x,y,16+x,y,width,count);
            height-=count;y+=count;destination=0;
        }
    });
    return true;
}
bool AmigaEffectSurface::verify(const downwell::PaletteEffectSurface<>& surface){
#ifdef DOWNWELL_BOB_VERIFY
    ++gDownwellEffectDmaChecks;
    if(!reference->samePixels(surface)){++gDownwellEffectDmaFailures;downwellEffectSurfaceFailed();return false;}
    return true;
#else
    (void)surface;return true;
#endif
}
bool AmigaEffectSurface::fixtures(downwell::PaletteEffectSurface<>& surface,const downwell::EffectMaskAssets& assets){
#ifdef DOWNWELL_BOB_VERIFY
    for(unsigned i=0;i<64;++i){
        surface.clear();
        const auto& image=*(i%2?assets.smoke(i%9):assets.currency(i%4==0,i%4));
        const auto& m=image.mask;
        const Surface::Asset asset{{m.data,m.bytes,m.width,m.height,m.stride},image.low,image.high};
        const int x=i<16?int(i)-8:i<32?int(i)+128:int(i%16)+64;
        const int y=i<32?int(i%8)-4:279-int(i%8);
        if(!surface.add(asset,x,y)||!surface.add(asset,x+3,y+2)||!verify(surface))return false;
        const auto& shade=assets.shade()->mask;
        if(!surface.subtract({shade.data,shade.bytes,shade.width,shade.height,shade.stride},x-2,y-3)||!verify(surface))return false;
    }
    surface.clear();if(!verify(surface))return false;
#else
    (void)surface;(void)assets;
#endif
    return true;
}
