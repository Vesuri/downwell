#ifndef DOWNWELL_CORE_STEERING_FIXED_STATE_H
#define DOWNWELL_CORE_STEERING_FIXED_STATE_H
#include "RunnerImageValue.h"
#include "FixedPoint.h"
#include "CheckCollisionUnits.h"
namespace downwell {
// scrCheckCollisionWith on Q16 positions and velocities.
using CheckCollisionFixed=CheckCollisionUnits<65536,65536>;

// The compact world's steering families (115, 118 and 173) in Q16 fixed
// point (W3.20): position, velocity and headings in Q16, movesp and maxsp
// in whole thousandths. Every speed constant is a whole thousandth and the
// VM compares reals within 1e-5, so the thousandths take every speed branch
// the binary64 sums take. SteeringEnemyBirthState and its WhiteJellyStep and
// RedBallStep remain the VM forms and the oracles.
struct SteeringFixedState {
    RunnerImageValue x{},y{},imageSpeed{}; // builtins as published
    std::int32_t pixelX=0,pixelY=0; // builtin x and y: whole pixels
    std::int32_t x16=0,y16=0,xsp16=0,ysp16=0; // xx, yy, xsp, ysp
    std::int32_t direction16=0,playerDirection16=0,hitDirection16=0;
    std::int32_t moveSpeedMilli=0,maxSpeedMilli=0;
    int health=0,money=0,cone=0,hitDamage=0;
    bool alive=false,scriptActive=false,hit=false,stomped=false,hitStun=false;
    bool initialized=false;
    static constexpr std::int32_t full=360*65536;
    static constexpr std::uint32_t imageRateBits=0x3e4ccccdu; // imgSp 0.2 as binary32
    static constexpr std::int32_t hitMilli=3000;

    // A Q16 value rounded to binary32's 24 significant bits, halves to even.
    static std::int32_t narrow(std::int32_t q16){
        std::uint32_t m=q16<0?0u-std::uint32_t(q16):std::uint32_t(q16);
        if(m<(1u<<24))return q16;
        unsigned shift=0;
        while((m>>shift)>=(1u<<24))++shift;
        const std::uint32_t unit=1u<<shift,rest=m&(unit-1);
        m-=rest;
        if(rest>unit/2||(rest==unit/2&&(m&unit)))m+=unit;
        return q16<0?-std::int32_t(m):std::int32_t(m);
    }
    // The direction setter's numeric part (RunnerDirectionValue::normalize):
    // narrow, fmod 360, then +360 and narrow again if negative.
    static std::int32_t normalizeDirection(std::int32_t q16){
        std::int32_t value=narrow(q16);
        if(value>=full||value<=-full)value%=full;
        if(value<0)value=narrow(value+full);
        return value;
    }
    void setDirection(std::int32_t wanted){direction16=normalizeDirection(wanted);}
    // The nearest Q16 of a bullet's Q32 turns, in degrees.
    static std::int32_t turnsDegrees(std::uint32_t turns){
        return std::int32_t((std::uint64_t(turns)*360u+0x8000u)>>16);
    }
    // ceil(yy * 256): typStompable's yy > ply + 2 on the Q8 lattice.
    long stompCeilQ8()const{return (long(y16)+255)>>8;}
    // The nearest Q16 of movesp.
    std::int32_t speed16()const{
        return std::int32_t((std::uint32_t(moveSpeedMilli)*65536u+500u)/1000u);
    }
protected:
    template<class Ops> bool publish(bool vertical,std::int32_t pixel,Ops& ops){
        RunnerImageValue wanted,accepted;
        if(!wanted.assignInteger(pixel)||!ops.position(vertical,wanted,accepted))return false;
        (vertical?y:x)=accepted;(vertical?pixelY:pixelX)=pixel;
        return true;
    }
    template<class Ops> bool publishPosition(Ops& ops){
        return publish(false,Fixed16::roundQ16(x16),ops)&&publish(true,Fixed16::roundQ16(y16),ops);
    }
    template<class Ops> bool publishImageSpeed(std::uint32_t bits,Ops& ops){
        RunnerImageValue wanted,accepted;wanted.bits=bits;
        if(!ops.imageSpeed(wanted,accepted))return false;
        imageSpeed=accepted;return true;
    }
    // parentLife -> subparentEnemy Create: move_snap(16, 16), then the
    // concrete event's own fields. Positions are whole pixels.
    template<class Ops> bool inheritedCreate(std::int32_t px,std::int32_t py,Ops& ops){
        if(initialized||px< -100000||px>100000||py< -100000||py>100000)return false;
        alive=true;health=1;money=1;xsp16=ysp16=0;
        hit=stomped=hitStun=false;hitDirection16=0;hitDamage=0;scriptActive=false;
        if(!publish(false,Fixed16::roundQ16(px*4096)*16,ops)||
           !publish(true,Fixed16::roundQ16(py*4096)*16,ops))return false;
        x16=pixelX*65536;y16=pixelY*65536;
        return true;
    }
    // Create's heading: point_direction(x, y, plx, ply) through the setter.
    template<class Ops> bool aimAtPlayer(Ops& ops){
        std::int32_t targetX,targetY;
        if(!ops.playerQ16(true,targetY)||!ops.playerQ16(false,targetX))return false;
        setDirection(FixedTrig::pointDirection(targetX-x16,targetY-y16));
        playerDirection16=direction16;scriptActive=true;
        return true;
    }
    // scrEnemyDeath's four emitMovingFx 704 at xx, yy: speed random(1) + 1,
    // angle random(359), image speed random(0.3 - 0.2) + 0.2, each the
    // nearest Q16 of the VM's sample.
    template<class Random,class Ops> bool deathBurst(Random& random,Ops& ops){
        for(unsigned i=0;i<4;++i){
            Fixed16 speed=Fixed16::random(random.nextU32(),Fixed16::fromInteger(1));
            const Fixed16 angle=Fixed16::random(random.nextU32(),Fixed16::fromInteger(359));
            const Fixed16 rate=Fixed16::random(random.nextU32(),Fixed16::raw(6554));
            speed.v+=65536;
            RunnerImageValue imageRate;
            if(!imageRate.assignQ16(rate.v+13107)||
               !ops.movingFxQ16(Fixed16::raw(x16),Fixed16::raw(y16),704,imageRate,angle,speed))return false;
        }
        return true;
    }
};
}
#endif
