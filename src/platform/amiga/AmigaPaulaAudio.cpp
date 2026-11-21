#include "AmigaPaulaAudio.h"
#include "framework/AmigaHardware.h"
#include "framework/AmigaPlatform.h"
#include <proto/exec.h>
#include <exec/memory.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

namespace downwell {
AmigaPaulaAudio* AmigaPaulaAudio::current_=nullptr;
namespace {
constexpr UWORD audioDma=DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3;
constexpr UWORD audioIrq=INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3;
constexpr UWORD modulation=0x00ff; // ADKCON USExVy / USExPy: no channel modulates another
volatile unsigned char& ciaPort(){return *reinterpret_cast<volatile unsigned char*>(0xbfe001UL);}
unsigned rasterLine(){
    const ULONG v=*reinterpret_cast<volatile ULONG*>(0xdff004UL); // VPOSR:VHPOSR
    return unsigned((v>>8)&0x1ffu);
}
}
// PaulaSequencer's register interface on the hardware.
struct AmigaPaulaAudio::Regs {
    const UBYTE* data[2];
    const UBYTE* silent;
    static UWORD base(unsigned c){return UWORD(aud0ptr+c*16);}
    static void pointer(unsigned c,const UBYTE* p,UWORD words){
        const ULONG a=reinterpret_cast<ULONG>(p);
        AmigaHardware::writeRegister(base(c),UWORD(a>>16));
        AmigaHardware::writeRegister(UWORD(base(c)+2),UWORD(a&0xffff));
        AmigaHardware::writeRegister(UWORD(base(c)+4),words);
    }
    void dma(unsigned mask,bool on){AmigaHardware::setDMAChannels(UWORD(mask&15u)|(on?DMAF_MASTER:0),on);}
    void sample(unsigned c,unsigned slot,std::uint32_t offset,std::uint16_t words){pointer(c,data[slot&1u]+offset,words);}
    void silence(unsigned c){pointer(c,silent,1);}
    void period(unsigned c,std::uint16_t p){AmigaHardware::writeRegister(UWORD(base(c)+6),p);}
    void volume(unsigned c,std::uint8_t v){AmigaHardware::writeRegister(UWORD(base(c)+8),v);}
};

bool AmigaPaulaAudio::bind(Slot slot,const void* pack,unsigned bytes){
    if(banks_[slot].ready())return false;
    PaulaBank bank;
    if(!bank.bind(pack,bytes)||!(TypeOfMem(const_cast<void*>(pack))&MEMF_CHIP))return false;
    // The plan reads the slot only in the main loop, and the VBlank only
    // from published commands: no command names an unbound slot.
    banks_[slot]=bank;
    return true;
}

void AmigaPaulaAudio::unbind(Slot slot){
    if(!banks_[slot].ready())return;
    if(installed_){
        if(plan_.stopSlot(slot)){
            frame();
            // The next VBlank applies the stops (volume 0, DMA off); wait
            // two so a VBlank already under way cannot be the one counted.
            platform_->waitVblank();platform_->waitVblank();
        }
    }
    banks_[slot]=PaulaBank{};
}

bool AmigaPaulaAudio::acquire(AmigaPlatform& platform){
    if(installed_)return platform_==&platform;
    if(platform_||current_)return false;
    // Reserve the active platform service first. Before takeover this
    // rejects immediately, with nothing allocated or touched.
    if(!platform.attachVblankService(vblank,this,
        [](void* self){static_cast<AmigaPaulaAudio*>(self)->release();}))return false;
    platform_=&platform;
    silent_=static_cast<UBYTE*>(AllocMem(silentBytes,MEMF_CHIP|MEMF_CLEAR));
    if(!silent_){release();return false;}
    // Take Paula only if no other driver (the wind stream, an OS client)
    // runs a channel or its interrupt.
    Disable();
    const bool available=!(AmigaHardware::enabledDMAChannels()&audioDma)&&
        !(AmigaHardware::enabledInterrupts()&audioIrq)&&!current_;
    if(available){
        savedAdk_=UWORD(AmigaHardware::readRegister(adkconr)&modulation);
        AmigaHardware::writeRegister(adkcon,modulation);
        savedFilter_=(ciaPort()&2)!=0;ciaPort()|=2; // LED low-pass off
        AmigaHardware::clearInterruptRequests(audioIrq);
        for(auto& m:mailbox_)m=PaulaChannelCommand{};
        Regs regs{{banks_[0].data(),banks_[1].data()},silent_};
        sequencer_.begin(regs);
        plan_.reset();
        lastField_=AmigaPlatform::vblankCount();
        current_=this;installed_=true;
    }
    Enable();
    if(!available){release();return false;}
    return true;
}

void AmigaPaulaAudio::release(){
    if(installed_){
        // Silence and DMA-off, then a whole VBlank before the silent word or
        // the caller's bank can be freed: nothing may still fetch them.
        Disable();
        Regs regs{{banks_[0].data(),banks_[1].data()},silent_};
        sequencer_.end(regs);
        installed_=false;
        Enable();
        platform_->waitVblank();
        platform_->detachVblankService(this);
        Disable();
        AmigaHardware::setDMAChannels(audioDma,false);
        AmigaHardware::writeRegister(adkcon,modulation);
        AmigaHardware::writeRegister(adkcon,UWORD(0x8000|savedAdk_));
        if(savedFilter_)ciaPort()|=2;else ciaPort()&=~2;
        current_=nullptr;
        Enable();
    }
    if(platform_)platform_->detachVblankService(this);
    platform_=nullptr;
    if(silent_){FreeMem(silent_,silentBytes);silent_=nullptr;}
    banks_[0]=PaulaBank{};banks_[1]=PaulaBank{};
}

void AmigaPaulaAudio::frame(){
    if(!installed_)return;
    const UWORD now=AmigaPlatform::vblankCount();
    const UWORD fields=UWORD(now-lastField_);
    lastField_=now;
    if(plan_.idle())return; // silence costs nothing beyond Paula's DMA
    plan_.advance(fields);
    Disable();
    plan_.publish([this](unsigned c,const PaulaChannelCommand& m){mailbox_[c].merge(m);pending_=true;});
    Enable();
}

void AmigaPaulaAudio::vblank(void* self){static_cast<AmigaPaulaAudio*>(self)->serviceVblank();}
void AmigaPaulaAudio::serviceVblank(){
    if(!installed_||(!pending_&&!sequencer_.busy()))return; // idle: nothing to do
    pending_=false;
    const unsigned first=rasterLine();
    Regs regs{{banks_[0].data(),banks_[1].data()},silent_};
    sequencer_.vblank(mailbox_,regs);
    const unsigned lines=rasterLine()-first;
    if(lines<312&&lines>longestService_)longestService_=lines;
}
}
