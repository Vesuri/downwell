#ifndef DOWNWELL_CORE_SMOKE_DRAW_H
#define DOWNWELL_CORE_SMOKE_DRAW_H
#include "EffectSpriteDraw.h"
#include "EffectMaskAssets.h"
namespace downwell {
struct SmokeDraw {
    using Command=EffectSpriteDraw::Command;
    static bool frame(RunnerImageValue index,unsigned& out){
        const auto magnitude=index.bits&0x7fffffffu;
        if(magnitude>=0x41100000u||((index.bits>>31)&&magnitude))return false;
        const unsigned exponent=magnitude>>23;
        out=exponent<127?0:((magnitude&0x7fffffu)|0x800000u)>>(150-exponent);
        return true;
    }
    static bool prepare(RunnerImageValue index,RunnerImageValue x,RunnerImageValue y,
        const EffectSurfacePlacement::View& view,bool death,bool lowSpec,bool surfaceExists,Command& out){
        return EffectSpriteDraw::prepare(true,84,index,x.promoted(),y.promoted(),{0x3f800000u},
            view,death,lowSpec,surfaceExists,out);
    }
    template<class Emit> static bool surface(RunnerImageValue index,RunnerImageValue x,RunnerImageValue y,
        const EffectMaskAssets& assets,const EffectSurfacePlacement::View& view,Emit emit){
        unsigned subimage;std::int32_t px,py;bool shown;
        if(EffectSpriteDraw::pixelFx(x,y,view,px,py,shown)){
            if(!shown)return true;
            if(!frame(index,subimage))return false;
            const auto* asset=assets.smoke(subimage);
            return asset&&emit(*asset,px,py);
        }
        Command command;if(!prepare(index,x,y,view,false,false,true,command))return false;
        if(command.placement.target==EffectSurfacePlacement::hidden)return true;
        if(!frame(index,subimage))return false;
        const auto* asset=assets.smoke(subimage);
        return asset&&emit(*asset,x.promoted(),y.promoted());
    }
};
}
#endif
