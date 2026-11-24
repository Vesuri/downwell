#include "AmigaGameplayEffects.h"
#include "AmigaGameplayRows.h"
#include "AmigaProfile.h"
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"
#ifdef DOWNWELL_PLATFORM_AMIGA
#include "AmigaEffectSurface.h"
#endif
#ifdef DOWNWELL_BOB_VERIFY
#include <cstring>
extern "C" { volatile unsigned gDownwellEffectPublishChecks,gDownwellEffectPublishFailures; }
extern "C" void downwellEffectSurfaceFailed();
#endif

namespace {
// The surface is already clipped to 160x284 and starts at a fixed screen X.
// Resolve the circular row once, not through the general renderer per word.
static void mergeWord(UBYTE* destination,UWORD mask,UWORD paint) {
    if(!mask)return;
#ifdef DOWNWELL_PLATFORM_AMIGA
    UWORD& word=*reinterpret_cast<UWORD*>(destination);
    const UWORD value=UWORD((word&~mask)|(paint&mask));
    if(value!=word)word=value;
#else
    const UWORD old=UWORD((unsigned(destination[0])<<8)|destination[1]);
    const UWORD value=UWORD((old&~mask)|(paint&mask));
    if(value!=old){destination[0]=UBYTE(value>>8);destination[1]=UBYTE(value);}
#endif
}
static void publishWord(UBYTE* row,unsigned x,UWORD alpha,UWORD low,UWORD high) {
    constexpr unsigned shift=downwellPlayfieldX%16;
    UBYTE* word=row+(downwellPlayfieldX/16+x/16)*2;
    mergeWord(word,UWORD(alpha>>shift),UWORD(low>>shift));
    mergeWord(word+downwellGameRingPlaneBytes,UWORD(alpha>>shift),UWORD(high>>shift));
    if constexpr(shift){
        mergeWord(word+2,UWORD(unsigned(alpha)<<(16-shift)),UWORD(unsigned(low)<<(16-shift)));
        mergeWord(word+2+downwellGameRingPlaneBytes,UWORD(unsigned(alpha)<<(16-shift)),UWORD(unsigned(high)<<(16-shift)));
    }
}
template<class Surface,class Paint>
void publish(AmigaRenderer renderer, Bitmap& display,
    unsigned buffer, unsigned origin, const Surface& surface,
    AmigaGameplayRows& rows,Paint paint) {
    DW_PROFILE_SCOPE(profileEffectPublish);
    (void)renderer;
    bool started = false;
    unsigned first = 0, end = 0;
    unsigned cachedY=downwellDisplayHeight;
    UBYTE* row=nullptr;
    const auto damage = [&] {
        rows.damage(origin, first, end - first, static_cast<UBYTE>(1u << buffer));
    };
    surface.visit([&](unsigned x, unsigned y, auto... planes) {
        if (!started) {
            // Earlier masked actors may still be using the blitter. CPU word
            // writes must not race them. Empty surfaces do not wait or damage.
            AmigaHardware::blitterDrain();
            started = true;
            first = y;
            end = y + 1;
        } else if (y > end) {
            damage();
            first = y;
            end = y + 1;
        } else if (y == end) {
            ++end;
        }
        if(y!=cachedY){
            cachedY=y;
            row=static_cast<UBYTE*>(display.data)+
                (UWORD(origin+y)%downwellGameRingHeight)*downwellBytesPerPlaneLine; // ring origin<320, y<284
        }
        paint(row,x,planes...);
    });
    if (started) damage();
}
}
void AmigaGameplayEffects::draw(AmigaRenderer renderer,Bitmap& display,unsigned buffer,
    unsigned origin,const downwell::BinaryEffectSurface<>& surface,AmigaGameplayRows& rows){
    publish(renderer,display,buffer,origin,surface,rows,
        [](UBYTE* row,unsigned x,std::uint16_t bits){
            publishWord(row,x,bits,bits,0);
        });
}
void AmigaGameplayEffects::draw(AmigaRenderer renderer,Bitmap& display,unsigned buffer,
    unsigned origin,const downwell::PaletteEffectSurface<>& surface,AmigaGameplayRows& rows){
#ifdef DOWNWELL_PLATFORM_AMIGA
    DW_PROFILE_SCOPE(profileEffectPublish);
#ifdef DOWNWELL_BOB_VERIFY
    alignas(UWORD) static UBYTE expected[downwellGameRingBytes];
    AmigaHardware::blitterDrain();
    std::memcpy(expected,display.data,sizeof(expected));
    Bitmap oracle(expected,downwellDmaWidth,downwellGameRingHeight,2,false,false,downwellDmaWidth);
    AmigaGameplayRows oracleRows;
    publish(renderer,oracle,buffer,origin,surface,oracleRows,
        [](UBYTE* row,unsigned x,std::uint16_t alpha,std::uint16_t low,std::uint16_t high){
            publishWord(row,x,alpha,low,high);
        });
#endif
    if(AmigaEffectSurface::publish(surface,display,buffer,origin,rows)){
#ifdef DOWNWELL_BOB_VERIFY
        AmigaHardware::blitterDrain();++gDownwellEffectPublishChecks;
        const auto* actual=static_cast<const UWORD*>(display.data);
        const auto* wanted=reinterpret_cast<const UWORD*>(expected);
        for(unsigned i=0;i<sizeof(expected)/sizeof(UWORD);++i)if(actual[i]!=wanted[i]){
            ++gDownwellEffectPublishFailures;downwellEffectSurfaceFailed();break;
        }
#endif
        return;
    }
#endif
    publish(renderer,display,buffer,origin,surface,rows,
        [](UBYTE* row,unsigned x,std::uint16_t alpha,std::uint16_t low,std::uint16_t high){
            publishWord(row,x,alpha,low,high);
        });
}
