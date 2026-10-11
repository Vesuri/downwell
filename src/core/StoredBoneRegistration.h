#ifndef DOWNWELL_CORE_STORED_BONE_REGISTRATION_H
#define DOWNWELL_CORE_STORED_BONE_REGISTRATION_H
#include "BoneProjectileInstances.h"
#include "InstanceStore.h"
#include "ObjectPack.h"
#include <cstring>
namespace downwell {
template<unsigned WorldCapacity,unsigned Capacity> class StoredBoneRegistration {
public:
    using Store=BoneProjectileInstances<Capacity>;
    using Record=typename Store::Record;
private:
    struct Entry {InstanceHandle local{},world{};unsigned id=0;};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack* objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    typename Store::Storage storage_;
    Store store_{Store::Binding::bindPositionedActor(*this)};
    bool fail(){good_=false;return false;}
    static bool same(InstanceHandle a,InstanceHandle b){return a.slot==b.slot&&a.generation==b.generation;}
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
    Entry* entry(InstanceHandle key){for(auto& e:entries_)if(e.id&&same(e.local,key))return &e;return nullptr;}
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.world);return r&&r->instanceId==e.id&&r->objectId==269?r:nullptr;
    }
    bool sync(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;const auto* source=store_.find(key);
        if(!r||!source)return fail();r->imageSpeed=stored(source->state.imageSpeed);return true;
    }
public:
    StoredBoneRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack* objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){
        if(!store_.bindStorage(&storage_))good_=false;
    }
    StoredBoneRegistration(const StoredBoneRegistration&)=delete;
    StoredBoneRegistration& operator=(const StoredBoneRegistration&)=delete;
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
    template<class Random> bool create(int x,int y,Random& random,InstanceHandle& key){
        if(!good_)return false;
        return (store_.create(nextId_,x,y,random,key)&&sync(key))||fail();
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        if(!good_)return false;
        return (store_.dispatch(key,event)&&sync(key))||fail();
    }
    bool advanceAnimation(InstanceHandle key){return good_&&((store_.advanceAnimation(key)&&sync(key))||fail());}
    bool activate(InstanceHandle key,bool active){return good_&&(store_.activate(key,active)||fail());}
    bool mark(InstanceHandle key){return good_&&(store_.markDestroyed(key)||fail());}
    bool readShootable(InstanceHandle world,MotionBinary64& health,bool& hitEffect){
        auto* r=records_.resolve(world);const auto* bone=r?store_.find(handle(*r)):nullptr;
        if(!good_||!store_.valid()||!bone)return fail();
        health=bone->state.health;hitEffect=bone->state.hitEffect;return true;
    }
    bool writeShootableHealth(InstanceHandle world,MotionBinary64 health){
        auto* r=records_.resolve(world);
        return (good_&&r&&store_.writeHealth(handle(*r),health))||fail();
    }
    bool writeShootableHit(InstanceHandle world){
        auto* r=records_.resolve(world);
        return (good_&&r&&store_.writeHit(handle(*r)))||fail();
    }
    bool assignEffect(InstanceHandle key,unsigned effectLocal,unsigned sourceId){
        return good_&&(store_.assignEffect(key,effectLocal,sourceId)||fail());
    }
    bool retire(InstanceHandle key){return good_&&(store_.retire(key)||fail());}
    bool clear(){return store_.clear()||fail();}

    bool birth(InstanceHandle,const Record&){return fail();}
    bool birthAt(InstanceHandle local,const Record& source,RunnerImageValue x,RunnerImageValue y){
        if(!good_)return false;
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;std::int32_t px,py;
        if(!free||!objects_||!objects_->findObject(269,definition)||source.sourceId!=nextId_||
            !x.promoted().roundPixel(px)||!y.promoted().roundPixel(py))return fail();
        if(records_.create(nextId_,269,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success)return fail();
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
        if(!r||!sync(key)||!lifecycle_.markDestroyed(e->world,*r))return fail();r->destroyRequested=true;return true;
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
        if(!r||!sync(key)||!x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
            !lifecycle_.publishedPosition(e->world,*r,x,y))return fail();
        r->x=px;r->y=py;r->preciseX=stored(x);r->preciseY=stored(y);return true;
    }
    bool imageIndex(InstanceHandle key,const Record&,RunnerImageValue image){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||!lifecycle_.imageIndex(e->world,*r,image))return fail();r->imageIndex=stored(image);return true;
    }
    bool mask(InstanceHandle key,const Record&,int mask){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!r||!sync(key)||!lifecycle_.mask(e->world,*r,mask))return fail();r->maskId=mask;return true;
    }
    // Bone scripts have neither sprite nor transform setters.
    bool sprite(InstanceHandle,const Record&,int){return fail();}
    bool transform(InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return fail();}
};
}
#endif
