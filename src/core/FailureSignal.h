#ifndef DOWNWELL_CORE_FAILURE_SIGNAL_H
#define DOWNWELL_CORE_FAILURE_SIGNAL_H
namespace downwell {
// Preserve a component's own status while notifying a borrowed enclosing
// lifetime on failure. Recovery of a component never resets the owner's
// terminal failure. The observer must outlive this component. Copies retain
// status, not another owner's observer; assignment keeps the destination's.
template<bool Healthy=true> class FailureSignal {
    bool value_=Healthy;
    bool* owner_=nullptr;
public:
    FailureSignal()=default;
    FailureSignal(bool value):value_(value){}
    FailureSignal(const FailureSignal& other):value_(other.value_){}
    bool operator=(bool value){
        value_=value;if(value!=Healthy&&owner_)*owner_=false;return value;
    }
    FailureSignal& operator=(const FailureSignal& other){*this=other.value_;return *this;}
    operator bool()const{return value_;}
    void observe(bool& owner){
        // Never silently detach a live enclosing owner.
        if(owner_&&owner_!=&owner){*owner_=false;owner=false;return;}
        owner_=&owner;if(value_!=Healthy)owner=false;
    }
};
}
#endif
