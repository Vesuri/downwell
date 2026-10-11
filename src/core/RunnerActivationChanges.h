#ifndef DOWNWELL_RUNNER_ACTIVATION_CHANGES_H
#define DOWNWELL_RUNNER_ACTIVATION_CHANGES_H
#include <cstdint>
namespace downwell {
// Activation requests change flags immediately, but publish room membership
// at a phase boundary. This is a set, not a request-order dispatch queue.
template<unsigned Capacity> class RunnerActivationChanges {
    std::uint32_t words_[(Capacity+31)/32]{};
public:
    void mark(unsigned slot){if(slot<Capacity)words_[slot>>5]|=std::uint32_t(1)<<(slot&31);}
    void clear(){for(auto& word:words_)word=0;}
    unsigned next(unsigned first,unsigned end)const{
        while(first<end){
            const unsigned limit=(first|31u)+1<end?(first|31u)+1:end;
            const auto word=words_[first>>5] & (~std::uint32_t(0)<<(first&31));
            if(word)for(;first<limit;++first)if(word&(std::uint32_t(1)<<(first&31)))return first;
            first=limit;
        }
        return end;
    }
};
}
#endif
