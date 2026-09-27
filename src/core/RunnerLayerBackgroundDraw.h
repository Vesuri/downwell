#ifndef DOWNWELL_CORE_RUNNER_LAYER_BACKGROUND_DRAW_H
#define DOWNWELL_CORE_RUNNER_LAYER_BACKGROUND_DRAW_H
#include "RunnerLayerBackground.h"
#include "RunnerDepthChanges.h"
namespace downwell {
// Dispatch at 0x48f0a0. The surrounding layer traversal supplies visibility,
// painter position and render state. A solid callback submits a viewport quad
// with source blending: it is NOT a framebuffer clear, even at alpha zero.
struct RunnerLayerBackgroundDraw {
    static bool solidColor(const RunnerLayerBackground& background,std::uint32_t& output){
        RunnerImageValue scaled;
        MotionBinary64 product;
        if(!RunnerSpatialSplit::valid(background.alpha)||
            !multiplyBinary64(background.alpha.promoted(),MotionBinary64::fromInteger(255),product)||
            !scaled.assign(product))return false;
        std::int32_t alpha;
        // Finite normal/zero, signed-int conversion domain. Nonfinite or
        // overflow inputs are not silently substituted for cvttss2si's result.
        if(!RunnerDepthService::integerDepth(scaled,alpha))return false;
        if(alpha<0)alpha=0;else if(alpha>255)alpha=255;
        output=(background.color&0xffffffu)|(std::uint32_t(alpha)<<24);return true;
    }
    template<class Lookup,class Solid,class Sprite>
    static bool visit(const RunnerLayerBackground& background,Lookup lookup,Solid solid,Sprite sprite){
        if(!(background.flags&1u))return true;
        bool exists=false;
        if(!lookup(background.spriteId,exists))return false;
        if(exists)return sprite(background);
        std::uint32_t color;
        return solidColor(background,color)&&solid(color);
    }
};
}
#endif
