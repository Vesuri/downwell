#ifndef DOWNWELL_CORE_DEBRIS_DRAW_H
#define DOWNWELL_CORE_DEBRIS_DRAW_H
#include "DebrisAnimation.h"
#include "EffectSurfacePlacement.h"
namespace downwell {
// Object-370 Draw, called at the instance's position in global Draw order.
// Keep the fractional subimage and coordinates for the renderer's verified
// sampling policy. This does not flatten direct-world drawing into surfaceFx.
struct DebrisDraw {
    static constexpr int sourceDepth=-1000; // Compatibility object-depth table.
    struct Command {
        EffectSurfacePlacement::Draw placement{};
        unsigned sprite=0;
        RunnerImageValue imageIndex{};
    };
    static bool prepare(const DebrisAnimation& effect,
                        const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        Command next;
        if(!effect.destroyRequested && effect.flash<=0){
            if(!EffectSurfacePlacement::fx(effect.x.promoted(),effect.y.promoted(),view,
                                          death,lowSpec,surfaceExists,next.placement))return false;
            if(next.placement.target!=EffectSurfacePlacement::hidden){
                next.sprite=effect.spriteId;
                next.imageIndex=effect.imageIndex;
            }
        }
        out=next;return true;
    }
};
}
#endif
