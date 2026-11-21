// W2.4 listening probe: the Paula mixer plays PaulaProbeSchedule (every bank
// sound, then the mixer exercises) from the Downwell.sfx pack in Chip RAM,
// one schedule step per field. FS-UAE records the output
// (amiga/verify_paula_mixer.sh) and tools/compare-paula-recording.py checks
// it against the host render of the same schedule. GDB checks the counters,
// that no field was missed, and the device and memory retirement.
#include "platform/amiga/AmigaPaulaAudio.h"
#include "platform/amiga/framework/AmigaPlatform.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include "platform/amiga/framework/CopperList.h"
#include "PaulaProbeSchedule.h"
#include "amiga-sfx.h"
#include <proto/exec.h>
#include <exec/memory.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

extern "C" {
volatile unsigned gPaulaProbePassed=0,gPaulaProbeFailed=0,gPaulaProbeFields=0,gPaulaProbeLate=0;
volatile unsigned gPaulaProbePlays=0,gPaulaProbeSteals=0,gPaulaProbeDrops=0,gPaulaProbeMissing=0;
volatile unsigned gPaulaProbeServiceLines=0,gPaulaProbeChipBank=0,gPaulaProbeFrameLines=0;
volatile unsigned gPaulaProbeBench[6]{};
volatile unsigned gPaulaProbePlay16Lines=0,gPaulaProbeGain16Lines=0,gPaulaProbeStepLines=0,gPaulaProbeStepField=0,gPaulaProbeMixLines=0,gPaulaProbeMixField=0;
__attribute__((noinline)) void downwellPaulaProbeStarted(){asm volatile("" ::: "memory");}
__attribute__((noinline)) void downwellPaulaProbeVerified(){asm volatile("" ::: "memory");}
}
namespace {
unsigned rasterLine(){return unsigned((*reinterpret_cast<volatile ULONG*>(0xdff004UL)>>8)&0x1ffu);}
int fail(unsigned reason){gPaulaProbeFailed=reason;return 20;}
__attribute__((noinline)) void benchApply(downwell::PaulaVoicePlan& p,const downwell::SoundRequest& r){p.apply(r);}
__attribute__((noinline)) void benchReset(downwell::PaulaVoicePlan& p){p.reset();}
volatile unsigned gBenchSink=0;
__attribute__((noinline)) void benchPublish(downwell::PaulaVoicePlan& p){
    p.publish([](unsigned c,const downwell::PaulaChannelCommand& m){gBenchSink=gBenchSink+c+m.volume;});
}
}

int main(){
    AmigaPlatform platform;
    if(!platform.available())return fail(1);
    auto* bank=static_cast<UBYTE*>(AllocMem(DOWNWELL_SFX_BANK_BYTES,MEMF_CHIP));
    if(!bank)return fail(2);
    gPaulaProbeChipBank=DOWNWELL_SFX_BANK_BYTES;
    if(platform.loadFile("Downwell.sfx",bank,DOWNWELL_SFX_BANK_BYTES)!=LONG(DOWNWELL_SFX_BANK_BYTES))return fail(3);
    downwell::AmigaPaulaAudio audio;
    if(audio.acquire(platform,bank,DOWNWELL_SFX_BANK_BYTES))return fail(4); // no pre-takeover admission
    auto* words=static_cast<UWORD*>(AllocMem(8,MEMF_CHIP|MEMF_CLEAR));
    if(!words)return fail(5);
    CopperList copper(words,4);
    if(!copper.move(color00,0)||!copper.finish()||!platform.takeover(copper,0x81,0xc1,44,300,0x38,0xd0))return fail(6);
    const ULONG chipBefore=AvailMem(MEMF_CHIP),fastBefore=AvailMem(MEMF_FAST);
    if(!audio.acquire(platform,bank,DOWNWELL_SFX_BANK_BYTES)||
       chipBefore-AvailMem(MEMF_CHIP)!=downwell::AmigaPaulaAudio::silentBytes)return fail(7);
    if((AmigaHardware::enabledDMAChannels()&(DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3))!=
       (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3)||
       (AmigaHardware::enabledInterrupts()&(INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3)))return fail(8);
    downwell_test::PaulaProbeSchedule schedule(audio.bank());
    if(!schedule.ready())return fail(9);
    {
        // Cost of the plan's apply alone: 16 plays (four fill the channels,
        // twelve steal) and 16 gain changes, on a scratch plan.
        downwell::PaulaVoicePlan scratch(audio.bank());
        downwell::SoundRequest r;r.kind=downwell::SoundRequest::play;r.priority=50;
        r.gainValue={0x3fe00000u,0};r.pitchValue={0x3ff00000u,0};
        platform.waitVblank();
        unsigned first=rasterLine();
        for(int i=0;i<16;++i){r.asset=std::int16_t(i&1?18:3);r.voice=downwell::SoundVoices::firstVoice+i;scratch.apply(r);}
        gPaulaProbePlay16Lines=rasterLine()-first;
        downwell::SoundRequest g;g.kind=downwell::SoundRequest::gain;g.gainValue={0x3fd66666u,0x66666666u};
        platform.waitVblank();
        first=rasterLine();
        for(int i=0;i<16;++i){g.voice=downwell::SoundVoices::firstVoice+12+(i&3);scratch.apply(g);}
        gPaulaProbeGain16Lines=rasterLine()-first;
        // Breakdown through noinline calls the optimiser cannot hoist: 16
        // each of reset, reset + one play on a free channel, a play that
        // steals (all four busy), publish of four pending starts, and a gain.
        platform.waitVblank();first=rasterLine();
        for(int i=0;i<16;++i)benchReset(scratch);
        gPaulaProbeBench[0]=rasterLine()-first;
        platform.waitVblank();first=rasterLine();
        for(int i=0;i<16;++i){benchReset(scratch);r.voice=i;benchApply(scratch,r);}
        gPaulaProbeBench[1]=rasterLine()-first;
        for(int i=0;i<4;++i){r.voice=100+i;benchApply(scratch,r);}
        platform.waitVblank();first=rasterLine();
        for(int i=0;i<16;++i){r.voice=200+i;benchApply(scratch,r);}
        gPaulaProbeBench[2]=rasterLine()-first;
        platform.waitVblank();first=rasterLine();
        for(int i=0;i<16;++i){benchPublish(scratch);r.voice=300+i;benchApply(scratch,r);}
        gPaulaProbeBench[3]=rasterLine()-first;
        platform.waitVblank();first=rasterLine();
        for(int i=0;i<16;++i){g.voice=300+(i&3)+12;benchApply(scratch,g);}
        gPaulaProbeBench[4]=rasterLine()-first;
    }
    downwellPaulaProbeStarted();
    platform.waitVblank();
    UWORD lastField=platform.vblankCount();
    for(unsigned field=0;;++field){
        // One step per field, as the host render: VBlank, then the step.
        platform.waitVblank();
        const UWORD now=platform.vblankCount();
        if(UWORD(now-lastField)!=1)++gPaulaProbeLate;
        lastField=now;
        // The schedule's own step (submit: the plan's apply) and the
        // driver's frame() (advance and publish), measured apart.
        const unsigned first=rasterLine();
        const bool more=schedule.step(field,[&](const downwell::SoundRequest& r){audio.submit(r);},
                                      [&](std::uint32_t m){audio.setMasterQ16(m);});
        const unsigned middle=rasterLine();
        audio.frame();
        const unsigned last=rasterLine();
        const unsigned step=middle-first,mix=last-middle,lines=last-first;
        if(step<312&&step>gPaulaProbeStepLines){gPaulaProbeStepLines=step;gPaulaProbeStepField=field;}
        if(mix<312&&mix>gPaulaProbeMixLines){gPaulaProbeMixLines=mix;gPaulaProbeMixField=field;}
        if(lines<312&&lines>gPaulaProbeFrameLines)gPaulaProbeFrameLines=lines;
        gPaulaProbeFields=field;
        if(!more)break;
    }
    const auto& c=audio.plan().counters();
    gPaulaProbePlays=c.plays;gPaulaProbeSteals=c.steals;gPaulaProbeDrops=c.drops;gPaulaProbeMissing=c.missing;
    gPaulaProbeServiceLines=audio.longestServiceLines();
    if(c.steals!=1||c.drops!=1||c.missing||gPaulaProbeLate)return fail(10);
    // Retire: DMA off, the silent word freed, ADKCON and the filter back.
    audio.release();
    if(audio.ready()||(AmigaHardware::enabledDMAChannels()&(DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3))||
       AvailMem(MEMF_CHIP)!=chipBefore||AvailMem(MEMF_FAST)!=fastBefore)return fail(11);
    // A second acquisition after release works, and the platform's restore
    // retires a live mixer.
    if(!audio.acquire(platform,bank,DOWNWELL_SFX_BANK_BYTES))return fail(12);
    downwell::SoundRequest r;r.kind=downwell::SoundRequest::play;r.asset=3;r.voice=downwell::SoundVoices::firstVoice;
    r.priority=50;r.gainValue={0x3ff00000u,0};r.pitchValue={0x3ff00000u,0};
    audio.submit(r);audio.frame();
    platform.waitVblank();platform.waitVblank();
    platform.restore();
    if(audio.ready()||(AmigaHardware::enabledDMAChannels()&(DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3)))return fail(13);
    FreeMem(words,8);
    FreeMem(bank,DOWNWELL_SFX_BANK_BYTES);
    gPaulaProbePassed=1;downwellPaulaProbeVerified();return 0;
}
