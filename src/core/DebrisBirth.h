#ifndef DOWNWELL_CORE_DEBRIS_BIRTH_H
#define DOWNWELL_CORE_DEBRIS_BIRTH_H
#include "RandomBinary64.h"
#include "RunnerImageValue.h"
#include "AlarmBank.h"
namespace downwell {
// Object 370 Create. No parent Create, collision query or relocation. The
// instance owner supplies position and lifetime; this does not run Step/Draw.
struct DebrisBirth {
    MotionBinary64 xsp{},ysp{},imageRate{};
    MotionBinary64 gravity{0x3fb47ae1u,0x47ae147bu}; // 0.08
    MotionBinary64 hardGravity{0x3fb99999u,0x9999999au}; // 0.1
    RunnerImageValue imageSpeed{},imageIndex{};
    AlarmBank alarms;
    int spriteId=91,flash=-1;
    bool disappearing=false,noTouch=true;
    template<class Random> bool create(int area,Random& random){
        DebrisBirth next;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(4),next.xsp)||
           !next.xsp.add(MotionBinary64::fromInteger(-2))||
           !randomBinary64(random.nextU32(),{0x3ff80000u,0},next.ysp)||
           !next.ysp.add({0xbfe00000u,0}))return false;
        if(area==1||area==3||area==5)next.spriteId=115;
        else if(area==2)next.spriteId=116;
        // Area 4 explicitly assigns 91; other areas retain the resource default.
        const MotionBinary64 rates[]={{0xbfe00000u,0},{0xbfd33333u,0x33333333u},{},
                                      {0x3fd33333u,0x33333333u},{0x3fe00000u,0}};
        next.imageRate=rates[random.chooseIndexSmall(5)];
        MotionBinary64 index;
        if(!next.imageSpeed.assign(next.imageRate)||
           !randomBinary64(random.nextU32(),MotionBinary64::fromInteger(7),index)||
           !next.imageIndex.assign(index))return false;
        // Both instance_number(370)<5 branches assign the same alarms.
        next.alarms.set(0,30);next.alarms.set(1,15);next.alarms.set(2,7);
        *this=next;return true;
    }
};
}
#endif
