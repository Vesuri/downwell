#ifndef DOWNWELL_CORE_AUDIO_MASTER_GAIN_H
#define DOWNWELL_CORE_AUDIO_MASTER_GAIN_H
#include "RunnerImageValue.h"
namespace downwell {
// Admitted settings range only. Device output gain is independent of the
// resource/voice controls exposed by audio_sound_get_gain. Reject unsupported
// values instead of inventing a clipping policy for arbitrary saved settings.
struct AudioMasterGain {
    RunnerImageValue value{0x3f800000u};
    bool set(MotionBinary64 gain){
        if(gain.zero())gain={};
        if(!gain.valid()||((gain.high&0x80000000u)&&!gain.zero())||
           gain.high>0x3ff00000u||(gain.high==0x3ff00000u&&gain.low))return false;
        return value.assign(gain);
    }
};
}
#endif
