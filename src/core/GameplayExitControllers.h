#ifndef DOWNWELL_GAMEPLAY_EXIT_CONTROLLERS_H
#define DOWNWELL_GAMEPLAY_EXIT_CONTROLLERS_H
#include "InstanceLifecycle.h"
namespace downwell {
// Identity-bearing exit trigger (306) and roomEndSetter (74). Their gameplay
// state remains in LevelExit/MainCameraMotion. The script variable `active`
// on the trigger is NOT runner activation: both records stay registered.
class GameplayExitControllers {
public:
    static constexpr unsigned capacity=128;
    struct Record {int x=0,y=0,objectId=306;unsigned lifetimeId=0;bool active=true;};
    using Binding=RecordLifecycleBinding<Record>;
    bool bind(Binding b){if(count_||!good_)return false;binding_=b;return true;}
    Binding binding()const{return binding_;}
    unsigned birthSequence()const{return next_;}
    bool restoreEmptyBirthSequence(unsigned n){if(count_||!good_||!n||n<next_)return false;next_=n;return true;}
    InstanceHandle handle(unsigned i)const{return i>=retired_&&i<count_?InstanceHandle{i,records_[i].lifetimeId}:InstanceHandle{};}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot>=retired_&&h.slot<count_&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    bool create(int object,int x,int y){
        if(!good_||count_==capacity||!next_||(object!=306&&object!=74))return good_=false;
        const auto slot=count_++;auto& record=records_[slot];record={x,y,object,next_++,true};
        return good_=binding_.birth(handle(slot),record);
    }
    bool clear(){
        if(!good_)return false;
        while(retired_<count_){
            if(!binding_.retire(handle(retired_),records_[retired_]))return good_=false;
            ++retired_;
        }
        count_=retired_=0;return true;
    }
private:
    Record records_[capacity]{};
    unsigned count_=0,retired_=0,next_=1;bool good_=true;Binding binding_{};
};
}
#endif
