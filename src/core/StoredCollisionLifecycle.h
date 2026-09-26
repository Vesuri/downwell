#ifndef DOWNWELL_CORE_STORED_COLLISION_LIFECYCLE_H
#define DOWNWELL_CORE_STORED_COLLISION_LIFECYCLE_H
#include "InstanceStore.h"
#include "RunnerSpatialInstances.h"
#include <cstring>
namespace downwell {
// Explicit adapter for the stored birth snapshot, not an observer that infers
// arbitrary later writes. Recreate around copied stores; all ownership is
// external. Movement/resource setters must still route subsequent mutations.
template<unsigned Capacity,unsigned Nodes,class Graph,class Trig,class Round>
struct StoredCollisionLifecycle {
    using Owner=RunnerSpatialInstances<Capacity,Nodes>;
    Owner& owner;
    typename Owner::Properties (&properties)[Capacity];
    const ObjectPack& objects;
    const SpriteCollisionPack& sprites;
    Graph& graph;
    Trig trig;
    Round round;
    bool allPrepared=false;
    bool birth(InstanceHandle h,const InstanceRecord& record){
        typename Owner::State state;
        if(!state.x.assign(MotionBinary64::fromInteger(record.x))||
           !state.y.assign(MotionBinary64::fromInteger(record.y)))return false;
        if(!record.active)state.flags|=2u;
        if(!owner.attach(h,record.objectId,state))return false;
        auto& p=properties[h.slot];p={};
        if(!owner.bindObjectFromPacks(h,record.objectId,true,p,objects,sprites,graph,allPrepared))return false;
        // Room layer/dynamic compatibility depth and placed fields supersede
        // object defaults before the loader's PreCreate/Create boundary.
        if(!p.depth.assign(MotionBinary64::fromInteger(record.depth)))return false;
        p.sprite.spriteId=record.spriteId;p.sprite.maskId=record.maskId;
        p.sprite.scaleX=stored(record.scaleX);p.sprite.scaleY=stored(record.scaleY);
        p.sprite.angle=stored(record.rotation);p.sprite.imageIndex=stored(record.imageIndex);
        return owner.publishBirth(h,graph,[&](InstanceHandle live,bool publish){
            return refresh(live,record,publish);
        });
    }
    bool refresh(InstanceHandle h,const InstanceRecord& record,bool publish){
        const auto* flags=matchingFlags(h,record);
        return flags&&owner.refreshSprite(h,*flags,allPrepared,sprites,properties[h.slot].sprite,
                                          true,trig,round,publish);
    }
    // Fractional position comes explicitly from the actor's authoritative motion
    // state, never from a rounded record or a guessed per-frame synchronization.
    bool move(InstanceHandle h,const InstanceRecord& record,RunnerImageValue x,RunnerImageValue y){
        const auto* flags=matchingFlags(h,record);
        return flags&&owner.move(h,x,y,*flags);
    }
    bool mask(InstanceHandle h,const InstanceRecord& record,int maskId){
        const auto* flags=matchingFlags(h,record);
        return flags&&owner.changeMask(h,properties[h.slot].sprite,maskId,*flags);
    }
    bool sprite(InstanceHandle h,const InstanceRecord& record,int spriteId){
        const auto* flags=matchingFlags(h,record);
        if(!flags)return false;
        auto& p=properties[h.slot];
        return owner.changeSpriteFromPack(h,p.sprite,p.timeline,p.cachedFrame,spriteId,sprites,*flags);
    }
    bool transform(InstanceHandle h,const InstanceRecord& record,typename Owner::TransformField field,
                   RunnerImageValue value,RunnerImageValue alpha,std::uint32_t blend){
        const auto* flags=matchingFlags(h,record);
        return flags&&owner.changeTransform(h,properties[h.slot].sprite,field,value,alpha,blend,*flags);
    }
    bool retire(InstanceHandle h,const InstanceRecord&){return owner.retire(h);}
    bool membership(InstanceHandle h,const InstanceRecord&,bool registered){
        return owner.registerObject(h,registered);
    }
    bool activation(InstanceHandle h,const InstanceRecord& record,bool active){
        const auto* flags=matchingFlags(h,record);
        return flags&&owner.setActive(h,active,*flags);
    }
private:
    const std::uint32_t* matchingFlags(InstanceHandle h,const InstanceRecord& record)const{
        if(!owner.valid()||record.objectId<0||owner.objectId(h)!=record.objectId)return nullptr;
        return graph.flags(unsigned(record.objectId));
    }
    static RunnerImageValue stored(float value){
        static_assert(sizeof(float)==4,"Stored runner fields require binary32");
        RunnerImageValue result;std::memcpy(&result.bits,&value,4);return result;
    }
};
}
#endif
