#ifndef DOWNWELL_CORE_GAMEPLAY_DITHER_ABOVE_H
#define DOWNWELL_CORE_GAMEPLAY_DITHER_ABOVE_H
#include "DitherAboveStep.h"
#include "InstanceLifecycle.h"
namespace downwell {
// Authored object 61, NOT the player-created entry wipe (73). The source room
// supplies the numeric ID. A generation survives room teardown so old handles
// cannot address a replacement controller.
class GameplayDitherAbove {
public:
    struct Record {
        unsigned lifetimeId=0,sourceId=0;
        RunnerImageValue x{},y{};
        bool active=true,destroyRequested=false;
    };
    using Binding=RecordLifecycleBinding<Record>;
    // Source numeric identity and published builtins, never a source pool key.
    // A zero ID denotes an absent (already retired) controller.
    struct Snapshot {
        unsigned sourceId=0;
        RunnerImageValue x{},y{};
        bool active=true,destroyRequested=false,lowSpec=false,ending=false;
    };
    explicit GameplayDitherAbove(Binding binding={}):binding_(binding){}
    bool valid()const{return good_;}
    InstanceHandle handle()const{return {0,record_.lifetimeId};}
    const Record* find(InstanceHandle h)const{
        return !h.slot&&h.generation&&h.generation==record_.lifetimeId?&record_:nullptr;
    }
    bool create(unsigned id,int x,int y){
        if(!good_||record_.lifetimeId||!next_||!id||id>0x7fffffffu)return fail();
        record_={};record_.sourceId=id;record_.lifetimeId=next_++;
        if(!record_.x.assignInteger(x)||!record_.y.assignInteger(y)||!binding_.birth(handle(),record_))return fail();
        preparePixelY();
        // Create's only script write is material="foliage"; this typed
        // actor has that material invariant and no additional RNG or setters.
        return true;
    }
    bool adopt(const Snapshot& source){
        if(!good_||record_.lifetimeId)return fail();
        if(!source.sourceId)return true;
        auto valid=[](RunnerImageValue v){
            const unsigned e=(v.bits>>23)&255u;
            return e!=255u&&(e!=0u||!(v.bits&0x7fffffu));
        };
        if(!next_||source.sourceId>0x7fffffffu||!valid(source.x)||!valid(source.y))return fail();
        record_={};record_.sourceId=source.sourceId;record_.lifetimeId=next_++;
        record_.x=source.x;record_.y=source.y;record_.active=source.active;
        if(!binding_.birth(handle(),record_))return fail();
        preparePixelY();
        // Publish destruction through the owner as well as the record. Birth
        // does not execute Create, setters, audio or RNG.
        return !source.destroyRequested||mark();
    }
    template<class CameraX,class CameraY>
    bool beginStep(CameraX cameraX,CameraY cameraY,bool lowSpec,bool ending){
        if(!good_)return false;
        if(!record_.lifetimeId||!record_.active||record_.destroyRequested)return true;
        struct Ops {
            GameplayDitherAbove& owner;CameraX& readX;CameraY& readY;bool low,end;
            bool cameraX(MotionBinary64& x){return readX(x);}
            bool cameraY(MotionBinary64& y){return readY(y);}
            bool lowSpec(bool& b){b=low;return true;}
            bool ending(bool& b){b=end;return true;}
            bool y(RunnerImageValue& y){y=owner.record_.y;return true;}
            bool setX(RunnerImageValue x){return owner.position(x,owner.record_.y);}
            bool setY(RunnerImageValue y){return owner.position(owner.record_.x,y);}
            bool destroySelf(){return owner.mark();}
        } ops{*this,cameraX,cameraY,lowSpec,ending};
        return DitherAboveStep::run(ops)||fail();
    }
    // Gameplay's camera publication is already integral. Retain the authored
    // controller in that domain rather than reconstructing binary64 arithmetic
    // every Begin Step. Imported fractional/large values keep the reference.
    // Lifecycle adapters must not mutate this store (InstanceLifecycle.h), so
    // the cache changes only at the position/lifetime publication boundaries.
    template<class CameraY>
    bool beginStepPixels(int cameraX,CameraY cameraY,bool lowSpec,bool ending){
        if(!good_)return false;
        if(!record_.lifetimeId||!record_.active||record_.destroyRequested)return true;
        if(!pixelYValid_)
            return beginStep([&](MotionBinary64& x){x=MotionBinary64::fromInteger(cameraX);return true;},
                [&](MotionBinary64& y){y=MotionBinary64::fromInteger(cameraY());return true;},lowSpec,ending);
        RunnerImageValue x;
        if(!x.assignInteger(cameraX)||!position(x,record_.y))return fail();
        // Read camera Y only after X publication, as in the source. The sum
        // fits int32 in the admitted Y range; comparing avoids subtraction
        // overflow even at either extreme of the integer camera domain.
        if(cameraY() >= pixelY_+(lowSpec?208:272)){
            RunnerImageValue y;
            if(!y.assignInteger(pixelY_+16)||!position(record_.x,y))return fail();
        }
        // round(integer) is identity except for signed zero. Keep this second
        // setter even after +=16: failure prefixes and publication are visible.
        auto rounded=record_.y;
        if(!(rounded.bits&0x7fffffffu))rounded.bits=0;
        return (position(record_.x,rounded)&&(!ending||mark()))||fail();
    }
    bool clear(){
        if(!good_)return false;
        if(record_.lifetimeId&&!binding_.retire(handle(),record_))return fail();
        record_={};pixelYValid_=false;return true;
    }
    bool retireMarked(InstanceHandle h){
        if(!good_||!find(h)||!record_.destroyRequested)return fail();
        return clear();
    }
private:
    bool fail(){good_=false;return false;}
    void preparePixelY(){
        RunnerImageValue narrowed;
        pixelYValid_=record_.y.roundPixel(pixelY_)&&pixelY_>=-(0x7fffff-16)&&pixelY_<=0x7fffff-16&&
            narrowed.assignInteger(pixelY_)&&
            (narrowed.bits==record_.y.bits||!(record_.y.bits&0x7fffffffu));
    }
    bool position(RunnerImageValue x,RunnerImageValue y){
        if(!good_||(binding_.hasOwner()&&!binding_.publishedPosition(handle(),record_,x,y)))return fail();
        const bool changed=record_.y.bits!=y.bits;
        record_.x=x;record_.y=y;
        if(changed)preparePixelY();
        return true;
    }
    bool mark(){
        if(!good_||(binding_.hasOwner()&&!binding_.markDestroyed(handle(),record_)))return fail();
        record_.destroyRequested=true;return true;
    }
    Record record_{};Binding binding_{};unsigned next_=1;bool good_=true;
    std::int32_t pixelY_=0;bool pixelYValid_=false;
};
}
#endif
