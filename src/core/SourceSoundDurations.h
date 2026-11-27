#ifndef DOWNWELL_CORE_SOURCE_SOUND_DURATIONS_H
#define DOWNWELL_CORE_SOURCE_SOUND_DURATIONS_H
// W2.3: the source length of each sound resource in 60 Hz steps (ceil of
// frames * 60 / rate), for SoundVoices' audio_is_playing model. The table is
// written by tools/sound-durations.py from the extracted audio; lengths only.
// gain() is the asset volume a fresh voice reads back (audio_sound_get_gain).
#include "AudioLevel.h"
#include <cstdint>
#ifdef DOWNWELL_INSTALLED_TABLES
// WI.3: bound from Downwell.tables (tools/install-table-headers.py).
#include "../../generated/installed/sound-durations.inc"
#include "../../generated/installed/sound-gains.inc"
#endif
namespace downwell {
struct SourceSoundDurations {
    static std::uint16_t ticks(int id){
#ifdef DOWNWELL_INSTALLED_TABLES
        const std::uint16_t* table=sourceSoundTicks;
        constexpr unsigned count=sourceSoundTicks_count;
#else
        static const std::uint16_t table[]={
#include "../../generated/sound-durations.inc"
        };
        constexpr unsigned count=sizeof(table)/sizeof(table[0]);
#endif
        return id>=0&&unsigned(id)<count?table[id]:0;
    }
    static AudioLevel gain(int id){
#ifdef DOWNWELL_INSTALLED_TABLES
        constexpr unsigned count=sourceSoundGains_count;
        // scrInitialize: audio_sound_gain(sfxGunM, 0.5, 0) before any run.
        if(id==18)return audio_level::half;
        if(id<0||unsigned(id)>=count)return audio_level::one;
        return audio_level::fromBits(sourceSoundGains[id][0],sourceSoundGains[id][1]);
#else
        struct Bits {std::uint32_t high,low;};
        static const Bits table[]={
#include "../../generated/sound-gains.inc"
        };
        constexpr unsigned count=sizeof(table)/sizeof(table[0]);
        // scrInitialize: audio_sound_gain(sfxGunM, 0.5, 0) before any run.
        if(id==18)return audio_level::half;
        if(id<0||unsigned(id)>=count)return audio_level::one;
        return audio_level::fromBits(table[id].high,table[id].low);
#endif
    }
};
}
#endif
