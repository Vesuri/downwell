#ifndef DOWNWELL_CORE_CALLBACK_FAILURE_SIGNAL_H
#define DOWNWELL_CORE_CALLBACK_FAILURE_SIGNAL_H
namespace downwell {
// bool& event status is borrowed only for the synchronous callback's extent.
// Observe those live borrows, not every implemented family on every query.
class CallbackFailureScopes {
    struct Node {const bool* value;const Node* previous;};
    const Node* head_=nullptr;
    friend class CallbackFailureSignal;
public:
    bool valid()const{
        for(auto* node=head_;node;node=node->previous)if(!*node->value)return false;
        return true;
    }
};
class CallbackFailureSignal {
    bool value_=true;
    bool* owner_=nullptr;
    CallbackFailureScopes* scopes_=nullptr;
    void notify(){if(!value_&&owner_)*owner_=false;}
public:
    CallbackFailureSignal()=default;
    CallbackFailureSignal(bool value):value_(value){}
    CallbackFailureSignal(const CallbackFailureSignal&)=delete;
    CallbackFailureSignal& operator=(const CallbackFailureSignal&)=delete;
    operator bool()const{return value_;}
    bool operator=(bool value){value_=value;notify();return value;}
    void observe(bool& owner,CallbackFailureScopes& scopes){
        if(owner_&&(owner_!=&owner||scopes_!=&scopes)){
            *owner_=false;owner=false;return;
        }
        owner_=&owner;scopes_=&scopes;notify();
    }
    class Borrow {
        CallbackFailureSignal& signal_;
        CallbackFailureScopes::Node node_;
    public:
        explicit Borrow(CallbackFailureSignal& signal):signal_(signal),
            node_{&signal.value_,signal.scopes_?signal.scopes_->head_:nullptr}{
            if(signal.scopes_)signal.scopes_->head_=&node_;
        }
        Borrow(const Borrow&)=delete;
        Borrow& operator=(const Borrow&)=delete;
        ~Borrow(){
            if(signal_.scopes_)signal_.scopes_->head_=node_.previous;
            signal_.notify();
        }
        bool& value(){return signal_.value_;}
    };
    Borrow borrow(){return Borrow(*this);}
};
}
#endif
