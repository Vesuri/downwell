#ifndef DOWNWELL_CORE_BLOOD_EFFECTS_H
#define DOWNWELL_CORE_BLOOD_EFFECTS_H
#include "BloodEffect.h"
namespace downwell {
// Fixed storage for scrBloodfx's object-220 specialization. Capture cutoffs at
// global phase entry, not when this family begins its local traversal.
template<unsigned Capacity> class BloodEffects {
    static_assert(Capacity>0,"Blood effect storage must be nonempty");
public:
    using Handle=std::uint32_t;
    struct Phase {Handle newest;};
    struct Slot {Handle handle=0;BloodEffect effect;};
    static constexpr unsigned capacity=Capacity;
    unsigned size()const{return count_;}
    Phase beginPhase()const{return {next_?next_-1u:0xffffffffu};}
    BloodEffect* get(Handle handle){
        if(handle)for(unsigned i=0;i<count_;++i)
            if(slots_[order_[i]].handle==handle)return &slots_[order_[i]].effect;
        return nullptr;
    }
    void clear(){
        for(unsigned i=0;i<count_;++i)slots_[order_[i]].handle=0;
        count_=0; // Preserve handle identity across room reset and stale phases.
    }
    Handle create(MotionBinary64 x,MotionBinary64 y,MotionBinary64 angle){
        if(!next_ || count_==Capacity)return 0;
        for(unsigned index=0;index<Capacity;++index)if(!slots_[index].handle){
            if(!slots_[index].effect.create(x,y,angle))return 0;
            for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
            order_[0]=index;++count_;
            return slots_[index].handle=next_++;
        }
        return 0;
    }
    template<class Random> bool burst(MotionBinary64 x,MotionBinary64 y,Random& random){
        return bloodBurst(x,y,random,[&](auto px,auto py,auto angle){return create(px,py,angle)!=0;});
    }
    bool advanceAnimation(Phase phase){
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[order_[i]];
            if(slot.handle<=phase.newest && !slot.effect.destroyRequested &&
               !slot.effect.advanceImage())return false;
        }
        retireRequested();return true;
    }
    // allowed(handle,effect) evaluates TimeStopBound per instance at Step.
    // It must not mutate this owner. Blood has no smoke-wall response.
    template<class Allowed> bool step(Phase phase,Allowed allowed){
        return stepChecked(phase,[&](Handle handle,const BloodEffect& effect,bool& result){
            result=allowed(handle,effect);return true;
        });
    }
    // Checked world queries distinguish unsupported geometry from a real
    // time-stop result. On failure, do not step the affected instance.
    template<class Query> bool stepChecked(Phase phase,Query query){
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest || slot.effect.destroyRequested)continue;
            bool allowed=false;
            if(!query(slot.handle,static_cast<const BloodEffect&>(slot.effect),allowed) ||
               !slot.effect.step(allowed))return false;
        }
        return true;
    }
    void retireRequested(){
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[order_[i]];
            if(slot.effect.destroyRequested)slot.handle=0;
            else order_[kept++]=order_[i];
        }
        count_=kept;
    }
    // Default sprite Draw, not drawFx/controller composition. Callers must
    // preserve image_angle and world depth when merging with other families.
    template<class Draw> void visitNewest(Draw draw)const{
        for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];draw(slot.handle,slot.effect);}
    }
private:
    Slot slots_[Capacity];
    unsigned order_[Capacity]{},count_=0;
    Handle next_=1;
};
}
#endif
