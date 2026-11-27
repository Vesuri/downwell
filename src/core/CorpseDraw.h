#ifndef DOWNWELL_CORE_CORPSE_DRAW_H
#define DOWNWELL_CORE_CORPSE_DRAW_H
#include "CorpseAnimation.h"
#include "EffectSpriteDraw.h"
#include "EffectMaskAssets.h"
namespace downwell {
struct CorpseDraw {
    static constexpr int sourceDepth=1000; // Same depth as smoke; merge instance order.
    using Command=EffectSpriteDraw::Command;
    // The room compositor supplies ordering relative to other drawFx actors.
    // Resolve immutable pre-mirrored art; never convert or flip pixels here.
    template<class Emit> static bool surface(const CorpseAnimation& corpse,const EffectMaskAssets& assets,
        const EffectSurfacePlacement::View& view,Emit emit){
        Command command;
        std::int32_t px,py;bool shown;
        const bool whole=corpse.valid&&EffectSpriteDraw::pixelFx(corpse.x,corpse.y,view,px,py,shown);
        if(whole){
            if(corpse.destroyRequested||corpse.flash>0||!shown)return true;
            command.sprite=corpse.spriteId;command.imageIndex=corpse.imageIndex;command.scaleX=corpse.imageXscale;
        }else{
            if(!prepare(corpse,view,false,false,true,command))return false;
            if(command.placement.target==EffectSurfacePlacement::hidden)return true;
        }
        const auto scale=command.scaleX.bits;
        if(!(scale&0x7fffffffu))return true;
        if((scale&0x7fffffffu)!=0x3f800000u)return false;
        const auto index=command.imageIndex.bits,magnitude=index&0x7fffffffu;
        if(magnitude>=0x3f800000u||((index>>31)&&magnitude))return false;
        EffectMaskAssets::Asset asset;
        if(!assets.corpse(command.sprite,(scale>>31)!=0,asset))return false;
        return whole?emit(asset,px,py):EffectSpriteDraw::emitAt(emit,asset,corpse.x,corpse.y);
    }
    static bool prepare(const CorpseAnimation& corpse,const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        if(!corpse.valid)return false;
        return EffectSpriteDraw::prepare(!corpse.destroyRequested&&corpse.flash<=0,
            corpse.spriteId,corpse.imageIndex,corpse.x,corpse.y,
            corpse.imageXscale,view,death,lowSpec,surfaceExists,out);
    }
};
}
#endif
