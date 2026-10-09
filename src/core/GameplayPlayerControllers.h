#ifndef DOWNWELL_CORE_GAMEPLAY_PLAYER_CONTROLLERS_H
#define DOWNWELL_CORE_GAMEPLAY_PLAYER_CONTROLLERS_H
#include "InstanceLifecycle.h"
namespace downwell {
// Registered player-created depth meter and camera. Source IDs are supplied
// by the enclosing synchronous creation stream, never by this local pool.
// This owns lifetime/publication only; it does not execute either Create body.
class GameplayPlayerControllers {
public:
    static constexpr unsigned capacity=2;
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        int objectId=-1;
        RunnerImageValue x{},y{};
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayPlayerControllers(Binding binding={}):binding_(binding){}
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle handle(unsigned slot)const{return slot<capacity?InstanceHandle{slot,records_[slot].lifetimeId}:InstanceHandle{};}
    bool create(int object,unsigned source,RunnerImageValue x,RunnerImageValue y,InstanceHandle& result){
        result={};
        if(!good_||!next_||count_==capacity||(object!=64&&object!=66)||!source||source>0x7fffffffu)return fail();
        for(const auto& r:records_)if(r.lifetimeId&&(r.sourceId==source||r.objectId==object))return fail();
        unsigned slot=0;while(records_[slot].lifetimeId)++slot;
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=source;r.objectId=object;r.x=x;r.y=y;++count_;
        if(!binding_.birth(handle(slot),r))return fail();
        result=handle(slot);return true;
    }
    bool position(InstanceHandle key,RunnerImageValue x,RunnerImageValue y){
        auto* r=const_cast<Record*>(find(key));
        if(!good_||!r||!binding_.publishedPosition(key,*r,x,y))return fail();
        r->x=x;r->y=y;return true;
    }
    bool activation(InstanceHandle key,bool active){
        auto* r=const_cast<Record*>(find(key));
        if(!good_||!r||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool clear(){
        if(!good_)return false;
        for(unsigned slot=0;slot<capacity;++slot){auto& r=records_[slot];
            if(!r.lifetimeId)continue;
            if(!binding_.retire(handle(slot),r))return fail();
            r={};--count_;
        }
        return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record records_[capacity]{};Binding binding_{};unsigned count_=0,next_=1;bool good_=true;
};
}
#endif
