#ifndef DOWNWELL_CORE_SMOKE_MOTION_H
#define DOWNWELL_CORE_SMOKE_MOTION_H
#include "SmokeBirth.h"
#include "SmokeDistance.h"
namespace downwell {
// fxSmoke's Step and collision state. The owner supplies runner-compatible
// lengthdir; the native distance gate follows the float-rounded source wrapper.
// Animation/Draw remain owner phases.
struct SmokeMotion : SmokeBirth {
    MotionBinary64 xstart{},ystart{},xsp{},ysp{};
    RunnerImageValue x{},y{};
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,Random& random) {
        SmokeMotion next;
        if (!next.SmokeBirth::create(px,py,random) ||
            !next.x.assign(next.xx) || !next.y.assign(next.yy)) return false;
        next.xstart=next.xx;next.ystart=next.yy;
        *this=next;return true;
    }
    // direction(speed,angle,vx,vy) returns success. escaped(xstart,ystart,x,y,
    // bool&) evaluates the source point_distance(...) > 16 comparison.
    template<class Random,class Direction>
    bool step(Random& random,Direction directionVector) {
        return step(random,directionVector,smokeEscapedDistance);
    }
    template<class Random,class Direction,class Escaped>
    bool step(Random& random,Direction directionVector,Escaped escaped) {
        if (!allSet) {
            if (!directionVector(speed,direction,xsp,ysp) || !sampleImageSpeed(random)) return false;
            allSet=true;
        }
        // Positions integrate the old velocity, publish rounded builtin floats,
        // then velocities decay. There is no TimeStopBound in this event.
        auto nx=xx,ny=yy;
        MotionBinary64 vx,vy;
        std::int32_t px,py;
        RunnerImageValue publishedX,publishedY;
        if (!nx.add(xsp) || !ny.add(ysp) || !nx.roundPixel(px) || !ny.roundPixel(py) ||
            !publishedX.assign(MotionBinary64::fromInteger(px)) ||
            !publishedY.assign(MotionBinary64::fromInteger(py)) ||
            !multiplyBinary64(xsp,{0x3fecccccu,0xcccccccdu},vx) ||
            !multiplyBinary64(ysp,{0x3fecccccu,0xcccccccdu},vy)) return false;
        xx=nx;yy=ny;x=publishedX;y=publishedY;xsp=vx;ysp=vy;
        if (noCollision) {
            bool outside=false;
            if (!escaped(xstart,ystart,x.promoted(),y.promoted(),outside)) return false;
            if (outside) noCollision=false;
        }
        return true;
    }
    // Registered room path. Retain step() above as the scalar oracle. The VM
    // publishes image_speed before allSet, and X before Y before damping;
    // observers must not see an atomically committed end-of-Step position.
    // A failed observer leaves its executed prefix intact and stops the owner.
    template<class Random,class Direction,class Publish,class Escaped>
    bool stepPublished(Random& random,Direction directionVector,Publish& publish,
                       Escaped escaped,bool& good) {
        if(!good)return false;
        if(!allSet){
            if(!directionVector(speed,direction,xsp,ysp)||!sampleImageSpeed(random)||
               !publish.imageSpeed(imageSpeed))return good=false;
            allSet=true;
        }
        std::int32_t rounded;
        if(!xx.add(xsp)||!yy.add(ysp)||!xx.roundPixel(rounded)||
           !x.assignInteger(rounded)||!publish.position(x,y)||
           !yy.roundPixel(rounded)||!y.assignInteger(rounded)||
           !publish.position(x,y))return good=false;
        if(!multiplyBinary64(xsp,{0x3fecccccu,0xcccccccdu},xsp)||
           !multiplyBinary64(ysp,{0x3fecccccu,0xcccccccdu},ysp))return good=false;
        if(noCollision){
            bool outside=false;
            if(!escaped(xstart,ystart,x.promoted(),y.promoted(),outside))return good=false;
            if(outside)noCollision=false;
        }
        return true;
    }
    void alarm0(){noCollision=false;}
    bool collideWall(){
        // The source changes smokesp, NOT xsp/ysp or image_speed. Once allSet,
        // this does not reinitialize motion or consume another random sample.
        if (noCollision) return true;
        MotionBinary64 scaled;
        if (!multiplyBinary64(speed,{0x3fc99999u,0x9999999au},scaled)) return false;
        speed=scaled;return true;
    }
    void collideWater(){} // Empty source event overrides inheritance.
};
}
#endif
