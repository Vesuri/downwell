#ifndef DOWNWELL_CORE_CORPSE_EXPIRY_EFFECTS_H
#define DOWNWELL_CORE_CORPSE_EXPIRY_EFFECTS_H
#include "CorpseExpiryEffect.h"
namespace downwell {
// Owner-local lifetime keys. Global scheduling captures the cutoff before any
// family's animation runs. Hooks must not mutate this owner; none are retained.
template<unsigned Capacity> class CorpseExpiryEffects {
    static_assert(Capacity>0,"nonempty expiry effect capacity required");
public:
    using Handle=std::uint32_t;
    struct Phase{Handle newest;};
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    Phase beginPhase()const{return {next_?next_-1:0xffffffffu};}
    CorpseExpiryEffect* get(Handle h){
        if(good_&&h)for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle==h&&slot.effect.valid&&!slot.effect.destroyRequested)return &slot.effect;
        }
        return nullptr;
    }
    template<class Hooks> Handle create(MotionBinary64 x,MotionBinary64 y,Hooks& hooks){
        if(!good_)return 0;
        if(!next_||count_==Capacity){good_=false;return 0;}
        unsigned index=0;while(index<Capacity&&slots_[index].handle)++index;
        if(index==Capacity){good_=false;return 0;}
        auto& slot=slots_[index];slot.effect={};
        if(!slot.effect.x.assign(x)||!slot.effect.y.assign(y)){good_=false;return 0;}
        // Constructor publication precedes the Create rate assignment.
        slot.effect.imageSpeed={0x3f800000u};slot.handle=next_++;
        for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
        order_[0]=index;++count_;
        if(!hooks.birth(slot.handle,static_cast<const CorpseExpiryEffect&>(slot.effect))||
           !slot.effect.create(x,y)){good_=false;return 0;}
        return slot.handle;
    }
    template<class Hooks> bool setSprite(Handle h,int sprite,unsigned frames,Hooks& hooks){
        auto* effect=get(h);
        if(!effect||!effect->setSprite(sprite,frames,[&](const auto& state){return hooks.sprite(h,state);})){good_=false;return false;}
        return true;
    }
    template<class Hooks> bool advanceAnimation(Phase phase,Hooks& hooks){
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest)continue;
            if(!slot.effect.advanceImage()){good_=false;return false;}
            if(slot.effect.destroyRequested){
                if(!hooks.retire(slot.handle,static_cast<const CorpseExpiryEffect&>(slot.effect))){good_=false;return false;}
                slot.handle=0;
            }
        }
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle)order_[kept++]=order_[i];
        count_=kept;return true;
    }
    template<class Hooks> bool clear(Hooks& hooks){
        for(unsigned i=0;i<count_;++i){auto& slot=slots_[order_[i]];
            if(slot.handle){
                if(!hooks.retire(slot.handle,static_cast<const CorpseExpiryEffect&>(slot.effect))){good_=false;return false;}
                slot.handle=0;
            }
        }
        count_=0;good_=true;return true; // Never reuse a lifetime key.
    }
    template<class Visit> bool visitNewest(Visit visit)const{
        if(!good_)return false;
        for(unsigned i=0;i<count_;++i){const auto& slot=slots_[order_[i]];
            if(!slot.effect.valid)return false;
            if(!slot.effect.destroyRequested)visit(slot.handle,slot.effect);
        }
        return true;
    }
private:
    struct Slot{Handle handle=0;CorpseExpiryEffect effect;};
    Slot slots_[Capacity];unsigned order_[Capacity]{},count_=0;
    Handle next_=1;bool good_=true;
};
}
#endif
