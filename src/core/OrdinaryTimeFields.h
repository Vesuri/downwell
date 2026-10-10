#ifndef DOWNWELL_CORE_ORDINARY_TIME_FIELDS_H
#define DOWNWELL_CORE_ORDINARY_TIME_FIELDS_H
#include "OrdinaryTimeField.h"
#include "InstanceLifecycle.h"
namespace downwell {
// Object 82 has no Step/alarm retirement: these instances live until room
// teardown. Storage order is creation order, not a promise about Draw/query
// traversal. The scheduler must select the source traversal explicitly.
template<unsigned Capacity> class OrdinaryTimeFields {
    static_assert(Capacity>0,"Time-field storage must be nonempty");
public:
    // Optional native registry bridge. Records are visible during callbacks;
    // no callback may mutate this store. Birth precedes radius RNG, and clear
    // retires in creation order before invalidating each key. Failure is final.
    using Binding=RecordLifecycleBinding<OrdinaryTimeField>;
    bool bindLifecycle(Binding binding){if(count_||!good_)return false;binding_=binding;return true;}
    Binding lifecycle()const{return binding_;}
    bool valid()const{return good_;}
    // Port-owned opaque keys, not original numeric instance IDs. Object 82
    // has no individual retirement, so each room's births form one interval.
    struct BirthSequence {std::uint32_t next=1;};
    unsigned size()const{return count_;}
    bool clear(){
        if(!good_)return false;
        for(;retired_<count_;++retired_)
            if(!binding_.retire({retired_,firstIdentity_+retired_},fields_[retired_])){good_=false;return false;}
        count_=retired_=0;firstIdentity_=nextIdentity_;return true;
    }
    BirthSequence birthSequence()const{return {nextIdentity_};}
    // Only for reconstructing an empty owner (Retry); callers preserve the
    // prior cursor, including exhausted zero. Never use this to rewind births.
    bool restoreEmptyBirthSequence(BirthSequence sequence){
        if(count_||!good_)return false;
        if(!nextIdentity_ && sequence.next)return false;
        if(sequence.next && sequence.next<nextIdentity_)return false;
        firstIdentity_=nextIdentity_=sequence.next;return true;
    }
    std::uint32_t identityAt(unsigned index)const{
        return index>=retired_&&index<count_?firstIdentity_+index:0;
    }
    const OrdinaryTimeField* find(std::uint32_t identity)const{
        if(!identity || identity<firstIdentity_)return nullptr;
        return at(identity-firstIdentity_);
    }
    const OrdinaryTimeField* at(unsigned index)const{return index>=retired_&&index<count_?&fields_[index]:nullptr;}
    bool adoptCreate(int x,int y,std::uint32_t word,std::uint32_t source=0){
        if(source>0x7fffffffu){good_=false;return false;}
        if(!birth(x,y,source))return false;
        fields_[count_-1].adoptCreate(x,y,word);return true;
    }
    // Runtime instance_create executes object 82 Create synchronously, unlike
    // adoption of a builder birth whose random sample was already consumed.
    // Reject port storage/key exhaustion before consuming the Create sample.
    bool create(int x,int y,GameMakerRandom& random){
        if(!birth(x,y))return false;
        fields_[count_-1].adoptCreate(x,y,random.nextU32());return true;
    }
    bool createWithSourceId(std::uint32_t source,int x,int y,GameMakerRandom& random){
        if(!source||source>0x7fffffffu){good_=false;return false;}
        if(!birth(x,y,source))return false;
        fields_[count_-1].adoptCreate(x,y,random.nextU32());return true;
    }
    // Equal-depth dynamic instances are inserted before existing equals by
    // runner 0x487610. The goal's boundary is the ordinary count at its birth;
    // later ordinary births therefore precede it. No coordinate sorting.
    // The goal callback must not mutate this owner or create more fields.
    template<class GoalDraw> bool advanceDraw(bool stopped,GameMakerRandom& random,
        bool goalPresent,unsigned goalBirthBoundary,GoalDraw goalDraw){
        if(!good_||(goalPresent && goalBirthBoundary>count_))return false;
        for(unsigned i=count_;i>0;--i){
            if(goalPresent && i==goalBirthBoundary)goalDraw();
            fields_[i-1].advanceDraw(stopped,random);
        }
        if(goalPresent && goalBirthBoundary==0)goalDraw();
        return true;
    }
private:
    bool birth(int x,int y,std::uint32_t source=0){
        if(!good_||count_==Capacity||!nextIdentity_)return false;
        for(unsigned i=retired_;source&&i<count_;++i)
            if(fields_[i].sourceId==source){good_=false;return false;}
        const unsigned index=count_++;const auto identity=nextIdentity_++;
        fields_[index]={};fields_[index].x=x;fields_[index].y=y;
        fields_[index].sourceId=source;
        if(!binding_.birth({index,identity},fields_[index])){good_=false;return false;}
        return true;
    }
    Binding binding_{};bool good_=true;unsigned retired_=0;
    OrdinaryTimeField fields_[Capacity];unsigned count_=0;
    std::uint32_t firstIdentity_=1,nextIdentity_=1;
};
}
#endif
