#ifndef DOWNWELL_CORE_INSTANCE_LIFECYCLE_H
#define DOWNWELL_CORE_INSTANCE_LIFECYCLE_H

#include "InstanceHandle.h"
#include "RunnerImageValue.h"

namespace downwell {

struct InstanceRecord;
// Default for stores without an external collision owner. An installed adapter
// must publish birth before game callbacks and retire before handle invalidation.
// Adapters must not mutate the store and should reference, not copy, their owner.
struct NoInstanceLifecycle {
    bool birth(InstanceHandle, const InstanceRecord&) const { return true; }
    bool retire(InstanceHandle, const InstanceRecord&) const { return true; }
};

// Borrowed, allocation-free binding for non-templated world adapters. Install
// before their first birth. The owner must outlive the world and must not be
// shared with a copy of it. An empty binding preserves the unindexed runtime.
template<class Record>
class RecordLifecycleBinding {
public:
    template<class Owner> static RecordLifecycleBinding bind(Owner& owner) {
        RecordLifecycleBinding b;
        b.context_=&owner;
        b.birth_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->birth(h,r);};
        b.retire_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->retire(h,r);};
        b.activation_=[](void* p,InstanceHandle h,const Record& r,bool v){return static_cast<Owner*>(p)->activation(h,r,v);};
        b.membership_=[](void* p,InstanceHandle h,const Record& r,bool v){return static_cast<Owner*>(p)->membership(h,r,v);};
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindMovement(Owner& owner) {
        auto b=bind(owner);
        b.move_=[](void* p,InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y){
            return static_cast<Owner*>(p)->move(h,r,x,y);
        };
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindSpriteChanges(Owner& owner) {
        auto b=bindMovement(owner);
        b.sprite_=[](void* p,InstanceHandle h,const Record& r,int sprite){
            return static_cast<Owner*>(p)->sprite(h,r,sprite);
        };
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindAnimation(Owner& owner) {
        auto b=bindSpriteChanges(owner);
        b.imageIndex_=[](void* p,InstanceHandle h,const Record& r,RunnerImageValue index){
            return static_cast<Owner*>(p)->imageIndex(h,r,index);
        };
        return b;
    }
    bool phaseIndex(InstanceHandle h,const Record& r,unsigned phase)const {
        if(!imageIndex_)return true;
        if(phase>0x7fffffffu)return false;
        RunnerImageValue value;
        if(!value.assign(MotionBinary64::fromInteger(static_cast<std::int32_t>(phase))))return false;
        // Positive integer divided by 256: exact exponent adjustment after
        // narrowing, with no division helper or intermediate host float.
        if(value.bits)value.bits-=8u<<23;
        return imageIndex(h,r,value);
    }
    bool imageIndex(InstanceHandle h,const Record& r,RunnerImageValue value)const {
        return !imageIndex_||imageIndex_(context_,h,r,value);
    }
    // For explicit rounded x/y assignments in the supported enemy scripts,
    // not xx/yy motion accumulators or a query-time synchronization pass.
    bool integerPosition(InstanceHandle h,const Record& r,std::int32_t x,std::int32_t y)const {
        if(!move_)return true;
        RunnerImageValue px,py;
        return px.assign(MotionBinary64::fromInteger(x))&&py.assign(MotionBinary64::fromInteger(y))&&
               move_(context_,h,r,px,py);
    }
    bool birth(InstanceHandle h,const Record& r)const{return !birth_||birth_(context_,h,r);}
    bool retire(InstanceHandle h,const Record& r)const{return !retire_||retire_(context_,h,r);}
    bool activation(InstanceHandle h,const Record& r,bool v)const{return !activation_||activation_(context_,h,r,v);}
    bool membership(InstanceHandle h,const Record& r,bool v)const{return !membership_||membership_(context_,h,r,v);}
    bool sprite(InstanceHandle h,const Record& r,int next)const{return !sprite_||sprite_(context_,h,r,next);}
private:
    void* context_=nullptr;
    bool (*birth_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*retire_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*activation_)(void*,InstanceHandle,const Record&,bool)=nullptr;
    bool (*membership_)(void*,InstanceHandle,const Record&,bool)=nullptr;
    bool (*move_)(void*,InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue)=nullptr;
    bool (*sprite_)(void*,InstanceHandle,const Record&,int)=nullptr;
    bool (*imageIndex_)(void*,InstanceHandle,const Record&,RunnerImageValue)=nullptr;
};
using InstanceLifecycleBinding = RecordLifecycleBinding<InstanceRecord>;

} // namespace downwell

#endif
