#include "AmigaWindAudio.h"
#include "core/MultiplyBinary32.h"
#include "framework/AmigaHardware.h"
#include "framework/AmigaPlatform.h"
#include "amiga-wind.h"
#include <proto/exec.h>
#include <exec/memory.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <limits.h>
#include <cstring>

#ifdef DOWNWELL_WIND_AUDIO_VERIFY
extern "C" void downwellWindQueued(unsigned,const unsigned char*,unsigned,bool);
extern "C" void downwellWindIrq();
#endif

namespace downwell {
AmigaWindAudio* AmigaWindAudio::current_=nullptr;
namespace {
constexpr unsigned audioDma=DMAF_AUD0|DMAF_AUD1;
constexpr unsigned audioIrq=INTF_AUD0|INTF_AUD1;
volatile unsigned char& ciaPort(){return *reinterpret_cast<volatile unsigned char*>(0xbfe001UL);}
void hardwareVolume(unsigned volume){
    AmigaHardware::writeRegister(aud0vol,volume);
    AmigaHardware::writeRegister(aud1vol,volume);
}
}
bool AmigaWindAudio::acquire(AmigaPlatform& platform){
    if(ready())return platform_==&platform;
    if(platform_||current_)return false;
    // Reserve the active platform service first. Before takeover this rejects
    // immediately: no IRQ installation, allocation or wait on our absent VBI.
    playing_=false;command_=0;
    if(!platform.attachVblankService(vblank,this,
        [](void* self){static_cast<AmigaWindAudio*>(self)->release();}))return false;
    platform_=&platform;
    source_=static_cast<std::uint8_t*>(AllocMem(DOWNWELL_WIND_BANK_BYTES,MEMF_PUBLIC));
    chip_=static_cast<std::uint8_t*>(AllocMem(chipBytes,MEMF_CHIP|MEMF_CLEAR));
    if(source_&&chip_)AmigaHardware::blitterDrain();
    if(!source_||!chip_||platform.loadFile("Downwell.wind",source_,DOWNWELL_WIND_BANK_BYTES)!=DOWNWELL_WIND_BANK_BYTES||
       !bank_.bind(source_,DOWNWELL_WIND_BANK_BYTES)){
        release();return false;
    }
    // This takeover driver must not steal a channel from an OS audio client.
    // No saved live DMA pointer is readable from Paula's write-only registers.
    Disable();
    const bool available=!(AmigaHardware::enabledDMAChannels()&audioDma)&&
        !(AmigaHardware::enabledInterrupts()&audioIrq)&&!current_;
    if(available){
        savedVector_=SysBase->IntVects[INTB_AUD0];
        SysBase->IntVects[INTB_AUD0].iv_Data=0;
        SysBase->IntVects[INTB_AUD0].iv_Code=(void(*)())interrupt;
        savedAdk_=AmigaHardware::readRegister(adkconr)&0x33;
        AmigaHardware::writeRegister(adkcon,0x33); // No channel 0/1 modulation.
        savedFilter_=(ciaPort()&2)!=0;ciaPort()|=2; // Disable analogue low-pass.
        current_=this;installed_=true;
        AmigaHardware::clearInterruptRequests(audioIrq);
    }
    Enable();
    if(!available){release();return false;}
    return true;
}
void AmigaWindAudio::halt(){
    AmigaHardware::setInterrupts(audioIrq,false);
    AmigaHardware::setDMAChannels(audioDma,false);
    hardwareVolume(0);
    AmigaHardware::clearInterruptRequests(audioIrq);
}
void AmigaWindAudio::release(){
    if(installed_){
        // Muting and DMA-off precede a whole VBI before freeing either source
        // or Chip bytes. No outstanding reload can outlive its backing.
        Disable();playing_=false;command_=3;Enable();
        platform_->waitVblank();
        platform_->detachVblankService(this);
        Disable();halt();
        SysBase->IntVects[INTB_AUD0]=savedVector_;
        AmigaHardware::writeRegister(adkcon,0x33);
        AmigaHardware::writeRegister(adkcon,0x8000|savedAdk_);
        if(savedFilter_)ciaPort()|=2;else ciaPort()&=~2;
        current_=nullptr;installed_=false;Enable();
    }
    if(platform_)platform_->detachVblankService(this);
    platform_=nullptr;bank_.reset();sample_={};
    if(chip_){FreeMem(chip_,chipBytes);chip_=nullptr;}
    if(source_){FreeMem(source_,DOWNWELL_WIND_BANK_BYTES);source_=nullptr;}
}
bool AmigaWindAudio::play(int resource,int /*priority*/,bool loop,int& voice){
    WindBank::Sample sample;
    if(!ready()||loop||!bank_.sample(resource,sample)||nextVoice_==INT_MAX)return false;
    Disable();
    if(playing_){Enable();return false;} // Source wind guard allows one voice.
    // A previous stop may not yet have reached VBI: do not touch DMA buffers.
    sample_=sample;resource_=resource;voice_=nextVoice_++;voice=voice_;
    gain_.current.assignInteger(1);updateVolume();
    playing_=true;command_=1;Enable();return true;
}
#ifdef DOWNWELL_ENCOUNTER_UPDATE_FAILURE_VERIFY
extern "C" {
volatile unsigned gDownwellEncounterAudioRejected=0;
__attribute__((noinline,used)) bool downwellRejectEncounterAudio(){
    ++gDownwellEncounterAudioRejected;return false;
}
}
#endif
bool AmigaWindAudio::isPlaying(int id,bool& playing){
#ifdef DOWNWELL_ENCOUNTER_UPDATE_FAILURE_VERIFY
    // Exercise the actual registered wind-contact callback, not a replacement
    // collision pass. Ordinary builds have no rejection or counter.
    return downwellRejectEncounterAudio();
#endif
    Disable();playing=ready()&&playing_&&matches(id);Enable();return ready();
}
bool AmigaWindAudio::stop(int id){
    if(!ready())return false;
    Disable();if(matches(id)){playing_=false;command_=3;}Enable();return true;
}
bool AmigaWindAudio::stopAll(){
    if(!ready())return false;
    Disable();playing_=false;command_=3;Enable();return true;
}
bool AmigaWindAudio::getGain(int id,MotionBinary64& value){
    if(!ready())return false;
    if(hasResource(id)){value=MotionBinary64::fromInteger(1);return true;}
    if(id!=voice_||id<1000000)return false;
    value=gain_.current.promoted();return true;
}
bool AmigaWindAudio::setGain(int id,MotionBinary64 value,int milliseconds){
    if(!ready()||id!=voice_||id<1000000||milliseconds||!gain_.set(value,0,0))return false;
    updateVolume();return true;
}
bool AmigaWindAudio::setMasterGain(MotionBinary64 value){
    if(!master_.set(value))return false;
    updateVolume();return true;
}
void AmigaWindAudio::updateVolume(){
    // Map the source's float gain to Paula's linear 0..64 volume. Keep the
    // source value unchanged for getGain; clipping is a device limitation.
    RunnerImageValue effective;
    // Both controls are finite and nonnegative, master <= 1. The retained
    // normal-only product can fail only on underflow, inaudible at Paula's
    // 1/64 resolution. It must not pull a floating-point runtime into the IRQ.
    if(!multiplyBinary32(gain_.current,master_.value,effective))effective.bits=0;
    const auto bits=effective.bits&0x7fffffffu; // -0 is silence, not saturation.
    unsigned volume=0;
    if(bits>=0x3f800000u)volume=64;
    else if(bits>=0x3c000000u){
        const unsigned exponent=(bits>>23)&255,shift=144-exponent;
        volume=((bits&0x7fffffu)|0x800000u)>>shift;
    }
    volume_=volume; // Only the low word varies; VBI alone writes AUDxVOL.
}
bool AmigaWindAudio::setPitch(int id,MotionBinary64 value){
    return ready()&&id==voice_&&id>=1000000&&value.high==0x3ff00000u&&!value.low;
}
unsigned AmigaWindAudio::fill(unsigned bank){
    unsigned bytes=sample_.length-offset_;
    if(bytes>fragmentBytes)bytes=fragmentBytes;
    if(bytes)std::memcpy(chip_+bank*fragmentBytes,sample_.bytes+offset_,bytes);
    offset_+=bytes;
    if(bytes&1)chip_[bank*fragmentBytes+bytes++]=0;
    return bytes;
}
void AmigaWindAudio::queue(unsigned bank,unsigned bytes){
    const auto* data=chip_+(silenceQueued_?fragmentBytes*2:bank*fragmentBytes);
    const auto pointer=reinterpret_cast<std::uint32_t>(data);
    for(unsigned channel=0;channel<2;++channel){
        const unsigned base=aud0ptr+channel*16;
        AmigaHardware::writeRegister(base,pointer>>16);
        AmigaHardware::writeRegister(base+2,pointer&0xffff);
        AmigaHardware::writeRegister(base+4,bytes/2);
    }
#ifdef DOWNWELL_WIND_AUDIO_VERIFY
    downwellWindQueued(bank,data,bytes,silenceQueued_);
#endif
}
void AmigaWindAudio::vblank(void* self){static_cast<AmigaWindAudio*>(self)->serviceVblank();}
void AmigaWindAudio::serviceVblank(){
    if(command_==1||command_==3){
        halt();command_=command_==1?2:0;return;
    }
    if(command_==2){
        offset_=0;bankIndex_=0;silenceQueued_=draining_=false;
        queue(0,fill(0));
        AmigaHardware::writeRegister(aud0per,WindBank::period());
        AmigaHardware::writeRegister(aud1per,WindBank::period());
        hardwareVolume(volume_);
        AmigaHardware::clearInterruptRequests(audioIrq);
        command_=0;
        AmigaHardware::setInterrupts(INTF_AUD0,true);
        AmigaHardware::setDMAChannels(audioDma|DMAF_MASTER,true);
        return;
    }
    if(playing_)hardwareVolume(volume_);
}
unsigned long AmigaWindAudio::interrupt(){
    // A different level-4 source can interrupt while AUD0 is masked. Do not
    // consume partially prepared state merely because a request is pending.
    if(!(AmigaHardware::readRegister(intreqr)&AmigaHardware::enabledInterrupts()&INTF_AUD0))return 0;
    AmigaHardware::clearInterruptRequests(INTF_AUD0);
    auto* s=current_;if(!s)return 0;
#ifdef DOWNWELL_WIND_AUDIO_VERIFY
    downwellWindIrq();
#endif
    if(s->command_)return 0; // A pending stop/restart owns the next VBI.
    if(s->draining_){
        // The silent loop is already playing. Stop DMA in VBI rather than
        // changing DMA control in the visible display; mask its rapid IRQs now.
        AmigaHardware::setInterrupts(INTF_AUD0,false);
        s->playing_=false;s->command_=3;return 0;
    }
    const unsigned bank=s->bankIndex_^1;
    unsigned bytes=0;
    if(s->silenceQueued_)s->draining_=true;
    else bytes=s->fill(bank);
    if(!bytes){
        bytes=2;s->silenceQueued_=true;
    }
    s->queue(bank,bytes);s->bankIndex_=bank;return 0;
}
}
