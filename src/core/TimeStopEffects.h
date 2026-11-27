#ifndef DOWNWELL_CORE_TIME_STOP_EFFECTS_H
#define DOWNWELL_CORE_TIME_STOP_EFFECTS_H
#include "PlayerTimeStop.h"
#include "TimeShardBurst.h"
#include "RunnerPointDirection.h"
#include "PortableAtan.h"
#include "SmokeDirection.h"
#include "SmokeTrigNarrow.h"
#include "TimeChangeEffects.h"
#include "TimeStopAudio.h"
#include "DivideBinary64.h"
namespace downwell {
// Shared transition/effect coordination. Backend adapters still supply exact
// field masks, audio state and synchronous side effects. Emit(change) performs
// the source sleep(30ms), after the owned time-change Create sets roomSpeed.
// Callbacks must not change this owner's state. Shard is TimeShardAnimation
// (the VM form) or TimeShardParticle (the compact world's Q16 shards, born
// by TimeShardBurst::emitFixed).
template<unsigned Capacity,class Shard=TimeShardAnimation> struct TimeStopEffects {
    PlayerTimeStop player;
    TimeShardBurst burst;
    TimeShardEffects<Capacity,Shard> shards;
    static constexpr bool fixedShards=Shard::fixedPoint;
    TimeChangeEffects<8> changes;
    int roomSpeed=60; // Source target rate; PAL backend must not add catch-up.
    int shakeAmt=0,shakeDur=0; // scrSShake's caller-local writes.
    bool succeeded=true;
    // The compact world's pose in Q16 (W3.20): exact from the player's Q8
    // position and speed, the vertical speed's fifths/1280 rounded.
    struct FixedPlayerPose {
        RunnerImageValue x,y; // published builtins, not private xx/yy
        Fixed16 xsp,ysp,xx,yy;
        bool fromQ8(int publishedX,int publishedY,long xxQ8,long yyQ8,long xspQ8,long yspFifths){
            if(xxQ8< -0x7fffffL||xxQ8>0x7fffffL||yyQ8< -0x7fffffL||yyQ8>0x7fffffL||
               xspQ8< -0x7fffffL||xspQ8>0x7fffffL||yspFifths< -0x7fffffL||yspFifths>0x7fffffL)return false;
            xx.v=std::int32_t(xxQ8*256);yy.v=std::int32_t(yyQ8*256);xsp.v=std::int32_t(xspQ8*256);
            const std::int32_t n=std::int32_t(yspFifths)*256; // fifths / 1280 = n / 5 in Q16
            ysp.v=(n<0?n-2:n+2)/5;
            return x.assignInteger(publishedX)&&y.assignInteger(publishedY);
        }
    };
    struct Binary64PlayerPose {
        RunnerImageValue x,y; // published builtins, not private xx/yy
        MotionBinary64 xsp,ysp;
        MotionBinary64 xx,yy; // private motion coordinates used by time-change Create
        // Exact conversion of the current portable player's representation,
        // not a claim that Q8/fifths replace the original VM motion arithmetic.
        template<class Motion> bool fromPortableMotion(RunnerImageValue publishedX,RunnerImageValue publishedY,
            const Motion& motion,int originX,int originY){
            x=publishedX;y=publishedY;
            return divideBinary64(MotionBinary64::fromInteger(motion.offsetXFixed()),MotionBinary64::fromInteger(256),xx)&&
                divideBinary64(MotionBinary64::fromInteger(motion.offsetYFixed()),MotionBinary64::fromInteger(256),yy)&&
                xx.add(MotionBinary64::fromInteger(originX))&&yy.add(MotionBinary64::fromInteger(originY))&&
                divideBinary64(MotionBinary64::fromInteger(motion.speedXFixed()),MotionBinary64::fromInteger(256),xsp)&&
                divideBinary64(MotionBinary64::fromInteger(motion.speedYFifths()),MotionBinary64::fromInteger(1280),ysp);
        }
    };
    // No <type_traits> in the freestanding build.
    template<bool Fixed,class=void> struct PoseFor {using type=Binary64PlayerPose;};
    template<class Unused> struct PoseFor<true,Unused> {using type=FixedPlayerPose;};
    using PlayerPose=typename PoseFor<fixedShards>::type;
    template<class Side,class Random,class Field,class Lookup,class Audio,class Emit>
    bool beginStepWithAudio(bool mainRoom,Side side,const PlayerPose& pose,bool lowSpec,
                   Random& random,Field field,Lookup lookup,TimeStopAudio& audioState,
                   SoundPlayState& caller,SoundPlayEnvironment& env,int area,int music,int wind,
                   Audio& audio,Emit emit){
        return beginStepWithAudioChecked(mainRoom,side,pose,lowSpec,random,field,lookup,
            [&](PlayerTimeStop::AudioQuery q,bool& value){return TimeStopAudio::query(q,music,wind,audio,value);},
            [&](PlayerTimeStop::Effect effect){
                return TimeStopAudio::handles(effect)?audioState.emit(effect,caller,env,area,music,wind,audio):emit(effect);
            });
    }
    bool reset(){
        if(!shards.clear()){succeeded=false;return false;}
        if(!changes.clear()){succeeded=false;return false;}
        player={};burst={};roomSpeed=60;shakeAmt=shakeDur=0;succeeded=true;return true;
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
        return beginStepWithAudioChecked(mainRoom,side,pose,lowSpec,random,field,lookup,
            [&](PlayerTimeStop::AudioQuery query,bool& value){value=audio(query);return true;},emit);
    }
    template<class Side,class Random,class Field,class Lookup,class Audio,class Emit>
    bool beginStepWithAudioChecked(bool mainRoom,Side side,const PlayerPose& pose,bool lowSpec,
                   Random& random,Field field,Lookup lookup,Audio audio,Emit emit){
        if(!succeeded||!shards.succeeded()||!changes.succeeded()){succeeded=false;return false;}
        succeeded=player.stepWithAudioChecked(mainRoom,side,field,audio,[&](PlayerTimeStop::Effect effect){
            if(effect==PlayerTimeStop::Effect::shake){shakeAmt=2;shakeDur=2;}
            if(effect==PlayerTimeStop::Effect::change){
                if constexpr(fixedShards)
                    return changes.createAtQ16(pose.xx.v,pose.yy.v,roomSpeed,[&](unsigned){return emit(effect);});
                else return changes.createAt(pose.xx,pose.yy,roomSpeed,[&](unsigned){return emit(effect);});
            }
            if(effect!=PlayerTimeStop::Effect::shards)return emit(effect);
            if constexpr(fixedShards)
                return burst.emitFixed(pose.x,pose.y,pose.xsp.v,pose.ysp.v,lowSpec,random,shards,lookup);
            else return burst.emit(pose.x,pose.y,pose.xsp,pose.ysp,lowSpec,random,shards,lookup,
                [](auto ax,auto ay,auto bx,auto by,auto& angle){
                    return RunnerPointDirection::evaluate(ax,ay,bx,by,angle,portableAtan);
                },[](auto speed,auto angle,bool vertical,auto& result){
                    MotionBinary64 x,y;
                    if(!SmokeDirection::vector(speed,angle,x,y,smokeTrigNarrowed))return false;
                    result=vertical?y:x;return true;
                });
        });
        return succeeded;
    }
};
}
#endif
