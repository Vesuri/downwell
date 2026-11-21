#ifndef DOWNWELL_AMIGA_PAULA_AUDIO_H
#define DOWNWELL_AMIGA_PAULA_AUDIO_H
// W2.4: the four-voice Paula mixer on the hardware (PaulaMixer.h holds the
// portable plan and sequencer). The main loop submits SoundRequests (or
// drains a ring) and calls frame() once per update; the VBlank applies the
// published commands. No audio interrupts, no allocation in the VBlank.
//
// The caller owns the bank: a DWSX pack whose samples are in Chip RAM, kept
// alive until release(). The driver takes all four channels (their DMA
// stays on, idling on an 8-byte Chip silent word) and turns the LED filter
// off; release() restores ADKCON and the filter.
#include "PaulaMixer.h"
#include <exec/types.h>
class AmigaPlatform;

namespace downwell {
class AmigaPaulaAudio {
public:
    static constexpr unsigned silentBytes=8;
    AmigaPaulaAudio():plan_(bank_){}
    ~AmigaPaulaAudio(){release();}
    AmigaPaulaAudio(const AmigaPaulaAudio&)=delete;
    AmigaPaulaAudio& operator=(const AmigaPaulaAudio&)=delete;
    bool acquire(AmigaPlatform& platform,const void* pack,unsigned bytes);
    void release(); // Before platform.restore(), and before the bank retires.
    bool ready()const{return installed_;}
    void submit(const SoundRequest& r){if(installed_)plan_.apply(r);}
    void drain(SoundRequests& requests){
        if(!installed_){requests.clear();return;}
        requests.drain([this](const SoundRequest& r){plan_.apply(r);});
    }
    void setMasterQ16(std::uint32_t master){plan_.setMasterQ16(master);}
    // Once per main-loop update: fades and expiry by the fields elapsed
    // since the last call, then publish the commands for the next VBlank.
    void frame();
    const PaulaVoicePlan& plan()const{return plan_;}
    const PaulaBank& bank()const{return bank_;}
    // Probe measurements: the longest VBlank service, in raster lines.
    unsigned longestServiceLines()const{return longestService_;}
private:
    struct Regs;
    static void vblank(void*);
    void serviceVblank();
    static AmigaPaulaAudio* current_;
    AmigaPlatform* platform_=nullptr;
    PaulaBank bank_;
    PaulaVoicePlan plan_;
    PaulaSequencer sequencer_;
    PaulaChannelCommand mailbox_[4]{};
    UBYTE* silent_=nullptr;
    bool installed_=false,savedFilter_=false;
    UWORD savedAdk_=0,lastField_=0;
    volatile unsigned longestService_=0;
};
}
#endif
