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
    enum class Write {x,y,alarm};
    struct IgnoreWrites {bool operator()(Write,const EntryDither&)const{return true;}};
    bool succeeded()const{return good_;}
    // Invoke reads in source order, rather than evaluating both before Create.
    // In particular, view Y is not derived from the newly created camera Y.
    template<class CameraX,class ViewY,class Publish=IgnoreWrites>
    bool create(CameraX cameraX,ViewY viewY,Publish publish={}){
        if(!good_||destroyRequested)return false;
        MotionBinary64 value;
        if(!cameraX(value)||!x.assign(value))return fail();
        if(!publish(Write::x,*this))return fail();
        if(!viewY(value)||!y.assign(value))return fail();
        if(!publish(Write::y,*this))return fail();
        alarm0=120;return publish(Write::alarm,*this)||fail();
    }
    template<class CameraX,class Publish=IgnoreWrites> bool step(CameraX cameraX,Publish publish={}){
        if(!good_||destroyRequested)return false;
        if(!SequenceTimeline::valid(y))return fail();
        auto next=y.promoted();
        if(!next.add(MotionBinary64::fromInteger(16))||!y.assign(next))return fail();
        if(!publish(Write::y,*this))return fail();
        MotionBinary64 value;
        if(!cameraX(value)||!x.assign(value))return fail();
        return publish(Write::x,*this)||fail();
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
