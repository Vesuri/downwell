#ifndef DOWNWELL_CORE_TIME_SHARD_LIFECYCLE_H
#define DOWNWELL_CORE_TIME_SHARD_LIFECYCLE_H
#include "TimeShardBirth.h"
#include "AlarmBank.h"
namespace downwell {
// Object 238 alarm/contact state. Motion and animation are separate phases;
// this is not a replacement for the complete ordinary Step event.
struct TimeShardLifecycle : TimeShardBirth {
    bool destroyRequested=false;
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,bool lowSpec,Random& random){
        if(!TimeShardBirth::create(px,py,lowSpec,random))return false;
        destroyRequested=false;return true;
    }
    // The scheduler visits each alarm index across all eligible instances.
    void advanceAlarm(unsigned index){
        if(destroyRequested)return;
        switch(index){
        case 0: if(AlarmBank::advanceValue(alarm0))destroyRequested=true;break;
        case 1: if(AlarmBank::advanceValue(alarm1))disappearing=true;break;
        case 2: if(AlarmBank::advanceValue(alarm2))noTouch=false;break;
        default: break;
        }
    }
    template<class Random> bool playerContact(MotionBinary64 playerXsp,MotionBinary64 playerYsp,Random& random){
        if(destroyRequested || noTouch)return true;
        if(!contactVelocity(playerXsp,random,xsp) || !contactVelocity(playerYsp,random,ysp))return false;
        noTouch=true;alarm2=5;return true;
    }
private:
    template<class Random> static bool contactVelocity(MotionBinary64 velocity,Random& random,MotionBinary64& result){
        if(!velocity.valid())return false;
        MotionBinary64 magnitude;
        // Even a stationary player consumes this draw. sign(-0) is zero.
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(2),magnitude))return false;
        result=velocity.zero()?MotionBinary64{}:magnitude;
        if(!velocity.zero() && (velocity.high&0x80000000u))result.high|=0x80000000u;
        return true;
    }
};
}
#endif
