#ifndef DOWNWELL_CORE_DEBRIS_DRAW_H
#define DOWNWELL_CORE_DEBRIS_DRAW_H
#include "DebrisAnimation.h"
#include "EffectSpriteDraw.h"
#include "EffectMaskAssets.h"
namespace downwell {
// Object-370 Draw, called at the instance's position in global Draw order.
// Keep the fractional subimage and coordinates for the renderer's verified
// sampling policy. This does not flatten direct-world drawing into surfaceFx.
struct DebrisDraw {
    static constexpr int sourceDepth=-1000; // Compatibility object-depth table.
    using Command=EffectSpriteDraw::Command;
    // Ordinary bitmap Draw floors the float subimage before modulo count
    // (runner 0x4af747..0x4af78b). Admit the nonnegative 0..7 range produced
    // by debris Create/sequence animation; do not silently coerce bad state.
    static bool frame(unsigned sprite,RunnerImageValue index,unsigned& out){
        const unsigned count=sprite==91?6:sprite==115||sprite==116?4:0;
        const auto magnitude=index.bits&0x7fffffffu;
        const unsigned exponent=magnitude>>23;
        if(!count||magnitude>0x40e00000u||((index.bits>>31)&&magnitude)||
           (!exponent&&magnitude))return false;
        const unsigned whole=exponent<127?0:((magnitude&0x7fffffu)|0x800000u)>>(150-exponent); // <= 7
        out=std::uint16_t(whole)%std::uint16_t(count);return true;
    }
    template<class Emit>
    static bool surface(const DebrisAnimation& effect,const EffectMaskAssets& assets,
                        const EffectSurfacePlacement::View& view,Emit emit){
        Command command;
        unsigned subimage;EffectMaskAssets::Asset asset;
        std::int32_t px,py;bool shown;
        if(EffectSpriteDraw::pixelFx(effect.x,effect.y,view,px,py,shown)){
            if(effect.destroyRequested||effect.flash>0||!shown)return true;
            return frame(effect.spriteId,effect.imageIndex,subimage)&&
                assets.debris(effect.spriteId,subimage,asset)&&emit(asset,px,py);
        }
        if(!prepare(effect,view,false,false,true,command))return false;
        if(command.placement.target==EffectSurfacePlacement::hidden)return true;
        return frame(command.sprite,command.imageIndex,subimage)&&assets.debris(command.sprite,subimage,asset)&&
            emit(asset,effect.x.promoted(),effect.y.promoted());
    }
    static bool prepare(const DebrisAnimation& effect,
                        const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        return EffectSpriteDraw::prepare(!effect.destroyRequested&&effect.flash<=0,
            effect.spriteId,effect.imageIndex,effect.x.promoted(),effect.y.promoted(),
            {0x3f800000u},view,death,lowSpec,surfaceExists,out);
    }
};
}
#endif
