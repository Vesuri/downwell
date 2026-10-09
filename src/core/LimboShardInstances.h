#ifndef DOWNWELL_CORE_LIMBO_SHARD_INSTANCES_H
#define DOWNWELL_CORE_LIMBO_SHARD_INSTANCES_H
#include "LimboShard.h"
#include "EffectSequenceAnimation.h"
#include "InstanceLifecycle.h"
#include "FailureSignal.h"
#include "AlarmBank.h"
namespace downwell {
// Native fixed-address lifetimes. The enclosing owner supplies source IDs,
// ordered queries and view reads; this store never invents VM IDs or order.
template<unsigned Capacity> class LimboShardInstances {
    static_assert(Capacity>0,"nonempty limbo capacity required");
public:
    struct Record {
        std::uint32_t lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{},timeline{};
        LimboShard state;AlarmBank alarms;
        bool active=true,destroyRequested=false;
        static constexpr int objectId=379;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit LimboShardInstances(Binding binding):binding_(binding){}
    LimboShardInstances(const LimboShardInstances&)=delete;
    LimboShardInstances& operator=(const LimboShardInstances&)=delete;
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
private:
    struct Dispatch {LimboShardInstances& o;explicit Dispatch(LimboShardInstances& v):o(v){o.dispatching_=true;}~Dispatch(){o.dispatching_=false;}};
    struct Publish {
        LimboShardInstances& owner;InstanceHandle key;Record& record;
        bool speed(RunnerImageValue){return true;} // state is the published speed owner.
        bool image(RunnerImageValue v){return owner.binding_.imageIndex(key,record,v)||owner.fail();}
        bool positionX(RunnerImageValue x){return position(x,record.y);}
        bool positionY(RunnerImageValue y){return position(record.x,y);}
        bool position(RunnerImageValue x,RunnerImageValue y){
            if(!owner.binding_.publishedPosition(key,record,x,y))return owner.fail();
            record.x=x;record.y=y;return true;
        }
        bool alarm(unsigned index,int ticks){record.alarms.set(index,ticks);return true;}
        bool destroy(){
            if(!owner.binding_.markDestroyed(key,record))return owner.fail();
            record.destroyRequested=true;return true;
        }
    };
public:
    template<class Random> bool create(std::uint32_t id,MotionBinary64 x,MotionBinary64 y,bool lowSpec,
        Random& random,InstanceHandle& output){
        output={};if(!good_||dispatching_||!next_||!id||id>0x7fffffffu||sourceHandle(id).generation)return fail();
        unsigned slot=0;while(slot<Capacity&&records_[slot].lifetimeId)++slot;
        RunnerImageValue px,py;if(slot==Capacity||!px.assign(x)||!py.assign(y))return fail();
        Dispatch guard(*this);
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=id;r.x=px;r.y=py;
        r.state.x=px;r.state.y=py;r.state.imageSpeed={0x3f800000u};++count_;
        const InstanceHandle key{slot,r.lifetimeId};Publish publish{*this,key,r};
        if(!binding_.birthAt(key,r,px,py)||!r.state.create(x,y,lowSpec,random,publish)||!good_)return fail();
        output=key;return true;
    }
    template<class Random,class Queries> bool step(InstanceHandle key,bool allowed,const LimboShard::Player& player,
        Random& random,Queries& queries){
        auto* r=resolve(key);if(!r||dispatching_)return fail();if(!r->active||r->destroyRequested)return true;
        Dispatch guard(*this);
        struct Environment:Publish {
            Queries& q;
            Environment(LimboShardInstances& o,InstanceHandle k,Record& r,Queries& queries):Publish{o,k,r},q(queries){}
            bool meeting(MotionBinary64 x,MotionBinary64 y,int object,bool& hit){return q.meeting(this->key,x,y,object,hit);}
            bool population(unsigned& count){return q.population(count);}
            bool outside(RunnerImageValue x,RunnerImageValue y,bool& result){return q.outside(x,y,result);}
            bool viewTop(MotionBinary64& y){return q.viewTop(y);}
        } env{*this,key,*r,queries};
        r->state.alarm2=r->alarms.get(2);
        return (r->state.step(allowed,player,random,env)&&good_)||fail();
    }
    template<class Random> bool bulletContact(InstanceHandle key,MotionBinary64 xsp,MotionBinary64 ysp,Random& random){
        auto* r=resolve(key);if(!r||dispatching_)return fail();if(!r->active||r->destroyRequested)return true;
        Dispatch guard(*this);Publish publish{*this,key,*r};r->state.alarm2=r->alarms.get(2);
        return (r->state.bulletContact(xsp,ysp,random,publish)&&good_)||fail();
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r||dispatching_)return fail();if(!r->active||r->destroyRequested)return true;
        Dispatch guard(*this);Publish publish{*this,key,*r};
        return (EffectSequenceAnimation::advance(r->timeline,r->state.imageSpeed,6,r->state.imageIndex)&&
            publish.image(r->state.imageIndex)&&good_)||fail();
    }
    bool activate(InstanceHandle key,bool active){
        auto* r=resolve(key);if(!r||dispatching_||r->destroyRequested)return fail();Dispatch guard(*this);
        if(!binding_.activation(key,*r,active)||!good_)return fail();
        r->active=active;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);if(!r||dispatching_||!r->destroyRequested||!r->state.destroyRequested)return fail();
        Dispatch guard(*this);if(!binding_.retire(key,*r)||!good_)return fail();*r={};--count_;return true;
    }
    bool clear(){
        if(!good_||dispatching_)return fail();
        Dispatch guard(*this);
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i])||!good_)return fail();
            records_[i]={};--count_;
        }
        return true;
    }
private:
    Record* resolve(InstanceHandle key){return good_?const_cast<Record*>(find(key)):nullptr;}
    bool fail(){good_=false;return false;}
    Binding binding_;Record records_[Capacity]{};std::uint32_t next_=1;unsigned count_=0;FailureSignal<> good_=true;bool dispatching_=false;
};
}
#endif
