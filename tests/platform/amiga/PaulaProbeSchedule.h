#ifndef DOWNWELL_TESTS_PAULA_PROBE_SCHEDULE_H
#define DOWNWELL_TESTS_PAULA_PROBE_SCHEDULE_H
// W2.4 listening schedule, shared by the Amiga probe (PaulaMixerProbe) and
// the host renderer of its expected output. One step per PAL field:
//
// 1. The reel: every sound of the bank in id order, alone, at its asset
//    gain; loops play for 60 fields and are stopped; eight silent fields
//    follow each sound's length.
// 2. Mixer exercises: four overlapping one-shots on all four channels, a
//    fifth that steals the lowest priority and a sixth that is dropped;
//    a one-shot at pitch 1.8; a loop faded in over 1 s and out over 1.5 s
//    and stopped; a loop paused, resumed and stopped by asset id; a sound
//    at master gain 0.5.
//
// Steps return false once the schedule is over. The probe and the host both
// check the plan's counters afterwards: exactly one steal and one drop.
#include "platform/amiga/PaulaMixer.h"
#include <cstdint>

namespace downwell_test {
class PaulaProbeSchedule {
public:
    static constexpr unsigned gapFields=8,loopFields=60;
    explicit PaulaProbeSchedule(const downwell::PaulaBank& bank):bank_(bank){
        // Six one-shots of at least 0.6 s for the overlap exercise, in id order.
        for(unsigned i=0;i<bank.count()&&picked_<6;++i){
            downwell::PaulaBank::Sound s;bank.sound(i,s);
            if(!s.loop&&s.bytes*std::uint32_t(s.period)>=std::uint32_t(0.6*3546895))overlap_[picked_++]=s.id;
        }
    }
    bool ready()const{return bank_.ready()&&picked_==6&&has(18)&&has(186)&&has(343)&&has(3);}
    unsigned reelEnd()const{return reelEnd_;}
    // Requests for this field (submit(SoundRequest)), and master gain
    // changes (master(Q16)). Returns false when the schedule is over.
    template<class Submit,class Master> bool step(unsigned field,Submit submit,Master master){
        using R=downwell::SoundRequest;
        if(index_<bank_.count()){
            if(field<next_)return true;
            downwell::PaulaBank::Sound s;bank_.sound(index_,s);
            if(stopAt_){
                if(field<stopAt_)return true;
                submit(stop(voice_));stopAt_=0;next_=field+gapFields;++index_;return true;
            }
            submit(play(s.id,50,s.loop,gain(s)));
            if(s.loop){stopAt_=field+loopFields;return true;}
            next_=field+fields(s)+gapFields;++index_;
            if(index_==bank_.count())reelEnd_=next_;
            return true;
        }
        if(index_==bank_.count()&&!reelEnd_)reelEnd_=field;
        const unsigned t=field-reelEnd_;
        switch(t){
        // Overlap: four starts in one field fill the four channels.
        case 0:
            for(unsigned i=0;i<4;++i)submit(play(overlap_[i],std::uint8_t(10+10*i),false,gainOf(overlap_[i])));
            break;
        case 2:submit(play(overlap_[4],25,false,gainOf(overlap_[4])));break; // steals priority 10
        case 3:submit(play(overlap_[5],5,false,gainOf(overlap_[5])));break;  // dropped
        // Pitch 1.8 (the low-ammo gun), then the same at pitch 1.
        case 60:submit(play(18,80,false,gainOf(18),{0x3ffccccdu,0xcccccccdu}));break;
        case 90:submit(play(18,80,false,gainOf(18)));break;
        // A loop faded in from 0 over 1 s, out over 1.5 s, then stopped.
        case 120:{
            submit(play(186,80,true,downwell::MotionBinary64{}));
            R g;g.kind=R::gain;g.voice=voice_;g.gainValue=gainOf(186);g.fadeMs=1000;submit(g);
            break;
        }
        case 195:{R g;g.kind=R::gain;g.voice=fadeVoice();g.gainValue={};g.fadeMs=1500;submit(g);break;}
        case 280:submit(stop(fadeVoice()));break;
        // A loop paused, resumed, then stopped by its asset id.
        case 290:submit(play(343,80,true,gainOf(343)));break;
        case 315:{R p;p.kind=R::pauseAll;submit(p);break;}
        case 340:{R p;p.kind=R::resumeAll;submit(p);break;}
        case 365:submit(stop(343));break;
        // Master gain 0.5 for one sound.
        case 380:master(32768u);submit(play(3,80,false,gainOf(3)));break;
        case 420:master(65536u);break;
        case 440:return false;
        default:break;
        }
        return true;
    }
private:
    bool has(int id)const{downwell::PaulaBank::Sound s;return bank_.find(id,s);}
    static unsigned fields(const downwell::PaulaBank::Sound& s){
        return unsigned((s.bytes*std::uint32_t(s.period)+71050u)/71051u); // < 2^26
    }
    static downwell::MotionBinary64 gain(const downwell::PaulaBank::Sound& s){
        // The asset gain as a double, from Q15 (exact).
        if(!s.assetGainQ15)return {};
        unsigned e=0;std::uint32_t m=s.assetGainQ15;
        while(!(m&0x8000u)){m<<=1;++e;}
        const std::uint32_t exponent=1023u-e; // m/2^15 in [1,2): value = m/2^15 * 2^-e
        return {(exponent<<20)|((m&0x7fffu)<<5),0};
    }
    downwell::MotionBinary64 gainOf(int id)const{downwell::PaulaBank::Sound s;return bank_.find(id,s)?gain(s):downwell::MotionBinary64{};}
    downwell::SoundRequest play(int id,std::uint8_t priority,bool loop,downwell::MotionBinary64 g,
                                downwell::MotionBinary64 pitch={0x3ff00000u,0}){
        downwell::SoundRequest r;r.kind=downwell::SoundRequest::play;r.asset=std::int16_t(id);
        r.voice=voice_=++nextVoice_;r.priority=priority;r.loop=loop;r.cut=true;r.category=8;
        r.gainValue=g;r.pitchValue=pitch;
        if(id==186)fade_=r.voice;
        return r;
    }
    int fadeVoice()const{return fade_;}
    static downwell::SoundRequest stop(int id){downwell::SoundRequest r;r.kind=downwell::SoundRequest::stop;r.voice=id;return r;}
    const downwell::PaulaBank& bank_;
    unsigned index_=0,next_=0,stopAt_=0,reelEnd_=0,picked_=0;
    int voice_=-1,nextVoice_=downwell::SoundVoices::firstVoice-1,fade_=-1;
    int overlap_[6]{};
};
}
#endif
