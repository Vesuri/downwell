#ifndef DOWNWELL_CORE_DEBRIS_DRAW_H
#define DOWNWELL_CORE_DEBRIS_DRAW_H
#include "DebrisAnimation.h"
#include "EffectSpriteDraw.h"
namespace downwell {
// Object-370 Draw, called at the instance's position in global Draw order.
// Keep the fractional subimage and coordinates for the renderer's verified
// sampling policy. This does not flatten direct-world drawing into surfaceFx.
struct DebrisDraw {
    static constexpr int sourceDepth=-1000; // Compatibility object-depth table.
    using Command=EffectSpriteDraw::Command;
    static bool prepare(const DebrisAnimation& effect,
                        const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        return EffectSpriteDraw::prepare(!effect.destroyRequested&&effect.flash<=0,
            effect.spriteId,effect.imageIndex,effect.x.promoted(),effect.y.promoted(),
            {0x3f800000u},view,death,lowSpec,surfaceExists,out);
    }
};
}
#endif
