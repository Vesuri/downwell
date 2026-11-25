#ifndef DOWNWELL_CORE_STORED_BLAST_REGISTRATION_H
#define DOWNWELL_CORE_STORED_BLAST_REGISTRATION_H
#include "BlastInstances.h"
#include "InstanceStore.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
template<unsigned WorldCapacity,unsigned Capacity> class StoredBlastRegistration {
public:
    using Store=BlastInstances<Capacity>;using Record=typename Store::Record;
private:
    struct Entry {InstanceHandle local{},world{};unsigned id=0;};Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;InstanceLifecycleBinding lifecycle_;
    std::uint32_t& nextId_;const int& room_;bool& good_;
    Store store_{Store::Binding::bindPositionedActor(*this),{this,[](void* p,InstanceHandle h,const Record& r,RunnerImageValue v){
        return static_cast<StoredBlastRegistration*>(p)->imageSpeed(h,r,v);
    }}};
    bool fail(){good_=false;return false;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(InstanceHandle key){for(auto& e:entries_)if(e.id&&e.local.slot==key.slot&&e.local.generation==key.generation)return &e;return nullptr;}
    InstanceRecord* record(const Entry& e){auto* r=records_.resolve(e.world);return r&&r->instanceId==e.id&&r->objectId==276?r:nullptr;}
public:
    StoredBlastRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,InstanceLifecycleBinding lifecycle,
        std::uint32_t& nextId,const int& room,bool& good):records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredBlastRegistration(const StoredBlastRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid(){
        if(!good_||!store_.valid())return false;
        for(auto& e:entries_)if(e.id&&(!record(e)||!store_.find(e.local)))return false;
        return true;
    }
    const Store& instances()const{return store_;}
    InstanceHandle handle(const InstanceRecord& r){for(auto& e:entries_)if(e.id==r.instanceId&&record(e)==&r)return e.local;return {};}
    InstanceHandle worldHandle(InstanceHandle key){auto* e=entry(key);return e&&record(*e)?e->world:InstanceHandle{};}
    template<class Services> bool create(MotionBinary64 x,MotionBinary64 y,Services& services,InstanceHandle& out){
        return good_&&(store_.create(nextId_,x,y,services,out)||fail());
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* e=entry(key);if(!good_||!e||!record(*e))return fail();
        return store_.dispatch(key,event)||fail();
    }
    bool advanceAnimation(InstanceHandle key){return dispatch(key,[](auto& state,auto& publish,bool& good){return state.advanceAnimation(publish,good);});}
    bool step(InstanceHandle key){return dispatch(key,[](auto& state,auto&,bool& good){return state.step(good);});}
    bool alarm(InstanceHandle key,unsigned index){return dispatch(key,[&](auto& state,auto&,bool& good){
        if(index!=0)return good=false;state.alarm0();return true;
    });}
    bool activate(InstanceHandle key,bool active){return good_&&(store_.activate(key,active)||fail());}
    bool retire(InstanceHandle key){return good_&&(store_.retire(key)||fail());}
    bool clear(){return store_.clear()||fail();}
    bool birth(InstanceHandle,const Record&){return fail();}
    bool birthAt(InstanceHandle local,const Record& source,RunnerImageValue x,RunnerImageValue y){
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;std::int32_t px,py;
        if(!good_||!free||!objects_||!objects_->findObject(276,definition)||definition.spriteId!=471||
           source.sourceId!=nextId_||!x.roundPixel(px)||!y.roundPixel(py))return fail();
        if(records_.create(nextId_,276,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
        *free={local,records_.handleAt(records_.size()-1),nextId_++};auto* r=record(*free);if(!r)return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        return lifecycle_.birthAt(free->world,*r,x,y)||fail();
    }
    bool retire(InstanceHandle key,const Record&){
        auto* e=entry(key);if(!e||!record(*e)||!records_.erase(e->id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
        *e={};return true;
    }
    bool markDestroyed(InstanceHandle key,const Record&){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.markDestroyed(e->world,*r))return fail();r->destroyRequested=true;return true;
    }
    bool activation(InstanceHandle key,const Record&,bool active){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        return (r&&records_.requestActivation(e->world,active,[&](auto h,const auto& current,bool value){return lifecycle_.activation(h,current,value);}))||fail();
    }
    bool membership(InstanceHandle key,const Record&,bool member){auto* e=entry(key);auto* r=e?record(*e):nullptr;return (r&&lifecycle_.membership(e->world,*r,member))||fail();}
    bool imageIndex(InstanceHandle key,const Record&,RunnerImageValue value){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.imageIndex(e->world,*r,value))return fail();r->imageIndex=stored(value);return true;
    }
    bool imageSpeed(InstanceHandle key,const Record&,RunnerImageValue value){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;if(!r)return fail();r->imageSpeed=stored(value);return true;
    }
    bool transform(InstanceHandle key,const Record&,RunnerTransformField field,RunnerImageValue value,RunnerImageValue alpha,std::uint32_t blend){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||field!=RunnerTransformField::scaleX||!lifecycle_.transform(e->world,*r,field,value,alpha,blend))return fail();r->scaleX=stored(value);return true;
    }
    bool move(InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue){return fail();}
    bool sprite(InstanceHandle,const Record&,int){return fail();}
    bool mask(InstanceHandle,const Record&,int){return fail();}
};
}
#endif
