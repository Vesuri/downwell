#ifndef DOWNWELL_CORE_CURRENCY_INSTANCES_H
#define DOWNWELL_CORE_CURRENCY_INSTANCES_H
#include "CurrencyCreate.h"
#include "InstanceLifecycle.h"
#include "SequenceTimeline.h"
#include "CurrencyAlarmEvents.h"
#include "CurrencyStepEnvelope.h"
#include "CurrencyFreeMotion.h"
#include "CurrencyHomingSource.h"
#include "TimeStopBound.h"
#include "PlayerSolidCollisionBinary64.h"
#include "SoundPlayScript.h"
#include "RunnerSpriteChange.h"
namespace downwell {
// Native currency lifetimes. Numeric source IDs are supplied by the room birth
// stream; never manufacture them from family-local generation handles.
template<unsigned Capacity> class CurrencyInstances {
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        int spriteId=133,maskId=-1;
        RunnerImageValue x{},y{},imageIndex{};
        CurrencyCreate state;
        SoundPlayState sound;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit CurrencyInstances(Binding binding):binding_(binding){}
    CurrencyInstances(const CurrencyInstances&)=delete;
    CurrencyInstances& operator=(const CurrencyInstances&)=delete;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle h)const{
        return h.generation&&h.slot<Capacity&&records_[h.slot].lifetimeId==h.generation?&records_[h.slot]:nullptr;
    }
    template<class Random,class Queries> bool create(unsigned sourceId,MotionBinary64 x,MotionBinary64 y,
        int area,bool attractor,Random& random,Queries& queries,InstanceHandle& output){
        if(!good_)return false;
        if(!binding_.hasOwner()||!sourceId||sourceId>0x7fffffffu||!next_||executing_)return fail();
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i){
            if(records_[i].lifetimeId&&records_[i].sourceId==sourceId)return fail();
            if(!records_[i].lifetimeId&&slot==Capacity)slot=i;
        }
        if(slot==Capacity)return fail();
        Record birth;if(!birth.x.assign(x)||!birth.y.assign(y))return fail();
        birth.state.x=birth.x;birth.state.y=birth.y;birth.state.imageSpeed={0x3f800000u};
        birth.sourceId=sourceId;birth.lifetimeId=next_++;
        records_[slot]=birth;++count_;const InstanceHandle key{slot,birth.lifetimeId};
        auto& r=records_[slot];executing_=true;
        struct Adapter {
            CurrencyInstances& owner;InstanceHandle key;Record& r;Queries& queries;
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){return queries.wall(key,x,y,hit);}
            bool tile(MotionBinary64 x,MotionBinary64 y,bool& hit){return queries.tile(key,100,x,y,hit);}
            bool publish(RunnerImageValue x,RunnerImageValue y){
                if(!owner.binding_.publishedPosition(key,r,x,y))return false;
                r.x=x;r.y=y;return true;
            }
        } adapter{*this,key,r,queries};
        const bool ok=binding_.birthAt(key,r,r.x,r.y)&&r.state.create(x,y,area,attractor,random,adapter);
        executing_=false;
        if(!ok||!good_)return fail();
        output=key;return true;
    }
    template<class Event> bool dispatch(InstanceHandle key,Event event){
        auto* r=resolve(key);if(!r||executing_)return fail();executing_=true;
        const bool ok=event(r->state,good_);executing_=false;return good_=good_&&ok;
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=resolve(key);if(!r||executing_||(r->spriteId!=133&&r->spriteId!=134))return fail();
        if(!r->active||r->destroyRequested)return true;
        auto next=r->imageIndex;bool crossed=false;
        if(!SequenceTimeline::advance(next,r->state.imageSpeed,4,crossed))return fail();
        next.bits&=0x7fffffffu;
        if(!binding_.imageIndex(key,*r,next))return fail();
        r->imageIndex=next;return true;
    }
    template<class DrawFx> bool draw(InstanceHandle key,DrawFx drawFx){
        auto* r=resolve(key);if(!r||executing_)return fail();
        if(!r->active||r->destroyRequested)return true;
        const int size=r->state.size;
        if(size==0||size==1){
            const int sprite=size==0?133:134;RunnerSpriteChange::Result change;
            if(!RunnerSpriteChange::plan(r->spriteId,sprite,r->imageIndex,4,change)||
                !binding_.sprite(key,*r,sprite))return fail();
            r->spriteId=sprite;if(change.resetIndex)r->imageIndex={};
            if(r->maskId!=sprite){if(!binding_.mask(key,*r,sprite))return fail();r->maskId=sprite;}
        }
        if(r->state.flash>0)return true;
        return good_=drawFx(key,static_cast<const Record&>(*r))&&good_;
    }
    template<class Other> bool advanceAlarm(InstanceHandle key,unsigned index,Other other){
        auto* r=resolve(key);if(!r||executing_||index>=5)return fail();
        if(!r->active||r->destroyRequested||!r->state.alarms.advanceOne(index))return true;
        return dispatch(key,[&](auto& state,bool& good){return CurrencyAlarmEvents::run(state,index,other,good);});
    }
    template<class Audio> bool expire(InstanceHandle key,MotionBinary64 volume,bool inWater,Audio& audio){
        auto* r=resolve(key);if(!r)return fail();
        if(r->state.attracted)return true;
        SoundPlayEnvironment env;env.gameVolume=volume;env.inWater=inWater;int voice=-1;
        if(!SoundPlayScript::plain(r->sound,env,audio,166,30,false,true,voice,good_))return false;
        return markDestroyed(key);
    }
    template<class Random,class Queries,class Math,class View,class Audio>
    bool stepSource(InstanceHandle key,Random& random,bool stopped,int width,
        MotionBinary64 playerX,MotionBinary64 playerY,Queries& queries,Math& math,View view,
        Audio& audio,MotionBinary64 volume,bool inWater){
        auto* r=resolve(key);if(!r||executing_)return fail();
        if(!r->active||r->destroyRequested)return true;
        struct Ops {
            CurrencyInstances& owner;InstanceHandle key;Record& r;Random& random;
            bool stopped;int width;MotionBinary64 playerX,playerY;
            Queries& queries;Math& math;View& view;Audio& audio;MotionBinary64 volume;bool inWater;
            bool meeting(int object,MotionBinary64 x,MotionBinary64 y,bool& hit){return queries.meeting(key,object,x,y,hit);}
            bool wall(MotionBinary64 x,MotionBinary64 y,bool& hit){return meeting(57,x,y,hit);}
            bool timeAllowed(bool& allowed){return TimeStopBound::evaluate(stopped,r.state.x.promoted(),r.state.y.promoted(),width,
                [&](auto x,auto y,bool& hit){return meeting(82,x,y,hit);},allowed);}
            bool destroy(){return owner.markDestroyed(key);}
            bool collision(int object,CurrencyCreate& state){
                state.checkTarget=object;SolidCollisionResult result{};
                const bool ok=PlayerSolidCollisionBinary64::resolve(state.xx,state.yy,state.xsp,state.ysp,
                    [&](auto x,auto y,bool& hit){return meeting(object,x,y,hit);},result);
                state.xCollision=result.x;state.yCollision=result.y;return ok;
            }
            bool viewTop(MotionBinary64& top){return view(top);}
            bool pointDirection(MotionBinary64 ax,MotionBinary64 ay,MotionBinary64 bx,MotionBinary64 by,MotionBinary64& out){
                return math.pointDirection(ax,ay,bx,by,out);
            }
            bool sound(int id,int priority,bool loop,bool cut,int& voice){
                SoundPlayEnvironment env;env.gameVolume=volume;env.inWater=inWater;
                return SoundPlayScript::plain(r.sound,env,audio,id,priority,loop,cut,voice,owner.good_);
            }
            bool water(MotionBinary64 x,MotionBinary64 y,bool& hit){return queries.water(key,x,y,hit);}
            bool freeMotion(CurrencyCreate& state){return CurrencyFreeMotion::run(state,playerX,playerY,random,*this,owner.good_);}
            bool attractedMotion(CurrencyCreate& state){return CurrencyHomingSource::run(state,playerX,playerY,math,owner.good_);}
            bool publish(RunnerImageValue x,RunnerImageValue y){
                if(!owner.binding_.publishedPosition(key,r,x,y))return false;r.x=x;r.y=y;return true;
            }
        } ops{*this,key,*r,random,stopped,width,playerX,playerY,queries,math,view,audio,volume,inWater};
        return dispatch(key,[&](auto& state,bool& good){return CurrencyStepEnvelope::run(state,random,ops,good);});
    }
    bool markDestroyed(InstanceHandle key){
        auto* r=resolve(key);if(!r)return fail();
        if(r->destroyRequested)return true;
        if(!binding_.markDestroyed(key,*r))return fail();r->destroyRequested=true;return true;
    }
    bool retire(InstanceHandle key){
        auto* r=resolve(key);
        if(!r||executing_||!r->destroyRequested||!binding_.retire(key,*r))return fail();
        *r={};--count_;return true;
    }
    bool clear(){
        if(executing_)return fail();
        for(unsigned i=0;i<Capacity;++i)if(records_[i].lifetimeId){
            if(!binding_.retire({i,records_[i].lifetimeId},records_[i]))return fail();records_[i]={};--count_;
        }
        good_=true;return true;
    }
private:
    Record* resolve(InstanceHandle h){return good_?const_cast<Record*>(find(h)):nullptr;}
    bool fail(){good_=false;return false;}
    Record records_[Capacity]{};unsigned next_=1,count_=0;bool good_=true,executing_=false;Binding binding_;
};
}
#endif
