#ifndef DOWNWELL_RUNNER_ALARM_PROJECTION_H
#define DOWNWELL_RUNNER_ALARM_PROJECTION_H
#include <cstdint>
namespace downwell {
// Indices into an immutable captured phase, not live counter values. Callers
// must abandon selection after binding mutations and still check eligibility
// and counters immediately before each event. Merge fences keep external
// object groups at their original positions even in an otherwise empty column.
template<unsigned Capacity> class RunnerAlarmProjection {
    static constexpr unsigned words=(Capacity+31)/32;
    std::uint32_t columns_[12][words?words:1]{};
    unsigned count_=0;
public:
    bool reset(unsigned count){
        if(count>Capacity)return false;
        count_=count;
        for(unsigned alarm=0;alarm<12;++alarm)
            for(unsigned word=0;word<(count+31)/32;++word)columns_[alarm][word]=0;
        return true;
    }
    bool include(unsigned index,unsigned mask){
        if(index>=count_||mask&~0xfffu)return false;
        for(unsigned alarm=0;mask;++alarm,mask>>=1)
            if(mask&1)columns_[alarm][index/32]|=std::uint32_t(1)<<(index%32);
        return true;
    }
    unsigned next(unsigned alarm,unsigned start)const{
        if(alarm>=12||start>=count_)return count_;
        unsigned word=start/32;
        auto bits=columns_[alarm][word]&(0xffffffffu<<(start%32));
        while(!bits){
            if(++word>=(count_+31)/32)return count_;
            bits=columns_[alarm][word];
        }
        unsigned bit=0;while(!(bits&1)){bits>>=1;++bit;}
        return word*32+bit;
    }
};
}
#endif
