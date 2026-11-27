#ifndef DOWNWELL_CORE_ENTRY_DITHER_H
#define DOWNWELL_CORE_ENTRY_DITHER_H
#include "AlarmBank.h"
#include "SequenceTimeline.h"
#include "FixedPoint.h"
namespace downwell {
// objEnterDither (73), not objTransDither (74). Coordinates are built-in
// binary32 fields: the camera and view reads deliver them directly, and
// y + 16 is evaluated in Q16 before it narrows (W3.20; exact for the
// whole-pixel views). The room owner controls eligibility and final retirement.
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
    // A read writes the binary32 field directly; the reference owners'
    // binary64 reads narrow once (until the SDL reference goes, W3.20).
    template<class Read> static auto read(Read& r,RunnerImageValue& out,int)->decltype(r(out)){return r(out);}
    template<class Read> static bool read(Read& r,RunnerImageValue& out,long){
        MotionBinary64 value;return r(value)&&out.assign(value);
    }
    // Invoke reads in source order, rather than evaluating both before Create.
    // In particular, view Y is not derived from the newly created camera Y.
    template<class CameraX,class ViewY,class Publish=IgnoreWrites>
    bool create(CameraX cameraX,ViewY viewY,Publish publish={}){
        if(!good_||destroyRequested)return false;
        if(!read(cameraX,x,0)||!SequenceTimeline::valid(x))return fail();
        if(!publish(Write::x,*this))return fail();
        if(!read(viewY,y,0)||!SequenceTimeline::valid(y))return fail();
        if(!publish(Write::y,*this))return fail();
        alarm0=120;return publish(Write::alarm,*this)||fail();
    }
    template<class CameraX,class Publish=IgnoreWrites> bool step(CameraX cameraX,Publish publish={}){
        if(!good_||destroyRequested)return false;
        if(!SequenceTimeline::valid(y))return fail();
        Fixed16 next;
        if(!Fixed16::fromBinary32(y.bits,next)||!next.addInteger(16)||!y.assignQ16(next.v))return fail();
        if(!publish(Write::y,*this))return fail();
        if(!read(cameraX,x,0)||!SequenceTimeline::valid(x))return fail();
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
