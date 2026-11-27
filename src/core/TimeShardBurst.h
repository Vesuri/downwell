#ifndef DOWNWELL_CORE_TIME_SHARD_BURST_H
#define DOWNWELL_CORE_TIME_SHARD_BURST_H
#include "TimeShardEffects.h"
#include "FixedPoint.h"
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
    // emit for the compact world's Q16 shards (W3 fixed point): the same
    // lookups and RNG draws in the same order, with point_direction,
    // random_range and lengthdir in Q16 degrees and pixels (FixedTrig). The
    // field and player positions are whole pixels; xsp and ysp are the
    // player's speeds. The binary64 caller fields keep their source roles
    // only as Q16 values: direction16, scattered16 and speed16.
    std::int32_t direction16=0,scattered16=0,speed16=0;
    template<unsigned Capacity,class Random,class Lookup>
    bool emitFixed(RunnerImageValue x,RunnerImageValue y,std::int32_t xsp,std::int32_t ysp,
                   bool lowSpec,Random& random,TimeShardEffects<Capacity,TimeShardParticle>& effects,Lookup lookup){
        // lookup(x, y, id, fieldX, fieldY) gives the field's whole pixels.
        std::int32_t found=-4,px,py,fieldX,fieldY;
        if(!x.roundPixel(px)||!y.roundPixel(py)||!lookup(x,y,found,fieldX,fieldY))return false;
        if(found>0){
            if(!lookup(x,y,field,fieldX,fieldY)||field<0||
               px-fieldX< -(1<<14)||px-fieldX>=(1<<14)||py-fieldY< -(1<<14)||py-fieldY>=(1<<14))return false;
            direction16=FixedTrig::pointDirection((px-fieldX)*65536,(py-fieldY)*65536);
        }else direction16=FixedTrig::pointDirection(xsp,ysp);
        for(unsigned i=0;i<3;++i){
            if(i==2)direction16+=180*65536;
            // random(90) - 45 and random(5) + 1, as Q16 from the top 16 bits.
            scattered16=direction16+std::int32_t(random.nextU32()>>16)*90-45*65536;
            speed16=std::int32_t(random.nextU32()>>16)*5+65536;
            // Complete all six child Create draws before evaluating lengthdir.
            const auto handle=effects.createWhole(px,py,lowSpec,random);
            shard=effects.sourceId(handle);
            auto* child=effects.get(handle);
            if(!child)return false;
            child->setVelocity(FixedTrig::lengthdir(false,speed16,scattered16),
                               FixedTrig::lengthdir(true,speed16,scattered16));
        }
        return true;
    }
};
}
#endif
