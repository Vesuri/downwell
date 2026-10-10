#ifndef DOWNWELL_CORE_SHOP_VIEW_EVENTS_H
#define DOWNWELL_CORE_SHOP_VIEW_EVENTS_H
#include "RunnerRealComparison.h"
namespace downwell {
// Shop Step blocks 96..113, inside the !criminal branch, after interaction.
// This is NOT scrInView, which the preceding radio branch calls separately.
struct ShopViewEvents {
    template<class Ops> static bool step(bool& visible,bool& angry,Ops& ops){
        MotionBinary64 edge,extent;int order;
        auto position=ops.x();
        if(!ops.view(0,edge)||!compareRunnerReal(position,edge,order))return false;
        bool inside=order>0;
        if(inside){
            position=ops.x();
            if(!ops.view(0,edge)||!ops.view(2,extent)||!edge.add(extent)||
               !compareRunnerReal(position,edge,order))return false;
            inside=order<0;
        }
        if(inside){
            position=ops.y();
            if(!ops.view(1,edge)||!compareRunnerReal(position,edge,order))return false;
            inside=order>0;
        }
        if(inside){
            position=ops.y();
            if(!ops.view(1,edge)||!ops.view(3,extent)||!edge.add(extent)||
               !compareRunnerReal(position,edge,order))return false;
            inside=order<0;
        }
        if(inside){
            if(!visible){visible=true;if(!ops.sound(103))return false;}
        }else if(visible)visible=false;
        if(visible){
            MotionBinary64 player;
            if(!ops.playerX(player)||!compareRunnerReal(player,ops.keeperX(),order))return false;
            if(order>0){
                if(!angry){angry=true;if(!ops.sound(94))return false;}
            }else if(angry)angry=false;
        }
        return true;
    }
};
}
#endif
