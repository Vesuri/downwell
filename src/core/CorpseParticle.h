#ifndef DOWNWELL_CORE_CORPSE_PARTICLE_H
#define DOWNWELL_CORE_CORPSE_PARTICLE_H
#include "CorpseAnimation.h"
#include "FixedPoint.h"
namespace downwell {
// The compact corpse owner's object 263 with Q16 motion (W3 fixed point).
// CorpseAnimation, its VM form and oracle, still runs on the registered
// skeleton corpses. Create and scrDeadBody's speed writes stay binary64;
// the first Step converts xx, yy, xsp and ysp once and they are not
// maintained after it. The published x/y stay whole-pixel builtins and
// health stays binary64 (it changes only on shots and blasts).
// Events: meeting(x16, y16, hit) against sParentSolid with Q16
// coordinates, area(), restInPieces(), create(object, px, py) in whole
// pixels and destroy().
struct CorpseParticle : CorpseAnimation {
    // Position in Q16; speeds in 1/65520 pixel, a Q16-like resolution in
    // which gravity, the bounce factors' operands and every threshold below
    // are exact (65520 = 20 * 3276), so they keep the binary64 comparisons'
    // outcomes. A Q16 speed would not: its rounding bias flips sign at each
    // floor bounce and moves the -1 gravity switch by a tick.
    std::int32_t x16=0,y16=0,xspUnits=0,yspUnits=0;
    bool fixed=false;
    static constexpr std::int32_t unit=65520;
    // gravity 0.1 and 0.2 below the -1 threshold, floor bounce above 0.2
    // ending below -0.5, the 0.3 landing stop and the 0.05 minimum
    // horizontal speed; speeds stay within 32 pixels.
    static constexpr std::int32_t gravityUnits=unit/10,hardGravityUnits=unit/5,fallUnits=-unit;
    static constexpr std::int32_t bounceUnits=unit/5,settleUnits=-unit/2,stopUnits=unit*3/10,creepUnits=unit/20,fastUnits=32*unit;
    template<class Random,class Queries>
    bool create(MotionBinary64 px,MotionBinary64 py,bool rip,Random& random,Queries& queries){
        fixed=false;x16=y16=xspUnits=yspUnits=0;
        return CorpseAnimation::create(px,py,rip,random,queries);
    }
    template<class Events> bool step(bool stopped,Events& events){
        if(!valid)return false;
        if(destroyRequested)return true;
        if(!prime()||!run(stopped,events)){valid=false;return false;}
        return true;
    }
    // scrOutofview against the view top in Q8: y below top - 160 or above
    // top + 3000 retires the corpse in the main room.
    template<class Events> bool endStep(bool mainRoom,long topQ8,Events& events){
        if(!valid)return false;
        if(destroyRequested)return true;
        long yq8;
        if(fixed)yq8=long(Fixed16::roundPixel(y16))*256;
        else{
            Fixed16 v;if(!Fixed16::fromBinary64(y.promoted(),v)){valid=false;return false;}
            yq8=long(v.v>>8);
        }
        if(mainRoom&&(yq8<topQ8-160*256||yq8>topQ8+3000*256)){destroyRequested=true;events.destroy();}
        return true;
    }
private:
    // Q16 -> 1/65520 and back, nearest (halves away from zero).
    static std::int32_t units(std::int32_t q16){
        const std::int32_t m=q16<0?-q16:q16,r=m-((m+2048)>>12);
        return q16<0?-r:r;
    }
    static std::int32_t q16(std::int32_t value){
        const std::int32_t m=value<0?-value:value,r=m+(m+2047)/4095;
        return value<0?-r:r;
    }
    // Round half away from zero: v * numerator / denominator.
    static std::int32_t ratio(std::int32_t v,std::int32_t numerator,std::int32_t denominator){
        const std::int32_t m=v<0?-v:v,r=(m*numerator+denominator/2)/denominator;
        return v<0?-r:r;
    }
    bool prime(){
        if(fixed)return true;
        Fixed16 a,b,c,d;
        if(!Fixed16::fromBinary64(xx,a)||!Fixed16::fromBinary64(yy,b)||
           !Fixed16::fromBinary64(xsp,c)||!Fixed16::fromBinary64(ysp,d))return false;
        x16=a.v;y16=b.v;xspUnits=units(c.v);yspUnits=units(d.v);fixed=true;return true;
    }
    static std::int32_t magnitude(std::int32_t v){return v<0?-v:v;}
    template<class Events> bool run(bool stopped,Events& events){
        if(stopped){
            for(loopIndex=0;loopIndex<=2;++loopIndex){
                const auto value=alarms.get(loopIndex);
                if(value==2147483647)return false;
                if(value>0)alarms.set(loopIndex,value+1);
            }
        }else{
            constexpr std::int32_t range=std::int32_t(1)<<30;
            if(magnitude(x16)>=range||magnitude(y16)>=range||
               magnitude(xspUnits)>fastUnits-hardGravityUnits||magnitude(yspUnits)>fastUnits-hardGravityUnits)return false;
            yspUnits+=yspUnits<fallUnits?hardGravityUnits:gravityUnits;
            bool hitWall;
            if(!events.meeting(x16+q16(xspUnits),y16,hitWall))return false;
            if(hitWall)xspUnits=-ratio(xspUnits,1,5);
            if(!events.meeting(x16,y16+q16(yspUnits),hitWall))return false;
            if(hitWall){
                if(yspUnits<=0)yspUnits=0;
                if(bouncing&&yspUnits>bounceUnits){
                    yspUnits=-ratio(yspUnits,1,2);xspUnits=ratio(xspUnits,2,5);
                    if(yspUnits>settleUnits)bouncing=false;
                }
                if(!bouncing&&yspUnits>0){
                    if(!events.meeting(x16,y16+65536,hitWall))return false;
                    if(hitWall){
                        yspUnits=0;grounded=true;
                        xspUnits=ratio(xspUnits,1,2);
                        if(magnitude(xspUnits)<stopUnits)xspUnits=0;
                    }else{
                        collisionDistance=0;loopIndex=1;
                        for(;;){
                            if(!events.meeting(x16,y16+loopIndex*65536,hitWall))return false;
                            if(hitWall)break;
                            ++collisionDistance;if(++loopIndex>16)break;
                        }
                        yspUnits=collisionDistance*unit;
                    }
                }
            }
            if(magnitude(xspUnits)<creepUnits)xspUnits=xspUnits>0?creepUnits:xspUnits<0?-creepUnits:0;
            std::int32_t dx=xspUnits,dy=yspUnits;
            if(events.area()==3){dx=ratio(dx,1,4);dy=ratio(dy,1,4);}
            x16+=q16(dx);y16+=q16(dy);
            const std::int32_t px=Fixed16::roundPixel(x16),py=Fixed16::roundPixel(y16);
            if(!y.assignInteger(py)||!x.assignInteger(px))return false;
            checkRange=4;
            if(events.restInPieces()){
                if(!active)health=MotionBinary64::fromInteger(1);
                int order;
                if(!compareRunnerReal(health,{},order))return false;
                if(order<=0&&active){
                    if(!events.create(272,px,py))return false;
                    if(events.restInPieces()==2)for(unsigned n=0;n<4;++n)
                        if(!events.create(272,px,py))return false;
                    for(unsigned n=0;n<3;++n)if(!events.create(274,px,py))return false;
                    destroyRequested=true;events.destroy();
                }
            }
        }
        // Unlike debris, frozen corpses still update disappearance flashing.
        flash=disappearing?-flash:-1;
        return true;
    }
};
}
#endif
