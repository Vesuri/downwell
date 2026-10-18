#include "platform/amiga/AmigaWindAudio.h"
#include "platform/amiga/framework/AmigaPlatform.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include "platform/amiga/framework/CopperList.h"
#include "core/TimeFieldWind.h"
#include <proto/exec.h>
#include <exec/memory.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <cstring>

extern "C" {
volatile unsigned gWindProbePassed=0,gWindProbeFailed=0,gWindProbeIrqs=0,gWindProbeRows=0;
volatile unsigned gWindProbeTrace[512][6]{}; // resource, bytes, VBI, bank, source offset, silence
__attribute__((noinline)) void downwellWindAudioVerified(){asm volatile("" ::: "memory");}
}
namespace {
downwell::WindBank::Sample reference;
volatile unsigned resource=0,offset=0,silence=0;
volatile unsigned latestRow[2]{};
}
extern "C" void downwellWindIrq(){++gWindProbeIrqs;}
extern "C" void downwellWindQueued(unsigned bank,const unsigned char* pcm,unsigned bytes,bool silent){
    if(gWindProbeRows>=512){gWindProbeFailed=1;return;}
    auto* row=gWindProbeTrace[gWindProbeRows++];
    row[0]=resource;row[1]=bytes;row[2]=AmigaPlatform::vblankCount();
    row[3]=bank;row[4]=offset;row[5]=silent;
    if(!silent)latestRow[bank]=gWindProbeRows;
    if(silent){
        if(bytes!=2||pcm[0]||pcm[1]||offset!=reference.length)gWindProbeFailed=2;
        ++silence;
    }else{
        unsigned count=reference.length-offset;
        if(count>4096)count=4096;
        if(!count||bytes!=((count+1)&~1u)||
           ((count&1)&&pcm[count]))gWindProbeFailed=3;
        offset+=count;
    }
}
namespace downwell {
struct AmigaWindAudioProbe {
    static unsigned volume(const AmigaWindAudio& audio){return audio.volume_;}
    static bool sample(AmigaWindAudio& audio,int id,WindBank::Sample& s){return audio.bank_.sample(id,s);}
    static bool check(AmigaWindAudio& audio,unsigned row){
        const unsigned bank=gWindProbeTrace[row][3],at=gWindProbeTrace[row][4];
        if(latestRow[bank]!=row+1)return false;
        const auto* pcm=audio.chip_+bank*audio.fragmentBytes;
        unsigned count=reference.length-at;if(count>4096)count=4096;
        bool same=true;
        for(unsigned i=0;i<count;++i)if(pcm[i]!=reference.bytes[at+i])same=false;
        return same&&latestRow[bank]==row+1;
    }
};
}
int main(){
    AmigaPlatform platform;
    if(!platform.available())return 20;
    downwell::AmigaWindAudio audio;
    if(audio.acquire(platform))return 20; // No pre-takeover device admission.
    auto* words=static_cast<UWORD*>(AllocMem(8,MEMF_CHIP|MEMF_CLEAR));
    if(!words)return 20;
    CopperList copper(words,4);
    if(!copper.move(color00,0)||!copper.finish()||!platform.takeover(copper,0x81,0xc1,44,300,0x38,0xd0))return 20;
    const ULONG chipBefore=AvailMem(MEMF_CHIP),fastBefore=AvailMem(MEMF_FAST);
    const auto savedAudio=SysBase->IntVects[INTB_AUD0];
    if(!audio.acquire(platform)||chipBefore-AvailMem(MEMF_CHIP)!=audio.chipBytes)return 20;
    bool playing=false;int voice=-1;
    if(!audio.isPlaying(-1,playing)||playing||audio.play(348,90,false,voice)||audio.play(349,90,true,voice))return 20;
    for(resource=349;resource<=355;++resource){
        if(!downwell::AmigaWindAudioProbe::sample(audio,resource,reference))return 20;
        offset=silence=0;const unsigned first=gWindProbeRows,irqs=gWindProbeIrqs;
        struct Random {
            unsigned choice,calls=0;
            unsigned chooseIndexSmall(unsigned count){if(count!=7)gWindProbeFailed=5;++calls;return choice;}
        } random{resource-349};
        downwell::TimeFieldWind wind;std::int32_t sound=-1;int channel=-1;
        const auto one=downwell::MotionBinary64::fromInteger(1);
        if(!wind.contact(random,sound,channel,one,audio)||sound!=int(resource)||channel!=sound||
           random.calls!=1||wind.voice<1000000)return 20;
        voice=wind.voice;
        downwell::MotionBinary64 gain;
        if(!audio.getGain(voice,gain)||gain.high!=0x3fe00000u||gain.low||
           !wind.contact(random,sound,channel,one,audio)||random.calls!=1||wind.voice!=voice)return 20;
        if(!audio.setMasterGain({0x3fe33333u,0x33333333u})||
           downwell::AmigaWindAudioProbe::volume(audio)!=19||
           !audio.getGain(voice,gain)||gain.high!=0x3fe00000u||gain.low||
           audio.setMasterGain(downwell::MotionBinary64::fromInteger(2))||
           downwell::AmigaWindAudioProbe::volume(audio)!=19||
           !audio.setMasterGain(one)||downwell::AmigaWindAudioProbe::volume(audio)!=32)return 20;
        if(!audio.setGain(voice,{0x80000000u,0},0)||downwell::AmigaWindAudioProbe::volume(audio)||
           !audio.setGain(voice,{0x3fe00000u,0},0)||downwell::AmigaWindAudioProbe::volume(audio)!=32)return 20;
        int rejected=-1;
        if(audio.play(resource,90,false,rejected)||rejected!=-1||
           audio.setPitch(voice,{0x3fe00000u,0})||audio.setGain(voice,one,100))return 20;
        const UWORD start=platform.vblankCount();unsigned checked=first;
        do {
            platform.waitVblank();
            if(!audio.isPlaying(voice,playing)||gWindProbeFailed||UWORD(platform.vblankCount()-start)>650)return 20;
            while(checked<gWindProbeRows){
                // Silent-loop reloads can happen twice before main resumes.
                // PCM buffers, in contrast, must be checked before reuse.
                if(!gWindProbeTrace[checked][5]&&!downwell::AmigaWindAudioProbe::check(audio,checked)){
                    gWindProbeFailed=4;return 20;
                }
                ++checked;
            }
        }while(playing);
        const unsigned fragments=(reference.length+4095)/4096;
        if(offset!=reference.length||silence!=2||gWindProbeRows-first!=fragments+2||gWindProbeIrqs-irqs!=fragments+2)return 20;
        // Each reload IRQ means the previous fragment has really played, not
        // merely that software wrote its pointer. First reload follows startup.
        // 4096 bytes at period 161 require 9–10 PAL fields, never one field.
        for(unsigned n=first+2;n<gWindProbeRows;++n){
            const unsigned elapsed=UWORD(gWindProbeTrace[n][2]-gWindProbeTrace[n-1][2]);
            const unsigned previousBytes=gWindProbeTrace[n-2][1];
            const unsigned expected=previousBytes*50/22030;
            if(elapsed+1<expected||elapsed>expected+1)return 20;
        }
    }
    // Retire while a start is pending: both logical and DMA state must vanish.
    if(!audio.play(349,90,false,voice))return 20;
    audio.release();
    if(audio.ready()||AmigaHardware::enabledDMAChannels()&(DMAF_AUD0|DMAF_AUD1)||
       AmigaHardware::enabledInterrupts()&(INTF_AUD0|INTF_AUD1)||
       SysBase->IntVects[INTB_AUD0].iv_Code!=savedAudio.iv_Code||
       SysBase->IntVects[INTB_AUD0].iv_Data!=savedAudio.iv_Data||
       AvailMem(MEMF_CHIP)!=chipBefore||AvailMem(MEMF_FAST)!=fastBefore)return 20;
    // Also retire actual active DMA through the platform's error/exit path.
    resource=349;offset=silence=0;
    if(!audio.acquire(platform)||!downwell::AmigaWindAudioProbe::sample(audio,349,reference)||
       !audio.play(349,90,false,voice))return 20;
    platform.waitVblank();platform.waitVblank();platform.waitVblank();
    if(!(AmigaHardware::enabledDMAChannels()&DMAF_AUD0)||!audio.isPlaying(voice,playing)||!playing)return 20;
    platform.restore();
    if(audio.ready()||SysBase->IntVects[INTB_AUD0].iv_Code!=savedAudio.iv_Code)return 20;
    FreeMem(words,8);
    gWindProbePassed=1;downwellWindAudioVerified();return 0;
}
