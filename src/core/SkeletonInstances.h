#ifndef DOWNWELL_CORE_SKELETON_INSTANCES_H
#define DOWNWELL_CORE_SKELETON_INSTANCES_H
#include "SkeletonActor.h"
#include "RegisteredSkeletonThrow.h"
#include "InstanceLifecycle.h"
#include "CallbackFailureSignal.h"
namespace downwell {
// Fixed, generation-addressed ownership. Source numeric IDs are allocated by
// the room, never derived from a local slot. Engine active != actor.active.
template<unsigned Capacity> class SkeletonInstances {
    static_assert(Capacity>0,"nonempty skeleton storage required");
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        static constexpr int objectId=120;
        RunnerImageValue x{},y{};
        int spriteId=208,publishedFacing=1;
        SkeletonActor state;
        RegisteredSkeletonThrow throwing;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    struct Builtins {
        void* context=nullptr;
        bool (*publish)(void*,InstanceHandle,const Record&)=nullptr;
    };
    explicit SkeletonInstances(Binding binding,Builtins builtins={}):binding_(binding),builtins_(builtins){}
    SkeletonInstances(const SkeletonInstances&)=delete;
    SkeletonInstances& operator=(const SkeletonInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner,CallbackFailureScopes& scopes){good_.observe(owner,scopes);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    bool create(unsigned id,int x,int y,InstanceHandle& result){
        if(!good_)return false;
        if(!id||id>0x7fffffffu||!next_||executing_.generation||!binding_.hasOwner())return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==id)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        if(slot==Capacity)return fail();
        Record birth;
        if(!birth.x.assignInteger(x)||!birth.y.assignInteger(y))return fail();
        birth.lifetimeId=next_++;birth.sourceId=id;
        records_[slot]=birth;++count_;result={slot,birth.lifetimeId};executing_=result;
        auto& r=records_[slot];
        // Publish constructor geometry before inherited move_snap. The native
        // slot survives a failed publication; the owning simulation must stop.
        bool ok=binding_.birthAt(result,r,r.x,r.y)&&good_;
        SkeletonActor created;
        if(ok)ok=created.create(x,y);
        RunnerImageValue snappedX,snappedY;
        if(ok)ok=snappedX.assignInteger(created.x)&&snappedY.assignInteger(created.y);
        if(ok)ok=publishPosition(result,r,snappedX,r.y);
        if(ok)ok=publishPosition(result,r,r.x,snappedY);
        if(ok)r.state=created;
        executing_={};return good_=good_&&ok;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);
        if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();
        r->destroyRequested=true;r->state.destroyRequested=true;return true;
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        if(!r->active||r->destroyRequested)return true;
        executing_=key;auto status=good_.borrow();
        const bool ok=event(r->state,r->throwing,status.value());
        RunnerImageValue x,y;
        const bool published=r->state.sprite==r->spriteId&&r->state.facing==r->publishedFacing&&
            x.assignInteger(r->state.x)&&y.assignInteger(r->state.y)&&x.bits==r->x.bits&&y.bits==r->y.bits;
        executing_={};return good_=good_&&ok&&published;
    }
    class Publisher {
        SkeletonInstances& store_;InstanceHandle key_;
    public:
        Publisher(SkeletonInstances& store,InstanceHandle key):store_(store),key_(key){}
        template<class Create> bool smoke(Create create){
            auto* r=store_.resolve(key_);if(!r)return store_.fail();
            return create(r->state.smokeScript)||store_.fail();
        }
        bool imageSpeed(RunnerImageValue value){
            auto* r=store_.resolve(key_);if(!r)return store_.fail();
            r->state.imageSpeed=value.promoted();return store_.publishBuiltins(key_,*r);
        }
        bool alarm(unsigned index,int value){
            auto* r=store_.resolve(key_);if(!r||index>=4)return store_.fail();
            r->state.alarms[index]=value;return store_.publishBuiltins(key_,*r);
        }
        bool imageIndex(RunnerImageValue value){
            auto* r=store_.resolve(key_);
            if(!r||!store_.binding_.imageIndex(key_,*r,value))return store_.fail();
            r->state.imageIndex=value.promoted();return true;
        }
        bool sprite(int sprite){
            auto* r=store_.resolve(key_);if(!r)return store_.fail();
            auto next=static_cast<const SkeletonAnimation&>(r->state);
            if(!next.setSprite(sprite)||!store_.binding_.sprite(key_,*r,sprite))return store_.fail();
            r->state.sprite=next.sprite;r->state.imageIndex=next.imageIndex;r->state.timeline=next.timeline;
            r->spriteId=next.sprite;
            return true;
        }
        bool facing(int direction){
            auto* r=store_.resolve(key_);RunnerImageValue scale;
            if(!r||direction< -1||direction>1||!scale.assignInteger(direction)||
               !store_.binding_.transform(key_,*r,RunnerTransformField::scaleX,scale,{0x3f800000u},0xffffffu))return store_.fail();
            r->state.facing=direction;r->publishedFacing=direction;return true;
        }
        bool position(RunnerImageValue x,RunnerImageValue y){
            auto* r=store_.resolve(key_);std::int32_t px,py;RunnerImageValue integerX,integerY;
            // These event setters receive round(xx), round(yy). Do not hide
            // fractional external writes behind SkeletonActor's integer fields.
            if(!r||!x.promoted().roundPixel(px)||!y.promoted().roundPixel(py)||
               !integerX.assignInteger(px)||!integerY.assignInteger(py)||integerX.bits!=x.bits||integerY.bits!=y.bits||
               !store_.publishPosition(key_,*r,x,y))return store_.fail();
            r->state.x=px;r->state.y=py;return true;
        }
        bool destroy(){return store_.markDestroyed(key_);}
        template<class Create> bool deadBody(Create create){
            auto* r=store_.resolve(key_);if(!r)return store_.fail();
            // Private script fields alias the actual caller: partial failure
            // must not lose myDeadBody, impactSp or the xsp fallback write.
            struct Facing {
                const int& value;
                operator MotionBinary64()const{return MotionBinary64::fromInteger(value);}
            };
            struct Source {
                MotionBinary64 x,y;Facing imageXscale;
                MotionBinary64& xsp;MotionBinary64& impactSp;
                int& bodySprite;std::uint32_t& myDeadBody;
            } source{r->x.promoted(),r->y.promoted(),{r->publishedFacing},
                r->state.xsp,r->state.impactSp,r->state.bodySprite,r->state.myDeadBody};
            return create(source)||store_.fail();
        }
    };
    bool endStep(InstanceHandle key,MotionBinary64 viewY,bool mainRoom){
        return dispatchPublished(key,[&](auto&,auto&,auto& publish,bool& good){
            const auto* record=find(key);bool retire=false;
            if(!record||!outOfViewRetirement(record->y.promoted(),viewY,mainRoom,retire))return good=false;
            return !retire||publish.destroy()||(good=false);
        });
    }
    template<class Event> bool dispatchPublished(InstanceHandle key,Event event){
        return dispatch(key,[&](auto& state,auto& throwing,bool& good){
            Publisher publish(*this,key);return event(state,throwing,publish,good);
        });
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r||executing_.generation)return fail();
        auto next=static_cast<const SkeletonAnimation&>(r->state);bool crossed;RunnerImageValue index;
        executing_=key;
        const bool ok=next.advanceImage(crossed)&&index.assign(next.imageIndex)&&binding_.imageIndex(key,*r,index);
        if(ok&&good_){r->state.timeline=next.timeline;r->state.imageIndex=next.imageIndex;}
        executing_={};return good_=good_&&ok;
    }
    bool advanceAlarm(InstanceHandle key,unsigned index){
        if(index>=4)return fail();
        return dispatch(key,[&](auto& state,auto&,bool&){
            if(!state.advanceAlarm(index)||index!=2)return true;
            return binding_.imageIndex(key,*find(key),{});
        });
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||!r->destroyRequested||executing_.generation||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_.generation)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();
            records_[i]={};--count_;
        }
        good_=true;return true;
    }
private:
    bool publishBuiltins(InstanceHandle key,const Record& r){
        return (!builtins_.publish||builtins_.publish(builtins_.context,key,r))||fail();
    }
    bool publishPosition(InstanceHandle key,Record& r,RunnerImageValue x,RunnerImageValue y){
        if(!binding_.publishedPosition(key,r,x,y))return false;r.x=x;r.y=y;return true;
    }
    bool fail(){good_=false;return false;}
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    Record records_[Capacity]{};Binding binding_;Builtins builtins_;
    unsigned next_=1,count_=0;CallbackFailureSignal good_=true;InstanceHandle executing_{};
};
}
#endif
