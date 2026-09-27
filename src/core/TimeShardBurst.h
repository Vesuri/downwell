#ifndef DOWNWELL_CORE_TIME_SHARD_BURST_H
#define DOWNWELL_CORE_TIME_SHARD_BURST_H
#include "TimeShardEffects.h"
namespace downwell {
// scrTimeShard writes these variables on its CALLER, not on the shard.
struct TimeShardBurst {
    std::int32_t field=-4;
    std::uint32_t shard=0;
    MotionBinary64 direction{},scatteredDirection{},speed{};
    // lookup returns success and fills id/x/y; negative id means no instance.
    // pointDirection and component implement audited runner math; component's
    // final bool selects lengthdir_y. Neither may mutate the shard store.
    template<unsigned Capacity,class Random,class Lookup,class PointDirection,class Component>
    bool emit(RunnerImageValue x,RunnerImageValue y,MotionBinary64 xsp,MotionBinary64 ysp,
              bool lowSpec,Random& random,TimeShardEffects<Capacity>& effects,
              Lookup lookup,PointDirection pointDirection,Component component){
        std::int32_t found=-4;MotionBinary64 fx,fy;
        if(!lookup(x,y,found,fx,fy))return false;
        if(found>0){
            if(!lookup(x,y,field,fx,fy) || field<0 ||
               !pointDirection(fx,fy,x.promoted(),y.promoted(),direction))return false;
        }else if(!pointDirection(MotionBinary64{},MotionBinary64{},xsp,ysp,direction))return false;
        for(unsigned i=0;i<3;++i){
            if(i==2 && !direction.add(MotionBinary64::fromInteger(180)))return false;
            MotionBinary64 scatter;
            if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(90),scatter) ||
               !scatter.add(MotionBinary64::fromInteger(-45)))return false;
            scatteredDirection=direction;
            if(!scatteredDirection.add(scatter) ||
               !randomBinary64(random.nextU32(),MotionBinary64::fromInteger(5),speed) ||
               !speed.add(MotionBinary64::fromInteger(1)))return false;
            // Complete all six child Create draws before evaluating lengthdir.
            const auto handle=effects.create(x.promoted(),y.promoted(),lowSpec,random);
            shard=effects.sourceId(handle);
            auto* child=effects.get(handle);
            if(!child)return false;
            if(!component(speed,scatteredDirection,false,child->xsp) ||
               !component(speed,scatteredDirection,true,child->ysp))return false;
        }
        return true;
    }
};
}
#endif
