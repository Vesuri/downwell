#ifndef DOWNWELL_CORE_CHECK_COLLISION_WITH_SCRIPT_H
#define DOWNWELL_CORE_CHECK_COLLISION_WITH_SCRIPT_H
#include "PlayerSolidCollisionBinary64.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Retained instance variables of scrCheckCollisionWith, not temporary locals.
// Query(x,y,target,hit) must use the caller's current collision mask and must
// not run events or mutate the caller. Unsupported queries abort the script.
struct CheckCollisionWithScript {
    using N=MotionBinary64;
    int checkTarget=-1,xcollision=0,ycollision=0;
    N workaroundx{},workaroundy{},whileLimit{},moveAmount{},xmoveAmount{},ymoveAmount{};
    static bool exhausted(N amount,N limit,bool& result){
        int order;
        if(!compareRunnerReal(PlayerSolidCollisionBinary64::magnitude(amount),
            PlayerSolidCollisionBinary64::magnitude(limit),order))return false;
        result=order>=0;return true;
    }
    template<class Query> bool run(int target,N& xx,N& yy,N xsp,N ysp,Query query){
        using M=PlayerSolidCollisionBinary64;
        checkTarget=target;
        if(!xx.valid()||!yy.valid()||!xsp.valid()||!ysp.valid())return false;
        if(!M::probe(xx,xsp,workaroundx)||!M::probe(yy,ysp,workaroundy))return false;
        xcollision=0;ycollision=0;
        const int sx=M::sign(xsp),sy=M::sign(ysp);
        const N dx=N::fromInteger(sx),dy=N::fromInteger(sy);
        bool hit,done;
        if(!query(workaroundx,yy,checkTarget,hit))return false;
        if(hit){
            if(!M::rounded(xx,xx))return false;
            whileLimit=xsp;moveAmount={};
            for(;;){
                N candidate;
                if(!M::offset(xx,moveAmount,dx,candidate)||!query(candidate,yy,checkTarget,hit))return false;
                if(hit)break;
                if(!moveAmount.add(dx)||!exhausted(moveAmount,whileLimit,done))return false;
                if(done){moveAmount={};break;}
            }
            if(!xx.add(moveAmount))return false;
            xcollision=sx;
        }
        if(!query(xx,workaroundy,checkTarget,hit))return false;
        if(hit){
            if(!M::rounded(yy,yy))return false;
            whileLimit=ysp;moveAmount={};
            for(;;){
                N candidate;
                if(!M::offset(yy,moveAmount,dy,candidate)||!query(xx,candidate,checkTarget,hit))return false;
                if(hit)break;
                if(!moveAmount.add(dy)||!exhausted(moveAmount,whileLimit,done))return false;
                if(done){moveAmount={};break;}
            }
            if(!yy.add(moveAmount))return false;
            ycollision=sy;
        }
        if(xcollision||ycollision)return true;
        if(!query(workaroundx,workaroundy,checkTarget,hit))return false;
        if(!hit)return true;
        if(!M::rounded(yy,yy)||!M::rounded(xx,xx))return false;
        whileLimit=ysp;xmoveAmount={};ymoveAmount={};
        for(;;){
            N qx,qy;
            if(!M::offset(yy,ymoveAmount,{},qy)||!M::offset(xx,xmoveAmount,dx,qx)||
                !query(qx,qy,checkTarget,hit))return false;
            if(hit)break;
            if(!xmoveAmount.add(dx))return false;
            if(!M::offset(yy,ymoveAmount,dy,qy)||!M::offset(xx,xmoveAmount,{},qx)||
                !query(qx,qy,checkTarget,hit))return false;
            if(hit)break;
            if(!ymoveAmount.add(dy)||!exhausted(ymoveAmount,whileLimit,done))return false;
            if(done){xmoveAmount={};ymoveAmount={};break;}
        }
        if(!yy.add(ymoveAmount)||!xx.add(xmoveAmount))return false;
        ycollision=sy;xcollision=sx;return true;
    }
};
}
#endif
