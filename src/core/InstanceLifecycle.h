#ifndef DOWNWELL_CORE_INSTANCE_LIFECYCLE_H
#define DOWNWELL_CORE_INSTANCE_LIFECYCLE_H

#include "InstanceHandle.h"
#include "RunnerImageValue.h"
#include "RunnerTransformField.h"
#include "InstancePlaceResult.h"
#include "NativeFamilyBirth.h"

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
    // Add a borrowed layer observer without replacing collision/query ownership.
    // Install before births; both owners must outlive every copy of this binding.
    // Each callback follows successful primary publication. A failure stops the
    // caller without rolling back the primary owner's completed mutation.
    template<class Observer> bool observeLayers(Observer& observer){
        if(layerContext_)return false;
        layerContext_=&observer;
        layerBirth_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Observer*>(p)->birth(h,r);};
        layerRetire_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Observer*>(p)->retire(h,r);};
        layerActivation_=[](void* p,InstanceHandle h,const Record& r,bool v){return static_cast<Observer*>(p)->activation(h,r,v);};
        layerMembership_=[](void* p,InstanceHandle h,const Record& r,bool v){return static_cast<Observer*>(p)->membership(h,r,v);};
        layerDepth_=[](void* p,InstanceHandle h,const Record& r,MotionBinary64 v){return static_cast<Observer*>(p)->depth(h,r,v);};
        return true;
    }
    // Selected GMS2 depth publication belongs to the layer owner, not the
    // legacy room/animation list. Missing ownership is not a successful write.
    bool publishedDepth(InstanceHandle h,const Record& r,MotionBinary64 value)const{
        return layerDepth_&&layerDepth_(layerContext_,h,r,value);
    }
    bool hasDepthWrites()const{return layerDepth_!=nullptr;}
    // Native owners already publish layer births/retirements themselves.
    // Bind only their depth setter, without installing those observers twice.
    template<class Owner> static RecordLifecycleBinding bindMaskDestructionDepth(Owner& owner){
        auto b=bindMaskDestruction(owner);
        b.layerContext_=&owner;
        b.layerDepth_=[](void* p,InstanceHandle h,const Record& r,MotionBinary64 value){
            return static_cast<Owner*>(p)->depth(h,r,value);
        };
        return b;
    }
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
    // Scene admission: the record owns its published position and there is no
    // collision-position observer. Keep the owner's lifetime/failure checks,
    // but don't narrow integer values just to copy them into an unused mirror.
    // Install on a freshly bound adapter, before any record births.
    template<class Owner> bool useRecordPosition(Owner& owner){
        if(context_!=&owner||!move_||integerPositionCheck_)return false;
        integerPositionCheck_=[](void* p,InstanceHandle h,const Record& r){
            return static_cast<Owner*>(p)->recordPosition(h,r);
        };
        move_=[](void* p,InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y){
            return static_cast<Owner*>(p)->recordPosition(h,r,x,y);
        };
        return true;
    }
    template<class Owner> static RecordLifecycleBinding bindPositionedLifetime(Owner& owner){
        auto b=bind(owner);
        b.birthAt_=[](void* p,InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y){
            return static_cast<Owner*>(p)->birthAt(h,r,x,y);
        };
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindPositionedDestruction(Owner& owner){
        auto b=bindPositionedLifetime(owner);
        b.destroyed_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->markDestroyed(h,r);};
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
        return narrowed.assignInteger(value)&&
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
    bool hasOwner()const{return context_!=nullptr;}
    bool sameDestruction(const RecordLifecycleBinding& other)const{
        return destroyed_&&context_==other.context_&&destroyed_==other.destroyed_;
    }
    template<class Owner> static RecordLifecycleBinding bindQueryDestruction(Owner& owner){
        auto b=bindQueries(owner);
        b.destroyed_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->markDestroyed(h,r);};
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindDestruction(Owner& owner){
        auto b=bindAnimation(owner);
        b.destroyed_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->markDestroyed(h,r);};
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindMaskDestruction(Owner& owner){
        auto b=bindMasks(owner);
        b.destroyed_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->markDestroyed(h,r);};
        return b;
    }
    template<class Owner> static RecordLifecycleBinding bindPointQueries(Owner& owner){
        auto b=bindFrameSnapshots(owner);
        b.positionMeeting_=[](void* p,int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
            return static_cast<Owner*>(p)->positionMeeting(object,x,y,hit);
        };
        return b;
    }
    bool hasPositionMeeting()const{return positionMeeting_!=nullptr;}
    bool positionMeeting(int object,MotionBinary64 x,MotionBinary64 y,bool& hit)const{
        // Point queries have no moving caller and must not fall back to a
        // shape query or treat an unavailable owner as an empty scene.
        bool next=false;
        if(!positionMeeting_||!positionMeeting_(context_,object,x,y,next))return false;
        hit=next;return true;
    }
    template<class Owner> static RecordLifecycleBinding bindFrameSnapshots(Owner& owner){
        auto b=bindPositionedBirths(owner);
        b.previous_=[](void* p,InstanceHandle h,const Record& r){return static_cast<Owner*>(p)->capturePrevious(h,r);};
        return b;
    }
    bool capturePrevious(InstanceHandle h,const Record& r)const{
        return !previous_||previous_(context_,h,r);
    }
    // Positioned actors may use external event query adapters without exposing
    // queries through their storage binding. Still publish every transform.
    template<class Owner> static RecordLifecycleBinding bindPositionedActor(Owner& owner){
        auto b=bindMaskDestruction(owner);
        b.birthAt_=[](void* p,InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y){
            return static_cast<Owner*>(p)->birthAt(h,r,x,y);
        };
        return b;
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
    template<class Owner> static RecordLifecycleBinding bindPositionedQueryActor(Owner& owner){
        auto b=bindPositionedActor(owner);
        b.placeMeeting_=[](void* p,InstanceHandle h,const Record& r,int object,
                          MotionBinary64 x,MotionBinary64 y,bool& hit){
            return static_cast<Owner*>(p)->placeMeeting(h,r,object,x,y,hit);
        };
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
        return birthAt_&&birthAt_(context_,h,r,x,y)&&(!layerBirth_||layerBirth_(layerContext_,h,r));
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
        if(!value.assignInteger(static_cast<std::int32_t>(phase)))return false;
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
        if(integerPositionCheck_)return integerPositionCheck_(context_,h,r);
        if(!move_)return true;
        RunnerImageValue px,py;
        return px.assignInteger(x)&&py.assignInteger(y)&&
               move_(context_,h,r,px,py);
    }
    // Engine position publication requires an installed authoritative owner;
    // unlike legacy integer-only adapters, absence is not a successful no-op.
    bool publishedPosition(InstanceHandle h,const Record& r,RunnerImageValue x,RunnerImageValue y)const{
        return move_&&move_(context_,h,r,x,y);
    }
    bool birth(InstanceHandle h,const Record& r)const{
        return (!birth_||birth_(context_,h,r))&&(!layerBirth_||layerBirth_(layerContext_,h,r));
    }
    void bindNativeAllocation(NativeFamilyBirth& birth){nativeAllocation_=&birth;}
    // An additional layer observer requires the complete callback path.
    NativeFamilyBirth* nativeAllocation()const{return layerBirth_?nullptr:nativeAllocation_;}
    bool retire(InstanceHandle h,const Record& r)const{
        return (!retire_||retire_(context_,h,r))&&(!layerRetire_||layerRetire_(layerContext_,h,r));
    }
    bool activation(InstanceHandle h,const Record& r,bool v)const{
        return (!activation_||activation_(context_,h,r,v))&&(!layerActivation_||layerActivation_(layerContext_,h,r,v));
    }
    bool membership(InstanceHandle h,const Record& r,bool v)const{
        return (!membership_||membership_(context_,h,r,v))&&(!layerMembership_||layerMembership_(layerContext_,h,r,v));
    }
    bool sprite(InstanceHandle h,const Record& r,int next)const{return !sprite_||sprite_(context_,h,r,next);}
private:
    void* layerContext_=nullptr;
    NativeFamilyBirth* nativeAllocation_=nullptr;
    bool (*layerBirth_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*layerRetire_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*layerActivation_)(void*,InstanceHandle,const Record&,bool)=nullptr;
    bool (*layerMembership_)(void*,InstanceHandle,const Record&,bool)=nullptr;
    bool (*layerDepth_)(void*,InstanceHandle,const Record&,MotionBinary64)=nullptr;
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
    bool (*integerPositionCheck_)(void*,InstanceHandle,const Record&)=nullptr;
    bool (*sprite_)(void*,InstanceHandle,const Record&,int)=nullptr;
    bool (*mask_)(void*,InstanceHandle,const Record&,int)=nullptr;
    bool (*placeMeeting_)(void*,InstanceHandle,const Record&,int,MotionBinary64,MotionBinary64,bool&)=nullptr;
    bool (*positionMeeting_)(void*,int,MotionBinary64,MotionBinary64,bool&)=nullptr;
    bool (*instancePlace_)(void*,InstanceHandle,const Record&,int,MotionBinary64,MotionBinary64,InstancePlaceResult&)=nullptr;
    bool (*imageIndex_)(void*,InstanceHandle,const Record&,RunnerImageValue)=nullptr;
    bool (*transform_)(void*,InstanceHandle,const Record&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t)=nullptr;
};
using InstanceLifecycleBinding = RecordLifecycleBinding<InstanceRecord>;

} // namespace downwell

#endif
