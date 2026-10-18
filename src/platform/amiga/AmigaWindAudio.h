#ifndef DOWNWELL_AMIGA_WIND_AUDIO_H
#define DOWNWELL_AMIGA_WIND_AUDIO_H
#include "WindBank.h"
#include "core/RunnerAudioGain.h"
#include <exec/execbase.h>
class AmigaPlatform;

namespace downwell {
// Actual backend for the currently admitted, monophonic UI wind event only.
// Unsupported loops/pitches/ramps fail; this is not a fake general audio mixer.
// PCM stays in a scene-local Fast lease. Stereo-centred channels 0/1 share two
// 4096-byte Chip fragments. No decoder, allocation or gain math runs in IRQs.
class AmigaWindAudio {
public:
    static constexpr unsigned fragmentBytes=4096,chipBytes=fragmentBytes*2+8; // Silent word, Exec-aligned.
    AmigaWindAudio()=default;
    ~AmigaWindAudio(){release();}
    AmigaWindAudio(const AmigaWindAudio&)=delete;
    AmigaWindAudio& operator=(const AmigaWindAudio&)=delete;
    bool acquire(AmigaPlatform& platform);
    void release(); // Before platform.restore(), and before scene resources retire.
    bool ready()const{return installed_&&bank_.ready();}
    bool hasResource(int resource)const{WindBank::Sample s;return ready()&&bank_.sample(resource,s);}
    bool play(int resource,int priority,bool loop,int& voice);
    bool isPlaying(int id,bool& playing);
    bool stop(int id);
    bool getGain(int id,MotionBinary64& value);
    bool setGain(int id,MotionBinary64 value,int milliseconds);
    bool setPitch(int id,MotionBinary64 value);
    void update(){} // Device-clock completion comes from Paula, not game ticks.
private:
    friend struct AmigaWindAudioProbe;
    static unsigned long interrupt();
    static void vblank(void*);
    void serviceVblank();
    void queue(unsigned bank,unsigned bytes);
    unsigned fill(unsigned bank);
    void halt();
    bool matches(int id)const{return id==voice_||id==resource_;}
    static AmigaWindAudio* current_;
    AmigaPlatform* platform_=nullptr;
    std::uint8_t* source_=nullptr;
    std::uint8_t* chip_=nullptr;
    WindBank bank_;
    WindBank::Sample sample_{};
    IntVector savedVector_{};
    bool installed_=false;
    std::uint16_t savedAdk_=0;
    bool savedFilter_=false;
    volatile bool playing_=false;
    // Main publishes commands with interrupts masked. VBI owns stop/start;
    // audio IRQ owns fragment reloads while running.
    volatile unsigned command_=0; // 0 none, 1 stop/start, 2 start next VBI, 3 stop
    int voice_=-1,resource_=-1,nextVoice_=1000000;
    RunnerAudioGain gain_{};
    volatile unsigned volume_=64;
    unsigned bankIndex_=0;
    std::uint32_t offset_=0;
    bool silenceQueued_=false,draining_=false;
};
}
#endif
