#ifndef DOWNWELL_CORE_AUDIO_MASTER_GAIN_H
#define DOWNWELL_CORE_AUDIO_MASTER_GAIN_H
#include "RunnerImageValue.h"
#include "AudioLevel.h"
namespace downwell {
// Admitted settings range only. Device output gain is independent of the
// resource/voice controls exposed by audio_sound_get_gain. Reject unsupported
// values instead of inventing a clipping policy for arbitrary saved settings.
struct AudioMasterGain {
    RunnerImageValue value{0x3f800000u};
    AudioLevel level=audio_level::one;
    bool set(AudioLevel gain){
        if(gain.v<0||gain.v>65536)return false;
        level=gain;
        return value.assign(gain.binary64());
    }
};
}
#endif
