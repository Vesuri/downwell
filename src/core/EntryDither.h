#ifndef DOWNWELL_CORE_ENTRY_DITHER_H
#define DOWNWELL_CORE_ENTRY_DITHER_H
#include "AlarmBank.h"
#include "SequenceTimeline.h"
namespace downwell {
// objEnterDither (73), not objTransDither (74). Coordinates are built-in
// binary32 fields; the y+16 expression is evaluated as a VM real before the
// setter narrows it. The room owner controls eligibility and final retirement.
struct EntryDither {
    static constexpr int objectId=73, spriteId=129;
    RunnerImageValue x{},y{};
    int alarm0=-1;
    bool destroyRequested=false;
private:
    bool good_=true;
    bool fail(){good_=false;return false;}
public:
    bool succeeded()const{return good_;}
    // Invoke reads in source order, rather than evaluating both before Create.
    // In particular, view Y is not derived from the newly created camera Y.
    template<class CameraX,class ViewY>
    bool create(CameraX cameraX,ViewY viewY){
        if(!good_||destroyRequested)return false;
        MotionBinary64 value;
        if(!cameraX(value)||!x.assign(value))return fail();
        if(!viewY(value)||!y.assign(value))return fail();
        alarm0=120;return true;
    }
    template<class CameraX> bool step(CameraX cameraX){
        if(!good_||destroyRequested)return false;
        if(!SequenceTimeline::valid(y))return fail();
        auto next=y.promoted();
        if(!next.add(MotionBinary64::fromInteger(16))||!y.assign(next))return fail();
        MotionBinary64 value;
        if(!cameraX(value)||!x.assign(value))return fail();
        return true;
    }
    // Global alarm dispatch has already decremented the authoritative counter.
    // Keep destruction distinct from deletion: no record or ID is recycled here.
    bool fireAlarm(unsigned index){
        if(!good_||destroyRequested)return false;
        if(index==0)destroyRequested=true;
        return true;
    }
    bool advanceAlarm(){
        if(!good_||destroyRequested)return false;
        return !AlarmBank::advanceValue(alarm0)||fireAlarm(0);
    }
};
}
#endif
