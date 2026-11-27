#ifndef DOWNWELL_CORE_CURRENCY_DRAW_H
#define DOWNWELL_CORE_CURRENCY_DRAW_H
#include "EffectSpriteDraw.h"
#include "EffectMaskAssets.h"
namespace downwell {
// Registered currency has default scale/angle/alpha. Its Draw event publishes
// sprite/mask first; this adapter routes the subsequent drawFx operation.
struct CurrencyDraw {
    using Command=EffectSpriteDraw::Command;
    static bool frame(RunnerImageValue index,unsigned& out){
        const auto magnitude=index.bits&0x7fffffffu;
        const unsigned exponent=magnitude>>23;
        if(magnitude>=0x40800000u||((index.bits>>31)&&magnitude)||(!exponent&&magnitude))return false;
        out=exponent<127?0:((magnitude&0x7fffffu)|0x800000u)>>(150-exponent);
        return true;
    }
    static bool prepare(unsigned sprite,RunnerImageValue index,Fixed16 x,Fixed16 y,
        const EffectSurfacePlacement::View& view,bool death,bool lowSpec,bool exists,Command& out){
        if(sprite!=133&&sprite!=134)return false;
        return EffectSpriteDraw::prepare(true,sprite,index,x,y,{0x3f800000u},view,death,lowSpec,exists,out);
    }
    template<class Emit> static bool surface(const Command& command,const EffectMaskAssets& assets,Emit emit){
        if(command.placement.target==EffectSurfacePlacement::hidden)return true;
        if(command.placement.target!=EffectSurfacePlacement::effects)return false;
        unsigned subimage;
        if(!frame(command.imageIndex,subimage))return false;
        const auto* asset=assets.currency(command.sprite==134,subimage);
        return asset&&emit(*asset,command.placement.x,command.placement.y);
    }
};
}
#endif
