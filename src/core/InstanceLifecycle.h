#ifndef DOWNWELL_CORE_INSTANCE_LIFECYCLE_H
#define DOWNWELL_CORE_INSTANCE_LIFECYCLE_H

#include "InstanceHandle.h"
#include "RunnerImageValue.h"
#include "RunnerTransformField.h"
#include "InstancePlaceResult.h"

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
    template<class Owner> static RecordLifecycleBinding bindTransforms(Owner& owner) {
        auto b=bindAnimation(owner);
        b.transform_=[](void* p,InstanceHandle h,const Record& r,RunnerTransformField field,
                       RunnerImageValue value,RunnerImageValue alpha,std::uint32_t blend){
            return static_cast<Owner*>(p)->transform(h,r,field,value,alpha,blend);
        };
        return b;
    }
    bool integerTransform(InstanceHandle h,const Record& r,RunnerTransformField field,int value,
                          RunnerImageValue alpha,std::uint32_t blend)const {
        if(!transform_)return true;
        RunnerImageValue narrowed;
        return narrowed.assign(MotionBinary64::fromInteger(value))&&
               transform_(context_,h,r,field,narrowed,alpha,blend);
    }
    template<class Owner> static RecordLifecycleBinding bindMasks(Owner& owner) {
        auto b=bindTransforms(owner);
        b.mask_=[](void* p,InstanceHandle h,const Record& r,int mask){
            return static_cast<Owner*>(p)->mask(h,r,mask);
        };
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindQueries(Owner& owner) {
        auto b=bindMasks(owner);
        b.placeMeeting_=[](void* p,InstanceHandle h,const Record& r,int object,
                          MotionBinary64 x,MotionBinary64 y,bool& hit){
            return static_cast<Owner*>(p)->placeMeeting(h,r,object,x,y,hit);
        };
        return b;
    }
    bool hasPlaceMeeting()const{return placeMeeting_!=nullptr;}
    template<class Owner> static RecordLifecycleBinding bindFrameSnapshots(Owner& owner){
        auto b=bindPositionedBirths(owner);
        b.previous_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->capturePrevious(h,r);};
        return b;
    }
    bool capturePrevious(InstanceHandle h,const Record& r)const{
        return !previous_||previous_(context_,h,r);
    }
    template<class Owner> static RecordLifecycleBinding bindPositionedBirths(Owner& owner){
        auto b=bindInstanceQueries(owner);
        b.birthAt_=[](void* p,InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y){
            return static_cast<Owner*>(p)->birthAt(h,r,x,y);
        };
        b.destroyed_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->markDestroyed(h,r);};
        b.number_=[](void* p,int object,unsigned& count){return static_cast<Owner*>(p)->instanceNumber(object,count);};
        return b;
    }
    bool instanceNumber(int object,unsigned& result)const{
        unsigned count=0;
        if(!number_||!number_(context_,object,count))return false;
        result=count;return true;
    }
    bool birthAt(InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y)const{
        // Missing positioned ownership must not silently round or become a
        // birth-then-move sequence with different insertion history.
        return birthAt_&&birthAt_(context_,h,r,x,y);
    }
    bool markDestroyed(InstanceHandle h,const Record& r)const{return destroyed_&&destroyed_(context_,h,r);}
    template<class Owner> static RecordLifecycleBinding bindInstanceQueries(Owner& owner){
        auto b=bindQueries(owner);
        b.instancePlace_=[](void* p,InstanceHandle h,const Record& r,int object,
                           MotionBinary64 x,MotionBinary64 y,InstancePlaceResult& result){
            return static_cast<Owner*>(p)->instancePlace(h,r,object,x,y,result);
        };
        return b;
    }
    bool instancePlace(InstanceHandle h,const Record& r,int object,MotionBinary64 x,MotionBinary64 y,
                       InstancePlaceResult& result)const{
        InstancePlaceResult next;
        if(!instancePlace_||!instancePlace_(context_,h,r,object,x,y,next))return false;
        if(next.id==-4){result={};return true;}
        if(next.id<=0||!next.handle.generation||next.handle.slot==0xffffffffu||!next.x.valid()||!next.y.valid())return false;
        result=next;return true;
    }
    bool placeMeeting(InstanceHandle h,const Record& r,int object,MotionBinary64 x,MotionBinary64 y,bool& hit)const{
        // Missing ownership is not a successful miss. Select the unindexed
        // adapter explicitly before calling this checked operation.
        return placeMeeting_&&placeMeeting_(context_,h,r,object,x,y,hit);
    }
    bool mask(InstanceHandle h,const Record& r,int next)const{return !mask_||mask_(context_,h,r,next);}
    bool transform(InstanceHandle h,const Record& r,RunnerTransformField field,RunnerImageValue value,
                   RunnerImageValue alpha,std::uint32_t blend)const {
        return !transform_||transform_(context_,h,r,field,value,alpha,blend);
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
    // Engine position publication requires an installed authoritative owner;
    // unlike legacy integer-only adapters, absence is not a successful no-op.
    bool publishedPosition(InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y)const{
        return move_&&move_(context_,h,r,x,y);
    }
    bool birth(InstanceHandle h,const Record& r)const{return !birth_||birth_(context_,h,r);}
    bool retire(InstanceHandle h,const Record& r)const{return !retire_||retire_(context_,h,r);}
    bool activation(InstanceHandle h,const Record& r,bool v)const{return !activation_||activation_(context_,h,r,v);}
    bool membership(InstanceHandle h,const Record& r,bool v)const{return !membership_||membership_(context_,h,r,v);}
    bool sprite(InstanceHandle h,const Record& r,int next)const{return !sprite_||sprite_(context_,h,r,next);}
private:
    void* context_=nullptr;
    bool (*birth_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*birthAt_)(void*,InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue)=nullptr;
    bool (*destroyed_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*number_)(void*,int,unsigned&)=nullptr;
    bool (*previous_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*retire_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*activation_)(void*,InstanceHandle,const Record&,bool)=nullptr;
    bool (*membership_)(void*,InstanceHandle,const Record&,bool)=nullptr;
    bool (*move_)(void*,InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue)=nullptr;
    bool (*sprite_)(void*,InstanceHandle,const Record&,int)=nullptr;
    bool (*mask_)(void*,InstanceHandle,const Record&,int)=nullptr;
    bool (*placeMeeting_)(void*,InstanceHandle,const Record&,int,MotionBinary64,MotionBinary64,bool&)=nullptr;
    bool (*instancePlace_)(void*,InstanceHandle,const Record&,int,MotionBinary64,MotionBinary64,InstancePlaceResult&)=nullptr;
    bool (*imageIndex_)(void*,InstanceHandle,const Record&,RunnerImageValue)=nullptr;
    bool (*transform_)(void*,InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t)=nullptr;
};
using InstanceLifecycleBinding = RecordLifecycleBinding<InstanceRecord>;

} // namespace downwell

#endif
