#ifndef DOWNWELL_CORE_STATIONARY_RETIREMENT_WINDOW_H
#define DOWNWELL_CORE_STATIONARY_RETIREMENT_WINDOW_H
#include <cstdint>
namespace downwell {
// A proof that a captured set of immutable Y coordinates cannot execute
// scrOutofview's destruction branch. This is not an event result cache:
// membership/eligibility changes invalidate it, and a boundary crossing still
// runs the original ordered event walk. Bounds are inclusive (source LT/GT).
class StationaryRetirementWindow {
public:
    struct Proof {
        std::int32_t low=-2146828288, high=2135195647;
        std::uint32_t revision=0, cutoff=0;
        bool supported=true;
        void include(int y) {
            // Keep every operation defined on the 32-bit target. Unusual
            // coordinates retain the original path, never a narrowed proof.
            if(y < -521288 || y > 524127) { supported=false; return; }
            const std::int32_t first=(y-3000)*4096L, last=(y+160)*4096L;
            if(first>low)low=first;
            if(last<high)high=last;
        }
    };
    void invalidate() {
        valid_=false;
        if(revision_!=0xffffffffu)++revision_;
    }
    // Synchronous birth notification extends a previously complete set.
    // Including a newborn before its phase is conservative: it can prevent
    // skipping, but cannot cause its event to run early. Other terrain only
    // advances the shared lifetime ceiling; it adds no retirement work.
    void append(int y,bool selected,std::uint32_t cutoff) {
        if(revision_!=0xffffffffu)++revision_;
        if(!valid_)return;
        if(selected)proof_.include(y);
        proof_.cutoff=cutoff;proof_.revision=revision_;
        valid_=proof_.supported&&revision_!=0xffffffffu;
    }
    Proof begin(std::uint32_t cutoff)const {
        Proof p;p.revision=revision_;p.cutoff=cutoff;return p;
    }
    void complete(const Proof& proof) {
        proof_=proof;
        valid_=proof.supported&&proof.revision==revision_&&revision_!=0xffffffffu;
    }
    bool contains(long topQ12,std::uint32_t cutoff)const {
        return valid_&&proof_.cutoff==cutoff&&topQ12>=proof_.low&&topQ12<=proof_.high;
    }
private:
    Proof proof_;
    std::uint32_t revision_=0;
    bool valid_=false;
};
}
#endif
