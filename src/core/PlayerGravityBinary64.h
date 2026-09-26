#ifndef DOWNWELL_CORE_PLAYER_GRAVITY_BINARY64_H
#define DOWNWELL_CORE_PLAYER_GRAVITY_BINARY64_H
#include "DivideBinary64.h"
#include "PlayerHorizontalBinary64.h"
namespace downwell {
// scrGravity_n, including its area -> water -> style branch priority. Final
// fields belong to the caller and remain assigned if a later operation fails.
struct PlayerGravityBinary64 {
    using N=MotionBinary64;
    N finalGravity{},finalMaximum{};
    bool apply(N& velocity,int area,bool water,int style,bool ballooning,N gravity,N maximum){
        if(area==4){finalMaximum=N::fromInteger(6);finalGravity=gravity;}
        else if(water){
            finalMaximum=N::fromInteger(6);
            if(!divideBinary64(gravity,N{0x3ff66666u,0x66666666u},finalGravity))return false;
        }else if(style==3){
            finalMaximum=maximum;
            if(!divideBinary64(gravity,N{0x3ff80000u,0},finalGravity))return false;
        }else{finalMaximum=maximum;finalGravity=gravity;}
        if(ballooning && !finalMaximum.add(N::fromInteger(-1)))return false;
        if(!velocity.add(finalGravity))return false;
        if(!finalMaximum.valid())return false;
        if(!PlayerHorizontalBinary64::less(velocity,finalMaximum))velocity=finalMaximum;
        return true;
    }
};
}
#endif
