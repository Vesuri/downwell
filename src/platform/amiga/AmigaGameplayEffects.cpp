#include "AmigaGameplayEffects.h"
#include "AmigaGameplayRows.h"
#include "AmigaProfile.h"
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"

namespace {
template<class Surface,class Paint>
void publish(AmigaRenderer renderer, Bitmap& display,
    unsigned buffer, unsigned origin, const Surface& surface,
    AmigaGameplayRows& rows,Paint paint) {
    DW_PROFILE_SCOPE(profileEffectPublish);
    renderer.game = renderer.target = static_cast<UBYTE*>(display.data);
    amigaRendererSetGameOrigin(&renderer, origin);
    amigaRendererSetClipRect(&renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    bool started = false;
    unsigned first = 0, end = 0;
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
        paint(&renderer,downwellPlayfieldX+x,y,planes...);
    });
    if (started) damage();
}
}
void AmigaGameplayEffects::draw(AmigaRenderer renderer,Bitmap& display,unsigned buffer,
    unsigned origin,const downwell::BinaryEffectSurface<>& surface,AmigaGameplayRows& rows){
    publish(renderer,display,buffer,origin,surface,rows,
        [](AmigaRenderer* target,int x,int y,std::uint16_t bits){
            amigaRendererDrawCoverageWord(target,x,y,bits,1);
        });
}
void AmigaGameplayEffects::draw(AmigaRenderer renderer,Bitmap& display,unsigned buffer,
    unsigned origin,const downwell::PaletteEffectSurface<>& surface,AmigaGameplayRows& rows){
    publish(renderer,display,buffer,origin,surface,rows,
        [](AmigaRenderer* target,int x,int y,std::uint16_t alpha,std::uint16_t low,std::uint16_t high){
            amigaRendererDrawPlanarWord(target,x,y,alpha,low,high);
        });
}
