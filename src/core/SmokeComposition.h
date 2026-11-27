#ifndef DOWNWELL_CORE_SMOKE_COMPOSITION_H
#define DOWNWELL_CORE_SMOKE_COMPOSITION_H
#include "EffectMaskAssets.h"
#include "EffectSurfacePlacement.h"
#include "SmokeEffect.h"
#include "SmokeParticle.h"

namespace downwell {
// A smoke effect's published position as Q16 (binary32 positions are
// whole or fractional pixels well inside the Q16 range).
inline Fixed16 smokeRealX(const SmokeEffect& e){Fixed16 r;Fixed16::fromBinary32(e.x.bits,r);return r;}
inline Fixed16 smokeRealY(const SmokeEffect& e){Fixed16 r;Fixed16::fromBinary32(e.y.bits,r);return r;}
inline Fixed16 smokeRealX(const SmokeParticle& e){return Fixed16::fromInteger(e.x);}
inline Fixed16 smokeRealY(const SmokeParticle& e){return Fixed16::fromInteger(e.y);}
// Shared logical Draw preparation. The owner publishes the result at the
// returned boundary, never treating direct fallback as a controller overlay.
// Raster must implement the backend's verified coordinate sampling policy.
// Shades visits only instances whose original Draw calls drawShade; it supplies
// their published builtin x/y, not private integration coordinates.
class SmokeComposition {
public:
    enum class Layer { none, direct, controller };
    struct Settings {
        bool death=false,lowSpec=false,noControl=false;
        bool effectsExist=true,shadeExists=true;
    };
    const BinaryEffectSurface<>& coverage()const{return coverage_;}
    Layer layer()const{return layer_;}
    bool visible()const{return visible_;}

    template<class SmokeOwner,class Shades,class Raster>
    bool prepare(const SmokeOwner& smoke,const EffectMaskAssets& assets,
                 const EffectSurfacePlacement::View& view,Settings settings,
                 Shades shades,Raster raster){
        coverage_.clear();layer_=Layer::none;visible_=false;
        // Empty owners need neither assets nor shade enumeration. Retirement
        // still clears the previous coverage and lets backend damage repay.
        if(!smoke.size())return true;
        if(!assets.ready())return false;
        const bool direct=settings.death || settings.lowSpec;
        bool okay=true;
        smoke.visitNewest([&](std::uint32_t,const auto& effect){
            if(!okay || effect.destroyRequested)return;
            EffectSurfacePlacement::Draw draw;
            okay=EffectSurfacePlacement::fx(smokeRealX(effect),smokeRealY(effect),view,
                                            settings.death,settings.lowSpec,settings.effectsExist,draw);
            if(!okay || draw.target==EffectSurfacePlacement::hidden)return;
            if(draw.target==EffectSurfacePlacement::direct){
                okay=EffectSurfacePlacement::relative(draw.x,view.x,draw.x) &&
                     EffectSurfacePlacement::relative(draw.y,view.y,draw.y);
                if(!okay)return;
            }
            const auto* asset=assets.smoke(effect.drawFrame());
            okay=asset && stamp(*asset,draw.x,draw.y,false,raster);
        });
        // Draws into surfaceShade are combined only at controller Draw, after
        // ALL smoke additions. Neither death nor lowSpec direct draws use it.
        if(okay && !direct && settings.effectsExist && settings.shadeExists && !settings.noControl){
            shades([&](Fixed16 x,Fixed16 y){
                if(!okay)return;
                EffectSurfacePlacement::Draw draw;
                okay=EffectSurfacePlacement::enemyShade(x,y,view,settings.lowSpec,
                                                       settings.noControl,settings.shadeExists,draw);
                if(okay && draw.target==EffectSurfacePlacement::shade)
                    okay=stamp(*assets.shade(),draw.x,draw.y,true,raster);
            });
        }
        if(!okay){coverage_.clear();return false;}
        coverage_.visit([&](unsigned,unsigned,std::uint16_t){visible_=true;});
        if(visible_)layer_=direct?Layer::direct:Layer::controller;
        return true;
    }
private:
    template<class Raster>
    bool stamp(const EffectMaskAssets::Asset& asset,Fixed16 x,Fixed16 y,
               bool subtract,Raster raster){
        // Pass the exact camera-relative anchor to sampling. Subtract the
        // integer crop origin afterwards, with checked signed arithmetic.
        std::int32_t px,py;
        if(!raster(x,px) || !raster(y,py))return false;
        if((asset.originX>0 && px<(-2147483647-1)+asset.originX) ||
           (asset.originX<0 && px>2147483647+asset.originX) ||
           (asset.originY>0 && py<(-2147483647-1)+asset.originY) ||
           (asset.originY<0 && py>2147483647+asset.originY))return false;
        px-=asset.originX;py-=asset.originY;
        return subtract?coverage_.subtract(asset.mask,px,py):coverage_.add(asset.mask,px,py);
    }
    BinaryEffectSurface<> coverage_;
    Layer layer_=Layer::none;
    bool visible_=false;
};
}
#endif
