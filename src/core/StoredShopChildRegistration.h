#ifndef DOWNWELL_CORE_STORED_SHOP_CHILD_REGISTRATION_H
#define DOWNWELL_CORE_STORED_SHOP_CHILD_REGISTRATION_H
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "ObjectPack.h"
#include "MultiplyBinary64.h"
#include "RunnerRealComparison.h"
#include "ShopChildCreate.h"
#include "ShopKeeperEvents.h"
#include "ShopKeeperAnimation.h"
#include "RunnerSpriteChange.h"
#include "ShopMenuEvents.h"
#include <cstring>
namespace downwell {
// Synchronous Create ownership for Shop's ten children. This is deliberately
// separate from ShopFixtures (drawing) and ShopFeedback (compact gameplay).
// The enclosing room supplies the one numeric-ID stream and lifecycle owner.
// Step/Alarm/Draw admission remains the enclosing scheduler's responsibility.
template<unsigned WorldCapacity,unsigned Capacity> class StoredShopChildRegistration {
    static_assert(Capacity>0,"shop child registration needs storage");
public:
    using State=ShopChildState;
    using Material=State::Material;
private:
    struct Entry {InstanceHandle key{};unsigned id=0;int object=-1;State state;RunnerImageValue keeperTimeline{};};
    Entry entries_[Capacity]{};
    InstanceStore<WorldCapacity>& records_;const ObjectPack& objects_;
    InstanceLifecycleBinding lifecycle_;std::uint32_t& nextId_;const int& room_;bool& good_;
    bool creating_=false;
    bool fail(){good_=false;return false;}
    Entry* entry(InstanceHandle h){
        for(auto& e:entries_)if(e.id&&e.key.slot==h.slot&&e.key.generation==h.generation)return &e;
        return nullptr;
    }
    InstanceRecord* record(const Entry& e){
        auto* r=records_.resolve(e.key);
        return r&&r->instanceId==e.id&&r->objectId==e.object?r:nullptr;
    }
    static float stored(RunnerImageValue v){float f;std::memcpy(&f,&v.bits,4);return f;}
public:
    StoredShopChildRegistration(InstanceStore<WorldCapacity>& records,const ObjectPack& objects,
        InstanceLifecycleBinding lifecycle,std::uint32_t& nextId,const int& room,bool& good)
        :records_(records),objects_(objects),lifecycle_(lifecycle),nextId_(nextId),room_(room),good_(good){}
    StoredShopChildRegistration(const StoredShopChildRegistration&)=delete;
    StoredShopChildRegistration& operator=(const StoredShopChildRegistration&)=delete;
    bool belongsTo(const InstanceStore<WorldCapacity>& records)const{return &records_==&records;}
    bool valid()const{return good_;}
    InstanceHandle handle(const InstanceRecord& r){
        const auto key=sourceHandle(r.instanceId);auto* e=entry(key);
        return e&&record(*e)==&r?key:InstanceHandle{};
    }
    InstanceHandle sourceHandle(unsigned id){
        for(auto& e:entries_)if(id&&e.id==id&&record(e))return e.key;
        return {};
    }
    const State* state(InstanceHandle h){auto* e=entry(h);return e&&record(*e)?&e->state:nullptr;}
    unsigned create(int object,int x,int y){
        return createReal(object,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y));
    }
    unsigned createReal(int object,MotionBinary64 x,MotionBinary64 y){
        if(!good_||creating_||(object!=278&&object!=279&&object!=98&&object!=280&&object!=212&&object!=283)){
            fail();return 0;
        }
        Entry* free=nullptr;for(auto& e:entries_)if(!e.id&&!free)free=&e;
        PackedObject definition;RunnerImageValue px,py;std::int32_t ix,iy;
        if(!free||!objects_.findObject(object,definition)||!nextId_||nextId_>0x7fffffffu||
           !px.assign(x)||!py.assign(y)||!px.roundPixel(ix)||!py.roundPixel(iy)){
            fail();return 0;
        }
        creating_=true;struct Reset {bool& flag;~Reset(){flag=false;}} reset{creating_};
        if(records_.create(nextId_,object,room_,definition.persistent(),definition.creationDepth)!=CreateInstanceResult::success){
            fail();return 0;
        }
        free->id=nextId_++;free->object=object;free->key=records_.handleAt(records_.size()-1);
        auto* r=record(*free);if(!r){fail();return 0;}
        r->x=ix;r->y=iy;r->preciseX=stored(px);r->preciseY=stored(py);
        r->spriteId=definition.spriteId;r->maskId=definition.textureMaskId;
        // Publish default builtins before executing Create setters. PreCreate
        // for these resources/ancestors only delegates to event_inherited.
        if(!lifecycle_.birthAt(free->key,*r,px,py)||!good_){fail();return 0;}
        struct Builtins {
            StoredShopChildRegistration& owner;Entry& e;InstanceRecord& r;
            bool speed(RunnerImageValue v){r.imageSpeed=stored(v);return true;}
            bool alarm(unsigned n,int v){r.alarms.set(n,v);return true;}
            bool index(RunnerImageValue v){
                if(!owner.lifecycle_.imageIndex(e.key,r,v)||!owner.good_)return false;
                r.imageIndex=stored(v);return true;
            }
        } builtins{*this,*free,*r};
        if(!free->state.create(object,builtins)){fail();return 0;}
        return free->id;
    }
    bool destroyPreviousText(unsigned id){
        if(!good_||creating_)return fail();
        const auto key=sourceHandle(id);auto* e=entry(key);auto* r=e?record(*e):nullptr;
        // Explicit-instance pushenv skips missing, inactive or marked targets.
        if(!r)return records_.find(id)?fail():true;
        if(!r->active||r->destroyRequested)return true;
        PackedObjectEvent event;
        if(e->object!=212||!e->state.initialized||objects_.findEvent(212,1,0,event)||
           !lifecycle_.markDestroyed(key,*r)||!good_)return fail();
        r->destroyRequested=true;return true;
    }
    bool assignText(unsigned id,const char* text){
        auto* e=entry(sourceHandle(id));
        if(!good_||creating_||!e||!record(*e)||e->object!=212||!e->state.initialized||!text)return fail();
        e->state.text=text;return true;
    }
    bool readX(unsigned id,int& x){int y;return position(id,x,y);}
    bool position(unsigned id,int& x,int& y){
        auto* e=entry(sourceHandle(id));auto* r=e?record(*e):nullptr;
        if(!good_||!r||!e->state.initialized)return fail();
        x=r->x;y=r->y;return true;
    }
    bool upgrade(unsigned id,unsigned value){
        auto* e=entry(sourceHandle(id));
        if(!good_||!e||!e->state.initialized||e->object!=283)return fail();
        e->state.whichUg=value;return true;
    }
    bool stored(unsigned id,unsigned value){
        auto* e=entry(sourceHandle(id));
        if(!good_||!e||!e->state.initialized||e->object!=279||value>0x7fffffffu)return fail();
        e->state.storedItem=int(value);return true;
    }
    bool purchaseDestroyItem(unsigned id){
        const auto key=sourceHandle(id);auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||!e->state.initialized||e->object!=283||!r->active||r->destroyRequested||
           !lifecycle_.markDestroyed(key,*r)||!good_)return fail();
        r->destroyRequested=true;return true;
    }
    bool menuKeeper(unsigned id,ShopMenuEvents::KeeperField field,int value){
        auto* e=entry(sourceHandle(id));
        if(!good_||creating_||!e||!record(*e)||!e->state.initialized||e->object!=280)return fail();
        switch(field){
            case ShopMenuEvents::KeeperField::thankful:e->state.thankful=value;break;
            case ShopMenuEvents::KeeperField::goodbye:e->state.goodbye=value;break;
            case ShopMenuEvents::KeeperField::confused:e->state.confused=value;break;
        }
        return true;
    }
    bool purchaseThankKeeper(unsigned id){
        return menuKeeper(id,ShopMenuEvents::KeeperField::thankful,1);
    }
    bool purchaseResetKeeper(unsigned id){
        const auto key=sourceHandle(id);auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||!e->state.initialized||e->object!=280||
           !lifecycle_.imageIndex(key,*r,{})||!good_)return fail();
        r->imageIndex=0;return true;
    }
    bool purchaseKeeperY(unsigned id,MotionBinary64& y){
        auto* e=entry(sourceHandle(id));auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||!e->state.initialized||e->object!=280)return fail();
        RunnerImageValue value;std::memcpy(&value.bits,&r->preciseY,4);y=value.promoted();return true;
    }
    bool stepRisingText(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=212||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        RunnerImageValue x,y;std::memcpy(&x.bits,&r->preciseX,4);std::memcpy(&y.bits,&r->preciseY,4);
        return e->state.stepRisingText(y,[&](RunnerImageValue value){
            std::int32_t iy;
            if(!value.roundPixel(iy)||!lifecycle_.publishedPosition(key,*r,x,value)||!good_)return false;
            r->preciseY=stored(value);r->y=iy;return true;
        })||fail();
    }
    template<class Lookup> bool stepItem(InstanceHandle key,Lookup lookup){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=283||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        return e->state.stepItem(r->spriteId,[&](unsigned index,int& sprite){return lookup(index,sprite)&&good_;},
            [&](int sprite){
                if(!lifecycle_.sprite(key,*r,sprite)||!good_)return false;
                r->spriteId=sprite;return true;
            })||fail();
    }
    bool advanceRisingTextAlarm(InstanceHandle key,unsigned number){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=212||!e->state.initialized||number>=AlarmBank::count)return fail();
        if(!r->active||r->destroyRequested||number>1||!r->alarms.advanceOne(number))return true;
        return risingTextAlarmEvent(key,number);
    }
    bool advanceKeeperAnimation(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=280||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        RunnerImageValue index,speed;
        std::memcpy(&index.bits,&r->imageIndex,4);std::memcpy(&speed.bits,&r->imageSpeed,4);
        return ShopKeeperAnimation::advance(e->keeperTimeline,r->spriteId,index,speed,[&](RunnerImageValue value){
            if(!lifecycle_.imageIndex(key,*r,value)||!good_)return false;
            r->imageIndex=stored(value);return true;
        })||fail();
    }
    template<class Environment> bool stepCase(InstanceHandle key,Environment& environment){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=279||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        struct Ops {
            StoredShopChildRegistration& owner;InstanceRecord& r;InstanceHandle key;Environment& env;
            bool sound(int a,int b,int c,int d){return env.sound(a,b,c,d)&&owner.good_;}
            bool image(RunnerImageValue value){
                if(!owner.lifecycle_.imageIndex(key,r,value)||!owner.good_)return false;
                r.imageIndex=stored(value);return true;
            }
            bool mask(int value){
                if(!owner.lifecycle_.mask(key,r,value)||!owner.good_)return false;
                r.maskId=value;return true;
            }
            bool flash(int a,int b,int c){
                RunnerImageValue x,y;std::memcpy(&x.bits,&r.preciseX,4);std::memcpy(&y.bits,&r.preciseY,4);
                return env.flash(x.promoted(),y.promoted(),a,b,c)&&owner.good_;
            }
            bool criminal(){return env.criminal()&&owner.good_;}
        } ops{*this,*r,key,environment};
        return e->state.caseStep(ops)||fail();
    }
    bool caseAlarmEvent(InstanceHandle key,unsigned number){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=279||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        return e->state.caseAlarm(number,[&](RunnerImageValue value){
            if(!lifecycle_.imageIndex(key,*r,value)||!good_)return false;
            r->imageIndex=stored(value);return true;
        })||fail();
    }
    bool collideCaseBullet(InstanceHandle key,int sprite){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=279||!e->state.initialized)return fail();
        e->state.caseBullet(sprite);return true;
    }
    bool keeperSprite(InstanceHandle key,int value){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=280)return fail();
        RunnerImageValue index;std::memcpy(&index.bits,&r->imageIndex,4);
        RunnerSpriteChange::Result change;
        const auto frames=ShopKeeperAnimation::frames(value);
        if(!frames||!RunnerSpriteChange::plan(r->spriteId,value,index,frames,change)||
           !lifecycle_.sprite(key,*r,value)||!good_)return fail();
        r->spriteId=value;if(change.resetIndex)r->imageIndex=0;
        if(change.resetTimeline)e->keeperTimeline={};return true;
    }
    template<class Environment,class Range> bool stepKeeper(InstanceHandle key,Environment& environment,Range range){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=280||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        struct Ops {
            StoredShopChildRegistration& owner;InstanceRecord& r;InstanceHandle key;Environment& env;Range& random;
            static MotionBinary64 real(float value){RunnerImageValue word;std::memcpy(&word.bits,&value,4);return word.promoted();}
            MotionBinary64 x(){return real(r.preciseX);}
            MotionBinary64 y(){return real(r.preciseY);}
            MotionBinary64 imageIndex(){return real(r.imageIndex);}
            MotionBinary64 imageSpeed(){return real(r.imageSpeed);}
            int spriteIndex(){return r.spriteId;}
            bool playerX(MotionBinary64& value){return env.playerX(value)&&owner.good_;}
            bool criminal(bool& value){return env.criminal(value)&&owner.good_;}
            bool area(unsigned& value){return env.area(value)&&owner.good_;}
            bool range(unsigned lo,unsigned hi,unsigned& value){value=random(lo,hi);return owner.good_;}
            bool sprite(int value){
                return owner.keeperSprite(key,value);
            }
            bool speed(MotionBinary64 value){RunnerImageValue v;if(!v.assign(value))return false;r.imageSpeed=stored(v);return true;}
            bool image(RunnerImageValue value){
                if(!owner.lifecycle_.imageIndex(key,r,value)||!owner.good_)return false;
                r.imageIndex=stored(value);return true;
            }
            bool alarm(unsigned n,int v){r.alarms.set(n,v);return true;}
            bool destroy(){
                if(!owner.lifecycle_.markDestroyed(key,r)||!owner.good_)return false;
                r.destroyRequested=true;return true;
            }
            bool createEnemy(int object,MotionBinary64 x,MotionBinary64 y){return env.createEnemy(object,x,y)&&owner.good_;}
        } ops{*this,*r,key,environment,range};
        return ShopKeeperEvents::step(e->state,ops)||fail();
    }
    // Called after the stored room dispatcher decrements the selected alarm.
    template<class Range> bool keeperAlarmEvent(InstanceHandle key,unsigned number,Range range){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=280||!e->state.initialized||number>2)return fail();
        if(!r->active||r->destroyRequested)return true;
        struct Ops {
            StoredShopChildRegistration& owner;InstanceRecord& r;InstanceHandle key;Range& random;
            bool image(RunnerImageValue value){
                if(!owner.lifecycle_.imageIndex(key,r,value)||!owner.good_)return false;
                r.imageIndex=stored(value);return true;
            }
            bool alarm(unsigned n,int v){r.alarms.set(n,v);return true;}
            bool range(unsigned lo,unsigned hi,unsigned& value){value=random(lo,hi);return owner.good_;}
        } ops{*this,*r,key,range};
        return e->state.keeperAlarm(number,r->spriteId,ops)||fail();
    }
    // The room's mixed alarm phase has already decremented this alarm.
    // Keep event execution separate from the standalone countdown adapter.
    bool risingTextAlarmEvent(InstanceHandle key,unsigned number){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=212||!e->state.initialized||number>1)return fail();
        if(!r->active||r->destroyRequested)return true;
        if(number==1){e->state.flashing=1;return true;}
        if(!lifecycle_.markDestroyed(key,*r)||!good_)return fail();
        r->destroyRequested=true;return true;
    }
    bool retire(InstanceHandle key){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||!r->destroyRequested||
           !records_.erase(e->id,[&](auto h,const auto& value){return lifecycle_.retire(h,value);}))return fail();
        *e={};return true;
    }
    // Draw operations use the current font and convert '#' to newlines for
    // width/text. The callback must execute the existing border-text script,
    // not replace its four black passes with a different outline algorithm.
    template<class Ops> bool drawRisingText(InstanceHandle key,Ops& ops){
        auto* e=entry(key);auto* r=e?record(*e):nullptr;
        if(!good_||creating_||!r||e->object!=212||!e->state.initialized)return fail();
        if(!r->active||r->destroyRequested)return true;
        auto& s=e->state;RunnerImageValue px,py;
        std::memcpy(&px.bits,&r->preciseX,4);std::memcpy(&py.bits,&r->preciseY,4);
        if(!ops.widthHashText(s.text,s.textWidth)||!good_)return fail();
        s.drawX=px.promoted();MotionBinary64 half,viewX,viewWidth;
        if(!multiplyBinary64(s.textWidth,{0x3fe00000u,0},half))return fail();
        auto over=px.promoted();
        if(!over.add(half)||!ops.view(0,viewX)||!good_||!ops.view(2,viewWidth)||!good_||!viewX.add(viewWidth))return fail();
        viewX.high^=0x80000000u;if(!over.add(viewX))return fail();s.overXview=over;
        auto under=px.promoted();half.high^=0x80000000u;
        if(!under.add(half)||!ops.view(0,viewX)||!good_)return fail();
        viewX.high^=0x80000000u;if(!under.add(viewX))return fail();s.underXview=under;
        int comparison;
        if(!compareRunnerReal(over,MotionBinary64::fromInteger(-10),comparison))return fail();
        if(comparison>0){
            over.high^=0x80000000u;
            if(!s.drawX.add(over)||!s.drawX.add(MotionBinary64::fromInteger(-10)))return fail();
        }else{
            if(!compareRunnerReal(under,MotionBinary64::fromInteger(10),comparison))return fail();
            if(comparison<0){
                under.high^=0x80000000u;
                if(!s.drawX.add(under)||!s.drawX.add(MotionBinary64::fromInteger(10)))return fail();
            }
        }
        if(!ops.align(1)||!good_)return fail();
        // drawing's source domain is integer +/-1 (or zero); the selected
        // runner's numeric conversion is positive, not C++ nonzero truth.
        if(s.drawing>0&&(!ops.borderTextBlack(s.text,s.drawX,py.promoted())||!good_))return fail();
        return (ops.align(0)&&good_)||fail();
    }
    bool clear(){
        if(creating_)return fail();
        for(auto& e:entries_)if(e.id){
            if(!record(e)||!records_.erase(e.id,[&](auto h,const auto& r){return lifecycle_.retire(h,r);}))return fail();
            e={};
        }
        return true;
    }
};
}
#endif
