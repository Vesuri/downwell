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
    // Positioned membership must already exist. Publish source setters in
    // order, retaining private writes and RNG prefixes on a terminal failure.
    // Keep create() below as the atomic legacy reference.
    template<class Random,class Publish> bool createPublished(int area,Random& random,Publish& publish){
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(4),xsp)||
           !xsp.add(MotionBinary64::fromInteger(-2))||
           !randomBinary64(random.nextU32(),{0x3ff80000u,0},ysp)||
           !ysp.add({0xbfe00000u,0}))return false;
        if(area>=1&&area<=5){
            const int sprite=area==2?116:area==4?91:115;
            if(!publish.sprite(sprite))return false;
            spriteId=sprite;
        }
        gravity={0x3fb47ae1u,0x47ae147bu};hardGravity={0x3fb99999u,0x9999999au};
        const MotionBinary64 rates[]={{0xbfe00000u,0},{0xbfd33333u,0x33333333u},{},
                                      {0x3fd33333u,0x33333333u},{0x3fe00000u,0}};
        imageRate=rates[random.chooseIndexSmall(5)];
        if(!imageSpeed.assign(imageRate)||!publish.imageSpeed(imageSpeed))return false;
        MotionBinary64 index;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(7),index)||
           !imageIndex.assign(index)||!publish.imageIndex(imageIndex))return false;
        // The source instance_number branches have identical bodies. Do not
        // replace this with a cap that suppresses births after the fifth one.
        const int ticks[]={30,15,7};
        for(unsigned i=0;i<3;++i){
            alarms.set(i,ticks[i]);if(!publish.alarm(i,ticks[i]))return false;
        }
        disappearing=false;flash=-1;noTouch=true;return true;
    }
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
