#ifndef DOWNWELL_CORE_FROG_SOURCE_STEP_H
#define DOWNWELL_CORE_FROG_SOURCE_STEP_H
#include "CheckCollisionUnits.h"
#include "FrogMotion.h"
namespace downwell {
// enmFrog Step's motion block with its source collisions, on the compact
// world's fixed point: x in Q16 (FrogHorizontalMotion), y and ysp in fifths
// of Q8 (FrogVerticalMotion, where ugrav 0.15 is exactly 192). After the
// gravity and the hit response:
// - scrCheckCollisionWith(87) against parentThinwall; a downward contact
//   lands only when the frog is not already inside a thin wall, so it jumps
//   up through platforms and lands on them;
// - scrCheckCollisionWith(57) against parentWall: a horizontal contact
//   rebounds xsp * -0.3, a vertical one zeroes ysp (landing if downward);
// - place_meeting(xx, yy + 1, sParentSolid) keeps or clears grounded.
// Ops: meeting(object, x, y, hit) for 87, 57 and 56 with mask 166 at room
// offsets in those units, hit(), scale(sign) and landing(grounded). The
// water multiplier (position_meeting with parentWater) is absent: generated
// rooms have no water and gInWater stays false in world one.
struct FrogSourceStep : CheckCollisionUnits<65536,1280> {
    template<class Ops>
    FrogStepEffects step(FrogHorizontalMotion& horizontal,FrogVerticalMotion& vertical,
                         FrogHitState& hitState,bool integrate,Ops& ops){
        const bool hit=hitState.consume();
        if(hit)horizontal.velocity=FrogHitState::dampHorizontal(horizontal.velocity);
        vertical.applyGravity(hit);
        if(hit&&!ops.hit())return {true,false};
        std::int32_t xx=std::int32_t(horizontal.position),yy=std::int32_t(vertical.position);
        std::int32_t xsp=std::int32_t(horizontal.velocity),ysp=std::int32_t(vertical.velocity);
        bool grounded=vertical.grounded;
        const auto against=[&](int object){
            return [&ops,object](std::int32_t x,std::int32_t y,bool& found){return ops.meeting(object,x,y,found);};
        };
        if(!run(xx,yy,xsp,ysp,against(87)))return {hit,false};
        if(ycollision==1){
            bool inside;
            if(!ops.meeting(87,xx,yy,inside))return {hit,false};
            if(!inside){grounded=true;ysp=0;}
        }
        if(!run(xx,yy,xsp,ysp,against(57)))return {hit,false};
        if(xcollision){
            const std::int32_t numerator=-xsp*3;
            xsp=numerator<0?-((-numerator+5)/10):(numerator+5)/10;
        }
        if(ycollision){ysp=0;if(ycollision==1)grounded=true;}
        bool below;
        if(!ops.meeting(56,xx,yy+1280,below))return {hit,false};
        if(below){if(ysp>0)grounded=true;}else grounded=false;
        if(xsp!=0&&!ops.scale(xsp<0?-1:1))return {hit,false};
        ops.landing(grounded);
        if(integrate){
            if(!grounded)xx+=xsp;
            yy+=ysp;
        }
        horizontal.position=xx;horizontal.velocity=xsp;
        vertical.position=yy;vertical.velocity=ysp;vertical.grounded=grounded;
        return {hit,true};
    }
};
}
#endif
