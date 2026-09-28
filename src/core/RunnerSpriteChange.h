#ifndef DOWNWELL_CORE_RUNNER_SPRITE_CHANGE_H
#define DOWNWELL_CORE_RUNNER_SPRITE_CHANGE_H
#include "RunnerImageValue.h"
namespace downwell {
// Ordinary sprite path of 0x46bdf0, for the verified consecutive unit tracks.
// Produces actions before mutation; caller installs the sprite, invalidates its
// cached frame and marks bounds/queues on changed=true. Missing/empty sprites
// have count zero and do not reset the sequence position. No skeletal resources.
struct RunnerSpriteChange {
    struct Result {bool changed=false,resetIndex=false,resetTimeline=false;};
    static bool plan(int current,int next,RunnerImageValue index,unsigned frames,Result& out){
        if(current==next){out={};return true;}
        const auto magnitude=index.bits&0x7fffffffu;
        const bool negative=(index.bits&0x80000000u)!=0;
        const auto exponent=magnitude>>23;
        // Match the admitted normal/zero, signed-conversion domain. Do not
        // silently emulate an unknown FP exception mode on CVTTSS2SI overflow.
        if(frames>65535 || (!exponent&&magnitude) || exponent==255 ||
           magnitude>0x4f000000u || (!negative&&magnitude==0x4f000000u))return false;
        bool reset;
        if(!frames)reset=!(negative&&magnitude>=0x3f800000u);
        else{
            RunnerImageValue bound;
            // The admitted frame count is <=65535: it is exactly representable
            // in binary32, without a software binary64 intermediate on 68000.
            if(!bound.assignInteger(static_cast<std::int32_t>(frames)))return false;
            reset=!negative&&magnitude>=bound.bits;
        }
        out={true,reset,reset&&frames!=0};return true;
    }
};
}
#endif
