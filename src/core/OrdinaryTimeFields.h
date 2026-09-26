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
private:
    OrdinaryTimeField fields_[Capacity];unsigned count_=0;
};
}
#endif
