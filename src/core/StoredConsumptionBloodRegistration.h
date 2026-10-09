#ifndef DOWNWELL_CORE_STORED_CONSUMPTION_BLOOD_REGISTRATION_H
#define DOWNWELL_CORE_STORED_CONSUMPTION_BLOOD_REGISTRATION_H
#include "ConsumptionBloodEffect.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
template<unsigned WorldCapacity,unsigned Capacity> class StoredConsumptionBloodRegistration {
    struct Entry {InstanceHandle key{};std::uint32_t id=0;ConsumptionBloodEffect state;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;InstanceLifecycleBinding lifecycle_;
    std::uint32_t& nextId_;const int& room_;bool& good_;
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(const InstanceRecord& record){
        for(auto& e:entries_)if(e.id==record.instanceId&&record.objectId==223&&records_.resolve(e.key)==&record)return &e;
        return nullptr;
    }
    struct Publish {
        StoredConsumptionBloodRegistration& owner;Entry& e;
        bool imageSpeed(RunnerImageValue v){auto* r=owner.records_.resolve(e.key);if(!r)return owner.fail();r->imageSpeed=stored(v);return true;}
        bool imageIndex(RunnerImageValue v){
            auto* r=owner.records_.resolve(e.key);
            if(!r||!owner.lifecycle_.imageIndex(e.key,*r,v))return owner.fail();
            r->imageIndex=stored(v);return true;
        }
        bool position(RunnerImageValue x,RunnerImageValue y){
            auto* r=owner.records_.resolve(e.key);std::int32_t ix,iy;
            if(!r||!x.promoted().roundPixel(ix)||!y.promoted().roundPixel(iy)||
               !owner.lifecycle_.publishedPosition(e.key,*r,x,y))return owner.fail();
            r->x=ix;r->y=iy;r->preciseX=stored(x);r->preciseY=stored(y);return true;
        }
        bool destroy(){
            auto* r=owner.records_.resolve(e.key);
            if(!r||!owner.lifecycle_.markDestroyed(e.key,*r))return owner.fail();
            r->destroyRequested=true;return true;
        }
    };
    template<class Globals> struct Environment:Publish {
        Globals& globals;
        Environment(StoredConsumptionBloodRegistration& o,Entry& e,Globals& g):Publish{o,e},globals(g){}
        bool gravity(MotionBinary64 v){
            auto* r=this->owner.records_.resolve(this->e.key);
            if(!r)return this->owner.fail();
            r->alarms.set(0,this->e.state.alarm0);
            return globals.gravity(v);
        }
        MotionBinary64 gravity(){return globals.gravity();}
        MotionBinary64 fallSpeed(){return globals.fallSpeed();}
    };
public:
    StoredConsumptionBloodRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredConsumptionBloodRegistration(const StoredConsumptionBloodRegistration&)=delete;
    StoredConsumptionBloodRegistration& operator=(const StoredConsumptionBloodRegistration&)=delete;
    bool valid()const{return good_;}
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    const ConsumptionBloodEffect* state(const InstanceRecord& r){auto* e=entry(r);return e?&e->state:nullptr;}
    template<class Random,class Globals> bool create(MotionBinary64 x,MotionBinary64 y,Random& random,Globals& globals,std::uint32_t& child){
        child=0;if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_||!objects_->findObject(223,definition)||definition.spriteId!=92||
           !nextId_||nextId_>0x7fffffffu||!px.assign(x)||!py.assign(y)||
           !px.promoted().roundPixel(ix)||!py.promoted().roundPixel(iy))return fail();
        if(records_.create(nextId_,223,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        free->id=nextId_++;free->key=records_.handleAt(records_.size()-1);
        auto* r=records_.resolve(free->key);if(!r)return fail();
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        Environment<Globals> env{*this,*free,globals};
        if(!lifecycle_.birthAt(free->key,*r,px,py)||!free->state.create(x,y,random,env)||!good_)return fail();
        child=free->id;return true;
    }
    template<class Globals> bool step(const InstanceRecord& r,Globals& globals){
        auto* e=entry(r);if(!good_||!e)return fail();if(!r.active||r.destroyRequested)return true;
        Environment<Globals> env{*this,*e,globals};
        return (e->state.step(env)&&good_)||fail();
    }
    bool advanceAnimation(const InstanceRecord& r){
        auto* e=entry(r);if(!good_||!e||r.spriteId!=92)return fail();if(!r.active||r.destroyRequested)return true;
        std::memcpy(&e->state.imageSpeed.bits,&r.imageSpeed,4);std::memcpy(&e->state.imageIndex.bits,&r.imageIndex,4);
        Publish publish{*this,*e};return (e->state.advanceImage(publish)&&good_)||fail();
    }
    bool kill(const InstanceRecord& r){
        auto* e=entry(r);if(!good_||!e)return fail();Publish publish{*this,*e};return e->state.kill(publish)||fail();
    }
    bool retire(const InstanceRecord& r){
        auto* e=entry(r);if(!good_||!e||!r.destroyRequested||!e->state.destroyRequested)return fail();
        return erase(*e);
    }
    bool clear(){for(auto& e:entries_)if(e.id&&!erase(e))return false;return true;}
private:
    bool erase(Entry& e){
        if(!records_.resolve(e.key)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        e={};return true;
    }
};
}
#endif
