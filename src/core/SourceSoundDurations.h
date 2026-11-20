#ifndef DOWNWELL_CORE_SOURCE_SOUND_DURATIONS_H
#define DOWNWELL_CORE_SOURCE_SOUND_DURATIONS_H
// W2.3: the source length of each sound resource in 60 Hz steps (ceil of
// frames * 60 / rate), for SoundVoices' audio_is_playing model. The table is
// written by tools/sound-durations.py from the extracted audio; lengths only.
#include <cstdint>
namespace downwell {
struct SourceSoundDurations {
    static std::uint16_t ticks(int id){
        static const std::uint16_t table[]={
#include "../../generated/sound-durations.inc"
        };
        constexpr unsigned count=sizeof(table)/sizeof(table[0]);
        return id>=0&&unsigned(id)<count?table[id]:0;
    }
};
}
#endif
