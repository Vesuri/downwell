#ifndef DOWNWELL_CORE_CURRENCY_SMOKE_COMPOSITION_H
#define DOWNWELL_CORE_CURRENCY_SMOKE_COMPOSITION_H
#include "SmokeComposition.h"
#include "PaletteEffectSurface.h"

namespace downwell {
// Controller surfaceFx producer order: currency (depth 10000), then smoke
// (1000), then shade subtraction at controller Draw. Direct fallback cannot be
// flattened here: intervening world objects must retain their painter order.
class CurrencySmokeComposition {
public:
    using Settings=SmokeComposition::Settings;
    const PaletteEffectSurface<>& surface()const{return surface_;}
    bool visible()const{return visible_;}
    bool directRequired()const{return direct_;}

    // Currency enumerates original Draw order, emitting only non-dflash gems:
    // emit(large, frame, publishedX, publishedY). No simulation/RNG is advanced.
    template<class Currency,class SmokeOwner,class Shades,class Raster>
    bool prepare(Currency currency,const SmokeOwner& smoke,const EffectMaskAssets& assets,
                 const EffectSurfacePlacement::View& view,Settings settings,
                 Shades shades,Raster raster){
        surface_.clear();visible_=false;direct_=settings.death||settings.lowSpec;
        if(direct_ || !settings.effectsExist)return true;
        bool okay=true,added=false;
        auto add=[&](const EffectMaskAssets::Asset* asset,MotionBinary64 x,MotionBinary64 y){
            if(!okay)return;
            EffectSurfacePlacement::Draw draw;
            okay=EffectSurfacePlacement::fx(x,y,view,false,false,true,draw);
            if(!okay || draw.target==EffectSurfacePlacement::hidden)return;
            okay=asset && stamp(*asset,draw.x,draw.y,false,raster);
            added|=okay;
        };
        currency([&](bool large,unsigned frame,MotionBinary64 x,MotionBinary64 y){
            add(assets.currency(large,frame),x,y);
        });
        smoke.visitNewest([&](std::uint32_t,const SmokeEffect& effect){
            if(!effect.destroyRequested)add(assets.smoke(effect.drawFrame()),effect.x.promoted(),effect.y.promoted());
        });
        if(okay && added && settings.shadeExists && !settings.noControl){
            shades([&](MotionBinary64 x,MotionBinary64 y){
                if(!okay)return;
                EffectSurfacePlacement::Draw draw;
                okay=EffectSurfacePlacement::enemyShade(x,y,view,false,false,true,draw);
                if(okay && draw.target==EffectSurfacePlacement::shade)
                    okay=assets.shade() && stamp(*assets.shade(),draw.x,draw.y,true,raster);
            });
        }
        if(!okay){surface_.clear();return false;}
        surface_.visit([&](unsigned,unsigned,std::uint16_t,std::uint16_t,std::uint16_t){visible_=true;});
        return true;
    }
private:
    template<class Raster>
    bool stamp(const EffectMaskAssets::Asset& asset,MotionBinary64 x,MotionBinary64 y,
               bool subtract,Raster raster){
        std::int32_t px,py;
        if(!raster(x,px) || !raster(y,py))return false;
        if((asset.originX>0 && px<(-2147483647-1)+asset.originX) ||
           (asset.originX<0 && px>2147483647+asset.originX) ||
           (asset.originY>0 && py<(-2147483647-1)+asset.originY) ||
           (asset.originY<0 && py>2147483647+asset.originY))return false;
        px-=asset.originX;py-=asset.originY;
        const auto& m=asset.mask;
        const PaletteEffectSurface<>::Mask mask{m.data,m.bytes,m.width,m.height,m.stride};
        return subtract?surface_.subtract(mask,px,py):surface_.add({mask,asset.low,asset.high},px,py);
    }
    PaletteEffectSurface<> surface_;
    bool visible_=false,direct_=false;
};
}
#endif
