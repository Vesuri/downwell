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
// The same test for a whole-pixel position, a whole view left and a Q8 view
// top (W3.20): values that differ differ by at least 1/256, beyond the
// runner's 1e-5 band, so Q8 integers decide exactly.
inline bool boneInPublishedViewQ8(int x,int y,int left,std::int32_t topQ8,int width,int height,bool& visible){
    constexpr int range=1<<22;
    if(x<=-range||x>=range||y<=-range||y>=range||left<=-range||left>=range||topQ8<=-(range<<8)||
       topQ8>=(range<<8)||width<0||width>=range||height<0||height>=range)return false;
    const std::int32_t yQ8=y*256;
    visible=x>left&&x<left+width&&yQ8>topQ8&&yQ8<topQ8+height*256;
    return true;
}
}
#endif
