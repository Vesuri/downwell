#ifndef DOWNWELL_TEST_BLAST_SKELETON_CHECKS_H
#define DOWNWELL_TEST_BLAST_SKELETON_CHECKS_H
#include "core/BlastSkeletonContact.h"
#include <cassert>
struct BlastHitAudio {
    unsigned calls=0;
    bool isPlaying(int id,bool& playing){assert(id==29);playing=true;++calls;return true;}
    bool stop(int id){assert(id==29);++calls;return true;}
    bool play(int id,int priority,bool loop,int& voice){assert(id==29&&priority==90&&!loop);voice=1001;++calls;return true;}
    bool getGain(int id,downwell::MotionBinary64& value){assert(id==1001);value=downwell::MotionBinary64::fromInteger(1);++calls;return true;}
    bool setGain(int id,downwell::MotionBinary64 value,int time){assert(id==1001&&!time&&value.high==0x3ff00000u&&!value.low);++calls;return true;}
    bool setPitch(int id,downwell::MotionBinary64 value){assert(id==1001&&value.high==0x3ff00000u&&!value.low);++calls;return true;}
};
// Impact is a deliberately explicit test dependency. These checks establish
// target writes/event routing, not natural object-219 creation or rendering.
template<class Invoke,class Blast,class Skeleton>
void checkBlastSkeleton(bool rejectImpact,unsigned id,Invoke invoke,Blast blast,Skeleton skeleton){
    using namespace downwell;BlastHitAudio audio;SoundPlayEnvironment env;unsigned impacts=0;
    auto impact=[&](auto& caller,MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,MotionBinary64 angle,int depth){
        assert(x.high==0x406e0800u&&!x.low&&y.high==0x40690000u&&!y.low&&sprite==605&&
            speed.high==0x3fd99999u&&speed.low==0x9999999au&&angle.high==0x4070e000u&&!angle.low&&!depth);
        assert(skeleton().health==10&&skeleton().damageStun&&skeleton().hitPending&&audio.calls==6);
        caller.myFx=700123;++impacts;return !rejectImpact;
    };
    assert(invoke(impact,env,audio)==!rejectImpact);
    const auto& victim=skeleton();const auto& caller=blast();
    assert(victim.health==10&&victim.hitPending&&victim.damageStun&&victim.hitDamage==0&&
        victim.hitDirection.high==0x4070e000u&&!victim.hitDirection.low&&
        victim.takenImpact.high==0x40240000u&&!victim.takenImpact.low&&
        caller.hits.count==1&&unsigned(caller.hits.ids[0])==id&&caller.myFx==700123&&!caller.destroyRequested&&
        audio.calls==6&&impacts==1);
    // Hit memory suppresses a duplicate pair; failure latches before retry.
    assert(invoke(impact,env,audio)==!rejectImpact&&audio.calls==6&&impacts==1&&skeleton().health==10);
}
#endif
