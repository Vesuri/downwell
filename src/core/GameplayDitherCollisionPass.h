#ifndef DOWNWELL_CORE_GAMEPLAY_DITHER_COLLISION_PASS_H
#define DOWNWELL_CORE_GAMEPLAY_DITHER_COLLISION_PASS_H
#include "GameplayCollisionOwner.h"
namespace downwell {
// Native counterpart of StoredDitherCollisionPass's production admission.
// Collect once using all source identities; filtering must not change the
// historical spatial tree, dirty-list retention or the sealed pair order.
template<unsigned Capacity=16384> class GameplayDitherCollisionPass {
public:
    GameplayDitherCollisionPass(GameplayCollisionOwner& owner,const SpriteMaskPack& masks)
        :owner_(owner),masks_(masks){}
    template<class Audio,class RoomState> bool run(GameplayWorld& world,MotionBinary64 volume,
        Audio& audio,RoomState roomState){
        if(!owner_.boundTo(world)||!world.effectsSucceeded()||!masks_.valid())return false;
        if(!owner_.collectCollisionPairs(pairs_))return false;
        RunnerCollisionEventContext context;
        // All admitted objects are non-solid. Fail rather than manufacture
        // engine/path motion if resource flags cease to satisfy that contract.
        struct Motion {
            bool rollback(InstanceHandle){return false;}
            bool prepareMotion(InstanceHandle){return false;}
            bool advanceMotion(InstanceHandle){return false;}
        } motion;
        return owner_.executeSelectedCollisionPairs(pairs_,masks_,context,motion,
            roomState,
            [&](InstanceHandle,InstanceHandle,int first,int second){
                return selected(first,second)||selected(second,first);
            },[&](const RunnerCollisionEventContext::Frame& event){
                return event.owner==255?owner_.dispatchTimeFieldWind(event,volume,audio):
                    owner_.dispatchDitherAboveCollision(event);
            });
    }
private:
    static bool selected(int a,int b){return (a==61&&b==60)||((a==255||a==371)&&b==35);}
    GameplayCollisionOwner& owner_;
    const SpriteMaskPack& masks_;
    RunnerCollisionPairs<GameplayCollisionOwner::View::capacity,Capacity> pairs_;
};
}
#endif
