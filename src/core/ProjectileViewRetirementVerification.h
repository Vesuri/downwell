#ifndef DOWNWELL_PROJECTILE_VIEW_RETIREMENT_VERIFICATION_H
#define DOWNWELL_PROJECTILE_VIEW_RETIREMENT_VERIFICATION_H
#include "DroneBulletSourceStep.h"
namespace downwell {
struct ProjectileViewRetirementVerification {
    struct Ops {
        int y=0,height=284;bool main=true,mutate=false;
        unsigned calls=0,fail=0,kills=0,rooms=0,reads=0;
        bool call(){return ++calls!=fail;}
        bool readY(MotionBinary64& out){++reads;out=MotionBinary64::fromInteger(y);return call();}
        bool viewY(MotionBinary64& out){out={};return call();}
        bool viewHeight(MotionBinary64& out){out=MotionBinary64::fromInteger(height);return call();}
        bool mainRoom(bool& out){++rooms;out=main;return call();}
        bool destroySelf(){if(!call())return false;++kills;if(mutate)y=-200;return true;}
    };
    static bool run(){
        const int positions[]={-162,-161,-160,-101,-100,-99,0,315,316,317,2999,3000,3001,3002};
        for(unsigned room=0;room<2;++room)for(int y:positions){
            Ops ops;ops.y=y;ops.main=room!=0;bool good=true;DroneBulletSourceStep shot;
            const unsigned expected=unsigned(room&&(y<-160||y>3000))+unsigned(y>316)+unsigned(y<-100);
            if(!shot.endStepWithView(ops,good)||!good||ops.kills!=expected||
                ops.rooms!=unsigned(y<-160||y>3000)||ops.reads!=unsigned(y<-160?3:4))return false;
        }
        Ops success;success.y=400;success.mutate=true;bool good=true;DroneBulletSourceStep shot;
        // First bullet check destroys below the viewport; synchronous callback
        // moves self above it, so the second check must read again and destroy.
        if(!shot.endStepWithView(success,good)||success.kills!=2)return false;
        for(unsigned failure=1;failure<=success.calls;++failure){
            Ops ops;ops.y=400;ops.mutate=true;ops.fail=failure;good=true;
            if(shot.endStepWithView(ops,good)||good||ops.calls!=failure)return false;
            if(shot.endStepWithView(ops,good)||ops.calls!=failure)return false;
        }
        return true;
    }
};
}
#endif
