#ifndef DOWNWELL_CORE_GAMEPLAY_GOAL_FIELDS_H
#define DOWNWELL_CORE_GAMEPLAY_GOAL_FIELDS_H
#include "InstanceLifecycle.h"
namespace downwell {
// Native collision records for goalTimeField (255). Presentation/radius RNG
// remains in RunSession. Keep every birth, including overlapping exit fixtures.
// Internal lifetime IDs are not Windows instance IDs.
class GameplayGoalFields {
public:
    static constexpr unsigned capacity=64;
    struct Record {
        int x=0,y=0,scaleX=1,scaleY=1;
        unsigned lifetimeId=0;
        bool active=true;
    };
    using Binding=RecordLifecycleBinding<Record>;
    bool bind(Binding binding){if(count_||!good_)return false;binding_=binding;return true;}
    Binding binding()const{return binding_;}
    unsigned birthSequence()const{return next_;}
    bool restoreEmptyBirthSequence(unsigned next){if(count_||!good_||!next||next<next_)return false;next_=next;return true;}
    const Record* find(InstanceHandle h)const{
        return h.slot>=retired_&&h.slot<count_&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle handle(unsigned index)const{
        return index>=retired_&&index<count_?InstanceHandle{index,records_[index].lifetimeId}:InstanceHandle{};
    }
    bool create(int x,int y){
        if(!good_||count_==capacity||!next_)return good_=false;
        const unsigned slot=count_++;
        auto& r=records_[slot];r={};r.x=x;r.y=y;r.lifetimeId=next_++;
        const InstanceHandle h{slot,r.lifetimeId};
        if(!binding_.birth(h,r))return good_=false;
        if(!binding_.integerTransform(h,r,RunnerTransformField::scaleX,2,{0x3f800000u},0xffffffu))return good_=false;
        r.scaleX=2;
        if(!binding_.integerTransform(h,r,RunnerTransformField::scaleY,2,{0x3f800000u},0xffffffu))return good_=false;
        r.scaleY=2;return true;
    }
    bool clear(){
        if(!good_)return false;
        while(retired_<count_){
            const auto h=handle(retired_);
            if(!binding_.retire(h,records_[retired_]))return good_=false;
            ++retired_;
        }
        retired_=count_=0;return true;
    }
private:
    Record records_[capacity]{};
    unsigned count_=0,retired_=0,next_=1;
    bool good_=true;
    Binding binding_{};
};
}
#endif
