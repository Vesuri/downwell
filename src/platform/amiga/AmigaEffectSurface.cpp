#include "AmigaEffectSurface.h"
#include "AmigaEffectStampBank.h"
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
AmigaEffectStampBank stampBank;
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
    if constexpr(AmigaEffectSurface::interleaved){
        AmigaHardware::blitterClear(reinterpret_cast<UWORD*>(data+first*AmigaEffectSurface::rowBytes*3),
            12,(end-first)*3,0);
        return;
    }
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
    const auto* combined=AmigaEffectSurface::interleaved&&low?stampBank.find(m,low,high):nullptr;
    const unsigned sourceY=combined?sy*3:sy;
    const unsigned targetY=combined?unsigned(y)*3:unsigned(y);
    const unsigned blitHeight=combined?height*3:height;
    const unsigned destinationStride=AmigaEffectSurface::rowBytes*
        (AmigaEffectSurface::interleaved&&!combined?3:1);
    const unsigned planeOffset=AmigaEffectSurface::interleaved?AmigaEffectSurface::rowBytes:AmigaEffectSurface::planeBytes;
    // Common offline stamps flatten alpha/low/high into consecutive physical
    // rows. The repeated mask makes one masked copy preserve all three planes.
    // Other assets retain separate plane writes with an interleaved row stride.
    // Register values equal Bitmap::copyWithMask and
    // patternWithMask's for these strides (mask rows of m.stride bytes, the
    // surface's 24-byte rows with a guard word left of x=0).
    auto* data=static_cast<UBYTE*>(context);
    const unsigned sourceWords=m.stride/2,surfaceWords=destinationStride/2;
    const unsigned dx=unsigned(x)+16;
    const unsigned sourceFirst=sx>>4,destFirst=dx>>4;
    unsigned span=((dx+width-1)>>4)-destFirst;
    if(((sx+width-1)>>4)-sourceFirst>span)span=((sx+width-1)>>4)-sourceFirst;
    const unsigned sourceLast=sourceFirst+span,destLast=destFirst+span,words=span+1;
    const int16_t sourceModulo=int16_t((sourceWords-words)*2),surfaceModulo=int16_t((surfaceWords-words)*2);
    const int16_t shift=int16_t(int(dx&15)-int(sx&15));
    const unsigned trailing=(sourceLast<<4)+16-sx-width;
    const uint16_t lead=uint16_t(0xffffu>>(sx&15)),edge=trailing>=16?0:uint16_t(0xffffu<<trailing);
    ULONG source,target;uint16_t first,last;
    if(shift>=0){
        source=ULONG(UWORD(sourceY))*UWORD(m.stride)+sourceFirst*2;
        target=ULONG(UWORD(targetY))*destinationStride+destFirst*2;
        first=lead;last=edge;
    }else{
        // Descending: start at the last word of the last row.
        source=ULONG(UWORD(sourceY+blitHeight-1))*UWORD(m.stride)+sourceLast*2;
        target=ULONG(UWORD(targetY+blitHeight-1))*destinationStride+destLast*2;
        first=edge;last=lead;
    }
    auto* mask=reinterpret_cast<uint16_t*>(const_cast<std::uint8_t*>(combined?combined->mask:m.data)+source);
    if(combined){
        AmigaHardware::blitterCopyWithMask(reinterpret_cast<uint16_t*>(const_cast<unsigned char*>(combined->pixels)+source),
            reinterpret_cast<uint16_t*>(data+target),mask,words,blitHeight,
            sourceModulo,surfaceModulo,sourceModulo,shift,shift,first,last,false);
    }else{
    if(low){
        AmigaHardware::blitterCopyWithMask(reinterpret_cast<uint16_t*>(const_cast<std::uint8_t*>(low)+source),
            reinterpret_cast<uint16_t*>(data+planeOffset+target),mask,
            words,height,sourceModulo,surfaceModulo,sourceModulo,shift,shift,first,last,false);
        AmigaHardware::blitterCopyWithMask(reinterpret_cast<uint16_t*>(const_cast<std::uint8_t*>(high)+source),
            reinterpret_cast<uint16_t*>(data+planeOffset*2+target),mask,
            words,height,sourceModulo,surfaceModulo,sourceModulo,shift,shift,first,last,false);
    }
    AmigaHardware::blitterPatternWithMask(low?0xffff:0,reinterpret_cast<uint16_t*>(data+target),mask,
        words,height,0,surfaceModulo,sourceModulo,0,shift,first,last,false);
    }
#ifdef DOWNWELL_BOB_VERIFY
    ++gDownwellEffectDmaStamps;
#endif
    return true;
}
}
bool AmigaEffectSurface::bind(downwell::PaletteEffectSurface<>& surface,void* storage,
    const unsigned char* stamps,unsigned stampBytes,const downwell::EffectMaskAssets& assets){
    if(!storage||(reinterpret_cast<ULONG>(storage)&1)||reinterpret_cast<ULONG>(storage)>=0x200000UL)return false;
    const ULONG stampAddress=reinterpret_cast<ULONG>(stamps);
    if(!stamps||(stampAddress&1)||stampAddress>=0x200000UL||stampBytes>0x200000UL-stampAddress)return false;
    auto* data=static_cast<UBYTE*>(storage);
    if(!stampBank.bind(stamps,stampBytes,assets))return false;
#ifdef DOWNWELL_BOB_VERIFY
    reference=new(referenceStorage) Surface;
#endif
    constexpr unsigned planeOffset=interleaved?rowBytes:planeBytes;
    if(!surface.bindStorage(reinterpret_cast<UWORD*>(data+2),
        reinterpret_cast<UWORD*>(data+planeOffset+2),reinterpret_cast<UWORD*>(data+planeOffset*2+2),
        rowBytes/2*(interleaved?3:1),{storage,stamp,clear,wait}))return false;
    boundSurface=&surface;boundStorage=data;return true;
}
bool AmigaEffectSurface::publish(const Surface& surface,Bitmap& display,
    unsigned buffer,unsigned origin,AmigaGameplayRows& rows){
    if(&surface!=boundSurface)return false;
    if constexpr(interleaved){
        Bitmap mask(boundStorage,192,284,1,false,false,576);
        Bitmap low(boundStorage+rowBytes,192,284,1,false,false,576);
        Bitmap high(boundStorage+rowBytes*2,192,284,1,false,false,576);
        Bitmap targetLow(display.data,downwellDmaWidth,downwellGameRingHeight,1,false,false,downwellDmaWidth);
        Bitmap targetHigh(static_cast<UBYTE*>(display.data)+downwellGameRingPlaneBytes,
            downwellDmaWidth,downwellGameRingHeight,1,false,false,downwellDmaWidth);
        surface.visitRectangles([&](unsigned x,unsigned y,unsigned width,unsigned height){
            rows.damage(origin,y,height,static_cast<UBYTE>(1u<<buffer));
            unsigned destination=(origin+y)%downwellGameRingHeight;
            while(height){
                const unsigned count=height<downwellGameRingHeight-destination?height:downwellGameRingHeight-destination;
                targetLow.copyWithMask(low,mask,downwellPlayfieldX+x,destination,16+x,y,16+x,y,width,count);
                targetHigh.copyWithMask(high,mask,downwellPlayfieldX+x,destination,16+x,y,16+x,y,width,count);
                height-=count;y+=count;destination=0;
            }
        });
        return true;
    }
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
