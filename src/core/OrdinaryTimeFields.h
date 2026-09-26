#ifndef DOWNWELL_CORE_ORDINARY_TIME_FIELDS_H
#define DOWNWELL_CORE_ORDINARY_TIME_FIELDS_H
#include "OrdinaryTimeField.h"
namespace downwell {
// Object 82 has no Step/alarm retirement: these instances live until room
// teardown. Storage order is creation order, not a promise about Draw/query
// traversal. The scheduler must select the source traversal explicitly.
template<unsigned Capacity> class OrdinaryTimeFields {
    static_assert(Capacity>0,"Time-field storage must be nonempty");
public:
    // Port-owned opaque keys, not original numeric instance IDs. Object 82
    // has no individual retirement, so each room's births form one interval.
    struct BirthSequence {std::uint32_t next=1;};
    unsigned size()const{return count_;}
    void clear(){count_=0;firstIdentity_=nextIdentity_;}
    BirthSequence birthSequence()const{return {nextIdentity_};}
    // Only for reconstructing an empty owner (Retry); callers preserve the
    // prior cursor, including exhausted zero. Never use this to rewind births.
    bool restoreEmptyBirthSequence(BirthSequence sequence){
        if(count_)return false;
        if(!nextIdentity_ && sequence.next)return false;
        if(sequence.next && sequence.next<nextIdentity_)return false;
        firstIdentity_=nextIdentity_=sequence.next;return true;
    }
    std::uint32_t identityAt(unsigned index)const{
        return index<count_?firstIdentity_+index:0;
    }
    const OrdinaryTimeField* find(std::uint32_t identity)const{
        if(!identity || identity<firstIdentity_)return nullptr;
        return at(identity-firstIdentity_);
    }
    const OrdinaryTimeField* at(unsigned index)const{return index<count_?&fields_[index]:nullptr;}
    bool adoptCreate(int x,int y,std::uint32_t word){
        if(count_==Capacity || !nextIdentity_)return false;
        fields_[count_].adoptCreate(x,y,word);++count_;++nextIdentity_;return true;
    }
    // Equal-depth dynamic instances are inserted before existing equals by
    // runner 0x487610. The goal's boundary is the ordinary count at its birth;
    // later ordinary births therefore precede it. No coordinate sorting.
    // The goal callback must not mutate this owner or create more fields.
    template<class GoalDraw> bool advanceDraw(bool stopped,GameMakerRandom& random,
        bool goalPresent,unsigned goalBirthBoundary,GoalDraw goalDraw){
        if(goalPresent && goalBirthBoundary>count_)return false;
        for(unsigned i=count_;i>0;--i){
            if(goalPresent && i==goalBirthBoundary)goalDraw();
            fields_[i-1].advanceDraw(stopped,random);
        }
        if(goalPresent && goalBirthBoundary==0)goalDraw();
        return true;
    }
private:
    OrdinaryTimeField fields_[Capacity];unsigned count_=0;
    std::uint32_t firstIdentity_=1,nextIdentity_=1;
};
}
#endif
