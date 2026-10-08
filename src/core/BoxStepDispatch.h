#ifndef DOWNWELL_CORE_BOX_STEP_DISPATCH_H
#define DOWNWELL_CORE_BOX_STEP_DISPATCH_H
#include "BoxDestructionScript.h"
#include "InstanceHandle.h"
namespace downwell {
// Borrow an event adapter for one world tick. No callback survives the scope;
// the reader resolves the caller generation again after every child effect.
class BoxStepDispatch {
public:
    bool bound()const{return dispatch_!=nullptr;}
    bool step(InstanceHandle key){return dispatch_&&dispatch_(context_,key);}
    template<class Read,class Operations,class Tick>
    bool run(bool& good,Read read,Operations operations,Tick tick){
        if(!good)return false;
        if(bound())return good=false;
        auto execute=[&](InstanceHandle key){
            decltype(auto) ops=operations(key);
            return BoxDestructionScript::runRetained(
                [&](BoxDestructionScript::Caller& caller){return read(key,caller);},ops,good);
        };
        context_=&execute;
        dispatch_=[](void* p,InstanceHandle key){return (*static_cast<decltype(execute)*>(p))(key);};
        const bool result=tick();
        dispatch_=nullptr;context_=nullptr;
        return good=good&&result;
    }
private:
    void* context_=nullptr;
    bool (*dispatch_)(void*,InstanceHandle)=nullptr;
};
}
#endif
