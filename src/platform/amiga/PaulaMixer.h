#ifndef DOWNWELL_AMIGA_PAULA_MIXER_H
#define DOWNWELL_AMIGA_PAULA_MIXER_H
// W2.4: the four-voice Paula mixer's portable half. No Amiga headers: the
// host tests it, renders its expected output, and replays route logs with it.
//
// - PaulaBank reads a DWSX pack (tools/pack-amiga-sfx.py).
// - PaulaVoicePlan runs in the main loop: it turns SoundRequests into one
//   pending command per channel (allocation, stealing, panning, one-shot
//   expiry, fades, pitch and volume), with no knowledge of the hardware.
// - PaulaSequencer runs in the VBlank: it applies the published commands to
//   Paula's registers through a Regs interface (the hardware driver on the
//   Amiga, a Paula model on the host).
//
// Hardware strategy: the four channels' DMA stays on while the mixer owns
// Paula, each idling on a two-byte silent loop at volume 0. A start on an
// idle channel is a pointer, length, period and volume write, which Paula
// latches when the silent word wraps (within two periods); one VBlank later
// a one-shot's reload registers go back to the silent word, so it ends on
// silence. A start that cuts a sound still playing, and a stop, turn the
// channel's DMA off in one VBlank and program it in the next (DMA control
// only in blanking). Loops leave their reload registers on the sample.
#include "core/SoundRequests.h"
#include <cstddef>
#include <cstdint>

namespace downwell {

// 16-bit multiply and 32/16 divide, the 68000's MULU.W and DIVU.W, so the
// mixer's per-request arithmetic calls no libgcc 32-bit helpers.
namespace paula_math {
inline std::uint32_t mulu(std::uint16_t a,std::uint16_t b){return std::uint32_t(a)*b;}
// The quotient must fit in 16 bits.
inline std::uint16_t divu(std::uint32_t n,std::uint16_t d){
#if defined(__mc68000__) || defined(__m68k__)
    asm("divu.w %1,%0":"+d"(n):"dm"(d));
    return std::uint16_t(n);
#else
    return std::uint16_t(n/d);
#endif
}
}

class PaulaBank {
public:
    struct Sound {
        std::uint32_t offset=0,bytes=0; // from data()
        std::uint16_t id=0,period=0,assetGainQ15=0;
        std::uint8_t volume=0;
        bool loop=false;
    };
    static constexpr unsigned header=32,entryBytes=16;
    bool bind(const void* bytes,std::size_t size){
        p_=nullptr;count_=0;indexSize_=0;
        const auto* p=static_cast<const std::uint8_t*>(bytes);
        if(!p||(reinterpret_cast<std::size_t>(p)&1)||size<header||
           p[0]!='D'||p[1]!='W'||p[2]!='S'||p[3]!='X'||u16(p+4)!=2)return false;
        const unsigned count=u16(p+6),indexSize=u16(p+16);
        const std::uint32_t total=u32(p+8),data=u32(p+12);
        const std::uint32_t index=header+entryBytes*count;
        if(total!=size||count>255||data!=index+indexSize+(indexSize&1)||data>size)return false;
        int previous=-1;
        for(unsigned i=0;i<count;++i){
            const auto* e=p+header+entryBytes*i;
            const std::uint32_t offset=u32(e+8),length=u32(e+12);
            if(int(u16(e))<=previous||u16(e)>=indexSize||p[index+u16(e)]!=i+1||!u16(e+2)||e[4]>64||!u16(e+6)||
               !length||(length&1)||(offset&1)||length>131070u||
               offset>size-data||length>size-data-offset)return false;
            previous=u16(e);
        }
        for(unsigned id=0;id<indexSize;++id)if(p[index+id]>count)return false;
        p_=p;count_=count;indexSize_=indexSize;index_=p+index;data_=p+data;return true;
    }
    bool ready()const{return p_!=nullptr;}
    unsigned count()const{return count_;}
    const std::uint8_t* data()const{return data_;}
    // The entry number of a sound id, or -1: one table read.
    int entry(int id)const{
        if(unsigned(id)>=indexSize_)return -1;
        return int(index_[id])-1;
    }
    bool sound(unsigned index,Sound& s)const{
        if(index>=count_)return false;
        const auto* e=p_+header+entryBytes*index;
        s.id=be16(e);s.period=be16(e+2);s.volume=e[4];s.loop=(e[5]&1)!=0;
        s.assetGainQ15=be16(e+6);s.offset=be32(e+8);s.bytes=be32(e+12);
        return true;
    }
    bool find(int id,Sound& s)const{const int e=entry(id);return e>=0&&sound(unsigned(e),s);}
private:
    static std::uint16_t u16(const std::uint8_t* p){return std::uint16_t((p[0]<<8)|p[1]);}
    static std::uint32_t u32(const std::uint8_t* p){return (std::uint32_t(u16(p))<<16)|u16(p+2);}
    // Entries are word-aligned: the big-endian 68000 loads them directly.
    static std::uint16_t be16(const std::uint8_t* p){
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
        return *static_cast<const std::uint16_t*>(__builtin_assume_aligned(p,2));
#else
        return u16(p);
#endif
    }
    static std::uint32_t be32(const std::uint8_t* p){return (std::uint32_t(be16(p))<<16)|be16(p+2);}
    const std::uint8_t* p_=nullptr;
    const std::uint8_t* index_=nullptr;
    const std::uint8_t* data_=nullptr;
    unsigned count_=0,indexSize_=0;
};

// One channel's pending change, published by the main loop once per frame.
struct PaulaChannelCommand {
    enum Op : std::uint8_t { none, start, stop };
    Op op=none;
    bool cut=false;       // start: the channel may still be sounding, restart its DMA
    bool loop=false;      // start: keep the reload registers on the sample
    bool periodDirty=false,volumeDirty=false;
    std::uint8_t volume=0;
    std::uint16_t period=0,words=0;
    std::uint32_t offset=0; // start: sample offset from the bank's data
    // Fold a later command into an unconsumed one: a start or stop replaces
    // the operation (a start after anything unconsumed must cut), and a
    // later period or volume rides along with whatever is pending.
    void merge(const PaulaChannelCommand& later){
        if(later.op!=none){
            const bool pending=op!=none;
            const bool wasCut=cut;
            *this=later;
            if(later.op==start&&pending)cut=true;
            if(later.op==start&&wasCut)cut=true;
            return;
        }
        if(later.periodDirty){period=later.period;periodDirty=true;}
        if(later.volumeDirty){volume=later.volume;volumeDirty=true;}
    }
};

class PaulaVoicePlan {
public:
    static constexpr unsigned channels=4;
    static constexpr std::uint32_t fieldClocks=71051; // a non-interlaced PAL frame: 313 lines x 227
    static constexpr std::uint16_t fieldHalfClocks=35525; // fieldClocks / 2, rounded down (expiry is late, never early)
    static constexpr std::uint16_t minPeriod=124;
    // A play only claims its channel (fresh); publish materialises the
    // channel's final claimant once (bank entry, gain, pitch, period, length
    // and volume), so the plays a burst steals within one frame cost little.
    struct Voice {
        bool active=false,fresh=false,loop=false,paused=false,gainKnown=false;
        std::uint8_t entry=0,priority=0,bankVolume=0,volume=0;
        std::int16_t asset=-1;
        int id=-1;
        std::uint16_t basePeriod=0,period=0,assetGainQ15=1,fadeFields=0;
        std::uint32_t serial=0,remaining=0,gainQ16=0,pitchQ16=65536,fadeTargetQ16=0; // remaining: half colour clocks
        std::int32_t fadeStepQ16=0;
        MotionBinary64 gainRaw{},pitchRaw{}; // fresh: the request's values
    };
    struct Counters { std::uint32_t plays=0,steals=0,drops=0,missing=0,music=0,expired=0,stops=0; };

    explicit PaulaVoicePlan(const PaulaBank& bank):bank_(bank){}
    // Forget every voice, command and counter (a new acquisition).
    void reset(){
        for(unsigned c=0;c<channels;++c){voices_[c].active=voices_[c].fresh=false;pending_[c]=PaulaChannelCommand{};sounding_[c]=false;}
        counters_={};serial_=0;lastSide_=1;
    }
    // WM may take channels for music later: the plan never allocates them.
    void reserveChannels(unsigned mask){reserved_=mask&15u;}
    // audio_master_gain x global.gameVolume (W2.6 group 6), Q16, at most 1.
    void setMasterQ16(std::uint32_t master){
        master_=master>65536u?65536u:master;
        for(unsigned c=0;c<channels;++c)if(voices_[c].active&&!voices_[c].fresh)refreshVolume(c);
    }
    const Voice& voice(unsigned c)const{return voices_[c];}
    const Counters& counters()const{return counters_;}
    unsigned activeVoices()const{unsigned n=0;for(const auto& v:voices_)n+=v.active;return n;}

    void apply(const SoundRequest& r){
        switch(r.kind){
        case SoundRequest::play:play(r);break;
        case SoundRequest::stop:each(r.voice,[&](unsigned c){stopChannel(c);});break;
        case SoundRequest::stopAll:for(unsigned c=0;c<channels;++c)if(voices_[c].active)stopChannel(c);break;
        case SoundRequest::pauseAll:for(unsigned c=0;c<channels;++c)setPaused(c,true);break;
        case SoundRequest::resumeAll:for(unsigned c=0;c<channels;++c)setPaused(c,false);break;
        case SoundRequest::pause:each(r.voice,[&](unsigned c){setPaused(c,true);});break;
        case SoundRequest::resume:each(r.voice,[&](unsigned c){setPaused(c,false);});break;
        case SoundRequest::gain:each(r.voice,[&](unsigned c){setGain(c,r.gainValue,r.fadeMs);});break;
        case SoundRequest::pitch:each(r.voice,[&](unsigned c){setPitch(c,r.pitchValue);});break;
        }
    }
    // Fields elapsed since the last call: fades step, one-shots expire.
    void advance(unsigned fields){
        if(!fields)return;
        for(unsigned c=0;c<channels;++c){
            Voice& v=voices_[c];
            if(!v.active||v.fresh)continue;
            if(v.fadeFields){
                const unsigned steps=fields<v.fadeFields?fields:v.fadeFields;
                v.fadeFields=std::uint16_t(v.fadeFields-steps);
                v.gainQ16=v.fadeFields?std::uint32_t(std::int32_t(v.gainQ16)+v.fadeStepQ16*std::int32_t(steps)):v.fadeTargetQ16;
                refreshVolume(c);
            }
            if(v.loop)continue;
            const std::uint32_t used=paula_math::mulu(std::uint16_t(fields<65535u?fields:65535u),fieldHalfClocks);
            if(used>=v.remaining){v.active=false;sounding_[c]=false;++counters_.expired;}
            else v.remaining-=used;
        }
    }
    // Materialise the frame's claims, then hand each channel's command to
    // the sequencer's mailbox (publishChannel merges).
    template<class Publish> void publish(Publish publishChannel){
        for(unsigned c=0;c<channels;++c){
            if(voices_[c].fresh)materialise(c);
            if(pending_[c].op==PaulaChannelCommand::none&&!pending_[c].periodDirty&&!pending_[c].volumeDirty)continue;
            publishChannel(c,pending_[c]);
            pending_[c]=PaulaChannelCommand{};
        }
    }
    // MotionBinary64 (a nonnegative double) as Q16, saturating at 256.
    static std::uint32_t q16(MotionBinary64 v){
        if(v.high&0x80000000u)return 0;
        const int e=int((v.high>>20)&0x7ffu)-1023;
        if(e<-17)return 0;
        if(e>=8)return 0xffffffu;
        const std::uint32_t mantissa=(1u<<20)|(v.high&0xfffffu); // 1.20
        return e>=4?mantissa<<(e-4):mantissa>>(4-e);
    }
    static unsigned side(unsigned channel){return channel==0||channel==3?0u:1u;}
private:
    static std::uint32_t clampGain(std::uint32_t g){return g>131072u?131072u:g;}
    template<class F> void each(int id,F f){
        for(unsigned c=0;c<channels;++c){
            const Voice& v=voices_[c];
            if(v.active&&(v.id==id||(id>=0&&id<SoundVoices::firstVoice&&v.asset==id)))f(c);
        }
    }
    void play(const SoundRequest& r){
        if(r.category==5){++counters_.music;return;} // WM
        const int entry=bank_.entry(r.asset);
        if(entry<0){++counters_.missing;return;}
        const int c=choose(r.priority);
        if(c<0){++counters_.drops;return;}
        Voice& v=voices_[c];
        if(v.active)++counters_.steals;
        ++counters_.plays;
        v.active=v.fresh=true;v.paused=v.gainKnown=false;v.fadeFields=0;
        v.entry=std::uint8_t(entry);v.id=r.voice;v.asset=r.asset;v.priority=r.priority;v.serial=++serial_;
        v.gainRaw=r.gainValue;v.pitchRaw=r.pitchValue;
        lastSide_=side(unsigned(c));
    }
    void materialise(unsigned c){
        Voice& v=voices_[c];
        v.fresh=false;
        if(!v.active)return;
        PaulaBank::Sound s;bank_.sound(v.entry,s);
        v.loop=s.loop;v.bankVolume=s.volume;v.basePeriod=s.period;v.assetGainQ15=s.assetGainQ15;
        if(!v.gainKnown)v.gainQ16=clampGain(q16(v.gainRaw));
        v.pitchQ16=q16(v.pitchRaw);
        computePeriod(v);
        // A byte lasts one period: words x period half colour clocks, plus
        // one field of margin (the start waits for a VBlank, then latches).
        const std::uint32_t length=paula_math::mulu(std::uint16_t(s.bytes/2),v.period);
        v.remaining=length<0xffffffffu-fieldHalfClocks?length+fieldHalfClocks:0xffffffffu;
        v.volume=effectiveVolume(v);
        PaulaChannelCommand m;
        m.op=PaulaChannelCommand::start;m.cut=sounding_[c];m.loop=s.loop;
        m.offset=s.offset;m.words=std::uint16_t(s.bytes/2);m.period=v.period;m.volume=v.volume;
        pending_[c].merge(m);
        sounding_[c]=true;
    }
    // A free channel on the side opposite the last start, then any free one;
    // otherwise the paused voice or the lowest priority (the oldest on ties)
    // if the new sound's priority is at least as high.
    int choose(std::uint8_t priority)const{
        int any=-1;
        for(unsigned c=0;c<channels;++c){
            if((reserved_>>c)&1u||voices_[c].active)continue;
            if(side(c)!=lastSide_)return int(c);
            if(any<0)any=int(c);
        }
        if(any>=0)return any;
        int victim=-1;
        for(unsigned c=0;c<channels;++c){
            if((reserved_>>c)&1u)continue;
            const Voice& v=voices_[c];
            if(victim<0){victim=int(c);continue;}
            const Voice& w=voices_[victim];
            if(v.paused!=w.paused){if(v.paused)victim=int(c);continue;}
            if(v.priority<w.priority||(v.priority==w.priority&&v.serial<w.serial))victim=int(c);
        }
        if(victim<0)return -1;
        const Voice& w=voices_[victim];
        return w.paused||priority>=w.priority?victim:-1;
    }
    // A fresh claim never reached the hardware: stopping it only cuts what
    // the channel still sounds from before.
    void stopChannel(unsigned c){
        Voice& v=voices_[c];
        v.active=v.fresh=false;++counters_.stops;
        if(!sounding_[c])return;
        PaulaChannelCommand m;m.op=PaulaChannelCommand::stop;
        pending_[c].merge(m);
        sounding_[c]=false;
    }
    void setPaused(unsigned c,bool paused){
        Voice& v=voices_[c];
        if(!v.active||v.paused==paused)return;
        v.paused=paused;
        if(!v.fresh)refreshVolume(c);
    }
    void setGain(unsigned c,MotionBinary64 gain,unsigned fadeMs){
        Voice& v=voices_[c];
        if(!fadeMs){
            v.fadeFields=0;
            if(v.fresh){v.gainRaw=gain;v.gainKnown=false;return;}
            v.gainQ16=clampGain(q16(gain));refreshVolume(c);return;
        }
        if(v.fresh&&!v.gainKnown){v.gainQ16=clampGain(q16(v.gainRaw));v.gainKnown=true;}
        const std::uint32_t target=clampGain(q16(gain));
        v.fadeFields=std::uint16_t((fadeMs+19u)/20u);
        v.fadeTargetQ16=target;
        v.fadeStepQ16=(std::int32_t(target)-std::int32_t(v.gainQ16))/std::int32_t(v.fadeFields);
        if(!v.fresh)refreshVolume(c);
    }
    void setPitch(unsigned c,MotionBinary64 value){
        Voice& v=voices_[c];
        if(v.fresh){v.pitchRaw=value;return;}
        v.pitchQ16=q16(value);
        const std::uint16_t before=v.period;
        computePeriod(v);
        if(v.period==before)return;
        // The rest of the sample now plays at the new period (rare: a
        // live voice's pitch change, not the play's own).
        if(!v.loop)v.remaining=v.remaining/before*v.period;
        PaulaChannelCommand m;m.periodDirty=true;m.period=v.period;
        pending_[c].merge(m);
    }
    void computePeriod(Voice& v){
        if(v.pitchQ16==65536u){v.period=v.basePeriod<minPeriod?minPeriod:v.basePeriod;return;}
        std::uint32_t period=v.pitchQ16?(std::uint32_t(v.basePeriod)*65536u+v.pitchQ16/2)/v.pitchQ16:65535u;
        if(period<minPeriod)period=minPeriod;
        if(period>65535u)period=65535u;
        v.period=std::uint16_t(period);
    }
    // Paula volume = bank volume x (voice gain / asset gain) x master. The
    // bank volume already holds the asset volume, so a script gain change is
    // relative to it (sndgain = gain x 0.7 is 0.7 of the bank volume).
    std::uint8_t effectiveVolume(const Voice& v)const{
        using namespace paula_math;
        if(v.paused)return 0;
        std::uint16_t volume=v.bankVolume;
        if(v.gainQ16!=2u*v.assetGainQ15){
            // bank volume x gain / asset gain, both as Q14 (gain <= 2).
            const std::uint16_t gain=std::uint16_t(v.gainQ16>>2);
            const std::uint16_t asset=std::uint16_t(v.assetGainQ15>>1?v.assetGainQ15>>1:1);
            const std::uint32_t n=mulu(v.bankVolume,gain)+asset/2u;
            volume=(n>>16)>=asset?std::uint16_t(64):divu(n,asset);
        }
        if(master_!=65536u)volume=std::uint16_t(mulu(volume>64u?64u:volume,std::uint16_t(master_>>2))>>14);
        return std::uint8_t(volume>64u?64u:volume);
    }
    void refreshVolume(unsigned c){
        Voice& v=voices_[c];
        const std::uint8_t volume=effectiveVolume(v);
        if(volume==v.volume)return;
        v.volume=volume;
        PaulaChannelCommand m;m.volumeDirty=true;m.volume=volume;
        pending_[c].merge(m);
    }
    const PaulaBank& bank_;
    Voice voices_[channels]{};
    PaulaChannelCommand pending_[channels]{};
    bool sounding_[channels]{}; // the hardware plays (or was told to play) this channel's last start
    Counters counters_{};
    std::uint32_t serial_=0,master_=65536u;
    unsigned reserved_=0,lastSide_=1;
};

// The VBlank half. Regs: dma(channelMask,on), sample(c,offset,words),
// silence(c), period(c,p), volume(c,v). Each call runs inside one VBlank.
class PaulaSequencer {
public:
    enum State : std::uint8_t { idle, oneShotStarted, playing, stopped };
    static constexpr std::uint16_t idlePeriod=PaulaVoicePlan::minPeriod;
    template<class Regs> void begin(Regs& regs){
        for(unsigned c=0;c<4;++c){
            regs.volume(c,0);regs.period(c,idlePeriod);regs.silence(c);
            state_[c]=idle;deferred_[c]=PaulaChannelCommand{};
        }
        regs.dma(15u,true);
    }
    template<class Regs> void end(Regs& regs){
        regs.dma(15u,false);
        for(unsigned c=0;c<4;++c){regs.volume(c,0);state_[c]=idle;}
    }
    // mailbox: the commands published since the last VBlank (consumed).
    template<class Regs> void vblank(PaulaChannelCommand* mailbox,Regs& regs){
        unsigned off=0,on=0;
        for(unsigned c=0;c<4;++c){
            PaulaChannelCommand m=mailbox[c];mailbox[c]=PaulaChannelCommand{};
            if(state_[c]==oneShotStarted){regs.silence(c);state_[c]=playing;}
            if(state_[c]==stopped){
                // DMA went off in the last VBlank: start the deferred (or a
                // newly published) sound, or go back to the silent idle loop.
                PaulaChannelCommand next=deferred_[c];deferred_[c]=PaulaChannelCommand{};
                if(m.op!=PaulaChannelCommand::none){next=m;m=PaulaChannelCommand{};}
                else{next.merge(m);m=PaulaChannelCommand{};}
                if(next.op==PaulaChannelCommand::start){program(c,next,regs);}
                else{regs.volume(c,0);regs.period(c,idlePeriod);regs.silence(c);state_[c]=idle;}
                on|=1u<<c;
                continue;
            }
            if(m.op==PaulaChannelCommand::start&&!m.cut){program(c,m,regs);continue;}
            if(m.op!=PaulaChannelCommand::none){
                regs.volume(c,0);off|=1u<<c;state_[c]=stopped;
                if(m.op==PaulaChannelCommand::start)deferred_[c]=m;
                continue;
            }
            if(m.periodDirty)regs.period(c,m.period);
            if(m.volumeDirty)regs.volume(c,m.volume);
        }
        if(off)regs.dma(off,false);
        if(on)regs.dma(on,true);
    }
    State state(unsigned c)const{return state_[c];}
private:
    template<class Regs> void program(unsigned c,const PaulaChannelCommand& m,Regs& regs){
        regs.sample(c,m.offset,m.words);regs.period(c,m.period);regs.volume(c,m.volume);
        state_[c]=m.loop?playing:oneShotStarted;
    }
    State state_[4]{};
    PaulaChannelCommand deferred_[4]{};
};
}
#endif
