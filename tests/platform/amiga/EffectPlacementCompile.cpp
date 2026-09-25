#include "core/EffectSurfacePlacement.h"
bool compileEffectPlacement(downwell::MotionBinary64 x,downwell::MotionBinary64 y,
    const downwell::EffectSurfacePlacement::View& view,
    downwell::EffectSurfacePlacement::Draw& fx,downwell::EffectSurfacePlacement::Draw& shade) {
    return downwell::EffectSurfacePlacement::fx(x,y,view,false,false,true,fx) &&
           downwell::EffectSurfacePlacement::enemyShade(x,y,view,false,false,true,shade);
}
