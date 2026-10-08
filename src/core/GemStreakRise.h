#ifndef DOWNWELL_CORE_GEM_STREAK_RISE_H
#define DOWNWELL_CORE_GEM_STREAK_RISE_H
#include "MultiplyBinary64.h"
namespace downwell {
// Controller-local Draw state, not a currency global or a Step timer.
// scrDrawHudInitStuff/scrGainGem set ascend to zero. The two HUD scripts
// update it before testing pause/room/visibility; only streak/death guard it.
struct GemStreakRise {
    MotionBinary64 ascend{};
    void reset() { ascend={}; }
    bool draw(unsigned streak,bool death,MotionBinary64 viewY,bool landscape,
              std::int32_t& worldY) {
        if(!streak||death)return true; // Source leaves both locals untouched.
        auto negative=ascend;negative.high^=0x80000000u;
        auto delta=MotionBinary64::fromInteger(16);
        if(!delta.add(negative)||!multiplyBinary64(delta,{0x3fc00000u,0},delta)||
           !ascend.add(delta))return false;
        // Preserve the two additions before subtracting the updated ascent.
        if(!viewY.add(MotionBinary64::fromInteger(landscape?24:50))||
           !viewY.add(MotionBinary64::fromInteger(4)))return false;
        negative=ascend;negative.high^=0x80000000u;
        return viewY.add(negative)&&viewY.roundPixel(worldY);
    }
};
}
#endif
