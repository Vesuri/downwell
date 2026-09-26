#ifndef DOWNWELL_CORE_PLAYER_DAMAGE_NUMBER_H
#define DOWNWELL_CORE_PLAYER_DAMAGE_NUMBER_H
#include "RunnerImageValue.h"
#include "MultiplyBinary64.h"
#include "AlarmBank.h"
namespace downwell {
// Object 221, no parent/sprite/mask. Step has no time-stop guard. Built-in
// positions narrow on EVERY write; user velocities remain VM binary64 values.
struct PlayerDamageNumber {
    RunnerImageValue x{},y{};
    MotionBinary64 amount{},direction{},xSpeed=MotionBinary64::fromInteger(999),
        ySpeed=MotionBinary64::fromInteger(3);
    int alarm0=60;
    bool valid=true,destroyRequested=false;
    bool create(MotionBinary64 px,MotionBinary64 py){
        PlayerDamageNumber next;
        next.valid=next.x.assign(px)&&next.y.assign(py);
        *this=next;return valid;
    }
    // scrDmgCalcFxPlayer subtracts four BEFORE instance_create narrows y.
    // The owner must perform publication/Create before these argument writes.
    bool assignDamage(MotionBinary64 damage,MotionBinary64 angle){
        if(!valid)return false;
        if(!damage.valid()||!angle.valid()){valid=false;return false;}
        amount=damage;direction=angle;return true;
    }
    void advanceAlarm(unsigned index){
        if(valid&&!destroyRequested&&index==0&&AlarmBank::advanceValue(alarm0))destroyRequested=true;
    }
    // lengthdirX(length,angle,result) supplies runner math. publish runs after
    // each built-in position write, Y then X. It may not mutate this instance.
    template<class LengthdirX,class Publish> bool step(LengthdirX lengthdirX,Publish publish){
        if(!valid)return false;
        if(destroyRequested)return true;
        const auto sentinel=MotionBinary64::fromInteger(999);
        if(xSpeed.high==sentinel.high&&xSpeed.low==sentinel.low){
            if(!lengthdirX(MotionBinary64::fromInteger(2),direction,xSpeed)){valid=false;return false;}
        }
        auto nextY=y.promoted(),negativeSpeed=ySpeed;negativeSpeed.high^=0x80000000u;
        if(!nextY.add(negativeSpeed)||!y.assign(nextY)||!publish(false,*this)){valid=false;return false;}
        auto nextX=x.promoted();
        if(!nextX.add(xSpeed)||!x.assign(nextX)||!publish(true,*this)){valid=false;return false;}
        constexpr MotionBinary64 damping{0x3fe99999u,0x9999999au}; // 0.8, NOT binary32.
        if(!multiplyBinary64(ySpeed,damping,ySpeed)||!multiplyBinary64(xSpeed,damping,xSpeed)){
            valid=false;return false;
        }
        return true;
    }
    // Draw uses the CURRENT font/vertical alignment. Text renders "-" plus
    // source string(amount); it must not round coordinates or change the font.
    // Four cardinal black offsets precede the red centre; no diagonal outline.
    template<class Draw> bool draw(Draw& out)const{
        if(!valid)return false;
        if(destroyRequested)return true;
        const auto sx=x.promoted(),sy=y.promoted();
        out.align(1);out.color(0);
        for(unsigned i=0;i<4;++i){
            auto px=sx,py=sy;
            auto& coordinate=i<2?px:py;
            if(!coordinate.add(MotionBinary64::fromInteger((i&1)?-1:1))||!out.text(px,py,amount))return false;
        }
        out.color(255);
        if(!out.text(sx,sy,amount))return false;
        out.align(0);return true; // Original leaves draw color red.
    }
};
}
#endif
