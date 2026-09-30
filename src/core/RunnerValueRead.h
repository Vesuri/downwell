#ifndef DOWNWELL_RUNNER_VALUE_READ_H
#define DOWNWELL_RUNNER_VALUE_READ_H
#include "PlacementNew.h"
namespace downwell {
// Actors borrow stable full records; compact terrain owns an expanded value.
// Retain this handle while using a reference obtained from it. Copies retain
// their own value, not a pointer into the first handle's temporary storage.
template<class T> class RunnerValueRead {
    union Value {T value;Value(){}~Value(){}} value_;
    const T* borrowed_=nullptr;
    bool owned_=false;
public:
    explicit RunnerValueRead(const T* p=nullptr):borrowed_(p){}
    explicit RunnerValueRead(const T& p):owned_(true){new(&value_.value)T(p);}
    RunnerValueRead(const RunnerValueRead& r):borrowed_(r.borrowed_),owned_(r.owned_){
        if(owned_)new(&value_.value)T(r.value_.value);
    }
    RunnerValueRead& operator=(const RunnerValueRead& r){
        if(this==&r)return *this;
        if(owned_)value_.value.~T();
        borrowed_=r.borrowed_;owned_=r.owned_;
        if(owned_)new(&value_.value)T(r.value_.value);return *this;
    }
    ~RunnerValueRead(){if(owned_)value_.value.~T();}
    explicit operator bool()const{return owned_||borrowed_;}
    const T* operator->()const{return owned_?&value_.value:borrowed_;}
    const T& operator*()const{return *operator->();}
};
}
#endif
