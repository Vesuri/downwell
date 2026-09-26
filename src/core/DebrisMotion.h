#ifndef DOWNWELL_CORE_DEBRIS_MOTION_H
#define DOWNWELL_CORE_DEBRIS_MOTION_H
#include "DebrisBirth.h"
#include "MultiplyBinary64.h"
namespace downwell {
struct DebrisMotion : DebrisBirth {
    RunnerImageValue x{},y{};
    bool destroyRequested=false;
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,int area,Random& random){
        DebrisMotion next;
        if(!next.x.assign(px)||!next.y.assign(py)||!next.DebrisBirth::create(area,random))return false;
        *this=next;return true;
    }
    void advanceAlarm(unsigned index){
        if(destroyRequested||index>2||!alarms.advanceOne(index))return;
        if(index==0)destroyRequested=true;
        else if(index==1)disappearing=true;
        else noTouch=false;
    }
    // stopped is the original TimeStopBound decision. The owner supplies the
    // precise sParentSolid (56) query and must stop on failure, not treat it as
    // a miss. Coordinate writes are individually narrowed before later probes.
    template<class Random,class Meeting> bool step(bool stopped,Random& random,Meeting meeting){
        if(destroyRequested)return true;
        if(stopped){
            for(unsigned i=0;i<3;++i){
                const auto n=alarms.get(i);
                if(n==0x7fffffff)return false;
                if(n>0)alarms.set(i,n+1);
            }
            imageSpeed={};return true;
        }
        if(!xsp.valid()||!ysp.valid()||!imageSpeed.assign(imageRate))return false;
        if(!ysp.add(less(ysp,MotionBinary64::fromInteger(-1))?hardGravity:gravity))return false;
        auto probe=x.promoted();bool hit=false;
        if(!probe.add(xsp)||!meeting(probe,y.promoted(),hit))return false;
        if(hit){xsp.high^=0x80000000u;if(!scale(xsp,{0x3fe66666u,0x66666666u}))return false;}
        probe=y.promoted();
        if(!probe.add(ysp)||!meeting(x.promoted(),probe,hit))return false;
        if(hit){
            if(ysp.zero()||(ysp.high&0x80000000u)){
                if(!scale(ysp,{}))return false;
            }else ysp=MotionBinary64::fromInteger(int(random.irandomSmall(0)));
            if(!scale(xsp,{0x3fe66666u,0x66666666u}))return false;
            const MotionBinary64 rates[]={{0xbfd33333u,0x33333333u},{0xbfb99999u,0x9999999au},{},
                                          {0x3fb99999u,0x9999999au},{0x3fd33333u,0x33333333u}};
            if(!imageSpeed.assign(rates[random.chooseIndexSmall(5)]))return false;
        }
        auto magnitude=xsp;magnitude.high&=0x7fffffffu;
        if(less(magnitude,{0x3fa99999u,0x9999999au}))
            xsp=xsp.zero()?MotionBinary64{}:MotionBinary64{0x3fa99999u|(xsp.high&0x80000000u),0x9999999au};
        probe=x.promoted();
        if(!probe.add(xsp)||!x.assign(probe))return false;
        probe=y.promoted();
        if(!probe.add(ysp)||!y.assign(probe)||!meeting(x.promoted(),y.promoted(),hit))return false;
        if(hit){
            probe=x.promoted();
            if(!probe.add(MotionBinary64::fromInteger(8))||!meeting(probe,y.promoted(),hit))return false;
            if(!hit){if(!x.assign(probe))return false;}
            else{
                probe=x.promoted();
                if(!probe.add(MotionBinary64::fromInteger(-8))||!meeting(probe,y.promoted(),hit))return false;
                if(!hit&&!x.assign(probe))return false;
            }
            probe=y.promoted();
            if(!probe.add(MotionBinary64::fromInteger(-8))||!meeting(x.promoted(),probe,hit))return false;
            if(!hit){if(!y.assign(probe))return false;}
            else{
                probe=y.promoted();
                if(!probe.add(MotionBinary64::fromInteger(8))||!meeting(x.promoted(),probe,hit))return false;
                if(!hit&&!y.assign(probe))return false;
            }
            if(!meeting(x.promoted(),y.promoted(),hit))return false;
            if(hit)destroyRequested=true;
        }
        // A destroy request does not abort the remaining instructions.
        flash=disappearing?-flash:-1;
        return true;
    }
private:
    static bool less(MotionBinary64 a,MotionBinary64 b){
        if(a.zero()&&b.zero())return false;
        const bool an=a.high>>31,bn=b.high>>31;if(an!=bn)return an;
        const bool magnitude=(a.high&0x7fffffffu)<(b.high&0x7fffffffu)||
            ((a.high&0x7fffffffu)==(b.high&0x7fffffffu)&&a.low<b.low);
        return an?(!magnitude&&(a.high!=b.high||a.low!=b.low)):magnitude;
    }
    static bool scale(MotionBinary64& value,MotionBinary64 factor){
        MotionBinary64 result;if(!multiplyBinary64(value,factor,result))return false;value=result;return true;
    }
};
}
#endif
