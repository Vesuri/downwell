#ifndef DOWNWELL_CORE_SHOP_KEEPER_EVENTS_H
#define DOWNWELL_CORE_SHOP_KEEPER_EVENTS_H
#include "ShopChildCreate.h"
#include "RunnerRealComparison.h"
namespace downwell {
// shopKeeper Step 0. The owner supplies narrowed builtins, real globals and
// synchronous child creation. Destruction marks the retained caller; it does
// not terminate this event or suppress its remaining reads/writes.
struct ShopKeeperEvents {
    template<class Ops> static bool step(ShopChildState& state,Ops& ops){
        const MotionBinary64 idle{0x3fb70a3du,0x70a3d70au};
        const MotionBinary64 thanks{0x3fd99999u,0x9999999au};
        const MotionBinary64 reaction{0x3fc99999u,0x9999999au};
        auto playerOrder=[&](MotionBinary64 x,int& result){
            MotionBinary64 player;return ops.playerX(player)&&compareRunnerReal(player,x,result);
        };
        int order;
        if(!state.goodbye&&!state.thankful&&!state.confused){
            if(!playerOrder(ops.x(),order))return false;
            if(order<0){
                if(!ops.sprite(485)||!ops.speed(idle))return false;
            }else{
                if(ops.spriteIndex()!=487){
                    unsigned delay;
                    if(!ops.sprite(487)||!ops.image(RunnerImageValue{})||
                       !ops.range(20,60,delay)||!ops.alarm(0,int(delay)))return false;
                }
                if(!ops.speed(MotionBinary64{}))return false;
            }
            bool criminal;
            if(!ops.criminal(criminal))return false;
            if(criminal){
                if(!playerOrder(MotionBinary64::fromInteger(170),order))return false;
                if(order>0&&(!ops.destroy()||!ops.createEnemy(284,ops.x(),ops.y())))return false;
            }
        }else if(state.thankful==1){
            if(!ops.alarm(2,0)||!ops.speed(thanks)||!ops.sprite(486)||
               !compareRunnerReal(ops.imageIndex(),MotionBinary64::fromInteger(4),order))return false;
            if(order>0){
                if(!ops.speed(MotionBinary64{})||!ops.alarm(2,60))return false;
                state.thankful=2;
            }
            state.confused=0;
        }else if(state.goodbye==1){
            if(!ops.speed(reaction)||!ops.sprite(496)||!ops.alarm(2,240))return false;
            state.thankful=state.confused=0;state.goodbye=2;
        }else if(state.confused==1){
            if(!ops.speed(reaction)||!ops.sprite(497)||
               !compareRunnerReal(ops.imageIndex(),MotionBinary64::fromInteger(1),order))return false;
            if(order>0){
                if(!ops.speed(MotionBinary64{})||!ops.alarm(2,60))return false;
                state.confused=2;
            }
            state.thankful=0;
        }
        if(state.goodbye&&!ops.speed(reaction))return false;
        if(!playerOrder(ops.x(),order))return false;
        if(order>0)state.thankful=state.confused=state.goodbye=0;
        unsigned area;
        if(!ops.area(area))return false;
        if(area==3){
            MotionBinary64 half;
            if(!multiplyBinary64(ops.imageSpeed(),{0x3fe00000u,0},half)||!ops.speed(half))return false;
        }
        return true;
    }
};
}
#endif
