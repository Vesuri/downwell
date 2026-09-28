#ifndef DOWNWELL_CORE_GAMEPLAY_ENTRY_DITHERS_H
#define DOWNWELL_CORE_GAMEPLAY_ENTRY_DITHERS_H
#include "EntryDither.h"
#include "InstanceLifecycle.h"
namespace downwell {
// Native typed storage for objEnterDither. Numeric IDs come from the room
// allocator, never from this store's generation sequence. No presentation timer.
class GameplayEntryDithers {
public:
    static constexpr unsigned capacity=2;
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        EntryDither effect;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayEntryDithers(Binding binding={}):binding_(binding){}
    GameplayEntryDithers(const GameplayEntryDithers&)=delete;
    GameplayEntryDithers& operator=(const GameplayEntryDithers&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<capacity&&records_[key.slot].lifetimeId==key.generation?
            &records_[key.slot]:nullptr;
    }
    InstanceHandle handle(unsigned slot)const{
        return slot<capacity?InstanceHandle{slot,records_[slot].lifetimeId}:InstanceHandle{};
    }
    template<class CameraX,class ViewY>
    bool create(unsigned sourceId,CameraX cameraX,ViewY viewY,InstanceHandle& result){
        if(!good_)return false;
        if(!sourceId||sourceId>0x7fffffffu||!next_)return fail();
        unsigned slot=capacity;
        for(unsigned i=0;i<capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==sourceId)return fail();
            if(!records_[i].lifetimeId&&slot==capacity)slot=i;
        }
        if(slot==capacity)return fail();
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=sourceId;++count_;
        const auto key=handle(slot);result=key;
        if(!binding_.birth(key,r))return fail();
        return r.effect.create(cameraX,viewY,[&](auto write,const auto& effect){
            return publish(key,r,write,effect);
        })||fail();
    }
    template<class CameraX> bool step(InstanceHandle key,CameraX cameraX){
        auto* r=resolve(key);if(!r)return fail();
        if(!r->active||r->destroyRequested)return true;
        r->effect.x=r->x;r->effect.y=r->y;
        return r->effect.step(cameraX,[&](auto write,const auto& effect){
            return publish(key,*r,write,effect);
        })||fail();
    }
    // Invoked once by the mixed-family alarm traversal for alarm zero.
    bool advanceAlarm(InstanceHandle key,unsigned index){
        auto* r=resolve(key);if(!r||index!=0)return fail();
        if(!r->active||r->destroyRequested)return true;
        if(!r->effect.advanceAlarm())return fail();
        if(r->effect.destroyRequested){
            if(!binding_.markDestroyed(key,*r))return fail();
            r->destroyRequested=true;
        }
        return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);
        if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);if(!r||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(!good_)return false;
        for(unsigned i=0;i<capacity;++i)if(records_[i].lifetimeId&&!retire(handle(i)))return false;
        return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool publish(InstanceHandle key,Record& r,EntryDither::Write write,const EntryDither& effect){
        if(write==EntryDither::Write::alarm)return true; // effect owns this alarm.
        const auto x=write==EntryDither::Write::x?effect.x:r.x;
        const auto y=write==EntryDither::Write::y?effect.y:r.y;
        if(!binding_.publishedPosition(key,r,x,y))return fail();
        r.x=x;r.y=y;return true;
    }
    Record records_[capacity]{};
    Binding binding_;unsigned next_=1,count_=0;bool good_=true;
};
}
#endif
