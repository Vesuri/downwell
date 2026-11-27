#ifndef DOWNWELL_CORE_GAMEPLAY_GOAL_FIELDS_H
#define DOWNWELL_CORE_GAMEPLAY_GOAL_FIELDS_H
#include "InstanceLifecycle.h"
#include "TimeFieldWind.h"
namespace downwell {
// Native collision records for goalTimeField (255) and startTimeField (371). Presentation/radius RNG
// remains in RunSession. Keep every birth, including overlapping exit fixtures.
// Internal lifetime IDs are not Windows instance IDs.
class GameplayGoalFields {
public:
    static constexpr unsigned capacity=64;
    struct Record {
        int x=0,y=0,scaleX=1,scaleY=1,objectId=255;
        unsigned lifetimeId=0;
        unsigned sourceId=0; // Authored/source identity, never a pool handle.
        std::int32_t windSound=0;
        SoundPlayState sound{};
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    bool bind(Binding binding){if(count_||!good_)return false;binding_=binding;return true;}
    Binding binding()const{return binding_;}
    unsigned birthSequence()const{return next_;}
    bool restoreEmptyBirthSequence(unsigned next){if(count_||!good_||!next||next<next_)return false;next_=next;return true;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot>=retired_&&h.slot<count_&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    InstanceHandle handle(unsigned index)const{
        return index>=retired_&&index<count_?InstanceHandle{index,records_[index].lifetimeId}:InstanceHandle{};
    }
    template<class Random,class Audio> bool contactWind(InstanceHandle h,TimeFieldWind& wind,
        Random& random,int& channel,AudioLevel volume,Audio& audio){
        auto* record=const_cast<Record*>(find(h));
        if(!good_||!record)return good_=false;
        return good_=wind.contact(random,record->windSound,record->sound,channel,volume,audio);
    }
    bool create(int x,int y,int object=255,InstanceHandle* created=nullptr,unsigned sourceId=0){
        if(!good_||count_==capacity||!next_||(object!=255&&object!=371)||sourceId>0x7fffffffu)return good_=false;
        if(sourceId)for(unsigned i=retired_;i<count_;++i)
            if(records_[i].lifetimeId&&records_[i].sourceId==sourceId)return good_=false;
        const unsigned slot=count_++;
        auto& r=records_[slot];r={};r.x=x;r.y=y;r.objectId=object;r.lifetimeId=next_++;
        r.sourceId=sourceId;
        const InstanceHandle h{slot,r.lifetimeId};
        if(!binding_.birth(h,r))return good_=false;
        if(!binding_.integerTransform(h,r,RunnerTransformField::scaleX,2,{0x3f800000u},0xffffffu))return good_=false;
        r.scaleX=2;
        if(!binding_.integerTransform(h,r,RunnerTransformField::scaleY,2,{0x3f800000u},0xffffffu))return good_=false;
        r.scaleY=2;if(created)*created=h;return true;
    }
    bool requestDestroy(InstanceHandle h){
        if(!good_||!find(h))return good_=false;
        auto& r=records_[h.slot];
        if(r.destroyRequested)return true;
        if(binding_.hasOwner()&&!binding_.markDestroyed(h,r))return good_=false;
        r.destroyRequested=true;return true;
    }
    bool retireMarked(InstanceHandle h){
        if(!good_||!find(h)||!records_[h.slot].destroyRequested)return good_=false;
        if(!binding_.retire(h,records_[h.slot]))return good_=false;
        records_[h.slot].lifetimeId=0;
        while(retired_<count_&&!records_[retired_].lifetimeId)++retired_;
        return true;
    }
    bool cleanupMarked(){
        if(!good_)return false;
        for(unsigned i=retired_;i<count_;++i)
            if(records_[i].lifetimeId&&records_[i].destroyRequested&&!retireMarked(handle(i)))return false;
        return true;
    }
    bool clear(){
        if(!good_)return false;
        while(retired_<count_){
            const auto h=handle(retired_);
            if(h.generation&&!binding_.retire(h,records_[retired_]))return good_=false;
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
