#ifndef DOWNWELL_CORE_TIME_STOP_EFFECTS_H
#define DOWNWELL_CORE_TIME_STOP_EFFECTS_H
#include "PlayerTimeStop.h"
#include "TimeShardBurst.h"
#include "RunnerPointDirection.h"
#include "PortableAtan.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include "TimeChangeEffects.h"
namespace downwell {
// Shared transition/effect coordination. Backend adapters still supply exact
// field masks, audio state and synchronous side effects. Emit(change) performs
// the source sleep(30ms), after the owned time-change Create sets roomSpeed.
// Callbacks must not change this owner's state.
template<unsigned Capacity> struct TimeStopEffects {
    PlayerTimeStop player;
    TimeShardBurst burst;
    TimeShardEffects<Capacity> shards;
    TimeChangeEffects<8> changes;
    int roomSpeed=60; // Source target rate; PAL backend must not add catch-up.
    bool succeeded=true;
    struct PlayerPose {
        RunnerImageValue x,y; // published builtins, not private xx/yy
        MotionBinary64 xsp,ysp;
        MotionBinary64 xx,yy; // private motion coordinates used by time-change Create
    };
    bool reset(){
        if(!shards.clear()){succeeded=false;return false;}
        if(!changes.clear()){succeeded=false;return false;}
        player={};burst={};roomSpeed=60;succeeded=true;return true;
    }
    template<class Random,class Field,class Lookup,class Audio,class Emit>
    bool beginStep(bool mainRoom,bool outsideSides,const PlayerPose& pose,bool lowSpec,
                   Random& random,Field field,Lookup lookup,Audio audio,Emit emit){
        return beginStepChecked(mainRoom,[&](bool& outside){outside=outsideSides;return true;},
            pose,lowSpec,random,field,lookup,audio,emit);
    }
    template<class Side,class Random,class Field,class Lookup,class Audio,class Emit>
    bool beginStepChecked(bool mainRoom,Side side,const PlayerPose& pose,bool lowSpec,
                   Random& random,Field field,Lookup lookup,Audio audio,Emit emit){
        if(!succeeded||!shards.succeeded()||!changes.succeeded()){succeeded=false;return false;}
        succeeded=player.stepChecked(mainRoom,side,field,audio,[&](PlayerTimeStop::Effect effect){
            if(effect==PlayerTimeStop::Effect::change)
                return changes.createAt(pose.xx,pose.yy,roomSpeed,[&](unsigned){return emit(effect);});
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
