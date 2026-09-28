#ifndef DOWNWELL_CORE_ENTRY_DITHER_PLACEMENT_H
#define DOWNWELL_CORE_ENTRY_DITHER_PLACEMENT_H
#include "EntryDitherMask.h"
#include "EffectSurfacePlacement.h"
#include "SequenceTimeline.h"
namespace downwell {
// Checked placement of the offline centered crop. Do not round unsupported
// fractional coordinates into a different image on one of the backends.
struct EntryDitherPlacement {
    int y=0;
    unsigned offset=0;
    static bool resolve(RunnerImageValue x,RunnerImageValue y,
                        MotionBinary64 viewX,MotionBinary64 viewY,
                        EntryDitherPlacement& result){
        if(!SequenceTimeline::valid(x)||!SequenceTimeline::valid(y))return false;
        MotionBinary64 dx,dy;std::int32_t px,py;unsigned offset;
        if(!EffectSurfacePlacement::relative(x.promoted(),viewX,dx)||
           !EffectSurfacePlacement::relative(y.promoted(),viewY,dy)||
           !integer(dx,px)||!integer(dy,py)||px!=80||!EntryDitherMask::offset(py,offset))return false;
        result={py,offset};return true;
    }
private:
    static bool integer(MotionBinary64 n,std::int32_t& out){
        if(!n.roundPixel(out))return false;
        const auto exact=MotionBinary64::fromInteger(out);
        return n.low==exact.low&&(n.high==exact.high||(!out&&!(n.high&0x7fffffffu)));
    }
};
}
#endif
