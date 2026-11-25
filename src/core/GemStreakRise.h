#ifndef DOWNWELL_CORE_GEM_STREAK_RISE_H
#define DOWNWELL_CORE_GEM_STREAK_RISE_H
#include <cstdint>
namespace downwell {
// Controller-local Draw state, not a currency global or a Step timer.
// scrDrawHudInitStuff/scrGainGem set ascend to zero. The two HUD scripts
// update it before testing pause/room/visibility; only streak/death guard it.
// Each guarded Draw sets ascend += (16 - ascend) * 0.125, so after n Draws
// ascend = 16 - 16 * (7/8)^n, whose fraction is never one half: the drawn
// y is the view y + 28 (landscape; else 54) plus a fixed offset per n,
// tabulated from the binary64 form (GemStreakRiseTest), -16 from n = 26.
struct GemStreakRise {
    unsigned steps=0; // guarded Draws since the reset, saturated
    void reset(){steps=0;}
    bool draw(unsigned streak,bool death,std::int32_t viewY,bool landscape,std::int32_t& worldY){
        static const signed char offsets[27]={0,-2,-4,-5,-7,-8,-9,-10,-11,-11,-12,-12,-13,-13,
                                              -14,-14,-14,-14,-15,-15,-15,-15,-15,-15,-15,-15,-16};
        if(!streak||death)return true; // Source leaves both locals untouched.
        if(steps<26)++steps;
        worldY=viewY+(landscape?24:50)+4+offsets[steps];
        return true;
    }
};
}
#endif
