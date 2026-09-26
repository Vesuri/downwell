#include "core/BinaryEffectSurface.h"
#include "core/PaletteEffectSurface.h"
bool compilePaletteEffect(downwell::PaletteEffectSurface<>& surface,
    const downwell::PaletteEffectSurface<>::Asset& asset,int x,int y) {
    surface.clear();return surface.add(asset,x,y) && surface.subtract(asset.alpha,x-32,y-16);
}
#include "core/EffectMaskAssets.h"
#include "core/SmokeComposition.h"
#include "core/CurrencySmokeComposition.h"
#include "core/SmokeEffects.h"
bool compileSmokeContacts(downwell::SmokeEffects<32>& smoke){
    return smoke.collideWalls(smoke.beginPhase(),[](downwell::ContactBounds,auto emit){return emit();});
}
bool compileCurrencySmokeComposition(downwell::CurrencySmokeComposition& composition,
    const downwell::SmokeEffects<32>& smoke,const downwell::EffectMaskAssets& assets,
    const downwell::EffectSurfacePlacement::View& view,
    downwell::SmokeComposition::Settings settings) {
    return composition.prepare([&](auto emit){emit(false,0,view.x,view.y);},
        smoke,assets,view,settings,[&](auto emit){emit(view.x,view.y);},
        [](downwell::MotionBinary64 n,std::int32_t& out){return n.roundPixel(out);});
}
bool compileSmokeComposition(downwell::SmokeComposition& composition,
    const downwell::SmokeEffects<32>& smoke,const downwell::EffectMaskAssets& assets,
    const downwell::EffectSurfacePlacement::View& view,
    downwell::SmokeComposition::Settings settings) {
    return composition.prepare(smoke,assets,view,settings,
        [&](auto emit){emit(view.x,view.y);},
        [](downwell::MotionBinary64 n,std::int32_t& out){return n.roundPixel(out);});
}
bool compileEffectAssets(downwell::EffectMaskAssets& assets,const std::uint8_t* pack,std::size_t size) {
    return assets.bind(pack,size);
}
bool compileBinaryEffect(downwell::BinaryEffectSurface<>& surface,
    const downwell::BinaryEffectSurface<>::Mask& effect,
    const downwell::BinaryEffectSurface<>::Mask& shade,int x,int y) {
    surface.clear();
    return surface.add(effect,x,y) && surface.subtract(shade,x-32,y-16);
}
