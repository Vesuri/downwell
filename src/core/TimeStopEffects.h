#ifndef DOWNWELL_CORE_TIME_STOP_EFFECTS_H
#define DOWNWELL_CORE_TIME_STOP_EFFECTS_H
#include "PlayerTimeStop.h"
#include "TimeShardBurst.h"
#include "RunnerPointDirection.h"
#include "PortableAtan.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
namespace downwell {
// Shared transition/effect coordination. Backend adapters still supply exact
// field masks, audio state and synchronous non-shard effects (including the
// time-change instance). They must not change this owner's state in callbacks.
template<unsigned Capacity> struct TimeStopEffects {
    PlayerTimeStop player;
    TimeShardBurst burst;
    TimeShardEffects<Capacity> shards;
    bool succeeded=true;
    struct PlayerPose {
        RunnerImageValue x,y; // published builtins, not private xx/yy
        MotionBinary64 xsp,ysp;
    };
    void reset(){player={};burst={};shards.clear();succeeded=true;}
    template<class Random,class Field,class Lookup,class Audio,class Emit>
    bool beginStep(bool mainRoom,bool outsideSides,const PlayerPose& pose,bool lowSpec,
                   Random& random,Field field,Lookup lookup,Audio audio,Emit emit){
        if(!succeeded)return false;
        succeeded=player.step(mainRoom,outsideSides,field,audio,[&](PlayerTimeStop::Effect effect){
            if(effect!=PlayerTimeStop::Effect::shards)return emit(effect);
            return burst.emit(pose.x,pose.y,pose.xsp,pose.ysp,lowSpec,random,shards,lookup,
                [](auto ax,auto ay,auto bx,auto by,auto& angle){
                    return RunnerPointDirection::evaluate(ax,ay,bx,by,angle,portableAtan);
                },[](auto speed,auto angle,bool vertical,auto& result){
                    MotionBinary64 x,y;
                    if(!SmokeDirection::vector(speed,angle,x,y,smokeTrig))return false;
                    result=vertical?y:x;return true;
                });
        });
        return succeeded;
    }
};
}
#endif
