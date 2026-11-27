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
                        Fixed16 viewX,Fixed16 viewY,
                        EntryDitherPlacement& result){
        if(!SequenceTimeline::valid(x)||!SequenceTimeline::valid(y))return false;
        Fixed16 fx,fy;std::int32_t px,py;unsigned offset;
        if(!Fixed16::fromBinary32(x.bits,fx)||!Fixed16::fromBinary32(y.bits,fy)||
           !integer(fx-viewX,px)||!integer(fy-viewY,py)||px!=80||!EntryDitherMask::offset(py,offset))return false;
        result={py,offset};return true;
    }
private:
    static bool integer(Fixed16 n,std::int32_t& out){
        if(n.v&0xffff)return false;
        out=n.v>>16;return true;
    }
};
}
#endif
