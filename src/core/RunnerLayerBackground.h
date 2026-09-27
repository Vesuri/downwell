#ifndef DOWNWELL_CORE_RUNNER_LAYER_BACKGROUND_H
#define DOWNWELL_CORE_RUNNER_LAYER_BACKGROUND_H
#include "RoomPack.h"
#include "RunnerImageValue.h"
#include "DivideBinary64.h"
#include <cstring>
namespace downwell {
// Background payload construction at 0x413bc9..0x413cb7. Element links,
// source element ID allocation, animation and drawing belong to the owner.
struct RunnerLayerBackground {
    std::int32_t spriteId=-1;
    std::uint32_t color=0,flags=0,speedType=0;
    RunnerImageValue alpha,firstFrame,speed;
    RunnerImageValue scaleX{0x3f800000u},scaleY{0x3f800000u};
    bool initialized=false;
    static bool create(const PackedRoomLayer& source,RunnerLayerBackground& out){
        if(source.type!=1||(source.backgroundFlags&~31u))return false;
        RunnerLayerBackground next;
        next.spriteId=source.backgroundSpriteId;
        next.color=source.backgroundColor&0xffffffu;
        next.flags=source.backgroundFlags;
        next.speedType=source.backgroundAnimationSpeedType;
        // Source uses cvtdq2ps followed by divss. Every byte is exact as a
        // float; verify all 256 rounded quotients against native binary32.
        MotionBinary64 alpha;
        if(!divideBinary64(MotionBinary64::fromInteger(source.backgroundColor>>24),
            MotionBinary64::fromInteger(255),alpha)||!next.alpha.assign(alpha))return false;
        // These are raw source field copies, not VM setters or conversions.
        static_assert(sizeof(source.backgroundFirstFrame)==sizeof(next.firstFrame.bits),"binary32 field size");
        std::memcpy(&next.firstFrame.bits,&source.backgroundFirstFrame,4);
        std::memcpy(&next.speed.bits,&source.backgroundAnimationSpeed,4);
        out=next;return true;
    }
    // Type 1 in the 0x412e9e switch goes directly to 0x41306b. No sprite
    // existence test, instance binding, or scale recalculation happens here.
    void initialize(){initialized=true;}
};
}
#endif
