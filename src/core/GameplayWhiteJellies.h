#ifndef DOWNWELL_CORE_GAMEPLAY_WHITE_JELLIES_H
#define DOWNWELL_CORE_GAMEPLAY_WHITE_JELLIES_H
#include "InstanceLifecycle.h"
#include "WhiteJellyStep.h"
#include "RunnerDirectionValue.h"
#include "RunnerSpriteChange.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Native owner for source object 115. No generic room-record duplication.
// Builtin operations publish through this owner at the actual script writes;
// direction and effect semantics remain explicit operation dependencies.
class GameplayWhiteJellies {
public:
    static constexpr unsigned capacity=32;
    struct Record:WhiteJellyStep {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue imageIndex{},timeline{};
        AlarmBank alarms;
        int spriteId=176,maskId=174;
        bool active=true,destroyRequested=false,busy=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayWhiteJellies(Binding binding={}):binding_(binding){}
    GameplayWhiteJellies(const GameplayWhiteJellies&)=delete;
    GameplayWhiteJellies& operator=(const GameplayWhiteJellies&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    InstanceHandle handle(unsigned slot)const{return slot<capacity?InstanceHandle{slot,records_[slot].lifetimeId}:InstanceHandle{};}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    template<class Random,class MakeOps>
    bool create(unsigned source,RunnerImageValue x,RunnerImageValue y,Random& random,MakeOps makeOps,InstanceHandle& output){
        output={};
        if(!good_||!next_||count_==capacity||!source||source>0x7fffffffu)return fail();
        for(const auto& r:records_)if(r.lifetimeId&&r.sourceId==source)return fail();
        unsigned slot=0;while(records_[slot].lifetimeId)++slot;
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=source;r.x=x;r.y=y;++count_;
        const auto key=handle(slot);r.busy=true;
        if(!binding_.birthAt(key,r,x,y)){r.busy=false;return fail();}
        auto ops=makeOps(key);
        const bool ok=r.WhiteJellyBirth::create(x,y,random,ops);r.busy=false;
        if(!good_||!ok)return fail();
        output=key;return true;
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        if(!good_||!find(key))return fail();
        auto& r=records_[key.slot];if(r.busy||!r.initialized)return fail();
        if(!r.active||r.destroyRequested)return true;
        r.busy=true;const bool ok=event(r);r.busy=false;
        return (good_&&ok)||fail();
    }
    template<class Random,class Ops> bool step(InstanceHandle key,bool stopped,Random& random,Ops& ops){
        return dispatch(key,[&](auto& r){return r.step(stopped,random,ops);});
    }
    bool position(InstanceHandle key,bool vertical,RunnerImageValue value,RunnerImageValue& accepted){
        if(!good_||!find(key))return fail();auto& r=records_[key.slot];
        const auto x=vertical?r.x:value,y=vertical?value:r.y;
        if(!binding_.publishedPosition(key,r,x,y))return fail();
        if(vertical)r.y=value;else r.x=value;accepted=value;return true;
    }
    bool meeting(InstanceHandle key,MotionBinary64 x,MotionBinary64 y,int object,bool& hit){
        if(!good_||!find(key))return fail();
        return binding_.placeMeeting(key,records_[key.slot],object,x,y,hit)||fail();
    }
    // This family never writes builtin speed: engine components stay zero,
    // independently of custom xsp/ysp. Keep the mandatory dirty publication.
    template<class Publish> bool direction(InstanceHandle key,MotionBinary64 wanted,
        MotionBinary64& accepted,Publish publish){
        if(!good_||!find(key))return fail();RunnerImageValue value;
        if(!RunnerDirectionValue::normalize(wanted,value))return fail();
        auto& r=records_[key.slot];r.direction=value.promoted();
        if(!publish(key,r))return fail();
        accepted=r.direction;return true;
    }
    bool imageIndex(InstanceHandle key,RunnerImageValue value){
        // The authored hit event only assigns zero. Do not claim arbitrary
        // inverse sequence mapping for external image_index writes.
        if(!good_||!find(key)||(value.bits&0x7fffffffu)||!binding_.imageIndex(key,records_[key.slot],value))return fail();
        records_[key.slot].imageIndex=value;return true;
    }
    bool setAlarm(InstanceHandle key,unsigned number,int value){
        if(!good_||!find(key)||number>=AlarmBank::count)return fail();
        records_[key.slot].alarms.set(number,value);return true;
    }
    template<class EffectAlarm> bool advanceAlarm(InstanceHandle key,unsigned number,EffectAlarm effectAlarm){
        return dispatch(key,[&](auto& r){
            if(number>=AlarmBank::count)return false;
            if(!r.alarms.advanceOne(number))return true;
            if(number==1){r.hitStun=false;return true;}
            return number==0&&effectAlarm(r);
        });
    }
    bool advanceAlarm(InstanceHandle key,unsigned number){
        // A fired effect alarm needs its source emitTimer and synchronous
        // effect owners. Never turn missing event dependencies into a no-op.
        return advanceAlarm(key,number,[](auto&){return false;});
    }
    static unsigned frames(int sprite){return sprite==176?4:sprite==175?1:0;}
    static bool drawFrame(int sprite,RunnerImageValue index,unsigned& frame){
        const unsigned count=frames(sprite),magnitude=index.bits&0x7fffffffu,exponent=magnitude>>23;
        if(!count||!SequenceTimeline::valid(index)||((index.bits>>31)&&magnitude)||
            magnitude>=SequenceTimeline::integerBits(count))return false;
        frame=exponent<127?0:((magnitude&0x7fffffu)|0x800000u)>>(150-exponent);return true;
    }
    bool sprite(InstanceHandle key,int value){
        if(!good_||!find(key)||!frames(value))return fail();auto& r=records_[key.slot];
        RunnerSpriteChange::Result change;
        if(!RunnerSpriteChange::plan(r.spriteId,value,r.imageIndex,frames(value),change)||
            !binding_.sprite(key,r,value))return fail();
        r.spriteId=value;
        if(change.resetIndex)r.imageIndex={};
        if(change.resetTimeline)r.timeline={};
        return true;
    }
    bool advanceAnimation(InstanceHandle key){
        if(!good_||!find(key))return fail();auto& r=records_[key.slot];
        if(r.busy||!r.initialized)return fail();
        if(!r.active||r.destroyRequested)return true;
        auto candidate=r.timeline;RunnerImageValue image;bool reconcile=false,crossed;
        if(!UnitSequenceFrames::evaluate(candidate,frames(r.spriteId),image)){
            if(r.imageIndex.bits&0x7fffffffu)return fail();reconcile=true;
        }else if(!UnitSequenceFrames::needsReconcile(r.imageIndex,image,reconcile))return fail();
        if(reconcile){if(r.imageIndex.bits&0x7fffffffu)return fail();candidate={};}
        r.busy=true;
        const bool ok=UnitSequenceFrames::advance(candidate,r.imageSpeed,frames(r.spriteId),image,crossed)&&
            binding_.imageIndex(key,r,image);
        r.busy=false;
        if(!good_||!ok)return fail();r.timeline=candidate;r.imageIndex=image;return true;
    }
    bool activate(InstanceHandle key,bool active){
        if(!good_||!find(key))return fail();auto& r=records_[key.slot];
        if(r.active==active)return true;
        if(!binding_.activation(key,r,active))return fail();r.active=active;return true;
    }
    bool markDestroyed(InstanceHandle key){
        if(!good_||!find(key))return fail();auto& r=records_[key.slot];
        if(r.destroyRequested)return true;
        if(!binding_.markDestroyed(key,r))return fail();r.destroyRequested=true;return true;
    }
    bool retireMarked(InstanceHandle key){
        if(!good_||!find(key))return fail();auto& r=records_[key.slot];
        if(r.busy||!r.destroyRequested||!binding_.retire(key,r))return fail();
        r={};--count_;return true;
    }
    bool clear(){
        if(!good_)return false;
        for(const auto& r:records_)if(r.busy)return fail();
        for(unsigned i=0;i<capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire(handle(i),records_[i]))return fail();records_[i]={};--count_;
        }
        return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record records_[capacity]{};Binding binding_;unsigned next_=1,count_=0;bool good_=true;
};
}
#endif
