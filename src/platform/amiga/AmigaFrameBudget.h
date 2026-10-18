#ifndef DOWNWELL_AMIGA_FRAME_BUDGET_H
#define DOWNWELL_AMIGA_FRAME_BUDGET_H
#include <cstdint>

// PAL beam-clock work budget. Required publication waiting is excluded, but
// speculative preparation itself is ordinary measured frame work, not idle
// work hidden in that wait. An inconsistent/overflowed sample declines work.
class AmigaFrameBudget {
public:
    struct Stamp { std::uint16_t field; std::uint32_t beam; };
    static constexpr std::uint32_t fieldTicks=313u*256u;
    static constexpr std::uint32_t invalid=0xffffffffu;
private:
    Stamp start_;
    std::uint32_t wait_=0;
    static std::uint32_t elapsed(Stamp from,Stamp to){
        const unsigned fields=std::uint16_t(to.field-from.field);
        if(fields>1000||from.beam>=fieldTicks||to.beam>=fieldTicks)return invalid;
        const std::uint32_t total=fields*fieldTicks+to.beam;
        return total>=from.beam?total-from.beam:invalid;
    }
public:
    explicit AmigaFrameBudget(Stamp start):start_(start){}
    void excludeWait(Stamp from,Stamp to){
        const auto ticks=elapsed(from,to);
        if(ticks==invalid||wait_==invalid||ticks>invalid-wait_)wait_=invalid;
        else wait_+=ticks;
    }
    std::uint32_t work(Stamp now)const{
        const auto ticks=elapsed(start_,now);
        return ticks==invalid||wait_==invalid||ticks<wait_?invalid:ticks-wait_;
    }
    // Experimental admission only, not a worst-case completion guarantee:
    // scalar fallback pieces measure 3.3 ms and can overrun this reserve.
    // Keep the candidate off by default until whole-fall timing gates pass.
    bool canPrepare(Stamp now)const{return work(now)<fieldTicks*9/10;}
};
#endif
