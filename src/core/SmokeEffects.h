#ifndef DOWNWELL_CORE_SMOKE_EFFECTS_H
#define DOWNWELL_CORE_SMOKE_EFFECTS_H
#include "SmokeEffect.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include "SkeletonEffectBursts.h"
#include "EnemyContact.h"
namespace downwell {
// Fixed storage with explicit global phase cutoffs. Birth completes Create
// synchronously; first Step's extra RNG draw is not consumed at birth. The world
// must capture each cutoff before running ANY instance in that global phase.
template<unsigned Capacity> class SmokeEffects {
    static_assert(Capacity>0,"Smoke storage must be nonempty");
public:
    using Handle=std::uint32_t;
    struct BirthSequence {Handle next=1;};
    BirthSequence birthSequence()const{return {next_};}
    bool restoreEmptyBirthSequence(BirthSequence sequence){
        if(count_ || (!next_ && sequence.next) || (sequence.next && sequence.next<next_))return false;
        next_=sequence.next;return true;
    }
    struct Phase {Handle newest;};
    struct Slot {Handle handle=0;SmokeEffect effect;};
    static constexpr unsigned capacity=Capacity;
    Phase beginPhase()const{return {next_?next_-1u:0xffffffffu};}
    unsigned size()const{return count_;}
    const Slot& slot(unsigned index)const{return slots_[index];}
    SmokeEffect* get(Handle handle){
        if(handle)for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle==handle)return &slots_[order_[i]].effect;
        return nullptr;
    }
    void clear(){
        for(unsigned i=0;i<count_;++i)slots_[order_[i]].handle=0;
        count_=0; // Do not reuse handles or make a stale phase match new births.
    }
    template<class Random> Handle create(MotionBinary64 x,MotionBinary64 y,Random& random){
        if(!next_ || count_==Capacity)return 0;
        for(unsigned index=0;index<Capacity;++index)if(!slots_[index].handle){
            if(!slots_[index].effect.create(x,y,random))return 0;
            for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
            order_[0]=index;++count_;
            return slots_[index].handle=next_++;
        }
        return 0;
    }
    template<class Random> bool skeletonBurst(MotionBinary64 x,MotionBinary64 y,Random& random){
        return skeletonSmokeBurst(x,y,random,[&](int object,MotionBinary64 px,MotionBinary64 py){
            return object==225 && create(px,py,random)!=0;
        });
    }
    template<class Random> bool boxBurst(MotionBinary64 x,MotionBinary64 y,Random& random){
        return boxSmokeBurst(x,y,random,[&](int object,MotionBinary64 px,MotionBinary64 py){
            return object==225 && create(px,py,random)!=0;
        });
    }
    bool advanceAnimation(Phase phase){
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[order_[i]];
            if(slot.handle<=phase.newest && !slot.effect.destroyRequested &&
               !slot.effect.advanceImage())return false;
        }
        retireRequested();return true;
    }
    template<class Random> bool step(Phase phase,Random& random){
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest || slot.effect.destroyRequested)continue;
            if(!slot.effect.step(random,[](MotionBinary64 speed,MotionBinary64 angle,
                                          MotionBinary64& x,MotionBinary64& y){
                return SmokeDirection::vector(speed,angle,x,y,smokeTrig);
            }))return false;
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
    // contacts(bounds, emit) enumerates each colliding parentWall instance,
    // returning success. It must not mutate this owner or collapse overlapping
    // walls to a boolean. Capture the phase at Collision entry, not Step entry.
    // Water's empty override needs no dispatch; it must not be reported as wall.
    template<class Contacts> bool collideWalls(Phase phase,Contacts contacts){
        for(unsigned i=0;i<count_;++i){
            auto& slot=slots_[order_[i]];
            auto& effect=slot.effect;
            if(slot.handle>phase.newest || effect.destroyRequested || effect.noCollision)continue;
            std::int32_t x,y;
            if(!effect.x.promoted().roundPixel(x) || !effect.y.promoted().roundPixel(y) ||
               x<(-2147483647-1)+2 || x>2147483647-2 ||
               y<(-2147483647-1)+1 || y>2147483647-2)return false;
            auto bounds=contactSpriteBounds(84);
            bounds.left+=x;bounds.right+=x;bounds.top+=y;bounds.bottom+=y;
            bool okay=true;
            if(!contacts(bounds,[&](){
                if(okay)okay=effect.collideWall();
                return okay;
            }) || !okay)return false;
        }
        return true;
    }
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
