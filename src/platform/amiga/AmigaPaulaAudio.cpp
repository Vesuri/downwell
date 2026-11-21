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
    const UBYTE* data;
    const UBYTE* silent;
    static UWORD base(unsigned c){return UWORD(aud0ptr+c*16);}
    static void pointer(unsigned c,const UBYTE* p,UWORD words){
        const ULONG a=reinterpret_cast<ULONG>(p);
        AmigaHardware::writeRegister(base(c),UWORD(a>>16));
        AmigaHardware::writeRegister(UWORD(base(c)+2),UWORD(a&0xffff));
        AmigaHardware::writeRegister(UWORD(base(c)+4),words);
    }
    void dma(unsigned mask,bool on){AmigaHardware::setDMAChannels(UWORD(mask&15u)|(on?DMAF_MASTER:0),on);}
    void sample(unsigned c,std::uint32_t offset,std::uint16_t words){pointer(c,data+offset,words);}
    void silence(unsigned c){pointer(c,silent,1);}
    void period(unsigned c,std::uint16_t p){AmigaHardware::writeRegister(UWORD(base(c)+6),p);}
    void volume(unsigned c,std::uint8_t v){AmigaHardware::writeRegister(UWORD(base(c)+8),v);}
};

bool AmigaPaulaAudio::acquire(AmigaPlatform& platform,const void* pack,unsigned bytes){
    if(installed_)return platform_==&platform;
    if(platform_||current_)return false;
    if(!bank_.bind(pack,bytes)||!(TypeOfMem(const_cast<UBYTE*>(bank_.data()))&MEMF_CHIP)){
        bank_=PaulaBank{};return false;
    }
    // Reserve the active platform service first. Before takeover this
    // rejects immediately, with nothing allocated or touched.
    if(!platform.attachVblankService(vblank,this,
        [](void* self){static_cast<AmigaPaulaAudio*>(self)->release();})){bank_=PaulaBank{};return false;}
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
        Regs regs{bank_.data(),silent_};
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
        Regs regs{bank_.data(),silent_};
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
    bank_=PaulaBank{};
}

void AmigaPaulaAudio::frame(){
    if(!installed_)return;
    const UWORD now=AmigaPlatform::vblankCount();
    plan_.advance(UWORD(now-lastField_));
    lastField_=now;
    Disable();
    plan_.publish([this](unsigned c,const PaulaChannelCommand& m){mailbox_[c].merge(m);});
    Enable();
}

void AmigaPaulaAudio::vblank(void* self){static_cast<AmigaPaulaAudio*>(self)->serviceVblank();}
void AmigaPaulaAudio::serviceVblank(){
    if(!installed_)return;
    const unsigned first=rasterLine();
    Regs regs{bank_.data(),silent_};
    sequencer_.vblank(mailbox_,regs);
    const unsigned lines=rasterLine()-first;
    if(lines<312&&lines>longestService_)longestService_=lines;
}
}
