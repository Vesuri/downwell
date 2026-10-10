#ifndef DOWNWELL_CORE_STORED_SHOP_CHILD_REGISTRATION_H
#define DOWNWELL_CORE_STORED_SHOP_CHILD_REGISTRATION_H
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "MultiplyBinary64.h"
#include <cstring>
namespace downwell {
// Synchronous Create ownership for Shop's ten children. This is deliberately
// separate from ShopFixtures (drawing) and ShopFeedback (compact gameplay).
// The enclosing room supplies the one numeric-ID stream and lifecycle owner.
// Step/Alarm/Draw admission remains the enclosing scheduler's responsibility.
template<unsigned WorldCapacity,unsigned Capacity> class StoredShopChildRegistration {
    static_assert(Capacity>0,"shop child registration needs storage");
public:
    enum class Material { unset,metal,rock };
    struct State {
        Material material=Material::unset;
        int wallHp=0,previousWallHp=0,keeper=0,storedItem=0,xsp=0;
        int opened=0,destroyed=0,flashing=0,thankful=0,goodbye=0,confused=0;
        unsigned whichUg=0;
        MotionBinary64 ysp{},decreaseBy{};
        const char* text=nullptr;
        int drawing=0;
        bool initialized=false;
    };
private:
    struct Entry {InstanceHandle key{};unsigned id=0;int object=-1;State state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack& objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool creating_=false;
    bool fail(){good_=false;return false;}
    Entry* entry(InstanceHandle h){
        for(auto& e:entries_)if(e.id&&e.key.slot==h.slot&&e.key.generation==h.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);
        return r&&r->instanceId==e.id&&r->objectId==e.object?r:nullptr;
    }
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
public:
    StoredShopChildRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack& objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredShopChildRegistration(const StoredShopChildRegistration&)=delete;
    StoredShopChildRegistration& operator=(const StoredShopChildRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid()const{return good_;}
    InstanceHandle handle(const InstanceRecord& r){
        const auto key=sourceHandle(r.instanceId);auto* e=entry(key);
        return e&&record(*e)==&r?key:InstanceHandle{};
    }
    InstanceHandle sourceHandle(unsigned id){
        for(auto& e:entries_)if(id&&e.id==id&&record(e))return e.key;
        return {};
    }
    const State* state(InstanceHandle h){auto* e=entry(h);return e&&record(*e)?&e->state:nullptr;}
    unsigned create(int object,int x,int y){
        if(!good_||creating_||(object!=278&&object!=279&&object!=98&&object!=280&&object!=212&&object!=283)){
            fail();return 0;
        }
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_.findObject(object,definition)||!nextId_||nextId_>0x7fffffffu||
           !px.assignInteger(x)||!py.assignInteger(y)||!px.roundPixel(ix)||!py.roundPixel(iy)){
            fail();return 0;
        }
        creating_=true;struct Reset {bool& flag;~Reset(){flag=false;}} reset{creating_};
        if(records_.create(nextId_,object,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success){
            fail();return 0;
        }
        free->id=nextId_++;free->object=object;free->key=records_.handleAt(records_.size()-1);
        auto* r=record(*free);if(!r){fail();return 0;}
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        // Publish default builtins before executing Create setters. PreCreate
        // for these resources/ancestors only delegates to event_inherited.
        if(!lifecycle_.birthAt(free->key,*r,px,py)||!good_){fail();return 0;}
        auto& s=free->state;
        switch(object){
        case 278:s.material=Material::metal;break;
        case 98:s.material=Material::rock;break;
        case 279:
            s.material=Material::metal;r->imageSpeed=0;
            s.wallHp=s.previousWallHp=1000000;break;
        case 280:{
            RunnerImageValue speed;
            if(!speed.assign({0x3fb70a3du,0x70a3d70au})){fail();return 0;}
            r->imageSpeed=stored(speed);break;
        }
        case 212:
            s.ysp=MotionBinary64::fromInteger(-3);s.text="+100 GEMS#+50 AMMO";
            r->alarms.set(0,90);r->alarms.set(1,60);s.drawing=true;
            s.decreaseBy={0x3feb3333u,0x33333333u};break;
        case 283:{
            const RunnerImageValue index{0x3f800000u};
            if(!lifecycle_.imageIndex(free->key,*r,index)||!good_){fail();return 0;}
            r->imageIndex=stored(index);r->imageSpeed=0;break;
        }
        }
        s.initialized=true;return free->id;
    }
    bool readX(unsigned id,int& x){int y;return position(id,x,y);}
    bool position(unsigned id,int& x,int& y){
        auto* e=entry(sourceHandle(id));auto* r=e?record(*e):nullptr;
        if(!good_||!r||!e->state.initialized)return fail();
        x=r->x;y=r->y;return true;
    }
    bool upgrade(unsigned id,unsigned value){
        auto* e=entry(sourceHandle(id));
        if(!good_||!e||!e->state.initialized||e->object!=283)return fail();
        e->state.whichUg=value;return true;
    }
    bool stored(unsigned id,unsigned value){
        auto* e=entry(sourceHandle(id));
        if(!good_||!e||!e->state.initialized||e->object!=279||value>0x7fffffffu)return fail();
        e->state.storedItem=int(value);return true;
    }
    bool stepRisingText(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=212||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        RunnerImageValue x,y;std::memcpy(&x.bits,&r->preciseX,4);std::memcpy(&y.bits,&r->preciseY,4);
        auto next=y.promoted();std::int32_t iy;
        if(!next.add(e->state.ysp)||!y.assign(next)||!y.roundPixel(iy)||
           !lifecycle_.publishedPosition(key,*r,x,y)||!good_)return fail();
        r->preciseY=stored(y);r->y=iy;
        // The source setter is observable before damping. Do not atomically
        // commit the final state or narrow the user variable ysp to binary32.
        if(!multiplyBinary64(e->state.ysp,e->state.decreaseBy,e->state.ysp))return fail();
        if(e->state.flashing)e->state.drawing=-e->state.drawing;
        return true;
    }
    bool advanceRisingTextAlarm(InstanceHandle key,unsigned number){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=212||!e->state.initialized||number>=AlarmBank::count)return fail();
        if(!r->active||r->destroyRequested||number>1||!r->alarms.advanceOne(number))return true;
        return risingTextAlarmEvent(key,number);
    }
    // The room's mixed alarm phase has already decremented this alarm.
    // Keep event execution separate from the standalone countdown adapter.
    bool risingTextAlarmEvent(InstanceHandle key,unsigned number){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=212||!e->state.initialized||number>1)return fail();
        if(!r->active||r->destroyRequested)return true;
        if(number==1){e->state.flashing=1;return true;}
        if(!lifecycle_.markDestroyed(key,*r)||!good_)return fail();
        r->destroyRequested=true;return true;
    }
    bool retire(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||!r->destroyRequested||
           !records_.erase(e->id,[&](auto h,const auto& value){return lifecycle_.retire(h,value);}))return fail();
        *e={};return true;
    }
    bool clear(){
        if(creating_)return fail();
        for(auto& e:entries_)if(e.id){
            if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
            e={};
        }
        return true;
    }
};
}
#endif
