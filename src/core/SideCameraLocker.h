#ifndef DOWNWELL_CORE_SIDE_CAMERA_LOCKER_H
#define DOWNWELL_CORE_SIDE_CAMERA_LOCKER_H
#include "MainCameraMotion.h"
#include "StateDigest.h"
#include "InstanceLifecycle.h"
namespace downwell {
// Object 62 Create and the default unscaled rectangle masks: camera 583
// (160x284, origin 80,142), locker 596 (100x160, origin 50,80).
// Coordinates here are published integer instance positions, not Q12 easing.
struct SideCameraLocker {
    int x, y;
    unsigned lifetimeId=0,sourceId=0;
    bool active=true;
    static int createdX(int x, int roomWidth = 480) {
        return x < 160 ? 80 : x > roomWidth - 160 ? roomWidth - 80 : x;
    }
    bool overlaps(int cameraX, int cameraY) const {
        return cameraX + 79 >= x - 50 && cameraX - 80 <= x + 49 &&
               cameraY + 141 >= y - 80 && cameraY - 142 <= y + 79;
    }
};
// Room-owned, creation-ordered registrations. Fail capacity explicitly rather
// than dropping camera behavior. No per-frame allocation or pixel conversion.
struct SideCameraLockers {
    static constexpr unsigned capacity = 128;
    SideCameraLocker entries[capacity]{};
    unsigned count = 0;
    using Binding=RecordLifecycleBinding<SideCameraLocker>;
    bool bindLifecycle(Binding binding){if(count||!good_)return false;binding_=binding;return true;}
    Binding lifecycle()const{return binding_;}
    unsigned birthSequence()const{return next_;}
    bool restoreEmptyBirthSequence(unsigned next){
        if(count||!good_||!next||next<next_)return false;next_=next;return true;
    }
    InstanceHandle handle(unsigned i)const{return i>=retired_&&i<count?InstanceHandle{i,entries[i].lifetimeId}:InstanceHandle{};}
    const SideCameraLocker* find(InstanceHandle h)const{
        return h.generation&&h.slot>=retired_&&h.slot<count&&entries[h.slot].lifetimeId==h.generation?&entries[h.slot]:nullptr;
    }
    bool clear() {
        if(!good_)return false;
        while(retired_<count){
            if(!binding_.retire(handle(retired_),entries[retired_]))return good_=false;
            ++retired_;
        }
        count=retired_=0;return true;
    }
    bool create(int x, int y,unsigned sourceId=0) {
        if (!good_||count == capacity||!next_) return false;
        if(sourceId>0x7fffffffu)return good_=false;
        for(unsigned i=retired_;sourceId&&i<count;++i)
            if(entries[i].sourceId==sourceId)return good_=false;
        const auto i=count++;entries[i]={x,y,next_++,sourceId,true};
        if(!binding_.birth(handle(i),entries[i]))return good_=false;
        if(x<160||x>320){
            entries[i].x=SideCameraLocker::createdX(x);
            RunnerImageValue px,py;
            if(!px.assignInteger(entries[i].x)||!py.assignInteger(y)||
               (binding_.hasOwner()&&!binding_.publishedPosition(handle(i),entries[i],px,py)))return good_=false;
        }
        return true;
    }
    bool overlaps(const MainCameraMotion& camera) const {
        for (unsigned i = 0; i < count; ++i)
            if (entries[i].overlaps(camera.pixelX(), camera.pixelY())) return true;
        return false;
    }
    void collide(MainCameraMotion& camera) const {
        for (unsigned i = 0; i < count; ++i)
            if (entries[i].overlaps(camera.pixelX(), camera.pixelY()))
                camera.collideSideLocker(entries[i].y);
    }
    std::uint64_t digest(const MainCameraMotion& camera) const {
        StateDigest state;
        state.addBool(camera.sideLocked);
        state.addI32(camera.x); state.addI32(camera.y);
        state.addI32(camera.targetX); state.addI32(camera.targetY);
        state.addI32(camera.slowdownX); state.addI32(camera.slowdownY);
        state.addU32(count);
        for (unsigned i = 0; i < count; ++i) {
            state.addI32(entries[i].x); state.addI32(entries[i].y);
        }
        return state.value();
    }
private:
    Binding binding_{};
    unsigned next_=1,retired_=0;
    bool good_=true;
};
}
#endif
