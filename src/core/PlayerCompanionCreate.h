#ifndef DOWNWELL_CORE_PLAYER_COMPANION_CREATE_H
#define DOWNWELL_CORE_PLAYER_COMPANION_CREATE_H
#include "MotionBinary64.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Custom reals from objDrone/decoyBalloon Create. Builtin image fields live
// on the registered instance. Alarm bodies below are shared; Step remains
// separate (drone angles are unbounded, unlike the smoke trig domain).
struct DroneCreateState {
    MotionBinary64 xdir{},ydir{},shotDelay{},burstCount{},droneShooting{};
};
struct BalloonCreateState {
    MotionBinary64 imgSp{0x3fb47ae1u,0x47ae147bu}; // 0.08
    MotionBinary64 xsp{},ysp{},grounded{},hit{};
    MotionBinary64 ugrav{0x3fb99999u,0x9999999au},ugravhard{0x3fc99999u,0x9999999au};
    MotionBinary64 hitStun{},noTouch=MotionBinary64::fromInteger(1),xMove{};
    MotionBinary64 stringLength=MotionBinary64::fromInteger(20),noHold{},held{};
    MotionBinary64 noHoldTime=MotionBinary64::fromInteger(40),touched{},bouncing{},dir2p{},dis2p{},xx{},yy{};
};
struct PlayerCompanionCreateState {
    std::uint32_t sourceId=0;
    DroneCreateState drone;
    BalloonCreateState balloon;
    RunnerImageValue timeline{};
    bool alarm(int object,unsigned number){
        if(object==249){
            if(number==3)return true; // Serialized empty event, not a reset.
            if(number==2){drone.shotDelay={};return true;}
        }else if(object==256){
            if(number==0){balloon.noHold={};return true;}
            if(number==1){balloon.hitStun={};return true;}
        }
        return false; // Drone Alarm 0 needs synchronous effect/bullet/audio owners.
    }
    // Original serialized sprites have consecutive, unit-length keys. The
    // caller owns phase eligibility and collision-index publication.
    template<class Publish> bool advanceImage(int object,int sprite,RunnerImageValue written,
        RunnerImageValue speed,Publish publish){
        const unsigned frames=object==249&&sprite==131?2:object==256&&sprite==526?4:0;
        RunnerImageValue evaluated;
        if(!frames||!UnitSequenceFrames::evaluate(timeline,frames,evaluated))return false;
        bool reconcile;
        if(!UnitSequenceFrames::needsReconcile(written,evaluated,reconcile))return false;
        auto next=reconcile?written:timeline;
        bool crossed;
        if(!UnitSequenceFrames::advance(next,speed,frames,evaluated,crossed)||!publish(evaluated))return false;
        timeline=next;
        return true; // Neither object has an Animation End event.
    }
    template<class Digest> void appendState(Digest& digest,int object)const{
        const auto add=[&](MotionBinary64 value){digest.addU32(value.high);digest.addU32(value.low);};
        digest.addU32(sourceId);
        digest.addU32(timeline.bits);
        if(object==249){
            add(drone.xdir);add(drone.ydir);add(drone.shotDelay);add(drone.burstCount);add(drone.droneShooting);
        }else{
            const MotionBinary64 values[]={balloon.imgSp,balloon.xsp,balloon.ysp,balloon.grounded,balloon.hit,
                balloon.ugrav,balloon.ugravhard,balloon.hitStun,balloon.noTouch,balloon.xMove,
                balloon.stringLength,balloon.noHold,balloon.held,balloon.noHoldTime,balloon.touched,
                balloon.bouncing,balloon.dir2p,balloon.dis2p,balloon.xx,balloon.yy};
            for(auto value:values)add(value);
        }
    }
};
}
#endif
