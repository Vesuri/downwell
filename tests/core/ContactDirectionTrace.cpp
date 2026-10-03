#include "core/RunnerImageValue.h"
#include <cstdio>
#include <cstdlib>
namespace downwell {
void traceContactDirection(std::int32_t px,std::int32_t py,int cx,int cy,int ox,int oy,
    MotionBinary64 dx,MotionBinary64 dy){
    RunnerImageValue x,y;
    if(!x.assign(dx)||!y.assign(dy))std::abort();
    std::printf("CONTACT_DIRECTION q8=%d,%d caller=%d,%d origin=%d,%d dx=%08x dy=%08x\n",
        int(px),int(py),cx,cy,ox,oy,unsigned(x.bits),unsigned(y.bits));
}
}
