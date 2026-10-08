#ifndef DOWNWELL_CORE_STORED_SKELETON_REGISTRATION_H
#define DOWNWELL_CORE_STORED_SKELETON_REGISTRATION_H
#include "SkeletonInstances.h"
#include "InstanceStore.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
template<unsigned WorldCapacity,unsigned Capacity> class StoredSkeletonRegistration {
public:
    using Store=SkeletonInstances<Capacity>;
    using Record=typename Store::Record;
private:
    struct Entry {InstanceHandle local{},world{};unsigned id=0;};
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
        auto* r=records_.resolve(e.world);return r&&r->instanceId==e.id&&r->objectId==120?r:nullptr;
    }
    bool sync(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;const auto* source=store_.find(key);
        RunnerImageValue speed,index;
        if(!r||!source||!speed.assign(source->state.imageSpeed)||!index.assign(source->state.imageIndex))return fail();
        // Sprite/transform writes require their event-time publication adapter;
        // never silently install a changed collision sprite at this boundary.
        if(r->spriteId!=source->state.sprite)return fail();
        std::uint32_t oldIndex;std::memcpy(&oldIndex,&r->imageIndex,4);
        if(oldIndex!=index.bits&&!imageIndex(key,*source,index))return false;
        r->imageSpeed=stored(speed);r->health=source->state.health;r->reward=source->state.money;
        r->grounded=source->state.grounded;
        for(unsigned i=0;i<4;++i)r->alarms.set(i,source->state.alarms[i]);
        return true;
    }
public:
    StoredSkeletonRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredSkeletonRegistration(const StoredSkeletonRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid(){
        if(!good_||!store_.valid())return false;
        for(auto& e:entries_)if(e.id&&(!record(e)||!store_.find(e.local)))return false;
        return true;
    }
    const Store& instances()const{return store_;}
    InstanceHandle worldHandle(InstanceHandle key){auto* e=entry(key);return e&&record(*e)?e->world:InstanceHandle{};}
    InstanceHandle handle(const InstanceRecord& r){
        for(auto& e:entries_)if(e.id==r.instanceId&&record(e)==&r)return e.local;return {};
    }
    bool create(int x,int y,InstanceHandle& key){
        return good_&&((store_.create(nextId_,x,y,key)&&sync(key))||fail());
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||!r)return fail();
        return (store_.dispatch(key,[&](auto& state,auto& throwing,bool& good){
            for(unsigned i=0;i<4;++i)state.alarms[i]=r->alarms.get(i);
            return event(state,throwing,good);
        })&&sync(key))||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||!r)return fail();
        if(!store_.dispatch(key,[&](auto& state,auto&,bool&){
            for(unsigned i=0;i<4;++i)state.alarms[i]=r->alarms.get(i);return true;
        }))return fail();
        return (store_.advanceAnimation(key)&&sync(key))||fail();
    }
    bool alarm(InstanceHandle key,unsigned index){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||index>=4)return fail();
        return dispatch(key,[&](auto& state,auto&,bool&){
            // The global stored-room alarm traversal already decremented it.
            state.alarms[index]=r->alarms.get(index);state.alarm(index);return true;
        });
    }
    bool activate(InstanceHandle key,bool active){return good_&&(store_.activate(key,active)||fail());}
    bool mark(InstanceHandle key){return good_&&(store_.markDestroyed(key)||fail());}
    bool retire(InstanceHandle key){return good_&&(store_.retire(key)||fail());}
    bool clear(){return store_.clear()||fail();}
    bool birth(InstanceHandle,const Record&){return fail();}
    bool birthAt(InstanceHandle local,const Record& source,RunnerImageValue x,RunnerImageValue y){
        if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;std::int32_t px,py;
        if(!free||!objects_||!objects_->findObject(120,definition)||source.sourceId!=nextId_||
           !x.promoted().roundPixel(px)||!y.promoted().roundPixel(py))return fail();
        if(records_.create(nextId_,120,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
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
        return (r&&records_.requestActivation(e->world,active,[&](InstanceHandle h,const InstanceRecord& current,bool value){
            return lifecycle_.activation(h,current,value);
        }))||fail();
    }
    bool membership(InstanceHandle key,const Record&,bool member){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        return (r&&lifecycle_.membership(e->world,*r,member))||fail();
    }
    bool move(InstanceHandle key,const Record&,RunnerImageValue x,RunnerImageValue y){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;std::int32_t px,py;
        if(!r||!x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
           !lifecycle_.publishedPosition(e->world,*r,x,y))return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);return true;
    }
    bool imageIndex(InstanceHandle key,const Record&,RunnerImageValue value){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.imageIndex(e->world,*r,value))return fail();r->imageIndex=stored(value);return true;
    }
    bool mask(InstanceHandle,const Record&,int){return fail();}
    bool sprite(InstanceHandle,const Record&,int){return fail();}
    bool transform(InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return fail();}
};
}
#endif
