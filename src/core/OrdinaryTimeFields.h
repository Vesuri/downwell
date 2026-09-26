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
    unsigned size()const{return count_;}
    void clear(){count_=0;}
    const OrdinaryTimeField* at(unsigned index)const{return index<count_?&fields_[index]:nullptr;}
    bool adoptCreate(int x,int y,std::uint32_t word){
        if(count_==Capacity)return false;
        fields_[count_].adoptCreate(x,y,word);++count_;return true;
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
};
}
#endif
