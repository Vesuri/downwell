#ifndef DOWNWELL_CORE_CORPSE_INSTANCES_H
#define DOWNWELL_CORE_CORPSE_INSTANCES_H
#include "CorpseAnimation.h"
#include "DeadBodyCreation.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
namespace downwell {
// Object 263 native ownership; storage generations are not source instance IDs.
// Event bodies remain shared with the stored host owner.
template<unsigned Capacity> class CorpseInstances {
    static_assert(Capacity>0,"nonempty corpse instance capacity required");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        int spriteId=172;
        RunnerImageValue imageXscale{0x3f800000u};
        CorpseAnimation state;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=263;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit CorpseInstances(Binding binding):binding_(binding){}
    CorpseInstances(const CorpseInstances&)=delete;
    CorpseInstances& operator=(const CorpseInstances&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<Capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle sourceHandle(std::uint32_t id)const{
        for(unsigned i=0;id&&i<Capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==id)return {i,records_[i].lifetimeId};
        return {};
    }
    class Publisher {
        CorpseInstances& owner_;InstanceHandle key_;
    public:
        Publisher(CorpseInstances& owner,InstanceHandle key):owner_(owner),key_(key){}
        bool position(RunnerImageValue x,RunnerImageValue y){
            auto* r=owner_.resolve(key_);
            if(!r||!owner_.binding_.publishedPosition(key_,*r,x,y))return owner_.fail();
            r->x=x;r->y=y;return true;
        }
        bool meeting(int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
            auto* r=owner_.resolve(key_);bool result=false;
            if(!r||!owner_.binding_.placeMeeting(key_,*r,object,x,y,result))return owner_.fail();
            hit=result;return true;
        }
        bool destroy(){
            auto* r=owner_.resolve(key_);if(!r)return owner_.fail();
            if(!r->destroyRequested&&!owner_.binding_.markDestroyed(key_,*r))return owner_.fail();
            r->destroyRequested=r->state.destroyRequested=true;return true;
        }
    };
    template<class Random,class Tile> bool create(std::uint32_t id,MotionBinary64 x,MotionBinary64 y,
        bool rip,Random& random,Tile tile,InstanceHandle& output){
        output={};if(!good_||executing_||!next_||!id||id>0x7fffffffu||sourceHandle(id).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        RunnerImageValue px,py;if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=id;r.x=px;r.y=py;
        r.state.x=px;r.state.y=py;r.state.imageSpeed={0x3f800000u};++count_;
        const InstanceHandle key{slot,r.lifetimeId};
        executing_=true;struct Reset {bool& flag;~Reset(){flag=false;}} reset{executing_};
        if(!binding_.birthAt(key,r,px,py))return fail();
        Publisher publish(*this,key);
        struct Queries {
            Publisher& output;Tile& tiles;InstanceHandle key;
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){return output.meeting(57,x,y,hit);}
            bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){return tiles(key,100,x,y,hit);}
            bool publish(RunnerImageValue x,RunnerImageValue y){return output.position(x,y);}
        } queries{publish,tile,key};
        if(!r.state.create(x,y,rip,random,queries)||!good_)return fail();
        output=key;return true;
    }
    // The enclosing creation stream supplies the source ID; the slot generation
    // is only a lifetime key. Child Create finishes before script overrides.
    template<class Source,class Random,class Tile> bool createFromDeath(std::uint32_t id,Source& source,
        int spriteId,MotionBinary64 impact,int explosion,bool rip,Random& random,Tile tile){
        return createFromDeathAllocated(source,spriteId,impact,explosion,rip,random,tile,
            [id](std::uint32_t& child){child=id;return true;});
    }
    // Allocate at instance_create, after the caller's bodySprite/impactSp
    // writes. An allocation failure must not discard that script prefix.
    template<class Source,class Random,class Tile,class Allocate> bool createFromDeathAllocated(Source& source,
        int spriteId,MotionBinary64 impact,int explosion,bool rip,Random& random,Tile tile,Allocate allocate){
        if(!good_||executing_)return fail();
        struct Events {
            CorpseInstances& owner;Allocate& allocate;Random& random;Tile& tile;bool rip;
            bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& child){
                InstanceHandle key;std::uint32_t sourceId=0;
                if(object!=263||!allocate(sourceId)||!owner.create(sourceId,x,y,rip,random,tile,key))return false;
                child=sourceId;return true;
            }
            Record* selected(unsigned id){return owner.resolve(owner.sourceHandle(id));}
            bool sprite(unsigned id,int value){return owner.sprite(owner.sourceHandle(id),value);}
            bool facing(unsigned id,MotionBinary64 value){return owner.facing(owner.sourceHandle(id),value);}
            bool verticalSpeed(unsigned id,MotionBinary64 value){auto* r=selected(id);if(!r)return false;r->state.ysp=value;return true;}
            bool horizontalSpeed(unsigned id,MotionBinary64 value){auto* r=selected(id);if(!r)return false;r->state.xsp=value;return true;}
            bool explosion(unsigned id,int value){auto* r=selected(id);if(!r)return false;r->state.explosionFx=value;return true;}
        } events{*this,allocate,random,tile,rip};
        return createDeadBody(source,spriteId,impact,explosion,events)||fail();
    }
    bool sprite(InstanceHandle key,int sprite){
        auto* r=resolve(key);if(!r)return fail();
        return r->state.setSprite(sprite,[&](const auto& state){
            if(!binding_.sprite(key,*r,sprite))return false;
            r->spriteId=sprite;return binding_.imageIndex(key,*r,state.imageIndex);
        })||fail();
    }
    bool facing(InstanceHandle key,MotionBinary64 value){
        auto* r=resolve(key);RunnerImageValue next;if(!r||!next.assign(value))return fail();
        const auto old=r->state.imageXscale.bits;
        if(next.bits==old||!((next.bits|old)&0x7fffffffu))return true;
        if(!binding_.transform(key,*r,RunnerTransformField::scaleX,next,{0x3f800000u},0xffffffu))return fail();
        r->state.imageXscale=r->imageXscale=next;return true;
    }
    template<class Execute> bool dispatchPublished(InstanceHandle key,Execute execute){
        auto* r=resolve(key);if(!r||executing_)return fail();if(!r->active||r->destroyRequested)return true;
        executing_=true;struct Reset {bool& flag;~Reset(){flag=false;}} reset{executing_};
        Publisher publish(*this,key);bool okay=true;
        return (execute(r->state,publish,okay)&&okay&&good_&&r->state.valid&&
            r->x.bits==r->state.x.bits&&r->y.bits==r->state.y.bits&&r->destroyRequested==r->state.destroyRequested&&
            r->spriteId==r->state.spriteId&&r->imageXscale.bits==r->state.imageXscale.bits)||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();if(!r->active||r->destroyRequested)return true;
        auto next=r->state;
        if(!next.advanceImage()||!binding_.imageIndex(key,*r,next.imageIndex))return fail();
        r->state.timeline=next.timeline;r->state.imageIndex=next.imageIndex;return true;
    }
    template<class Create,class Sprite> bool advanceAlarm(InstanceHandle key,unsigned index,Create create,Sprite sprite){
        return dispatchPublished(key,[&](auto& state,auto& publish,bool& okay){
            struct Events {
                Create& createChild;Sprite& setSprite;Publisher& publish;bool& okay;
                bool create(int object,RunnerImageValue x,RunnerImageValue y,std::uint32_t& child){return createChild(object,x.promoted(),y.promoted(),child);}
                bool sprite(std::uint32_t child,int value){return setSprite(child,value);}
                void destroy(){okay=publish.destroy()&&okay;}
            } events{create,sprite,publish,okay};
            return state.advanceAlarm(index,events);
        });
    }
    bool endStep(InstanceHandle key,MotionBinary64 top,bool mainRoom){
        return dispatchPublished(key,[&](auto& state,auto& publish,bool& okay){
            struct Events {Publisher& publish;bool& okay;void destroy(){okay=publish.destroy()&&okay;}} events{publish,okay};
            return state.endStep(mainRoom,top,events);
        });
    }
    bool writeShootableHealth(InstanceHandle key,MotionBinary64 health){
        auto* r=resolve(key);if(!r||!health.valid())return fail();r->state.health=health;return true;
    }
    bool writeShootableHit(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();r->state.hit=true;return true;
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||r->destroyRequested||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);if(!r||executing_||!r->destroyRequested||!r->state.destroyRequested||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};std::uint32_t next_=1;unsigned count_=0;
    bool executing_=false;FailureSignal<> good_=true;
};
}
#endif
