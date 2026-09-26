#ifndef DOWNWELL_CORE_TIME_SHARD_BIRTH_H
#define DOWNWELL_CORE_TIME_SHARD_BIRTH_H
#include "RandomBinary64.h"
#include "RunnerImageValue.h"
namespace downwell {
// Object 238 Create. Script velocity overrides happen AFTER this entire event,
// so the two initial velocity choices still consume the shared RNG stream.
struct TimeShardBirth {
    RunnerImageValue x{},y{},imageSpeed{},imageIndex{};
    MotionBinary64 xx{},yy{},xsp{},ysp{},savedSpeed{};
    MotionBinary64 gravity{0x3fb47ae1u,0x47ae147bu}; // 0.08
    MotionBinary64 hardGravity{0x3fb99999u,0x9999999au}; // 0.1
    int alarm0=0,alarm1=40,alarm2=30,shardLimit=20,dflash=-1;
    bool disappearing=false,noTouch=true;
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,bool lowSpec,Random& random){
        TimeShardBirth next;
        if(!next.x.assign(px) || !next.y.assign(py))return false;
        next.xsp=MotionBinary64::fromInteger(int(random.irandomSmall(4))-2);
        static constexpr int vertical[]={0,-1,-1,-1,-2,-2,-3};
        next.ysp=MotionBinary64::fromInteger(vertical[random.chooseIndexSmall(7)]);
        next.xx=next.x.promoted();next.yy=next.y.promoted();next.shardLimit=lowSpec?3:20;
        static constexpr MotionBinary64 rates[]={
            {0xbfe00000u,0},{0xbfd33333u,0x33333333u},{0,0},
            {0x3fd33333u,0x33333333u},{0x3fe00000u,0}};
        next.savedSpeed=rates[random.chooseIndexSmall(5)];
        if(!next.imageSpeed.assign(next.savedSpeed))return false;
        MotionBinary64 index;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(7),index) ||
           !next.imageIndex.assign(index))return false;
        // Alarm setter 0x431299 truncates the VM double before storage.
        next.alarm0=90+int(random.randomIntegerSmall(70));
        *this=next;return true;
    }
};
}
#endif
