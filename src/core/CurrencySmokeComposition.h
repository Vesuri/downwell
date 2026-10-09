#ifndef DOWNWELL_CORE_CURRENCY_SMOKE_COMPOSITION_H
#define DOWNWELL_CORE_CURRENCY_SMOKE_COMPOSITION_H
#include "SmokeComposition.h"
#include "PaletteEffectSurface.h"
#include "Profile.h"

namespace downwell {
// Controller surfaceFx producer order: currency (depth 10000), then smoke
// (1000), then shade subtraction at controller Draw. Direct fallback cannot be
// flattened here: intervening world objects must retain their painter order.
class CurrencySmokeComposition {
public:
    using Settings=SmokeComposition::Settings;
    const PaletteEffectSurface<>& surface()const{return surface_;}
    PaletteEffectSurface<>& surface(){return surface_;}
    bool visible()const{return visible_;}
    bool directRequired()const{return direct_;}

    // Some source producers (title text) already subtract the camera in their
    // Draw event. Keep those surface-local coordinates out of onSurface's
    // world-to-view subtraction; do not add the camera back just to cancel it.
    template<class Raster> struct SurfaceEmitter {
        CurrencySmokeComposition& owner;const EffectSurfacePlacement::View& view;
        Raster& raster;bool& okay;bool& added;
        bool atSurface(const EffectMaskAssets::Asset& asset,MotionBinary64 x,MotionBinary64 y){
            if(!okay)return false;
            okay=owner.stamp(asset,x,y,false,raster);added|=okay;return okay;
        }
        bool operator()(const EffectMaskAssets::Asset& asset,MotionBinary64 x,MotionBinary64 y){
            if(!okay)return false;
            EffectSurfacePlacement::Draw draw;
            okay=EffectSurfacePlacement::onSurface(x,y,view,true,draw);
            return okay&&atSurface(asset,draw.x,draw.y);
        }
    };

    // Currency enumerates original Draw order, emitting only non-dflash gems:
    // emit(large, frame, publishedX, publishedY). No simulation/RNG is advanced.
    template<class Currency,class SmokeOwner,class Shades,class Raster>
    bool prepare(Currency currency,const SmokeOwner& smoke,const EffectMaskAssets& assets,
                 const EffectSurfacePlacement::View& view,Settings settings,
                 Shades shades,Raster raster){
        return prepareWithLateEffects(currency,smoke,assets,view,settings,
            [](auto){return true;},shades,raster);
    }

    // Additional surfaceFx producers between smoke and controller Draw (for
    // example depth -1000 debris). The caller enumerates global painter order,
    // resolves subimages, and emits (offline asset, published world X/Y).
    // Return false on enumeration failure. No local family sort, frame rounding
    // or simulation belongs here. Direct fallback remains world Draw's job.
    template<class Currency,class SmokeOwner,class LateEffects,class Shades,class Raster>
    bool prepareWithLateEffects(Currency currency,const SmokeOwner& smoke,const EffectMaskAssets& assets,
                 const EffectSurfacePlacement::View& view,Settings settings,
                 LateEffects lateEffects,Shades shades,Raster raster){
        return prepareWithSurfaceProducers(currency,smoke,assets,view,settings,
            lateEffects,[](auto){return true;},shades,raster);
    }

    // Unconditional drawOnSurface producers after the supplied drawFx producers
    // and before controller shade subtraction. Their origins may be outside
    // the viewport (entry dither), and opaque black replaces earlier colors.
    // Caller owns depth ordering and transforms; this accepts resolved assets.
    template<class Currency,class SmokeOwner,class LateEffects,class SurfaceEffects,class Shades,class Raster>
    bool prepareWithSurfaceProducers(Currency currency,const SmokeOwner& smoke,const EffectMaskAssets& assets,
                 const EffectSurfacePlacement::View& view,Settings settings,
                 LateEffects lateEffects,SurfaceEffects surfaceEffects,Shades shades,Raster raster){
        { DW_PROFILE_SCOPE(profileEffectClear); surface_.clear(); }
        visible_=false;direct_=settings.death||settings.lowSpec;
        if(!settings.effectsExist)return true;
        bool okay=true,added=false;
        using Placement=EffectSurfacePlacement;
        Placement::PixelView pixels;
        const bool pixelView=Placement::pixelView(view,pixels);
        auto addReal=[&](const EffectMaskAssets::Asset* asset,MotionBinary64 x,MotionBinary64 y){
            if(!okay)return;
            Placement::Draw draw;
            { DW_PROFILE_SCOPE(profileEffectPlacement);
              okay=Placement::fx(x,y,view,false,false,true,draw); }
            if(!okay || draw.target==Placement::hidden)return;
            okay=asset && stamp(*asset,draw.x,draw.y,false,raster);
            added|=okay;
        };
        // Producers may emit whole pixels as std::int32_t. Those are already
        // raster positions: the raster is not consulted for them.
        auto add=[&](const EffectMaskAssets::Asset* asset,auto x,auto y){
            if constexpr(pixel(decltype(x){})){
                if(!okay)return;
                if(!pixelView||!Placement::smallPixel(x)||!Placement::smallPixel(y))
                    return addReal(asset,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y));
                if(!Placement::pixelInView(x,y,pixels))return;
                okay=asset && stampPixel(*asset,x-pixels.x,y-pixels.y,false);
                added|=okay;
            }else addReal(asset,x,y);
        };
        if(!direct_){
            { DW_PROFILE_SCOPE(profileEffectCurrency);
            currency([&](bool large,unsigned frame,auto x,auto y){
                add(assets.currency(large,frame),x,y);
            });
            }
            { DW_PROFILE_SCOPE(profileEffectSmoke);
            smoke.visitNewest([&](std::uint32_t,const SmokeEffect& effect){
                if(!effect.destroyRequested)add(assets.smoke(effect.drawFrame()),effect.x.promoted(),effect.y.promoted());
            });
            }
            { DW_PROFILE_SCOPE(profileEffectDebris);
            if(okay)okay=lateEffects([&](const EffectMaskAssets::Asset& asset,MotionBinary64 x,MotionBinary64 y){
                add(&asset,x,y);return okay;
            })&&okay;
            }
        }
        { DW_PROFILE_SCOPE(profileEffectCover);
        SurfaceEmitter<Raster> surfaceEmit{*this,view,raster,okay,added};
        if(okay)okay=surfaceEffects(surfaceEmit)&&okay;
        }
        if(okay && added && !settings.lowSpec && settings.shadeExists && !settings.noControl){
            DW_PROFILE_SCOPE(profileEffectShade);
            shades([&](auto x,auto y){
                if(!okay)return;
                if constexpr(pixel(decltype(x){})){
                    if(pixelView&&Placement::smallPixel(x)&&Placement::smallPixel(y)){
                        if(Placement::pixelInView(x,y,pixels))
                            okay=assets.shade() && stampPixel(*assets.shade(),
                                Placement::snapShadePixel(x)-pixels.x,Placement::snapShadePixel(y)-pixels.y,true);
                        return;
                    }
                }
                const auto realX=real(x),realY=real(y);
                EffectSurfacePlacement::Draw draw;
                okay=EffectSurfacePlacement::enemyShade(realX,realY,view,false,false,true,draw);
                if(okay && draw.target==EffectSurfacePlacement::shade)
                    okay=assets.shade() && stamp(*assets.shade(),draw.x,draw.y,true,raster);
            });
        }
        if(!okay){surface_.clear();return false;}
        { DW_PROFILE_SCOPE(profileEffectVisibility);
          visible_=surface_.intersects(0,0,160,284); }
        return true;
    }
private:
    static constexpr bool pixel(MotionBinary64){return false;}
    static constexpr bool pixel(std::int32_t){return true;}
    static MotionBinary64 real(MotionBinary64 n){return n;}
    static MotionBinary64 real(std::int32_t n){return MotionBinary64::fromInteger(n);}
    template<class Raster>
    bool stamp(const EffectMaskAssets::Asset& asset,MotionBinary64 x,MotionBinary64 y,
               bool subtract,Raster raster){
        std::int32_t px,py;
        { DW_PROFILE_SCOPE(profileEffectRaster);
        if(!raster(x,px) || !raster(y,py))return false;
        }
        return stampPixel(asset,px,py,subtract);
    }
    bool stampPixel(const EffectMaskAssets::Asset& asset,std::int32_t px,std::int32_t py,bool subtract){
        if((asset.originX>0 && px<(-2147483647-1)+asset.originX) ||
           (asset.originX<0 && px>2147483647+asset.originX) ||
           (asset.originY>0 && py<(-2147483647-1)+asset.originY) ||
           (asset.originY<0 && py>2147483647+asset.originY))return false;
        px-=asset.originX;py-=asset.originY;
        DW_PROFILE_SCOPE(profileEffectStamp);
        const auto& m=asset.mask;
        const PaletteEffectSurface<>::Mask mask{m.data,m.bytes,m.width,m.height,m.stride};
        return subtract?surface_.subtract(mask,px,py):surface_.add({mask,asset.low,asset.high},px,py);
    }
    PaletteEffectSurface<> surface_;
    bool visible_=false,direct_=false;
};
}
#endif
