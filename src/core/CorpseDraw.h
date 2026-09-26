#ifndef DOWNWELL_CORE_CORPSE_DRAW_H
#define DOWNWELL_CORE_CORPSE_DRAW_H
#include "CorpseAnimation.h"
#include "EffectSpriteDraw.h"
namespace downwell {
struct CorpseDraw {
    static constexpr int sourceDepth=1000; // Same depth as smoke; merge instance order.
    using Command=EffectSpriteDraw::Command;
    static bool prepare(const CorpseAnimation& corpse,const EffectSurfacePlacement::View& view,
                        bool death,bool lowSpec,bool surfaceExists,Command& out){
        if(!corpse.valid)return false;
        return EffectSpriteDraw::prepare(!corpse.destroyRequested&&corpse.flash<=0,
            corpse.spriteId,corpse.imageIndex,corpse.x.promoted(),corpse.y.promoted(),
            corpse.imageXscale,view,death,lowSpec,surfaceExists,out);
    }
};
}
#endif
