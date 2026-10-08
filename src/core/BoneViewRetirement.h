#ifndef DOWNWELL_CORE_BONE_VIEW_RETIREMENT_H
#define DOWNWELL_CORE_BONE_VIEW_RETIREMENT_H
#include "RunnerRealComparison.h"
namespace downwell {
// bulletEnmBone's scrInView(0,0,0). These are published instance and camera
// coordinates, not private motion accumulators or collision-mask bounds.
// The camera snapshot is supplied by the event phase, never followed here.
inline bool boneInPublishedView(MotionBinary64 x,MotionBinary64 y,
    MotionBinary64 left,MotionBinary64 top,MotionBinary64 width,MotionBinary64 height,bool& visible){
    int order;
    if(!compareRunnerReal(x,left,order))return false;
    if(order<=0){visible=false;return true;}
    auto right=left;
    if(!right.add(width)||!compareRunnerReal(x,right,order))return false;
    if(order>=0){visible=false;return true;}
    if(!compareRunnerReal(y,top,order))return false;
    if(order<=0){visible=false;return true;}
    auto bottom=top;
    if(!bottom.add(height)||!compareRunnerReal(y,bottom,order))return false;
    visible=order<0;return true;
}
}
#endif
