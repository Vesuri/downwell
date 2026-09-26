#ifndef DOWNWELL_CORE_PLAYER_HORIZONTAL_BINARY64_H
#define DOWNWELL_CORE_PLAYER_HORIZONTAL_BINARY64_H
#include "MotionBinary64.h"
namespace downwell {
// Default non-water living-player velocity rules from scrPlMovement blocks
// 20..80 and its final simultaneous-key override. Exact VM double literals
// from scrPlayerInit; this does not perform movement or collision queries.
struct PlayerHorizontalBinary64 {
    using Number=MotionBinary64;
    static bool less(Number a,Number b){
        if(a.zero() && b.zero())return false;
        const bool an=(a.high>>31)!=0,bn=(b.high>>31)!=0;
        if(an!=bn)return an;
        const auto ah=a.high&0x7fffffffu,bh=b.high&0x7fffffffu;
        return an ? (ah>bh || (ah==bh && a.low>b.low))
                  : (ah<bh || (ah==bh && a.low<b.low));
    }
    static Number negative(Number n){n.high^=0x80000000u;return n;}
    static bool apply(Number& speed,int& facing,bool grounded,bool left,bool right,bool locked=false){
        if(!speed.valid())return false;
        if(locked){speed={};return true;}
        const Number acceleration{0x3fc99999u,0x9999999au}; // 0.2
        const Number boost=Number::fromInteger(grounded?2:1);
        const Number maximum=grounded?Number{0x40000000u,0}:Number{0x40040000u,0};
        if(left){
            if(!speed.add(negative(acceleration)))return false;
            if(less({},speed) && !speed.add(negative(boost)))return false;
            if(less(speed,negative(maximum)))speed=negative(maximum);
            facing=-1;
        }
        if(right){
            if(!speed.add(acceleration))return false;
            if(less(speed,{}) && !speed.add(boost))return false;
            if(less(maximum,speed))speed=maximum;
            facing=1;
        }
        if(!left && !right){
            const Number friction=grounded?Number{0x3fd99999u,0x9999999au}
                                         :Number{0x3fb99999u,0x9999999au};
            if(!speed.zero() && !speed.add(less(speed,{})?friction:negative(friction)))return false;
            Number magnitude=speed;magnitude.high&=0x7fffffffu;
            if(less(magnitude,Number{0x3fd33333u,0x33333333u}))speed={}; // abs < 0.3
        }
        if(left && right)speed={};
        return true;
    }
};
}
#endif
