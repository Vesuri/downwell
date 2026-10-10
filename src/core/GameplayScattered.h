#ifndef DOWNWELL_CORE_GAMEPLAY_SCATTERED_H
#define DOWNWELL_CORE_GAMEPLAY_SCATTERED_H
#include "InstanceLifecycle.h"
#include "DeferredInstanceQueue.h"
#include "ScatteredEvents.h"
#include "CurrencyRewardFields.h"
#include "SoundPlayScript.h"
namespace downwell {
// Scattered16 (289), a shootable/stompable scenery actor, not objSpike_n.
// Builder adoption must not repeat its already-consumed Create RNG.
class GameplayScattered {
public:
    static constexpr unsigned capacity=64;
    struct Record:ScatteredScriptState {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{},imageIndex{},scaleX{0x3f800000u};
        int maskId=-1;
        CurrencyRewardFields reward{};
        SoundPlayState sound{};
        bool active=true,destroyRequested=false,initialized=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayScattered(Binding binding={}):binding_(binding){}
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    InstanceHandle handle(unsigned slot)const{return slot<capacity?InstanceHandle{slot,records_[slot].lifetimeId}:InstanceHandle{};}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    template<class Random,class Effects> bool step(InstanceHandle h,Random& random,Effects& effects){
        if(!good_||!find(h)||!records_[h.slot].initialized)return fail();
        return ScatteredEvents::step(records_[h.slot],random,effects)||fail();
    }
    template<class Random,class Effects> bool playerContact(InstanceHandle h,Random& random,Effects& effects){
        if(!good_||!find(h)||!records_[h.slot].initialized)return fail();
        return ScatteredEvents::playerContact(records_[h.slot],random,effects)||fail();
    }
    bool alarm0(InstanceHandle h){
        if(!good_||!find(h))return fail();
        ScatteredEvents::alarm0(records_[h.slot]);return true;
    }
    template<class F> bool dispatch(InstanceHandle h,F f){
        if(!good_||!find(h))return fail();
        return f(records_[h.slot])||fail();
    }
    bool sprite(InstanceHandle h,int value){
        if(!good_||!find(h)||!binding_.sprite(h,records_[h.slot],value))return fail();
        records_[h.slot].spriteId=value;return true;
    }
    bool markDestroyed(InstanceHandle h){
        if(!good_||!find(h))return fail();
        auto& r=records_[h.slot];if(r.destroyRequested)return true;
        if(!binding_.markDestroyed(h,r))return fail();
        r.destroyRequested=true;return true;
    }
    bool retireMarked(InstanceHandle h){
        if(!good_||!find(h)||!records_[h.slot].destroyRequested||
           !binding_.retire(h,records_[h.slot]))return fail();
        records_[h.slot]={};--count_;return true;
    }
    bool adopt(unsigned source,const DeferredCreateInstance& command){
        if(!good_||!next_||count_==capacity||!source||source>0x7fffffffu||command.objectId!=289||
            (command.integerScaleX!=1&&command.integerScaleX!=-1)||
            command.integerImageIndex<0||command.integerImageIndex>16)return fail();
        for(const auto& r:records_)if(r.lifetimeId&&r.sourceId==source)return fail();
        unsigned slot=0;while(records_[slot].lifetimeId)++slot;
        auto& r=records_[slot];r={};r.hitEffect=0;r.lifetimeId=next_++;r.sourceId=source;r.active=command.active;++count_;
        if(!r.x.assignInteger(command.x)||!r.y.assignInteger(command.y)||!binding_.birth(handle(slot),r))return fail();
        // Inherited parentShootableThing Create precedes these setters.
        r.hitEffect=1;
        if(!binding_.imageIndex(handle(slot),r,{}))return fail();
        RunnerImageValue scale;if(!scale.assignInteger(command.integerScaleX)||
            !binding_.transform(handle(slot),r,RunnerTransformField::scaleX,scale,{0x3f800000u},0xffffffu))return fail();
        r.scaleX=scale;
        if(!binding_.sprite(handle(slot),r,550))return fail();
        if(!binding_.mask(handle(slot),r,550))return fail();
        r.maskId=550;r.scatterType=command.integerImageIndex;
        RunnerImageValue index;if(!index.assignInteger(r.scatterType)||!binding_.imageIndex(handle(slot),r,index))return fail();
        r.imageIndex=index;r.initialized=true;return true;
    }
    bool clear(){
        if(!good_)return false;
        for(unsigned slot=0;slot<capacity;++slot)if(records_[slot].lifetimeId){
            if(!binding_.retire(handle(slot),records_[slot]))return fail();
            records_[slot]={};--count_;
        }
        return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record records_[capacity]{};Binding binding_;unsigned next_=1,count_=0;bool good_=true;
};
}
#endif
