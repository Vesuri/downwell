#ifndef DOWNWELL_CORE_EFFECT_SPRITE_DRAW_H
#define DOWNWELL_CORE_EFFECT_SPRITE_DRAW_H
#include "EffectSurfacePlacement.h"
#include "RunnerImageValue.h"
namespace downwell {
// Shared drawFx routing for effects retaining default Y scale, angle, alpha
// and white blend. Preserve fractional inputs; backend sampling is separate.
struct EffectSpriteDraw {
    struct Command {
        EffectSurfacePlacement::Draw placement{};
        unsigned sprite=0;
        RunnerImageValue imageIndex{},scaleX{0x3f800000u};
    };
    static bool prepare(bool visible,unsigned sprite,RunnerImageValue index,
                        MotionBinary64 x,MotionBinary64 y,RunnerImageValue scaleX,
                        const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        Command next;
        if(visible){
            if(!EffectSurfacePlacement::fx(x,y,view,death,lowSpec,surfaceExists,next.placement))return false;
            if(next.placement.target!=EffectSurfacePlacement::hidden){
                next.sprite=sprite;next.imageIndex=index;next.scaleX=scaleX;
            }
        }
        out=next;return true;
    }
};
}
#endif
