#include "AmigaSoundBanks.h"
#include "AmigaDataFiles.h"
#include "framework/AmigaPlatform.h"
#include "amiga-sfx.h"
#include <proto/exec.h>
#include <exec/memory.h>

// GDB reads these: the bank in each slot (AmigaSoundBanks::Bank), the Chip
// the packs hold, loads, and loads skipped or failed.
extern "C" {
volatile ULONG gDownwellSoundReady=0,gDownwellSoundResidentBank=0,gDownwellSoundSceneBank=0;
volatile ULONG gDownwellSoundChipBytes=0,gDownwellSoundBankLoads=0,gDownwellSoundBankFailures=0;
}

namespace downwell {
namespace {
struct Pack { const char* file; ULONG bytes; };
Pack pack(AmigaSoundBanks::Bank bank){
    switch(bank){
    case AmigaSoundBanks::run:return {AmigaDataFile::soundRun,DOWNWELL_SFX_RUN_BYTES};
    case AmigaSoundBanks::front:return {AmigaDataFile::soundFront,DOWNWELL_SFX_FRONT_BYTES};
    case AmigaSoundBanks::results:return {AmigaDataFile::soundResults,DOWNWELL_SFX_RESULTS_BYTES};
    case AmigaSoundBanks::clear:return {AmigaDataFile::soundClear,DOWNWELL_SFX_CLEAR_BYTES};
    default:return {nullptr,0};
    }
}
}

bool AmigaSoundBanks::acquire(AmigaPlatform& platform){
    if(audio_.ready())return platform_==&platform;
    if(!audio_.acquire(platform))return false;
    platform_=&platform;
    publishState();
    return true;
}

void AmigaSoundBanks::release(){
    free(AmigaPaulaAudio::scene);
    free(AmigaPaulaAudio::resident);
    audio_.release();
    platform_=nullptr;
    publishState();
}

void AmigaSoundBanks::free(AmigaPaulaAudio::Slot slot){
    Slot& s=slots_[slot];
    audio_.unbind(slot); // nothing fetches the pack after this
    if(s.memory)FreeMem(s.memory,s.bytes);
    s=Slot{};
}

void AmigaSoundBanks::set(AmigaPaulaAudio::Slot slot,Bank bank){
    if(slots_[slot].bank==bank)return;
    free(slot);
    if(bank!=none&&audio_.ready()){
        const Pack p=pack(bank);
        UBYTE* memory=nullptr;
        if(AvailMem(MEMF_CHIP|MEMF_LARGEST)>=p.bytes+chipMargin)
            memory=static_cast<UBYTE*>(AllocMem(p.bytes,MEMF_CHIP));
        if(memory&&platform_->loadData(p.file,memory,p.bytes)==LONG(p.bytes)&&
           audio_.bind(slot,memory,p.bytes)){
            slots_[slot]={bank,memory,p.bytes};
            ++gDownwellSoundBankLoads;
        }else{
            if(memory)FreeMem(memory,p.bytes);
            ++gDownwellSoundBankFailures;
        }
    }
    publishState();
}

void AmigaSoundBanks::publishState(){
    gDownwellSoundReady=audio_.ready();
    gDownwellSoundResidentBank=slots_[AmigaPaulaAudio::resident].bank;
    gDownwellSoundSceneBank=slots_[AmigaPaulaAudio::scene].bank;
    gDownwellSoundChipBytes=slots_[0].bytes+slots_[1].bytes+(audio_.ready()?AmigaPaulaAudio::silentBytes:0);
}
}
