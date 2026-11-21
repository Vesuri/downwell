#ifndef DOWNWELL_TESTS_PAULA_MODEL_H
#define DOWNWELL_TESTS_PAULA_MODEL_H
// Host model of Paula's four DMA channels as PaulaSequencer drives them:
// registers latch when DMA turns on and whenever a buffer ends, each byte
// plays for `period` colour clocks, and channels 0/3 sum left, 1/2 right.
// Output is zero-order hold with no filter (the comparison recording runs
// FS-UAE with its filter emulation off). It records every register write
// so tests can check the sequencer's protocol.
#include "platform/amiga/PaulaMixer.h"
#include <cstdint>
#include <vector>

namespace downwell_test {
class PaulaModel {
public:
    static constexpr std::uint32_t clock=3546895;
    struct Write { unsigned field,channel; char what; std::uint32_t value; };
    explicit PaulaModel(const std::uint8_t* data):data_(data){}
    // PaulaSequencer's Regs.
    void dma(unsigned mask,bool on){
        for(unsigned c=0;c<4;++c)if((mask>>c)&1u){
            Channel& ch=ch_[c];
            log(c,on?'D':'d',0);
            if(on&&!ch.dma){ch.dma=true;latch(ch);}
            else if(!on){ch.dma=false;ch.out=0;}
        }
    }
    void sample(unsigned c,std::uint32_t offset,std::uint16_t words){
        ch_[c].regSilent=false;ch_[c].regOffset=offset;ch_[c].regWords=words;log(c,'s',offset);
    }
    void silence(unsigned c){ch_[c].regSilent=true;ch_[c].regWords=1;log(c,'z',0);}
    void period(unsigned c,std::uint16_t p){ch_[c].period=p<124?124:p;log(c,'p',p);}
    void volume(unsigned c,std::uint8_t v){ch_[c].volume=v>64?64:v;log(c,'v',v);}

    void setField(unsigned f){field_=f;}
    const std::vector<Write>& writes()const{return writes_;}
    void clearWrites(){writes_.clear();}
    bool playingSample(unsigned c)const{return ch_[c].dma&&!ch_[c].liveSilent;}
    std::uint32_t liveOffset(unsigned c)const{return ch_[c].liveOffset;}
    // Advance `clocks` colour clocks, appending stereo frames at `rate`.
    void render(std::uint32_t clocks,unsigned rate,std::vector<std::int16_t>* out){
        renderFrames(clocks,std::uint32_t((std::uint64_t(clocks)*rate+clock/2)/clock),out);
    }
    // Advance `clocks` colour clocks as `frames` output frames. FS-UAE writes
    // 882 frames (44.1 kHz / 50) per emulated field, whatever the field's
    // length in colour clocks, so its recordings follow this timebase.
    void renderFrames(std::uint32_t clocks,std::uint32_t frames,std::vector<std::int16_t>* out){
        if(!frames)return;
        // Q16 colour clocks per output frame.
        const std::uint64_t step=(std::uint64_t(clocks)<<16)/frames;
        pending_+=std::uint64_t(clocks)<<16;
        while(pending_>=step){
            pending_-=step;
            const std::uint32_t c16=std::uint32_t(step);
            int left=0,right=0;
            for(unsigned c=0;c<4;++c){
                Channel& ch=ch_[c];
                if(!ch.dma)continue;
                ch.acc+=c16;
                const std::uint32_t per=std::uint32_t(ch.period)<<16;
                while(ch.acc>=per){
                    ch.acc-=per;
                    ch.out=ch.liveSilent?0:std::int8_t(data_[ch.liveOffset+ch.pos]);
                    if(++ch.pos>=ch.liveWords*2u)latch(ch);
                }
                const int v=ch.out*int(ch.volume);
                if(c==0||c==3)left+=v;else right+=v;
            }
            if(out){out->push_back(std::int16_t(left*2));out->push_back(std::int16_t(right*2));}
        }
    }
private:
    struct Channel {
        bool dma=false,regSilent=true,liveSilent=true;
        std::uint32_t regOffset=0,liveOffset=0,liveWords=1,pos=0,acc=0;
        std::uint16_t regWords=1,period=124;
        std::uint8_t volume=0;
        std::int8_t out=0;
    };
    void latch(Channel& ch){
        ch.liveSilent=ch.regSilent;ch.liveOffset=ch.regOffset;ch.liveWords=ch.regWords?ch.regWords:65536u;ch.pos=0;
    }
    void log(unsigned c,char what,std::uint32_t value){writes_.push_back({field_,c,what,value});}
    const std::uint8_t* data_;
    Channel ch_[4]{};
    std::vector<Write> writes_;
    std::uint64_t pending_=0;
    unsigned field_=0;
};
}
#endif
