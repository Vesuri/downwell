#ifndef DOWNWELL_LOWEST_FREE_SLOT_MAP_H
#define DOWNWELL_LOWEST_FREE_SLOT_MAP_H
#include <cstdint>
namespace downwell {
// Occupancy only, not an allocator: preserves the lowest-slot scan's result.
// One summary bit per word avoids revisiting thousands of occupied records.
template<unsigned Capacity> class LowestFreeSlotMap {
    static constexpr unsigned Words=(Capacity+31)/32, Summaries=(Words+31)/32;
    std::uint32_t free_[Words?Words:1]{},summary_[Summaries?Summaries:1]{};
    unsigned lowest_=0;
    static unsigned firstBit(std::uint32_t word){
        unsigned bit=0;
        if(!(word&0xffffu)){word>>=16;bit+=16;}
        if(!(word&0xffu)){word>>=8;bit+=8;}
        if(!(word&0xfu)){word>>=4;bit+=4;}
        if(!(word&3u)){word>>=2;bit+=2;}
        return bit+(!(word&1u));
    }
public:
    LowestFreeSlotMap(){clear();}
    void clear(){
        lowest_=0;
        for(auto& word:summary_)word=0;
        for(unsigned w=0;w<Words;++w){
            free_[w]=0xffffffffu;
            if(w+1==Words&&(Capacity&31))free_[w]=(std::uint32_t(1)<<(Capacity&31))-1;
            summary_[w>>5]|=std::uint32_t(1)<<(w&31);
        }
    }
    // The owning link structure validates the slot before changing occupancy.
    void occupy(unsigned slot){
        const unsigned w=slot>>5;
        free_[w]&=~(std::uint32_t(1)<<(slot&31));
        if(!free_[w])summary_[w>>5]&=~(std::uint32_t(1)<<(w&31));
        if(slot==lowest_){
            const unsigned next=slot+1;
            if(next<Capacity&&(free_[next>>5]&(std::uint32_t(1)<<(next&31))))lowest_=next;
            else lowest_=search(next);
        }
    }
    void release(unsigned slot){
        const unsigned w=slot>>5;
        free_[w]|=std::uint32_t(1)<<(slot&31);
        summary_[w>>5]|=std::uint32_t(1)<<(w&31);
        if(slot<lowest_)lowest_=slot;
    }
    unsigned first(unsigned start=0)const{
        return start<=lowest_?lowest_:search(start);
    }
private:
    unsigned search(unsigned start)const{
        if(start>=Capacity)return Capacity;
        unsigned w=start>>5;
        const auto candidates=free_[w]&(0xffffffffu<<(start&31));
        if(candidates)return w*32+firstBit(candidates);
        if(++w==Words)return Capacity;
        unsigned s=w>>5;
        auto groups=summary_[s]&(0xffffffffu<<(w&31));
        for(;;){
            if(groups){w=s*32+firstBit(groups);return w*32+firstBit(free_[w]);}
            if(++s==Summaries)return Capacity;
            groups=summary_[s];
        }
    }
};
}
#endif
