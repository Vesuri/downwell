#ifndef DOWNWELL_CORE_STORED_FLASH_BALL_REGISTRATION_H
#define DOWNWELL_CORE_STORED_FLASH_BALL_REGISTRATION_H
#include "FlashBallInstances.h"
#include "InstanceStore.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
// Resource-backed stored-world adapter for the shared flash lifetime owner.
template<unsigned WorldCapacity,unsigned Capacity> class StoredFlashBallRegistration {
public:
    using Store=FlashBallInstances<Capacity>;
    using Record=typename Store::Record;
private:
    struct Entry {InstanceHandle local{},world{};std::uint32_t id=0;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    Store store_{Store::Binding::bindPositionedActor(*this)};
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(InstanceHandle key){
        for(auto& e:entries_)if(e.id&&e.local.slot==key.slot&&e.local.generation==key.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.world);return r&&r->instanceId==e.id&&r->objectId==224?r:nullptr;
    }
public:
    StoredFlashBallRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){store_.observeFailure(good);}
    StoredFlashBallRegistration(const StoredFlashBallRegistration&)=delete;
    StoredFlashBallRegistration& operator=(const StoredFlashBallRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid(){
        if(!good_||!store_.valid())return false;
        for(auto& e:entries_)if(e.id&&(!record(e)||!store_.find(e.local)))return false;
        return true;
    }
    const Store& instances()const{return store_;}
    InstanceHandle worldHandle(InstanceHandle key){auto* e=entry(key);return e&&record(*e)?e->world:InstanceHandle{};}
    InstanceHandle handle(const InstanceRecord& r){
        for(auto& e:entries_)if(e.id==r.instanceId&&record(e)==&r)return e.local;
        return {};
    }
    template<class Random> bool create(MotionBinary64 x,MotionBinary64 y,Random& random,InstanceHandle& output){
        output={};if(!good_)return false;
        InstanceHandle key;
        if(!store_.create(nextId_,x,y,random,key))return fail();
        auto* e=entry(key);auto* r=e?record(*e):nullptr;const auto* child=store_.find(key);
        if(!r||!child)return fail();
        r->imageSpeed=stored(child->state.imageSpeed);output=key;return true;
    }
    bool advanceAnimation(InstanceHandle key){return good_&&(store_.advanceAnimation(key)||fail());}
    bool activate(InstanceHandle key,bool active){return good_&&(store_.activate(key,active)||fail());}
    bool retire(InstanceHandle key){return good_&&(store_.retire(key)||fail());}
    bool clear(){return good_&&(store_.clear()||fail());}
    bool birth(InstanceHandle,const Record&){return fail();}
    bool birthAt(InstanceHandle local,const Record& source,RunnerImageValue x,RunnerImageValue y){
        if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;std::int32_t px,py;
        if(!free||!objects_||!objects_->findObject(224,definition)||definition.spriteId!=96||source.sourceId!=nextId_||
           !x.roundPixel(px)||!y.roundPixel(py))return fail();
        if(records_.create(nextId_,224,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        *free={local,records_.handleAt(records_.size()-1),nextId_++};
        auto* r=record(*free);if(!r)return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        return lifecycle_.birthAt(free->world,*r,x,y)||fail();
    }
    bool retire(InstanceHandle key,const Record&){
        auto* e=entry(key);if(!e||!record(*e))return fail();
        if(!records_.erase(e->id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        *e={};return true;
    }
    bool markDestroyed(InstanceHandle key,const Record&){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.markDestroyed(e->world,*r))return fail();r->destroyRequested=true;return true;
    }
    bool activation(InstanceHandle key,const Record&,bool active){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        return (r&&records_.requestActivation(e->world,active,[&](auto h,const auto& current,bool value){
            return lifecycle_.activation(h,current,value);
        }))||fail();
    }
    bool membership(InstanceHandle,const Record&,bool){return fail();}
    bool move(InstanceHandle key,const Record&,RunnerImageValue x,RunnerImageValue y){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;std::int32_t px,py;
        if(!r||!x.roundPixel(px)||!y.roundPixel(py)||
           !lifecycle_.publishedPosition(e->world,*r,x,y))return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);return true;
    }
    bool imageIndex(InstanceHandle key,const Record&,RunnerImageValue image){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.imageIndex(e->world,*r,image))return fail();r->imageIndex=stored(image);return true;
    }
    bool mask(InstanceHandle,const Record&,int){return fail();}
    bool sprite(InstanceHandle,const Record&,int){return fail();}
    bool transform(InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return fail();}
};
}
#endif
