#ifndef DOWNWELL_CORE_GAMEPLAY_PLAYER_CONTROLLERS_H
#define DOWNWELL_CORE_GAMEPLAY_PLAYER_CONTROLLERS_H
#include "InstanceLifecycle.h"
#include "CameraCreateState.h"
#include "PlayerCompanionCreate.h"
namespace downwell {
// Registered player-created meter, camera, balloon and up to two Drones. Source IDs are supplied
// by the enclosing synchronous creation stream, never by this local pool.
// Birth owns lifetime/publication. Camera Create is an explicit checked phase;
// it must finish before the enclosing player publishes its myCam reference.
class GameplayPlayerControllers {
public:
    static constexpr unsigned capacity=5;
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        int objectId=-1;
        RunnerImageValue x{},y{};
        CameraCreateState camera;
        PlayerCompanionCreateState companion;
        RunnerImageValue imageIndex{},imageSpeed{};
        AlarmBank alarms;
        bool companionInitialized=false;
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    explicit GameplayPlayerControllers(Binding binding={}):binding_(binding){}
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Record* find(InstanceHandle key)const{
        return key.generation&&key.slot<capacity&&records_[key.slot].lifetimeId==key.generation?&records_[key.slot]:nullptr;
    }
    InstanceHandle handle(unsigned slot)const{return slot<capacity?InstanceHandle{slot,records_[slot].lifetimeId}:InstanceHandle{};}
    InstanceHandle sourceHandle(unsigned id)const{
        for(unsigned i=0;id&&i<capacity;++i)if(records_[i].lifetimeId&&records_[i].sourceId==id)return handle(i);
        return {};
    }
    bool cameraModes(InstanceHandle key,bool freeCam,bool clearAutoScroll){
        auto* r=const_cast<Record*>(find(key));
        if(!good_||!r||r->objectId!=64||!r->camera.initialized)return fail();
        if(freeCam)r->camera.freeCam=1;
        if(clearAutoScroll)r->camera.autoScroll=0;
        return true;
    }
    bool create(int object,unsigned source,RunnerImageValue x,RunnerImageValue y,InstanceHandle& result){
        result={};
        if(!good_||!next_||count_==capacity||(object!=64&&object!=66&&object!=249&&object!=256)||!source||source>0x7fffffffu)return fail();
        unsigned siblings=0;
        for(const auto& r:records_)if(r.lifetimeId){
            if(r.sourceId==source)return fail();
            if(r.objectId==object)++siblings;
        }
        if(siblings>=(object==249?2u:1u))return fail();
        unsigned slot=0;while(records_[slot].lifetimeId)++slot;
        auto& r=records_[slot];r={};r.lifetimeId=next_++;r.sourceId=source;r.objectId=object;r.x=x;r.y=y;++count_;
        if(!binding_.birth(handle(slot),r))return fail();
        result=handle(slot);return true;
    }
    bool position(InstanceHandle key,RunnerImageValue x,RunnerImageValue y){
        auto* r=const_cast<Record*>(find(key));
        if(!good_||!r||!binding_.publishedPosition(key,*r,x,y))return fail();
        r->x=x;r->y=y;return true;
    }
    bool initializeCamera(InstanceHandle key,int width,bool ground,RunnerImageValue playerX,RunnerImageValue playerY){
        auto* r=const_cast<Record*>(find(key));
        if(!good_||!r||r->objectId!=64)return fail();
        if(!r->camera.create(width,ground,r->x,r->y,playerX,playerY,
            [&](bool vertical,RunnerImageValue value,RunnerImageValue& accepted){
                const auto old=vertical?r->y:r->x;
                // Numeric equality preserves the old public zero sign.
                if(!((value.bits|old.bits)&0x7fffffffu))value=old;
                if(!position(key,vertical?r->x:value,vertical?value:r->y))return false;
                accepted=vertical?r->y:r->x;return true;
            }))return fail();
        return true;
    }
    bool createCamera(unsigned source,RunnerImageValue x,RunnerImageValue y,int width,bool ground,
        RunnerImageValue playerX,RunnerImageValue playerY,InstanceHandle& result){
        result={};InstanceHandle born;
        if(!create(64,source,x,y,born)||!initializeCamera(born,width,ground,playerX,playerY))return false;
        result=born;return true;
    }
    bool createCompanion(int object,unsigned source,RunnerImageValue x,RunnerImageValue y,InstanceHandle& result){
        result={};InstanceHandle born;
        if(object!=249&&object!=256)return fail();
        if(!create(object,source,x,y,born))return false;
        auto& r=records_[born.slot];r.companion.sourceId=source;
        if(object==249)r.imageSpeed={0x3f000000u};
        else {
            if(!r.imageSpeed.assign(r.companion.balloon.imgSp)||!binding_.imageIndex(born,r,{}))return fail();
            r.imageIndex={};r.companion.balloon.xx=r.x.promoted();r.companion.balloon.yy=r.y.promoted();
        }
        r.companionInitialized=true;result=born;return true;
    }
    bool reverseDrone(InstanceHandle key){
        auto* r=const_cast<Record*>(find(key));
        if(!good_||!r||r->objectId!=249||!r->companionInitialized)return fail();
        r->companion.drone.xdir=r->companion.drone.ydir=MotionBinary64::fromInteger(180);return true;
    }
    bool advanceAnimation(InstanceHandle key){
        auto* r=const_cast<Record*>(find(key));if(!good_||!r)return fail();
        if(r->objectId!=249&&r->objectId!=256)return true;
        if(!r->companionInitialized)return fail();
        return r->companion.advanceImage(r->objectId,r->objectId==249?131:526,r->imageIndex,r->imageSpeed,
            [&](RunnerImageValue value){
                if(!binding_.imageIndex(key,*r,value))return false;
                r->imageIndex=value;return true;
            })||fail();
    }
    bool setAlarm(InstanceHandle key,unsigned number,std::int32_t value){
        auto* r=const_cast<Record*>(find(key));if(!good_||!r||number>=AlarmBank::count)return fail();
        r->alarms.set(number,value);return true;
    }
    bool advanceAlarm(InstanceHandle key,unsigned number){
        auto* r=const_cast<Record*>(find(key));if(!good_||!r||number>=AlarmBank::count)return fail();
        if(!r->alarms.advanceOne(number))return true;
        if(r->objectId==64&&number==0){r->camera.camShake=r->camera.camShakeAmt=0;return true;}
        return (r->companionInitialized&&r->companion.alarm(r->objectId,number))||fail();
    }
    bool activation(InstanceHandle key,bool active){
        auto* r=const_cast<Record*>(find(key));
        if(!good_||!r||!binding_.activation(key,*r,active))return fail();
        r->active=active;return true;
    }
    bool clear(){
        if(!good_)return false;
        for(unsigned slot=0;slot<capacity;++slot){auto& r=records_[slot];
            if(!r.lifetimeId)continue;
            if(!binding_.retire(handle(slot),r))return fail();
            r={};--count_;
        }
        return true;
    }
private:
    bool fail(){good_=false;return false;}
    Record records_[capacity]{};Binding binding_{};unsigned count_=0,next_=1;bool good_=true;
};
}
#endif
