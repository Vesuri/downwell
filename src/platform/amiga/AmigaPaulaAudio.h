#ifndef DOWNWELL_AMIGA_PAULA_AUDIO_H
#define DOWNWELL_AMIGA_PAULA_AUDIO_H
// W2.4: the four-voice Paula mixer on the hardware (PaulaMixer.h holds the
// portable plan and sequencer). The main loop submits SoundRequests (or
// drains a ring) and calls frame() once per update; the VBlank applies the
// published commands. No audio interrupts, no allocation in the VBlank.
//
// The caller owns the banks: DWSX packs in Chip RAM, bound to a slot
// (resident: the run's, kept across scenes; scene: one loaded over it or
// alone) and kept alive until unbind() or release(). The driver takes all
// four channels (their DMA stays on, idling on an 8-byte Chip silent word)
// and turns the LED filter off; release() restores ADKCON and the filter.
#include "PaulaMixer.h"
#include <exec/types.h>
class AmigaPlatform;

namespace downwell {
class AmigaPaulaAudio {
public:
    static constexpr unsigned silentBytes=8;
    enum Slot : unsigned { resident=0, scene=1 };
    AmigaPaulaAudio():plan_(banks_[resident],&banks_[scene]){}
    ~AmigaPaulaAudio(){release();}
    AmigaPaulaAudio(const AmigaPaulaAudio&)=delete;
    AmigaPaulaAudio& operator=(const AmigaPaulaAudio&)=delete;
    bool acquire(AmigaPlatform& platform);
    void release(); // Before platform.restore(); unbinds both banks.
    // A Chip pack for a slot (false: not a pack, or not in Chip). Works
    // before acquire() too. The slot must be unbound.
    bool bind(Slot slot,const void* pack,unsigned bytes);
    // Stop the slot's voices and wait until no channel fetches the bank:
    // afterwards the caller may free it. Call between frames, not in the
    // middle of a drain.
    void unbind(Slot slot);
    bool bound(Slot slot)const{return banks_[slot].ready();}
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
    const PaulaBank& bank(Slot slot=resident)const{return banks_[slot];}
    // Probe measurements: the longest VBlank service, in raster lines.
    unsigned longestServiceLines()const{return longestService_;}
private:
    struct Regs;
    static void vblank(void*);
    void serviceVblank();
    static AmigaPaulaAudio* current_;
    AmigaPlatform* platform_=nullptr;
    PaulaBank banks_[2];
    PaulaVoicePlan plan_;
    PaulaSequencer sequencer_;
    PaulaChannelCommand mailbox_[4]{};
    UBYTE* silent_=nullptr;
    bool installed_=false,savedFilter_=false;
    volatile bool pending_=false; // the mailbox holds a published command
    UWORD savedAdk_=0,lastField_=0;
    volatile unsigned longestService_=0;
};
}
#endif
